#ifndef _divoom_popup_menu_h_
#define _divoom_popup_menu_h_
typedef void DIVOOM_POPUP_MENU_CANCEL_CALLBACK(void *para);
typedef int DIVOOM_POPUP_MENU_KEY_CALLBACK(void *para, int key_id);

/*********************************************************************
  Function:        divoom_popup_menu_init
  Description:     初始化弹窗模块
  Input:           none
  Return:          NONE
**********************************************************************/

void divoom_popup_menu_init(void);

/*********************************************************************
  Function:        divoom_popup_menu_create
  Description:     创建弹窗提醒
  Input:           width
                   high
				   opa 	透明度 0x0~0xFF

                   pexit_callbck
                   user_data
                   pkey_callback
                   main_obj         父窗口句柄
                   back_flag        是否增加透明度底图
  Return:          NONE
**********************************************************************/

void *divoom_popup_menu_create(int width, int high, int opa, DIVOOM_POPUP_MENU_CANCEL_CALLBACK *pexit_callbck, void *user_data,DIVOOM_POPUP_MENU_KEY_CALLBACK *pkey_callback);


/*********************************************************************
  Function:        divoom_popup_menu_update_disp_buf
  Description:     更指定的弹窗提醒的显示缓冲区
  Input:           phandle
                   buf
                   play_time
  Return:          NONE
**********************************************************************/

void divoom_popup_menu_update_disp_buf(void *phandle, void *buf, int play_time);

/*********************************************************************
  Function:        divoom_popup_menu_destroy
  Description:     将清除指定的弹窗提醒
  Input:           none
  Return:          NONE
**********************************************************************/

void divoom_popup_menu_destroy(void *phandle);

/*********************************************************************
  Function:        divoom_popup_menu_destroy_all
  Description:     将清除所有的弹窗提醒
  Input:           none
  Return:          NONE
**********************************************************************/

void divoom_popup_menu_destroy_all(void);


/*********************************************************************
  Function:        divoom_popup_menu_set_actice_obj
  Description:     设置激活控件
  Input:           phandle
                   buf
                   play_time
  Return:          NONE
**********************************************************************/

void divoom_popup_menu_set_actice_obj(void *phandle);

#endif


