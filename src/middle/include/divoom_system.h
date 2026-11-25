#ifndef _divoom_system_h_
#define _divoom_system_h_
#include "divoom_type.h"
#include "divoom_system_task_conf.h"


#define DIVOOM_SYSTEM_TASK_DEFAULT_PRIO		2
#define DIVOOM_MAIN_DIAL_TASK_PRIO		3
#define DIVOOM_SUB_DIAL_TASK_PRIO		2
#define DIVOOM_LVGL_HANDLER_TASK_PRIO		10
/* 冷开提示音播放期临时提权：低于 LVGL，避免 push 背压饿死 UI；供数靠 ring+sink */
#define DIVOOM_LOCAL_TONE_PLAY_TASK_PRIO	5
/* 电台解码高于 LVGL，在切表盘时仍能及时向 PCM 环形缓冲供数。 */
#define DIVOOM_NET_AUDIO_DECODE_TASK_PRIO	11
/* sink 必须高于 LVGL：复杂表盘 lv_timer_handler 常占 60ms+，
 * I2S DMA 只有 ~43ms，sink 插不进去时 ring 是满的、喇叭却没声。
 * writei 阻塞在 DMA 上，高优先级大部分时间在睡，不应拖死按键。
 */
#define DIVOOM_AUDIO_SINK_TASK_PRIO		12

#if DIVOOM_AUDIO_SINK_TASK_PRIO <= DIVOOM_LOCAL_TONE_PLAY_TASK_PRIO
#error audio_sink prio must be above local_tone play prio
#endif
#if DIVOOM_AUDIO_SINK_TASK_PRIO <= DIVOOM_LVGL_HANDLER_TASK_PRIO
#error audio_sink prio must stay above LVGL handler prio
#endif
#if DIVOOM_NET_AUDIO_DECODE_TASK_PRIO <= DIVOOM_LVGL_HANDLER_TASK_PRIO || \
    DIVOOM_NET_AUDIO_DECODE_TASK_PRIO >= DIVOOM_AUDIO_SINK_TASK_PRIO
#error network audio decode prio must be between LVGL and audio_sink
#endif

#define DIVOOM_SYSTEM_TASK_MODEL_MSG_PRIO	4
#define DIVOOM_PLAY_PICTURE_SEND_DATA_PRIO	6
#define DIVOOM_PLAY_PICTURE_READ_DATA_PRIO	3
#define DIVOOM_PLAY_MQTT_READ_DATA_PRIO	5
#define DIVOOM_PLAY_PICTURE_DISP_CTRL_PRIO	7
#define DIVOOM_MIC_CTRL_PRIO				8

//#define DIVOOM_SCREEN_QUEUE_TASK_PRIO	9



typedef void* DIVOOM_TASK_CALLBACK(void *param);

void *divoom_system_create_sem(int max_count, int init_count);
void divoom_system_get_sem(void *handle);
/*********************************************************************
  Function:        divoom_system_get_sem_with_time
  Description:    带超时获取信号量
  Input:            
  Return:           NONE
**********************************************************************/

int divoom_system_get_sem_with_time(void *handle, int time_s);

void divoom_system_put_sem(void *handle);
int divoom_system_put_sem_fromisr(void *handle);
int divoom_system_get_sem_by_time(void *handle, int time);
void divoom_system_destroy_sem(void *handle);
void *divoom_system_create_lock(void);
void divoom_system_get_lock_inner(void *handle, const char *func_name, int line);
#define divoom_system_get_lock(handle) divoom_system_get_lock_inner(handle, __func__,  __LINE__)
/*********************************************************************
  Function:        divoom_system_get_lock_direct
  Description:     获取锁,直接获取，不检查
  Input:            
  Return:           NONE
**********************************************************************/

void divoom_system_get_lock_direct(void *handle);


/*********************************************************************
  Function:        divoom_system_free_lock
  Description:    释放锁
  Input:            
  Return:           NONE
**********************************************************************/

void divoom_system_free_lock_direct(void *handle);


/*********************************************************************
  Function:        divoom_system_try_lock
  Description:    获取锁
  Input:            
  Return:           NONE
**********************************************************************/

int divoom_system_try_lock(void *handle);

void divoom_system_free_lock(void *handle);
void divoom_system_destroy_lock(void *handle);
//void *divoom_system_malloc(int size);
#define	divoom_system_malloc(size) divoom_malloc(size, DIVOOM_MODEL_MAIN, __LINE__, __FILE__)
#define	divoom_system_malloc_no_zero(size) divoom_malloc_raw(size, DIVOOM_MODEL_MAIN, __LINE__, __FILE__, 0)
#define divoom_system_malloc_out(size) divoom_system_malloc(size)
//void divoom_system_free(void *ptr);
void divoom_system_pause_task(void *handle);
void divoom_system_run_task(void *handle);
void divoom_system_destroy_task(void *handle);
uint32 divoom_system_get_tickcount(void);
uint32 divoom_system_get_tickms(void);
uint32 divoom_system_get_ticks(void);
uint32 divoom_system_get_utc_time(void);
void divoom_system_init_system_time(void);
uint32 divoom_system_set_interva_ms(uint32 time);
void divoom_system_delay_ms(uint32 time);
void divoom_system_yield(void);
int divoom_system_task_get_current_prio(void);
void divoom_system_task_set_current_prio(int prio);
void divoom_system_task_set_prio(void *task, int prio);
long divoom_system_get_ram_free_size(void);
long divoom_system_get_ram_total_size(void);
long divoom_system_get_ram_largest_free_block_size(void);
void divoom_system_print_ram_info(const char *file_name, int file_line);

/*********************************************************************
  Function:        divoom_system_create_new_task
  Description:    创建任务
  Input:            func  任务主函数
                      para  任务参数
  Return:          任务句柄
**********************************************************************/

void *divoom_system_create_new_task(DIVOOM_TASK_CALLBACK func, void *para, const char *func_name, int line);

/*********************************************************************
  Function:        divoom_system_create_task
  Description:    创建任务（可配置栈大小，默认优先级）
  Input:            func       任务主函数
                    task_name  任务名称
                    para       任务参数
                    stack_size 栈大小（字节）
  Return:          任务句柄
**********************************************************************/

void *divoom_system_create_task(DIVOOM_TASK_CALLBACK func, const char *task_name, void *para, int stack_size);

/*********************************************************************
  Function:        divoom_system_create_task_prio
  Description:    创建任务（可配置栈大小与优先级）
  Input:            func       任务主函数
                    task_name  任务名称
                    para       任务参数
                    stack_size 栈大小（字节）
                    prio       任务优先级
  Return:          任务句柄
**********************************************************************/

void *divoom_system_create_task_prio(DIVOOM_TASK_CALLBACK func, const char *task_name, void *para, int stack_size, int prio);
void *divoom_system_create_joinable_task(DIVOOM_TASK_CALLBACK func,
    const char *task_name, void *para, int stack_size);
int divoom_system_join_task(void *handle);
void *divoom_system_thread_self(void);
int divoom_system_thread_equal(void *a, void *b);

/*********************************************************************
  Function:        divoom_system_destroy_lock
  Description:    销毁锁
  Input:            
  Return:           NONE
**********************************************************************/

void divoom_system_reboot();

#endif
