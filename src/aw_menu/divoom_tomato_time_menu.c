#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_menu_direct_ctrl.h"
#include "divoom_lvgl_dial_menu.h"
#include "divoom_libwebp_decode.h"
#include "divoom_tomato_time_menu.h"
#include "divoom_pixel_format.h"

#define DIVOOM_TOMATO_NAME_WIDTH    90
#define DIVOOM_TOMATO_NAME_HIGH     16
#define DIVOOM_LVGL_FONT 

typedef struct _divoom_tomato_time_icon_info
{
	void *img_cont; //图片容器
	void *img_obj; //图片对象
}divoom_tomato_time_icon_info, *Pdivoom_tomato_time_icon_info;

typedef struct _divoom_tomato_time_menu_ctrl
{
    int menu_id;
	void *main_obj; //主屏幕对象
	void *main_btn_obj; //中间大矩形
	void *time_cont; //倒计时容器
	void *time_obj; //倒计时对象
	void *tomato_name_cont; //任务名容器
	void *tomato_name_obj; //任务名称
	void *tomato_total_obj; //统计番茄个数
	void *icon_group; //图标组
	lv_obj_t *start_btn_obj;
	lv_obj_t *label_start_obj;	//开始文本框
	lv_style_t backgroud_type;
	lv_style_t time_text_type;
    lv_style_t style;
	lv_style_t style2;
	lv_style_t style3;
	lv_style_t style4;
	int tomato_start_flag; //1正在倒计时
	int tomato_index; //当前显示第几个番茄时钟
	int tomato_num; //番茄时钟的总数
	int tomato_status;
	int tomato_sub_status;
	int tomato_off; //当前执行到第几个番茄
	int tomato_id; //当前执行的id
	divoom_tomato_time_icon_info icon_info[8]; //小图标
    struct _lv_timer_t * tomato_refresh_time;

	lv_obj_t *arrow_btn_obj; //返回箭头按钮
	lv_obj_t *label_time_obj; //时间文本
	lv_obj_t *set_btn_obj; //设置按钮
	lv_obj_t *page_container; //页面滚动容器
	lv_obj_t *page_points[20]; //页面点数
	lv_point_t start_pressed_point;
	lv_point_t end_pressed_point;
	int screen_status;
	void *image_play_lock;
	uint32 image_play_time;
	Pdivoom_lvgl_dial_menu_image_ctrl pimage_handle;
	int model_id;
	int image_id;
	struct _lv_timer_t *pimage_refresh_time;
}divoom_tomato_time_menu_ctrl, *Pdivoom_tomato_time_menu_ctrl;

static Pdivoom_tomato_time_menu_ctrl phandle_tomato_ctrl = NULL;

void divoom_tomato_time_menu_update_data(void *handle, Pdivoom_tomato_time_info tomato_data);
void divoom_tomato_time_menu_control(int flag);
void divoom_tomato_time_menu_disp_name(Pdivoom_tomato_time_menu_ctrl phandle);
void divoom_tomato_time_menu_change_page(void *handle, int new_page);
void divoom_tomato_time_menu_display_time_label(void *obj);

void divoom_tomato_time_menu_set_img_dsc_t(Pdivoom_lvgl_dial_menu_image_ctrl pimage_handle, int w, int h)
{
    lv_color_format_t cf;
	pimage_handle->lvgl_image_buf = divoom_system_malloc(sizeof(lv_img_dsc_t));

	if (pimage_handle->image_type == DIVOOM_LVGL_DIAL_MENU_TYPE_DIVOOM_QUEUE || 
		pimage_handle->image_type == DIVOOM_LVGL_DIAL_MENU_TYPE_DIVOOM_EQ_DIAL|| pimage_handle->image_type == DIVOOM_LVGL_DIAL_MENU_TYPE_RGB_IMAGE)
		cf = LV_COLOR_FORMAT_RGB565;
	else  
		cf = LV_COLOR_FORMAT_RGB565A8;
	pimage_handle->lvgl_image_buf->data =
        divoom_system_malloc(divoom_pixel_format_size((uint32)w, (uint32)h, cf));
    divoom_lv_img_dsc_init_fmt(pimage_handle->lvgl_image_buf, w, h, cf,
        pimage_handle->lvgl_image_buf->data);
}

void divoom_tomato_time_menu_read_image_buf(Pdivoom_lvgl_dial_menu_image_ctrl pimage_handle, struct _lv_timer_t *user_time_handle)
{
    uint8 *frame_buf = NULL;
    uint8 *decode_tmp_buf = NULL;
    uint8 *dst_buf;
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
                    divoom_system_malloc(divoom_pixel_format_size(pimage_handle->area_w,
                        pimage_handle->area_h, pimage_handle->lvgl_image_buf->header.cf));
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
                uint8 *alpha_buf = rgb565_buf + pimage_handle->area_w *
                    pimage_handle->area_h * DIVOOM_APP_RGB565_BYTES_PER_PX;
                for(int i = 0;i < pimage_handle->area_w*pimage_handle->area_h; i++)
                {
                    if(rgb565_buf[0] == 0 && rgb565_buf[1] == 0)
                    {
                        alpha_buf[i] = 0;
                    }
                    rgb565_buf += DIVOOM_APP_RGB565_BYTES_PER_PX;
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
                    divoom_system_malloc(divoom_pixel_format_size(pimage_handle->area_w,
                        pimage_handle->area_h, pimage_handle->lvgl_image_buf->header.cf));
            }
            if(pimage_handle->lvgl_image_buf->data == NULL)
            {
                return;
            }
            dst_buf = (uint8 *)pimage_handle->lvgl_image_buf->data;
            if(!pimage_handle->image_strech &&
                pimage_handle->lvgl_image_buf->header.cf == LV_COLOR_FORMAT_RGB565A8)
            {
                frame_buf = divoom_libwebp_decode_get_frame_rgb565a8_buffer(pimage_handle->pwebp_handle,
                    &pimage_handle->frame_time, &end_flag, dst_buf);
            }
            else
            {
                decode_tmp_buf = divoom_system_malloc(divoom_pixel_format_size(
                    (uint32)pimage_handle->image_width,
                    (uint32)pimage_handle->image_high,
                    LV_COLOR_FORMAT_RGB565A8));
                if(decode_tmp_buf == NULL)
                {
                    return;
                }
                frame_buf = divoom_libwebp_decode_get_frame_rgb565a8_buffer(pimage_handle->pwebp_handle,
                    &pimage_handle->frame_time, &end_flag, decode_tmp_buf);
            }
        }
        else if(pimage_handle->resource_type == DIVOOM_RESOURCE_TYPE_JPEG)
        {
            pimage_handle->frame_time = 1000;
            end_flag = 1;
            frame_buf = divoom_decode_jpg_bg_get_lvfmt_buf(pimage_handle->pwebp_handle);
        }
        else if(pimage_handle->resource_type == DIVOOM_RESOURCE_TYPE_JPEG_GIF)
        {            
            frame_buf = divoom_decode_jpeg_gif_get_frame_rgb565a8_buffer(pimage_handle->pwebp_handle, &pimage_handle->frame_time, &end_flag, NULL);
        }
        else if(pimage_handle->resource_type == DIVOOM_RESOURCE_TYPE_GIF)
        {            
            frame_buf = divoom_gif_lib_get_frame_rgb565a8_data(pimage_handle->pwebp_handle, &pimage_handle->frame_time, &end_flag);
        }
        else
        {
            if(decode_tmp_buf)
            {
                divoom_system_free(decode_tmp_buf);
            }
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
                divoom_system_malloc(divoom_pixel_format_size(pimage_handle->area_w,
                    pimage_handle->area_h, pimage_handle->lvgl_image_buf->header.cf));
        }
        if(pimage_handle->lvgl_image_buf->data == NULL || frame_buf == NULL)
        {
            if(decode_tmp_buf)
            {
                divoom_system_free(decode_tmp_buf);
            }
            return;
        }
        dst_buf = (uint8 *)pimage_handle->lvgl_image_buf->data;
        if(frame_buf != dst_buf)
        {
            divoom_rgb565a8_to_lvfmt_nearest(frame_buf,
                (uint32)pimage_handle->image_width,
                (uint32)pimage_handle->image_high, dst_buf,
                (uint32)pimage_handle->area_w,
                (uint32)pimage_handle->area_h,
                pimage_handle->lvgl_image_buf->header.cf);
        }
        if(decode_tmp_buf)
        {
            divoom_system_free(decode_tmp_buf);
        }
        lv_obj_invalidate(pimage_handle->lvgl_image_obj);
    }
}

void divoom_tomato_time_menu_refresh_local_image_time(struct _lv_timer_t *user_time_handle)
{
    Pdivoom_lvgl_dial_menu_image_ctrl pimage_handle = (Pdivoom_lvgl_dial_menu_image_ctrl)lv_timer_get_user_data(user_time_handle);
    divoom_tomato_time_menu_read_image_buf(pimage_handle, user_time_handle);
    if(pimage_handle->frame_time > 0)
    {
        // 解码后按当前显示帧的持续时间安排下一次换帧。
        lv_timer_set_period(user_time_handle, pimage_handle->frame_time);
    }
}

extern void *divoom_lvgl_dial_menu_open_resource_handle(char *file_name, int *resource_type, int *out_w, int *out_h, int *out_num);

void divoom_tomato_time_menu_add_disp_gif(int mode_id, int image_id)
{
	if (phandle_tomato_ctrl == NULL || phandle_tomato_ctrl->pimage_handle)
		return;

	divoom_system_get_lock(phandle_tomato_ctrl->image_play_lock);
	phandle_tomato_ctrl->model_id = mode_id;
	phandle_tomato_ctrl->image_id = image_id;
	divoom_system_free_lock(phandle_tomato_ctrl->image_play_lock);
}

void divoom_tomato_time_menu_disp_gif(void)
{
	char file_name[256];

	if (phandle_tomato_ctrl == NULL || phandle_tomato_ctrl->pimage_handle)
		return;

	divoom_system_get_lock(phandle_tomato_ctrl->image_play_lock);
	if (phandle_tomato_ctrl->model_id != 0 && phandle_tomato_ctrl->image_id != 0)
	{
		divoom_manage_image_download_get_file_name(phandle_tomato_ctrl->model_id, phandle_tomato_ctrl->image_id, file_name);
		phandle_tomato_ctrl->model_id = 0;
		phandle_tomato_ctrl->image_id = 0;
	}
	else
	{
		divoom_system_free_lock(phandle_tomato_ctrl->image_play_lock);
		return;
	}
	divoom_system_free_lock(phandle_tomato_ctrl->image_play_lock);
	phandle_tomato_ctrl->pimage_handle = divoom_system_malloc(sizeof(divoom_lvgl_dial_menu_image_ctrl));
    if (phandle_tomato_ctrl->pimage_handle == NULL)
        return;            

    memset(phandle_tomato_ctrl->pimage_handle, 0, sizeof(divoom_lvgl_dial_menu_image_ctrl));
	phandle_tomato_ctrl->pimage_handle->pwebp_handle = divoom_lvgl_dial_menu_open_resource_handle(file_name, &phandle_tomato_ctrl->pimage_handle->resource_type, &phandle_tomato_ctrl->pimage_handle->area_w,
            &phandle_tomato_ctrl->pimage_handle->area_h, &phandle_tomato_ctrl->pimage_handle->frame_num);
	phandle_tomato_ctrl->pimage_handle->area_h = 256;
	phandle_tomato_ctrl->pimage_handle->area_w = 256;
    if(phandle_tomato_ctrl->pimage_handle->pwebp_handle == NULL)
    {
        divoom_system_free(phandle_tomato_ctrl->pimage_handle);
		phandle_tomato_ctrl->pimage_handle = NULL;
        return;
    }

    phandle_tomato_ctrl->pimage_handle->image_type = DIVOOM_LVGL_DIAL_MENU_TYPE_LOCAL_IMAGE;
	phandle_tomato_ctrl->pimage_handle->play_time = 5;
	phandle_tomato_ctrl->pimage_handle->cur_disp_id = 0;
	phandle_tomato_ctrl->image_play_time = divoom_system_get_ticks() + 10;

	phandle_tomato_ctrl->pimage_handle->lvgl_image_obj = lv_img_create(phandle_tomato_ctrl->main_obj);
	lv_obj_align(phandle_tomato_ctrl->pimage_handle->lvgl_image_obj, LV_ALIGN_CENTER, 0, 0);  // 居中对齐
    lv_obj_set_size(phandle_tomato_ctrl->pimage_handle->lvgl_image_obj, phandle_tomato_ctrl->pimage_handle->area_w, phandle_tomato_ctrl->pimage_handle->area_h);
	divoom_tomato_time_menu_set_img_dsc_t(phandle_tomato_ctrl->pimage_handle, phandle_tomato_ctrl->pimage_handle->area_w, phandle_tomato_ctrl->pimage_handle->area_h);
    lv_img_set_src(phandle_tomato_ctrl->pimage_handle->lvgl_image_obj, phandle_tomato_ctrl->pimage_handle->lvgl_image_buf);
	divoom_tomato_time_menu_read_image_buf(phandle_tomato_ctrl->pimage_handle, NULL);
    phandle_tomato_ctrl->pimage_refresh_time = lv_timer_create(divoom_tomato_time_menu_refresh_local_image_time, phandle_tomato_ctrl->pimage_handle->frame_time, phandle_tomato_ctrl->pimage_handle);
}

extern void divoom_lvgl_dial_menu_release_resource_handle(void *pwebp_handle, int resource_type);
void divoom_tomato_time_menu_close_gif(int flag)
{
	if (phandle_tomato_ctrl == NULL || phandle_tomato_ctrl->pimage_handle == NULL)
		return;

	if (divoom_system_get_ticks() > phandle_tomato_ctrl->image_play_time)
		flag = 1;

	if (phandle_tomato_ctrl->pimage_handle->play_time <= 0 || flag)
	{
		if (phandle_tomato_ctrl->pimage_refresh_time)
		{
			lv_timer_del(phandle_tomato_ctrl->pimage_refresh_time);
			phandle_tomato_ctrl->pimage_refresh_time = NULL;
		}
		lv_img_cache_invalidate_src(phandle_tomato_ctrl->pimage_handle->lvgl_image_buf);  
		lv_obj_del(phandle_tomato_ctrl->pimage_handle->lvgl_image_obj);

		if(phandle_tomato_ctrl->pimage_handle->image_type == DIVOOM_LVGL_DIAL_MENU_TYPE_LOCAL_IMAGE)
        {
			divoom_lvgl_dial_menu_release_resource_handle(phandle_tomato_ctrl->pimage_handle->pwebp_handle, phandle_tomato_ctrl->pimage_handle->resource_type);
        }
        else if(phandle_tomato_ctrl->pimage_handle->image_type == DIVOOM_LVGL_DIAL_MENU_TYPE_DIVOOM_IMAGE)
        {
			divoom_http_download_image_free_image_handle(phandle_tomato_ctrl->pimage_handle->pwebp_handle);
        }

		if (phandle_tomato_ctrl->pimage_handle->lvgl_image_buf)
		{
			if (phandle_tomato_ctrl->pimage_handle->lvgl_image_buf->data)
				divoom_system_free((uint8 *)phandle_tomato_ctrl->pimage_handle->lvgl_image_buf->data);

			divoom_system_free(phandle_tomato_ctrl->pimage_handle->lvgl_image_buf);
		}
		if (phandle_tomato_ctrl->pimage_handle->divoom_rgb_buf)
			divoom_system_free(phandle_tomato_ctrl->pimage_handle->divoom_rgb_buf);

		divoom_system_free(phandle_tomato_ctrl->pimage_handle);
		phandle_tomato_ctrl->pimage_handle = NULL;
	}
}

int divoom_tomato_time_menu_play_over(uint8 mode)
{
	divoom_sys_msg_info model_msg = {0};

//	model_msg.src_id = DIVOOM_MODEL_APP_MENU;
	model_msg.dest_id = DIVOOM_MODEL_APP_TOMATO_TIME;
	model_msg.msg_id = DIVOOM_APP_TOMATO_EXIT_DISP_EVENT;
	model_msg.msg_type = 1;
	divoom_sys_model_crl_msg_push(&model_msg);
	return 0;
}

void divoom_tomato_time_menu_disp_notice_pic(void)
{
	divoom_app_schedule_item_info push_info = {0};
	push_info.model_id = DIVOOM_MODEL_APP_TOMATO_TIME;
	push_info.event_id = DIVOOM_TOMATO_SCHEDULE_TOMATO_ACTIVE;
	push_info.attr = DIVOOM_APP_SCHEDULE_ATTR_WAIT;
	push_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE | DIVOOM_SCHEDULE_VOICE_RESOUCE_TYPE;
	if(divoom_app_schedule_push(&push_info))
	{
        divoom_disp_Play_net_image_image_info notice_info = {0};

        notice_info.model_id = DIVOOM_MANAGE_IMAGE_NOTICE_NET_GIF;
        notice_info.event_id = DIVOOM_MODEL_MAIN;
        notice_info.play_time = 1;
        notice_info.pcallback = divoom_tomato_time_menu_play_over;
//        notice_info.image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_NOTICE_NET_PIC + DIVOOM_LOCAL_USE_PIC_NOTICE_TOMATO_START;
        divoom_disp_ctrl_set_mode(DIVOOM_DISP_PLAY_NET_IMAGE_CTRL, (void *)&notice_info);
	}
	else
	{
		divoom_print("Here Can't get Scheduel Priority!");
	}
}

void divoom_tomato_time_menu_exit_disp(void)
{
	divoom_sys_msg_info model_msg = {0};

//	model_msg.src_id = DIVOOM_MODEL_APP_MENU;
	model_msg.dest_id = DIVOOM_MODEL_APP_TOMATO_TIME;
	model_msg.msg_id = DIVOOM_APP_TOMATO_EXIT_DISP;
	model_msg.msg_type = 1;
	divoom_sys_model_crl_msg_push(&model_msg);
}

Pdivoom_tomato_time_info divoom_tomato_time_menu_get_tomato_info(int index)
{
	Pdivoom_tomato_time_info tomato_info = NULL;

	tomato_info = divoom_tomato_time_get_one_tomato(index);
	if (tomato_info)
	{
		return tomato_info;
	}
	else
	{
		return divoom_tomato_time_get_default_tomato();
	}
}

void divoom_tomato_time_menu_control(int flag)
{
	divoom_tomato_time_set_tomato(NULL, flag);
}

void divoom_tomato_time_menu_change_tomato_info(int id)
{
	divoom_tomato_time_change_tomato_info(id);
}

Pdivoom_tomato_time_info divoom_tomato_time_menu_get_cur_tomato(int *index)
{
	//获取当前的番茄时钟
	int i = 0;	
	i = divoom_tomato_time_get_cur_tomato_index(); //得到当前的番茄任务位置
	if (i == 0xff) //不存在
		*index = 0;
	else
		*index = i;
	return divoom_tomato_time_menu_get_tomato_info(i);
}

int divoom_tomato_time_menu_get_tomato_num(void)
{
	return divoom_tomato_time_get_tomato_num();
}

int divoom_tomato_time_menu_get_disp_time(void)
{
	return divoom_tomato_time_get_time();
}

int divoom_tomato_time_menu_get_main_status(void)
{
	//返回番茄主状态
	return divoom_tomato_time_get_main_status();
}

int divoom_tomato_time_menu_get_sub_status(void)
{
	//返回番茄次状态
	return divoom_tomato_time_get_sub_status();
}

int divoom_tomato_time_menu_get_tomato_offset(void)
{
	//返回第几个番茄钟
	return divoom_tomato_time_get_tomato_offset();
}

int divoom_tomato_time_menu_get_cur_do_tomato_num(void)
{
	//返回当前执行了多个番茄时钟
	return divoom_tomato_time_get_cur_do_tomato_num();
}

static void divoom_tomato_time_menu_event_cb(lv_event_t * e)
{
	Pdivoom_tomato_time_menu_ctrl phandle = (Pdivoom_tomato_time_menu_ctrl)lv_event_get_user_data(e);
	lv_obj_t *cur_obj = lv_event_get_target(e);
	lv_event_code_t code = lv_event_get_code(e);
	Pdivoom_tomato_time_info tomato_data = NULL;

	if (code == LV_EVENT_KEY)
	{
		char c = *((char *)lv_event_get_param(e));

		if (c == DIVOOM_LVGL_KEY_PRE)
		{
			//向左滑动
			if (divoom_tomato_time_menu_get_sub_status() == TOMATO_START)
			{
				//divoom_menu_control_pass_key_to_sys(e);
				return;
			}

           	//切上一个番茄时钟
			phandle->tomato_num = divoom_tomato_time_menu_get_tomato_num();
			if (phandle->tomato_index > 0)
				phandle->tomato_index--;
			else
				phandle->tomato_index = phandle->tomato_num -1;

			tomato_data = divoom_tomato_time_menu_get_tomato_info(phandle->tomato_index);
			divoom_tomato_time_menu_update_data(phandle, tomato_data);
			divoom_tomato_time_menu_change_tomato_info(tomato_data->id);
			divoom_tomato_time_menu_change_page(phandle, phandle->tomato_index);
		}
		else if (c == DIVOOM_LVGL_KEY_NEXT)
		{
			//向右滑动
			if (divoom_tomato_time_menu_get_sub_status() == TOMATO_START)
			{
				//divoom_menu_control_pass_key_to_sys(e);
				return;
			}
		
			//切下一个番茄时钟
			phandle->tomato_index++;
			phandle->tomato_num = divoom_tomato_time_menu_get_tomato_num();
			if (phandle->tomato_index >= phandle->tomato_num)
				phandle->tomato_index = 0;

			tomato_data = divoom_tomato_time_menu_get_tomato_info(phandle->tomato_index);
			divoom_tomato_time_menu_update_data(phandle, tomato_data);
			divoom_tomato_time_menu_change_tomato_info(tomato_data->id);
			divoom_tomato_time_menu_change_page(phandle, phandle->tomato_index);
		}
		else if (c == DIVOOM_LVGL_KEY_ENTER)
		{
			if (divoom_tomato_time_menu_get_sub_status() != TOMATO_START)
			{
//					divoom_tomato_time_menu_disp_notice_pic();
				//倒计时3秒
				phandle->tomato_start_flag = 3;
			}
			else
			{
				//停止当前番茄时钟
				divoom_tomato_time_menu_control(2);
			}
		}
		else if (c == DIVOOM_LVGL_KEY_HOME) //menu键进入任务编辑
		{
			//进入编辑菜单
			tomato_data = divoom_tomato_time_menu_get_cur_tomato(&phandle->tomato_index); //获取当前正在执行的番茄时钟
			if (tomato_data)
			{
				divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_TOMA_EDIT, tomato_data, 0, 0);
			}
		}
        else if (c == DIVOOM_LVGL_KEY_ESC)
		{
			//退出
			if (divoom_tomato_time_menu_get_sub_status() == TOMATO_START)
			{
				divoom_tomato_time_menu_control(2);
			}

			divoom_menu_control_return_pre();
			divoom_tomato_time_menu_exit_disp();
		}
		else
		{
			char *key_info = (char *)lv_event_get_param(e);
			divoom_main_keypad_send_key_info(key_info[0], key_info[1]);
		}

	}
}

void divoom_tomato_time_menu_update_data(void *handle, Pdivoom_tomato_time_info tomato_data)
{
	Pdivoom_tomato_time_menu_ctrl phandle = (Pdivoom_tomato_time_menu_ctrl)handle;
	int i = 0;

	if (tomato_data == NULL)
		return;

	lv_table_set_cell_value_fmt(phandle->time_obj, 0, 0, "%d", tomato_data->work_time/10);
	lv_table_set_cell_value_fmt(phandle->time_obj, 0, 1, "%d", tomato_data->work_time%10);
	lv_table_set_cell_value(phandle->time_obj, 0, 2, ":");
	lv_table_set_cell_value(phandle->time_obj, 0, 3, "0");
	lv_table_set_cell_value(phandle->time_obj, 0, 4, "0");
	for (i = 7; i >= 2*(4 -tomato_data->roll_times); i--)
	{
		if (i%2 == 0)
		{
			divoom_menu_control_lv_img_set_src(phandle->icon_info[i].img_obj, "27/27_work.png");
		}
		else
		{
			if (i == 7)
				divoom_menu_control_lv_img_set_src(phandle->icon_info[i].img_obj, "27/27_long_reset.png");
			else
				divoom_menu_control_lv_img_set_src(phandle->icon_info[i].img_obj, "27/27_reset.png");
		}
		lv_obj_set_style_img_opa(phandle->icon_info[i].img_obj, LV_OPA_COVER, 0);
	}

	//对不显示的图标要进行隐藏
	if (tomato_data->roll_times != 4)
	{
		for (i = 0; i < 2*(4 -tomato_data->roll_times); i++)
		{
			lv_obj_set_style_img_opa(phandle->icon_info[i].img_obj, LV_OPA_0, 0);
		}
	}

	if (tomato_data->roll_times == 0)
	{
		divoom_menu_control_lv_img_set_src(phandle->icon_info[7].img_obj, "27/27_work.png");
		lv_obj_set_style_img_opa(phandle->icon_info[7].img_obj, LV_OPA_COVER, 0);
	}
}

void divoom_tomato_time_menu_refresh(void *handle)
{
    int i = 0, j = 0, index = 0, off = 0, off2 = 0;
	Pdivoom_tomato_time_menu_ctrl phandle = (Pdivoom_tomato_time_menu_ctrl)handle;
	int sec_time = divoom_tomato_time_menu_get_disp_time();

	Pdivoom_tomato_time_info ptomato = NULL;
	divoom_tomato_time_info tomato_data = {0};

	ptomato = divoom_tomato_time_menu_get_cur_tomato(&phandle->tomato_index); //获取当前正在执行的番茄时钟
	divoom_tomato_time_menu_display_time_label(phandle->label_time_obj);

	if (!ptomato)
	{
		return;
	}
	memcpy(&tomato_data, ptomato, sizeof(divoom_tomato_time_info));

	//	刷新图标
	off = divoom_tomato_time_menu_get_tomato_offset();
	if (tomato_data.id != phandle->tomato_id || phandle->tomato_off != off || 
		divoom_tomato_time_menu_get_main_status() != phandle->tomato_status || divoom_tomato_time_menu_get_sub_status() != phandle->tomato_sub_status)
	{
		phandle->tomato_id = tomato_data.id;
		phandle->tomato_sub_status = divoom_tomato_time_menu_get_sub_status();
//		divoom_print("%d, %d, %d, %d", phandle->tomato_off, off, phandle->tomato_status, divoom_tomato_time_menu_get_main_status());

		//隐藏图标
		for (i = 0; i < 2*(4 - tomato_data.roll_times); i++)
		{
			//对不显示的图标要进行隐藏
			lv_obj_set_style_img_opa(phandle->icon_info[i].img_obj, LV_OPA_0, 0);
		}

		for (i = 7; i >= 2*(4 -tomato_data.roll_times); i--) //初始化图标
		{
			if (i%2 == 0)
			{
				divoom_menu_control_lv_img_set_src(phandle->icon_info[i].img_obj, "27/27_work.png");
			}
			else
			{
				if (i == 7)
					divoom_menu_control_lv_img_set_src(phandle->icon_info[i].img_obj, "27/27_long_reset.png");
				else
					divoom_menu_control_lv_img_set_src(phandle->icon_info[i].img_obj, "27/27_reset.png");
			}
			
			lv_obj_set_style_img_opa(phandle->icon_info[i].img_obj, LV_OPA_COVER, 0);
		}

		if (tomato_data.roll_times == 0)
		{
			divoom_menu_control_lv_img_set_src(phandle->icon_info[7].img_obj, "27/27_work.png");
			lv_obj_set_style_img_opa(phandle->icon_info[7].img_obj, LV_OPA_COVER, 0);
		}

		if (divoom_tomato_time_menu_get_main_status() == TOMATO_MAIN_RESET) //休息
		{
			phandle->tomato_off = off;
			if (off)
				off2 = off -1;
			i = 2*(4 -tomato_data.roll_times) + 2*off2 + 1;
			index = i;
			if (i == 7)
				divoom_menu_control_lv_img_set_src(phandle->icon_info[i].img_obj, "27/27_long_reseting.png");
			else if (i < 7)
				divoom_menu_control_lv_img_set_src(phandle->icon_info[i].img_obj, "27/27_reseting.png");
		}
		else if (divoom_tomato_time_menu_get_main_status() == TOMATO_MAIN_FOCUS)//工作
		{
			phandle->tomato_off = off; 
			if (divoom_tomato_time_menu_get_sub_status() != TOMATO_STOP)
			{
				i = 2*(4 -tomato_data.roll_times) + 2*off;
				if (i <= 7)
					divoom_menu_control_lv_img_set_src(phandle->icon_info[i].img_obj, "27/27_working.png");
//				phandle->tomato_off = off;

				if (tomato_data.roll_times == 0)
				{
					divoom_menu_control_lv_img_set_src(phandle->icon_info[7].img_obj, "27/27_working.png");
				}
			}
		}
		if (off > 0 && tomato_data.roll_times) //把之前执行过的也设置一下
		{
			for (j = 0; j < off; j++)
			{
				i = 2*(4 -tomato_data.roll_times) + 2*j;
				divoom_menu_control_lv_img_set_src(phandle->icon_info[i].img_obj, "27/27_working.png");

				i = 2*(4 -tomato_data.roll_times) + 2*j + 1;
				if (i == 7)
					divoom_menu_control_lv_img_set_src(phandle->icon_info[i].img_obj, "27/27_long_reseting.png");
				else
					divoom_menu_control_lv_img_set_src(phandle->icon_info[i].img_obj, "27/27_reseting.png");
			}
		}
	}

	if (divoom_tomato_time_menu_get_main_status() != phandle->tomato_status)
	{
		phandle->tomato_status = divoom_tomato_time_menu_get_main_status();
		if (phandle->tomato_status == TOMATO_MAIN_FOCUS)
			lv_style_set_text_color(&phandle->time_text_type, lv_color_hex(0xE56767));
		else if(phandle->tomato_status == TOMATO_MAIN_RESET)
		{
			if (index != 7)
				lv_style_set_text_color(&phandle->time_text_type, lv_color_hex(0XF9AC64));
			else
				lv_style_set_text_color(&phandle->time_text_type, lv_color_hex(0X58D0F5));
		}
	}
	lv_table_set_cell_value_fmt(phandle->time_obj, 0, 0, "%d", sec_time/60/10);
	lv_table_set_cell_value_fmt(phandle->time_obj, 0, 1, "%d", sec_time/60%10);
	lv_table_set_cell_value(phandle->time_obj, 0, 2, ":");
	lv_table_set_cell_value_fmt(phandle->time_obj, 0, 3, "%d", sec_time%60/10);
	lv_table_set_cell_value_fmt(phandle->time_obj, 0, 4, "%d", sec_time%60%10);
    lv_label_set_text(phandle->tomato_name_obj, tomato_data.name);
	lv_label_set_text_fmt(phandle->tomato_total_obj, "x%d", divoom_tomato_time_menu_get_cur_do_tomato_num());

	if (phandle->tomato_start_flag == 0)	
	{
		if (divoom_tomato_time_menu_get_sub_status() != TOMATO_START)
			lv_label_set_text(phandle->label_start_obj, divoom_menu_control_get_disp_str("Start"));
		else
			lv_label_set_text(phandle->label_start_obj, divoom_menu_control_get_disp_str("FocusOn"));
	}
	else
	{
		lv_label_set_text_fmt(phandle->label_start_obj, "%d", phandle->tomato_start_flag);
		if (phandle->tomato_start_flag > 0)
			phandle->tomato_start_flag--;

		if (phandle->tomato_start_flag == 0)
		{
			lv_label_set_text_fmt(phandle->label_start_obj, "%d", phandle->tomato_start_flag);
			divoom_tomato_time_menu_control(5);
		}
	}
}

void divoom_tomato_time_menu_task(struct _lv_timer_t * tomato_refresh_time)
{
    Pdivoom_tomato_time_menu_ctrl phandle = (Pdivoom_tomato_time_menu_ctrl)lv_timer_get_user_data(tomato_refresh_time);
	divoom_tomato_time_menu_refresh(phandle);
	divoom_tomato_time_menu_disp_gif();
	divoom_tomato_time_menu_close_gif(0);
}

void divoom_tomato_time_menu_display_time_label(void *obj)
{
	char disp_info[10];
	struct tm *time1; 
	time_t rawtime; 
	time(&rawtime); 
	time1=localtime(&rawtime); 

	sprintf(disp_info, "%02d:%02d", time1->tm_hour, time1->tm_min);
	lv_label_set_text(obj, disp_info);
}

void divoom_tomato_time_menu_change_page(void *handle, int new_page) 
{
	Pdivoom_tomato_time_menu_ctrl phandle = (Pdivoom_tomato_time_menu_ctrl)handle;
 	int spacing = 24; 
    int point_width = 16; 
	int total_num = divoom_tomato_time_menu_get_tomato_num();
    int target_x = 0;

	new_page %= 10; 
	target_x = new_page * (point_width + spacing);

    lv_obj_scroll_to_x(phandle->page_container, target_x, LV_ANIM_OFF);

	if (total_num > 10 )
		total_num = 10;

    for (int i = 0; i < total_num; i++) 
	{
        if (i == new_page) 
		{
            lv_obj_set_style_bg_color(phandle->page_points[i], lv_color_hex(0X333333), 0);
        } 
		else 
		{
            lv_obj_set_style_bg_color(phandle->page_points[i], lv_color_hex(0XC2C2C2), 0);
        }
    }
}

void divoom_tomato_time_menu_set_points_in_scrollable_container(void *handle, int total_points) 
{
	Pdivoom_tomato_time_menu_ctrl phandle = (Pdivoom_tomato_time_menu_ctrl)handle;

    //创建一个滚动容器
    phandle->page_container = lv_obj_create(phandle->main_obj);
    lv_obj_set_height(phandle->page_container, 50); 

	lv_obj_set_style_pad_all(phandle->page_container, 0, 0);
	lv_obj_set_style_border_opa(phandle->page_container, LV_OPA_0, 0);	 
	lv_obj_set_style_bg_color(phandle->page_container, lv_color_hex(DIVOOM_TOMATO_MENU_BACK_GROUD_COLOR), 0);
	lv_obj_set_style_bg_opa(phandle->page_container, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT); //滚动条透明度设为0
	lv_obj_set_style_bg_opa(phandle->page_container, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
	lv_obj_set_style_shadow_width(phandle->page_container, 0, 0); //去掉边框底部的阴影

	lv_obj_set_scroll_dir(phandle->page_container, LV_DIR_HOR);
	lv_obj_align(phandle->page_container, LV_ALIGN_CENTER, 0, 970 - DIVOOM_SCREEN_HIGH/2 + 20);

	if (total_points > 10 )
		total_points = 10;

    //设置点的宽度和间隔
    int spacing = 24; 
    int point_width = 16; 
	int point_high = 8;
    //计算总宽度
    int total_width = total_points * point_width + (total_points) * spacing;
	
	if (total_width > DIVOOM_SCREEN_WIDTH)
		total_width = DIVOOM_SCREEN_WIDTH;		

    lv_obj_set_width(phandle->page_container, total_width);

    for (int i = 0; i < total_points; i++) 
	{
        int point_x = i * (point_width + spacing);

        lv_obj_t *point = lv_obj_create(phandle->page_container);
        lv_obj_set_size(point, point_width, point_high);
		lv_obj_set_style_radius(point, 99, 0);
        lv_obj_set_style_bg_color(point, lv_color_hex(0XC2C2C2), 0); 
		lv_obj_set_style_border_width(point, 0, 0);
		lv_obj_set_style_border_opa(point, LV_OPA_0, 0);	 
		lv_obj_set_style_bg_opa(point, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT); //滚动条透明度设为0
		lv_obj_set_style_bg_opa(point, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
		lv_obj_set_style_shadow_width(point, 0, 0); //去掉边框底部的阴影

        lv_obj_align(point, LV_ALIGN_TOP_LEFT, point_x, 0);
        phandle->page_points[i] = point;
    }
}

void *divoom_tomato_time_menu_set_return_arrow_obj(void *obj)
{
	//返回箭头
	lv_obj_t *arrow_btn_obj = lv_btn_create(obj);
	lv_obj_set_x(arrow_btn_obj, 15);
	lv_obj_set_y(arrow_btn_obj, 44);
	lv_obj_set_width(arrow_btn_obj, 72);
	lv_obj_set_height(arrow_btn_obj, 72);
	lv_obj_set_style_pad_all(arrow_btn_obj, 0, 0);
	lv_obj_set_style_border_width(arrow_btn_obj, 1, 0);
	lv_obj_set_style_border_opa(arrow_btn_obj, LV_OPA_0, 0);	 
	lv_obj_set_style_bg_color(arrow_btn_obj, lv_color_hex(DIVOOM_TOMATO_MENU_BACK_GROUD_COLOR), 0);
	lv_obj_set_style_bg_opa(arrow_btn_obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT); //滚动条透明度设为0
	lv_obj_set_style_bg_opa(arrow_btn_obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
	lv_obj_set_style_shadow_width(arrow_btn_obj, 0, 0); //去掉边框底部的阴影

	lv_obj_t *arrow_img = lv_img_create(arrow_btn_obj); //在容器里面创建图像
	lv_obj_set_align(arrow_img, LV_ALIGN_CENTER);
	divoom_menu_control_lv_img_set_src(arrow_img, "27/27_arrow.png");
	return arrow_btn_obj;
}

void *divoom_tomato_time_menu_set_label_time_obj(void *obj, int x, int y, int menu_id)
{
	lv_obj_t *label_time_obj = lv_label_create(obj);
	lv_obj_remove_style_all(label_time_obj);
	lv_obj_set_x(label_time_obj, x);
	lv_obj_set_y(label_time_obj, y);
	lv_obj_set_width(label_time_obj, 160);
	lv_obj_set_height(label_time_obj, 60);
	lv_obj_set_style_text_color(label_time_obj, lv_color_hex(DIVOOM_TOMATO_MENU_TEXT_COLOR), 0);
	lv_obj_set_style_text_font(label_time_obj, divoom_menu_control_get_font_info2(22, 60, menu_id), 0);
	lv_obj_set_style_text_align(label_time_obj, LV_TEXT_ALIGN_LEFT, 0);
	return label_time_obj;
}

void divoom_tomato_time_menu_draw_init(void *handle, void *data)
{
	Pdivoom_tomato_time_menu_ctrl phandle = (Pdivoom_tomato_time_menu_ctrl)handle;
	Pdivoom_tomato_time_info tomato_data = (Pdivoom_tomato_time_info)data;
	int i = 0;

	//时间对象
	phandle->label_time_obj = divoom_tomato_time_menu_set_label_time_obj(phandle->main_obj, 340, 60,phandle->menu_id);
	divoom_tomato_time_menu_display_time_label(phandle->label_time_obj);

	//1
	phandle->tomato_name_cont = lv_obj_create(phandle->main_obj);
//	lv_obj_remove_style_all(phandle->tomato_name_cont); //remove下面的样式不起作用
	lv_obj_set_x(phandle->tomato_name_cont, 40);
	lv_obj_set_y(phandle->tomato_name_cont, 180);
	lv_obj_set_width(phandle->tomato_name_cont, 720);
	lv_obj_set_height(phandle->tomato_name_cont, 160);
	lv_obj_set_style_bg_color(phandle->tomato_name_cont, lv_color_hex(DIVOOM_TOMATO_MENU_BACK_GROUD_COLOR), 0);
	lv_obj_set_style_bg_opa(phandle->tomato_name_cont, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT); //滚动条透明度设为0
	lv_obj_set_style_bg_opa(phandle->tomato_name_cont, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
	lv_obj_set_style_pad_all(phandle->tomato_name_cont,0,0); //对象边距设为0

	lv_style_init(&phandle->style2);
	lv_style_set_radius(&phandle->style2, 80);
	lv_style_set_border_width(&phandle->style2, 8);
	lv_style_set_border_color(&phandle->style2, lv_color_hex(DIVOOM_TOMATO_MENU_BORDER_COLOR));
	lv_obj_add_style(phandle->tomato_name_cont, &phandle->style2, 0);

	phandle->tomato_name_obj = lv_label_create(phandle->tomato_name_cont);
	lv_obj_remove_style_all(phandle->tomato_name_obj);
	lv_obj_set_x(phandle->tomato_name_obj, 60);
//	lv_obj_set_y(phandle->tomato_name_obj, 41);
	lv_obj_set_width(phandle->tomato_name_obj, 340);

	lv_style_init(&phandle->style3);
	lv_style_set_text_font(&phandle->style3, divoom_menu_control_get_font_info(60, phandle->menu_id));
	lv_style_set_text_color(&phandle->style3, lv_color_hex(DIVOOM_TOMATO_MENU_TEXT_COLOR));
	lv_obj_add_style(phandle->tomato_name_obj, &phandle->style3, 0);
	lv_obj_set_align(phandle->tomato_name_obj, LV_ALIGN_LEFT_MID);
	lv_label_set_text(phandle->tomato_name_obj, tomato_data->name);
	lv_label_set_long_mode(phandle->tomato_name_obj, LV_LABEL_LONG_SCROLL_CIRCULAR); //重新设置一下让文本从头开始
	lv_obj_set_style_anim_speed(phandle->tomato_name_obj, 30, 0); //设置滚动速度

	lv_obj_t *tomato_total_obj = lv_obj_create(phandle->tomato_name_cont);
	lv_obj_remove_style_all(tomato_total_obj);
	lv_obj_set_x(tomato_total_obj, 544 - 50);
	lv_obj_set_y(tomato_total_obj, 41);
	lv_obj_set_width(tomato_total_obj, 120 + 40+40);
	lv_obj_set_height(tomato_total_obj, 78);

	phandle->tomato_total_obj = lv_label_create(tomato_total_obj); //统计番茄个数
	lv_obj_remove_style_all(phandle->tomato_total_obj);
	lv_style_init(&phandle->style4);
	lv_style_set_text_font(&phandle->style4, divoom_menu_control_get_font_info(78, phandle->menu_id));
	lv_style_set_text_color(&phandle->style4, lv_color_hex(DIVOOM_TOMATO_MENU_TEXT_COLOR));
	lv_obj_add_style(phandle->tomato_total_obj, &phandle->style4, 0);
	lv_obj_set_align(phandle->tomato_total_obj, LV_ALIGN_LEFT_MID);
	lv_label_set_text_fmt(phandle->tomato_total_obj, "x%d", divoom_tomato_time_menu_get_cur_do_tomato_num());

	lv_obj_t *tomato_icon_obj = lv_obj_create(phandle->tomato_name_cont);
	lv_obj_remove_style_all(tomato_icon_obj);
	lv_obj_set_x(tomato_icon_obj, 452 - 30);
	lv_obj_set_y(tomato_icon_obj, 44);
	lv_obj_set_width(tomato_icon_obj, 72);
	lv_obj_set_height(tomato_icon_obj, 72);

	lv_obj_t *img_obj = lv_img_create(tomato_icon_obj); //在容器里面创建图像
	lv_obj_set_align(img_obj, LV_ALIGN_CENTER);
	divoom_menu_control_lv_img_set_src(img_obj, "27/27_working.png");
	phandle->tomato_off = -1;

	//2
	phandle->main_btn_obj = lv_obj_create(phandle->main_obj); //创建中间的大边框容器
	lv_obj_set_style_pad_all(phandle->main_btn_obj,0,0); //对象边距设为0
//	lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE); //禁用滚动条
	lv_obj_set_style_bg_color(phandle->main_btn_obj, lv_color_hex(DIVOOM_TOMATO_MENU_BACK_GROUD_COLOR), 0);
	lv_obj_set_style_bg_opa(phandle->main_btn_obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT); //滚动条透明度设为0
    lv_obj_set_style_bg_opa(phandle->main_btn_obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
	lv_obj_set_style_radius(phandle->main_btn_obj, 80, 0);
	lv_obj_set_x(phandle->main_btn_obj, 40);
	lv_obj_set_y(phandle->main_btn_obj, 380);
	lv_obj_set_width(phandle->main_btn_obj, 720);
	lv_obj_set_height(phandle->main_btn_obj, 548);

    lv_style_init(&phandle->style);
    lv_style_set_radius(&phandle->style, 8);
	lv_style_set_border_width(&phandle->style, 8);
	lv_style_set_border_color(&phandle->style, lv_color_hex(DIVOOM_TOMATO_MENU_BORDER_COLOR));
    lv_obj_add_style(phandle->main_btn_obj, &phandle->style, 0);

	phandle->time_cont = lv_obj_create(phandle->main_btn_obj); //创建倒计时容器
	lv_obj_remove_style_all(phandle->time_cont);

	lv_obj_set_width(phandle->time_cont, 680);
    lv_obj_set_height(phandle->time_cont, 314);
	lv_obj_set_x(phandle->time_cont, 20 - 15);
	lv_obj_set_y(phandle->time_cont, 41);
	lv_obj_set_style_pad_all(phandle->time_cont,0,0); //对象边距设为0
	lv_obj_set_style_bg_opa(phandle->time_cont, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(phandle->time_cont, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);

	phandle->time_obj = lv_table_create(phandle->time_cont);
	lv_obj_remove_style_all(phandle->time_obj);

	lv_style_init(&phandle->time_text_type);
	lv_style_set_text_font(&phandle->time_text_type, divoom_menu_control_get_font_info2(24, 280, phandle->menu_id));
	lv_style_set_text_color(&phandle->time_text_type, lv_color_hex(0xE56767));
    lv_obj_add_style(phandle->time_obj, &phandle->time_text_type, 0);
	lv_obj_set_style_text_align(phandle->time_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_table_set_row_cnt(phandle->time_obj, 1); //1行
    lv_table_set_col_cnt(phandle->time_obj, 5); //5列
    lv_obj_set_align(phandle->time_obj, LV_ALIGN_CENTER);

	if (tomato_data->id == 0) //当前没有番茄任务
	{
		lv_table_set_cell_value_fmt(phandle->time_obj, 0, 0, "%d", tomato_data->work_time/10);
		lv_table_set_cell_value_fmt(phandle->time_obj, 0, 1, "%d", tomato_data->work_time%10);
		lv_table_set_cell_value(phandle->time_obj, 0, 2, ":");
		lv_table_set_cell_value(phandle->time_obj, 0, 3, "0");
		lv_table_set_cell_value(phandle->time_obj, 0, 4, "0");
	}
	lv_table_set_col_width(phandle->time_obj, 0, 140); //设置单元格宽度
	lv_table_set_col_width(phandle->time_obj, 1, 140);
	lv_table_set_col_width(phandle->time_obj, 2, 80);
	lv_table_set_col_width(phandle->time_obj, 3, 140);
	lv_table_set_col_width(phandle->time_obj, 4, 140);

	if (tomato_data->roll_times > 4)
		tomato_data->roll_times = 0;

	for (i = 0; i < 8; i++)
	{
		phandle->icon_info[i].img_cont = lv_obj_create(phandle->main_btn_obj); //创建图像的容器
		lv_obj_remove_style_all(phandle->icon_info[i].img_cont);
		lv_obj_set_x(phandle->icon_info[i].img_cont, i*86 + 23 -10);
		lv_obj_set_y(phandle->icon_info[i].img_cont, 389);
		lv_obj_set_width(phandle->icon_info[i].img_cont, 72);
		lv_obj_set_height(phandle->icon_info[i].img_cont, 72);
		lv_obj_set_style_pad_all(phandle->icon_info[i].img_cont, 0, 0);
		phandle->icon_info[i].img_obj = lv_img_create(phandle->icon_info[i].img_cont); //在容器里面创建图像
		lv_obj_set_align(phandle->icon_info[i].img_obj, LV_ALIGN_CENTER);
	}

	for (i = 7; i >= 2*(4 -tomato_data->roll_times); i--) //初始化图标
	{
		if (i%2 == 0)
		{
			divoom_menu_control_lv_img_set_src(phandle->icon_info[i].img_obj, "27/27_work.png");
		}
		else
		{
			if (i == 7)
				divoom_menu_control_lv_img_set_src(phandle->icon_info[i].img_obj, "27/27_long_reset.png");
			else
				divoom_menu_control_lv_img_set_src(phandle->icon_info[i].img_obj, "27/27_reset.png");
		}
	}

	if (tomato_data->roll_times == 0)
	{
		divoom_menu_control_lv_img_set_src(phandle->icon_info[7].img_obj, "27/27_work.png");
	}	

	//3
	divoom_tomato_time_menu_set_points_in_scrollable_container(phandle, divoom_tomato_time_menu_get_tomato_num());
	divoom_tomato_time_menu_change_page(phandle, 0);

	//4
	lv_obj_t *start_obj = lv_obj_create(phandle->main_obj);
	lv_obj_set_x(start_obj, 40);
	lv_obj_set_y(start_obj, 1020);
	lv_obj_set_width(start_obj, 720);
	lv_obj_set_height(start_obj, 180);
	lv_obj_set_style_pad_all(start_obj, 0, 0);
	lv_obj_set_style_border_width(start_obj, 8, 0);
	lv_obj_set_style_border_color(start_obj, lv_color_hex(0XB0BBEF), 0);
	lv_obj_set_style_radius(start_obj, 99, 0);
	lv_obj_set_style_bg_color(start_obj, lv_color_hex(DIVOOM_TOMATO_MENU_BACK_GROUD_COLOR), 0);
	lv_obj_set_style_bg_opa(start_obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT); //滚动条透明度设为0
	lv_obj_set_style_bg_opa(start_obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
	lv_obj_set_style_shadow_width(start_obj, 0, 0); //去掉边框底部的阴影

	phandle->start_btn_obj = lv_btn_create(start_obj);
	//lv_obj_remove_style_all(phandle->btn[i]);
	lv_obj_set_width(phandle->start_btn_obj, 688 -10);
	lv_obj_set_height(phandle->start_btn_obj, 133);
	lv_obj_set_style_pad_all(phandle->start_btn_obj, 0, 0);
	lv_obj_set_style_border_width(phandle->start_btn_obj, 0, 0);
	lv_obj_set_style_border_opa(phandle->start_btn_obj, LV_OPA_0, 0);
	lv_obj_set_style_radius(phandle->start_btn_obj, 99, 0);
	lv_obj_set_style_bg_color(phandle->start_btn_obj, lv_color_hex(0x637DFF), 0);
	lv_obj_set_style_bg_opa(phandle->start_btn_obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT); //滚动条透明度设为0
	lv_obj_set_style_bg_opa(phandle->start_btn_obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
	lv_obj_set_style_shadow_width(phandle->start_btn_obj, 0, 0); //去掉边框底部的阴影
	lv_obj_set_align(phandle->start_btn_obj, LV_ALIGN_CENTER);

	phandle->label_start_obj = lv_label_create(phandle->start_btn_obj);
	lv_obj_remove_style_all(phandle->label_start_obj);
	lv_obj_set_style_text_font(phandle->label_start_obj, divoom_menu_control_get_font_info(72, phandle->menu_id), 0);
	lv_obj_set_style_text_color(phandle->label_start_obj, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_align(phandle->label_start_obj, LV_ALIGN_CENTER);

	divoom_tomato_time_menu_refresh(phandle);
	phandle->tomato_refresh_time = lv_timer_create(divoom_tomato_time_menu_task, 1000, phandle);
}

void *divoom_tomato_time_menu_init(void *user_data, int menu_id)
{
    Pdivoom_tomato_time_menu_ctrl phandle = divoom_system_malloc(sizeof(divoom_tomato_time_menu_ctrl));
	Pdivoom_tomato_time_info tomato_data = NULL;

	memset(phandle, 0, sizeof(divoom_tomato_time_menu_ctrl));
    phandle->menu_id = menu_id;
    phandle->main_obj = lv_btn_create(NULL);
    lv_style_init(&phandle->backgroud_type);
    lv_style_set_bg_color(&phandle->backgroud_type, lv_color_hex(DIVOOM_TOMATO_MENU_BACK_GROUD_COLOR));
    lv_style_set_text_color(&phandle->backgroud_type, lv_color_hex(DIVOOM_TOMATO_MENU_TEXT_COLOR));
    lv_obj_add_style(phandle->main_obj, &phandle->backgroud_type, 0);
    lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	phandle->tomato_num = divoom_tomato_time_menu_get_tomato_num();
	tomato_data = divoom_tomato_time_menu_get_cur_tomato(&phandle->tomato_index); //获取当前正在执行的番茄时钟
	phandle->tomato_id = tomato_data->id;
	divoom_tomato_time_menu_draw_init(phandle, tomato_data);
	lv_obj_add_event_cb(phandle->main_obj, divoom_tomato_time_menu_event_cb, LV_EVENT_KEY, phandle);
	phandle->image_play_lock = divoom_system_create_lock();
	phandle_tomato_ctrl = phandle;
    return phandle;
}

void divoom_tomato_time_menu_disp(void *handle, void *active)
{
	Pdivoom_tomato_time_menu_ctrl phandle = (Pdivoom_tomato_time_menu_ctrl)handle;

    lv_group_focus_obj(phandle->main_obj);
    lv_scr_load(phandle->main_obj);
	if (divoom_tomato_time_menu_update(phandle->tomato_id))
	{
		phandle->tomato_off = -1;
		divoom_tomato_time_menu_change_tomato_info(phandle->tomato_id);
	}
}

void divoom_tomato_time_menu_clean(void *handle)
{
	Pdivoom_tomato_time_menu_ctrl phandle = (Pdivoom_tomato_time_menu_ctrl)handle;

	if(phandle->tomato_refresh_time)
	{
		lv_timer_del(phandle->tomato_refresh_time);
	}
	divoom_tomato_time_menu_close_gif(1);
	divoom_system_destroy_lock(phandle->image_play_lock);
	lv_obj_del(phandle->main_obj);
	lv_style_reset(&phandle->backgroud_type);
	lv_style_reset(&phandle->time_text_type);
	lv_style_reset(&phandle->style);
	lv_style_reset(&phandle->style2);
	lv_style_reset(&phandle->style3);
	lv_style_reset(&phandle->style4);
	divoom_system_free(phandle);
}
