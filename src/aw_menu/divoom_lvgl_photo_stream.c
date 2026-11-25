#include "divoom.h"
#include "lvgl.h"
#include "divoom_thumb_cache.h"
#include "divoom_menu_control.h"
#include "divoom_lvgl_photo_slide_effect.h"
#include "divoom_pixel_format.h"

#define DIVOOM_LVGL_PHOTO_STREAM_IMAGE_WIDTH  236          //图片宽度
#define DIVOOM_LVGL_PHOTO_STREAM_IMAGE_HIGH   378          //每个照片区域大小  

#define DIVOOM_LVGL_PHOTO_STREAM_IMAGE_AREA_WIDTH  236          //图片宽度
#define DIVOOM_LVGL_PHOTO_STREAM_IMAGE_AREA_WIDTH_SEP  44       //列与列之间的间隔
#define DIVOOM_LVGL_PHOTO_STREAM_IMAGE_AREA_HIGH   360          //每个照片区域大小   
#define DIVOOM_LVGL_PHOTO_STREAM_IMAGE_IMAGE_NUM_PRE_SCREEN   (DIVOOM_SCREEN_HIGH/DIVOOM_LVGL_PHOTO_STREAM_IMAGE_AREA_HIGH)          //每个屏最多可以放几个图像   

#define DIVOOM_LVGL_PHOTO_STREAM_BASE_SPEED_TIME   8            //照片流的基本速度
#define DIVOOM_LVGL_PHOTO_STREAM_BASE_SPEED_STEPS   1          //照片流的基本速度   

#define DIVOOM_LVGL_PHOTO_STREAM_MAX_SPEED_STEPS   4          //照片流的最大速度  

#define DIVOOM_LVGL_PHOTO_STREAM_SPEED_UP_CONTINUE_TIME   3000          //用户加速滑动的最大时间  


#define DIVOOM_LVGL_PHOTO_STREAM_IMAGE_CACHE_NUM  (DIVOOM_LVGL_PHOTO_STREAM_IMAGE_IMAGE_NUM_PRE_SCREEN + 2)
#define DIVOOM_LVGL_PHOTO_STREAM_IMAGE_PRE_READ_NUM  4
struct _divoom_lvgl_photo_stream_ctrl;

typedef struct _divoom_lvgl_photo_stream_list_item
{
    int dir;                    //照片流的流动方向, 1:向上流动，-1：向下流动
    int start_speed_up_time;    //开始加速时间
    int step_dots;      //每次移动的点数
    int cur_tick;       //更新的计数，如果cur_tick=speed，
    int speed;          //流动的速度
    int press_time;
	int move_steps;
    int base_pos;
    int dest_pos;
    int timer_tick;
    int idle_tick;
    void  *cont;    //照片流一列的容器
    lv_timer_t *timer;
    int pos_y[DIVOOM_LVGL_PHOTO_STREAM_IMAGE_CACHE_NUM];  
    uint8 img_update_flag[DIVOOM_LVGL_PHOTO_STREAM_IMAGE_CACHE_NUM];  
    lv_img_dsc_t *image_data[DIVOOM_LVGL_PHOTO_STREAM_IMAGE_CACHE_NUM];
    int   image_id[DIVOOM_LVGL_PHOTO_STREAM_IMAGE_CACHE_NUM];
    int   image_pos[DIVOOM_LVGL_PHOTO_STREAM_IMAGE_CACHE_NUM];
    void *img_cout_obj[DIVOOM_LVGL_PHOTO_STREAM_IMAGE_CACHE_NUM];
    void *img_obj[DIVOOM_LVGL_PHOTO_STREAM_IMAGE_CACHE_NUM];
    struct _divoom_lvgl_photo_stream_ctrl *ppraent_obj;
    
}divoom_lvgl_photo_stream_list_item, *Pdivoom_lvgl_photo_stream_list_item;


typedef struct _divoom_lvgl_photo_stream_ctrl
{
    int menu_id;
    int album_id;
    int pic_num;
    void *main_obj;
    lv_style_t backgroud_type;
    lv_style_t cout_type;
    int pre_read_img_pos;       //预读的位置
    int read_img_pos;           //上一次读到的位置
    void *pget_pic_handle;
    int pic_total;
    Pdivoom_pic_phone_manage_slide_config pslide_config;
    divoom_lvgl_photo_stream_list_item photo_stream_list_info[DIVOOM_PIC_ALBUM_MANAGE_LIST_NUMBER];
}divoom_lvgl_photo_stream_ctrl, *Pdivoom_lvgl_photo_stream_ctrl;

void divoom_lvgl_photo_stream_update_finish_read_over_callback(void *para, uint8 *rgb_buf, int image_w, int image_h)
{    
	int i = 0; 
	for(int i = 0; i < 100; i ++)
	{
        if(divoom_get_lvgl_try_lock()== 0)
        {
            lv_obj_invalidate(para);                
            divoom_free_lvgl_lock();
            break;
        }
        divoom_system_delay_ms(10);
	}
}


void divoom_lvgl_photo_stream_pre_read_pic(Pdivoom_lvgl_photo_stream_ctrl phandle)
{
    phandle->pre_read_img_pos = (phandle->pre_read_img_pos + 1)%phandle->pic_total;
    divoom_pic_album_manage_pre_read_cache(NULL, 0, divoom_pic_album_manage_get_disp_order_pic_id(phandle->pget_pic_handle, phandle->pre_read_img_pos, NULL));
}

static int divoom_lvgl_photo_stream_read_pic_lvfmt(int pic_id, lv_img_dsc_t *image_data,
                                                   int image_w, int image_h,
                                                   DIVOOM_PIC_ALBUM_MANAGE_READ_OVER *pcallback,
                                                   void *para)
{
    uint8 *dst;
    int ret;

    if(image_data == NULL || image_data->data == NULL)
    {
        return 0;
    }

    dst = (uint8 *)image_data->data;
    ret = divoom_pic_album_manage_read_pic(NULL, 0, pic_id, 1,
        dst, image_w, image_h, pcallback, para);
    if(ret)
    {
        divoom_lv_img_dsc_init_fmt(image_data, image_w, image_h, LV_COLOR_FORMAT_RGB565A8, dst);
    }
    return ret;
}


static void divoom_lvgl_photo_stream_update_y(Pdivoom_lvgl_photo_stream_ctrl phandle, Pdivoom_lvgl_photo_stream_list_item pcout_info, int steps)
{
    int y, h, max_high, image_w = DIVOOM_LVGL_PHOTO_STREAM_IMAGE_WIDTH, image_h = DIVOOM_LVGL_PHOTO_STREAM_IMAGE_HIGH;
    
    max_high = DIVOOM_LVGL_PHOTO_STREAM_IMAGE_AREA_HIGH  * DIVOOM_LVGL_PHOTO_STREAM_IMAGE_CACHE_NUM ;
    for (int i = 0; i < DIVOOM_LVGL_PHOTO_STREAM_IMAGE_CACHE_NUM; i++)
    {
        if(pcout_info->img_obj[i] == NULL)
        {
            break;
        }
        h = pcout_info->pos_y[i];
        y = h + steps;
        if(y >= max_high)
        {
            //只有向下运动才可能出现这情况
            y  -= max_high;
            //这个位置要重新加载图片
            phandle->read_img_pos = (phandle->read_img_pos + 1)%phandle->pic_total;
            pcout_info->image_pos[i] = phandle->read_img_pos ;
            pcout_info->image_id[i] = divoom_pic_album_manage_get_disp_order_pic_id(phandle->pget_pic_handle, pcout_info->image_pos[i], NULL);
            if(divoom_lvgl_photo_stream_read_pic_lvfmt(pcout_info->image_id[i], pcout_info->image_data[i],
                image_w, image_h, divoom_lvgl_photo_stream_update_finish_read_over_callback, pcout_info->img_obj[i]))
            {   
                lv_img_dsc_t *image_tmp_handle = divoom_system_malloc(sizeof(lv_img_dsc_t));
                pcout_info->img_update_flag[i] = 0;
                memcpy(image_tmp_handle, pcout_info->image_data[i], sizeof(lv_img_dsc_t));
                lv_img_cache_invalidate_src(NULL);
                lv_obj_set_size(pcout_info->img_obj[i], image_w, image_h);
                lv_img_set_src(pcout_info->img_obj[i], image_tmp_handle);
                divoom_system_free(pcout_info->image_data[i]);
                pcout_info->image_data[i] = image_tmp_handle;
            }
            else
            {
                pcout_info->img_update_flag[i] = 1;
            }
            divoom_lvgl_photo_stream_pre_read_pic(phandle);
        }
        else if(y < 0)
        {
            //只有向上运动才可能出现这情况
            y  += max_high;
            phandle->read_img_pos = (phandle->read_img_pos + 1)%phandle->pic_total;
            pcout_info->image_pos[i] = phandle->read_img_pos ;
            pcout_info->image_id[i] = divoom_pic_album_manage_get_disp_order_pic_id(phandle->pget_pic_handle, pcout_info->image_pos[i], NULL);
            if(divoom_lvgl_photo_stream_read_pic_lvfmt(pcout_info->image_id[i], pcout_info->image_data[i],
                image_w, image_h, divoom_lvgl_photo_stream_update_finish_read_over_callback, pcout_info->img_obj[i]))
            {   
                lv_obj_invalidate(pcout_info->img_obj[i]);                
                pcout_info->img_update_flag[i] = 0;
            }
            else
            {
                pcout_info->img_update_flag[i] = 1;
            }
            divoom_lvgl_photo_stream_pre_read_pic(phandle);
        }
        lv_obj_set_y(pcout_info->img_cout_obj[i], y);
        pcout_info->pos_y[i] = y;
    }
}

static void divoom_lvgl_photo_stream_time_task(lv_timer_t *timer)
{
    Pdivoom_lvgl_photo_stream_ctrl phandle;
    Pdivoom_lvgl_photo_stream_list_item pcout_info = (Pdivoom_lvgl_photo_stream_list_item)lv_timer_get_user_data(timer);

    phandle = (Pdivoom_lvgl_photo_stream_ctrl)pcout_info->ppraent_obj;
    
    divoom_lvgl_photo_stream_update_y(phandle, pcout_info, 2 * pcout_info->dir);
    
}

void *divoom_lvgl_photo_stream_init(void *user_data, int menu_id)
{
    int i, j, image_w = DIVOOM_LVGL_PHOTO_STREAM_IMAGE_WIDTH, image_h = DIVOOM_LVGL_PHOTO_STREAM_IMAGE_HIGH;
    Pdivoom_lvgl_photo_stream_ctrl phandle;
    Pdivoom_lvgl_photo_slide_effect_info para_info = (Pdivoom_lvgl_photo_slide_effect_info)user_data;

    
    phandle = divoom_system_malloc(sizeof(divoom_lvgl_photo_stream_ctrl));
    phandle->menu_id = menu_id;
    phandle->album_id = para_info->album_id;
    divoom_thumb_cache_set_dest_pic_size(DIVOOM_LVGL_PHOTO_STREAM_IMAGE_WIDTH, DIVOOM_LVGL_PHOTO_STREAM_IMAGE_HIGH);
    divoom_thumb_cache_set_clock_info(NULL);
    phandle->main_obj = lv_btn_create(NULL);
	lv_obj_clear_flag(phandle->main_obj, LV_OBJ_FLAG_SCROLLABLE);
    divoom_menu_control_init_lable_style(&phandle->cout_type);
    lv_style_init(&phandle->backgroud_type);
    lv_style_set_bg_color(&phandle->backgroud_type, lv_color_hex(DIVOOM_MENNU_NORMAL_BG_COLOR));
    lv_style_set_text_color(&phandle->backgroud_type, lv_color_hex(DIVOOM_MENNU_TEXT_COLOR));
    lv_obj_add_style(phandle->main_obj, &phandle->backgroud_type, 0);
    lv_img_cache_invalidate_src(NULL);
    phandle->pslide_config = divoom_pic_phone_manage_get_slide_config();
    phandle->pget_pic_handle = divoom_pic_album_manage_create_disp_order_handle(para_info->album_id, phandle->pslide_config->disp_order, phandle->pslide_config->disp_reverse, 0); 
    phandle->read_img_pos = divoom_pic_album_manage_get_first_play_info(phandle->pget_pic_handle, NULL);   
    phandle->pic_total = divoom_pic_album_manage_get_disp_order_pic_num(phandle->pget_pic_handle);
    phandle->pic_num = phandle->pic_total;

    if(phandle->pic_total == 0)
    {
        //给出一个提示，没有照片的提示,理论是不会出现这情况，在上一级就返回了
        void* use_font = divoom_menu_control_get_font_info(40, phandle->menu_id);
        void *label_obj = lv_label_create(phandle->main_obj);
        if(use_font != NULL)
        {
            lv_obj_set_style_text_font(label_obj, use_font, 0);
        }
        lv_label_set_long_mode(label_obj, LV_LABEL_LONG_WRAP);

        // 设置标签宽度（必需，否则文本不会换行）
        lv_obj_set_width(label_obj, 600);  // 设置想要的宽度，比如200像素s

        lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_align(label_obj, LV_ALIGN_CENTER);
        lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("PlsAddPhoto"));
        
        return phandle;
    }

    phandle->read_img_pos = phandle->read_img_pos / DIVOOM_PIC_ALBUM_MANAGE_LIST_NUMBER * DIVOOM_PIC_ALBUM_MANAGE_LIST_NUMBER;
    for(i = 0; i < DIVOOM_PIC_ALBUM_MANAGE_LIST_NUMBER; i++)
    {
        
        phandle->photo_stream_list_info[i].ppraent_obj = phandle;
        phandle->photo_stream_list_info[i].dir = (i == 1)?1:-1; //只有中间列默认值是向下流动，其它二列向上流动
        phandle->photo_stream_list_info[i].timer = lv_timer_create(divoom_lvgl_photo_stream_time_task, 16, &phandle->photo_stream_list_info[i]);
        
        lv_timer_pause(phandle->photo_stream_list_info[i].timer);        
        phandle->photo_stream_list_info[i].cont = lv_obj_create(phandle->main_obj);
        lv_obj_clear_flag(phandle->photo_stream_list_info[i].cont, LV_OBJ_FLAG_CLICKABLE);
        
    	lv_obj_clear_flag(phandle->photo_stream_list_info[i].cont, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_style(phandle->photo_stream_list_info[i].cont, &phandle->cout_type, 0);
        lv_obj_set_style_bg_color(phandle->photo_stream_list_info[i].cont, lv_color_black(), 0);
    	lv_obj_set_size(phandle->photo_stream_list_info[i].cont, DIVOOM_LVGL_PHOTO_STREAM_IMAGE_AREA_WIDTH, DIVOOM_LVGL_PHOTO_STREAM_IMAGE_AREA_HIGH * DIVOOM_LVGL_PHOTO_STREAM_IMAGE_CACHE_NUM);
    	//lv_obj_center(phandle->photo_stream_list_info[i].cont);  
        lv_obj_set_pos(phandle->photo_stream_list_info[i].cont, i*(DIVOOM_LVGL_PHOTO_STREAM_IMAGE_AREA_WIDTH + DIVOOM_LVGL_PHOTO_STREAM_IMAGE_AREA_WIDTH_SEP), 0-DIVOOM_LVGL_PHOTO_STREAM_IMAGE_AREA_HIGH);
    	
        if(phandle->pic_num == 0)
        {
            divoom_print("err info!\n");
            continue;
        }
        
        for(j = 0; j < DIVOOM_LVGL_PHOTO_STREAM_IMAGE_CACHE_NUM; j++)
        {
            
            phandle->photo_stream_list_info[i].img_cout_obj[j] = lv_img_create(phandle->photo_stream_list_info[i].cont);
            lv_obj_set_pos(phandle->photo_stream_list_info[i].img_cout_obj[j], 0, j*DIVOOM_LVGL_PHOTO_STREAM_IMAGE_AREA_HIGH);
            lv_obj_set_size(phandle->photo_stream_list_info[i].img_cout_obj[j], DIVOOM_LVGL_PHOTO_STREAM_IMAGE_AREA_WIDTH, DIVOOM_LVGL_PHOTO_STREAM_IMAGE_AREA_HIGH);
            lv_obj_set_style_bg_color(phandle->photo_stream_list_info[i].img_cout_obj[j], lv_color_black(), 0);
            lv_obj_add_style(phandle->photo_stream_list_info[i].img_cout_obj[j], &phandle->cout_type, 0);
            phandle->photo_stream_list_info[i].img_obj[j] = lv_img_create(phandle->photo_stream_list_info[i].img_cout_obj[j]);
            phandle->photo_stream_list_info[i].pos_y[j] = j*DIVOOM_LVGL_PHOTO_STREAM_IMAGE_AREA_HIGH;
            

            phandle->photo_stream_list_info[i].image_pos[j] = (j * DIVOOM_PIC_ALBUM_MANAGE_LIST_NUMBER + i) %phandle->pic_total; 
            phandle->photo_stream_list_info[i].image_id[j] = divoom_pic_album_manage_get_disp_order_pic_id(phandle->pget_pic_handle, phandle->photo_stream_list_info[i].image_pos[j], NULL);
            

            phandle->photo_stream_list_info[i].image_data[j] = divoom_system_malloc(sizeof(lv_img_dsc_t));
            divoom_lv_img_dsc_init_fmt(phandle->photo_stream_list_info[i].image_data[j],
                image_w, image_h, LV_COLOR_FORMAT_RGB565A8, NULL);
            phandle->photo_stream_list_info[i].image_data[j]->data = divoom_system_malloc(phandle->photo_stream_list_info[i].image_data[j]->data_size);

            divoom_lvgl_photo_stream_read_pic_lvfmt(phandle->photo_stream_list_info[i].image_id[j],
                phandle->photo_stream_list_info[i].image_data[j], image_w, image_h, NULL, NULL);
            lv_obj_set_size(phandle->photo_stream_list_info[i].img_obj[j], image_w, image_h);
            lv_img_set_src(phandle->photo_stream_list_info[i].img_obj[j], phandle->photo_stream_list_info[i].image_data[j]);
            lv_obj_center(phandle->photo_stream_list_info[i].img_obj[j]);
            phandle->photo_stream_list_info[i].img_update_flag[i] = 0;
            phandle->read_img_pos ++;
            
        }
        
        //lv_obj_add_event_cb(phandle->photo_stream_list_info[i].cont, divoom_lvgl_photo_stream_event_cb,
        //    LV_EVENT_ALL, &phandle->photo_stream_list_info[i]);
    }
    

    phandle->pre_read_img_pos = phandle->read_img_pos;
    for(i = 0; i < DIVOOM_PIC_ALBUM_MANAGE_LIST_NUMBER * DIVOOM_LVGL_PHOTO_STREAM_IMAGE_PRE_READ_NUM; i++)
    {
        divoom_lvgl_photo_stream_pre_read_pic(phandle);
    } 
    


    return  phandle;
}


void divoom_lvgl_photo_stream_disp(void *handle, void *active)
{
    int i;
    Pdivoom_lvgl_photo_stream_ctrl phandle = (Pdivoom_lvgl_photo_stream_ctrl)handle;
    
    for(i = 0; i < DIVOOM_PIC_ALBUM_MANAGE_LIST_NUMBER; i++)
    {
        if(phandle->photo_stream_list_info[i].timer)
        {
            lv_timer_resume(phandle->photo_stream_list_info[i].timer);        
        }
    }
    lv_scr_load(phandle->main_obj);
}

void divoom_lvgl_photo_stream_pause(void *handle, int exit_flag)
{
    int i;
    Pdivoom_lvgl_photo_stream_ctrl phandle = (Pdivoom_lvgl_photo_stream_ctrl)handle;
    
    divoom_thumb_cache_clear_all();

    for(i = 0; i < DIVOOM_PIC_ALBUM_MANAGE_LIST_NUMBER; i++)
    {
        if(phandle->photo_stream_list_info[i].timer)
        {
            lv_timer_pause(phandle->photo_stream_list_info[i].timer);  
        }
    }
}

void *divoom_lvgl_photo_stream_get_lvgl_obj(void *handle)
{
    Pdivoom_lvgl_photo_stream_ctrl phandle = (Pdivoom_lvgl_photo_stream_ctrl)handle;
    
    return phandle->main_obj;
}

void divoom_lvgl_photo_stream_update_time(void *handle, int play_time)
{
}

void divoom_lvgl_photo_stream_play_next(void *handle, int dir)
{

}

void divoom_lvgl_photo_stream_remove_notice(void *handle, uint32 *pic_list, int pic_num)
{
    Pdivoom_lvgl_photo_stream_ctrl phandle = (Pdivoom_lvgl_photo_stream_ctrl)handle;

    for(int i = 0; i < pic_num; i++)
    {
        divoom_pic_album_manage_remove_pic_id_from_disp_handle(phandle->pget_pic_handle, pic_list[i]);
        phandle->pic_num --;        
    }
    
    if(phandle->read_img_pos >= phandle->pic_num)
    {
        phandle->read_img_pos = phandle->pic_num - 1;
    }
}


void divoom_lvgl_photo_stream_get_pic_info(void *handle, Pdivoom_lvgl_photo_view_para_info pic_info)
{
    uint32 cur_pos = U32_MAX, cur_id = 0;
    Pdivoom_lvgl_photo_stream_ctrl phandle = (Pdivoom_lvgl_photo_stream_ctrl)handle;
    
    
    phandle->pic_num = divoom_pic_album_manage_get_disp_order_pic_num(phandle->pget_pic_handle);

    for(int i = 0; i < DIVOOM_PIC_ALBUM_MANAGE_LIST_NUMBER; i++)
    {
        
        for(int j = 0; j < DIVOOM_LVGL_PHOTO_STREAM_IMAGE_CACHE_NUM; j++)
        {
            if(phandle->photo_stream_list_info[i].image_id[j] <= 0)
            {
                 continue;
            }
            if(cur_pos > phandle->photo_stream_list_info[i].image_pos[j])
            {
                cur_pos = phandle->photo_stream_list_info[i].image_pos[j];
                cur_id  = phandle->photo_stream_list_info[i].image_id[j];
            }
            
        }
    }

    divoom_print("image info: %d, %d\n", cur_pos, cur_id);
    pic_info->cur_pos = cur_pos;
    pic_info->pic_id = cur_id;
    pic_info->pic_num = &phandle->pic_num;
    pic_info->pget_pic_handle = phandle->pget_pic_handle;
    pic_info->pdel_callback = NULL;
}



void divoom_lvgl_photo_stream_eixt(void *handle)
{
    int i, j, cur_pos = 0, cur_id = 0;
    Pdivoom_lvgl_photo_stream_ctrl phandle = (Pdivoom_lvgl_photo_stream_ctrl)handle;

    divoom_lvgl_photo_stream_pause(phandle, 1);
    lv_style_reset(&phandle->backgroud_type);
    lv_style_reset(&phandle->cout_type);
    lv_obj_del(phandle->main_obj);
    for(i = 0; i < DIVOOM_PIC_ALBUM_MANAGE_LIST_NUMBER; i++)
    {
        if(phandle->photo_stream_list_info[i].timer)
        {
            lv_timer_del(phandle->photo_stream_list_info[i].timer);
        }
        for(j = 0; j < DIVOOM_LVGL_PHOTO_STREAM_IMAGE_CACHE_NUM; j++)
        {
            if(cur_pos > phandle->photo_stream_list_info[i].image_pos[j])
            {
                cur_pos = phandle->photo_stream_list_info[i].image_pos[j];
                cur_id  = phandle->photo_stream_list_info[i].image_id[j];
            }
            if(phandle->photo_stream_list_info[i].image_data[j])
            {
                if(phandle->photo_stream_list_info[i].image_data[j]->data)
                {
                    divoom_system_free((uint8 *)phandle->photo_stream_list_info[i].image_data[j]->data);
                    phandle->photo_stream_list_info[i].image_data[j]->data = NULL;
                }
                divoom_system_free(phandle->photo_stream_list_info[i].image_data[j]);
                phandle->photo_stream_list_info[i].image_data[j] = NULL;
            }
        }
    }
    if(cur_id > 0)
    {
        divoom_pic_album_manage_update_last_play_album_pic_id(phandle->album_id, cur_id);
    }
    divoom_pic_album_manage_destory_disp_order_handle(phandle->pget_pic_handle);
    divoom_system_free(phandle);
    
}

