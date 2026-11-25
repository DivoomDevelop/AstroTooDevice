#ifndef _divoom_disp_phone_ctrl_h_
#define _divoom_disp_phone_ctrl_h_

enum{
	DIVOOM_DISP_PHONE_SET_PIC, //设置图片
	DIVOOM_DISP_PHONE_RESUME_PIC, //恢复上一次画板的图片
	DIVOOM_DISP_PHONE_SET_DOT, //设置画点
	DIVOOM_DISP_PHONE_SET_OTHER, //设置其他属性
};

void divoom_disp_phone_ctrl_init(void * data);

void divoom_disp_phone_ctrl_update(void * data);

void divoom_disp_phone_ctrl_close(void);
void divoom_disp_phone_ctrl_add_text(void *data);
void divoom_disp_phone_ctrl_clear_text(void);

#endif


