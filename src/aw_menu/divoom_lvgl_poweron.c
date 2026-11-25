#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_main.h"
#include "divoom_lvgl_poweron.h"
#include "divoom_bt_music.h"
#include "divoom_jl_amp_adapter.h"

extern const lv_img_dsc_t divoom_boot_logo_img;

#define DIVOOM_LVGL_POWER_ON_TEST_FILE_NAME "/usr/share/divoom_app/poweron_movie/test.mp4"
/* 开机源视频 12 秒，额外预留 3 秒启动和调度时间。 */
#define DIVOOM_LVGL_POWER_ON_TIMEOUT_MS 15000u
/* 老化测试视频连续播放满 8 小时后静音 */
#define DIVOOM_STRESS_TEST_MUTE_AFTER_MS    (8u * 3600u * 1000u)

typedef struct _divoom_lvgl_poweron_ctrl
{
    int menu_id;
    void *main_obj;
    void *image_obj;
    void *play_bg_handle;

	int time_tick;
	uint32_t stress_play_start_ms;
	uint8 stress_volume_muted;
	divoom_channel_set_rgb_info rgb_info;
	lv_timer_t *rgb_timer;
	lv_timer_t *boot_timer;
	uint32_t boot_start_ms;
	uint8 boot_timeout_requested;
}divoom_lvgl_poweron_ctrl, *Pdivoom_lvgl_poweron_ctrl;
static Pdivoom_lvgl_poweron_ctrl pdivoom_lvgl_poweron_handle = NULL;
static int s_poweron_video_phase = 0;
static int s_boot_video_exited_notified = 0;
static int s_boot_av_finished = 0;

int divoom_lvgl_poweron_is_video_phase(void)
{
	return s_poweron_video_phase;
}

int divoom_lvgl_poweron_will_play_video(void)
{
	if (divoom_main_get_poweron_mode() == DIVOOM_ONOFF_MODE_CLOCK)
		return 0;
#if !DIVOOM_BUILD_RTOS_DEVICE
	if (divoom_main_is_app_exit_flag())
		return 0;
#endif
	return (divoom_port_file_exists((char *)DIVOOM_LVGL_POWER_ON_FILE_NAME)) ? 1 : 0;
}

int divoom_lvgl_poweron_is_boot_pending(void)
{
	/* 产测无开机动画，勿因 power_on.mp4 存在而永久 pending */
	if (divoom_product_test_is_active())
		return 0;
	if (s_boot_av_finished)
		return 0;
	if (s_poweron_video_phase)
		return 1;
	return divoom_lvgl_poweron_will_play_video() ? 1 : 0;
}

int divoom_lvgl_poweron_should_defer_connect_tone(void)
{
	return divoom_lvgl_poweron_is_boot_pending();
}

void divoom_lvgl_poweron_notify_jl_audio_ready(void)
{
	/* 跳过动画也要结束 pending，否则 CLOCK 变 NORMAL 后 0x19 会被当成「视频未播完」一直 defer */
	s_boot_video_exited_notified = 1;
	s_boot_av_finished = 1;
	s_poweron_video_phase = 0;
	divoom_power_monitor_set_boot_complete();
#if defined(DIVOOM_TIME_FRAME_SPEAKER) && defined(DIVOOM_TIMES_VIBE)
	if (divoom_btcore_uart_cmd_try_notify_aic_host_audio_ready() != 0)
		divoom_print("poweron: jl aic host audio ready notify failed\n");
	divoom_btcore_uart_cmd_try_pending_poweroff();
#endif
}

void divoom_lvgl_poweron_notify_boot_ready(void)
{
	divoom_lvgl_poweron_notify_jl_audio_ready();
	divoom_jl_release_boot_audio_gates_once();
}

void divoom_lvgl_poweron_on_boot_video_exited(void)
{
	if (s_boot_video_exited_notified)
		return;
	divoom_print("poweron: boot video exited\n");
	divoom_lvgl_poweron_notify_boot_ready();
}

static void divoom_lvgl_poweron_play_deferred_connect_tone(void)
{
	divoom_bt_music_play_deferred_connect_tone_if_any();
}

/* 与开机视频同启；复位开机、定时开机不播；产测老化长视频不播 */
static void divoom_lvgl_poweron_play_boot_tone(void)
{
	char tone_path[128];

	if (divoom_main_get_poweron_mode() == DIVOOM_ONOFF_MODE_RESET)
		return;
	if (divoom_main_get_poweron_mode() == DIVOOM_ONOFF_MODE_POWER_SCHEDULE)
		return;
	if (divoom_product_test_is_active())
		return;
	memset(tone_path, 0, sizeof(tone_path));
	if (!divoom_voice_read_get_filename(DIVOOM_VOICE_ID_POWER_ON, tone_path))
		return;
	divoom_print("poweron: sync play boot tone %s\n", tone_path);
	divoom_local_music_ctrl_tone_play(tone_path, divoom_main_get_tone_volume(), NULL, 0);
}

static void divoom_lvgl_poweron_stress_test_mute_volume(Pdivoom_lvgl_poweron_ctrl phandle)
{
	if (!phandle || phandle->stress_volume_muted)
		return;

	phandle->stress_volume_muted = 1;
	divoom_local_movie_play_set_movie_volume(0);
#if defined(DIVOOM_TIME_FRAME_SPEAKER) && defined(DIVOOM_TIMES_VIBE)
	divoom_main_set_jl_mcu_tone_volume(0);
#else
	divoom_main_set_sys_vol(0);
#endif
	divoom_print("stress test: mute volume after %u hours\n", DIVOOM_STRESS_TEST_MUTE_AFTER_MS / 3600000u);
}

static void divoom_lvgl_poweron_stress_test_check_mute(Pdivoom_lvgl_poweron_ctrl phandle)
{
	uint32_t now_ms;
	uint32_t elapsed_ms;

	if (!phandle || phandle->stress_volume_muted || phandle->stress_play_start_ms == 0)
		return;

	now_ms = divoom_system_get_tickms();
	elapsed_ms = now_ms - phandle->stress_play_start_ms;
	if (elapsed_ms >= DIVOOM_STRESS_TEST_MUTE_AFTER_MS)
		divoom_lvgl_poweron_stress_test_mute_volume(phandle);
}

void divoom_lvgl_power_on_product_stress_testing_menu_time(lv_timer_t *noise_timer)
{
	int time;
	Pdivoom_lvgl_poweron_ctrl phandle = (Pdivoom_lvgl_poweron_ctrl)lv_timer_get_user_data(noise_timer);

	divoom_lvgl_poweron_stress_test_check_mute(phandle);

	time = phandle->time_tick % 3;
	phandle->time_tick++;
	if(phandle->time_tick < 0)
	{
		phandle->time_tick = 0;
	}
	
	if(time == 0)	//R
	{
		phandle->rgb_info.rgb_light_list[0].select_effect = DIVOOM_DISP_DRAW_CTRL_SIGNAL_COLOR - DIVOOM_DISP_DRAW_CTRL_MULTICOLOR_END -1;
		phandle->rgb_info.brightness = 100;
		phandle->rgb_info.color[0] = 0xFF;
		phandle->rgb_info.color[1] = 0;
		phandle->rgb_info.color[2] = 0;
		phandle->rgb_info.color_cyc_flag = 0;
		divoom_disp_ambient_ctrl_set_disp_info(&phandle->rgb_info, 1);
	}
	else if(time == 1)	//G
	{
		phandle->rgb_info.rgb_light_list[0].select_effect = DIVOOM_DISP_DRAW_CTRL_SIGNAL_COLOR - DIVOOM_DISP_DRAW_CTRL_MULTICOLOR_END -1;
		phandle->rgb_info.brightness = 100;
		phandle->rgb_info.color[0] = 0;
		phandle->rgb_info.color[1] = 0xFF;
		phandle->rgb_info.color[2] = 0;
		phandle->rgb_info.color_cyc_flag = 0;
		divoom_disp_ambient_ctrl_set_disp_info(&phandle->rgb_info, 1);
	}
	else	//B
	{
		phandle->rgb_info.rgb_light_list[0].select_effect = DIVOOM_DISP_DRAW_CTRL_SIGNAL_COLOR - DIVOOM_DISP_DRAW_CTRL_MULTICOLOR_END -1;
		phandle->rgb_info.brightness = 100;
		phandle->rgb_info.color[0] = 0;
		phandle->rgb_info.color[1] = 0;
		phandle->rgb_info.color[2] = 0xFF;
		phandle->rgb_info.color_cyc_flag = 0;
		divoom_disp_ambient_ctrl_set_disp_info(&phandle->rgb_info, 1);
	}
}


extern void divoom_boot_logo_setup(lv_obj_t *parent, lv_obj_t *logo_img);

static void divoom_lvgl_poweron_clear_logo(Pdivoom_lvgl_poweron_ctrl phandle)
{
	lv_obj_t *logo_img;

	if (phandle == NULL || phandle->image_obj == NULL)
		return;

	logo_img = (lv_obj_t *)phandle->image_obj;
	lv_obj_del(logo_img);
	phandle->image_obj = NULL;
}

static void divoom_lvgl_poweron_stop_boot_timer(Pdivoom_lvgl_poweron_ctrl phandle)
{
	if (phandle->boot_timer)
	{
		lv_timer_del(phandle->boot_timer);
		phandle->boot_timer = NULL;
	}
}

void *divoom_lvgl_poweron_init(void *user_data, int menu_id)
{
    Pdivoom_lvgl_poweron_ctrl phandle;
    int has_poweron_video;

    
    phandle = divoom_system_malloc(sizeof(divoom_lvgl_poweron_ctrl));
    phandle->menu_id = menu_id;
	phandle->boot_timer = NULL;
    phandle->boot_timeout_requested = 0;
    pdivoom_lvgl_poweron_handle = phandle;
	has_poweron_video = divoom_port_file_exists((char *)DIVOOM_LVGL_POWER_ON_FILE_NAME) ? 1 : 0;
	s_poweron_video_phase = has_poweron_video;
    phandle->main_obj = lv_obj_create(NULL);
    lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_clear_flag(phandle->main_obj, LV_OBJ_FLAG_SCROLLABLE);
	phandle->image_obj = NULL;
	if (!has_poweron_video) {
		phandle->image_obj = lv_img_create(phandle->main_obj);
		if (phandle->image_obj != NULL) {
			divoom_boot_logo_setup(phandle->main_obj, phandle->image_obj);
			lv_img_set_src(phandle->image_obj, &divoom_boot_logo_img);
			lv_obj_center(phandle->image_obj);
		} else {
			divoom_boot_logo_setup(phandle->main_obj, NULL);
		}
	} else {
		divoom_boot_logo_setup(phandle->main_obj, NULL);
	}
	if(DIVOOM_MENU_CONTROL_ID_PRODUCT_STRESS_TESTING_MENU == phandle->menu_id)
    {
    	phandle->time_tick = 0;
		phandle->stress_play_start_ms = 0;
		phandle->stress_volume_muted = 0;
		memset(&phandle->rgb_info, 0, sizeof(phandle->rgb_info));
    	phandle->rgb_timer = lv_timer_create(divoom_lvgl_power_on_product_stress_testing_menu_time, 2000, phandle);
	}
    return phandle;
}
int divoom_lvgl_poweron_disp_play_over(void *user_data, int status)
{
    int next_menu_id;
    Pdivoom_lvgl_poweron_ctrl phandle = (Pdivoom_lvgl_poweron_ctrl)user_data;    

    if(phandle == NULL || pdivoom_lvgl_poweron_handle != phandle)
    {
        //异步结束，可能会导致死机异常
        return 1;
    }
    if(status == 0 || !s_poweron_video_phase)
    {
        return 1;
    }
	divoom_lvgl_poweron_clear_logo(phandle);
	s_poweron_video_phase = 0;
	divoom_lvgl_poweron_on_boot_video_exited();
    next_menu_id = divoom_menu_control_menu_get_first_menu_id();
    if(DIVOOM_MENU_CONTROL_ID_PRODUCT_STRESS_TESTING_MENU == phandle->menu_id)
    {
        divoom_menu_control_return_pre();
    }
    else
    {
        if(DIVOOM_MENU_CONTROL_ID_MAIN == next_menu_id || DIVOOM_MENU_CONTROL_ID_SYSTEM_POWER_ON == next_menu_id)
        {
            //显示表盘
            divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP ,DIVOOM_LIGHT_APP_EVENT_START_UP_DISP_DIAL);
        
        }
        else
        {
            divoom_menu_control_load_next_pop_cur(next_menu_id, NULL, 0, 0);    
        }
    }
	divoom_lvgl_poweron_play_deferred_connect_tone();
    //开机动画播放完了，再初始化WIFI模块
    return 1;
}

static void divoom_lvgl_poweron_boot_timer(lv_timer_t *timer)
{
	Pdivoom_lvgl_poweron_ctrl phandle = lv_timer_get_user_data(timer);
	uint32_t now_ms;

	if (phandle == NULL || phandle != pdivoom_lvgl_poweron_handle)
	{
		return;
	}
	if (!s_poweron_video_phase)
	{
		divoom_lvgl_poweron_stop_boot_timer(phandle);
		return;
	}
	/* 蓝牙对时会把 tick 纪元清零。起点仍是旧值时无符号相减会下溢，误判超时并停片。 */
	now_ms = divoom_system_get_tickms();
	if (now_ms < phandle->boot_start_ms)
		phandle->boot_start_ms = now_ms;
	if ((uint32_t)(now_ms - phandle->boot_start_ms) <
		DIVOOM_LVGL_POWER_ON_TIMEOUT_MS)
		return;

	if (!phandle->boot_timeout_requested)
	{
		phandle->boot_timeout_requested = 1;
		divoom_print("poweron: video timeout after %u ms; stop boot video\n",
			DIVOOM_LVGL_POWER_ON_TIMEOUT_MS);
	}
	/* 只停止本菜单的视频；不在 LVGL 锁内阻塞等待播放器的结束回调。
	 * 返回 0 时通道及回调均已退出，再补走正常菜单切换流程。 */
	if (divoom_local_movie_play_stop_owner(phandle, 0) != 0)
		return;
	divoom_lvgl_poweron_stop_boot_timer(phandle);
	if (s_poweron_video_phase)
		divoom_lvgl_poweron_disp_play_over(phandle, 1);
}

extern void divoom_boot_logo_discard_early(void);
void divoom_lvgl_poweron_disp(void *handle, void *active_obj)
{
    Pdivoom_lvgl_poweron_ctrl phandle = (Pdivoom_lvgl_poweron_ctrl)handle;

	divoom_boot_logo_discard_early();
    lv_scr_load(phandle->main_obj);
	if (divoom_port_file_exists((char *)DIVOOM_LVGL_POWER_ON_FILE_NAME))
	{
		divoom_lvgl_poweron_clear_logo(phandle);
		lv_refr_now(lv_disp_get_default());
        divoom_menu_control_change_disp_mode(phandle->main_obj, 1);
        if(DIVOOM_MENU_CONTROL_ID_PRODUCT_STRESS_TESTING_MENU == phandle->menu_id)
        {
            phandle->stress_play_start_ms = divoom_system_get_tickms();
            phandle->stress_volume_muted = 0;
            if (divoom_port_file_exists((char *)DIVOOM_LVGL_POWER_ON_TEST_FILE_NAME))
            {
                divoom_local_movie_play_play(DIVOOM_LVGL_POWER_ON_TEST_FILE_NAME, divoom_main_get_tone_volume()*20/100, 1209600,divoom_lvgl_poweron_disp_play_over, phandle);
            }
            else
            {
                divoom_local_movie_play_play(DIVOOM_LVGL_POWER_ON_FILE_NAME, divoom_main_get_tone_volume()*20/100, 1209600,divoom_lvgl_poweron_disp_play_over, phandle);
            }
        }
        else
        {
            divoom_lvgl_poweron_stop_boot_timer(phandle);
            phandle->boot_start_ms = divoom_system_get_tickms();
            phandle->boot_timeout_requested = 0;
            phandle->boot_timer = lv_timer_create(divoom_lvgl_poweron_boot_timer, 100, phandle);
            if (phandle->boot_timer == NULL)
            {
                divoom_print("poweron: boot timer create failed, skip video\n");
                divoom_menu_control_change_disp_mode(phandle->main_obj, 0);
                divoom_lvgl_poweron_disp_play_over(phandle, 1);
                return;
            }
            divoom_local_movie_play_play(DIVOOM_LVGL_POWER_ON_FILE_NAME, divoom_main_get_tone_volume()*60/100, 0,divoom_lvgl_poweron_disp_play_over, phandle);
            divoom_lvgl_poweron_play_boot_tone();
        }
        
	}
    else
    {
        int next_menu_id;

        next_menu_id = divoom_menu_control_menu_get_first_menu_id();
        if(DIVOOM_MENU_CONTROL_ID_MAIN == next_menu_id || DIVOOM_MENU_CONTROL_ID_SYSTEM_POWER_ON == next_menu_id)
        {
            //显示表盘
            divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP ,DIVOOM_LIGHT_APP_EVENT_START_UP_DISP_DIAL);
        
        }
        else
        {
            divoom_menu_control_load_next_pop_cur(next_menu_id, NULL, 0, 0);    
        }
        divoom_menu_control_load_next_pop_cur(next_menu_id, NULL, 0, 0);
        divoom_lvgl_poweron_on_boot_video_exited();
        divoom_lvgl_poweron_play_deferred_connect_tone();
    }
}   

void divoom_lvgl_poweron_pause(void *handle)
{
    
    Pdivoom_lvgl_poweron_ctrl phandle = (Pdivoom_lvgl_poweron_ctrl)handle;
    divoom_lvgl_poweron_stop_boot_timer(phandle);
    
    /* 播完后的开机页延迟销毁时，新表盘可能已接管全局播放器和透明图层。 */
    if (s_poweron_video_phase)
    {
        s_poweron_video_phase = 0;
        divoom_local_movie_play_stop_movie();
        lv_refr_now(lv_disp_get_default());
        divoom_menu_control_change_disp_mode(phandle->main_obj, 0);
    }
}

void divoom_lvgl_poweron_exit(void *handle)
{
    
    Pdivoom_lvgl_poweron_ctrl phandle = (Pdivoom_lvgl_poweron_ctrl)handle;
    divoom_lvgl_poweron_stop_boot_timer(phandle);

	/*
	 * CLEAR_ALL_LOAD / 切表盘会 pause 停片再 destroy。
	 * disp_play_over(status==0) 不结束 pending；若不在此补通知，
	 * is_boot_pending 会因 power_on.mp4 仍存在而一直为 1，音量 OSD/连接提示音永久被挡。
	 * on_boot_video_exited 幂等：正常播完路径已通知过则直接 return。
	 */
	divoom_lvgl_poweron_on_boot_video_exited();
	divoom_lvgl_poweron_play_deferred_connect_tone();

    pdivoom_lvgl_poweron_handle = NULL;
	if(phandle->rgb_timer)
	{
		lv_timer_del(phandle->rgb_timer);
    }
    lv_obj_del(phandle->main_obj);
    divoom_system_free(phandle);
}
