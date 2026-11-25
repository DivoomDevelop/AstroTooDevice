#include "lvgl/lvgl.h"
#include "lv_drivers/display/sunxifb.h"
#include "lv_drivers/indev/evdev.h"
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>


pthread_mutex_t lvgl_mutex;

void divoom_main(void);

int divoom_get_lvgl_try_lock(void)
{
    return pthread_mutex_trylock(&lvgl_mutex);
}

void divoom_get_lvgl_lock(void)
{
    pthread_mutex_lock(&lvgl_mutex);
}

void divoom_free_lvgl_lock(void)
{
    pthread_mutex_unlock(&lvgl_mutex);
}

void divoom_main_check_run_log(int flag);
void divoom_sigterm_handler(int sig);
void divoom_user_key_init(void);
void divoom_user_key_read(lv_indev_drv_t *drv, lv_indev_data_t *data);

void divoom_main_lvgl_print(const char * buf)
{
    printf("%s",buf);
}

void divoom_set_mirror_mode(int mirror_flag)
{
    if(mirror_flag)
    {
        sunxifb_update_rotated(LV_DISP_ROT_H);
    }
    else
    {
        sunxifb_update_rotated(LV_DISP_ROT_NONE);
    }
}

static int redirect_stdio_to_console(void)
{
	int fd;

	fd = open("/dev/console", O_RDWR);
	if (fd < 0) {
		perror("open /dev/console");
		return -1;
	}

	/* 让 stdin 也可用控制台（按需；很多程序不需要就把下面两行去掉） */
	if (dup2(fd, STDERR_FILENO) < 0)
		goto err;

	if (fd > STDERR_FILENO)
		close(fd);

	return 0;
err:
	perror("dup2");
	if (fd >= 0)
		close(fd);
	return -1;
}

int main(int argc, char *argv[]) {
    lv_disp_drv_t disp_drv;
    lv_disp_draw_buf_t disp_buf;
    lv_indev_drv_t indev_drv;
    uint32_t rotated = LV_DISP_ROT_NONE;

	redirect_stdio_to_console();
	signal(SIGTERM, divoom_sigterm_handler);
    signal(SIGPWR, divoom_sigterm_handler);
    signal(SIGABRT, divoom_sigterm_handler);
    signal(SIGSEGV, divoom_sigterm_handler);
    signal(SIGINT, divoom_sigterm_handler);


    divoom_main_check_run_log(1);
    
        
    pthread_mutex_init(&lvgl_mutex, NULL);
        

    lv_disp_drv_init(&disp_drv);
        

    /*LittlevGL init*/
    lv_init();
        
    //lv_log_register_print_cb(divoom_main_lvgl_print);

    /*Linux frame buffer device init*/
    sunxifb_init(rotated);
        

    /*A buffer for LittlevGL to draw the screen's content*/
    static uint32_t width, height;
    sunxifb_get_sizes(&width, &height);
        

    static lv_color_t *buf;
    buf = (lv_color_t*) sunxifb_alloc(width * height * sizeof(lv_color_t),
            "lv_g2d_test");

    if (buf == NULL) {
        sunxifb_exit();
        printf("malloc draw buffer fail\n");
        return 0;
    }
        

    /*Initialize a descriptor for the buffer*/
    lv_disp_draw_buf_init(&disp_buf, buf, NULL, width * height);
        

    /*Initialize and register a display driver*/
    disp_drv.draw_buf = &disp_buf;
    disp_drv.flush_cb = sunxifb_flush;
    disp_drv.hor_res = width;
    disp_drv.ver_res = height;
    disp_drv.rotated = rotated;
    disp_drv.screen_transp = 0;
    lv_disp_drv_register(&disp_drv);
        

    //evdev_init();
    divoom_user_key_init();
    lv_indev_drv_init(&indev_drv); /*Basic initialization*/
    indev_drv.type = LV_INDEV_TYPE_KEYPAD; /*See below.*/
    indev_drv.read_cb = divoom_user_key_read; /*See below.*/
    /*Register the driver in LVGL and save the created input device object*/
    lv_indev_t *evdev_indev = lv_indev_drv_register(&indev_drv);
        

    divoom_main();
       

    /*Handle LitlevGL tasks (tickless mode)*/
    while (1) {
        divoom_get_lvgl_lock();
        lv_task_handler();
        divoom_free_lvgl_lock();
        divoom_port_delay_ms((1000) / 1000U); 
    }

    /*sunxifb_free((void**) &buf, "lv_g2d_test");*/
    /*sunxifb_exit();*/
    return 0;
}

/*Set in lv_conf.h as `LV_TICK_CUSTOM_SYS_TIME_EXPR`*/
uint32_t custom_tick_get(void) {
    static uint64_t start_ms = 0;
    if (start_ms == 0) {
        struct timeval tv_start;
        gettimeofday(&tv_start, NULL);
        start_ms = ((uint64_t) tv_start.tv_sec * 1000000
                + (uint64_t) tv_start.tv_usec) / 1000;
    }

    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    uint64_t now_ms;
    now_ms = ((uint64_t) tv_now.tv_sec * 1000000 + (uint64_t) tv_now.tv_usec)
            / 1000;

    uint32_t time_ms = now_ms - start_ms;
    return time_ms;
}
