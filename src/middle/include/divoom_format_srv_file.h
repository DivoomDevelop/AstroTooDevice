#ifndef _divoom_format_srv_file_h_
#define _divoom_format_srv_file_h_
/**********************************************
加载动画
**************************************************/
uint8 *divoom_format_srv_gif_file_to_buf(char *net_addr, int *real_size);
int divoom_format_srv_file_do(char *net_addr, char *local_file, uint8 iframe_flag, uint8 *sha1_sum);
uint8* divoom_format_pic_single(uint8 *file_buf, int *pic_len, int tol_size, int flag);
uint8 *divoom_format_pic_scan_rgb(uint8 *file_buf, int *pic_len, int pic_size, int speed, int mode);
uint8* divoom_format_pic_gif(uint8 *file_buf, int *pic_len, int tol_size, int flag, uint8 iframe_flag);
uint8 *divoom_format_srv_draw_pic_handle(uint8 *pic_buf, int *pic_len, int pic_size, int form, int pic_time);
uint8 *divoom_format_srv_buf_to_local(uint8 *raw_buf, int *pic_len, int raw_size);
int divoom_format_srv_hot_file(char *net_addr, char *local_file, uint8 *sha1_num);

#endif


