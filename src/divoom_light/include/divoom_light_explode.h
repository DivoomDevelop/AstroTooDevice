#ifndef _divoom_light_explode_h_
#define _divoom_light_explode_h_

uint32 divoom_light_explode_init(void);
void divoom_light_explode_close(void);
void divoom_light_explode_main(void);
void divoom_light_explode_get_taskid(uint32 id);

uint32 divoom_light_explode_get_refresh_time(void);
void divoom_light_explode_set_refresh_time(void);
bool divoom_light_explode_is_get_color(uint8 x, uint8 y,uint8 *pic_buf);



/*
//内核中调用会导致error:dump_backtrace ,此处函数暂不可用
void divoom_light_explode_draw_random_pos_vline(uint8 h, Pdivoom_light_color_info color,uint8 *pic_buf);
void divoom_light_explode_draw_random_pos_hline(uint8 w, Pdivoom_light_color_info color,uint8 *pic_buf);
void divoom_light_explode_draw_multi_random_vline(uint8 amount, uint8 h, uint8 color_array_len, Pdivoom_light_color_info color,uint8 *pic_buf);
void divoom_light_explode_draw_multi_random_hline(uint8 amount, uint8 w, uint8 color_array_len, Pdivoom_light_color_info color,uint8 *pic_buf);
*/


void divoom_light_explode_draw_random_pos_rect(uint8 w,uint8 h, Pdivoom_light_color_info color,uint8 *pic_buf);
void divoom_light_explode_draw_multi_random_rect(uint8 amount, uint8 w,uint8 h, uint8 color_array_len, Pdivoom_light_color_info color,uint8 *pic_buf);
void divoom_light_explode_fill_random_pos_rect(uint8 w,uint8 h, Pdivoom_light_color_info color,uint8 *pic_buf);
void divoom_light_explode_fill_multi_random_pos_rect(uint8 amount, uint8 w,uint8 h, uint8 color_array_len, Pdivoom_light_color_info color,uint8 *pic_buf);
void divoom_light_explode_draw_multi_random_pos_square(uint8 amount, uint8 w,uint8 color_array_len, Pdivoom_light_color_info color,uint8 *pic_buf);
void divoom_light_explode_fill_multi_random_pos_square(uint8 amount, uint8 w,uint8 color_array_len, Pdivoom_light_color_info color,uint8 *pic_buf);

void divoom_light_explode_draw_random_pos_circle(uint8 r, Pdivoom_light_color_info color,uint8 *pic_buf);
void divoom_light_explode_fill_random_pos_circle(uint8 r, Pdivoom_light_color_info color,uint8 *pic_buf);
void divoom_light_explode_draw_multi_random_pos_circle(uint8 amount, uint8 r,uint8 color_array_len, Pdivoom_light_color_info color,uint8 *pic_buf);
void divoom_light_explode_fill_multi_random_pos_circle(uint8 amount, uint8 r,uint8 color_array_len, Pdivoom_light_color_info color,uint8 *pic_buf);


/********************
对称函数
********************/
void divoom_light_explode_symmetry_lr_point(uint8 x, uint8 y, uint8 *pic_buf, uint8 *pic_swap);
void divoom_light_explode_symmetry_lr_pic(uint8 *pic_buf, uint8 *pic_swap);
void divoom_light_explode_symmetry_updown_point(uint8 x, uint8 y, uint8 *pic_buf, uint8 *pic_swap);
void divoom_light_explode_symmetry_updown_pic(uint8 *pic_buf, uint8 *pic_swap);


void divoom_light_explode_turn_random_color(uint8 *pic_buf, Pdivoom_light_color_info color_palette,uint8 array_len);//图像重新随机颜色

void divoom_light_explode_counterclockwise_rect_color(uint8 x, uint8 y, uint8 w, uint8 h, uint8 *pic_buf);//颜色矩形逆时针回转
void divoom_light_explode_clockwise_rect_color(uint8 x, uint8 y, uint8 w, uint8 h, uint8 *pic_buf);//颜色矩形顺时针回转

void divoom_light_explode_spin_square_pic(uint8 *pic_buf, uint8 direction);

#endif





