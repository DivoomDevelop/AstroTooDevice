#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_lvgl_font_manage.h"
#include "divoom_lvgl_dial_menu.h"
#include "divoom_lvgl_top_danmaku_menu.h"
#include "divoom_pixel_format.h"

#define DIVOOM_DANMAKU_DISP_LABEL_AREA				80
#define DIVOOM_DANMAKU_LABEL_PAD_ALL 					17
#define DIVOOM_DANMAKU_LABEL_TAIL_LENGTH 				8
#define DIVOOM_DANMAKU_LABEL_CONT_PAD_ALL 		(DIVOOM_DANMAKU_LABEL_TAIL_LENGTH	+ DIVOOM_DANMAKU_LABEL_PAD_ALL)

/* SendText 同屏最大数量由 12 减半为 6，达到上限时继续淘汰最早弹幕。 */
#define DIVOOM_LVGL_TOP_DANMAKU_MENU_MAX_LABEL_NUM  	    		6
#define DIVOOM_LVGL_TOP_DANMAKU_MENU_MAX_LABEL_COLOR_NUM  	 		20    		
#define DIVOOM_LVGL_TOP_DANMAKU_MENU_MAX_FACE_NUM   		36
#define DIVOOM_LVGL_TOP_DANMAKU_MENU_ANNIVERSARY_MAX_FACE_NUM       		23
/* SendFace 资源已离线缩放为 100x100，随机坐标和重叠占位必须使用同一尺寸。 */
#define DIVOOM_LVGL_TOP_DANMAKU_MENU_FACE_SIZE                         100

//#define DIVOOM_LVGL_TOP_DANMAKU_MENU_MAX_LINE_NUM		  	7

typedef struct _divoom_lvgl_top_danmaku_menu_label_user_item
{
	int user_id;
	int bg_color_index;
}divoom_lvgl_top_danmaku_menu_label_user_item, *Pdivoom_lvgl_top_danmaku_menu_label_user_item;


typedef struct _divoom_lvgl_top_danmaku_menu_ctrl
{
	void *main_obj;		
	
	//Pdivoom_lvgl_dial_menu_image_ctrl pimage_handle;
	//void *disp_image_queue;
	
	int change_flag;
	int next_update_time;
	int image_start_tick;
	void *image_obj[2];
	lv_timer_t *image_refresh_timer;
	int image_time_tick[2];
	int pre_face_id[2];
	void *image_queue;

	void *label_obj[DIVOOM_LVGL_TOP_DANMAKU_MENU_MAX_LABEL_NUM];	//弹幕对象

	int label_angle[DIVOOM_LVGL_TOP_DANMAKU_MENU_MAX_LABEL_NUM];
	int label_time_tick[DIVOOM_LVGL_TOP_DANMAKU_MENU_MAX_LABEL_NUM];
	lv_timer_t *label_refresh_timer;
	int used_color_index[DIVOOM_LVGL_TOP_DANMAKU_MENU_MAX_LABEL_COLOR_NUM];
	void *label_user_queue;
}divoom_lvgl_top_danmaku_menu_ctrl, *Pdivoom_lvgl_top_danmaku_menu_ctrl;

Pdivoom_lvgl_top_danmaku_menu_ctrl Pdivoom_lvgl_top_danmaku_menu_handle = NULL;

const int divoom_top_danmaku_label_bg_color_list[] = {
	0xFFFFFF, 0xF4A4A4, 0x9EDBD0, 0xF7B19A, 0x98D1EA,
	0xF7C28D, 0xA8C0F2, 0xF1D488, 0xA7CFC0, 0xE8E08F,
	0xB2A8F0, 0xCBE691, 0xCFA6E5, 0xACE5A7, 0xDEA6D6,
	0x9FE0BD, 0xE6A6B7, 0x97D9E3, 0xE9D5B5, 0xBFD1E0,
};

const int divoom_top_danmaku_anniversary_img_list_index[DIVOOM_LVGL_TOP_DANMAKU_MENU_ANNIVERSARY_MAX_FACE_NUM] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 26, 27, 28, 29, 30, 32, 33, 34, 35,
};


const char *divoom_top_danmaku_img_list[] = {
	"83/83_face_0.png",    "83/83_face_1.png",     "83/83_face_2.png",
	"83/83_face_3.png",    "83/83_face_4.png",     "83/83_face_5.png",
	"83/83_face_6.png",    "83/83_face_7.png",     "83/83_face_8.png",
	"83/83_face_9.png",    "83/83_face_10.png",    "83/83_face_11.png",
	"83/83_face_12.png",  "83/83_face_13.png",    "83/83_face_14.png",
	"83/83_face_15.png",  "83/83_face_16.png",    "83/83_face_17.png",
	"83/83_face_18.png",  "83/83_face_19.png",    "83/83_face_20.png",
	"83/83_face_21.png",  "83/83_face_22.png",    "83/83_face_23.png",
	"83/83_face_24.png",  "83/83_face_25.png",    "83/83_face_26.png",
	"83/83_face_27.png",  "83/83_face_28.png",    "83/83_face_29.png",
	"83/83_face_30.png",  "83/83_face_31.png",    "83/83_face_32.png",
	"83/83_face_33.png",  "83/83_face_34.png",    "83/83_face_35.png",
};


//extern void *divoom_lvgl_dial_menu_open_resource_handle(char *file_name, int *resource_type, int *out_w, int *out_h, int *out_num);
//extern void divoom_lvgl_dial_menu_release_resource_handle(void *pwebp_handle, int resource_type);

#if 0
void divoom_lvgl_top_danmaku_menu_close_img(Pdivoom_lvgl_dial_menu_image_ctrl pimage_handle)
{
	if(pimage_handle->disp_time_handle)
	{
		lv_timer_del(pimage_handle->disp_time_handle);
		pimage_handle->disp_time_handle = NULL;
	}
	lv_img_cache_invalidate_src(pimage_handle->lvgl_image_buf);  
	lv_obj_del(pimage_handle->lvgl_image_obj);

	if(pimage_handle->image_type == DIVOOM_LVGL_DIAL_MENU_TYPE_LOCAL_IMAGE)
    {
		divoom_lvgl_dial_menu_release_resource_handle(pimage_handle->pwebp_handle, pimage_handle->resource_type);
    }
    else if(pimage_handle->image_type == DIVOOM_LVGL_DIAL_MENU_TYPE_DIVOOM_IMAGE)
    {
		divoom_http_download_image_free_image_handle(pimage_handle->pwebp_handle);
    }

	if(pimage_handle->lvgl_image_buf)
	{
		if (pimage_handle->lvgl_image_buf->data)
			divoom_system_free((uint8 *)pimage_handle->lvgl_image_buf->data);

		divoom_system_free(pimage_handle->lvgl_image_buf);
	}
	if (pimage_handle->divoom_rgb_buf)
		divoom_system_free(pimage_handle->divoom_rgb_buf);

	divoom_system_free(pimage_handle);
	pimage_handle = NULL;
}


void divoom_lvgl_top_danmaku_menu_clear_all_in(void)
{
	int i, num;
	Pdivoom_lvgl_dial_menu_image_ctrl pimage_handle;
	
	num = divoom_queue_items(Pdivoom_lvgl_top_danmaku_menu_handle->disp_image_queue);
	
	for(i = 0; i < num; i++)
	{
		pimage_handle = divoom_queue_pop(Pdivoom_lvgl_top_danmaku_menu_handle->disp_image_queue);
		divoom_lvgl_top_danmaku_menu_close_img(pimage_handle);
	}
}

void divoom_lvgl_top_danmaku_menu_set_img_dsc_t(Pdivoom_lvgl_dial_menu_image_ctrl pimage_handle, int w, int h)
{
	pimage_handle->lvgl_image_buf = divoom_system_malloc(sizeof(lv_img_dsc_t));

	if (pimage_handle->image_type == DIVOOM_LVGL_DIAL_MENU_TYPE_DIVOOM_QUEUE || 
		pimage_handle->image_type == DIVOOM_LVGL_DIAL_MENU_TYPE_DIVOOM_EQ_DIAL|| pimage_handle->image_type == DIVOOM_LVGL_DIAL_MENU_TYPE_RGB_IMAGE)
		divoom_lv_img_dsc_init_fmt(pimage_handle->lvgl_image_buf, w, h,
			LV_COLOR_FORMAT_RGB565, NULL);
	else  
		divoom_lv_img_dsc_init_fmt(pimage_handle->lvgl_image_buf, w, h,
			LV_COLOR_FORMAT_RGB565A8, NULL);
	pimage_handle->lvgl_image_buf->data =
		divoom_system_malloc(pimage_handle->lvgl_image_buf->data_size);
}

void divoom_lvgl_top_danmaku_menu_read_image_buf(Pdivoom_lvgl_dial_menu_image_ctrl pimage_handle, struct _lv_timer_t *user_time_handle)
{
    uint8 *frame_buf;
    int frame_buf_is_lvfmt = 0;
	int end_flag = 0;

	if (pimage_handle->image_type == DIVOOM_LVGL_DIAL_MENU_TYPE_LOCAL_IMAGE)
	{
        if(pimage_handle->resource_type == DIVOOM_RESOURCE_TYPE_DIVOOM)
        {
            Pdivoom_http_download_image_file_info pimage_info = pimage_handle->pwebp_handle;

            pimage_handle->frame_time = pimage_info->speed;
            if(pimage_handle->cur_disp_id >= pimage_info->num)
            {
                pimage_handle->cur_disp_id = 0;
				if (pimage_handle->play_time > 0)
					pimage_handle->play_time--;
            }
            if(pimage_handle->lvgl_image_buf->data == NULL)
            {
                pimage_handle->lvgl_image_buf->data =
                    divoom_system_malloc(pimage_handle->lvgl_image_buf->data_size);
            }
            if(pimage_handle->divoom_rgb_buf == NULL)
            {
                pimage_handle->divoom_rgb_buf = divoom_system_malloc(256*256*2); //默认最大的图为256*256 RGB565
            }
            //read cache
            if(pimage_info->image_type == DIVOOM_IMAGE_BUF_TYPE_ZSTD_ENCODE)
            {
                divoom_zstd_manage_decode_rgb_buf(pimage_info->zstd_handle, pimage_handle->divoom_rgb_buf);
            }
            else if(pimage_info->image_type == DIVOOM_IMAGE_BUF_TYPE_ZSTD_WEBP_ENCODE)
            {
                divoom_webp_zstd_mix_decode_rgb_buf(pimage_info->zstd_handle, pimage_handle->divoom_rgb_buf);
            }
            else
            {
                divoom_manage_image_decode_rgb565_buf(
                    pimage_info->pimage_table[pimage_handle->cur_disp_id]->image_data,
                    pimage_info->pimage_table[pimage_handle->cur_disp_id]->image_size,
                    pimage_info->width,
                    pimage_info->pimage_table[pimage_handle->cur_disp_id]->image_type,
                    pimage_handle->divoom_rgb_buf);
            }
            divoom_rk_mpp_stretch_image_lvfmt(pimage_handle->divoom_rgb_buf,
                pimage_info->width, pimage_info->width, RK_FORMAT_RGB_565,
                (uint8 *)pimage_handle->lvgl_image_buf->data,
                pimage_handle->area_w, pimage_handle->area_h,
                pimage_handle->lvgl_image_buf->header.cf);
            if(pimage_handle->apha_flag)
            {
                //将黑色的转成全透明的
                uint8 *rgb565_buf = (uint8 *)pimage_handle->lvgl_image_buf->data;
                uint8 *alpha = rgb565_buf + (uint32)pimage_handle->area_w *
                    (uint32)pimage_handle->area_h * 2U;
                for(int i = 0;i < pimage_handle->area_w*pimage_handle->area_h; i++)
                {
                    if(pimage_handle->lvgl_image_buf->header.cf == LV_COLOR_FORMAT_RGB565A8 &&
                        rgb565_buf[0] == 0 && rgb565_buf[1] == 0)
                    {
                        alpha[i] = 0;
                    }
                    rgb565_buf += 2;
                }
            }
            lv_obj_invalidate(pimage_handle->lvgl_image_obj);
    		pimage_handle->cur_disp_id++;
			return ;
        }
        else if(pimage_handle->resource_type == DIVOOM_RESOURCE_TYPE_WEBP)
        {
            if(pimage_handle->lvgl_image_buf->data == NULL)
            {
                pimage_handle->lvgl_image_buf->data =
                    divoom_system_malloc(pimage_handle->lvgl_image_buf->data_size);
            }
            if(pimage_handle->lvgl_image_buf->header.cf == LV_COLOR_FORMAT_RGB565A8)
            {
                frame_buf = divoom_libwebp_decode_get_frame_rgb565a8_buffer(pimage_handle->pwebp_handle,
                    &pimage_handle->frame_time, &end_flag, (uint8 *)pimage_handle->lvgl_image_buf->data);
                frame_buf_is_lvfmt = 1;
            }
            else
            {
                frame_buf = divoom_libwebp_decode_get_frame_buffer(pimage_handle->pwebp_handle, &pimage_handle->frame_time,&end_flag, NULL);
            }
        }
        else if(pimage_handle->resource_type == DIVOOM_RESOURCE_TYPE_JPEG)
        {
            pimage_handle->frame_time = 1000;
            end_flag = 1;
            frame_buf = divoom_decode_jpg_bg_get_rgba_buf(pimage_handle->pwebp_handle);
        }
        else if(pimage_handle->resource_type == DIVOOM_RESOURCE_TYPE_JPEG_GIF)
        {            
            frame_buf = divoom_decode_jpeg_gif_bgra_get_bgra_data(pimage_handle->pwebp_handle, &pimage_handle->frame_time, &end_flag);
        }
        else if(pimage_handle->resource_type == DIVOOM_RESOURCE_TYPE_GIF)
        {            
            frame_buf = divoom_gif_lib_bgra_get_bgra_data(pimage_handle->pwebp_handle, &pimage_handle->frame_time, &end_flag);
        }
        else
        {
            return ;
        }
        if(end_flag)
        {
            if (pimage_handle->play_time > 0)
				pimage_handle->play_time--;
        }
        if(pimage_handle->lvgl_image_buf->data == NULL)
        {
            pimage_handle->lvgl_image_buf->data =
                divoom_system_malloc(pimage_handle->lvgl_image_buf->data_size);
        }
        if(frame_buf_is_lvfmt) {
        }
        else if(pimage_handle->resource_type == DIVOOM_RESOURCE_TYPE_GIF) {
            memcpy((uint8 *)pimage_handle->lvgl_image_buf->data, frame_buf,
                pimage_handle->lvgl_image_buf->data_size);
        }
        else if(pimage_handle->lvgl_image_buf->header.cf == LV_COLOR_FORMAT_RGB565A8) {
            if(pimage_handle->resource_type == DIVOOM_RESOURCE_TYPE_JPEG)
                divoom_rgba8888_to_rgb565a8_buf(frame_buf,
                    (uint8 *)pimage_handle->lvgl_image_buf->data,
                    (uint32)pimage_handle->area_w * (uint32)pimage_handle->area_h);
            else
                divoom_bgra8888_to_rgb565a8_buf(frame_buf,
                    (uint8 *)pimage_handle->lvgl_image_buf->data,
                    (uint32)pimage_handle->area_w * (uint32)pimage_handle->area_h);
        }
        else {
            divoom_rk_mpp_stretch_image_lvfmt(frame_buf, pimage_handle->area_w,
                pimage_handle->area_h,
                pimage_handle->resource_type == DIVOOM_RESOURCE_TYPE_JPEG ?
                    RK_FORMAT_RGBA_8888 : RK_FORMAT_BGRA_8888,
                (uint8 *)pimage_handle->lvgl_image_buf->data,
                pimage_handle->area_w, pimage_handle->area_h,
                pimage_handle->lvgl_image_buf->header.cf);
        }
        if(pimage_handle->resource_type == DIVOOM_RESOURCE_TYPE_GIF)
        {
            
        }
        if(pimage_handle->resource_type == DIVOOM_RESOURCE_TYPE_WEBP && !frame_buf_is_lvfmt)
        {
            divoom_system_free(frame_buf);
        }
        lv_obj_invalidate(pimage_handle->lvgl_image_obj);
    }
}


void divoom_lvgl_top_danmaku_menu_refresh_local_image_time(struct _lv_timer_t *user_time_handle)
{
	Pdivoom_lvgl_dial_menu_image_ctrl pimage_handle = (Pdivoom_lvgl_dial_menu_image_ctrl)lv_timer_get_user_data(user_time_handle);
    divoom_lvgl_top_danmaku_menu_read_image_buf(pimage_handle, user_time_handle);
    if(pimage_handle->frame_time > 0)
    {
        // 解码后按当前显示帧的持续时间安排下一次换帧。
        lv_timer_set_period(user_time_handle, pimage_handle->frame_time);
    }
}

void *divoom_lvgl_top_danmaku_menu_disp_img(int model_id, int image_id, void *parent_obj)
{
	char file_name[256];
	Pdivoom_lvgl_dial_menu_image_ctrl pimage_handle;

	if (Pdivoom_lvgl_top_danmaku_menu_handle == NULL)
		return NULL;

	//divoom_system_get_lock(phandle_ctrl->image_play_lock);
	if (model_id != 0 && image_id != 0)
	{
		divoom_manage_image_download_get_file_name(model_id, image_id, file_name);
	}
	else
	{
		//divoom_system_free_lock(phandle_ctrl->image_play_lock);
		return NULL;
	}
	//divoom_system_free_lock(phandle_ctrl->image_play_lock);
	
	pimage_handle = divoom_system_malloc(sizeof(divoom_lvgl_dial_menu_image_ctrl));
    if (pimage_handle == NULL)
        return NULL;            

    memset(pimage_handle, 0, sizeof(divoom_lvgl_dial_menu_image_ctrl));
	pimage_handle->pwebp_handle = divoom_lvgl_dial_menu_open_resource_handle(file_name, &pimage_handle->resource_type, &pimage_handle->area_w,
            &pimage_handle->area_h, &pimage_handle->frame_num);
	pimage_handle->area_h = 16;
	pimage_handle->area_w = 16;
    if(pimage_handle->pwebp_handle == NULL)
    {
        divoom_system_free(pimage_handle);
		pimage_handle = NULL;
        return NULL;
    }

    pimage_handle->image_type = DIVOOM_LVGL_DIAL_MENU_TYPE_LOCAL_IMAGE;
	pimage_handle->play_time = 5;
	pimage_handle->cur_disp_id = 0;
	//image_play_time = divoom_system_get_ticks() + 10;

	pimage_handle->obj = parent_obj;
	pimage_handle->lvgl_image_obj = lv_img_create(parent_obj);
	//lv_obj_align(pimage_handle->lvgl_image_obj, LV_ALIGN_CENTER, 0, 0);  // 居中对齐
	//lv_obj_set_pos(pimage_handle->lvgl_image_obj, , );
    lv_obj_set_size(pimage_handle->lvgl_image_obj, pimage_handle->area_w, pimage_handle->area_h);
	divoom_lvgl_top_danmaku_menu_set_img_dsc_t(pimage_handle, pimage_handle->area_w, pimage_handle->area_h);

	lv_img_set_src(pimage_handle->lvgl_image_obj, pimage_handle->lvgl_image_buf);
	divoom_lvgl_top_danmaku_menu_read_image_buf(pimage_handle, NULL);

	pimage_handle->disp_time_handle = lv_timer_create(divoom_lvgl_top_danmaku_menu_refresh_local_image_time, pimage_handle->frame_time, pimage_handle);

	//divoom_queue_push(Pdivoom_lvgl_top_danmaku_menu_handle->disp_image_queue, pimage_handle);

	return pimage_handle;
}
#endif

#if 0
int divoom_lvgl_top_danmaku_menu_get_line_min_x(int cur_line)
{
	int tmp_min_end_x, res = 0;

	if(cur_line < 0 ||  cur_line >= Pdivoom_lvgl_top_danmaku_menu_handle->line_num)
		return DIVOOM_SCREEN_WIDTH;

	if(Pdivoom_lvgl_top_danmaku_menu_handle->line_last_obj[cur_line] == NULL)
		return DIVOOM_SCREEN_WIDTH;

	tmp_min_end_x = lv_obj_get_x(Pdivoom_lvgl_top_danmaku_menu_handle->line_last_obj[cur_line]) + lv_obj_get_width(Pdivoom_lvgl_top_danmaku_menu_handle->line_last_obj[cur_line]);

	if(tmp_min_end_x < DIVOOM_SCREEN_WIDTH - DIVOOM_LVGL_TOP_DANMAKU_MENU_LABEL_SPACE)
	{
		res = DIVOOM_SCREEN_WIDTH;
	}
	else
	{
		res = tmp_min_end_x + DIVOOM_LVGL_TOP_DANMAKU_MENU_LABEL_SPACE;
	}

	return res;
}


int divoom_lvgl_top_danmaku_menu_select_line(int *start_x)
{
	int i, select_line, tmp_min_end_x, rand_line;
	uint32_t min_end_x;
	
	min_end_x = T_U32_MAX;
	rand_line = rand() % Pdivoom_lvgl_top_danmaku_menu_handle->line_num;
	
	/*for(i = 0; i < Pdivoom_lvgl_top_danmaku_menu_handle->line_num; i++)
	{
		if(Pdivoom_lvgl_top_danmaku_menu_handle->line_last_obj[i] == NULL)
		{
			*start_x = DIVOOM_SCREEN_WIDTH;
			return i;
		}
		tmp_min_end_x = lv_obj_get_x(Pdivoom_lvgl_top_danmaku_menu_handle->line_last_obj[i]) + lv_obj_get_width(Pdivoom_lvgl_top_danmaku_menu_handle->line_last_obj[i]);
			
		if(min_end_x > tmp_min_end_x)
		{
			//divoom_print("min_end_x = %d, tmp_min_end_x = %d\n", min_end_x, tmp_min_end_x);
			min_end_x = tmp_min_end_x;
			select_line = i;
		}
		divoom_print("min_end_x = %d, tmp_min_end_x = %d\n", min_end_x, tmp_min_end_x);
	}*/
	min_end_x =  divoom_lvgl_top_danmaku_menu_get_line_min_x(rand_line);

	if(min_end_x == T_U32_MAX)
	{
		select_line = 0;
		*start_x = DIVOOM_SCREEN_WIDTH;
	}
	else if(min_end_x <= DIVOOM_SCREEN_WIDTH)
	{
		*start_x = DIVOOM_SCREEN_WIDTH;
	}
	else
	{
		*start_x = min_end_x;
	}
	
	return rand_line;
}
#endif

#if 0
void divoom_lvgl_top_danmaku_menu_laebl_anim_cb(void *var, int32_t v)
{
	int i, x, width, cur_line;
	int speed_pix = 5;
	static int time = 0;
	int rand_num = rand() % 1000;
	char buffer[100] = {0};
	//lv_obj_t *label = (lv_obj_t *)var;
	uint32_t current_time;
	uint32_t current_tick;
	current_tick = lv_tick_get();

	if(Pdivoom_lvgl_top_danmaku_menu_handle == NULL)
		return;

	time++;
	if(time % 100 == 0)
	{
		sprintf(buffer, "出门记得带伞 %d", rand_num);
		divoom_lvgl_top_danmaku_menu_add_danmaku_label_item(buffer, -1, 0);

		//divoom_lvgl_top_danmaku_menu_add_danmaku_image_item(0);
		if((time+50) % 250 == 0)
		{
			if(Pdivoom_lvgl_top_danmaku_menu_handle->pdanmaku_config->disp_area_flag == 0)
				Pdivoom_lvgl_top_danmaku_menu_handle->pdanmaku_config->disp_area_flag = 1;
			else if(Pdivoom_lvgl_top_danmaku_menu_handle->pdanmaku_config->disp_area_flag == 1)
				Pdivoom_lvgl_top_danmaku_menu_handle->pdanmaku_config->disp_area_flag = 2;
			else
				Pdivoom_lvgl_top_danmaku_menu_handle->pdanmaku_config->disp_area_flag = 0;

			divoom_print("flag = %d\n", Pdivoom_lvgl_top_danmaku_menu_handle->pdanmaku_config->disp_area_flag);
			divoom_lvgl_top_danmaku_menu_update_danmaku_config(0);
		}
	}
	
	
	current_time = current_tick - Pdivoom_lvgl_top_danmaku_menu_handle->pre_time_tick;
	divoom_print("delta_time = %d\n", current_time);
	
	for(i = 0; i < DIVOOM_LVGL_TOP_DANMAKU_MENU_MAX_LABEL_NUM; i++)
	{
		if(Pdivoom_lvgl_top_danmaku_menu_handle->label_obj[i] == NULL)
			break;
	
		if(Pdivoom_lvgl_top_danmaku_menu_handle->label_obj[i] != NULL)
		{
			//divoom_print("i = %d\n", i);
			//x = lv_obj_get_x(Pdivoom_lvgl_top_danmaku_menu_handle->label_obj[i]);
			//width = lv_obj_get_width(Pdivoom_lvgl_top_danmaku_menu_handle->label_obj[i]);
			
			x = Pdivoom_lvgl_top_danmaku_menu_handle->label_x[i];
			width = Pdivoom_lvgl_top_danmaku_menu_handle->label_width[i];
			
			if(x + width <= 0)
			{
				cur_line = (lv_obj_get_y(Pdivoom_lvgl_top_danmaku_menu_handle->label_obj[i]) - Pdivoom_lvgl_top_danmaku_menu_handle->label_start_y) / 60;
				x = divoom_lvgl_top_danmaku_menu_get_line_min_x(cur_line);
				divoom_print("x = %d, cur_line = %d\n", x, cur_line);
				Pdivoom_lvgl_top_danmaku_menu_handle->line_last_obj[cur_line] = Pdivoom_lvgl_top_danmaku_menu_handle->label_obj[i];
				lv_obj_set_x(Pdivoom_lvgl_top_danmaku_menu_handle->label_obj[i], x);
				Pdivoom_lvgl_top_danmaku_menu_handle->label_x[i] = x; 
			}
			else
			{
				// 线性插值公式
				int speed = 100;  //速度    单位 : 像素/秒  
				uint32_t delta_time;
				int32_t diff_x  = speed_pix;
				delta_time = current_tick - Pdivoom_lvgl_top_danmaku_menu_handle->pre_time_tick; 
				
        		//diff_x = speed * delta_time/1000;

				#if 0
				int32_t diff_v;

				//divoom_print("v = %d, pre_v = %d\n", v, pre_v);
				if(v >= pre_v)
					diff_v = v - pre_v;
				else
					diff_v = v;
				

				
				diff_x = diff_v;
				#endif
				
				int set_x = x - diff_x;
				
				//divoom_print("delta_time = %d, diff_x = %d, x = %d\n", delta_time, diff_x, x);
				
        		lv_obj_set_x(Pdivoom_lvgl_top_danmaku_menu_handle->label_obj[i], set_x);
				Pdivoom_lvgl_top_danmaku_menu_handle->label_x[i] = set_x; 
			}
		}
	}

	Pdivoom_lvgl_top_danmaku_menu_handle->pre_time_tick = current_tick;
}
#endif


#if 0
int divoom_lvgl_top_danmaku_menu_add_line_label_obj(void *label_obj, int line)
{
	int i, j;

	//for(i = 0; i < DIVOOM_LVGL_TOP_DANMAKU_MENU_MAX_LINE_NUM; i++)
	//{
		for(j = 0; j < DIVOOM_LVGL_TOP_DANMAKU_MENU_MAX_LINE_LABEL_NUM; j++)
		{
			if(Pdivoom_lvgl_top_danmaku_menu_handle->line_label_obj[line][j] == NULL)
			{
				if(label_obj)
				{
					Pdivoom_lvgl_top_danmaku_menu_handle->line_label_obj[line][j] = label_obj;
				}
				return j;
			}
		}
	//}

	return -1;	
}

void divoom_lvgl_top_danmaku_menu_delete_line_all_labels(int line)
{
	for(int j = 0; j < DIVOOM_LVGL_TOP_DANMAKU_MENU_MAX_LINE_LABEL_NUM; j++)
	{
		if(Pdivoom_lvgl_top_danmaku_menu_handle->line_label_obj[line][j] != NULL)
		{
			lv_obj_del(Pdivoom_lvgl_top_danmaku_menu_handle->line_label_obj[line][j]);
			Pdivoom_lvgl_top_danmaku_menu_handle->line_label_obj[line][j] = NULL;
		}
	}
}

int divoom_lvgl_top_danmaku_menu_full_check_new_line_delete_labels(int line, int add_label_width)
{
	int i;

	for(i = 0; i < DIVOOM_LVGL_TOP_DANMAKU_MENU_MAX_LINE_LABEL_NUM; i++)
	{
		if(Pdivoom_lvgl_top_danmaku_menu_handle->line_label_obj[line][i] != NULL)	//该行有label，并且是第一个label
		{
			if(lv_obj_get_width(Pdivoom_lvgl_top_danmaku_menu_handle->line_label_obj[line][i]) < add_label_width)	//删除整行label
			{
				divoom_lvgl_top_danmaku_menu_delete_line_all_labels(line);
			}
			else	//删除单个label
			{
				//lv_obj_del(Pdivoom_lvgl_top_danmaku_menu_handle->line_label_obj[line][i]);
				//Pdivoom_lvgl_top_danmaku_menu_handle->line_label_obj[line][i] = NULL;

				divoom_lvgl_top_danmaku_menu_delete_line_all_labels(line);
			}
			break;
		}
	}

}

void *divoom_lvgl_top_danmaku_menu_get_line_last_obj(int line)
{
	int i;

	if(Pdivoom_lvgl_top_danmaku_menu_handle->line_label_obj[line][0] == NULL)
		return NULL;

	if(Pdivoom_lvgl_top_danmaku_menu_handle->line_label_obj[line][DIVOOM_LVGL_TOP_DANMAKU_MENU_MAX_LINE_LABEL_NUM - 1] != NULL)
	{
		Pdivoom_lvgl_top_danmaku_menu_handle->cur_line++;
		
		if(Pdivoom_lvgl_top_danmaku_menu_handle->cur_line >= Pdivoom_lvgl_top_danmaku_menu_handle->line_num)
		{
			Pdivoom_lvgl_top_danmaku_menu_handle->cur_line = 0;
		}

		divoom_lvgl_top_danmaku_menu_full_check_new_line_delete_labels(Pdivoom_lvgl_top_danmaku_menu_handle->cur_line, 0);
		
		return NULL;
	}

	for(i = DIVOOM_LVGL_TOP_DANMAKU_MENU_MAX_LINE_LABEL_NUM - 1; i >= 0; i--)
	{
		if(Pdivoom_lvgl_top_danmaku_menu_handle->line_label_obj[line][i] != NULL)
		{
			return Pdivoom_lvgl_top_danmaku_menu_handle->line_label_obj[line][i];
		}
	}

	return NULL;
}


int divoom_lvgl_top_danmaku_menu_get_line_min_x(int add_label_width)
{
	int tmp_min_end_x, cur_line, res = 0;
	void *line_last_obj;

	if(cur_line < 0 ||  cur_line >= Pdivoom_lvgl_top_danmaku_menu_handle->line_num)
	{
		return DIVOOM_LVGL_TOP_DANMAKU_MENU_FIRST_LABEL_SPACE;
	}
	
	cur_line = Pdivoom_lvgl_top_danmaku_menu_handle->cur_line;
	line_last_obj = divoom_lvgl_top_danmaku_menu_get_line_last_obj(cur_line);
	
	if(line_last_obj == NULL)
	{
		return DIVOOM_LVGL_TOP_DANMAKU_MENU_FIRST_LABEL_SPACE;
	}
	
	tmp_min_end_x = lv_obj_get_x(line_last_obj) + lv_obj_get_width(line_last_obj);

	if(tmp_min_end_x + DIVOOM_LVGL_TOP_DANMAKU_MENU_LABEL_SPACE + add_label_width < DIVOOM_SCREEN_WIDTH)
	{
		res = tmp_min_end_x + DIVOOM_LVGL_TOP_DANMAKU_MENU_LABEL_SPACE; //放在当前行
	}
	else
	{
		Pdivoom_lvgl_top_danmaku_menu_handle->cur_line++; //放不下,放在下一行
		
		if(Pdivoom_lvgl_top_danmaku_menu_handle->cur_line >= Pdivoom_lvgl_top_danmaku_menu_handle->line_num)
		{
			Pdivoom_lvgl_top_danmaku_menu_handle->cur_line = 0;
		}

		//下一行为空放在开头,不为空比较第一个label的宽度和新加的宽度，删掉整行或者替换这个弹幕,放在开头
		divoom_lvgl_top_danmaku_menu_full_check_new_line_delete_labels(Pdivoom_lvgl_top_danmaku_menu_handle->cur_line, add_label_width);
		
		res = DIVOOM_LVGL_TOP_DANMAKU_MENU_FIRST_LABEL_SPACE;
	}
	
	return res;
}

void divoom_lvgl_top_danmaku_menu_add_danmaku_label_item_test(char *disp_info, int text_color, int lock_flag)
{
	if(Pdivoom_lvgl_top_danmaku_menu_handle == NULL)
	{
		divoom_lvgl_top_danmaku_menu_init();
		
		if(Pdivoom_lvgl_top_danmaku_menu_handle == NULL)
			return;	
	}
	
	if(disp_info == NULL)
	{
		divoom_print("test\n");
		return;
	}
	

	int line, start_x, start_y, index;
	int width, height;

	if(lock_flag)
	{
		divoom_get_lvgl_lock();
	}
	
	//处理坐标和动画
	lv_obj_t *label = lv_label_create(Pdivoom_lvgl_top_danmaku_menu_handle->main_obj);
	lv_obj_set_style_text_font(label, divoom_menu_control_get_font_info(26, DIVOOM_MENU_CONTROL_ID_MAIN), 0);
	if(text_color == -1)
	{
		lv_obj_set_style_text_color(label, lv_color_hex(0x333333), 0);
	}
	else
	{
		lv_obj_set_style_text_color(label, lv_color_hex(text_color), 0);
	}
	lv_label_set_text(label, disp_info);
	
	//line = divoom_lvgl_top_danmaku_menu_select_line(&start_x);
	//lv_obj_set_pos(label, start_x, Pdivoom_lvgl_top_danmaku_menu_handle->label_start_y + line * 60);
	lv_obj_update_layout(label);  // 立即更新布局获取尺寸
	//index = divoom_lvgl_top_danmaku_menu_add_label_obj(label);
	////Pdivoom_lvgl_top_danmaku_menu_handle->label_x[index] = start_x;
	
	
	
	width = lv_obj_get_width(label);
	height = lv_obj_get_height(label);

	start_x = divoom_lvgl_top_danmaku_menu_get_line_min_x(width);
	start_y = Pdivoom_lvgl_top_danmaku_menu_handle->cur_line * 60;
	
	lv_obj_set_pos(label, start_x, Pdivoom_lvgl_top_danmaku_menu_handle->label_start_y + start_y);

	#if 0
	if(flag == 0)
	{
		lv_anim_t a;
	
		lv_anim_init(&a);
		//lv_anim_set_values(&a, start_x, -lv_obj_get_width(label));
		//lv_anim_set_values(&a, 0, 150);
		//lv_anim_set_values(&a, 0, 5);
		lv_anim_set_exec_cb(&a, divoom_lvgl_top_danmaku_menu_laebl_anim_cb);
		lv_anim_set_path_cb(&a, lv_anim_path_linear);
		//lv_anim_set_path_cb(&a, lv_anim_path_step);
		lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
		//lv_anim_set_var(&a, label);
		lv_anim_set_time(&a, 2000);
		//lv_anim_set_time(&a, 16);
		lv_anim_set_delay(&a, 0);
		//lv_anim_set_playback_time(&a, 16);
		//lv_anim_set_playback_delay(&a, 0);
		lv_anim_set_repeat_delay(&a, 0);
		//lv_anim_set_ready_cb(&a);

		flag = 1;
		Pdivoom_lvgl_top_danmaku_menu_handle->pre_time_tick = lv_tick_get();
		lv_anim_start(&a);
	}
	divoom_print("cur_line = %d, start_x = %d, index = %d\n", line, start_x, index);
	#endif

	line = Pdivoom_lvgl_top_danmaku_menu_handle->cur_line;
	index = divoom_lvgl_top_danmaku_menu_add_line_label_obj(label, line);

	//if(Pdivoom_lvgl_top_danmaku_menu_handle->label_refresh_timer == NULL)
	//{
	//	Pdivoom_lvgl_top_danmaku_menu_handle->label_refresh_timer = lv_timer_create(divoom_lvgl_top_danmaku_menu_time_handle, 100, NULL);
	//}
	
	if(lock_flag)
	{
		divoom_free_lvgl_lock();
	}
}
#endif

void divoom_lvgl_top_danmaku_menu_del_menu(void)
{
	if(Pdivoom_lvgl_top_danmaku_menu_handle != NULL)
	{
		if(Pdivoom_lvgl_top_danmaku_menu_handle->label_refresh_timer)
		{
			lv_timer_del(Pdivoom_lvgl_top_danmaku_menu_handle->label_refresh_timer);
		}
		if(Pdivoom_lvgl_top_danmaku_menu_handle->image_refresh_timer)
		{
			lv_timer_del(Pdivoom_lvgl_top_danmaku_menu_handle->image_refresh_timer);
		}
    	divoom_lvgl_top_control_remove_top(Pdivoom_lvgl_top_danmaku_menu_handle);
	
		//divoom_lvgl_top_danmaku_menu_clear_all_in();
		//divoom_queue_destroy(Pdivoom_lvgl_top_danmaku_menu_handle->disp_image_queue, NULL);

		divoom_queue_destroy(Pdivoom_lvgl_top_danmaku_menu_handle->label_user_queue, NULL);	
		divoom_queue_destroy(Pdivoom_lvgl_top_danmaku_menu_handle->image_queue, NULL);	

		lv_obj_del_async(Pdivoom_lvgl_top_danmaku_menu_handle->main_obj);
		divoom_system_free(Pdivoom_lvgl_top_danmaku_menu_handle);
    
		Pdivoom_lvgl_top_danmaku_menu_handle = NULL;
	}
}


int divoom_lvgl_top_danmaku_menu_add_label_obj(void *label_obj)
{
	int i;

	for(i = 0; i < DIVOOM_LVGL_TOP_DANMAKU_MENU_MAX_LABEL_NUM; i++)
	{
		if(Pdivoom_lvgl_top_danmaku_menu_handle->label_obj[i] == NULL)
		{
			if(label_obj)
			{
				Pdivoom_lvgl_top_danmaku_menu_handle->label_obj[i] = label_obj;
			}
			return i;
		}
	}

	return -1;	
}


void divoom_lvgl_top_danmaku_menu_time_handle(lv_timer_t *refresh_timer)
{
	if(Pdivoom_lvgl_top_danmaku_menu_handle == NULL)
		return;

	uint32_t current_tick = lv_tick_get();
	uint32_t delta_time;
	int time, flag;
	int opa, zoom, i, font_size; 
	lv_font_t *font_obj;
	
	lv_obj_invalidate(Pdivoom_lvgl_top_danmaku_menu_handle->main_obj);
	//0~0.5s 改变透明度和放大大小，0.5~1.5s 不改变             	1.5s到2.0s缩小	
	/*for(i = 0; i < DIVOOM_LVGL_TOP_DANMAKU_MENU_MAX_LABEL_NUM; i++)
	{
		if(Pdivoom_lvgl_top_danmaku_menu_handle->label_obj[i] == NULL)
			continue;
	
		if(Pdivoom_lvgl_top_danmaku_menu_handle->label_obj[i] != NULL)
		{
			delta_time = current_tick - Pdivoom_lvgl_top_danmaku_menu_handle->label_time_tick[i]; 
			time = delta_time % 4000;		

			if(time < 1000)
			{
				opa = LV_OPA_100 * time / 1000;
				font_size = 26 + 18 * time / 1000;

				font_obj = divoom_menu_control_get_font_info(font_size, DIVOOM_MENU_CONTROL_ID_TOP_DANMAKU_MENU);
				font_obj->fallback = divoom_lvgl_font_manage_load_font(182, font_size, NULL, 0, 10, 64, NULL, DIVOOM_MENU_CONTROL_ID_TOP_DANMAKU_MENU, NULL);//表情备用字体
					
				lv_obj_set_style_text_font(Pdivoom_lvgl_top_danmaku_menu_handle->label_obj[i], font_obj, 0);
				lv_obj_set_style_text_opa(Pdivoom_lvgl_top_danmaku_menu_handle->label_obj[i], opa, 0);
			}
			else if(time >= 1000 && time <= 3000)
			{
				if(lv_obj_get_style_text_opa(Pdivoom_lvgl_top_danmaku_menu_handle->label_obj[i], 0) != LV_OPA_100)
				{
					font_obj = divoom_menu_control_get_font_info(44, DIVOOM_MENU_CONTROL_ID_TOP_DANMAKU_MENU);
					font_obj->fallback = divoom_lvgl_font_manage_load_font(182, 44, NULL, 0, 10, 64, NULL, DIVOOM_MENU_CONTROL_ID_TOP_DANMAKU_MENU, NULL);//表情备用字体
					
					lv_obj_set_style_text_font(Pdivoom_lvgl_top_danmaku_menu_handle->label_obj[i], font_obj, 0);
					lv_obj_set_style_text_opa(Pdivoom_lvgl_top_danmaku_menu_handle->label_obj[i], LV_OPA_100, 0);
				}
			}
			else if(time > 3000 && time <= 4000)
			{
				opa = LV_OPA_100 - LV_OPA_100 * (time - 3000) / 1000;
				font_size = 44 - 18 * (time - 3000) / 1000;

				font_obj = divoom_menu_control_get_font_info(font_size, DIVOOM_MENU_CONTROL_ID_TOP_DANMAKU_MENU);
				font_obj->fallback = divoom_lvgl_font_manage_load_font(182, font_size, NULL, 0, 10, 64, NULL, DIVOOM_MENU_CONTROL_ID_TOP_DANMAKU_MENU, NULL);//表情备用字体					

				lv_obj_set_style_text_font(Pdivoom_lvgl_top_danmaku_menu_handle->label_obj[i], font_obj, 0);
				lv_obj_set_style_text_opa(Pdivoom_lvgl_top_danmaku_menu_handle->label_obj[i], opa, 0);
			}	
		}
	}*/
}


// 检查两个边界框是否重叠
int divoom_lvgl_top_danmaku_menu_check_overlap(int new_x1, int new_x2, int new_y1, int new_y2,
                   int new_x3, int new_x4, int new_y3, int new_y4) {
    if(new_x1 > new_x4 || new_x2 < new_x3) return 0; // 水平方向不重叠
    if(new_y1 > new_y4 || new_y2 < new_y3) return 0; // 垂直方向不重叠

	return 1; // 重叠
}

// 获取旋转后的边界框
void divoom_lvgl_top_danmaku_menu_get_rotated_box(int x1, int y1, int width, int height, int angle,
                     int * new_x1, int * new_x2, int * new_y1, int * new_y2) {
    // 假设旋转中心是对象的中心
    int center_x = x1 + width / 2;
    int center_y = y1 + height / 2;

    // 旋转后的四个顶点
    int points[4][2] = {
        {x1, y1},
        {x1 + width, y1},
        {x1 + width, y1 + height},
        {x1, y1 + height}
    };

    // 旋转每个顶点
    for(int i = 0; i < 4; i++) {
        int p_x = points[i][0];
        int p_y = points[i][1];
        int rotated_x = (p_x - center_x) * cos(angle * 3.14 / 180) - (p_y - center_y) * sin(angle * 3.14 / 180) + center_x;
        int rotated_y = (p_x - center_x) * sin(angle * 3.14 / 180) + (p_y - center_y) * cos(angle * 3.14 / 180) + center_y;
        points[i][0] = rotated_x;
        points[i][1] = rotated_y;
    }

    // 计算新的边界框
    *new_x1 = *new_y1 = LV_COORD_MAX;
    *new_x2 = *new_y2 = LV_COORD_MIN;
    for(int i = 0; i < 4; i++) {
        if(points[i][0] < *new_x1) *new_x1 = points[i][0];
        if(points[i][1] < *new_y1) *new_y1 = points[i][1];
        if(points[i][0] > *new_x2) *new_x2 = points[i][0];
        if(points[i][1] > *new_y2) *new_y2 = points[i][1];
    }
}

// 检查两个旋转后的对象是否重叠
int divoom_lvgl_top_danmaku_menu_check_objects_overlap(int x1, int y1, int width1, int height1, int angle1,
                           int x2, int y2, int width2, int height2, int angle2) {
    int new_x1, new_x2, new_y1, new_y2;
    int new_x3, new_x4, new_y3, new_y4;

    divoom_lvgl_top_danmaku_menu_get_rotated_box(x1, y1, width1, height1, angle1, &new_x1, &new_x2, &new_y1, &new_y2);
    divoom_lvgl_top_danmaku_menu_get_rotated_box(x2, y2, width2, height2, angle2, &new_x3, &new_x4, &new_y3, &new_y4);

    return divoom_lvgl_top_danmaku_menu_check_overlap(new_x1, new_x2, new_y1, new_y2, new_x3, new_x4, new_y3, new_y4);
}


int divoom_lvgl_top_danmaku_menu_check_danmaku_overlap(int rand_x, int rand_y, int width, int height, int angle)
{
	int x1, w1, y1, h1, angle1;

	for(int i = 0; i < DIVOOM_LVGL_TOP_DANMAKU_MENU_MAX_LABEL_NUM; i++)
	{
		if(Pdivoom_lvgl_top_danmaku_menu_handle->label_obj[i] == NULL)
			continue;

		x1 = lv_obj_get_x(Pdivoom_lvgl_top_danmaku_menu_handle->label_obj[i]);
		w1 = lv_obj_get_width(Pdivoom_lvgl_top_danmaku_menu_handle->label_obj[i]);

		y1 = lv_obj_get_y(Pdivoom_lvgl_top_danmaku_menu_handle->label_obj[i]);
		h1 = lv_obj_get_height(Pdivoom_lvgl_top_danmaku_menu_handle->label_obj[i]);
		angle1 = Pdivoom_lvgl_top_danmaku_menu_handle->label_angle[i];

		#if 1
		if(divoom_lvgl_top_danmaku_menu_check_objects_overlap(rand_x, rand_y, width, height, angle, x1, y1, w1, h1, angle1) == 0)
		{
			//不重叠
			continue;
		}
		else
		{
			//重叠
			return 1;
		}
		#else
		if(x1 + w1 <= rand_x || rand_x + width <= x1 || y1 + h1 <= rand_y || rand_y + height <= y1)
		{
			//不重叠
			continue;
		}
		else
		{
			//重叠
			return 1;
		}
		#endif
	}

	return 0;
}




int divoom_lvgl_top_danmaku_menu_user_id_compare(void *item1, void *item2)
{
	int user_id = *(int*)item1;
	Pdivoom_lvgl_top_danmaku_menu_label_user_item puser_info = (Pdivoom_lvgl_top_danmaku_menu_label_user_item)item2;

	if(user_id == puser_info->user_id)	
		return 0;
	else
		return 1;
}

int divoom_lvgl_top_danmaku_menu_user_bg_color_compare(void *item1, void *item2)
{
	int bg_color_index = *(int*)item1;
	Pdivoom_lvgl_top_danmaku_menu_label_user_item puser_info = (Pdivoom_lvgl_top_danmaku_menu_label_user_item)item2;

	if(bg_color_index == puser_info->bg_color_index)	
		return 0;
	else
		return 1;
}

void divoom_lvgl_top_danmaku_menu_del_oldest_danmaku(void)
{
	int min_tick, min_index, i;
		
	min_tick = 0;
	min_index = 0;

	for(i = 0; i < DIVOOM_LVGL_TOP_DANMAKU_MENU_MAX_LABEL_NUM; i++)
	{
		if(Pdivoom_lvgl_top_danmaku_menu_handle->label_obj[i] != NULL)
		{
			if(min_tick == 0 && min_index == 0)	
			{
				min_tick = Pdivoom_lvgl_top_danmaku_menu_handle->label_time_tick[i];
				min_index = i;
			}

			if(min_tick > Pdivoom_lvgl_top_danmaku_menu_handle->label_time_tick[i])
			{
				min_tick = Pdivoom_lvgl_top_danmaku_menu_handle->label_time_tick[i];
				min_index = i;
			}
		}
	}

	if(Pdivoom_lvgl_top_danmaku_menu_handle->label_obj[min_index] != NULL)
	{
		lv_obj_del(Pdivoom_lvgl_top_danmaku_menu_handle->label_obj[min_index]);
		Pdivoom_lvgl_top_danmaku_menu_handle->label_time_tick[min_index] = 0;
		Pdivoom_lvgl_top_danmaku_menu_handle->label_angle[min_index] = 0;
		Pdivoom_lvgl_top_danmaku_menu_handle->label_obj[min_index] = NULL;
	}
}

void divoom_lvgl_top_danmaku_menu_add_danmaku_label_item(char *disp_info, int text_color, int lock_flag, int user_id)
{
	if(Pdivoom_lvgl_top_danmaku_menu_handle == NULL)
	{
		divoom_lvgl_top_danmaku_menu_init();
		
		if(Pdivoom_lvgl_top_danmaku_menu_handle == NULL)
			return;	
	}
	
	if(disp_info == NULL)
	{
		divoom_print("test\n");
		return;
	}

	int index, i;
	int width, height, rand_x, rand_y, rand_angle, left_right_flag, bg_color_index, available_count;
	int available_color[DIVOOM_LVGL_TOP_DANMAKU_MENU_MAX_LABEL_COLOR_NUM] = {0};
	lv_font_t *font_obj; 
	Pdivoom_lvgl_top_danmaku_menu_label_user_item puser_info, pnew_info;
	
	if(lock_flag)
	{
		divoom_get_lvgl_lock();
	}
	
	if(divoom_lvgl_top_danmaku_menu_add_label_obj(NULL) == -1)
	{
		divoom_lvgl_top_danmaku_menu_del_oldest_danmaku();
	}
	
	if(user_id <= 0)
	{
		bg_color_index = rand() % DIVOOM_LVGL_TOP_DANMAKU_MENU_MAX_LABEL_COLOR_NUM;
	}
	else
	{
		puser_info = divoom_queue_get_cmp(Pdivoom_lvgl_top_danmaku_menu_handle->label_user_queue, (void*)&user_id, divoom_lvgl_top_danmaku_menu_user_id_compare);
		
		if(puser_info) //使用固定的颜色
		{
			bg_color_index = puser_info->bg_color_index;
		}
		else
		{
			pnew_info = divoom_system_malloc(sizeof(divoom_lvgl_top_danmaku_menu_label_user_item));

			pnew_info->user_id = user_id;
			
			if(divoom_queue_is_full(Pdivoom_lvgl_top_danmaku_menu_handle->label_user_queue))
			{
				puser_info = divoom_queue_pop(Pdivoom_lvgl_top_danmaku_menu_handle->label_user_queue);	
			
				bg_color_index = puser_info->bg_color_index;
				divoom_system_free(puser_info);
			}
			else //随机一个颜色
			{
				available_count = 0;

				for (i = 0; i < DIVOOM_LVGL_TOP_DANMAKU_MENU_MAX_LABEL_COLOR_NUM; i++) 
				{
        			if(Pdivoom_lvgl_top_danmaku_menu_handle->used_color_index[i] == 0) 
					{
            			available_color[available_count++] = i;	//没有使用的颜色
        			}
   				}
	
				if (available_count == 0) 
				{
       	 			puser_info = divoom_queue_pop(Pdivoom_lvgl_top_danmaku_menu_handle->label_user_queue);	
					
					if(puser_info)
					{
						bg_color_index = puser_info->bg_color_index;
						divoom_system_free(puser_info);
					}
					else
					{
						bg_color_index = rand() % DIVOOM_LVGL_TOP_DANMAKU_MENU_MAX_LABEL_COLOR_NUM;
					}
    			}
				else
				{
					bg_color_index= available_color[rand() % available_count];
				}
				Pdivoom_lvgl_top_danmaku_menu_handle->used_color_index[bg_color_index] = 1;

				#if 0
				while (1)
				{
					color_index = rand() % DIVOOM_LVGL_TOP_DANMAKU_MENU_MAX_LABEL_COLOR_NUM;
					bg_color = divoom_top_danmaku_label_bg_color_list[color_index];
			
					puser_info = divoom_queue_get_cmp(Pdivoom_lvgl_top_danmaku_menu_handle->label_user_queue, (void*)&bg_color, divoom_lvgl_top_danmaku_menu_user_bg_color_compare);	
			
					if(puser_info == NULL)
					{
						break;
					}
				}
				#endif
			}
			
			pnew_info->bg_color_index = bg_color_index;
			divoom_queue_push(Pdivoom_lvgl_top_danmaku_menu_handle->label_user_queue, pnew_info);
		}
	}
	lv_obj_t *disp_cont = lv_obj_create(Pdivoom_lvgl_top_danmaku_menu_handle->main_obj);
	lv_obj_set_style_bg_opa(disp_cont, 0, 0);	
	lv_obj_set_style_border_width(disp_cont, 0, 0);
	lv_obj_set_style_pad_all(disp_cont, 0, 0);

	left_right_flag = rand() % 2;

	lv_obj_t *tail_img = lv_img_create(disp_cont);

	lv_obj_t *label_cont = lv_obj_create(disp_cont);
	lv_obj_set_style_radius(label_cont, 28, 0);
	lv_obj_set_style_bg_opa(label_cont, LV_OPA_100, 0);
	lv_obj_set_style_bg_color(label_cont, lv_color_hex(divoom_top_danmaku_label_bg_color_list[bg_color_index]), 0);
	lv_obj_set_style_pad_all(label_cont, 0, 0);
	lv_obj_set_style_border_width(label_cont, 0, 0);

	//处理坐标和动画
	lv_obj_t *label = lv_label_create(label_cont);
	font_obj = divoom_lvgl_font_manage_load_font(DIVOOM_DIAL_FONT_DEFAULT_ID, 32, NULL, 0, 10, 64, NULL, DIVOOM_MENU_CONTROL_ID_TOP_DANMAKU_MENU, NULL);//文字统一使用系统默认字体
	if(font_obj != NULL)
	{
		font_obj->fallback = divoom_lvgl_font_manage_load_font(182, 32, NULL, 0, 10, 64, NULL, DIVOOM_MENU_CONTROL_ID_TOP_DANMAKU_MENU, NULL);//表情备用字体
	}
	else
	{
		font_obj = (lv_font_t *)LV_FONT_DEFAULT;
	}
	lv_obj_set_style_text_font(label, font_obj, 0);
	//lv_obj_set_style_text_opa(label, LV_OPA_TRANSP, 0);
	//lv_obj_set_style_text_color(label, lv_color_hex(0x0), 0);

	if(text_color == -1)
	{
		lv_obj_set_style_text_color(label, lv_color_hex(0x0), 0);
	}
	else
	{
		lv_obj_set_style_text_color(label, lv_color_hex(text_color), 0);
	}

	lv_label_set_text(label, disp_info);
	lv_obj_update_layout(label);  // 立即更新布局获取尺寸
	
	width = lv_obj_get_width(label);
	height = lv_obj_get_height(label);

	if(width >= DIVOOM_SCREEN_WIDTH - 2*DIVOOM_DANMAKU_DISP_LABEL_AREA - DIVOOM_DANMAKU_LABEL_CONT_PAD_ALL)
	{	
		width = DIVOOM_SCREEN_WIDTH - 2*DIVOOM_DANMAKU_DISP_LABEL_AREA - DIVOOM_DANMAKU_LABEL_CONT_PAD_ALL;
		lv_obj_set_width(label, width);
		lv_obj_update_layout(label);  // 立即更新布局获取尺寸
		height = lv_obj_get_height(label);
	}

	divoom_lvgl_top_danmaku_menu_retry_add_label:
	for(i = 0; i < 10; i++)	//检查重叠
	{
		if(DIVOOM_SCREEN_WIDTH - 2*DIVOOM_DANMAKU_DISP_LABEL_AREA - DIVOOM_DANMAKU_LABEL_CONT_PAD_ALL > width)
		{
			rand_x = DIVOOM_DANMAKU_DISP_LABEL_AREA + rand() % (DIVOOM_SCREEN_WIDTH - DIVOOM_DANMAKU_DISP_LABEL_AREA*2 - DIVOOM_DANMAKU_LABEL_CONT_PAD_ALL - width);
		}
		else
		{
			rand_x = 80;
		}
	
		if(DIVOOM_SCREEN_HIGH - 2*DIVOOM_DANMAKU_DISP_LABEL_AREA - DIVOOM_DANMAKU_LABEL_PAD_ALL * 2> height)
		{
			rand_y = DIVOOM_DANMAKU_DISP_LABEL_AREA + rand() % (DIVOOM_SCREEN_HIGH - DIVOOM_DANMAKU_DISP_LABEL_AREA*2 - DIVOOM_DANMAKU_LABEL_PAD_ALL * 2 - height);
		}
		else
		{
			rand_y = 80;
		}

		rand_angle = -10 + rand() % 21;

		if(divoom_lvgl_top_danmaku_menu_check_danmaku_overlap(rand_x, rand_y, width + DIVOOM_DANMAKU_LABEL_CONT_PAD_ALL, height + DIVOOM_DANMAKU_LABEL_PAD_ALL, rand_angle)== 0)
		{
			break;
		}
	}
	
	if(i >= 10)
	{
		divoom_lvgl_top_danmaku_menu_del_oldest_danmaku();
		goto divoom_lvgl_top_danmaku_menu_retry_add_label;
	}

	lv_obj_set_size(label_cont, width + DIVOOM_DANMAKU_LABEL_PAD_ALL * 2, height + DIVOOM_DANMAKU_LABEL_PAD_ALL * 2);
	lv_obj_set_size(disp_cont, width + DIVOOM_DANMAKU_LABEL_PAD_ALL * 2 + DIVOOM_DANMAKU_LABEL_TAIL_LENGTH, height + DIVOOM_DANMAKU_LABEL_PAD_ALL * 2);
	lv_obj_align_to(label, label_cont, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_img_recolor(tail_img, lv_color_hex(divoom_top_danmaku_label_bg_color_list[bg_color_index]), 0);
	lv_obj_set_style_img_recolor_opa(tail_img, LV_OPA_100, 0);
	lv_obj_set_style_img_opa(tail_img, LV_OPA_100, 0);
	
	if(left_right_flag)
	{
		divoom_menu_control_lv_img_set_src(tail_img, "83/83_right.png");
		lv_obj_set_pos(label_cont, 0, 0);
		lv_obj_align_to(tail_img, label_cont, LV_ALIGN_OUT_BOTTOM_RIGHT, 8, -36);
	}
	else
	{
		divoom_menu_control_lv_img_set_src(tail_img, "83/83_left.png");
		lv_obj_set_pos(label_cont, 8, 0);
		lv_obj_align_to(tail_img, label_cont, LV_ALIGN_OUT_BOTTOM_LEFT, -8, -36);
	}
	
	lv_obj_set_style_transform_angle(disp_cont, rand_angle * 10, 0);
	lv_obj_set_pos(disp_cont, rand_x, rand_y);

	index = divoom_lvgl_top_danmaku_menu_add_label_obj(disp_cont);

	if(index == -1)
	{
		lv_obj_del(disp_cont);
		
		if(lock_flag)
		{
			divoom_free_lvgl_lock();
		}
		return;
	}

	Pdivoom_lvgl_top_danmaku_menu_handle->label_time_tick[index] = lv_tick_get();
	Pdivoom_lvgl_top_danmaku_menu_handle->label_angle[index] = rand_angle;

	if(Pdivoom_lvgl_top_danmaku_menu_handle->label_refresh_timer == NULL)
	{
		Pdivoom_lvgl_top_danmaku_menu_handle->label_refresh_timer = lv_timer_create(divoom_lvgl_top_danmaku_menu_time_handle, 100, NULL);
	}
	
	lv_obj_invalidate(Pdivoom_lvgl_top_danmaku_menu_handle->main_obj);
	
	if(lock_flag)
	{
		divoom_free_lvgl_lock();
	}
}


void divoom_lvgl_top_danmaku_menu_image_refresh_process_single(void)
{
	if(Pdivoom_lvgl_top_danmaku_menu_handle == NULL)
		return;

	int i;
	int *rand_face;
	int num = divoom_queue_items(Pdivoom_lvgl_top_danmaku_menu_handle->image_queue);

	for(i = 0; i < num; i++)
	{
		rand_face = divoom_queue_pop(Pdivoom_lvgl_top_danmaku_menu_handle->image_queue);		
		
		if(rand_face == NULL)
		{
			continue;
		}

		if(i == (num - 1))
		{
			int rand_x = rand() % (DIVOOM_SCREEN_WIDTH - DIVOOM_LVGL_TOP_DANMAKU_MENU_FACE_SIZE);
			int rand_y = rand() % (DIVOOM_SCREEN_HIGH - DIVOOM_LVGL_TOP_DANMAKU_MENU_FACE_SIZE);
			int rand_angle = rand() % 91 - 45;

			if(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj[0] == NULL)
			{
				Pdivoom_lvgl_top_danmaku_menu_handle->image_obj[0] = lv_img_create(Pdivoom_lvgl_top_danmaku_menu_handle->main_obj);
				divoom_menu_control_lv_img_set_src(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj[0], divoom_top_danmaku_img_list[*rand_face]);
			}
			else
			{
				divoom_menu_control_lv_img_set_src(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj[0], divoom_top_danmaku_img_list[*rand_face]);
			}
	
			lv_obj_set_style_img_opa(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj[0], LV_OPA_0, 0);
			lv_img_set_zoom(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj[0], 256 * 40/100);
			lv_img_set_angle(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj[0], rand_angle*10);
			lv_obj_set_pos(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj[0], rand_x, rand_y);
			lv_obj_set_size(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj[0], DIVOOM_LVGL_TOP_DANMAKU_MENU_FACE_SIZE, DIVOOM_LVGL_TOP_DANMAKU_MENU_FACE_SIZE);

			Pdivoom_lvgl_top_danmaku_menu_handle->image_time_tick[0] = lv_tick_get();
		}

		divoom_system_free(rand_face);
	}
}

void divoom_lvgl_top_danmaku_menu_image_single_refresh_time_handle(lv_timer_t *refresh_timer)
{
	if(Pdivoom_lvgl_top_danmaku_menu_handle == NULL)
		return;

	uint32_t current_tick, delta_time; 
	int time, flag;
	int opa, zoom; 

	current_tick = lv_tick_get();
	delta_time = current_tick - Pdivoom_lvgl_top_danmaku_menu_handle->image_time_tick[0]; 
	time = delta_time % 2000;
	flag = time / 500;

	//0~0.5s 改变透明度和放大大小，0.5~1.5s 不改变             	1.5s到2.0s缩小
	if(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj[0] != NULL)
	{
		if(flag == 0)
		{
			opa = LV_OPA_100 * time / 500;
			zoom = 256 * (1 - 40/100) * time / 500;

			lv_img_set_zoom(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj[0], zoom);
			lv_obj_set_style_img_opa(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj[0], opa, 0);
		}
		else if(flag > 0 && flag < 3)
		{
			lv_img_set_zoom(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj[0], 256);
			lv_obj_set_style_img_opa(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj[0], LV_OPA_100, 0);
		}
		else if(flag == 3)
		{
			opa = LV_OPA_100 - LV_OPA_100 * (time - 1500) / 500;
			zoom = 256 * 40/100 + 256 * 60 / 100 * (time - 1500) / 500;

			lv_img_set_zoom(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj[0], zoom);
			lv_obj_set_style_img_opa(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj[0], opa, 0);
		}
		lv_obj_move_foreground(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj[0]);
	}

	if(Pdivoom_lvgl_top_danmaku_menu_handle->image_time_tick[0] == 0)
		divoom_lvgl_top_danmaku_menu_image_refresh_process_single();
	else if(time > 650 && time < 2000)
		divoom_lvgl_top_danmaku_menu_image_refresh_process_single();
}

#if 0
void divoom_lvgl_top_danmaku_menu_add_danmaku_image_item(int lock_flag)
{
	if(Pdivoom_lvgl_top_danmaku_menu_handle == NULL)
	{
		divoom_lvgl_top_danmaku_menu_init();
		
		if(Pdivoom_lvgl_top_danmaku_menu_handle == NULL)
			return;
	}
	int rand_face = rand() % DIVOOM_LVGL_TOP_DANMAKU_MENU_MAX_FACE_NUM;
	//int rand_x = rand() % (DIVOOM_SCREEN_WIDTH - DIVOOM_LVGL_TOP_DANMAKU_MENU_FACE_SIZE);
	//int rand_y = rand() % (DIVOOM_SCREEN_HIGH - DIVOOM_LVGL_TOP_DANMAKU_MENU_FACE_SIZE);
	//int rand_angle = rand() % 91 - 45;
	int *image_id;

	while(Pdivoom_lvgl_top_danmaku_menu_handle->pre_face_id == rand_face)
	{
		rand_face = rand() % DIVOOM_LVGL_TOP_DANMAKU_MENU_MAX_FACE_NUM;
	}
	Pdivoom_lvgl_top_danmaku_menu_handle->pre_face_id = rand_face;
	
	if(lock_flag)
	{
		divoom_get_lvgl_lock();
	}

	#if 0
	if(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj == NULL)
	{
		Pdivoom_lvgl_top_danmaku_menu_handle->image_obj = lv_img_create(Pdivoom_lvgl_top_danmaku_menu_handle->main_obj);
		divoom_menu_control_lv_img_set_src(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj, divoom_top_danmaku_img_list[rand_face]);
		
	}
	else
	{
		divoom_menu_control_lv_img_set_src(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj, divoom_top_danmaku_img_list[rand_face]);
	}
	
	lv_obj_set_style_img_opa(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj, LV_OPA_0, 0);
	lv_img_set_zoom(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj, 256 * 40/100);
	lv_img_set_angle(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj, rand_angle*10);
	lv_obj_set_pos(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj, rand_x, rand_y);
	lv_obj_set_size(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj, DIVOOM_LVGL_TOP_DANMAKU_MENU_FACE_SIZE, DIVOOM_LVGL_TOP_DANMAKU_MENU_FACE_SIZE);

	Pdivoom_lvgl_top_danmaku_menu_handle->image_time_tick = lv_tick_get();
	#endif

	image_id = (int *)divoom_system_malloc(sizeof(int));
	*image_id = rand_face;

	divoom_queue_push(Pdivoom_lvgl_top_danmaku_menu_handle->image_queue, image_id);

	if(Pdivoom_lvgl_top_danmaku_menu_handle->image_refresh_timer == NULL)
	{
		Pdivoom_lvgl_top_danmaku_menu_handle->image_refresh_timer = lv_timer_create(divoom_lvgl_top_danmaku_menu_image_refresh_time_handle, 100, NULL);
	}

	if(lock_flag)
	{
		divoom_free_lvgl_lock();
	}
}
#endif

int divoom_lvgl_top_danmaku_menu_check_face_overlap(int x1, int y1, int x2, int y2)
{
	if(x1 + DIVOOM_LVGL_TOP_DANMAKU_MENU_FACE_SIZE <= x2 || x2 + DIVOOM_LVGL_TOP_DANMAKU_MENU_FACE_SIZE <= x1 ||
		y1 + DIVOOM_LVGL_TOP_DANMAKU_MENU_FACE_SIZE <= y2 || y2 + DIVOOM_LVGL_TOP_DANMAKU_MENU_FACE_SIZE <= y1)
	{
		//不重叠
		return 0;
	}
	
	return 1;
}


void divoom_lvgl_top_danmaku_menu_image_refresh_process_double(void)
{
	if(Pdivoom_lvgl_top_danmaku_menu_handle == NULL)
		return;

	int i, j;
	int *rand_face;
	int rand_x[2], rand_y[2], rand_angle;
	int num = divoom_queue_items(Pdivoom_lvgl_top_danmaku_menu_handle->image_queue);

	for(i = 0; i < num; i++)
	{
		rand_face = divoom_queue_pop(Pdivoom_lvgl_top_danmaku_menu_handle->image_queue);		
		
		if(rand_face == NULL)
		{
			continue;
		}

		if(i == (num - 1))
		{
			for(j = 0;j < 2; j++)
			{
				rand_angle = rand() % 91 - 45;
				
				rand_x[j] = rand() % (DIVOOM_SCREEN_WIDTH - DIVOOM_LVGL_TOP_DANMAKU_MENU_FACE_SIZE);
				rand_y[j] = rand() % (DIVOOM_SCREEN_HIGH - DIVOOM_LVGL_TOP_DANMAKU_MENU_FACE_SIZE);

				if(j == 1)
				{
					while(divoom_lvgl_top_danmaku_menu_check_face_overlap(rand_x[0], rand_y[0], rand_x[1], rand_y[1]))
					{
						rand_x[1] = rand() % (DIVOOM_SCREEN_WIDTH - DIVOOM_LVGL_TOP_DANMAKU_MENU_FACE_SIZE);
						rand_y[1] = rand() % (DIVOOM_SCREEN_HIGH - DIVOOM_LVGL_TOP_DANMAKU_MENU_FACE_SIZE);
					}
				}
				
				if(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj[j] == NULL)
				{
					Pdivoom_lvgl_top_danmaku_menu_handle->image_obj[j] = lv_img_create(Pdivoom_lvgl_top_danmaku_menu_handle->main_obj);
					divoom_menu_control_lv_img_set_src(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj[j], divoom_top_danmaku_img_list[rand_face[j]]);
				}
				else
				{
					divoom_menu_control_lv_img_set_src(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj[j], divoom_top_danmaku_img_list[rand_face[j]]);
				}
	
				lv_obj_set_style_img_opa(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj[j], LV_OPA_0, 0);
				lv_img_set_zoom(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj[j], 256 * 40/100);
				lv_img_set_angle(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj[j], rand_angle*10);
				lv_obj_set_pos(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj[j], rand_x[j], rand_y[j]);
				lv_obj_set_size(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj[j], DIVOOM_LVGL_TOP_DANMAKU_MENU_FACE_SIZE, DIVOOM_LVGL_TOP_DANMAKU_MENU_FACE_SIZE);

				Pdivoom_lvgl_top_danmaku_menu_handle->image_time_tick[j] = lv_tick_get() - rand() % 2000;
			}

			Pdivoom_lvgl_top_danmaku_menu_handle->image_start_tick = lv_tick_get();

			if(Pdivoom_lvgl_top_danmaku_menu_handle->change_flag)
				Pdivoom_lvgl_top_danmaku_menu_handle->next_update_time = 2000 + rand() % 2000 + lv_tick_get();
		}

		divoom_system_free(rand_face);
	}
}



void divoom_lvgl_top_danmaku_menu_image_double_refresh_time_handle(lv_timer_t *refresh_timer)
{
	if(Pdivoom_lvgl_top_danmaku_menu_handle == NULL)
		return;

	uint32_t current_tick, delta_time; 
	int time, flag;
	int opa, zoom, app_check_flag = 0; 

	current_tick = lv_tick_get();

	for(int i = 0; i < 2; i++)
	{
		//0~0.5s 改变透明度和放大大小，0.5~1.5s 不改变             	1.5s到2.0s缩小
		if(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj[i] != NULL)
		{
			delta_time = current_tick - Pdivoom_lvgl_top_danmaku_menu_handle->image_time_tick[i]; 
			time = delta_time % 2000;
			flag = time / 500;

			if(flag == 0)
			{
				opa = LV_OPA_100 * time / 500;
				zoom = 256 * (1 - 40/100) * time / 500;

				lv_img_set_zoom(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj[i], zoom);
				lv_obj_set_style_img_opa(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj[i], opa, 0);
			}
			else if(flag > 0 && flag < 3)
			{
				lv_img_set_zoom(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj[i], 256);
				lv_obj_set_style_img_opa(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj[i], LV_OPA_100, 0);
			}
			else if(flag == 3)
			{
				opa = LV_OPA_100 - LV_OPA_100 * (time - 1500) / 500;
				zoom = 256 * 40/100 + 256 * 60 / 100 * (time - 1500) / 500;

				lv_img_set_zoom(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj[i], zoom);
				lv_obj_set_style_img_opa(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj[i], opa, 0);
			}
			
		}

	}

	if(Pdivoom_lvgl_top_danmaku_menu_handle->image_time_tick[0] == 0 && Pdivoom_lvgl_top_danmaku_menu_handle->image_time_tick[1] == 0)
	{
		divoom_lvgl_top_danmaku_menu_image_refresh_process_double();
	}
	//else if(app_check_flag && Pdivoom_lvgl_top_danmaku_menu_handle->change_flag == 0)
	//{
	//	divoom_lvgl_top_danmaku_menu_image_refresh_process_double();
	//}
	else if(Pdivoom_lvgl_top_danmaku_menu_handle->change_flag && Pdivoom_lvgl_top_danmaku_menu_handle->next_update_time <= current_tick)
	{
		Pdivoom_lvgl_top_danmaku_menu_handle->next_update_time = 2000 + rand() % 2000 + current_tick;

		divoom_lvgl_top_danmaku_menu_add_danmaku_image_item(1, 0, -1);
		divoom_lvgl_top_danmaku_menu_image_refresh_process_double();
	}

}

void divoom_lvgl_top_danmaku_menu_add_danmaku_image_item(int change_flag, int lock_flag, int face_index)
{
	if(Pdivoom_lvgl_top_danmaku_menu_handle == NULL)
	{
		divoom_lvgl_top_danmaku_menu_init();
		
		if(Pdivoom_lvgl_top_danmaku_menu_handle == NULL)
			return;
	}
	int rand_face[2];
	int *image_id, i;

	if(lock_flag)
	{
		divoom_get_lvgl_lock();
	}
	
    if(Pdivoom_lvgl_top_danmaku_menu_handle == NULL)
    {
    	if(lock_flag)
    	{
    		divoom_free_lvgl_lock();
    	}
        return;
    }
	if(Pdivoom_lvgl_top_danmaku_menu_handle->change_flag != change_flag)
	{
		Pdivoom_lvgl_top_danmaku_menu_handle->change_flag = change_flag;

		//模式改变先删除对象和定时器
		for(i = 0; i < 2; i++)
		{
			if(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj[i])
			{
				lv_obj_del(Pdivoom_lvgl_top_danmaku_menu_handle->image_obj[i]);
				Pdivoom_lvgl_top_danmaku_menu_handle->image_obj[i] = NULL;
			}
			Pdivoom_lvgl_top_danmaku_menu_handle->pre_face_id[i] = 0;
			Pdivoom_lvgl_top_danmaku_menu_handle->image_time_tick[i] = 0;
		}
		if(Pdivoom_lvgl_top_danmaku_menu_handle->image_refresh_timer)
		{
			lv_timer_del(Pdivoom_lvgl_top_danmaku_menu_handle->image_refresh_timer);
			Pdivoom_lvgl_top_danmaku_menu_handle->image_refresh_timer = NULL;
		}

		divoom_queue_pop_all(Pdivoom_lvgl_top_danmaku_menu_handle->image_queue, NULL);	
	}

	if(change_flag)
	{
		for(i = 0; i < 2; i++)
		{
			rand_face[i] = rand() % DIVOOM_LVGL_TOP_DANMAKU_MENU_ANNIVERSARY_MAX_FACE_NUM;

			while(Pdivoom_lvgl_top_danmaku_menu_handle->pre_face_id[i] == rand_face[i])
			{
				rand_face[i] = rand() % DIVOOM_LVGL_TOP_DANMAKU_MENU_ANNIVERSARY_MAX_FACE_NUM;
			}
			Pdivoom_lvgl_top_danmaku_menu_handle->pre_face_id[i] = rand_face[i];
		}

		image_id = (int *)divoom_system_malloc(sizeof(int)*2);
		image_id[0] = divoom_top_danmaku_anniversary_img_list_index[rand_face[0]];
		image_id[1] = divoom_top_danmaku_anniversary_img_list_index[rand_face[1]];

		divoom_queue_push(Pdivoom_lvgl_top_danmaku_menu_handle->image_queue, image_id);

		if(Pdivoom_lvgl_top_danmaku_menu_handle->image_refresh_timer == NULL)
		{
			Pdivoom_lvgl_top_danmaku_menu_handle->image_refresh_timer = lv_timer_create(divoom_lvgl_top_danmaku_menu_image_double_refresh_time_handle, 100, NULL);
		}
	}
	else
	{
		image_id = (int *)divoom_system_malloc(sizeof(int));
		
		if(face_index >= 0 && face_index < DIVOOM_LVGL_TOP_DANMAKU_MENU_MAX_FACE_NUM)
		{
			*image_id = face_index;
		}
		else
		{
			rand_face[0] = rand() % DIVOOM_LVGL_TOP_DANMAKU_MENU_MAX_FACE_NUM;

			while(Pdivoom_lvgl_top_danmaku_menu_handle->pre_face_id[0] == rand_face[0])
			{
				rand_face[0] = rand() % DIVOOM_LVGL_TOP_DANMAKU_MENU_MAX_FACE_NUM;
			}
			Pdivoom_lvgl_top_danmaku_menu_handle->pre_face_id[0] = rand_face[0];

			*image_id = rand_face[0];
		}
		
		divoom_queue_push(Pdivoom_lvgl_top_danmaku_menu_handle->image_queue, image_id);

		if(Pdivoom_lvgl_top_danmaku_menu_handle->image_refresh_timer == NULL)
		{
			Pdivoom_lvgl_top_danmaku_menu_handle->image_refresh_timer = lv_timer_create(divoom_lvgl_top_danmaku_menu_image_single_refresh_time_handle, 100, NULL);
		}
	}

	if(lock_flag)
	{
		divoom_free_lvgl_lock();
	}
}



#if 0
void divoom_lvgl_top_danmaku_menu_refresh_labels(void)
{
	//对所有label重新随机
	//last_obj = NULL select_line
	int i, line, start_x;

	for(i = 0; i < DIVOOM_LVGL_TOP_DANMAKU_MENU_MAX_LINE_NUM; i++)
	{
		Pdivoom_lvgl_top_danmaku_menu_handle->line_last_obj[i] = NULL;
	}
	
	for(i = 0; i < DIVOOM_LVGL_TOP_DANMAKU_MENU_MAX_LABEL_NUM; i++)
	{
		if(Pdivoom_lvgl_top_danmaku_menu_handle->label_obj[i] != NULL)
		{
			line = divoom_lvgl_top_danmaku_menu_select_line(&start_x);
			lv_obj_set_pos(Pdivoom_lvgl_top_danmaku_menu_handle->label_obj[i], start_x, Pdivoom_lvgl_top_danmaku_menu_handle->label_start_y + line * 60);
			lv_obj_update_layout(Pdivoom_lvgl_top_danmaku_menu_handle->label_obj[i]);  // 立即更新布局获取尺寸

			Pdivoom_lvgl_top_danmaku_menu_handle->label_x[i] = start_x;
			Pdivoom_lvgl_top_danmaku_menu_handle->line_last_obj[line] = Pdivoom_lvgl_top_danmaku_menu_handle->label_obj[i];
		}
	}
}

void divoom_lvgl_top_danmaku_menu_update_danmaku_config(int lock_flag)
{
	int tmp_line_num, tmp_start_y;

	if(lock_flag)
	{
		divoom_get_lvgl_lock();
	}

	Pdivoom_lvgl_top_danmaku_menu_handle->pdanmaku_config = divoom_main_model_get_danmaku_config();
	
	tmp_line_num = Pdivoom_lvgl_top_danmaku_menu_handle->line_num;
	tmp_start_y = Pdivoom_lvgl_top_danmaku_menu_handle->label_start_y;
		
	if(Pdivoom_lvgl_top_danmaku_menu_handle->pdanmaku_config->disp_area_flag == 0)
	{
		Pdivoom_lvgl_top_danmaku_menu_handle->line_num = divoom_top_danmaku_disp_line_num[0];
		Pdivoom_lvgl_top_danmaku_menu_handle->label_start_y = 0;
	}
	else if(Pdivoom_lvgl_top_danmaku_menu_handle->pdanmaku_config->disp_area_flag == 1)
	{
		Pdivoom_lvgl_top_danmaku_menu_handle->line_num = divoom_top_danmaku_disp_line_num[1];
		Pdivoom_lvgl_top_danmaku_menu_handle->label_start_y = 0;
	}
	else if(Pdivoom_lvgl_top_danmaku_menu_handle->pdanmaku_config->disp_area_flag == 2)
	{
		Pdivoom_lvgl_top_danmaku_menu_handle->line_num = divoom_top_danmaku_disp_line_num[2];
		Pdivoom_lvgl_top_danmaku_menu_handle->label_start_y = 860;
	}
	else
	{
		Pdivoom_lvgl_top_danmaku_menu_handle->line_num = divoom_top_danmaku_disp_line_num[0];
		Pdivoom_lvgl_top_danmaku_menu_handle->label_start_y = 0;
	}
	
	if((tmp_line_num != Pdivoom_lvgl_top_danmaku_menu_handle->line_num)
		|| (tmp_start_y != Pdivoom_lvgl_top_danmaku_menu_handle->label_start_y))
	{
		//divoom_lvgl_top_danmaku_menu_refresh_labels();
	}

	if(lock_flag)
	{
		divoom_free_lvgl_lock();
	}
}

void divoom_lvgl_top_danmaku_menu_refresh_labels_y(void)
{
	int i, j;

	for(j = 0; j < DIVOOM_LVGL_TOP_DANMAKU_MENU_MAX_LINE_NUM; j++)
	{
		for(i = 0; i < DIVOOM_LVGL_TOP_DANMAKU_MENU_MAX_LINE_LABEL_NUM; i++)
		{
			if(Pdivoom_lvgl_top_danmaku_menu_handle->line_label_obj[j][i] != NULL)
			{
				lv_obj_set_y(Pdivoom_lvgl_top_danmaku_menu_handle->line_label_obj[j][i], Pdivoom_lvgl_top_danmaku_menu_handle->label_start_y + j*60);
			}
		}
	}
}

void divoom_lvgl_top_danmaku_menu_move_line_labels(void)
{
	if(Pdivoom_lvgl_top_danmaku_menu_handle->line_label_obj[5][0] == NULL) //如果超过五行才删除
	{
		return;
	}
	
	divoom_lvgl_top_danmaku_menu_delete_line_all_labels(0);
	divoom_lvgl_top_danmaku_menu_delete_line_all_labels(1);
	
	for(int i = 2; i < DIVOOM_LVGL_TOP_DANMAKU_MENU_MAX_LINE_NUM; i++)
	{
		for(int j = 0; j < DIVOOM_LVGL_TOP_DANMAKU_MENU_MAX_LINE_LABEL_NUM; j++)
		{
			if(Pdivoom_lvgl_top_danmaku_menu_handle->line_label_obj[i][j] != NULL)
			{
				Pdivoom_lvgl_top_danmaku_menu_handle->line_label_obj[i-2][j] = Pdivoom_lvgl_top_danmaku_menu_handle->line_label_obj[i][j];
				Pdivoom_lvgl_top_danmaku_menu_handle->line_label_obj[i][j] = NULL;
				lv_obj_set_y(Pdivoom_lvgl_top_danmaku_menu_handle->line_label_obj[i-2][j], Pdivoom_lvgl_top_danmaku_menu_handle->label_start_y + (i-2)*60);
			}
		}
	}

	if(Pdivoom_lvgl_top_danmaku_menu_handle->cur_line >= 2)
	{
		Pdivoom_lvgl_top_danmaku_menu_handle->cur_line -= 2;
	}
	else
	{
		Pdivoom_lvgl_top_danmaku_menu_handle->cur_line = 0;
	}
}

void divoom_lvgl_top_danmaku_menu_update_danmaku_config(int lock_flag)
{
	if(Pdivoom_lvgl_top_danmaku_menu_handle == NULL)
	{
		return;	
	}
	
	int tmp_line_num, tmp_start_y;

	if(lock_flag)
	{
		divoom_get_lvgl_lock();
	}

	Pdivoom_lvgl_top_danmaku_menu_handle->pdanmaku_config = divoom_main_model_get_danmaku_config();
	
	tmp_line_num = Pdivoom_lvgl_top_danmaku_menu_handle->line_num;
	tmp_start_y = Pdivoom_lvgl_top_danmaku_menu_handle->label_start_y;
		
	if(Pdivoom_lvgl_top_danmaku_menu_handle->pdanmaku_config->disp_area_flag == 0)
	{
		Pdivoom_lvgl_top_danmaku_menu_handle->line_num = divoom_top_danmaku_disp_line_num[0];
		Pdivoom_lvgl_top_danmaku_menu_handle->label_start_y = 0;
	}
	else if(Pdivoom_lvgl_top_danmaku_menu_handle->pdanmaku_config->disp_area_flag == 1)
	{
		Pdivoom_lvgl_top_danmaku_menu_handle->line_num = divoom_top_danmaku_disp_line_num[1];
		Pdivoom_lvgl_top_danmaku_menu_handle->label_start_y = 0;
	}
	else if(Pdivoom_lvgl_top_danmaku_menu_handle->pdanmaku_config->disp_area_flag == 2)
	{
		Pdivoom_lvgl_top_danmaku_menu_handle->line_num = divoom_top_danmaku_disp_line_num[2];
		Pdivoom_lvgl_top_danmaku_menu_handle->label_start_y = 860;
	}
	else
	{
		Pdivoom_lvgl_top_danmaku_menu_handle->line_num = divoom_top_danmaku_disp_line_num[0];
		Pdivoom_lvgl_top_danmaku_menu_handle->label_start_y = 0;
	}

	if(tmp_line_num == 7 && Pdivoom_lvgl_top_danmaku_menu_handle->line_num == 5)
	{
		divoom_lvgl_top_danmaku_menu_move_line_labels();
	}
	
	if(tmp_start_y != Pdivoom_lvgl_top_danmaku_menu_handle->label_start_y)
	{
		divoom_lvgl_top_danmaku_menu_refresh_labels_y();
	}

	if(lock_flag)
	{
		divoom_free_lvgl_lock();
	}
}
#endif


static void divoom_lvgl_top_danmaku_menu_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
	
	if(code == LV_EVENT_KEY)
    {
        char *key_info = (char *)lv_event_get_param(e);
        char c = key_info[0];

        if(Pdivoom_lvgl_top_danmaku_menu_handle == NULL)
        {
            divoom_print("test!\n");
			return;
        }

		divoom_lvgl_top_danmaku_menu_del_menu();
	}
}

void divoom_lvgl_top_danmaku_menu_init(void)
{
	divoom_get_lvgl_lock();

	if(Pdivoom_lvgl_top_danmaku_menu_handle != NULL)
	{
		divoom_free_lvgl_lock();
		return;
	}

	Pdivoom_lvgl_top_danmaku_menu_handle = (Pdivoom_lvgl_top_danmaku_menu_ctrl)divoom_system_malloc(sizeof(divoom_lvgl_top_danmaku_menu_ctrl));

	if(Pdivoom_lvgl_top_danmaku_menu_handle == NULL)
	{
		divoom_free_lvgl_lock();
		return;
	}

	memset(Pdivoom_lvgl_top_danmaku_menu_handle, 0, sizeof(divoom_lvgl_top_danmaku_menu_ctrl));
	
	#if 0
	Pdivoom_lvgl_top_danmaku_menu_handle->pdanmaku_config = divoom_main_model_get_danmaku_config();

	if(Pdivoom_lvgl_top_danmaku_menu_handle->pdanmaku_config->disp_area_flag == 0)
	{
		Pdivoom_lvgl_top_danmaku_menu_handle->line_num = divoom_top_danmaku_disp_line_num[0];
		Pdivoom_lvgl_top_danmaku_menu_handle->label_start_y = 0;
	}
	else if(Pdivoom_lvgl_top_danmaku_menu_handle->pdanmaku_config->disp_area_flag == 1)
	{
		Pdivoom_lvgl_top_danmaku_menu_handle->line_num = divoom_top_danmaku_disp_line_num[1];
		Pdivoom_lvgl_top_danmaku_menu_handle->label_start_y = 0;
	}
	else if(Pdivoom_lvgl_top_danmaku_menu_handle->pdanmaku_config->disp_area_flag == 2)
	{
		Pdivoom_lvgl_top_danmaku_menu_handle->line_num = divoom_top_danmaku_disp_line_num[2];
		Pdivoom_lvgl_top_danmaku_menu_handle->label_start_y = 860;
	}
	else
	{
		Pdivoom_lvgl_top_danmaku_menu_handle->line_num = divoom_top_danmaku_disp_line_num[0];
		Pdivoom_lvgl_top_danmaku_menu_handle->label_start_y = 0;
	}
	#endif
	
	Pdivoom_lvgl_top_danmaku_menu_handle->main_obj = lv_btn_create(lv_layer_top()); //在顶层创建主页面

	lv_obj_remove_style_all(Pdivoom_lvgl_top_danmaku_menu_handle->main_obj);
	lv_obj_set_style_bg_opa(Pdivoom_lvgl_top_danmaku_menu_handle->main_obj, LV_OPA_0, 0);
	lv_obj_set_style_pad_all(Pdivoom_lvgl_top_danmaku_menu_handle->main_obj, 0, 0);
	lv_obj_set_style_border_width(Pdivoom_lvgl_top_danmaku_menu_handle->main_obj, 0, 0);
	lv_obj_set_style_shadow_width(Pdivoom_lvgl_top_danmaku_menu_handle->main_obj, 0, 0);
	lv_obj_set_style_outline_width(Pdivoom_lvgl_top_danmaku_menu_handle->main_obj, 0, 0);
	lv_obj_clear_flag(Pdivoom_lvgl_top_danmaku_menu_handle->main_obj, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_size(Pdivoom_lvgl_top_danmaku_menu_handle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_pos(Pdivoom_lvgl_top_danmaku_menu_handle->main_obj, 0, 0);
	lv_obj_add_event_cb(Pdivoom_lvgl_top_danmaku_menu_handle->main_obj, divoom_lvgl_top_danmaku_menu_event_cb, LV_EVENT_KEY, NULL);
	
	divoom_lvgl_top_control_add_top(Pdivoom_lvgl_top_danmaku_menu_handle, Pdivoom_lvgl_top_danmaku_menu_handle->main_obj);
	
	Pdivoom_lvgl_top_danmaku_menu_handle->image_queue = divoom_queue_init(40, __func__, __LINE__);
	Pdivoom_lvgl_top_danmaku_menu_handle->label_user_queue = divoom_queue_init(20, __func__, __LINE__);

	divoom_free_lvgl_lock();
}



void divoom_lvgl_top_danmaku_menu_exit(void)
{
	if(Pdivoom_lvgl_top_danmaku_menu_handle != NULL)
	{
		if(Pdivoom_lvgl_top_danmaku_menu_handle->label_refresh_timer)
		{
			lv_timer_del(Pdivoom_lvgl_top_danmaku_menu_handle->label_refresh_timer);
		}
		if(Pdivoom_lvgl_top_danmaku_menu_handle->image_refresh_timer)
		{
			lv_timer_del(Pdivoom_lvgl_top_danmaku_menu_handle->image_refresh_timer);
		}
    	divoom_lvgl_top_control_remove_top(Pdivoom_lvgl_top_danmaku_menu_handle);

		//divoom_lvgl_top_danmaku_menu_clear_all_in();
		//divoom_queue_destroy(Pdivoom_lvgl_top_danmaku_menu_handle->disp_image_queue, NULL);

		divoom_queue_destroy(Pdivoom_lvgl_top_danmaku_menu_handle->label_user_queue, NULL);	
		divoom_queue_destroy(Pdivoom_lvgl_top_danmaku_menu_handle->image_queue, NULL);			

		lv_obj_del_async(Pdivoom_lvgl_top_danmaku_menu_handle->main_obj);
		divoom_system_free(Pdivoom_lvgl_top_danmaku_menu_handle);
    
		Pdivoom_lvgl_top_danmaku_menu_handle = NULL;
	}
}
