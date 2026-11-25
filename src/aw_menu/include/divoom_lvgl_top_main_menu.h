#ifndef _divoom_lvgl_top_main_menu_h_
#define _divoom_lvgl_top_main_menu_h_


typedef struct _divoom_lvgl_top_main_menu_image_item
{
    int x;
    int y;
    int w;  
    int h;  
    int antialias_flag;       
    int angle;      
    char *image_name;   
    uint8 *rgb_buf;     
}divoom_lvgl_top_main_menu_image_item, *Pdivoom_lvgl_top_main_menu_image_item;

typedef struct _divoom_lvgl_top_main_menu_image_ctrl
{
    uint8 *divoom_rgb_buf;
    int cur_disp_id;    
	int resource_type;	   
    int image_type;     
    int area_w;         
    int area_h;         
    int angle_value;    
    int image_strech;   
    int frame_num;
    int frame_time;
    int image_width;
    int image_high;
	int play_time; 
	int pwebp_handle_free_flag; 
    char *image_name;
    void *pwebp_handle;          
    void *disp_time_handle; 
    void *lvgl_image_obj;    
	//Pdivoom_lvgl_image_info lvgl_image_info;
	lv_img_dsc_t *lvgl_image_buf;
    //DIVOOM_GET_EQ_DISP_BUF_CALLBACK pread_disp_buf_callback;
    void *user_data;
	void *obj;
    //DIVOOM_LVGL_DIAL_MENU_FINISH_CALLBACK pfinish_callback;
}divoom_lvgl_top_main_menu_image_ctrl, *Pdivoom_lvgl_top_main_menu_image_ctrl;


void divoom_lvgl_top_main_menu_init(void);

void divoom_lvgl_top_main_menu_exit(void);



#endif

