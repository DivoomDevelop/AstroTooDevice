#ifndef _divoom_lvgl_stream_cont_h_
#define _divoom_lvgl_stream_cont_h_

/*********************************************************************
  Function:        divoom_stream_cont_init
  Description:     初始化数字流控件
  Input:           main_obj  	  主界面指针
  				   x,y		 	  控件坐标
  				   cont_w,cont_h  控件的宽度,高度
  				   cont_font_w，cont_font_h	控件的单个组件的宽高
  				   unseleted_font_size 未选中字体大小,小于cont_font_w和cont_font_h
  				   seleted_font_size   选中字体大小
  				   num_font_cont  总个数
  				   start_flag	  0:从0开始 1:从1开始
  				   num_min_width  0~9的数字宽度 1:1位 2:2位
  Return:          返回控制句柄指针
**********************************************************************/
void* divoom_stream_cont_init(void* main_obj,int x,int y,int cont_w,int cont_h,int cont_font_w,int cont_font_h, int unseleted_font_size, int seleted_font_size, int font_color_hex, int num_font_cont,int start_flag,int num_min_width);

/*********************************************************************
  Function:        divoom_stream_cont_move
  Description:     控件里面的组件移动
  Input:           cont  控制句柄指针
  				   step  步数
  Return:          
**********************************************************************/
void divoom_stream_cont_move(void* cont,int step);

/*********************************************************************
  Function:        divoom_stream_cont_get_cont_obj
  Description:     获取滚动的对象
  Input:           cont  控制句柄
  Return:          滚动控件对象
**********************************************************************/
void *divoom_stream_cont_get_cont_obj(void* cont);

/*********************************************************************
  Function:        divoom_stream_cont_pause_cont
  Description:     退出不使用时暂停滚动,
  				   暂停后停留在滚动后选中值
  Input:           cont  控制句柄
  Return:          
**********************************************************************/
void divoom_stream_cont_pause_cont(void *cont);


/*********************************************************************
  Function:        divoom_stream_cont_get_current_selected
  Description:     获取当前选中的值
  Input:           cont  控制句柄指针 
  Return:          返回当前选中的值
**********************************************************************/
int divoom_stream_cont_get_current_selected(void *cont);

/*********************************************************************
  Function:        divoom_stream_cont_set_current_selected
  Description:     设置当前选中的值
  Input:           cont  控制句柄指针 
  				   index 要设置选中的值
  Return:          成功1，失败0
**********************************************************************/
int divoom_stream_cont_set_current_selected(void *cont,int index);

void divoom_stream_cont_key_roll_up(void *cont);
void divoom_stream_cont_key_roll_down(void *cont);

/*********************************************************************
  Function:        divoom_stream_cont_del
  Description:     释放数字流控件的资源
  Input:           cont  控制句柄指针 
  Return:          
**********************************************************************/
void divoom_stream_cont_del(void* cont);







#endif


