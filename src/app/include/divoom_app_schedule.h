#ifndef _divoom_app_schedule_h_
#define _divoom_app_schedule_h_
//当模块在执行，被切换到后面时，将会调用这个函数，如果返回非0,将直接从队列中删除
typedef uint8 DIVOOM_APP_SCHEDULE_MODEL_PAUSE_EVENT_CALLBACK(uint8 event_id, void *user_data, uint8 active_model, uint8 active_event, uint32 resouce_type);
//当模块从后台被激活，将会调用这个函数
typedef uint8 DIVOOM_APP_SCHEDULE_MODEL_PLAY_EVENT_CALLBACK(uint8 event_id, void *user_data, uint32 resouce_type);
//当模块达到条件，将要卸载时，将会调用这个函数
typedef uint8 DIVOOM_APP_SCHEDULE_MODEL_DESTROY_EVENT_CALLBACK(uint8 event_id, void *user_data, uint32 status, uint32 resouce_type);
//按键事件
typedef uint8 DIVOOM_APP_SCHEDULE_MODEL_KEY_EVENT_CALLBACK(uint8 key_id, uint8 key_type, uint32 resouce_type);


enum
{
    DIVOOM_APP_SCHEDULE_RESOURCE_VOICE, //提示音资源
    DIVOOM_APP_SCHEDULE_RESOURCCE_RGB,   //RGB灯资源
    DIVOOM_APP_SCHEDULE_RESOURCCE_WHITE,   //白灯资源
    DIVOOM_APP_SCHEDULE_RESOURCCE_END,   //资源个数
};     //DIVOOM资源请求类型
    

enum
{
    DIVOOM_APP_SCHEDULE_ATTR_CANCEL, //如果碰到高优化级的事件，将直接取消
    DIVOOM_APP_SCHEDULE_ATTR_WAIT,   //如果碰到高优化级的事件，将直接等待
};
enum
{
    DIVOOM_APP_SCHEDULE_CANCEL_FAIL,        //事件不存在
    DIVOOM_APP_SCHEDULE_CANCEL_OK_RUN,      //事件在激活状态
    DIVOOM_APP_SCHEDULE_CANCEL_OK_QUEUE,    //事件在队列中
};
    
enum
{
    DIVOOM_APP_SCHEDULE_RET_WAIT,           //请求暂时不能被执行
    DIVOOM_APP_SCHEDULE_RET_RUN,            //请求可以立刻执行
};
typedef struct _divoom_app_schedule_item_info
{
    uint8 model_id;
    uint8 event_id;
    uint8 attr;         //申请的事件是否等待执行，
    uint8 destroy_flag; //当PUSH时，已经有这个事件了，是否需要调用DESTROY通知应用
    uint8 resource_type;//申请的资源ID,可以多个资源或,DIVOOM_APP_SCHEDULE_RESOURCE_TYPE
    void *user_data;
}divoom_app_schedule_item_info, *Pdivoom_app_schedule_item_info;

typedef struct _divoom_app_schedule_model_info
{
    uint8 model_id;
    DIVOOM_APP_SCHEDULE_MODEL_PLAY_EVENT_CALLBACK *play_callback;
    DIVOOM_APP_SCHEDULE_MODEL_PAUSE_EVENT_CALLBACK *pause_callback;
    DIVOOM_APP_SCHEDULE_MODEL_DESTROY_EVENT_CALLBACK *destroy_callback;
	DIVOOM_APP_SCHEDULE_MODEL_KEY_EVENT_CALLBACK *key_callback;
}divoom_app_schedule_model_info, *Pdivoom_app_schedule_model_info;


/**
 * *  @brief       	divoom_app_schedule_init
 				事件模块初始化
 * *  @author      	aj
 * *  @date       	2016-02-24
 * *  @param[in]   none
				
 * *  @return        void
 * */

void divoom_app_schedule_init(void);


/**
 * *  @brief       	divoom_app_schedule_add_model
 				增加模块的回调，如果没有增加，事件将不会被管理
 * *  @author      	aj
 * *  @date       	2016-02-24
 * *  @param[in]   model_info  模块相应的回调
				
 * *  @return        void
 * */

void divoom_app_schedule_add_model(Pdivoom_app_schedule_model_info model_info);



/**
 * *  @brief       	divoom_app_schedule_get_active_model
 				返回当前激活的模块,如果没有事件，默认返回音乐
 * *  @author      	aj
 * *  @date       	2016-02-24
 * *  @param[in]   NONE
				
 * *  @return        返回当前事件的运行状态
 * */
uint8 divoom_app_schedule_get_active_model(uint8 *event_id,uint8 res_type);


/**
 * *  @brief       	divoom_app_schedule_push
 				增加一个事件
 * *  @author      	aj
 * *  @date       	2016-02-24
 * *  @param[in]   pevent_info  事件相关的数据
				
 * *  @return        返回当前事件的运行状态
 * */

uint32 divoom_app_schedule_push(Pdivoom_app_schedule_item_info pevent_info);



/**
 * *  @brief       	divoom_app_schedule_cancel
 				取消某个事件
 * *  @author      	aj
 * *  @date       	2016-02-24
 * *  @param[in]   pevent_info  事件相关信息
				
 * *  @return        TRUE:是成功；FALSE:不成功
 * */

uint8 divoom_app_schedule_cancel(Pdivoom_app_schedule_item_info pevent_info);

/** 下一次 cancel 释放活跃资源时不自动 start_new_event（一次性） */
void divoom_app_schedule_set_suppress_resume(uint8 suppress);

uint8 divoom_app_schedule_key_handle_event(uint8 key_id, uint8 key_type);


/**
 * *  @brief       	divoom_app_schedule_main
 				事件主循环，主要为超时任务做检查
 * *  @author      	aj
 * *  @date       	2016-02-24
 * *  @param[in]   none
				
 * *  @return        none
 * */
 

//void divoom_app_schedule_main(void *arg);

#endif

