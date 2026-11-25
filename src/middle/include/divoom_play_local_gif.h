#ifndef _divoom_play_local_gif_h_
#define _divoom_play_local_gif_h_

/*********************************************************************
  Function:        divoom_play_local_gif_exit_image
  Description:     模块退出，释放所有资源
  Input:           
  Return:          none
**********************************************************************/

void divoom_play_local_gif_add_file(char *file_name, int new_flag);
/*********************************************************************
  Function:        divoom_play_local_gif_wait_notice
  Description:     收到命令，正在等待下载
  Input:           
  Return:          none
**********************************************************************/

void divoom_play_local_gif_wait_notice(void);

/*********************************************************************
  Function:        divoom_play_local_gif_exit_image
  Description:     模块退出，释放所有资源
  Input:           
  Return:          none
**********************************************************************/

void divoom_play_local_gif_exit_image(void);

/*********************************************************************
  Function:        divoom_play_local_gif_init
  Description:     模块初始化
  Input:           insert_time   插入显示时间的间隔,为0将不显示时间
                   gif_time      动画显示最少时间
  Return:          返回句柄
**********************************************************************/

void divoom_play_local_gif_init(char *file_name);

/*********************************************************************
  Function:        divoom_play_local_gif_ctrl_time_frame
  Description:     是否增加时间显示//只有动画才使用这个功能，默认ＩＤ为3;
  Input:           on_off   开关选项
  Return:          返回句柄
**********************************************************************/
void divoom_play_local_gif_ctrl_time_frame(int on_off);

#endif

