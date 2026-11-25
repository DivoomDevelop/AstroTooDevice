#ifndef _divoom_system_msg_notice_h_
#define _divoom_system_msg_notice_h_


/*********************************************************************
  Function:        divoom_system_msg_notice_init
  Description:     初始化
  Input:           

  Return:          none
**********************************************************************/

void divoom_system_msg_notice_init(void);


/*********************************************************************
  Function:        divoom_system_msg_notice_clear_all
  Description:     取消所有的弹窗信息
  Input:           

  Return:          none
**********************************************************************/

void divoom_system_msg_notice_clear_all(int lvgl_lock);


/*********************************************************************
  Function:        divoom_system_msg_notice_clear_one
  Description:     取消指定的弹窗信息
  Input:           

  Return:          none
**********************************************************************/

void divoom_system_msg_notice_clear_one(void *lvgl_obj, int lvgl_lock);

//result: 1:执行，0：移除（事件太多被害人取消，或者选择了某个其它的也全部取消）
//返回值为1：所有事件全部清除，返回为0，只清除当前事件
typedef int divoom_system_msg_notice_callback(void *para, int result);
void *divoom_system_msg_notice_add(int width, int high, divoom_system_msg_notice_callback *pcallback, void *para);

/** 在默认按键处理前先调用；返回非 0 表示已拦截该按键（不向其它 case 派发） */
typedef int divoom_system_msg_notice_pre_key_cb(void *para, char key0);

void divoom_system_msg_notice_set_pre_key_cb(void *lvgl_obj, divoom_system_msg_notice_pre_key_cb *pre_key_cb);

/** 等价于用户对当前条按下 ENTER（与 pcallback(...,callback_result) 及关闭语义一致）；须在 LVGL 线程 */
void divoom_system_msg_notice_deliver_item_result(void *lvgl_obj, int callback_result);

/** 事件列表重建后更新弹窗回调参数，避免 para 指向已释放的旧事件 */
void divoom_system_msg_notice_update_para(void *lvgl_obj, void *para);

#endif


