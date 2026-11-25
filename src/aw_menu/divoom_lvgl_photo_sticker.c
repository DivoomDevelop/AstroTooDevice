//相片墙贴效果
#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_lvgl_photo_slide_effect.h"
#include "divoom_lvgl_play_gif_file.h"
#include "divoom_pixel_format.h"


typedef struct _divoom_lvgl_photo_stricker_pos_info
{
	int x;
	int y;
	int w;
	int h;
	int angle;
}divoom_lvgl_photo_stricker_pos_info, *Pdivoom_lvgl_photo_stricker_pos_info;


const static divoom_lvgl_photo_stricker_pos_info photo_stricker_effect0_pos_table[] =
{
	{10,132,360,200, -2},
	{17,368,360,200, -2},
	{27,619,360,576, -2},
	{390,118,360,576, -2},
	{410,745,360,200, -2},
	{418,982,360,200, -2},
};

const static divoom_lvgl_photo_stricker_pos_info photo_stricker_effect1_pos_table[] =
{
	{70,125,296,472, -2},
	{84,613,296,472, -2},
	{481,722,296,472, 1},
	{489,229,296,472, 1},
};

const static divoom_lvgl_photo_stricker_pos_info photo_stricker_effect2_pos_table[] =
{
	{52,743,304,304, 4},
	{88,117,564,316, -8},
	{427,573,296,472, -6},
};

const static divoom_lvgl_photo_stricker_pos_info photo_stricker_effect3_pos_table[] =
{
	{53,223,472,264, -7},
	{95,560,308,308, -7},
	{239,864,508,284, 0},
	{420,316,304,480, 3},
};

const static divoom_lvgl_photo_stricker_pos_info photo_stricker_effect4_pos_table[] =
{
	{31,59,388,620, -3},
	{110,844,564,316, -1},
	{376,194,388,620, 4},
};
    

const static divoom_lvgl_photo_stricker_pos_info photo_stricker_effect5_pos_table[] =
{
	{95,228,388,620, 0},
	{224,854,564,316, 2},
};

const static divoom_lvgl_photo_stricker_pos_info photo_stricker_effect6_pos_table[] =
{
	{51,769,564,316, -2},
	{320,41,388,620, -2},
};



typedef struct _divoom_lvgl_photo_stricker_item_info
{
	int pos_num;
	char *bg_image_addr;
	char *fr_image_addr;
	Pdivoom_lvgl_photo_stricker_pos_info pos_info_table;
}divoom_lvgl_photo_stricker_item_info, *Pdivoom_lvgl_photo_stricker_item_info;

static const divoom_lvgl_photo_stricker_item_info photo_stricker_effect_table[] = 
{
	{
		sizeof(photo_stricker_effect0_pos_table)/sizeof(photo_stricker_effect0_pos_table[0]),
		"14/effect0_bg.png",
		"14/effect0_fr.png",
		(Pdivoom_lvgl_photo_stricker_pos_info)photo_stricker_effect0_pos_table,
	},
	{
		sizeof(photo_stricker_effect1_pos_table)/sizeof(photo_stricker_effect1_pos_table[0]),
		"14/effect1_bg.png",
		"14/effect1_fr.png",
		(Pdivoom_lvgl_photo_stricker_pos_info)photo_stricker_effect1_pos_table,
	},
	{
		sizeof(photo_stricker_effect2_pos_table)/sizeof(photo_stricker_effect2_pos_table[0]),
		"14/effect2_bg.png",
		"14/effect2_fr.png",
		(Pdivoom_lvgl_photo_stricker_pos_info)photo_stricker_effect2_pos_table,
	},
	{
		sizeof(photo_stricker_effect3_pos_table)/sizeof(photo_stricker_effect3_pos_table[0]),
		"14/effect3_bg.png",
		"14/effect3_fr.png",
		(Pdivoom_lvgl_photo_stricker_pos_info)photo_stricker_effect3_pos_table,
	},
	{
		sizeof(photo_stricker_effect4_pos_table)/sizeof(photo_stricker_effect4_pos_table[0]),
		"14/effect4_bg.png",
		"14/effect4_fr.png",
		(Pdivoom_lvgl_photo_stricker_pos_info)photo_stricker_effect4_pos_table,
	},
	{
		sizeof(photo_stricker_effect5_pos_table)/sizeof(photo_stricker_effect5_pos_table[0]),
		"14/effect5_bg.png",
		"14/effect5_fr.png",
		(Pdivoom_lvgl_photo_stricker_pos_info)photo_stricker_effect5_pos_table,
	},
	{
		sizeof(photo_stricker_effect6_pos_table)/sizeof(photo_stricker_effect6_pos_table[0]),
		"14/effect6_bg.png",
		"14/effect6_fr.png",
		(Pdivoom_lvgl_photo_stricker_pos_info)photo_stricker_effect6_pos_table,
	},
};


typedef struct _divoom_lvgl_photo_stricker_image_item
{
    void *img_obj;
    divoom_pic_phone_manage_item_offset  raw_info;
    Pdivoom_pic_phone_manage_item_offset  image_info;
    lv_img_dsc_t image_data; 
    void *play_gif_handle;
}divoom_lvgl_photo_stricker_image_item, *Pdivoom_lvgl_photo_stricker_image_item;

typedef struct _divoom_lvgl_photo_stricker_image_page
{
	int image_nun;
	Pdivoom_lvgl_photo_stricker_image_item p_cur_handle[20];
	Pdivoom_lvgl_photo_stricker_image_item p_next_handle[20];
}divoom_lvgl_photo_stricker_image_page, *Pdivoom_lvgl_photo_stricker_image_page;


typedef struct _divoom_lvgl_photo_stricker_ctrl
{
    int menu_id;
    int album_id;
    int push_index;
    int blinds_index;
    int refresh_num;
    int run_flag;
    int run_tick;
    int end_flag;
    int update_display_flag;    //0：无状态，1：正在更新数据；2：数据更新完，将更新显示
    lv_timer_t *refresh_timer;  //照片读完刷新显示的定时器
    lv_timer_t *read_timer;     //切换读照片的定时器
    void *main_obj;      
    lv_style_t main_style;
    void *pread_sem;            //读照片线程的信号量
    void *pexit_sem;            //读照片线程的信号量
    lv_anim_t image_anim;
    int pic_num;
    int cur_update_pos;
    int use_pos;                //照片列表的使用位置
    divoom_lvgl_photo_stricker_image_page pimage_info;
	Pdivoom_lvgl_photo_stricker_item_info  pcur_active_item;
    void *panim_cur_image_obj;
    void *panim_pre_image_obj;
    uint8 *panim_cur_image_buf;
    uint8 *panim_pre_image_buf;
    void *pget_pic_handle;
    void *prand_handle;
    void *pmix_rand_handle;
    void *pmosaic_random_handle;
    Pdivoom_pic_phone_manage_slide_config pslide_config;

}divoom_lvgl_photo_stricker_ctrl, *Pdivoom_lvgl_photo_stricker_ctrl;


Pdivoom_lvgl_photo_stricker_item_info divoom_lvgl_photo_sticker_get_active_effect(Pdivoom_pic_phone_manage_slide_config pslide_config)
{
    switch(pslide_config->cur_play_effect_type)
    {        
        case DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE0:
        	return (Pdivoom_lvgl_photo_stricker_item_info)&photo_stricker_effect_table[0];	
            break;
            
        case DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE1:
        	return (Pdivoom_lvgl_photo_stricker_item_info)&photo_stricker_effect_table[1];	
            
        case DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE2:

        	return (Pdivoom_lvgl_photo_stricker_item_info)&photo_stricker_effect_table[2];	
            
        case DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE3:

        	return (Pdivoom_lvgl_photo_stricker_item_info)&photo_stricker_effect_table[3];	
            
        case DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE4:

        	return (Pdivoom_lvgl_photo_stricker_item_info)&photo_stricker_effect_table[4];	
            
        case DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE5:

        	return (Pdivoom_lvgl_photo_stricker_item_info)&photo_stricker_effect_table[5];	
            
        case DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE6:

        	return (Pdivoom_lvgl_photo_stricker_item_info)&photo_stricker_effect_table[6];	
            
        default:
        	return (Pdivoom_lvgl_photo_stricker_item_info)&photo_stricker_effect_table[0];	
            
    }
}

void divoom_lvgl_photo_stricker_get_next_image_info(Pdivoom_lvgl_photo_stricker_ctrl phandle, int width_flag, Pdivoom_pic_phone_manage_item_offset pitem_info)
{
    if(phandle->use_pos >= phandle->pic_num)
    {
        phandle->use_pos = 0;
    }
    divoom_pic_album_manage_get_stricker_disp_order_pic_id(phandle->pget_pic_handle, phandle->use_pos, width_flag, pitem_info);
    phandle->use_pos ++;

}

int divoom_lvgl_photo_stricker_load_image_init_buffer(Pdivoom_lvgl_photo_stricker_ctrl phandle, int pos,Pdivoom_lvgl_photo_stricker_image_item pread_item, int lock_flag)
{
    int  image_w, image_h, dest_w, dest_h;
    char file_name[256];
    uint8 *jpeg_buf = NULL;

    if(phandle->pic_num == 0)
    {
        strcpy(file_name, DIVOOM_EMPTY_ALBUM_PHOTO_NAME);
    }
    else
    {
        if(phandle->pcur_active_item->pos_info_table[pos].w == phandle->pcur_active_item->pos_info_table[pos].h)
        {
            divoom_lvgl_photo_stricker_get_next_image_info(phandle, rand()%2, pread_item->image_info);
        }
        else
        {
            divoom_lvgl_photo_stricker_get_next_image_info(phandle, phandle->pcur_active_item->pos_info_table[pos].w > phandle->pcur_active_item->pos_info_table[pos].h, pread_item->image_info);
        }
        divoom_pic_phone_manage_get_picture_name(file_name, pread_item->image_info->id);
    }
    dest_w = phandle->pcur_active_item->pos_info_table[pos].w;
    dest_h = phandle->pcur_active_item->pos_info_table[pos].h;
    if(pread_item->play_gif_handle)
    {
        divoom_lvgl_play_gif_file_destory_handle(pread_item->play_gif_handle, 0);
       pread_item->play_gif_handle = NULL;
    }
    if(phandle->pic_num && pread_item->image_info->pic_video_flag)
    {
        divoom_decode_movie_review_pic_get_lvfmt_buf(file_name,
            (uint8 *)pread_item->image_data.data, dest_w, dest_h, 0, 0,
            LV_COLOR_FORMAT_RGB565A8);
    }
    else
    {
        pread_item->play_gif_handle = divoom_lvgl_play_gif_file_create_handle(file_name, pread_item->img_obj, &pread_item->image_data, dest_w, dest_h, lock_flag, 1);
    }
    if(lock_flag == 0)
    {
        //启动的那一次    
        divoom_lvgl_play_gif_file_start_play(pread_item->play_gif_handle);
    }
        //lv_obj_set_style_img_opa(pread_item[i].img_obj, 0, 0);
    return 1;
}

int divoom_lvgl_photo_stricker_load_image_item_buffer(Pdivoom_lvgl_photo_stricker_ctrl phandle)
{
    int image_w, image_h, dest_w, dest_h, lock_flag = 1;
    char file_name[256];
    uint8 *jpeg_buf = NULL;
    Pdivoom_lvgl_photo_stricker_image_item pread_item;

    //只更新一个
    phandle->cur_update_pos = divoom_random_get(phandle->prand_handle);
    pread_item = phandle->pimage_info.p_next_handle[phandle->cur_update_pos];
    if(phandle->pic_num == 0)
    {
        strcpy(file_name, DIVOOM_EMPTY_ALBUM_PHOTO_NAME);
    }
    else
    {
        if(phandle->pcur_active_item->pos_info_table[phandle->cur_update_pos].w == phandle->pcur_active_item->pos_info_table[phandle->cur_update_pos].h)
        {
            divoom_lvgl_photo_stricker_get_next_image_info(phandle, rand()%2, pread_item->image_info);
        }
        else
        {
            divoom_lvgl_photo_stricker_get_next_image_info(phandle, phandle->pcur_active_item->pos_info_table[phandle->cur_update_pos].w > phandle->pcur_active_item->pos_info_table[phandle->cur_update_pos].h, pread_item->image_info);
        }
        divoom_pic_phone_manage_get_picture_name(file_name, pread_item->image_info->id);
    }
    divoom_print("filename : %s\n", file_name);
    dest_w = phandle->pcur_active_item->pos_info_table[phandle->cur_update_pos].w;
    dest_h = phandle->pcur_active_item->pos_info_table[phandle->cur_update_pos].h;
    if(pread_item->play_gif_handle)
    {
        divoom_lvgl_play_gif_file_destory_handle(pread_item->play_gif_handle, 0);
       pread_item->play_gif_handle = NULL;
    }
    if(phandle->pic_num && pread_item->image_info->pic_video_flag)
    {
        divoom_decode_movie_review_pic_get_lvfmt_buf(file_name,
            (uint8 *)pread_item->image_data.data, dest_w, dest_h, 0, 0,
            LV_COLOR_FORMAT_RGB565A8);
    }
    else
    {
        pread_item->play_gif_handle = divoom_lvgl_play_gif_file_create_handle(file_name, pread_item->img_obj, &pread_item->image_data, dest_w, dest_h, lock_flag, 1);
    }
    if(lock_flag == 0)
    {
        //启动的那一次    
        divoom_lvgl_play_gif_file_start_play(pread_item->play_gif_handle);
    }
    return 1;
}

//读照片的线程
void *divoom_lvgl_photo_sticker_update_image_buffer(void *data)
{
    Pdivoom_lvgl_photo_stricker_ctrl phandle = (Pdivoom_lvgl_photo_stricker_ctrl)data;

    while(phandle->run_flag)
    {
        divoom_system_get_sem(phandle->pread_sem);
        if(phandle->run_flag == 0)
        {
            break;
        }
        if(phandle->update_display_flag != 1)
        {
            continue;
        }
        if(divoom_lvgl_photo_stricker_load_image_item_buffer(phandle))
        {
            phandle->end_flag = 0;
            phandle->update_display_flag = 2;
        }
    }
    divoom_system_put_sem(phandle->pexit_sem);

    return  NULL;
    
}

//这个是切换照片时调用
void divoom_lvgl_photo_sticker_read_time_handle(lv_timer_t *timer)
{
    Pdivoom_lvgl_photo_stricker_ctrl phandle = (Pdivoom_lvgl_photo_stricker_ctrl)lv_timer_get_user_data(timer);

    if(phandle->pic_num > phandle->pcur_active_item->pos_num && phandle->pslide_config)
    {
        phandle->run_tick ++;
        //视频只要播放结束就可以启动，照片播放固定的时间
        if(phandle->run_tick >= phandle->pslide_config->play_time / 100 && phandle->end_flag)
        {
            phandle->run_tick = 0;
            phandle->update_display_flag = 1;
            divoom_system_put_sem(phandle->pread_sem);
        }
    }

}

void divoom_lvgl_photo_sticker_fade_in_anim_cb(void * a, int32_t v)
{
    Pdivoom_lvgl_photo_stricker_ctrl phandle = (Pdivoom_lvgl_photo_stricker_ctrl)a;
    
    lv_obj_set_style_img_opa(phandle->panim_cur_image_obj, v, 0);
    lv_obj_set_style_img_opa(phandle->panim_pre_image_obj, (LV_OPA_100 -v), 0);
    if(v == LV_OPA_100)
    {
        phandle->end_flag = 1;
    }
}

void divoom_lvgl_photo_sticker_anim_push_down(void * a, int32_t v)
{
    Pdivoom_lvgl_photo_stricker_ctrl phandle = (Pdivoom_lvgl_photo_stricker_ctrl)a;



    lv_obj_set_style_img_opa(phandle->panim_cur_image_obj, v , 0);
    lv_obj_set_y(phandle->panim_cur_image_obj, phandle->pcur_active_item->pos_info_table[phandle->cur_update_pos].h * (v - 255)/255);
    lv_obj_set_style_img_opa(phandle->panim_pre_image_obj, LV_OPA_100  - v, 0);
    lv_obj_set_y(phandle->panim_pre_image_obj, phandle->pcur_active_item->pos_info_table[phandle->cur_update_pos].h *v/255);
    if(v == 255)
    {
        lv_obj_add_flag(phandle->panim_pre_image_obj, LV_OBJ_FLAG_HIDDEN);
        phandle->end_flag = 1;
    }
}

void divoom_lvgl_photo_sticker_anim_push_up(void * a, int32_t v)
{
    Pdivoom_lvgl_photo_stricker_ctrl phandle = (Pdivoom_lvgl_photo_stricker_ctrl)a;



    lv_obj_set_style_img_opa(phandle->panim_cur_image_obj, v , 0);
    lv_obj_set_y(phandle->panim_cur_image_obj, phandle->pcur_active_item->pos_info_table[phandle->cur_update_pos].h * (255 - v)/255);
    lv_obj_set_style_img_opa(phandle->panim_pre_image_obj, LV_OPA_100  - v, 0);
    lv_obj_set_y(phandle->panim_pre_image_obj, phandle->pcur_active_item->pos_info_table[phandle->cur_update_pos].h *(0 - v)/255);
    if(v == 255)
    {
        lv_obj_add_flag(phandle->panim_pre_image_obj, LV_OBJ_FLAG_HIDDEN);
        phandle->end_flag = 1;
    }
}

void divoom_lvgl_photo_sticker_anim_push_right(void * a, int32_t v)
{
    Pdivoom_lvgl_photo_stricker_ctrl phandle = (Pdivoom_lvgl_photo_stricker_ctrl)a;



    lv_obj_set_style_img_opa(phandle->panim_cur_image_obj, v, 0);
    lv_obj_set_x(phandle->panim_cur_image_obj, phandle->pcur_active_item->pos_info_table[phandle->cur_update_pos].w * (v - 255)/255);

    lv_obj_set_style_img_opa(phandle->panim_pre_image_obj, LV_OPA_100  - v, 0);
    lv_obj_set_x(phandle->panim_pre_image_obj, phandle->pcur_active_item->pos_info_table[phandle->cur_update_pos].w * v / 255);
    if(v == 255)
    {
        lv_obj_add_flag(phandle->panim_pre_image_obj, LV_OBJ_FLAG_HIDDEN);
        phandle->end_flag = 1;
    }
}

void divoom_lvgl_photo_sticker_anim_push_left(void * a, int32_t v)
{
    Pdivoom_lvgl_photo_stricker_ctrl phandle = (Pdivoom_lvgl_photo_stricker_ctrl)a;



    lv_obj_set_style_img_opa(phandle->panim_cur_image_obj, v, 0);
    lv_obj_set_x(phandle->panim_cur_image_obj, phandle->pcur_active_item->pos_info_table[phandle->cur_update_pos].w * (255 - v)/255);

    lv_obj_set_style_img_opa(phandle->panim_pre_image_obj, LV_OPA_100  - v, 0);
    lv_obj_set_x(phandle->panim_pre_image_obj, phandle->pcur_active_item->pos_info_table[phandle->cur_update_pos].w * (0 - v) / 255);
    if(v == 255)
    {
        lv_obj_add_flag(phandle->panim_pre_image_obj, LV_OBJ_FLAG_HIDDEN);
        phandle->end_flag = 1;
    }
}

void divoom_lvgl_photo_sticker_anim_blinds(void * a, int32_t v)
{
    int area_w, area_h;
    Pdivoom_lvgl_photo_stricker_ctrl phandle = (Pdivoom_lvgl_photo_stricker_ctrl)a;

    area_w = phandle->pcur_active_item->pos_info_table[phandle->cur_update_pos].w;
    area_h = phandle->pcur_active_item->pos_info_table[phandle->cur_update_pos].h;
    if(phandle->blinds_index == 0)
    {
        divoom_app_com_set_image_opa((uint8 *)phandle->panim_cur_image_buf, phandle->refresh_num, 0, (v - phandle->refresh_num), area_h, 0xFF, area_w, area_h);
        divoom_app_com_set_image_opa((uint8 *)phandle->panim_pre_image_buf, phandle->refresh_num, 0, (v - phandle->refresh_num), area_h, 0, area_w, area_h);
    }
    else if(phandle->blinds_index == 1)
    {
        divoom_app_com_set_image_opa((uint8 *)phandle->panim_cur_image_buf, area_w  - v, 0, (v - phandle->refresh_num), area_h, 0xFF, area_w, area_h);
        divoom_app_com_set_image_opa((uint8 *)phandle->panim_pre_image_buf, area_w  - v, 0, (v - phandle->refresh_num), area_h, 0, area_w, area_h);
    }
    else if(phandle->blinds_index == 2)
    {
        divoom_app_com_set_image_opa((uint8 *)phandle->panim_cur_image_buf, 0, phandle->refresh_num, area_w, (v - phandle->refresh_num), 0xFF, area_w, area_h);
        divoom_app_com_set_image_opa((uint8 *)phandle->panim_pre_image_buf, 0, phandle->refresh_num, area_w, (v - phandle->refresh_num), 0, area_w, area_h);
    }
    else
    {
        divoom_app_com_set_image_opa((uint8 *)phandle->panim_cur_image_buf, 0, area_h - v, area_w, (v - phandle->refresh_num), 0xFF, area_w, area_h);
        divoom_app_com_set_image_opa((uint8 *)phandle->panim_pre_image_buf, 0, area_h - v, area_w, (v - phandle->refresh_num), 0, area_w, area_h);
    }
    phandle->refresh_num = v;

    if((v == area_w && phandle->blinds_index < 2) || (v == area_h && phandle->blinds_index >= 2))
    {
        lv_obj_add_flag(phandle->panim_pre_image_obj, LV_OBJ_FLAG_HIDDEN);
        phandle->end_flag = 1;
    }
    lv_obj_invalidate(phandle->panim_pre_image_obj);
    lv_obj_invalidate(phandle->panim_cur_image_obj);
}

void divoom_lvgl_photo_sticker_anim_mosaic(void * a, int32_t v)
{
    int index, i, j, row_num, col_num;
    int area_w, area_h;
    Pdivoom_lvgl_photo_stricker_ctrl phandle = (Pdivoom_lvgl_photo_stricker_ctrl)a;

    area_w = phandle->pcur_active_item->pos_info_table[phandle->cur_update_pos].w;
    area_h = phandle->pcur_active_item->pos_info_table[phandle->cur_update_pos].h;


    row_num = (phandle->pcur_active_item->pos_info_table[phandle->cur_update_pos].w + 9)/10;
    if(v == LV_IMG_ZOOM_NONE)
    {
        int last_num = divoom_random_get_no_use(phandle->pmosaic_random_handle);
        for(j = 0; j < last_num; j++)
        {
            index = divoom_random_get(phandle->pmosaic_random_handle);
            divoom_app_com_set_image_opa((uint8 *)phandle->panim_cur_image_buf, (index%row_num)*10, (index/row_num)*10, 10, 10, 0xFF, area_w, area_h);
            divoom_app_com_set_image_opa((uint8 *)phandle->panim_pre_image_buf, (index%row_num)*10, (index/row_num)*10, 10, 10, 0, area_w, area_h);
        }
        divoom_random_destroy(phandle->pmosaic_random_handle);
        lv_obj_add_flag(phandle->panim_pre_image_obj, LV_OBJ_FLAG_HIDDEN);
        phandle->pmosaic_random_handle = NULL;
        phandle->end_flag = 1;
    }
    else
    {
        int refresh_num;
        
        col_num = (phandle->pcur_active_item->pos_info_table[phandle->cur_update_pos].h + 9)/10;
        refresh_num = v*row_num*col_num/LV_IMG_ZOOM_NONE;
        for(i =0; i < refresh_num - phandle->refresh_num; i++)
        {
            index = divoom_random_get(phandle->pmosaic_random_handle);
            divoom_app_com_set_image_opa((uint8 *)phandle->panim_cur_image_buf, (index%row_num)*10, (index/row_num)*10, 10, 10, 0xFF, area_w, area_h);
            divoom_app_com_set_image_opa((uint8 *)phandle->panim_pre_image_buf, (index%row_num)*10, (index/row_num)*10, 10, 10, 0, area_w, area_h);
        }
        phandle->refresh_num = refresh_num;
    }
    lv_obj_invalidate(phandle->panim_pre_image_obj);
    lv_obj_invalidate(phandle->panim_cur_image_obj);

}

void divoom_lvgl_photo_sticker_disp_image_cout(Pdivoom_lvgl_photo_stricker_ctrl phandle)
{
    int i, slide_mode;
    lv_img_dsc_t *tmp_image_data;
    void *ptmp_item;
        
    
    lv_img_cache_invalidate_src(NULL);

    if(phandle->pimage_info.p_next_handle[phandle->cur_update_pos]->play_gif_handle)
    {
        divoom_lvgl_play_gif_file_start_play(phandle->pimage_info.p_next_handle[phandle->cur_update_pos]->play_gif_handle);
    }

    phandle->panim_cur_image_obj = phandle->pimage_info.p_next_handle[phandle->cur_update_pos]->img_obj;
    phandle->panim_cur_image_buf = (uint8 *)phandle->pimage_info.p_next_handle[phandle->cur_update_pos]->image_data.data;
    phandle->panim_pre_image_obj = phandle->pimage_info.p_cur_handle[phandle->cur_update_pos]->img_obj;
    phandle->panim_pre_image_buf = (uint8 *)phandle->pimage_info.p_cur_handle[phandle->cur_update_pos]->image_data.data;
    

    
    ptmp_item = phandle->pimage_info.p_cur_handle[phandle->cur_update_pos];
    phandle->pimage_info.p_cur_handle[phandle->cur_update_pos] = phandle->pimage_info.p_next_handle[phandle->cur_update_pos];
    phandle->pimage_info.p_next_handle[phandle->cur_update_pos] = ptmp_item;
    
    
    lv_anim_set_var(&phandle->image_anim, phandle);
    
    lv_anim_path_linear(&phandle->image_anim);
    lv_anim_set_path_cb(&phandle->image_anim, lv_anim_path_linear);
    slide_mode = phandle->pslide_config->effect_type;
    if(slide_mode == DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_MIX)
    {
        int slide_list[] ={
            DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_FADE_OUT,   //渐隐方式
            DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_PUSH,    //推送方式
            DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_BLINDS,      //百叶效果
            DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_MOSAIC,      //马赛克效果
        };
        if(phandle->pmix_rand_handle == NULL)
        {
            phandle->pmix_rand_handle = divoom_random_create(4);
        }
        slide_mode = slide_list[divoom_random_get(phandle->pmix_rand_handle)];
    }
    lv_obj_set_style_img_opa(phandle->panim_cur_image_obj, LV_OPA_100, 0);
    lv_obj_clear_flag(phandle->panim_cur_image_obj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_pos(phandle->panim_cur_image_obj, 0, 0);
    lv_obj_set_style_img_opa(phandle->panim_pre_image_obj, LV_OPA_100, 0);
    lv_obj_clear_flag(phandle->panim_pre_image_obj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_pos(phandle->panim_pre_image_obj, 0, 0);

    divoom_print("slide_mode: %d\n", slide_mode);
    switch(slide_mode)
    {
        case DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_FADE_OUT:
            lv_anim_set_values(&phandle->image_anim, 0, LV_OPA_100);
            lv_obj_set_style_img_opa(phandle->panim_pre_image_obj, LV_OPA_100, 0);
            lv_obj_set_style_img_opa(phandle->panim_cur_image_obj, 0, 0);
            lv_anim_set_exec_cb(&phandle->image_anim, (lv_anim_exec_xcb_t) divoom_lvgl_photo_sticker_fade_in_anim_cb);
            break;
            
        case DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_PUSH:
            phandle->push_index = divoom_app_com_get_rand_index(phandle->push_index, 4);
            lv_anim_set_values(&phandle->image_anim, 0, 255);
            if(phandle->push_index == 0)
            {
                lv_anim_set_exec_cb(&phandle->image_anim, (lv_anim_exec_xcb_t) divoom_lvgl_photo_sticker_anim_push_up);
            }
            else if(phandle->push_index == 1)
            {
                lv_anim_set_exec_cb(&phandle->image_anim, (lv_anim_exec_xcb_t) divoom_lvgl_photo_sticker_anim_push_down);
            }
            else if(phandle->push_index == 2)
            {
                lv_anim_set_exec_cb(&phandle->image_anim, (lv_anim_exec_xcb_t) divoom_lvgl_photo_sticker_anim_push_left);
            }
            else 
            {
                lv_anim_set_exec_cb(&phandle->image_anim, (lv_anim_exec_xcb_t) divoom_lvgl_photo_sticker_anim_push_right);
            }
            break;
            
        case DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_BLINDS:
            phandle->blinds_index = divoom_app_com_get_rand_index(phandle->blinds_index, 4);
            divoom_app_com_set_image_opa((uint8 *)phandle->panim_cur_image_buf, 0, 0, phandle->pcur_active_item->pos_info_table[phandle->cur_update_pos].w, 
                phandle->pcur_active_item->pos_info_table[phandle->cur_update_pos].h, 0, phandle->pcur_active_item->pos_info_table[phandle->cur_update_pos].w, phandle->pcur_active_item->pos_info_table[phandle->cur_update_pos].h);
            if(phandle->blinds_index < 2)
            {
                lv_anim_set_values(&phandle->image_anim, 0, phandle->pcur_active_item->pos_info_table[phandle->cur_update_pos].w);
            }
            else
            {
                lv_anim_set_values(&phandle->image_anim, 0, phandle->pcur_active_item->pos_info_table[phandle->cur_update_pos].h);
            }
            lv_anim_set_exec_cb(&phandle->image_anim, (lv_anim_exec_xcb_t) divoom_lvgl_photo_sticker_anim_blinds);
            break;
            
        case DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_MOSAIC:
            if(phandle->pmosaic_random_handle)
            {
                divoom_random_destroy(phandle->pmosaic_random_handle);
                phandle->pmosaic_random_handle = NULL;
            }
            phandle->pmosaic_random_handle = divoom_random_create(((phandle->pcur_active_item->pos_info_table[phandle->cur_update_pos].w +9)/10) * ((phandle->pcur_active_item->pos_info_table[phandle->cur_update_pos].h + 9)/10));
            phandle->refresh_num = 0;
            divoom_app_com_set_image_opa((uint8 *)phandle->panim_cur_image_buf, 0, 0, phandle->pcur_active_item->pos_info_table[phandle->cur_update_pos].w, 
                phandle->pcur_active_item->pos_info_table[phandle->cur_update_pos].h, 0, phandle->pcur_active_item->pos_info_table[phandle->cur_update_pos].w, phandle->pcur_active_item->pos_info_table[phandle->cur_update_pos].h);
            lv_anim_set_values(&phandle->image_anim, 0, LV_IMG_ZOOM_NONE);
            lv_anim_set_exec_cb(&phandle->image_anim, (lv_anim_exec_xcb_t) divoom_lvgl_photo_sticker_anim_mosaic);
            break;
            
        case DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_NORMAL:
            lv_obj_add_flag(phandle->panim_pre_image_obj, LV_OBJ_FLAG_HIDDEN);
            break;
            
    }
    lv_anim_set_time(&phandle->image_anim, 2000);
    if(slide_mode != DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_NORMAL)
    {
        lv_anim_start(&phandle->image_anim);
    }
    

}

//这个是读完数据后进行调用来更新图像
void divoom_lvgl_photo_sticker_refresh_image_time_handle(lv_timer_t *timer)
{
    int i;
    Pdivoom_lvgl_photo_stricker_ctrl phandle = (Pdivoom_lvgl_photo_stricker_ctrl)lv_timer_get_user_data(timer);
    
    if(phandle->update_display_flag != 2)
    {
        return;
    }
    phandle->update_display_flag = 0;
    divoom_lvgl_photo_sticker_disp_image_cout(phandle);
}

void *divoom_lvgl_photo_sticker_init(void *user_data, int menu_id)
{
    int i;
    lv_img_dsc_t *image_data_handle;
    Pdivoom_lvgl_photo_slide_effect_info para_info = (Pdivoom_lvgl_photo_slide_effect_info)user_data;
	Pdivoom_lvgl_photo_stricker_pos_info pcur_pos_info;
	Pdivoom_lvgl_photo_stricker_ctrl phandle;

	phandle = divoom_system_malloc(sizeof(divoom_lvgl_photo_stricker_ctrl));
    phandle->menu_id = menu_id;
    phandle->album_id = para_info->album_id;
    phandle->run_flag = 1;
    phandle->update_display_flag = 0;    
    phandle->pread_sem = divoom_system_create_sem(1, 0);
    phandle->pexit_sem = divoom_system_create_sem(1, 0);
    phandle->pslide_config = divoom_pic_phone_manage_get_slide_config();
	phandle->pcur_active_item = divoom_lvgl_photo_sticker_get_active_effect(phandle->pslide_config);	
    phandle->pimage_info.image_nun = phandle->pcur_active_item->pos_num;
	for(i = 0; i < phandle->pimage_info.image_nun; i++)
	{
    	phandle->pimage_info.p_cur_handle[i] = divoom_system_malloc(sizeof(divoom_lvgl_photo_stricker_image_item));
    	phandle->pimage_info.p_next_handle[i] = divoom_system_malloc(sizeof(divoom_lvgl_photo_stricker_image_item));
	}
    phandle->main_obj = lv_btn_create(NULL); 
    lv_obj_set_pos(phandle->main_obj, 0, 0);
    lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    divoom_menu_control_init_btn_stype(NULL, NULL, &phandle->main_style, NULL, NULL, NULL, 0);
    lv_style_set_pad_top(&phandle->main_style, 0);
    lv_style_set_pad_bottom(&phandle->main_style, 0);
    lv_style_set_pad_left(&phandle->main_style, 0);
    lv_style_set_pad_right(&phandle->main_style, 0);
    lv_obj_add_style(phandle->main_obj, &phandle->main_style, 0);
    if(strlen(phandle->pcur_active_item->bg_image_addr))
    {
        void *bg_img_obj = lv_img_create(phandle->main_obj);
        lv_obj_set_size(bg_img_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
        divoom_menu_control_lv_img_set_src(bg_img_obj, phandle->pcur_active_item->bg_image_addr);
    }
    
    
    phandle->pget_pic_handle = divoom_pic_album_manage_create_disp_order_handle(para_info->album_id, DIVOOM_PIC_PHOTO_SLIDE_DISP_STRICKER, phandle->pslide_config->disp_reverse, 0);
    
    phandle->pic_num = divoom_pic_album_manage_get_disp_order_pic_num(phandle->pget_pic_handle);
	pcur_pos_info = phandle->pcur_active_item->pos_info_table;
	phandle->prand_handle = divoom_random_create(phandle->pimage_info.image_nun);
    phandle->cur_update_pos = -1;
	for(i = 0; i < phandle->pimage_info.image_nun; i++)
	{
        void *cout_obj = lv_obj_create(phandle->main_obj);
        lv_obj_add_style(cout_obj, &phandle->main_style, 0);
    	lv_obj_set_style_border_width(cout_obj, 0, 0);
    	lv_obj_set_style_pad_all(cout_obj, 0, 0);
	    lv_obj_set_pos(cout_obj, pcur_pos_info[i].x, pcur_pos_info[i].y);
	    lv_obj_set_size(cout_obj, pcur_pos_info[i].w, pcur_pos_info[i].h);
        lv_obj_set_style_transform_pivot_x(cout_obj, pcur_pos_info[i].w/2, 0);
        lv_obj_set_style_transform_pivot_y(cout_obj, pcur_pos_info[i].h/2, 0);
        lv_obj_set_scrollbar_mode(cout_obj, LV_SCROLLBAR_MODE_OFF);
		if(pcur_pos_info[i].angle)
		{
			// 设置角度（0-3600，实际角度需要除以10）
            lv_obj_set_style_transform_angle(cout_obj, pcur_pos_info[i].angle*10, 0);
			
		}

        phandle->pimage_info.p_cur_handle[i]->image_info = &phandle->pimage_info.p_cur_handle[i]->raw_info;
        divoom_lv_img_dsc_init_fmt(&phandle->pimage_info.p_cur_handle[i]->image_data,
            pcur_pos_info[i].w, pcur_pos_info[i].h, LV_COLOR_FORMAT_RGB565A8, NULL);
		phandle->pimage_info.p_cur_handle[i]->image_data.data = divoom_system_malloc(phandle->pimage_info.p_cur_handle[i]->image_data.data_size);  
		phandle->pimage_info.p_cur_handle[i]->img_obj = lv_img_create(cout_obj);
	    lv_obj_set_size(phandle->pimage_info.p_cur_handle[i]->img_obj, pcur_pos_info[i].w, pcur_pos_info[i].h);
	    lv_img_set_src(phandle->pimage_info.p_cur_handle[i]->img_obj, &phandle->pimage_info.p_cur_handle[i]->image_data);
        lv_obj_center(phandle->pimage_info.p_cur_handle[i]->img_obj);
		
        phandle->pimage_info.p_next_handle[i]->image_info = &phandle->pimage_info.p_next_handle[i]->raw_info;
        divoom_lv_img_dsc_init_fmt(&phandle->pimage_info.p_next_handle[i]->image_data,
            pcur_pos_info[i].w, pcur_pos_info[i].h, LV_COLOR_FORMAT_RGB565A8, NULL);
		phandle->pimage_info.p_next_handle[i]->image_data.data = divoom_system_malloc(phandle->pimage_info.p_next_handle[i]->image_data.data_size);  
		phandle->pimage_info.p_next_handle[i]->img_obj = lv_img_create(cout_obj);
		lv_img_set_src(phandle->pimage_info.p_next_handle[i]->img_obj, &phandle->pimage_info.p_next_handle[i]->image_data);
	    lv_obj_set_size(phandle->pimage_info.p_next_handle[i]->img_obj, pcur_pos_info[i].w, pcur_pos_info[i].h);
        lv_obj_set_style_img_opa(phandle->pimage_info.p_next_handle[i]->img_obj, 0, 0);
        lv_obj_center(phandle->pimage_info.p_next_handle[i]->img_obj);
		
	}
    if(strlen(phandle->pcur_active_item->fr_image_addr))
    {
        void *bg_img_obj = lv_img_create(phandle->main_obj);
        lv_obj_set_size(bg_img_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
        divoom_menu_control_lv_img_set_src(bg_img_obj, phandle->pcur_active_item->fr_image_addr);
    	lv_obj_move_foreground(bg_img_obj);
    }
    
    divoom_system_create_task(divoom_lvgl_photo_sticker_update_image_buffer, "lv_sticker", phandle, DIVOOM_TASK_STACK_LVGL_STICKER);
    
    lv_anim_init(&phandle->image_anim);
    
	lv_obj_clear_flag(phandle->main_obj, LV_OBJ_FLAG_SCROLLABLE);
    
    
	for(i = 0; i < phandle->pimage_info.image_nun; i++)
	{
        divoom_lvgl_photo_stricker_load_image_init_buffer(phandle, i, phandle->pimage_info.p_cur_handle[i], 0);
        lv_obj_set_style_img_opa(phandle->pimage_info.p_cur_handle[i]->img_obj, LV_OPA_100, 0);
	}
    phandle->run_tick = 0;
    phandle->end_flag  = 1;
    phandle->read_timer = lv_timer_create(divoom_lvgl_photo_sticker_read_time_handle, 100, phandle);
    
    lv_timer_pause(phandle->read_timer);        
    phandle->refresh_timer = lv_timer_create(divoom_lvgl_photo_sticker_refresh_image_time_handle, 16, phandle);
    lv_timer_pause(phandle->refresh_timer);   
    
    //lv_obj_add_event_cb(phandle->main_obj, divoom_lvgl_photo_fade_out_event_cb,
    //    LV_EVENT_ALL, phandle);
    return phandle;


}

void divoom_lvgl_photo_sticker_disp(void *handle, void *active_obj)
{
    Pdivoom_lvgl_photo_stricker_ctrl phandle = (Pdivoom_lvgl_photo_stricker_ctrl)handle;
    
    if(phandle->refresh_timer)
    {
        lv_timer_resume(phandle->refresh_timer);
    }
    if(phandle->read_timer)
    {
        lv_timer_resume(phandle->read_timer);
    }
    lv_scr_load(phandle->main_obj);
}

void divoom_lvgl_photo_sticker_pause(void *handle, int exit_flag)
{
    Pdivoom_lvgl_photo_stricker_ctrl phandle = (Pdivoom_lvgl_photo_stricker_ctrl)handle;
    
    if(phandle)
    {
        lv_anim_del(phandle, NULL);
        if(phandle->refresh_timer)
        {
            lv_timer_pause(phandle->refresh_timer);
        }
        if(phandle->read_timer)
        {
            lv_timer_pause(phandle->read_timer);
        }
        
    }
}

void divoom_lvgl_photo_sticker_exit(void *handle)
{
    int last_pic_id = 0;
    Pdivoom_lvgl_photo_stricker_ctrl phandle = (Pdivoom_lvgl_photo_stricker_ctrl)handle;
    
    lv_anim_del(phandle, NULL);
    if(phandle->refresh_timer)
    {
        lv_timer_del(phandle->refresh_timer);
    }
    if(phandle->read_timer)
    {
        lv_timer_del(phandle->read_timer);
    }
    phandle->run_flag = 0;
    divoom_system_put_sem(phandle->pread_sem);
    divoom_system_put_sem(phandle->pread_sem);
    divoom_free_lvgl_lock();
    divoom_system_get_sem(phandle->pexit_sem);
    divoom_get_lvgl_lock();
    divoom_system_destroy_sem(phandle->pexit_sem);
    //在这里清除所有信息
    divoom_system_destroy_sem(phandle->pread_sem);
    divoom_pic_album_manage_destory_disp_order_handle(phandle->pget_pic_handle);
    divoom_print("exit all!\n");
    
    if(phandle->cur_update_pos>=0 && phandle->cur_update_pos < phandle->pimage_info.image_nun)
    {
        if(phandle->pimage_info.p_cur_handle[phandle->cur_update_pos] && phandle->pimage_info.p_cur_handle[phandle->cur_update_pos]->image_info)
        {
            last_pic_id = phandle->pimage_info.p_cur_handle[phandle->cur_update_pos]->image_info->id;
        }
    }
    else if(phandle->pimage_info.p_cur_handle[0] && phandle->pimage_info.p_cur_handle[0]->image_info)
    {
        last_pic_id = phandle->pimage_info.p_cur_handle[0]->image_info->id;
    }
    if(last_pic_id > 0)
    {
        divoom_pic_album_manage_update_last_play_album_pic_id(phandle->album_id, last_pic_id);
    }

    for(int i = 0; i < phandle->pcur_active_item->pos_num; i++)
    {
        if(phandle->pimage_info.p_cur_handle[i]->play_gif_handle)
        {
            divoom_lvgl_play_gif_file_destory_handle(phandle->pimage_info.p_cur_handle[i]->play_gif_handle, 0);
            phandle->pimage_info.p_cur_handle[i]->play_gif_handle = NULL;
        }
        divoom_system_free((uint8 *)phandle->pimage_info.p_cur_handle[i]->image_data.data);
        divoom_system_free(phandle->pimage_info.p_cur_handle[i]);
        
        if(phandle->pimage_info.p_next_handle[i]->play_gif_handle)
        {
            divoom_lvgl_play_gif_file_destory_handle(phandle->pimage_info.p_next_handle[i]->play_gif_handle, 0);
            phandle->pimage_info.p_next_handle[i]->play_gif_handle = NULL;
        }
        divoom_system_free((uint8 *)phandle->pimage_info.p_next_handle[i]->image_data.data);
        divoom_system_free(phandle->pimage_info.p_next_handle[i]);
    }
    lv_style_reset(&phandle->main_style);
    lv_obj_del(phandle->main_obj);
    divoom_random_destroy(phandle->prand_handle);
    if(phandle->pmix_rand_handle)
    {
        divoom_random_destroy(phandle->pmix_rand_handle);
    }
    if(phandle->pmosaic_random_handle)
    {
        divoom_random_destroy(phandle->pmosaic_random_handle);
    }
    divoom_system_free(phandle);
}

void divoom_lvgl_photo_sticker_get_pic_info(void *handle, Pdivoom_lvgl_photo_view_para_info pic_info)
{
    Pdivoom_lvgl_photo_stricker_ctrl phandle = (Pdivoom_lvgl_photo_stricker_ctrl)handle;
    
    if(phandle == NULL || phandle->pic_num == 0)
    {
        return;
    }
    pic_info->pic_id = 0;
    if(phandle->cur_update_pos>=0 && phandle->cur_update_pos < phandle->pimage_info.image_nun)
    {
        if(phandle->pimage_info.p_cur_handle[phandle->cur_update_pos] && phandle->pimage_info.p_cur_handle[phandle->cur_update_pos]->image_info)
        {
            pic_info->pic_id = phandle->pimage_info.p_cur_handle[phandle->cur_update_pos]->image_info->id;
        }
    }
    else if(phandle->pimage_info.p_cur_handle[0] && phandle->pimage_info.p_cur_handle[0]->image_info)
    {
        pic_info->pic_id = phandle->pimage_info.p_cur_handle[0]->image_info->id;
    }
    pic_info->pic_num = &phandle->pic_num;
    pic_info->pget_pic_handle = phandle->pget_pic_handle;
    pic_info->pdel_callback = NULL;
}

void *divoom_lvgl_photo_sticker_get_lvgl_obj(void *handle)
{
    Pdivoom_lvgl_photo_stricker_ctrl phandle = (Pdivoom_lvgl_photo_stricker_ctrl)handle;
    
    return phandle->main_obj;
}

void divoom_lvgl_photo_sticker_update_disp(void *handle)
{
    Pdivoom_lvgl_photo_stricker_ctrl phandle = (Pdivoom_lvgl_photo_stricker_ctrl)handle;

	for(int i = 0; i < phandle->pimage_info.image_nun; i++)
	{
        divoom_lvgl_photo_stricker_load_image_init_buffer(phandle, i, phandle->pimage_info.p_cur_handle[i], 0);
	}

}
void divoom_lvgl_photo_sticker_update_time(void *handle, int play_time)
{
}

void divoom_lvgl_photo_sticker_remove_notice(void *handle, uint32 *pic_list, int pic_num)
{
    Pdivoom_lvgl_photo_stricker_ctrl phandle = (Pdivoom_lvgl_photo_stricker_ctrl)handle;

    for(int j = 0; j < pic_num; j++)
    {
    	for(int i = 0; i < phandle->pcur_active_item->pos_num && phandle->run_flag; i++)
    	{
            if(phandle->pimage_info.p_cur_handle[i]->image_info->id == pic_list[j] && phandle->pimage_info.p_cur_handle[i]->play_gif_handle)
            {
                divoom_lvgl_play_gif_file_destory_handle(phandle->pimage_info.p_cur_handle[i]->play_gif_handle, 0);
                phandle->pimage_info.p_cur_handle[i]->play_gif_handle = NULL;
            }
            if(phandle->pimage_info.p_next_handle[i]->image_info->id == pic_list[j] && phandle->pimage_info.p_next_handle[i]->play_gif_handle)
            {
                divoom_lvgl_play_gif_file_destory_handle(phandle->pimage_info.p_next_handle[i]->play_gif_handle, 0);
                phandle->pimage_info.p_next_handle[i]->play_gif_handle = NULL;
            }
    	}
        divoom_pic_album_manage_remove_pic_id_from_disp_handle(phandle->pget_pic_handle, pic_list[j]);
        phandle->pic_num --;        
    }
    
    if(phandle->use_pos >= phandle->pic_num)
    {
        phandle->use_pos = phandle->pic_num - 1;
    }
}
