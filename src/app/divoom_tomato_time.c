#include "divoom.h"
#include "divoom_tomato_time.h"
#include "divoom_menu_control.h"
#include "divoom_tomato_time_menu.h"
#include "divoom_astrootoo_tools.h"
#include "divoom_lvgl_dial_menu.h"
#include "divoom_libwebp_decode.h"
#include "divoom_pixel_format.h"

#ifdef DIVOOM_TIME_FRAME_SPEAKER
#define DIVOOM_TOMATO_SCHEDULE_RESOURCE_TYPE (DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE | DIVOOM_SCHEDULE_VOICE_RESOUCE_TYPE)
#else
#define DIVOOM_TOMATO_SCHEDULE_RESOURCE_TYPE DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE
#endif

#define DIVOOM_TOMATO_FOCUS_TIME	25
#define DIVOOM_REST_TIME	5
#define DIVOOM_LONG_REST_TIME 	15
#define DIVOOM_ROLL_TIMES	4
#define DIVOOM_TOMATO_TIME_NUM_MAX 	100
#define DIVOOM_TOMATO_RESTORE_TIME (30)   //S
#define DIVOOM_TOMATO_TRY_VOICE_TIME (60)   //S
#define DIVOOM_TOMATO_FOCUS_NUM_MAX 100
#define DIVOOM_TOMATO_TIP_TIME	10
#define DIVOOM_TOMATO_TODAY_TOTAL_TABLE		"tomato_today_total"

#define DIVOOM_TOMATO_SAVE_LOCAL_CONFIG


#define DIVOOM_TOMATO_ONE_TIME 60

enum{
	DIVOOM_TOOL_TOMATO_SET,
};

enum{
	TOMATO_WAIT,
	TOMATO_TRY_LISTEN,
};

typedef struct _divoom_tomato_status
{
	DIVOOM_TOMATO_MAIN_STATUS main_status; //0专注状态 1休息状态
	DIVOOM_TOMATO_STATUS sub_status;
}divoom_tomato_status, *Pdivoom_tomato_status;

typedef struct _divoom_tomato_time_struct
{
	int next_over_time;
	int frist_get_flag; //0 还未获取 1已经获取
	int voice_start; //0 没有播放 1正在播放
	divoom_tomato_time_info tomato_info;
	divoom_tomato_time_info default_tomato_info; //保存一个默认的
	uint8 tomato_rest_time; //休息时间长度
	uint32 tomato_disp_value;
	uint32 tomato_reach_time;
	uint8 status;
	uint32 end_time;
	divoom_tomato_status tomato_status; //状态
	uint8 tomato_resume_sub; //被schedule打断前的子状态，恢复时据此回到START或PUASE
	uint8 tomato_cur_roll_times; //统计当前是第几个番茄时间
	uint32 tomato_total_times; //统计执行了多少个番茄时间
	int tomato_index; //记录当前是第几个番茄任务
	int tomato_cur_num; //记录当前收到的番茄任务数
	int tomato_total_num; //记录番茄任务总数
	void *tomato_image_lock;
	void *tomato_queue;
	void *tomato_focus_queue[DIVOOM_TOMATO_TIME_NUM_MAX]; //每个番茄时间的专注记录用1个队列保存
	uint32 update_tomato_flag;
	uint32 image_notice_time;
	struct _lv_timer_t *pimage_refresh_time;
	Pdivoom_lvgl_dial_menu_image_ctrl pimage_handle;
}divoom_tomato_time_struct, *Pdivoom_tomato_time_struct;

typedef struct _divoom_today_tomato_info
{
	int year;
	int mon;
	int day;
	int total_times;
}divoom_today_tomato_info, *Pdivoom_today_tomato_info;


/* AstroToo 内置提示音：与闹钟经典 12 项同源（SoundType=0, SoundSystem=index） */
const uint8 divoom_tomato_voice_table[] =
{
	DIVOOM_VOICE_ID_CLOCK_RAINSTORM, DIVOOM_VOICE_ID_CLOCK_OCEAN_WAVES,
	DIVOOM_VOICE_ID_CLOCK_URBAN, DIVOOM_VOICE_ID_CLOCK_FOREST,
	DIVOOM_VOICE_ID_CLOCK_WHALE, DIVOOM_VOICE_ID_CLOCK_ENERGIZE,
	DIVOOM_VOICE_ID_CLOCK_EAGLE, DIVOOM_VOICE_ID_CLOCK_CAMPANULA,
	DIVOOM_VOICE_ID_CLOCK_NORMAL1, DIVOOM_VOICE_ID_CLOCK_PIANO,
	DIVOOM_VOICE_ID_CLOCK_NORMAL3, DIVOOM_VOICE_ID_CLOCK_NORMAL2,
};

#define DIVOOM_TOMATO_VOICE_NUM  (sizeof(divoom_tomato_voice_table) / sizeof(divoom_tomato_voice_table[0]))

static Pdivoom_tomato_time_struct ptomato_time_handle = NULL;
static uint8 tomato_gif_start_pending;

static void divoom_tomato_time_start_gif_download_cb(int ret);
int divoom_tomato_time_start_notice_gif(void);

void divoom_tomato_time_save_all_tomato_info_local(void);
void divoom_tomato_time_delete_focus_record(Pdivoom_tomato_time_focus_done_json done_info, int flag);
void divoom_tomato_time_save_focus_record(void);
void divoom_tomato_time_delete_tomato_info(int id);
void divoom_tomato_time_get_todoy_tomato_total(uint8 flag);

int divoom_tomato_time_play_over(uint8 mode)
{
	divoom_sys_msg_info model_msg = {0};

//	model_msg.src_id = DIVOOM_MODEL_APP_MENU;
	model_msg.dest_id = DIVOOM_MODEL_APP_TOMATO_TIME;
	model_msg.msg_id = DIVOOM_APP_TOMATO_EXIT_DISP_EVENT;
//	model_msg.msg_type = 1;
	divoom_sys_model_crl_msg_push(&model_msg);
	return 0;
}

void divoom_tomato_time_set_img_dsc_t(Pdivoom_lvgl_dial_menu_image_ctrl pimage_handle, int w, int h)
{
	lv_img_dsc_t *img_info = divoom_system_malloc(sizeof(lv_img_dsc_t));
    lv_color_format_t cf;
	pimage_handle->lvgl_image_buf = img_info;

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

void divoom_tomato_time_read_image_buf(Pdivoom_lvgl_dial_menu_image_ctrl pimage_handle, struct _lv_timer_t *user_time_handle)
{
    uint8 *frame_buf = NULL;
    uint8 *decode_tmp_buf = NULL;
    uint8 *dst_buf;
	int end_flag = 0;

	if (pimage_handle->play_time == 0)
		return;

	if (pimage_handle->image_type == DIVOOM_LVGL_DIAL_MENU_TYPE_LOCAL_IMAGE)
	{
        if(pimage_handle->resource_type == DIVOOM_RESOURCE_TYPE_DIVOOM)
        {
            Pdivoom_http_download_image_file_info pimage_info = pimage_handle->pwebp_handle;

            pimage_handle->frame_time = pimage_info->speed;
            if(pimage_handle->cur_disp_id >= pimage_info->num -1)
            {
                pimage_handle->cur_disp_id = 0;
				if (pimage_handle->play_time > 0)
					pimage_handle->play_time--;

				if (pimage_handle->play_time == 0)
					return;
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
            /* 最后一轮已播完 frame_num 帧：勿再 decode（webp 会 reset 并写回第 0 帧导致闪屏） */
            if (pimage_handle->play_time == 1 &&
                pimage_handle->frame_num > 0 &&
                pimage_handle->cur_disp_id >= pimage_handle->frame_num)
            {
                pimage_handle->play_time = 0;
                return;
            }
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
        else if(pimage_handle->resource_type == DIVOOM_RESOURCE_TYPE_GIF)
        {            
            frame_buf = divoom_gif_lib_get_frame_rgb565a8_data(pimage_handle->pwebp_handle, &pimage_handle->frame_time, &end_flag);
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

			if (pimage_handle->play_time == 0)
            {
                if(decode_tmp_buf)
                {
                    divoom_system_free(decode_tmp_buf);
                }
				return;
            }
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
		pimage_handle->cur_disp_id++;
    }
}

void divoom_tomato_time_refresh_local_image_time(struct _lv_timer_t *user_time_handle)
{
    Pdivoom_lvgl_dial_menu_image_ctrl pimage_handle = (Pdivoom_lvgl_dial_menu_image_ctrl)lv_timer_get_user_data(user_time_handle);
    
    divoom_tomato_time_read_image_buf(pimage_handle, user_time_handle);
    if(pimage_handle->frame_time > 0)
    {
        // 解码后按当前显示帧的持续时间安排下一次换帧。
        lv_timer_set_period(user_time_handle, pimage_handle->frame_time);
    }
}

extern void *divoom_lvgl_dial_menu_open_resource_handle(char *file_name, int *resource_type, int *out_w, int *out_h, int *out_num);
extern void divoom_lvgl_dial_menu_release_resource_handle(void *pwebp_handle, int resource_type);

void divoom_tomato_time_disp_gif(int mode_id, int image_id)
{
	char file_name[256];

	if (ptomato_time_handle == NULL || ptomato_time_handle->pimage_handle)
		return;

	divoom_manage_image_download_get_file_name(mode_id, image_id, file_name);
	ptomato_time_handle->pimage_handle = divoom_system_malloc(sizeof(divoom_lvgl_dial_menu_image_ctrl));
    if (ptomato_time_handle->pimage_handle == NULL)
        return;            

    memset(ptomato_time_handle->pimage_handle, 0, sizeof(divoom_lvgl_dial_menu_image_ctrl));
	ptomato_time_handle->pimage_handle->pwebp_handle = divoom_lvgl_dial_menu_open_resource_handle(file_name, &ptomato_time_handle->pimage_handle->resource_type, &ptomato_time_handle->pimage_handle->area_w,
            &ptomato_time_handle->pimage_handle->area_h, &ptomato_time_handle->pimage_handle->frame_num);
    if(ptomato_time_handle->pimage_handle->pwebp_handle == NULL)
    {
		divoom_manage_image_del_direct(DIVOOM_MANAGE_IMAGE_TOMATO_TIME_FILE, &image_id, 1);
        divoom_system_free(ptomato_time_handle->pimage_handle);
		ptomato_time_handle->pimage_handle = NULL;
        return;
    }

    ptomato_time_handle->pimage_handle->image_type = DIVOOM_LVGL_DIAL_MENU_TYPE_LOCAL_IMAGE;
	ptomato_time_handle->pimage_handle->play_time = 1;
	ptomato_time_handle->pimage_handle->cur_disp_id = 0;
	if (image_id == DIVOOM_MANAGE_IMAGE_ID_BASE_TOMATO_GIF + 1)
	{
		ptomato_time_handle->image_notice_time = 5 + divoom_system_get_ticks();
	}
	else 
	{
		ptomato_time_handle->image_notice_time = 10 + divoom_system_get_ticks();
	}

	divoom_get_lvgl_lock();
	ptomato_time_handle->pimage_handle->lvgl_top_obj = lv_obj_create(lv_layer_top());
	lv_obj_set_style_radius(ptomato_time_handle->pimage_handle->lvgl_top_obj, 0, 0);
	lv_obj_set_style_bg_color(ptomato_time_handle->pimage_handle->lvgl_top_obj, lv_color_hex(0x0), 0);
	lv_obj_set_style_border_width(ptomato_time_handle->pimage_handle->lvgl_top_obj, 0, 0);
	lv_obj_set_style_shadow_width(ptomato_time_handle->pimage_handle->lvgl_top_obj, 0, 0);
    lv_obj_set_size(ptomato_time_handle->pimage_handle->lvgl_top_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	ptomato_time_handle->pimage_handle->lvgl_image_obj = lv_img_create(ptomato_time_handle->pimage_handle->lvgl_top_obj);
	lv_obj_align(ptomato_time_handle->pimage_handle->lvgl_image_obj, LV_ALIGN_CENTER, 0, 0);  // 居中对齐
    lv_obj_set_size(ptomato_time_handle->pimage_handle->lvgl_image_obj, ptomato_time_handle->pimage_handle->area_w, ptomato_time_handle->pimage_handle->area_h);
	divoom_tomato_time_set_img_dsc_t(ptomato_time_handle->pimage_handle, ptomato_time_handle->pimage_handle->area_w, ptomato_time_handle->pimage_handle->area_h);
	lv_img_set_src(ptomato_time_handle->pimage_handle->lvgl_image_obj, ptomato_time_handle->pimage_handle->lvgl_image_buf);
	divoom_tomato_time_read_image_buf(ptomato_time_handle->pimage_handle, NULL);
    ptomato_time_handle->pimage_refresh_time = lv_timer_create(divoom_tomato_time_refresh_local_image_time, ptomato_time_handle->pimage_handle->frame_time, ptomato_time_handle->pimage_handle);
	divoom_free_lvgl_lock();
}

void divoom_tomato_time_close_gif(int flag)
{
	int will_close = 0;

	if (ptomato_time_handle == NULL || ptomato_time_handle->pimage_handle == NULL)
		return;

	if (ptomato_time_handle->pimage_handle->play_time > 0)
	{
		if (divoom_system_get_ticks() > ptomato_time_handle->image_notice_time)
			flag = 1;
	}

	will_close = (ptomato_time_handle->pimage_handle->play_time <= 0 || flag);

	if (will_close)
	{
		divoom_get_lvgl_lock();
		if (ptomato_time_handle->pimage_refresh_time)
		{
			lv_timer_del(ptomato_time_handle->pimage_refresh_time);
			ptomato_time_handle->pimage_refresh_time = NULL;
		}
		lv_img_cache_invalidate_src(ptomato_time_handle->pimage_handle->lvgl_image_buf); //清理缓存
		lv_obj_del(ptomato_time_handle->pimage_handle->lvgl_top_obj);
		divoom_free_lvgl_lock();

		divoom_tomato_time_play_over(0);

		if(ptomato_time_handle->pimage_handle->image_type == DIVOOM_LVGL_DIAL_MENU_TYPE_LOCAL_IMAGE)
        {
			divoom_lvgl_dial_menu_release_resource_handle(ptomato_time_handle->pimage_handle->pwebp_handle, ptomato_time_handle->pimage_handle->resource_type);
        }
        else if(ptomato_time_handle->pimage_handle->image_type == DIVOOM_LVGL_DIAL_MENU_TYPE_DIVOOM_IMAGE)
        {
			divoom_http_download_image_free_image_handle(ptomato_time_handle->pimage_handle->pwebp_handle);
        }
		if (ptomato_time_handle->pimage_handle->lvgl_image_buf)
		{
			if (ptomato_time_handle->pimage_handle->lvgl_image_buf->data)
				divoom_system_free((uint8 *)ptomato_time_handle->pimage_handle->lvgl_image_buf->data);

			divoom_system_free(ptomato_time_handle->pimage_handle->lvgl_image_buf);
		}
		if (ptomato_time_handle->pimage_handle->divoom_rgb_buf)
			divoom_system_free(ptomato_time_handle->pimage_handle->divoom_rgb_buf);

		divoom_system_free(ptomato_time_handle->pimage_handle);
		ptomato_time_handle->pimage_handle = NULL;
	}
}

void divoom_tomato_time_download_start_gif(void)
{
	divoom_manage_image_item_info image_info;
	image_info.image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_TOMATO_GIF + 1;
	strcpy(image_info.file_addr, "group1/M00/02/62/Ci0s8WeltCuEMcQMAAAAAC7URKo16.webp");
	image_info.flag = 1;
	image_info.pcallback = divoom_tomato_time_start_gif_download_cb;
	divoom_manage_image_add(DIVOOM_MANAGE_IMAGE_TOMATO_TIME_FILE, &image_info, 1);
}

static void divoom_tomato_time_start_gif_download_cb(int ret)
{
	if (!tomato_gif_start_pending)
		return;

	tomato_gif_start_pending = 0;
	if (ret && divoom_tomato_time_start_notice_gif())
		return;

	divoom_tomato_time_menu_control(4);
}

int divoom_tomato_time_start_notice_gif(void)
{
	if (!divoom_manage_image_check_exist(DIVOOM_MANAGE_IMAGE_TOMATO_TIME_FILE, DIVOOM_MANAGE_IMAGE_ID_BASE_TOMATO_GIF + 1))
    {
		divoom_tomato_time_download_start_gif();
		return 0;
    }
	//创建一个动画进行显示
	divoom_system_get_lock(ptomato_time_handle->tomato_image_lock);
	divoom_tomato_time_disp_gif(DIVOOM_MANAGE_IMAGE_TOMATO_TIME_FILE, DIVOOM_MANAGE_IMAGE_ID_BASE_TOMATO_GIF + 1);
	divoom_system_free_lock(ptomato_time_handle->tomato_image_lock);
	return 1;
}

void divoom_tomato_time_disp_notice_gif(void)
{
	/* 新番茄页叠 256 完成动画；无本地文件则 skip。勿再走旧 tomato_time_menu GIF。 */
	divoom_astrootoo_tools_pomo_request_notice_anim();
}

void divoom_tomato_time_db_get_data_callback(void *para, int row, int col, char *data)
{
	Pdivoom_today_tomato_info today_tomato_info = (Pdivoom_today_tomato_info)para;

//	divoom_print("wft test row %d, col %d, data: %s\n", row, col, data);

	if (para == NULL)
		return;

	if (col == 1)
	{
		today_tomato_info->total_times = atoi(data);
	}
	else if (col == 2)
	{
		today_tomato_info->year = atoi(data);
	}
	else if (col == 3)
	{
		today_tomato_info->mon = atoi(data);
	}
	else if (col == 4)
	{
		today_tomato_info->day = atoi(data);
	}
}

void divoom_tomato_time_set_vol(uint8 vol)
{
	divoom_local_music_ctrl_set_tone_volume(vol);
}

void divoom_tomato_time_get_tomato_list(void)
{
	int finish_flag = 0; //0 未完成 1已完成
	divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_TOMATO_TIME_GET_LIST, DIVOOM_SERVER_TOPIC, (void *)&finish_flag);
}

void divoom_tomato_time_cancel(void)
{
	divoom_app_schedule_item_info tomato_request_info = {0};
    tomato_request_info.model_id = DIVOOM_MODEL_APP_TOMATO_TIME;
    tomato_request_info.event_id = DIVOOM_TOMATO_SCHEDULE_TOMATO_ACTIVE;
    tomato_request_info.attr = DIVOOM_APP_SCHEDULE_ATTR_CANCEL;
	tomato_request_info.resource_type = DIVOOM_TOMATO_SCHEDULE_RESOURCE_TYPE;
    divoom_app_schedule_cancel(&tomato_request_info);
}

int divoom_tomato_time_try_listen_over(char *music_name, int status)
{
    divoom_app_schedule_item_info tomato_request_info = {0};
    tomato_request_info.model_id = DIVOOM_MODEL_APP_TOMATO_TIME;
    tomato_request_info.event_id = DIVOOM_TOMATO_SCHEDULE_TOMATO_TRY_VOICE;
    tomato_request_info.attr = DIVOOM_APP_SCHEDULE_ATTR_CANCEL;
	tomato_request_info.resource_type = DIVOOM_TOMATO_SCHEDULE_RESOURCE_TYPE;
    divoom_app_schedule_cancel(&tomato_request_info);
	return 0;
}

void divoom_tomato_time_try_listen(uint8 voice_id, uint8 vol, uint8 on_off, uint8 lvgl)
{
    divoom_app_schedule_item_info alarm_request_info = {0};
	char path_name[128];

    if(on_off == 0)
    {
		if(ptomato_time_handle->status == TOMATO_TRY_LISTEN)
		{
			ptomato_time_handle->end_time = 0;
			divoom_tomato_time_try_listen_over(NULL, 0);
			divoom_local_music_ctrl_stop_tone();
			ptomato_time_handle->status = TOMATO_WAIT;
		}
        return;
    }

	if (voice_id >= DIVOOM_TOMATO_VOICE_NUM)
	{
		return;
	}

	if (lvgl == 0)
	{
	    alarm_request_info.model_id = DIVOOM_MODEL_APP_TOMATO_TIME;
	    alarm_request_info.event_id = DIVOOM_TOMATO_SCHEDULE_TOMATO_TRY_VOICE;
	    alarm_request_info.attr = DIVOOM_APP_SCHEDULE_ATTR_CANCEL;
		alarm_request_info.resource_type = DIVOOM_TOMATO_SCHEDULE_RESOURCE_TYPE;
	    if(divoom_app_schedule_push(&alarm_request_info)!= DIVOOM_APP_SCHEDULE_RET_RUN)
	    {
//	        return;
		}
	}
    ptomato_time_handle->status = TOMATO_TRY_LISTEN;
    ptomato_time_handle->end_time = divoom_system_get_ticks() + DIVOOM_TOMATO_TRY_VOICE_TIME;
	divoom_voice_read_get_filename(divoom_tomato_voice_table[voice_id], path_name);
	divoom_local_music_ctrl_tone_play(path_name, vol, divoom_tomato_time_try_listen_over, DIVOOM_TOMATO_TRY_VOICE_TIME);	
}

int divoom_tomato_time_find_tomato_compare(void *item1, void *item2)
{
    int *tomato_id = (int *)item1;
    Pdivoom_tomato_time_info data_item = (Pdivoom_tomato_time_info)item2;

    if(data_item->id == *tomato_id)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

void divoom_tomato_time_set_system_bell(uint8 system_index)
{
	Pdivoom_tomato_time_info data_item;

	if (!ptomato_time_handle)
	{
		return;
	}
	if (system_index >= DIVOOM_TOMATO_VOICE_NUM)
	{
		system_index = 0;
	}

	ptomato_time_handle->tomato_info.sound_type = 0;
	ptomato_time_handle->tomato_info.sound_system_index = system_index;

	data_item = divoom_queue_get_cmp(ptomato_time_handle->tomato_queue,
		&ptomato_time_handle->tomato_info.id, divoom_tomato_time_find_tomato_compare);
	if (data_item)
	{
		data_item->sound_type = 0;
		data_item->sound_system_index = system_index;
		data_item->update_flag = 1;
	}
	if (ptomato_time_handle->tomato_info.id == 0)
	{
		ptomato_time_handle->default_tomato_info.sound_type = 0;
		ptomato_time_handle->default_tomato_info.sound_system_index = system_index;
		ptomato_time_handle->default_tomato_info.update_flag = 1;
	}
#ifdef DIVOOM_TOMATO_SAVE_LOCAL_CONFIG
	divoom_tomato_time_save_all_tomato_info_local();
#endif
}

void divoom_tomato_time_set_sound_on_off(uint8 on_off)
{
	Pdivoom_tomato_time_info data_item;

	if (!ptomato_time_handle)
	{
		return;
	}

	on_off = on_off ? 1 : 0;
	ptomato_time_handle->tomato_info.sound_on_off = on_off;

	data_item = divoom_queue_get_cmp(ptomato_time_handle->tomato_queue,
		&ptomato_time_handle->tomato_info.id, divoom_tomato_time_find_tomato_compare);
	if (data_item)
	{
		data_item->sound_on_off = on_off;
		data_item->update_flag = 1;
	}
	if (ptomato_time_handle->tomato_info.id == 0)
	{
		ptomato_time_handle->default_tomato_info.sound_on_off = on_off;
		ptomato_time_handle->default_tomato_info.update_flag = 1;
	}
#ifdef DIVOOM_TOMATO_SAVE_LOCAL_CONFIG
	divoom_tomato_time_save_all_tomato_info_local();
#endif
}

//设备端菜单改时长/重复次数后写回，联网上报由menu_update与dev_update_all负责
void divoom_tomato_time_set_local_config(uint8 work_min, uint8 short_min, uint8 long_min, uint8 roll_times)
{
	Pdivoom_tomato_time_info data_item;

	if (!ptomato_time_handle)
	{
		return;
	}
	if (work_min == 0 || short_min == 0 || long_min == 0)
	{
		return;
	}
	if (roll_times > DIVOOM_ROLL_TIMES)
	{
		roll_times = DIVOOM_ROLL_TIMES;
	}

	ptomato_time_handle->tomato_info.work_time = (int8)work_min;
	ptomato_time_handle->tomato_info.short_rest_time = (int8)short_min;
	ptomato_time_handle->tomato_info.long_rest_time = (int8)long_min;
	ptomato_time_handle->tomato_info.roll_times = roll_times;

	data_item = divoom_queue_get_cmp(ptomato_time_handle->tomato_queue,
		&ptomato_time_handle->tomato_info.id, divoom_tomato_time_find_tomato_compare);
	if (data_item)
	{
		data_item->work_time = (int8)work_min;
		data_item->short_rest_time = (int8)short_min;
		data_item->long_rest_time = (int8)long_min;
		data_item->roll_times = roll_times;
		data_item->update_flag = 1;
	}
	if (ptomato_time_handle->tomato_info.id == 0)
	{
		ptomato_time_handle->default_tomato_info.work_time = (int8)work_min;
		ptomato_time_handle->default_tomato_info.short_rest_time = (int8)short_min;
		ptomato_time_handle->default_tomato_info.long_rest_time = (int8)long_min;
		ptomato_time_handle->default_tomato_info.roll_times = roll_times;
		ptomato_time_handle->default_tomato_info.update_flag = 1;
	}

	//未在跑时同步刷新到点时刻，让idle显示立即跟上新时长
	if (ptomato_time_handle->tomato_status.sub_status == TOMATO_STOP)
	{
		ptomato_time_handle->tomato_reach_time = divoom_system_get_ticks()
			+ ptomato_time_handle->tomato_info.work_time * DIVOOM_TOMATO_ONE_TIME;
	}

#ifdef DIVOOM_TOMATO_SAVE_LOCAL_CONFIG
	divoom_tomato_time_save_all_tomato_info_local();
#endif
}

int divoom_tomato_time_find_focus_compare(void *item1, void *item2)
{
    int *start_time = (int *)item1;
	Pdivoom_tomato_time_focus_action_json data_item = (Pdivoom_tomato_time_focus_action_json)item2;

    if(data_item->start_time == *start_time)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

void divoom_tomato_time_update_one_tomato_info(Pdivoom_tomato_time_info tomato_info, int download_flag)
{
    uint32 i, image_id;
    char file_name[128];
    Pdivoom_tomato_time_info data_item;
    divoom_manage_image_item_info image_info;

    data_item = divoom_queue_get_cmp(ptomato_time_handle->tomato_queue, &tomato_info->id, divoom_tomato_time_find_tomato_compare);
    if(data_item)
    {
		if(strcmp(data_item->image_file_id, tomato_info->image_file_id))
		{
		    image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_TOMATO_GIF + tomato_info->id;
		    divoom_manage_image_del_direct(DIVOOM_MANAGE_IMAGE_TOMATO_TIME_FILE, &image_id, 1);
		}

		if(strcmp(data_item->sound_voice_file_id, tomato_info->sound_voice_file_id))
		{
			image_id = DIVOOM_VOICE_ID_START_TOMATO_VOICE + tomato_info->id;
			divoom_manage_image_del_direct(DIVOOM_MANAGE_IMAGE_LEAVEMSG_FILE, &image_id, 1);
		}
		memcpy(data_item, tomato_info, sizeof(divoom_tomato_time_info));
		ptomato_time_handle->update_tomato_flag = 1;
    }
    else
    {
        image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_TOMATO_GIF + tomato_info->id;
        divoom_manage_image_del_direct(DIVOOM_MANAGE_IMAGE_TOMATO_TIME_FILE, &image_id, 1);

		image_id = DIVOOM_VOICE_ID_START_TOMATO_VOICE + tomato_info->id;
		divoom_manage_image_del_direct(DIVOOM_MANAGE_IMAGE_LEAVEMSG_FILE, &image_id, 1);

        
        data_item = divoom_system_malloc(sizeof(divoom_tomato_time_info));
        if(data_item == NULL)
        {
            divoom_print("%s: it is the error and pls check it!\n",__func__);
            return;
        }

		memcpy(data_item, tomato_info, sizeof(divoom_tomato_time_info));
        divoom_queue_push(ptomato_time_handle->tomato_queue, data_item);
		ptomato_time_handle->update_tomato_flag = 1;
    }

    if (strlen(tomato_info->image_file_id))
    {
        image_info.image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_TOMATO_GIF + tomato_info->id;
    	strcpy(image_info.file_addr, tomato_info->image_file_id);
    	image_info.flag = download_flag;
    	image_info.pcallback = divoom_app_com_finishcallback;
    	divoom_manage_image_add(DIVOOM_MANAGE_IMAGE_TOMATO_TIME_FILE, &image_info, 1);
    }

	if (strlen(tomato_info->sound_voice_file_id))
    {
        image_info.image_id = DIVOOM_VOICE_ID_START_TOMATO_VOICE + tomato_info->id;
    	strcpy(image_info.file_addr, tomato_info->sound_voice_file_id);
    	image_info.flag = download_flag;
    	image_info.pcallback = divoom_app_com_finishcallback;
    	divoom_manage_image_add_font(DIVOOM_MANAGE_IMAGE_LEAVEMSG_FILE, &image_info, 1);
    }
}

void divoom_tomato_time_update_tomato_info(void *data, uint8 flag)
{
	int i = 0, j = 0, update_flag = 0;
	Pdivoom_tomato_time_get_list_json tomato_info = (Pdivoom_tomato_time_get_list_json)data;
	Pdivoom_tomato_time_info tomato_time_item = NULL;
	divoom_tomato_time_focus_done_json focus_done;

    if(tomato_info == NULL)
        return;

	if (flag == 0 && tomato_info->tomato_time_list[0].finish_flag)
	{
		//删除这个id
		divoom_tomato_time_delete_tomato_info(tomato_info->tomato_time_list[0].id);
#ifdef DIVOOM_TOMATO_SAVE_LOCAL_CONFIG
		divoom_tomato_time_save_all_tomato_info_local();
		divoom_tomato_time_save_focus_record();
#endif
		return;
	}

	if (flag == 1)
	{
		for (i = 0; i < DIVOOM_TOMATO_TIME_NUM_MAX; i++)
		{
			tomato_time_item = divoom_queue_get_index_item(ptomato_time_handle->tomato_queue, i);

			if (tomato_time_item)
			{
				for (j = 0; j < tomato_info->tomato_time_num; j++)
				{
					if (tomato_time_item->id == tomato_info->tomato_time_list[i].id)
						break;
				}

				if (j == tomato_info->tomato_time_num) //这个id不存在了
				{
					//删除该id的专注记录
					focus_done.tomato_id = tomato_time_item->id;
					divoom_tomato_time_delete_focus_record(&focus_done, 1);
					divoom_queue_pop_item(ptomato_time_handle->tomato_queue, tomato_time_item); //出队列
					divoom_system_free(tomato_time_item);//释放
					update_flag = 1;
				}
			}
		}

#ifdef DIVOOM_TOMATO_SAVE_LOCAL_CONFIG
		if (update_flag)
			divoom_tomato_time_save_focus_record();//更新记录
#endif
	}

	for(i = 0; i < tomato_info->tomato_time_num; i++)
	{
        divoom_tomato_time_update_one_tomato_info(&tomato_info->tomato_time_list[i], 1);

		if (ptomato_time_handle->tomato_info.id == tomato_info->tomato_time_list[i].id) //当前正在进行要改变
		{
			memcpy(&ptomato_time_handle->tomato_info, &tomato_info->tomato_time_list[i], sizeof(divoom_tomato_time_info));
		}

		if (ptomato_time_handle->tomato_info.id == 0)
		{
			memcpy(&ptomato_time_handle->tomato_info, &tomato_info->tomato_time_list[i], sizeof(divoom_tomato_time_info));
		}
	}
#ifdef DIVOOM_TOMATO_SAVE_LOCAL_CONFIG
	divoom_tomato_time_save_all_tomato_info_local();
#endif
}

void divoom_tomato_time_update_tomato_list_init(void)
{
	int i = 0;
	Pdivoom_tomato_time_info tomato_time_item = NULL;

	for (i = 0; i < DIVOOM_TOMATO_TIME_NUM_MAX; i++)
	{
		tomato_time_item = divoom_queue_get_index_item(ptomato_time_handle->tomato_queue, i);

		if (tomato_time_item)
		{
			tomato_time_item->exist_flag = 0; //标记不存在
		}
	}
}

void divoom_tomato_time_update_tomato_list_one_by_one(void *data)
{
	int i = 0, j = 0, update_flag = 0;
	Pdivoom_tomato_time_get_list_json tomato_info = (Pdivoom_tomato_time_get_list_json)data;
	Pdivoom_tomato_time_info tomato_time_item = NULL;

    if(tomato_info == NULL)
        return;

	//查找当前番茄任务在列表中是否存在，存在则做一个标记
	for (i = 0; i < DIVOOM_TOMATO_TIME_NUM_MAX; i++)
	{
		tomato_time_item = divoom_queue_get_index_item(ptomato_time_handle->tomato_queue, i);

		if (tomato_time_item)
		{
			if (tomato_time_item->id == tomato_info->tomato_time_list->id)
			{
				tomato_time_item->exist_flag = 1; //标记存在

				if (tomato_time_item->update_flag == 1) //设备本地有更新
					update_flag = 1;
				break;
			}
		}
	}

	if (update_flag == 0)
	{
		tomato_info->tomato_time_list->exist_flag = 1;
	    divoom_tomato_time_update_one_tomato_info(tomato_info->tomato_time_list, 0);
		if (ptomato_time_handle->tomato_info.id == tomato_info->tomato_time_list->id || ptomato_time_handle->tomato_info.id == 0) //当前正在进行要改变
		{
			memcpy(&ptomato_time_handle->tomato_info, tomato_info->tomato_time_list, sizeof(divoom_tomato_time_info));
		}
	}
}

void divoom_tomato_time_update_tomato_list_end(void)
{
	int i = 0;
	Pdivoom_tomato_time_info tomato_time_item = NULL;
	divoom_tomato_time_focus_done_json focus_done;

	for (i = 0; i < DIVOOM_TOMATO_TIME_NUM_MAX; i++)
	{
		tomato_time_item = divoom_queue_get_index_item(ptomato_time_handle->tomato_queue, i);

		if (tomato_time_item)
		{
			if (tomato_time_item->exist_flag == 0)
			{
				//删除该id的专注记录
				divoom_tomato_time_delete_tomato_info(tomato_time_item->id);
				i--;
			}
		}
	}
#ifdef DIVOOM_TOMATO_SAVE_LOCAL_CONFIG
	divoom_tomato_time_save_focus_record();
	divoom_tomato_time_save_all_tomato_info_local();
#endif
}

void divoom_tomato_time_update_tomato_list(void *tomato_data)
{
	int i = 0, j = 0, update_flag = 0;
	Pdivoom_tomato_time_get_list_json tomato_list = (Pdivoom_tomato_time_get_list_json)tomato_data;

	if (tomato_list == NULL)
		return;

	for (i = 0; i < tomato_list->tomato_time_num; i++)
	{
		update_flag = 0;
		Pdivoom_tomato_time_info tomato_time_item = NULL;

		//查找当前番茄任务在列表中是否存在，存在则做一个标记
		for (j = 0; j < DIVOOM_TOMATO_TIME_NUM_MAX; j++)
		{
			tomato_time_item = divoom_queue_get_index_item(ptomato_time_handle->tomato_queue, j);

			if (tomato_time_item)
			{
				if (tomato_time_item->id == tomato_list->tomato_time_list[i].id)
				{
					tomato_time_item->exist_flag = 1; //标记存在

					if (tomato_time_item->update_flag == 1) //设备本地有更新
						update_flag = 1;
					break;
				}
			}
		}

		if (update_flag == 0)
		{
			tomato_list->tomato_time_list[i].exist_flag = 1;
		    divoom_tomato_time_update_one_tomato_info(&tomato_list->tomato_time_list[i], 0);
			if (ptomato_time_handle->tomato_info.id == tomato_list->tomato_time_list[i].id || ptomato_time_handle->tomato_info.id == 0) //当前正在进行要改变
			{
				memcpy(&ptomato_time_handle->tomato_info, &tomato_list->tomato_time_list[i], sizeof(divoom_tomato_time_info));
			}
		}
	}
}

void divoom_tomato_time_delete_focus_record(Pdivoom_tomato_time_focus_done_json done_info, int flag)
{
	int i = 0;
	Pdivoom_tomato_time_focus_action_json focus_info = NULL;

	if (flag) //删除这个id的所有记录
	{
		for (i = 0; i < DIVOOM_TOMATO_TIME_NUM_MAX; i++)
		{
			if (divoom_queue_items(ptomato_time_handle->tomato_focus_queue[i]))
			{
				focus_info = divoom_queue_get_index_item(ptomato_time_handle->tomato_focus_queue[i], 0);
				if (focus_info->tomato_id == done_info->tomato_id)
				{
					divoom_queue_pop_all(ptomato_time_handle->tomato_focus_queue[i], NULL);
					break;
				}
			}
		}
	}
	else //删除这个id的某个记录
	{
		for (i = 0; i < DIVOOM_TOMATO_TIME_NUM_MAX; i++)
		{
			if (divoom_queue_items(ptomato_time_handle->tomato_focus_queue[i]))
			{
				focus_info = divoom_queue_get_index_item(ptomato_time_handle->tomato_focus_queue[i], 0);
				if (focus_info->tomato_id == done_info->tomato_id)
				{
					focus_info = divoom_queue_pop_cmp(ptomato_time_handle->tomato_focus_queue[i], &done_info->start_time, divoom_tomato_time_find_focus_compare);
					if (focus_info)
						divoom_system_free(focus_info);
					break;
				}
			}
		}
	}
}

void divoom_tomato_time_update_focus_record(void)
{
	int i = 0, focus_num = 0;
	Pdivoom_tomato_time_focus_action_json focus_info = NULL;

	Pdivoom_tomato_time_focus_action_json tomato_focus_action = divoom_system_malloc(sizeof(divoom_tomato_time_focus_action_json));
	tomato_focus_action->tomato_id = ptomato_time_handle->tomato_info.id;
	tomato_focus_action->end_time = divoom_system_get_utc_time();
	tomato_focus_action->start_time = tomato_focus_action->end_time - ptomato_time_handle->tomato_info.work_time*DIVOOM_TOMATO_ONE_TIME;
	tomato_focus_action->focus_time = ptomato_time_handle->tomato_info.work_time;
	for (i = 0; i < DIVOOM_TOMATO_TIME_NUM_MAX; i++)
	{
		focus_num = divoom_queue_items(ptomato_time_handle->tomato_focus_queue[i]);
		if (focus_num)
		{
			if (focus_num >= DIVOOM_TOMATO_FOCUS_NUM_MAX) //达到了最大值删除最早的记录
			{
				focus_info = divoom_queue_pop(ptomato_time_handle->tomato_focus_queue[i]);
				divoom_system_free(focus_info);
			}
			focus_info = divoom_queue_get_index_item(ptomato_time_handle->tomato_focus_queue[i], 0);
			if (focus_info->tomato_id == tomato_focus_action->tomato_id)
			{
				divoom_queue_push(ptomato_time_handle->tomato_focus_queue[i], tomato_focus_action);
				break;
			}
		}
	}

	if (i == DIVOOM_TOMATO_TIME_NUM_MAX)
	{
		for (i = 0; i < DIVOOM_TOMATO_TIME_NUM_MAX; i++)
		{
			if (!divoom_queue_items(ptomato_time_handle->tomato_focus_queue[i]))
			{
				divoom_queue_push(ptomato_time_handle->tomato_focus_queue[i], tomato_focus_action);
				break;
			}
		}
	}
}

uint8 divoom_tomato_time_get_main_status(void)
{
	return ptomato_time_handle->tomato_status.main_status;
}

uint8 divoom_tomato_time_get_sub_status(void)
{
	return ptomato_time_handle->tomato_status.sub_status;
}

uint32 divoom_tomato_time_get_time(void)
{
    uint32 cur_time;

	if (ptomato_time_handle)
	{
        cur_time = divoom_system_get_ticks();
		//divoom_debug("%d, %d, %d", ptomato_time_handle->tomato_status.main_status, ptomato_time_handle->tomato_status.sub_status, ptomato_time_handle->tomato_reach_time);
		if (ptomato_time_handle->tomato_status.main_status == TOMATO_MAIN_FOCUS)
		{
			if (ptomato_time_handle->tomato_status.sub_status == TOMATO_START)//开始
			{
				if (ptomato_time_handle->frist_get_flag == 0)
				{
					ptomato_time_handle->frist_get_flag = 1;
					ptomato_time_handle->tomato_reach_time = cur_time + ptomato_time_handle->tomato_info.work_time * DIVOOM_TOMATO_ONE_TIME;
					ptomato_time_handle->tomato_disp_value = ptomato_time_handle->tomato_info.work_time * DIVOOM_TOMATO_ONE_TIME; //25
				}

		        if (ptomato_time_handle->tomato_reach_time > cur_time)
		        {
					ptomato_time_handle->tomato_disp_value = ptomato_time_handle->tomato_reach_time - cur_time;
					return ptomato_time_handle->tomato_disp_value;
		        }
			}
			else if (ptomato_time_handle->tomato_status.sub_status == TOMATO_PUASE)
			{
				ptomato_time_handle->tomato_disp_value = ptomato_time_handle->tomato_disp_value;
				return ptomato_time_handle->tomato_disp_value;
			}
			else if (ptomato_time_handle->tomato_status.sub_status == TOMATO_STOP)
			{
				ptomato_time_handle->tomato_disp_value = ptomato_time_handle->tomato_info.work_time * DIVOOM_TOMATO_ONE_TIME;
				return ptomato_time_handle->tomato_disp_value;
			}
		}
		else if (ptomato_time_handle->tomato_status.main_status == TOMATO_MAIN_RESET)//休息
		{
			if (ptomato_time_handle->tomato_status.sub_status == TOMATO_START)
			{
				if (ptomato_time_handle->frist_get_flag == 0)
				{
					ptomato_time_handle->frist_get_flag = 1;
				}

		        if (ptomato_time_handle->tomato_reach_time > cur_time)
		        {
					ptomato_time_handle->tomato_disp_value = ptomato_time_handle->tomato_reach_time - cur_time;
					return ptomato_time_handle->tomato_disp_value;
		        }
			}
			else if (ptomato_time_handle->tomato_status.sub_status == TOMATO_PUASE)
			{
				ptomato_time_handle->tomato_disp_value = ptomato_time_handle->tomato_disp_value;
				return ptomato_time_handle->tomato_disp_value;
			}
			else if (ptomato_time_handle->tomato_status.sub_status == TOMATO_STOP)
			{
				ptomato_time_handle->tomato_disp_value = ptomato_time_handle->tomato_info.work_time * DIVOOM_TOMATO_ONE_TIME;
				return ptomato_time_handle->tomato_disp_value;
			}
		}
		else //等待状态返回工作时长
		{
			ptomato_time_handle->tomato_disp_value = ptomato_time_handle->tomato_info.work_time * DIVOOM_TOMATO_ONE_TIME;
			return ptomato_time_handle->tomato_disp_value;
		}
	}
	return 0;
}

//保存现场：把剩余秒冻结进tomato_disp_value，并停掉可能正在响的提示音
static void divoom_tomato_time_session_freeze(void)
{
	divoom_tomato_time_get_time();
	ptomato_time_handle->voice_start = 0;
	ptomato_time_handle->next_over_time = 0;
	divoom_local_music_ctrl_stop_tone();
}

//恢复现场：按冻结的剩余秒重算到点时刻
static void divoom_tomato_time_session_thaw(void)
{
	uint32 remain = ptomato_time_handle->tomato_disp_value;

	if (remain == 0)
		remain = ptomato_time_handle->tomato_info.work_time * DIVOOM_TOMATO_ONE_TIME;

	ptomato_time_handle->tomato_reach_time = divoom_system_get_ticks() + remain;
	//必须置1，否则get_time会把reach_time重设成整段work_time
	ptomato_time_handle->frist_get_flag = 1;
	ptomato_time_handle->next_over_time = 0;
	ptomato_time_handle->voice_start = 0;
}

void divoom_tomato_time_set_tomato(Pdivoom_tomato_time_info info, int flag)
{
	if (flag == 0)//停止
	{
		//时间回到初始状态；update() 在 WAIT 后不再跑，须在此停到点提示音
		ptomato_time_handle->tomato_status.main_status = TOMATO_MAIN_WAIT;
		ptomato_time_handle->tomato_status.sub_status = TOMATO_STOP;
		ptomato_time_handle->tomato_resume_sub = TOMATO_STOP;
		ptomato_time_handle->voice_start = 0;
		ptomato_time_handle->next_over_time = 0;
		ptomato_time_handle->tomato_reach_time = divoom_system_get_ticks() + ptomato_time_handle->tomato_info.work_time * DIVOOM_TOMATO_ONE_TIME;
		divoom_local_music_ctrl_stop_tone();
	}
	else if (flag == 1) //启动一个新的番茄
	{
		//运行状态
		ptomato_time_handle->tomato_status.main_status = TOMATO_MAIN_FOCUS;
		ptomato_time_handle->tomato_status.sub_status = TOMATO_START;
		memcpy(&ptomato_time_handle->tomato_info, info, sizeof(divoom_tomato_time_info));
		ptomato_time_handle->tomato_cur_roll_times = 0;
		ptomato_time_handle->frist_get_flag = 0;
		ptomato_time_handle->tomato_resume_sub = TOMATO_STOP;
	}
	else if (flag == 2) //要么停止/要么启动
	{
		//在播放状态则停止，否则就播放
		ptomato_time_handle->tomato_status.main_status = TOMATO_MAIN_FOCUS;
		ptomato_time_handle->tomato_resume_sub = TOMATO_STOP;
		if (ptomato_time_handle->tomato_status.sub_status == TOMATO_START)
		{
			ptomato_time_handle->tomato_status.sub_status = TOMATO_STOP;
			ptomato_time_handle->tomato_reach_time = divoom_system_get_ticks() + ptomato_time_handle->tomato_info.work_time * DIVOOM_TOMATO_ONE_TIME;
			ptomato_time_handle->tomato_cur_roll_times = 0;
			divoom_local_music_ctrl_stop_tone();
//			divoom_tomato_time_menu_exit_disp(); //显示不退出
		}
		else
		{
			ptomato_time_handle->tomato_status.sub_status = TOMATO_START;
			ptomato_time_handle->tomato_reach_time = divoom_system_get_ticks() + ptomato_time_handle->tomato_info.work_time * DIVOOM_TOMATO_ONE_TIME;
			ptomato_time_handle->frist_get_flag = 0;
			ptomato_time_handle->tomato_cur_roll_times = 0;

			//lvgl启动显示申请优先级
			divoom_app_schedule_item_info event_info = {0};
			event_info.model_id = DIVOOM_MODEL_APP_TOMATO_TIME;
			event_info.event_id = DIVOOM_TOMATO_SCHEDULE_TOMATO_ACTIVE;
			event_info.attr = DIVOOM_APP_SCHEDULE_ATTR_WAIT;
			event_info.resource_type = DIVOOM_TOMATO_SCHEDULE_RESOURCE_TYPE;
			divoom_app_schedule_push(&event_info);
		}
	}
	else if (flag == 3) //切到下一个，暂停
	{
		//运行状态
		ptomato_time_handle->tomato_status.main_status = TOMATO_MAIN_FOCUS;
		ptomato_time_handle->tomato_status.sub_status = TOMATO_STOP;
		memcpy(&ptomato_time_handle->tomato_info, info, sizeof(divoom_tomato_time_info));
		ptomato_time_handle->tomato_cur_roll_times = 0;
		ptomato_time_handle->frist_get_flag = 0;
		ptomato_time_handle->tomato_resume_sub = TOMATO_STOP;
	}
	else if (flag == 4) //启动当前时钟
	{
		divoom_app_schedule_item_info event_info = {0};
		uint32 schedule_ret = 0;
		int on_pomodoro_menu;

		ptomato_time_handle->tomato_status.main_status = TOMATO_MAIN_FOCUS;
		on_pomodoro_menu = (divoom_menu_control_get_active_menu_id() == DIVOOM_MENU_CONTROL_ID_POMODORO_MENU);

		if (ptomato_time_handle->tomato_status.sub_status == TOMATO_START)
		{
			if (on_pomodoro_menu)
				divoom_astrootoo_tools_pomo_request_sync_active_page();
			return;
		}

		ptomato_time_handle->tomato_status.sub_status = TOMATO_START;
		ptomato_time_handle->tomato_reach_time = divoom_system_get_ticks() + ptomato_time_handle->tomato_info.work_time * DIVOOM_TOMATO_ONE_TIME;
		ptomato_time_handle->frist_get_flag = 0;
		ptomato_time_handle->tomato_cur_roll_times = 0;
		ptomato_time_handle->tomato_resume_sub = TOMATO_STOP;

		event_info.model_id = DIVOOM_MODEL_APP_TOMATO_TIME;
		event_info.event_id = DIVOOM_TOMATO_SCHEDULE_TOMATO_ACTIVE;
		event_info.attr = DIVOOM_APP_SCHEDULE_ATTR_WAIT;
		event_info.resource_type = DIVOOM_TOMATO_SCHEDULE_RESOURCE_TYPE;
		schedule_ret = divoom_app_schedule_push(&event_info);

		if (!on_pomodoro_menu && schedule_ret)
		{
			divoom_menu_control_clearall_load(DIVOOM_MENU_CONTROL_ID_POMODORO_MENU, NULL, DIVOOM_MODEL_APP_TOMATO_TIME, DIVOOM_TOMATO_SCHEDULE_TOMATO_ACTIVE);
		}
		else if (on_pomodoro_menu)
		{
			divoom_astrootoo_tools_pomo_request_sync_active_page();
		}
	}
	else if (flag == 5) //启动当前时钟
	{
		ptomato_time_handle->tomato_status.main_status = TOMATO_MAIN_FOCUS;

		if (ptomato_time_handle->tomato_status.sub_status == TOMATO_START)
			return;
		else
		{
			ptomato_time_handle->tomato_status.sub_status = TOMATO_START;
			ptomato_time_handle->tomato_reach_time = divoom_system_get_ticks() + ptomato_time_handle->tomato_info.work_time * DIVOOM_TOMATO_ONE_TIME;
			ptomato_time_handle->frist_get_flag = 0;
			ptomato_time_handle->tomato_cur_roll_times = 0;
			ptomato_time_handle->tomato_resume_sub = TOMATO_STOP;

//			if (divoom_menu_control_get_active_menu_id() != DIVOOM_MENU_CONTROL_ID_POMODORO_MENU)
			{
				//lvgl启动显示申请优先级
				divoom_app_schedule_item_info event_info = {0};
				event_info.model_id = DIVOOM_MODEL_APP_TOMATO_TIME;
				event_info.event_id = DIVOOM_TOMATO_SCHEDULE_TOMATO_ACTIVE;
				event_info.attr = DIVOOM_APP_SCHEDULE_ATTR_WAIT;
				event_info.resource_type = DIVOOM_TOMATO_SCHEDULE_RESOURCE_TYPE;
				if (divoom_app_schedule_push(&event_info)) //只申请优先级
				{
				}
			}
		}
	}
	else if (flag == 6) //被打断恢复当前番茄时钟
	{
		//保留main_status与tomato_cur_roll_times：休息段被打断回来仍是休息段，轮次不清零
		if (ptomato_time_handle->tomato_status.main_status == TOMATO_MAIN_WAIT)
			return; //已彻底停止，没有会话可恢复
		if (ptomato_time_handle->tomato_status.sub_status != TOMATO_STOP)
			return;

		if (ptomato_time_handle->tomato_resume_sub == TOMATO_PUASE)
		{
			//打断前就是暂停，回来仍停在暂停，等用户按继续
			ptomato_time_handle->tomato_status.sub_status = TOMATO_PUASE;
		}
		else
		{
			divoom_tomato_time_session_thaw();
			ptomato_time_handle->tomato_status.sub_status = TOMATO_START;
		}
		ptomato_time_handle->tomato_resume_sub = TOMATO_STOP;

		if (divoom_menu_control_get_active_menu_id() != DIVOOM_MENU_CONTROL_ID_POMODORO_MENU)
		{
			//启动lvgl番茄时钟显示
			divoom_menu_control_clearall_load(DIVOOM_MENU_CONTROL_ID_POMODORO_MENU, NULL, DIVOOM_MODEL_APP_TOMATO_TIME, DIVOOM_TOMATO_SCHEDULE_TOMATO_ACTIVE);
		}
	}
	else if (flag == 7) //被优先级高的打断
	{
		//记住打断前的子状态，恢复时据此还原；剩余时间冻结在tomato_disp_value
		if (ptomato_time_handle->tomato_status.sub_status == TOMATO_START)
		{
			divoom_tomato_time_session_freeze();
			ptomato_time_handle->tomato_resume_sub = TOMATO_START;
			ptomato_time_handle->tomato_status.sub_status = TOMATO_STOP;
		}
		else if (ptomato_time_handle->tomato_status.sub_status == TOMATO_PUASE)
		{
			ptomato_time_handle->tomato_resume_sub = TOMATO_PUASE;
			ptomato_time_handle->tomato_status.sub_status = TOMATO_STOP;
		}
	}
	else if (flag == 8) //UI暂停
	{
		if (ptomato_time_handle->tomato_status.sub_status != TOMATO_START)
			return;

		divoom_tomato_time_session_freeze();
		ptomato_time_handle->tomato_status.sub_status = TOMATO_PUASE;
	}
	else if (flag == 9) //UI继续
	{
		if (ptomato_time_handle->tomato_status.sub_status != TOMATO_PUASE)
			return;

		divoom_tomato_time_session_thaw();
		ptomato_time_handle->tomato_status.sub_status = TOMATO_START;
	}
}

void divoom_tomato_time_start_tomato_info(int id)
{
	Pdivoom_tomato_time_info data_item;

    data_item = divoom_queue_get_cmp(ptomato_time_handle->tomato_queue, &id, divoom_tomato_time_find_tomato_compare);
    if(data_item)
    {
    	//开启这个番茄时钟
		divoom_tomato_time_set_tomato(data_item, 3);
    }
}

//切到下一个番茄
void divoom_tomato_time_change_tomato_info(int id)
{
	Pdivoom_tomato_time_info data_item;

    data_item = divoom_queue_get_cmp(ptomato_time_handle->tomato_queue, &id, divoom_tomato_time_find_tomato_compare);
    if(data_item)
    {
    	//开启这个番茄时钟
		divoom_tomato_time_set_tomato(data_item, 3);
    }
	if (id == 0)
	{
		divoom_tomato_time_set_tomato(&ptomato_time_handle->default_tomato_info, 3);
	}
}

void divoom_tomato_time_delete_tomato_all(void)
{
	Pdivoom_tomato_time_info data_item, data_item2;
    divoom_tomato_time_focus_done_json done_info;

	while (divoom_queue_not_empty(ptomato_time_handle->tomato_queue))
	{
	    data_item = divoom_queue_pop(ptomato_time_handle->tomato_queue);
	    if (data_item)
	    {
			divoom_debug("id %d, name: %s", data_item->id, data_item->name);
	    	//删除这个id的所有记录
	    	done_info.tomato_id = data_item->id;
			divoom_tomato_time_delete_focus_record(&done_info, 1);
			ptomato_time_handle->update_tomato_flag = 1;
			if (data_item->id == ptomato_time_handle->tomato_info.id) //当前正在执行任务
			{
				ptomato_time_handle->tomato_status.main_status = TOMATO_MAIN_WAIT;
				//停止当前番茄时钟
				divoom_tomato_time_set_tomato(NULL, 0);
				//切到下一个番茄时钟
				data_item2 = divoom_tomato_time_menu_get_tomato_info(0);
				if (data_item2)
					divoom_tomato_time_set_tomato(data_item2, 3);
			}
	    	divoom_system_free(data_item);
	    }
	}
}

void divoom_tomato_time_delete_tomato_info(int id)
{
    Pdivoom_tomato_time_info data_item;
    divoom_tomato_time_focus_done_json done_info;

    data_item = divoom_queue_pop_cmp(ptomato_time_handle->tomato_queue, &id, divoom_tomato_time_find_tomato_compare);
    if (data_item)
    {
		divoom_debug("id %d, name: %s", data_item->id, data_item->name);
    	//删除这个id的所有记录
    	done_info.tomato_id = id;
		divoom_tomato_time_delete_focus_record(&done_info, 1);
    	divoom_system_free(data_item);
		ptomato_time_handle->update_tomato_flag = 1;

		if (id == ptomato_time_handle->tomato_info.id) //当前正在执行任务
		{
			ptomato_time_handle->tomato_status.main_status = TOMATO_MAIN_WAIT;
			//停止当前番茄时钟
			divoom_tomato_time_set_tomato(NULL, 0);
			//切到下一个番茄时钟
			data_item = divoom_tomato_time_menu_get_tomato_info(0);
			if (data_item)
				divoom_tomato_time_set_tomato(data_item, 3);
			divoom_tomato_time_cancel();
		}
    }
}

int divoom_tomato_time_menu_update(uint32 tomato_id)
{
    Pdivoom_tomato_time_info data_item;
	int ret = 0;

    data_item = divoom_queue_get_cmp(ptomato_time_handle->tomato_queue, &tomato_id, divoom_tomato_time_find_tomato_compare);

	if (data_item && tomato_id != 0)
	{
		if (data_item->update_flag == 1)
		{
			ret = 1;
		}
        if(divoom_net_mqtt_get_net_status())
		{
			if (data_item->update_flag == 1)
			{
				data_item->update_flag = 0;
            	divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_TOMATO_TIME_DEV_SET_ALARM, DIVOOM_SERVER_TOPIC, data_item);
			}
		}
	}
	else if (tomato_id == 0)
	{
		if (ptomato_time_handle->default_tomato_info.update_flag == 1)
		{
			ret = 1;
		}
	}
	return ret;
}

void divoom_tomato_time_dev_update_all(void) //本地有更新上传到app,一次传一个
{
	int i = 0, num = 0;
	Pdivoom_tomato_time_info tomato_time_item = NULL;

	if(!divoom_net_mqtt_get_net_status() || !ptomato_time_handle)
		return;

	for (i = 0; i < DIVOOM_TOMATO_TIME_NUM_MAX; i++)
	{
		tomato_time_item = divoom_queue_get_index_item(ptomato_time_handle->tomato_queue, i);

		if (tomato_time_item && tomato_time_item->update_flag == 2)
		{
			tomato_time_item->update_flag = 0;
			divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_TOMATO_TIME_DEV_SET_ALARM, DIVOOM_SERVER_TOPIC, tomato_time_item);
			return;
		}
	}
}

//当模块在执行，被切换到后面时，将会调用这个函数，如果返回非0,将直接从队列中删除
uint8 divoom_tomato_time_schedule_pause_callback(uint8 event_id, void *user_data, uint8 active_model, uint8 active_event, uint32 resouce_type)
{
	divoom_tomato_time_set_tomato(NULL, 7);
    return 1;
}
//当模块从后台被激活，将会调用这个函数
uint8 divoom_tomato_time_schedule_play_callback(uint8 event_id, void *user_data, uint32 resouce_type)
{
	divoom_tomato_time_set_tomato(NULL, 6);
    return 1;
}
//当模块达到条件，将要卸载时，将会调用这个函数
uint8 divoom_tomato_time_schedule_destory_callback(uint8 event_id, void *user_data, uint32 status, uint32 resouce_type)
{
	//试听事件走同一个回调，不判event_id会把正在跑的番茄一起停掉
	if (event_id == DIVOOM_TOMATO_SCHEDULE_TOMATO_ACTIVE)
	{
		//资源被彻底放弃，清掉可能残留的PUASE，避免无人推进也无人收尾
		divoom_tomato_time_set_tomato(NULL, 0);
	}
    return 1;
}

//不模块处理激活时，系统有按键过来，将调用这个函数，如果返回０，这个按键信息将传递到系统，否则将不需要
uint8 divoom_tomato_time_schedule_key_callback(uint8 key_value, uint8 key_status, uint32 resouce_type)
{
	uint16 event_id = 0, ret = 0;
//	uint8 model_id = divoom_app_schedule_get_active_model((uint16 *)&event_id);
#if 0
	if(DIVOOM_MODEL_APP_TOMATO_TIME == model_id)
	{
		uint8 key_set_flag = 0xff;
		switch(key_value)
		{
			case DIVOOM_KEY_OK:
				if(key_status == DIVOOM_KEY_PRESS_SHORT)
				{
					//开启番茄时间
					divoom_tomato_time_set_tomato(NULL , 2);
					ret = 1;
				}
				break;

			case DIVOOM_KEY_NEXT:
			case DIVOOM_KEY_RIGHT:
				divoom_tomato_time_get_next_tomato();
				ret = 1;
				break;

			case DIVOOM_KEY_PRE:
			case DIVOOM_KEY_LEFT:
				divoom_tomato_time_get_pre_tomato();
				ret = 1;
				break;

			default:
				break;
		}
	}
#endif
    return ret;
}

uint8 divoom_tomato_time_event(Pdivoom_sys_msg_info msg)
{
	divoom_app_schedule_item_info event_info = {0};
	divoom_debug("event:%d\n", msg->msg_id);
	if(msg)
	{
		switch(msg->msg_id)
		{
			case DIVOOM_APP_TOMATO_GET:
				divoom_tomato_time_update_tomato_list(msg->data);
				divoom_tomato_time_update_tomato_list_end();
				break;

			case DIVOOM_APP_TOMATO_SET: //app设置番茄时间
				divoom_tomato_time_update_tomato_info(msg->data, 0);
				break;

			case DIVOOM_APP_TOMATO_SEND_LIST_COUNT:
				//得到总数
				ptomato_time_handle->tomato_total_num = *(int *)msg->data;

				if (ptomato_time_handle->tomato_total_num == 0)
				{
					divoom_tomato_time_delete_tomato_all();
					break;
				}
				ptomato_time_handle->tomato_cur_num = 0;
				//把番茄列表里面所有的任务标记置0
				divoom_tomato_time_update_tomato_list_init();
				break;

			case DIVOOM_APP_TOMATO_SEND_ONE_BY_ONE:
				ptomato_time_handle->tomato_cur_num++;
				divoom_tomato_time_update_tomato_list_one_by_one(msg->data);
				if (ptomato_time_handle->tomato_cur_num >= ptomato_time_handle->tomato_total_num)
				{
					//把标记不存在的都删除
					divoom_tomato_time_update_tomato_list_end();
				}
				break;

			case DIVOOM_APP_TOMATO_START:
				{
					uint8 event_id = 0, mode_id = 0;
					mode_id = divoom_app_schedule_get_active_model(&event_id, DIVOOM_APP_SCHEDULE_RESOURCCE_RGB);
					if ((mode_id == DIVOOM_MODEL_ALARM && event_id == DIVOOM_ALARM_APP_EVENT_ALARM) || (mode_id == DIVOOM_MODEL_MAIN && event_id == DIVOOM_MAIN_PLAY_ENTER_POWER_OFF_DISP_MENU_EVENT))
						break;
					divoom_tomato_time_start_tomato_info(*(int *)msg->data);
					if (divoom_tomato_time_start_notice_gif() == 0)
						tomato_gif_start_pending = 1;
				}
				break;

			case DIVOOM_APP_TOMATO_FOCUS_DONE://app已收到某条记录
				divoom_tomato_time_delete_focus_record((Pdivoom_tomato_time_focus_done_json)msg->data, 0);
				#ifdef DIVOOM_TOMATO_SAVE_LOCAL_CONFIG
				divoom_tomato_time_save_focus_record();//更新记录
				#endif
				break;

			case DIVOOM_APP_TOMATO_DELETE:
				divoom_tomato_time_delete_tomato_info(*(int *)msg->data);
                divoom_main_save_notice_dis();
				#ifdef DIVOOM_TOMATO_SAVE_LOCAL_CONFIG
				divoom_tomato_time_save_focus_record();
				divoom_tomato_time_save_all_tomato_info_local();
				#endif
				break;

			case DIVOOM_APP_TOMATO_LISTEN:
			{
				Pdivoom_tomato_time_listen_info_json info = (Pdivoom_tomato_time_listen_info_json)msg->data;
				divoom_tomato_time_try_listen(info->sound_type ,info->volume, info->start_flag, 0);
			}
				break;

			case DIVOOM_APP_TOMATO_LISTEN_VOLUME:
				divoom_tomato_time_set_vol(*(int *)msg->data);
				break;

			case DIVOOM_APP_TOMATO_EXIT_DISP_EVENT:
				divoom_tomato_time_menu_control(4);
				break;

			case DIVOOM_APP_TOMATO_EXIT_DISP:
				divoom_tomato_time_cancel();
				break;

			default:
				divoom_print("No This Command In %s--%d, command = %d\n", __func__, __LINE__, msg->msg_id);
		}

		return 1;
	}

	return 0;
}

uint8 divoom_tomato_time_check_save_tomato_info(void)
{
	FILE *fd = NULL;
	divoom_tomato_time_info data_read, *cur_item;
    int i, number, read_bytes, ret;

	if((fd = divoom_port_file_open(DIVOOM_TOMATO_INFO_SAVE_PATH, "r")) == NULL)
	{
		divoom_print("Some Wrong in %d: Maybe no this file:%s!\n", __LINE__, DIVOOM_TOMATO_INFO_SAVE_PATH);
		return true;
	}

    ret = divoom_port_file_read(&number, sizeof(number), 1, fd);
    if(number != divoom_queue_items(ptomato_time_handle->tomato_queue))
    {
		divoom_print("tomato number is different!\n");
		goto divoom_tomato_check_save_tomato_info_err_table;
    }

    for(i = 0; i < number; i++)
    {
        read_bytes = divoom_port_file_read(&data_read, sizeof(divoom_tomato_time_info), 1, fd);
        if(read_bytes == 0)
        {
    		divoom_print("read tomato is different!\n");
		    goto divoom_tomato_check_save_tomato_info_err_table;
        }
        cur_item = divoom_queue_get_index_item(ptomato_time_handle->tomato_queue, i);
        if(cur_item == NULL ||
            memcmp(cur_item, &data_read, sizeof(divoom_tomato_time_info)))
        {
    		divoom_print("tomato  data is different!\n");
		    goto divoom_tomato_check_save_tomato_info_err_table;
        }
    }

	divoom_port_file_close(fd);
    return false;

    divoom_tomato_check_save_tomato_info_err_table:
    divoom_port_file_close(fd);
    return true;
}

void divoom_tomato_time_get_todoy_tomato_total(uint8 flag)
{
	divoom_today_tomato_info today_tomato_info = {0};

	if (flag == 0)
	{
		divoom_database_manage_get_record(DIVOOM_TOMATO_TODAY_TOTAL_TABLE, NULL, 1, divoom_tomato_time_db_get_data_callback, &today_tomato_info);
		ptomato_time_handle->tomato_total_times = today_tomato_info.total_times;
	}
	else
	{
		struct tm *time1; 
		char item_data[128] = {0};

		time_t rawtime; 
		time(&rawtime); 
		time1=localtime(&rawtime); 
		time1->tm_year += 1900;

		divoom_database_manage_get_record(DIVOOM_TOMATO_TODAY_TOTAL_TABLE, NULL, 1, divoom_tomato_time_db_get_data_callback, &today_tomato_info);
		if (time1->tm_year != today_tomato_info.year || time1->tm_mon != today_tomato_info.mon || time1->tm_mday != today_tomato_info.day)
			ptomato_time_handle->tomato_total_times = 0;

		sprintf(item_data, "total_times = %d, year = %d, mon = %d, day = %d WHERE id = 1", ptomato_time_handle->tomato_total_times, time1->tm_year, time1->tm_mon, time1->tm_mday);
		divoom_database_manage_update_record(DIVOOM_TOMATO_TODAY_TOTAL_TABLE, item_data);
	}
}

void divoom_tomato_time_save_todoy_tomato_total(int flag)
{
	divoom_today_tomato_info today_tomato_info = {0};
	struct tm *time1; 
	char item_data[128] = {0};

	time_t rawtime; 
	time(&rawtime); 
	time1=localtime(&rawtime); 
	time1->tm_year += 1900;
	

	divoom_database_manage_get_record(DIVOOM_TOMATO_TODAY_TOTAL_TABLE, NULL, 1, divoom_tomato_time_db_get_data_callback, &today_tomato_info);
	if (time1->tm_year != today_tomato_info.year || time1->tm_mon != today_tomato_info.mon || time1->tm_mday != today_tomato_info.day)
	{
		if (flag == 0)
			ptomato_time_handle->tomato_total_times = 0;
		else
			ptomato_time_handle->tomato_total_times = 1;
	}

	sprintf(item_data, "total_times=%d, year=%d, mon=%d, day=%d WHERE id=1", ptomato_time_handle->tomato_total_times, time1->tm_year, time1->tm_mon, time1->tm_mday);
	divoom_database_manage_update_record(DIVOOM_TOMATO_TODAY_TOTAL_TABLE, item_data);
}

void divoom_tomato_time_save_all_tomato_info_local(void)
{
    int i, number;
	FILE *fd = NULL;
	Pdivoom_tomato_time_info data_read;
	divoom_cJSON *root, *tomato_list, *tomato_item;

    divoom_print("save config!\n");
    number = divoom_queue_items(ptomato_time_handle->tomato_queue);
	if((fd = divoom_port_file_open(DIVOOM_TOMATO_INFO_SAVE_PATH, "w+")) == NULL)
	{
		divoom_print("Some Wrong in %d, maybe no this file:%s\n", __LINE__, DIVOOM_TOMATO_INFO_SAVE_PATH);
		return;
	}
	divoom_debug("number %d", number);

	root = divoom_cJSON_CreateObject();
	tomato_list = divoom_cJSON_CreateArray();
	divoom_cJSON_AddItemToObject(root, "TomatoList", tomato_list);

	for (i = 0; i < number; i++)
	{
		data_read = divoom_queue_get_index_item(ptomato_time_handle->tomato_queue, i);
		if (data_read)
		{
			tomato_item = divoom_cJSON_CreateObject();
			divoom_cJSON_AddItemToArray(tomato_list, tomato_item);
			divoom_cJSON_AddNumberToObject(tomato_item, "TomatoId", data_read->id);
			divoom_cJSON_AddStringToObject(tomato_item, "TomatoName", data_read->name);
			divoom_cJSON_AddStringToObject(tomato_item, "ImageFileId", data_read->image_file_id);
			divoom_cJSON_AddNumberToObject(tomato_item, "WorkTime", data_read->work_time);
			divoom_cJSON_AddNumberToObject(tomato_item, "ShortRestTime", data_read->short_rest_time);
			divoom_cJSON_AddNumberToObject(tomato_item, "LongRestTime", data_read->long_rest_time);
			divoom_cJSON_AddNumberToObject(tomato_item, "TimeCyclerType", data_read->roll_times);
			divoom_cJSON_AddNumberToObject(tomato_item, "SoundOnOff", data_read->sound_on_off);
			divoom_cJSON_AddNumberToObject(tomato_item, "SoundVolume", data_read->sound_volume);
			divoom_cJSON_AddNumberToObject(tomato_item, "SoundType", data_read->sound_type);
			divoom_cJSON_AddNumberToObject(tomato_item, "SoundSystem", data_read->sound_system_index);

			if (data_read->update_flag == 1)
				data_read->update_flag = 2;
			divoom_cJSON_AddNumberToObject(tomato_item, "UpdateFlag", data_read->update_flag);
		}
	}

	char *string = divoom_cJSON_Print(root);
	divoom_cJSON_Delete(root);

	if (string)
	{
		divoom_port_file_write(string, strlen(string), 1, fd);
		divoom_system_free(string);
	}
	divoom_port_file_close(fd);
}

void divoom_tomato_time_save_focus_record(void)
{
	int i, j, number;
	FILE *fd;
	Pdivoom_tomato_time_focus_action_json focus_action = NULL;
	divoom_cJSON *root, *tomato_list, *tomato_item, *tomato_focus_list, *tomato_focus_item;

	if((fd = divoom_port_file_open(DIVOOM_TOMATO_FOCUS_RECORD_SAVE_PATH, "w+")) == NULL)
	{
		divoom_print("Some Wrong in %d, maybe no this file:%s\n", __LINE__, DIVOOM_TOMATO_INFO_SAVE_PATH);
		return;
	}

	root = divoom_cJSON_CreateObject();
	tomato_list = divoom_cJSON_CreateArray();
	divoom_cJSON_AddItemToObject(root, "TomatoList", tomato_list);

	for (i = 0; i < DIVOOM_TOMATO_TIME_NUM_MAX; i++)
	{
		number = divoom_queue_items(ptomato_time_handle->tomato_focus_queue[i]);
		if (number)
		{
			tomato_focus_list = divoom_cJSON_CreateArray();
			divoom_cJSON_AddItemToArray(tomato_list, tomato_focus_list);
			for (j = 0; j < number; j++)
			{
				focus_action = divoom_queue_get_index_item(ptomato_time_handle->tomato_focus_queue[i], j);
				tomato_focus_item = divoom_cJSON_CreateObject();
				divoom_cJSON_AddItemToArray(tomato_focus_list, tomato_focus_item);
				divoom_cJSON_AddNumberToObject(tomato_focus_item, "TomatoId", focus_action->tomato_id);
				divoom_cJSON_AddNumberToObject(tomato_focus_item, "StartTime", focus_action->start_time);
				divoom_cJSON_AddNumberToObject(tomato_focus_item, "EndTime", focus_action->end_time);
				divoom_cJSON_AddNumberToObject(tomato_focus_item, "FocusTime", focus_action->focus_time);
			}
		}
	}

	char *string = divoom_cJSON_Print(root);
	divoom_cJSON_Delete(root);

	if (string)
	{
		divoom_port_file_write(string, strlen(string), 1, fd);
		divoom_system_free(string);
	}
	divoom_port_file_close(fd);
}

int divoom_tomato_time_get_cur_do_tomato_num(void)
{
	if (ptomato_time_handle == NULL)
		return 0;

	return ptomato_time_handle->tomato_total_times;
}

int divoom_tomato_time_get_tomato_offset(void)
{
	if (ptomato_time_handle == NULL)
		return 0;

	return ptomato_time_handle->tomato_cur_roll_times;
}

int divoom_tomato_time_get_tomato_num(void)
{
	if (ptomato_time_handle == NULL)
		return 0;

	return divoom_queue_items(ptomato_time_handle->tomato_queue);
}

int divoom_tomato_time_get_cur_tomato_index(void)
{
	int i = 0, num = 0;
	Pdivoom_tomato_time_info tomato_info = NULL;

	//获取当前的番茄时钟
	if (ptomato_time_handle == NULL)
		return 0;

	//查找当前是第几个番茄时钟
	num = divoom_queue_items(ptomato_time_handle->tomato_queue);
	for (i = 0; i < num; i++)
	{
		tomato_info = divoom_queue_get_index_item(ptomato_time_handle->tomato_queue, i);
		if (tomato_info->id == ptomato_time_handle->tomato_info.id)
			break;
	}

	if (i == num)
		return 0xff;

	return i;
}

Pdivoom_tomato_time_info divoom_tomato_time_get_one_tomato(int index)
{
	if (ptomato_time_handle == NULL)
		return NULL;

	if (index >= divoom_queue_items(ptomato_time_handle->tomato_queue))
		index = 0;

	return divoom_queue_get_index_item(ptomato_time_handle->tomato_queue, index);
}

Pdivoom_tomato_time_info divoom_tomato_time_get_pre_tomato(void)
{
	int index = 0;
	int total_tomato = divoom_queue_items(ptomato_time_handle->tomato_queue);

	ptomato_time_handle->tomato_index--;
	if (ptomato_time_handle->tomato_index < 0)
		ptomato_time_handle->tomato_index = total_tomato;

	index = ptomato_time_handle->tomato_index;
	Pdivoom_tomato_time_info tomato_time = divoom_queue_get_index_item(ptomato_time_handle->tomato_queue, index);
	memcpy(&ptomato_time_handle->tomato_info, tomato_time, sizeof(divoom_tomato_time_info));
	return tomato_time;
}

Pdivoom_tomato_time_info divoom_tomato_time_get_next_tomato(void)
{
	int index = 0;
	int total_tomato = divoom_queue_items(ptomato_time_handle->tomato_queue);

	ptomato_time_handle->tomato_index++;
	if (ptomato_time_handle->tomato_index >= total_tomato)
		ptomato_time_handle->tomato_index = 0;

	index = ptomato_time_handle->tomato_index;
	Pdivoom_tomato_time_info tomato_time = divoom_queue_get_index_item(ptomato_time_handle->tomato_queue, index);
	memcpy(&ptomato_time_handle->tomato_info, tomato_time, sizeof(divoom_tomato_time_info));
	return tomato_time;
}

void divoom_tomato_time_set_default_tomato(void)
{
	ptomato_time_handle->default_tomato_info.work_time = 15;
	strcpy(ptomato_time_handle->default_tomato_info.name, "Default");
	ptomato_time_handle->default_tomato_info.roll_times = 4;
	ptomato_time_handle->default_tomato_info.short_rest_time = 5;
	ptomato_time_handle->default_tomato_info.long_rest_time = 10;
	ptomato_time_handle->default_tomato_info.id = 0;
	ptomato_time_handle->default_tomato_info.sound_on_off = 1;
	ptomato_time_handle->default_tomato_info.sound_volume = 50;
	ptomato_time_handle->default_tomato_info.sound_type = 0;
	ptomato_time_handle->default_tomato_info.sound_system_index = 0;
}

Pdivoom_tomato_time_info divoom_tomato_time_get_default_tomato(void)
{
	return &ptomato_time_handle->default_tomato_info;
}

void divoom_tomato_time_load_tomato_data(void)
{
	int i, tomato_num, file_len = 0, ret = 0;
	divoom_cJSON *root, *tomato_list, *tomato_item;
	FILE *fd = NULL;
	uint8 *read_buf = NULL;
	Pdivoom_tomato_time_info tomato_info;

	if ((fd = divoom_port_file_open(DIVOOM_TOMATO_INFO_SAVE_PATH, "r")) == NULL)
	{
		divoom_print("Some Wrong in %d: Maybe no this file:%s!\n", __LINE__, DIVOOM_TOMATO_INFO_SAVE_PATH);
		return ;
	}

	divoom_port_file_seek(fd, 0, SEEK_END);
	file_len = divoom_port_file_tell(fd);
	divoom_port_file_seek(fd, 0, SEEK_SET);
	read_buf = divoom_system_malloc(file_len + 1);
	ret = divoom_port_file_read(read_buf, file_len, 1, fd);
	root = divoom_cJSON_Parse(read_buf);
	divoom_system_free(read_buf);

	if (root == NULL)
	{
		divoom_port_file_close(fd);
		return ;
	}

	tomato_list = divoom_cJSON_GetObjectItem(root, "TomatoList");
	if(tomato_list == NULL)
	{
		divoom_port_file_close(fd);
		divoom_cJSON_Delete(root);
		return ;
	}
	else
	{
		tomato_num = divoom_cJSON_GetArraySize(tomato_list);
		if(tomato_num == 0)
		{
			divoom_port_file_close(fd);
			divoom_cJSON_Delete(root);
			return ;
		}
	}

	for (i = 0; i < tomato_num; i++)
	{
		tomato_item = divoom_cJSON_GetArrayItem(tomato_list, i);
		if (tomato_item == NULL)
			break;

		tomato_info = divoom_system_malloc(sizeof(divoom_tomato_time_info));
		tomato_info->id = divoom_cJSON_GetObjectInt(tomato_item, "TomatoId");
		divoom_cJSON_GetObjectString(tomato_item, "TomatoName", tomato_info->name);
		divoom_cJSON_GetObjectString(tomato_item, "ImageFileId", tomato_info->image_file_id);
		tomato_info->work_time = divoom_cJSON_GetObjectInt(tomato_item, "WorkTime");
		tomato_info->short_rest_time = divoom_cJSON_GetObjectInt(tomato_item, "ShortRestTime");
		tomato_info->long_rest_time = divoom_cJSON_GetObjectInt(tomato_item, "LongRestTime");
		tomato_info->roll_times = divoom_cJSON_GetObjectInt(tomato_item, "TimeCyclerType");
		tomato_info->sound_on_off = divoom_cJSON_GetObjectInt(tomato_item, "SoundOnOff");
		tomato_info->sound_volume = divoom_cJSON_GetObjectInt(tomato_item, "SoundVolume");
		tomato_info->sound_type = divoom_cJSON_GetObjectInt(tomato_item, "SoundType");
		tomato_info->sound_system_index = divoom_cJSON_GetObjectInt(tomato_item, "SoundSystem");
		tomato_info->update_flag = divoom_cJSON_GetObjectInt(tomato_item, "UpdateFlag");
        divoom_queue_push(ptomato_time_handle->tomato_queue, tomato_info);
	}
	divoom_port_file_close(fd);
	divoom_cJSON_Delete(root);
}

void divoom_tomato_time_load_focus_record(void)
{
	int i, j, tomato_num, tomato_focus_num, file_len = 0, ret = 0;
	divoom_cJSON *root, *tomato_list, *tomato_item, *tomato_focus_list, *tomato_focus_item;
	Pdivoom_tomato_time_get_list_json tomato_info;
	Pdivoom_tomato_time_focus_action_json tomato_focus_action;
	FILE *fd = NULL;
	uint8 *read_buf = NULL;

	if ((fd = divoom_port_file_open(DIVOOM_TOMATO_FOCUS_RECORD_SAVE_PATH, "r")) == NULL)
	{
		divoom_print("Some Wrong in %d: Maybe no this file:%s!\n", __LINE__, DIVOOM_TOMATO_FOCUS_RECORD_SAVE_PATH);
		return ;
	}

	divoom_port_file_seek(fd, 0, SEEK_END);
	file_len = divoom_port_file_tell(fd);
	divoom_port_file_seek(fd, 0, SEEK_SET);
	read_buf = divoom_system_malloc(file_len + 1);
	ret = divoom_port_file_read(read_buf, file_len, 1, fd);
	root = divoom_cJSON_Parse(read_buf);
	divoom_system_free(read_buf);

	if (root == NULL)
	{
		divoom_port_file_close(fd);
		return ;
	}

	tomato_list = divoom_cJSON_GetObjectItem(root, "TomatoList");
	if(tomato_list == NULL)
	{
		divoom_port_file_close(fd);
		divoom_cJSON_Delete(root);
		return ;
	}
	else
	{
		tomato_num = divoom_cJSON_GetArraySize(tomato_list);
		if(tomato_num == 0)
		{
			divoom_port_file_close(fd);
			divoom_cJSON_Delete(root);
			return ;
		}
	}

	for (i = 0; i < tomato_num; i++)
	{
		tomato_focus_list = divoom_cJSON_GetArrayItem(tomato_list, i);
		if (tomato_focus_list == NULL)
			break;

		tomato_focus_num = divoom_cJSON_GetArraySize(tomato_focus_list);
		for (j = 0; j < tomato_focus_num; j++)
		{
			tomato_focus_item = divoom_cJSON_GetArrayItem(tomato_focus_list, j);
			tomato_focus_action = divoom_system_malloc(sizeof(divoom_tomato_time_focus_action_json));
			tomato_focus_action->tomato_id = divoom_cJSON_GetObjectInt(tomato_focus_item, "TomatoId");
			tomato_focus_action->start_time = divoom_cJSON_GetObjectInt(tomato_focus_item, "StartTime");
			tomato_focus_action->end_time = divoom_cJSON_GetObjectInt(tomato_focus_item, "EndTime");
			tomato_focus_action->focus_time = divoom_cJSON_GetObjectInt(tomato_focus_item, "FocusTime");
			divoom_queue_push(ptomato_time_handle->tomato_focus_queue[i], tomato_focus_action);
		}
	}
	divoom_port_file_close(fd);
	divoom_cJSON_Delete(root);
}

void divoom_tomato_time_load_config(void)
{
	divoom_tomato_time_load_tomato_data();
	divoom_tomato_time_load_focus_record(); //获取之前的记录
	divoom_tomato_time_get_todoy_tomato_total(0);
}

void divoom_tomato_time_config_save(void)
{
	divoom_tomato_time_save_all_tomato_info_local();
	divoom_tomato_time_save_focus_record();
	divoom_tomato_time_save_todoy_tomato_total(1);
}

void divoom_tomato_time_init(void)
{
	int i = 0;
	Pdivoom_tomato_time_info tomato_info = NULL;

	if (ptomato_time_handle)
		return;

	ptomato_time_handle = divoom_system_malloc(sizeof(divoom_tomato_time_struct));
	if (ptomato_time_handle == NULL)
		return;

	memset(ptomato_time_handle, 0, sizeof(divoom_tomato_time_struct));
	ptomato_time_handle->tomato_queue = divoom_queue_init(DIVOOM_TOMATO_TIME_NUM_MAX, __func__, __LINE__);
	ptomato_time_handle->tomato_image_lock = divoom_system_create_lock();

	for (i = 0; i < DIVOOM_TOMATO_TIME_NUM_MAX; i++)
	{
		ptomato_time_handle->tomato_focus_queue[i] = divoom_queue_init(DIVOOM_TOMATO_FOCUS_NUM_MAX, __func__, __LINE__);
	}

	char *table_item = "id INT PRIMARY KEY     NOT NULL," \
		"total_times INTEGER DEFAULT 0," \
		"year INTEGER DEFAULT 0," \
		"mon INTEGER DEFAULT 0," \
		"day INTEGER DEFAULT 0";

	divoom_database_manage_add_new_table(DIVOOM_TOMATO_TODAY_TOTAL_TABLE, table_item);
    divoom_database_manage_add_table_index(DIVOOM_TOMATO_TODAY_TOTAL_TABLE, "total_times");
    divoom_database_manage_add_table_index(DIVOOM_TOMATO_TODAY_TOTAL_TABLE, "year");
    divoom_database_manage_add_table_index(DIVOOM_TOMATO_TODAY_TOTAL_TABLE, "mon");
    divoom_database_manage_add_table_index(DIVOOM_TOMATO_TODAY_TOTAL_TABLE, "day");
//	divoom_database_manage_remove_record(DIVOOM_TOMATO_TODAY_TOTAL_TABLE, 1);

	char *item_data = "1, 0, 2024, 7, 23";
	divoom_database_manage_insert_new_item(DIVOOM_TOMATO_TODAY_TOTAL_TABLE, item_data);
	divoom_database_manage_get_record(DIVOOM_TOMATO_TODAY_TOTAL_TABLE, NULL, 1, divoom_tomato_time_db_get_data_callback, NULL);

	divoom_tomato_time_load_config();
	divoom_tomato_time_set_default_tomato();
	divoom_tomato_time_save_todoy_tomato_total(0);

	//初始化当前的
	tomato_info = divoom_tomato_time_get_one_tomato(0);
	if (tomato_info)
	{
		divoom_tomato_time_set_tomato(tomato_info, 3); //设置当前番茄时钟
	}
	else
	{
		tomato_info = divoom_tomato_time_get_default_tomato();
		divoom_tomato_time_set_tomato(tomato_info, 3); //设置当前番茄时钟
	}

	divoom_sys_model_crl_register(DIVOOM_MODEL_APP_TOMATO_TIME, divoom_tomato_time_event);
}

void divoom_tomato_time_play_phase_notice_tone(void)
{
	char path_name[128];
	uint8 vol;
	uint8 bell_idx;

	if (ptomato_time_handle == NULL)
	{
		return;
	}
	if (!ptomato_time_handle->tomato_info.sound_on_off)
	{
		return;
	}

	path_name[0] = 0;
	vol = ptomato_time_handle->tomato_info.sound_volume;
	if (ptomato_time_handle->tomato_info.sound_type == 0)
	{
		bell_idx = ptomato_time_handle->tomato_info.sound_system_index;
		if (bell_idx < DIVOOM_TOMATO_VOICE_NUM)
		{
			divoom_voice_read_get_filename(divoom_tomato_voice_table[bell_idx], path_name);
			divoom_local_music_ctrl_tone_play(path_name, vol, NULL, DIVOOM_TOMATO_RESTORE_TIME);
		}
	}
	else if (ptomato_time_handle->tomato_info.sound_type == 1)
	{
		divoom_voice_read_get_filename(DIVOOM_VOICE_ID_START_TOMATO_VOICE + ptomato_time_handle->tomato_info.id, path_name);
		divoom_local_music_ctrl_tone_play(path_name, vol, NULL, DIVOOM_TOMATO_RESTORE_TIME);
	}
}

void divoom_tomato_time_update(void)
{
    uint32 cur_time, next_reach_time, next_over_time;

	if (ptomato_time_handle == NULL)
		return;

	cur_time = divoom_system_get_ticks();

	if (ptomato_time_handle->tomato_status.main_status == TOMATO_MAIN_WAIT)
		return;

	//离开番茄页时没有UI来调get_time，这里自举一次，否则倒计时会停在frist_get_flag==0
	if (ptomato_time_handle->tomato_status.sub_status == TOMATO_START
		&& ptomato_time_handle->frist_get_flag == 0)
	{
		divoom_tomato_time_get_time();
	}

	if (ptomato_time_handle->tomato_status.sub_status != TOMATO_STOP)
	{
		if (ptomato_time_handle->tomato_reach_time >= cur_time)
        {
            next_reach_time = ptomato_time_handle->tomato_reach_time - cur_time;
        }
        else
        {
            next_reach_time = 0;
        }

		if (ptomato_time_handle->next_over_time >= cur_time) //闹铃时间
		{
			next_over_time = ptomato_time_handle->next_over_time - cur_time;
		}
		else
		{
			next_over_time = 0;
		}

		if (ptomato_time_handle->tomato_status.main_status == TOMATO_MAIN_FOCUS)//专注模式
		{
			switch (ptomato_time_handle->tomato_status.sub_status)
			{
				case TOMATO_START: //处理正常专注时间
				{
					if (ptomato_time_handle->frist_get_flag == 0)
						break;

					if (next_over_time == 0 && ptomato_time_handle->voice_start)//闹铃时间到
					{
						ptomato_time_handle->voice_start = 0;
						divoom_local_music_ctrl_stop_tone();
					}

					if (next_reach_time == 0) //时间到了进入休息时间段
					{
						ptomato_time_handle->tomato_cur_roll_times++;
						ptomato_time_handle->tomato_total_times++;

						if (ptomato_time_handle->tomato_cur_roll_times >= ptomato_time_handle->tomato_info.roll_times)
						{
							ptomato_time_handle->tomato_rest_time = ptomato_time_handle->tomato_info.long_rest_time;
//							ptomato_time_handle->tomato_cur_roll_times = 0;
						}
						else
						{
							ptomato_time_handle->tomato_rest_time = ptomato_time_handle->tomato_info.short_rest_time;
						}

						if (ptomato_time_handle->tomato_info.roll_times == 0)
						{
							ptomato_time_handle->tomato_status.main_status = TOMATO_MAIN_FOCUS;
							ptomato_time_handle->tomato_reach_time = cur_time + ptomato_time_handle->tomato_info.work_time * DIVOOM_TOMATO_ONE_TIME; //休息倒计时，要连续
						}
						else
						{
							ptomato_time_handle->tomato_status.main_status = TOMATO_MAIN_RESET;
							ptomato_time_handle->tomato_reach_time = cur_time + ptomato_time_handle->tomato_rest_time * DIVOOM_TOMATO_ONE_TIME; //休息倒计时，要连续
						}
						ptomato_time_handle->next_over_time = DIVOOM_TOMATO_TIP_TIME + cur_time;
						ptomato_time_handle->tomato_status.sub_status = TOMATO_START;
						ptomato_time_handle->frist_get_flag = 0; //获取时间
						ptomato_time_handle->voice_start = 1;

						divoom_tomato_time_disp_notice_gif();
						divoom_tomato_time_play_phase_notice_tone();

						//保存一次专注记录
						divoom_tomato_time_update_focus_record();
						#ifdef DIVOOM_TOMATO_SAVE_LOCAL_CONFIG
						divoom_tomato_time_save_focus_record();
						divoom_tomato_time_save_todoy_tomato_total(1);
						#endif
					}
				}
				break;

				case TOMATO_STOP:
				case TOMATO_PUASE:
					break;
			}
		}
		else
		{
			switch (ptomato_time_handle->tomato_status.sub_status)
			{
				case TOMATO_START:
				{
					if (next_over_time == 0 && ptomato_time_handle->voice_start)//闹铃时间到
					{
						ptomato_time_handle->voice_start = 0;
						divoom_local_music_ctrl_stop_tone();
					}

					if (next_reach_time == 0) //进入专注时间
					{
						ptomato_time_handle->tomato_status.main_status = TOMATO_MAIN_FOCUS;
						ptomato_time_handle->tomato_status.sub_status = TOMATO_START;
						ptomato_time_handle->frist_get_flag = 0; //重新获取时间
						ptomato_time_handle->voice_start = 1;
						ptomato_time_handle->tomato_reach_time = cur_time + ptomato_time_handle->tomato_info.work_time * DIVOOM_TOMATO_ONE_TIME; //专注时间
						ptomato_time_handle->next_over_time = DIVOOM_TOMATO_TIP_TIME + cur_time;
						divoom_tomato_time_disp_notice_gif();
						divoom_tomato_time_play_phase_notice_tone();

						if (ptomato_time_handle->tomato_cur_roll_times >= ptomato_time_handle->tomato_info.roll_times)
						{
							ptomato_time_handle->tomato_cur_roll_times = 0;
						}
					}
				}
					break;

				case TOMATO_STOP:
				case TOMATO_PUASE:
					break;
			}
		}
	}

	if (ptomato_time_handle->status == TOMATO_TRY_LISTEN)
	{
		if (cur_time > ptomato_time_handle->end_time)
		{
			divoom_tomato_time_try_listen_over(NULL, 0);
			divoom_local_music_ctrl_stop_tone();
			ptomato_time_handle->status = TOMATO_WAIT;
		}
	}
}

void divoom_tomato_time_send_focus_record(void)
{
	int i = 0, cur_time = 0;
	static int send_time = 0;
	Pdivoom_tomato_time_focus_action_json tomato_focus_action = NULL;

	if (ptomato_time_handle == NULL)
		return;

	if (!divoom_net_mqtt_get_net_status())
		return;

	cur_time = divoom_system_get_ticks();
	if (cur_time >= send_time) //每次发送各个任务的第一条
	{
		for (i = 0; i < DIVOOM_TOMATO_TIME_NUM_MAX; i++)
		{
			tomato_focus_action = divoom_queue_get_index_item(ptomato_time_handle->tomato_focus_queue[i], 0);
			if (tomato_focus_action)
				divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_TOMATO_TIME_FOCUS_ACTION, DIVOOM_SERVER_TOPIC, (void *)tomato_focus_action);
		}

		send_time = cur_time + 5;//间隔5s发送一次
	}
}

void divoom_tomato_time_loop(void)
{
	divoom_tomato_time_update();
	divoom_tomato_time_send_focus_record();
	divoom_tomato_time_dev_update_all();
	divoom_system_get_lock(ptomato_time_handle->tomato_image_lock);
	divoom_tomato_time_close_gif(0);
	divoom_system_free_lock(ptomato_time_handle->tomato_image_lock);
}

void divoom_tomato_time_close(void)
{
	if (ptomato_time_handle)
	{
		divoom_system_free(ptomato_time_handle);
		ptomato_time_handle = NULL;
	}
}
