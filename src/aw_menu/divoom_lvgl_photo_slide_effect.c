//幻灯片播放入口
#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_lvgl_photo_stream.h"
//#include "divoom_lvgl_photo_fade_out.h"
#include "divoom_lvgl_photo_push.h"
//#include "divoom_lvgl_photo_magazine.h"
//#include "divoom_lvgl_photo_origami.h"
//#include "divoom_lvgl_photo_rotate.h"
//#include "divoom_lvgl_photo_rotate_roller.h"
//#include "divoom_lvgl_photo_magazine_push.h"
//#include "divoom_lvgl_photo_magazine_fade_out.h"
#include "divoom_lvgl_photo_slide_effect.h"
#include "divoom_lvgl_select_album_play.h"
//#include "divoom_lvgl_photo_magazine_fold.h"
#include "divoom_lvgl_dial_menu.h"
#include "divoom_lvgl_font_manage.h"
//#include "divoom_lvgl_photo_normal.h"
#include "divoom_lvgl_photo_sticker.h"
#include "divoom_lvgl_top_control.h"
#include "divoom_astrootoo_time_pick.h"
#include "middle/include/divoom_aw_pic_decode.h"
#include "divoom_pixel_format.h"
#if !defined(DIVOOM_SIMULATOR) || !(DIVOOM_SIMULATOR)
extern void *sunxifb_alloc(size_t size, char *label);
extern void sunxifb_free(void **data, char *label);
extern void sunxifb_flush_cache(void *data, size_t size);
extern void *sunxifb_mem_get_phyaddr(void *data);
#endif

const char *divoom_lvgl_photo_slide_effect_name_table[] = 
{
    "Push",
    "Fade out",
    "photo stream",
    "magazine",
    "Rotate",
    "Normal",
};
enum{
    DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_BTN_LIKE,    //收藏
    DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_BTN_HIDE,    //隐藏
    DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_BTN_FILL,    //适配
    DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_BTN_ROTATE,      //照片旋转
    DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_BTN_EFFECT,    //幻灯片效果
    DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_BTN_SELECT_ALBUM,    //选择相册播放
    DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_BTN_CUR_ALBUM,    //浏览当前相册
    DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_BTN_SETTING,      //设置
    DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_BTN_END,      //结束
};

typedef void DIVOOM_MENU_CONTROL_MENU_LVGL_SLIDE_PAUSE(void *obj, int exit_flag);
typedef void DIVOOM_MENU_CONTROL_MENU_LVGL_SLIDE_REMOVE_NOTICE(void *obj, uint32 *pic_list, int pic_num);

typedef struct _divoom_lvgl_photo_slide_effect_item_callback
{
    int slide_type;
    DIVOOM_MENU_CONTROL_MENU_INIT *pinit;    //菜单的初始化
    DIVOOM_MENU_CONTROL_MENU_LVGL_CLEAN *pclean;    //菜单的加载动画lv_scr_clean
    DIVOOM_MENU_CONTROL_MENU_LVGL_DISP *pdisp;    //菜单的加载动画  内部要调用lv_scr_load
    DIVOOM_MENU_CONTROL_MENU_LVGL_SLIDE_PAUSE *pause;     //暂停播放
    DIVOOM_MENU_CONTROL_MENU_LVGL_GET_MAIN_OBJ *pget_lvgl_obj;      //获取LVGL  最上层的结构体方便做事件处理
    DIVOOM_MENU_CONTROL_MENU_LVGL_UPDATE_PLAY_TIME *pupdate_time;   //更新播放速度
    DIVOOM_MENU_CONTROL_MENU_LVGL_PLAY_NEXT  *pplay_next;   //播放下一组
    DIVOOM_MENU_CONTROL_MENU_LVGL_GET_PIC_INFO *pget_pic_info;      //获取照片列表相关信息
    DIVOOM_MENU_CONTROL_MENU_LVGL_UPDATE_DISP  *pupdate_disp;
    DIVOOM_MENU_CONTROL_MENU_LVGL_SLIDE_REMOVE_NOTICE *premove_notice;  //如果正在播放的相簿，有文件被移除
    
}divoom_lvgl_photo_slide_effect_item_callback, *Pdivoom_lvgl_photo_slide_effect_item_callback;

static const divoom_lvgl_photo_slide_effect_item_callback divoom_slide_effect_all_table[] =
{
    //{DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_MAGAZINE_FOLD, divoom_lvgl_photo_magazine_fold_init,  divoom_lvgl_photo_magazine_fold_exit, divoom_lvgl_photo_magazine_fold_disp, divoom_lvgl_photo_magazine_fold_pause, divoom_lvgl_photo_magazine_fold_get_lvgl_obj, divoom_lvgl_photo_magazine_fold_update_time, divoom_lvgl_photo_magazine_fold_play_next, divoom_lvgl_photo_magazine_fold_get_pic_info, NULL},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_BLINDS, divoom_lvgl_photo_push_init,  divoom_lvgl_photo_push_exit, divoom_lvgl_photo_push_disp, divoom_lvgl_photo_push_pause, divoom_lvgl_photo_push_get_lvgl_obj, divoom_lvgl_photo_push_update_time, divoom_lvgl_photo_push_play_next, divoom_lvgl_photo_push_get_pic_info, divoom_lvgl_photo_push_update_disp, divoom_lvgl_photo_push_remove_notice},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_MOSAIC, divoom_lvgl_photo_push_init,  divoom_lvgl_photo_push_exit, divoom_lvgl_photo_push_disp, divoom_lvgl_photo_push_pause, divoom_lvgl_photo_push_get_lvgl_obj, divoom_lvgl_photo_push_update_time, divoom_lvgl_photo_push_play_next, divoom_lvgl_photo_push_get_pic_info, divoom_lvgl_photo_push_update_disp, divoom_lvgl_photo_push_remove_notice},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_LARGE_IN, divoom_lvgl_photo_push_init,  divoom_lvgl_photo_push_exit, divoom_lvgl_photo_push_disp, divoom_lvgl_photo_push_pause, divoom_lvgl_photo_push_get_lvgl_obj, divoom_lvgl_photo_push_update_time, divoom_lvgl_photo_push_play_next, divoom_lvgl_photo_push_get_pic_info, divoom_lvgl_photo_push_update_disp, divoom_lvgl_photo_push_remove_notice},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_PUSH, divoom_lvgl_photo_push_init,  divoom_lvgl_photo_push_exit, divoom_lvgl_photo_push_disp, divoom_lvgl_photo_push_pause, divoom_lvgl_photo_push_get_lvgl_obj, divoom_lvgl_photo_push_update_time, divoom_lvgl_photo_push_play_next, divoom_lvgl_photo_push_get_pic_info, divoom_lvgl_photo_push_update_disp, divoom_lvgl_photo_push_remove_notice},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_MIX, divoom_lvgl_photo_push_init,  divoom_lvgl_photo_push_exit, divoom_lvgl_photo_push_disp, divoom_lvgl_photo_push_pause, divoom_lvgl_photo_push_get_lvgl_obj, divoom_lvgl_photo_push_update_time, divoom_lvgl_photo_push_play_next, divoom_lvgl_photo_push_get_pic_info, divoom_lvgl_photo_push_update_disp, divoom_lvgl_photo_push_remove_notice},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_FADE_OUT, divoom_lvgl_photo_push_init,  divoom_lvgl_photo_push_exit, divoom_lvgl_photo_push_disp, divoom_lvgl_photo_push_pause, divoom_lvgl_photo_push_get_lvgl_obj, divoom_lvgl_photo_push_update_time, divoom_lvgl_photo_push_play_next, divoom_lvgl_photo_push_get_pic_info, divoom_lvgl_photo_push_update_disp, divoom_lvgl_photo_push_remove_notice},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_ROTATE, divoom_lvgl_photo_push_init,  divoom_lvgl_photo_push_exit, divoom_lvgl_photo_push_disp, divoom_lvgl_photo_push_pause, divoom_lvgl_photo_push_get_lvgl_obj, divoom_lvgl_photo_push_update_time, divoom_lvgl_photo_push_play_next, divoom_lvgl_photo_push_get_pic_info, divoom_lvgl_photo_push_update_disp, divoom_lvgl_photo_push_remove_notice},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_PHOTO_STEAM, divoom_lvgl_photo_stream_init,  divoom_lvgl_photo_stream_eixt, divoom_lvgl_photo_stream_disp, divoom_lvgl_photo_stream_pause, divoom_lvgl_photo_stream_get_lvgl_obj, divoom_lvgl_photo_stream_update_time, divoom_lvgl_photo_stream_play_next, divoom_lvgl_photo_stream_get_pic_info, NULL, divoom_lvgl_photo_stream_remove_notice},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE0, divoom_lvgl_photo_sticker_init,  divoom_lvgl_photo_sticker_exit, divoom_lvgl_photo_sticker_disp, divoom_lvgl_photo_sticker_pause, divoom_lvgl_photo_sticker_get_lvgl_obj, divoom_lvgl_photo_sticker_update_time, NULL, divoom_lvgl_photo_sticker_get_pic_info, NULL, divoom_lvgl_photo_sticker_remove_notice},
    //{DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_MAGAZINE_FADE_OUT, divoom_lvgl_photo_magazine_fade_out_init,  divoom_lvgl_photo_magazine_fade_out_exit, divoom_lvgl_photo_magazine_fade_out_disp, divoom_lvgl_photo_magazine_fade_out_pause, divoom_lvgl_photo_magazine_fade_out_get_lvgl_obj, divoom_lvgl_photo_magazine_fade_out_update_time, divoom_lvgl_photo_magazine_fade_out_play_next, divoom_lvgl_photo_magazine_fade_out_get_pic_info, NULL},
    //{DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_MAGAZINE_PUSH, divoom_lvgl_photo_magazine_push_init,  divoom_lvgl_photo_magazine_push_exit, divoom_lvgl_photo_magazine_push_disp, divoom_lvgl_photo_magazine_push_pause, divoom_lvgl_photo_magazine_push_get_lvgl_obj, divoom_lvgl_photo_magazine_push_update_time, divoom_lvgl_photo_magazine_push_play_next, divoom_lvgl_photo_magazine_push_get_pic_info, NULL},
   // {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_ROTATE_ROLLER, divoom_lvgl_photo_rotate_roller_init,  divoom_lvgl_photo_rotate_roller_exit, divoom_lvgl_photo_rotate_roller_disp, divoom_lvgl_photo_rotate_roller_pause, divoom_lvgl_photo_rotate_roller_get_lvgl_obj, divoom_lvgl_photo_rotate_roller_update_time, NULL, NULL, NULL},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE1, divoom_lvgl_photo_sticker_init,  divoom_lvgl_photo_sticker_exit, divoom_lvgl_photo_sticker_disp, divoom_lvgl_photo_sticker_pause, divoom_lvgl_photo_sticker_get_lvgl_obj, divoom_lvgl_photo_sticker_update_time, NULL, divoom_lvgl_photo_sticker_get_pic_info, NULL, divoom_lvgl_photo_sticker_remove_notice},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE2, divoom_lvgl_photo_sticker_init,  divoom_lvgl_photo_sticker_exit, divoom_lvgl_photo_sticker_disp, divoom_lvgl_photo_sticker_pause, divoom_lvgl_photo_sticker_get_lvgl_obj, divoom_lvgl_photo_sticker_update_time, NULL, divoom_lvgl_photo_sticker_get_pic_info, NULL, divoom_lvgl_photo_sticker_remove_notice},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE3, divoom_lvgl_photo_sticker_init,  divoom_lvgl_photo_sticker_exit, divoom_lvgl_photo_sticker_disp, divoom_lvgl_photo_sticker_pause, divoom_lvgl_photo_sticker_get_lvgl_obj, divoom_lvgl_photo_sticker_update_time, NULL, divoom_lvgl_photo_sticker_get_pic_info, NULL, divoom_lvgl_photo_sticker_remove_notice},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE4, divoom_lvgl_photo_sticker_init,  divoom_lvgl_photo_sticker_exit, divoom_lvgl_photo_sticker_disp, divoom_lvgl_photo_sticker_pause, divoom_lvgl_photo_sticker_get_lvgl_obj, divoom_lvgl_photo_sticker_update_time, NULL, divoom_lvgl_photo_sticker_get_pic_info, NULL, divoom_lvgl_photo_sticker_remove_notice},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE5, divoom_lvgl_photo_sticker_init,  divoom_lvgl_photo_sticker_exit, divoom_lvgl_photo_sticker_disp, divoom_lvgl_photo_sticker_pause, divoom_lvgl_photo_sticker_get_lvgl_obj, divoom_lvgl_photo_sticker_update_time, NULL, divoom_lvgl_photo_sticker_get_pic_info, NULL, divoom_lvgl_photo_sticker_remove_notice},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE6, divoom_lvgl_photo_sticker_init,  divoom_lvgl_photo_sticker_exit, divoom_lvgl_photo_sticker_disp, divoom_lvgl_photo_sticker_pause, divoom_lvgl_photo_sticker_get_lvgl_obj, divoom_lvgl_photo_sticker_update_time, NULL, divoom_lvgl_photo_sticker_get_pic_info, NULL, divoom_lvgl_photo_sticker_remove_notice},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE7, divoom_lvgl_photo_sticker_init,  divoom_lvgl_photo_sticker_exit, divoom_lvgl_photo_sticker_disp, divoom_lvgl_photo_sticker_pause, divoom_lvgl_photo_sticker_get_lvgl_obj, divoom_lvgl_photo_sticker_update_time, NULL, divoom_lvgl_photo_sticker_get_pic_info, NULL, divoom_lvgl_photo_sticker_remove_notice},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE8, divoom_lvgl_photo_sticker_init,  divoom_lvgl_photo_sticker_exit, divoom_lvgl_photo_sticker_disp, divoom_lvgl_photo_sticker_pause, divoom_lvgl_photo_sticker_get_lvgl_obj, divoom_lvgl_photo_sticker_update_time, NULL, divoom_lvgl_photo_sticker_get_pic_info, NULL, divoom_lvgl_photo_sticker_remove_notice},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE9, divoom_lvgl_photo_sticker_init,  divoom_lvgl_photo_sticker_exit, divoom_lvgl_photo_sticker_disp, divoom_lvgl_photo_sticker_pause, divoom_lvgl_photo_sticker_get_lvgl_obj, divoom_lvgl_photo_sticker_update_time, NULL, divoom_lvgl_photo_sticker_get_pic_info, NULL, divoom_lvgl_photo_sticker_remove_notice},
};
    
typedef struct _divoom_lvgl_photo_slide_effect_add_setting_ctrl
{
    int old_id;
    int active_id;
    lv_style_t active_normal;
    lv_style_t press;
    void *pdirect_menu_ctrl;
    void *back_obj;
    void *btn_list_obj[3];
    void *icon_list_obj[3];
}divoom_lvgl_photo_slide_effect_add_setting_ctrl, *Pdivoom_lvgl_photo_slide_effect_add_setting_ctrl;

typedef struct _divoom_lvgl_photo_slide_effect_add_effect_ctrl
{
    int old_id;
    int active_id;
    lv_style_t active_normal;
    lv_style_t unactive_normal;
    lv_style_t press;
    void *pdirect_menu_ctrl;
    void *effect_cout_obj;
    void *effect_btn_obj[DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_END_TYPE]; 
}divoom_lvgl_photo_slide_effect_add_effect_ctrl, *Pdivoom_lvgl_photo_slide_effect_add_effect_ctrl;

typedef struct _divoom_lvgl_photo_slide_effect_select_album_ctrl
{
    int old_id;
    int active_id;
    lv_style_t active_normal;
    lv_style_t unactive_normal;
    lv_style_t press;
    void *pdirect_menu_ctrl;
    void *select_album_cout_obj;
    void *confirm_message_ctrl;
    void *last_active_obj;
    void *select_album_btn_obj[DIVOOM_PHOTO_MAXALBUM]; 
    int   album_id_list[DIVOOM_PHOTO_MAXALBUM]; 
}divoom_lvgl_photo_slide_effect_select_album_ctrl, *Pdivoom_lvgl_photo_slide_effect_select_album_ctrl;

typedef struct _divoom_lvgl_photo_slide_effect_play_setting_ctrl
{
    int status;     //0:非选中状态          
    int active_id;
    lv_style_t active_normal;
    lv_style_t unactive_normal;
    lv_style_t press;
    void *pdirect_menu_ctrl;
    void *play_setting_cout_obj;
    void *play_speed_obj;
    void *play_setting_btn_obj[2]; 
}divoom_lvgl_photo_slide_effect_play_setting_ctrl, *Pdivoom_lvgl_photo_slide_effect_play_setting_ctrl;

#define DIVOOM_SLIDE_WEATHER_IMAGE_COUNT 10

/* 1: LVGL直接加载PNG；0: 预解码为RGB565A8缓冲。 */
#ifndef DIVOOM_SLIDE_WEATHER_LVGL_USE_PNG
#define DIVOOM_SLIDE_WEATHER_LVGL_USE_PNG 1
#endif

typedef struct _divoom_lvgl_photo_slide_effect_ctrl
{
    uint8 destory_top_btn_flag;
    uint8 first_run_flag;
    uint8 select_effect_item;
    uint8 weather_images_loaded;
    int menu_id;
    int update_effect_flag; //是否要更新效果
    int pre_menu_event;
    int puase_time;
    int video_volumn;
    uint8 btn_hide_flag;    //btn控件隐藏标记
    void *main_obj;
    void *top_btn_cout;
    void *lv_snapshot_obj;
    lv_img_dsc_t *lv_snapshot_img;
    lv_timer_t *start_timer;    //切换到本页面后再启动图片/视频播放
    lv_timer_t *refresh_timer;  //更新效果
    lv_timer_t *refresh_top_timer;  //更新显示
    divoom_lvgl_photo_slide_effect_para slide_info;         //当前正在的效果
    divoom_lvgl_photo_slide_effect_para update_slide_info;  //将要更新的效果
    Pdivoom_pic_phone_manage_slide_config pslide_config;
    Pdivoom_lvgl_photo_slide_effect_item_callback pcur_runing_slide;
    divoom_lvgl_photo_view_para_info photo_view_info;
    divoom_lvgl_select_album_play_para palbum_select_info;
    divoom_lvgl_photo_slide_effect_add_setting_ctrl psetting_menu_info;
    divoom_lvgl_photo_slide_effect_add_effect_ctrl  peffect_menu_info;
    divoom_lvgl_photo_slide_effect_select_album_ctrl  pselect_album_info;
    divoom_lvgl_photo_slide_effect_play_setting_ctrl  play_setting_info;
    divoom_lvgl_photo_slide_effect_info pcur_play_info;
    lv_style_t  top_disp_style;
    lv_style_t  top_plate_style;
    lv_style_t  top_menu_style;
    void *top_disp_obj;
    void *top_plate_obj;
    void *weather_plate_obj;
    void *title_label_obj;
    void *time_disp_obj;
    void *weather_disp_obj;
    void *temp_disp_obj;
#if !DIVOOM_SLIDE_WEATHER_LVGL_USE_PNG
    lv_img_dsc_t weather_img_dsc[DIVOOM_SLIDE_WEATHER_IMAGE_COUNT];
    divoom_aw_png_decode_buf_t weather_img_decode[DIVOOM_SLIDE_WEATHER_IMAGE_COUNT];
#endif
    uint8 video_osd_enabled;
    uint8 video_osd_front_index;
    void *video_osd_buf[2];
    uint32 video_osd_buf_size;
    void *handle;
}divoom_lvgl_photo_slide_effect_ctrl, *Pdivoom_lvgl_photo_slide_effect_ctrl;

/* 真源 GalleryAlbumWatchFaceLayer + WatchFace.module.css albumWidget* */
#define DIVOOM_SLIDE_TOP_WIDGET_MARGIN   8
#define DIVOOM_SLIDE_TOP_PLATE_RADIUS    12
#define DIVOOM_SLIDE_TOP_PLATE_OPA       (LV_OPA_COVER * 35 / 100)
#define DIVOOM_SLIDE_TOP_STACK_GAP       8
#define DIVOOM_SLIDE_TOP_PLATE_PAD_V     6
#define DIVOOM_SLIDE_TOP_PLATE_PAD_H     10
#define DIVOOM_SLIDE_TOP_WEATHER_PAD_V   4
#define DIVOOM_SLIDE_TOP_WEATHER_PAD_H   8
#define DIVOOM_SLIDE_TOP_WEATHER_GAP     6
#define DIVOOM_SLIDE_TOP_WEATHER_ICON    28
#define DIVOOM_SLIDE_TOP_FONT            24
#define DIVOOM_SLIDE_TOP_LINE_H          24
#define DIVOOM_SLIDE_TOP_TIME_W          72
#define DIVOOM_SLIDE_TOP_TEMP_W          56
/* 仅显示时间时扩大背板并居中文字，尺寸单位为像素。 */
#define DIVOOM_SLIDE_TOP_CLOCK_ONLY_W    126
#define DIVOOM_SLIDE_TOP_CLOCK_ONLY_H    48
/* 单时间字形受字体基线影响，按仿真像素测量向上补偿 7px，使视觉中心落在背板中心。 */
#define DIVOOM_SLIDE_TOP_CLOCK_OFFSET_Y  (-7)
/* LVGL 字形基线比天气图标的视觉中心低，按仿真像素测量向上补偿 7px。 */
#define DIVOOM_SLIDE_TOP_TEMP_OFFSET_Y   (-7)
#define DIVOOM_SLIDE_TOP_TEXT_COLOR      0xFFFFFF

/* albumWidgetDescription · WatchFace.module.css */
#define DIVOOM_SLIDE_TITLE_MARGIN        8
#define DIVOOM_SLIDE_TITLE_PAD_V         8
#define DIVOOM_SLIDE_TITLE_PAD_H         12
#define DIVOOM_SLIDE_TITLE_RADIUS        DIVOOM_SLIDE_TOP_PLATE_RADIUS
#define DIVOOM_SLIDE_TITLE_PLATE_OPA     DIVOOM_SLIDE_TOP_PLATE_OPA
#define DIVOOM_SLIDE_TITLE_FONT          24
#define DIVOOM_SLIDE_TITLE_WIDTH         (DIVOOM_SCREEN_WIDTH - DIVOOM_SLIDE_TITLE_MARGIN * 2)

void divoom_lvgl_photo_slide_effect_add_effect_btn(Pdivoom_lvgl_photo_slide_effect_ctrl phandle);
void divoom_lvgl_photo_slide_effect_add_setting_btn(Pdivoom_lvgl_photo_slide_effect_ctrl phandle);
void divoom_lvgl_photo_slide_effect_select_album_btn(Pdivoom_lvgl_photo_slide_effect_ctrl phandle);
void divoom_lvgl_photo_slide_effect_add_play_setting_btn(Pdivoom_lvgl_photo_slide_effect_ctrl phandle);
void divoom_lvgl_photo_slide_effect_destory_btn(Pdivoom_lvgl_photo_slide_effect_ctrl phandle);
void divoom_lvgl_photo_slide_effect_create_btn(Pdivoom_lvgl_photo_slide_effect_ctrl phandle);

void divoom_lvgl_photo_slide_effect_update_top_disp(Pdivoom_lvgl_photo_slide_effect_ctrl phandle);
void divoom_lvgl_photo_slide_effect_refresh_title_overlay(Pdivoom_lvgl_photo_slide_effect_ctrl phandle);

static void divoom_lvgl_photo_slide_effect_show_overlays(Pdivoom_lvgl_photo_slide_effect_ctrl phandle)
{
    if (phandle == NULL)
    {
        return;
    }
    if (phandle->pslide_config->clock_flag == 0 && phandle->pslide_config->weather_flag == 0)
    {
        lv_obj_add_flag(phandle->top_disp_obj, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        divoom_lvgl_photo_slide_effect_update_top_disp(phandle);
    }
    divoom_lvgl_photo_slide_effect_refresh_title_overlay(phandle);
}



static void divoom_lvgl_photo_slide_effect_event_cb(lv_event_t * e)
{
	int clock_id;
	Pdivoom_lvgl_dial_ctr_info data = NULL;
	divoom_sys_msg_info model_msg = {0};
	static lv_point_t p1;
    lv_point_t p2;
	lv_event_code_t code = lv_event_get_code(e);
    Pdivoom_lvgl_photo_slide_effect_ctrl phandle = (Pdivoom_lvgl_photo_slide_effect_ctrl)lv_event_get_user_data(e);

    if(code == LV_EVENT_KEY)
    {
        char *key_code = ((char *)lv_event_get_param(e));
        

        switch(key_code[0])
        {            
            case  DIVOOM_LVGL_KEY_NEXT:
            {
                divoom_light_app_play_next_clock(1);            
				//divoom_menu_control_clearall_load(DIVOOM_MENU_CONTROL_ID_DIAL_PREVIEW, NULL, 0, 0);
            }
                break;
               
            case DIVOOM_LVGL_KEY_PRE:
    		{
                divoom_light_app_play_next_clock(0);
				//divoom_menu_control_clearall_load(DIVOOM_MENU_CONTROL_ID_DIAL_PREVIEW, NULL, 0, 0);
    		
    		}
                break;
               
            case DIVOOM_LVGL_KEY_ENTER:
            {
                Pdivoom_lvgl_photo_manage_para pmanage_info = divoom_system_malloc(sizeof(divoom_lvgl_photo_manage_para));
                //进入配置菜单   
                #if 1
                divoom_menu_control_clearall_load(DIVOOM_MENU_CONTROL_ID_MAIN, NULL, 0, 0);
                #else
                #if 0
                phandle->btn_hide_flag = 0;
                //if(phandle->pcur_runing_slide && phandle->handle) phandle->pcur_runing_slide->pdisp(phandle->handle, NULL);
                divoom_lvgl_photo_slide_effect_create_btn(phandle);
                #else
                if(phandle->pcur_runing_slide && phandle->handle && phandle->pcur_runing_slide->pause)
                {
                    phandle->pcur_runing_slide->pause(phandle->handle, 1);
                }
                pmanage_info->album_id = phandle->pcur_play_info.album_id;  
                if(pmanage_info->album_id == 0)
                {
                    pmanage_info->album_id = divoom_pic_album_manage_get_album_id_from_type(DIVOOM_PIC_ALBUM_TYPE_ALL);
                }
                if(phandle->pcur_runing_slide && phandle->handle && phandle->pcur_runing_slide->pget_pic_info)
                {
                    divoom_lvgl_photo_view_para_info pic_info;
                    phandle->pcur_runing_slide->pget_pic_info(phandle->handle, &pic_info);
                    pmanage_info->pic_id = pic_info.pic_id;
                }
                else
                {
                    pmanage_info->pic_id = 0;
                }
                pmanage_info->operate_type = DIVOOM_LVGL_PHOTO_MANAGE_CLOCK_VIEW_PICTURE;
    			divoom_menu_control_clearall_load(DIVOOM_MENU_CONTROL_ID_PHOTO_MANAGE, pmanage_info, 0, 0);
                #endif
                #endif
            }
                break;

            default:
                divoom_main_keypad_send_key_info(key_code[0], key_code[1]);
                break;
                #if 0
            case DIVOOM_LVGL_KEY_HOME:
                //进入浏览照片模式
                phandle->photo_view_info.cur_pos = -1;
                if(phandle->pcur_runing_slide && phandle->handle && phandle->pcur_runing_slide->pget_pic_info)
                {
                    phandle->pcur_runing_slide->pget_pic_info(phandle->handle, &phandle->photo_view_info);
                    if(phandle->photo_view_info.cur_pos != -1)
                    {
                        if(phandle->pcur_runing_slide && phandle->handle && phandle->pcur_runing_slide->pause)
                        {
                            phandle->pcur_runing_slide->pause(phandle->handle);
                        }
                        phandle->pre_menu_event = DIVOOM_MENU_CONTROL_ID_PHOTO_SLIDE_EFFECT;
                        divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_PHOTO_VIEW, &phandle->photo_view_info, 0, 0);
                    }
                }
                break;  
             #endif
        }
    }
}

int divoom_lvgl_photo_slide_effect_start_new_config(Pdivoom_lvgl_photo_slide_effect_ctrl phandle ,Pdivoom_lvgl_photo_slide_effect_para pnew_config)
{
    int i;
    void *new_handle, *new_obj;
    int was_running;
    void *pclean_handle = NULL;
    Pdivoom_lvgl_photo_slide_effect_item_callback pcur_new_slide = NULL;
    DIVOOM_MENU_CONTROL_MENU_LVGL_CLEAN *pclean_callback = NULL;
    
    
    if(phandle->handle && phandle->pcur_runing_slide &&
        pnew_config->slide_type == phandle->slide_info.slide_type &&
        pnew_config->album_id == phandle->slide_info.album_id)
    {
        divoom_print("pass the same album!\n");
        return 0;
    }
        
    divoom_print("change album!: %d, %d, %d, %d\n", pnew_config->slide_type, phandle->slide_info.slide_type,pnew_config->album_id, phandle->slide_info.album_id);
    
    for(i = 0; i < sizeof(divoom_slide_effect_all_table)/sizeof(divoom_slide_effect_all_table[0]); i++)   
    {
        if(divoom_slide_effect_all_table[i].slide_type == pnew_config->slide_type)
        {
            pcur_new_slide = (Pdivoom_lvgl_photo_slide_effect_item_callback)&divoom_slide_effect_all_table[i];
            break;
        }
    }
    
    
    if(pcur_new_slide == NULL)
    {
        pcur_new_slide = (Pdivoom_lvgl_photo_slide_effect_item_callback)&divoom_slide_effect_all_table[0];
    }
    
    was_running = phandle->pcur_runing_slide != NULL && phandle->handle != NULL;
    if(was_running)
    {
        //这个要先关闭之前的，再开始新的

        lv_img_dsc_t *new_snapshot = lv_snapshot_take(lv_scr_act(), LV_COLOR_FORMAT_RGB565);
        if(new_snapshot)
        {
            lv_img_dsc_t *old_snapshot = phandle->lv_snapshot_img;
            lv_img_set_src(phandle->lv_snapshot_obj, new_snapshot);
            phandle->lv_snapshot_img = new_snapshot;
            if(old_snapshot) lv_snapshot_free(old_snapshot);
        }
        lv_scr_load(phandle->lv_snapshot_obj);
        lv_group_focus_obj(phandle->lv_snapshot_obj);
        divoom_lvgl_top_control_update_active();
        
        
        pclean_callback = phandle->pcur_runing_slide->pclean;
        pclean_handle = phandle->handle;
        if(phandle->pcur_runing_slide && phandle->handle && phandle->pcur_runing_slide->pause)
        {    
            phandle->pcur_runing_slide->pause(pclean_handle, 1);    
            if(phandle->pcur_runing_slide && phandle->handle && phandle->pcur_runing_slide->pget_pic_info)
            {
                divoom_lvgl_photo_view_para_info pic_info = {0};
                phandle->pcur_runing_slide->pget_pic_info(pclean_handle, &pic_info);
                phandle->pcur_play_info.pic_pos = pic_info.pic_id;
            }
            else
            {
                phandle->pcur_play_info.pic_pos = divoom_pic_album_manage_get_last_play_album_pic_id(phandle->slide_info.album_id, NULL);
            }
        }
        phandle->handle = NULL;
        phandle->pcur_runing_slide = NULL;
        phandle->main_obj = phandle->lv_snapshot_obj;
        if(pclean_callback)
        {
            pclean_callback(pclean_handle);
        }
        lv_img_cache_invalidate_src(NULL);
    }
    
    divoom_print("we will play: %d, %d\n", pnew_config->album_id, pnew_config->slide_type);
    if(phandle->pcur_play_info.album_id != pnew_config->album_id)
    {
        phandle->pcur_play_info.album_id = pnew_config->album_id;
        phandle->pcur_play_info.pic_pos = 0;
    }
    new_handle = pcur_new_slide->pinit(&phandle->pcur_play_info, phandle->menu_id);
    new_obj = new_handle ? pcur_new_slide->pget_lvgl_obj(new_handle) : NULL;
    if(new_obj == NULL)
    {
        if(new_handle && pcur_new_slide->pclean) pcur_new_slide->pclean(new_handle);
        phandle->handle = NULL;
        phandle->pcur_runing_slide = NULL;
        phandle->main_obj = phandle->lv_snapshot_obj;
        phandle->first_run_flag = 1;
        phandle->update_effect_flag = 1;
        lv_timer_pause(phandle->refresh_timer);
        lv_timer_pause(phandle->refresh_top_timer);
        lv_scr_load(phandle->lv_snapshot_obj);
        divoom_print("photo slide: child initialization failed; retain transition for retry\n");
        return 0;
    }
    phandle->handle = new_handle;
    phandle->pcur_runing_slide = pcur_new_slide;
    phandle->main_obj = new_obj;
    phandle->first_run_flag = 0;
    memcpy(&phandle->slide_info, pnew_config, sizeof(divoom_lvgl_photo_slide_effect_para));
    if(was_running) pcur_new_slide->pdisp(new_handle, NULL);
    lv_obj_set_style_bg_color(phandle->main_obj, lv_color_black(), 0);
    lv_group_focus_obj(phandle->main_obj);
    divoom_lvgl_top_control_update_active();
    
    
    lv_obj_add_event_cb(phandle->main_obj, divoom_lvgl_photo_slide_effect_event_cb,
        LV_EVENT_KEY, phandle);
    phandle->pslide_config->album_id = pnew_config->album_id;
    divoom_pic_phone_manage_save_slide_config(0);
    return 1;
    
    
}

void divoom_lvgl_photo_slide_effect_start_new_slide_type(Pdivoom_lvgl_photo_slide_effect_ctrl phandle ,int slide_type)
{

    if(phandle->slide_info.slide_type == slide_type)
    {
        divoom_print("this is the same album name and passed %d!\n", slide_type);
        return;
    }
    
    phandle->update_slide_info.slide_type = slide_type;
    phandle->update_slide_info.album_id = phandle->slide_info.album_id;
    phandle->update_effect_flag = 1;
    
}

void divoom_lvgl_photo_slide_effect_start_new_album_name(Pdivoom_lvgl_photo_slide_effect_ctrl phandle ,uint32 album_id)
{
    if(phandle->slide_info.album_id == album_id && phandle->update_slide_info.slide_type == phandle->slide_info.slide_type &&
        divoom_pic_album_manage_get_refresh_album_info() == 0)
    {
        divoom_print("this is the same album name and passed %d!\n", album_id);
        return;
    }
    phandle->update_slide_info.slide_type = phandle->slide_info.slide_type;
    phandle->update_slide_info.album_id = album_id;
    phandle->update_effect_flag = 1;
}

void divoom_lvgl_photo_slide_effect_update_play_time(Pdivoom_lvgl_photo_slide_effect_ctrl phandle ,int play_time)
{
    if(phandle->pcur_runing_slide && phandle->handle && phandle->pcur_runing_slide->pupdate_time)
        phandle->pcur_runing_slide->pupdate_time(phandle->handle, play_time);
}


static void divoom_lvgl_photo_slide_effect_setting_event_cb(lv_event_t * e)
{
    int active_id;
    lv_obj_t *cur_obj;
    lv_group_t * cur_group_obj = lv_obj_get_group(lv_event_get_target_obj(e));
    Pdivoom_lvgl_photo_slide_effect_ctrl phandle = (Pdivoom_lvgl_photo_slide_effect_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target_obj(e);
    if(code == LV_EVENT_KEY)
    {
        char *key_code = ((char *)lv_event_get_param(e));

        switch(key_code[0])
        {            
            case DIVOOM_LVGL_KEY_NEXT:
            case DIVOOM_LVGL_KEY_PRE:
                cur_obj = divoom_menu_direct_ctrl_get(phandle->psetting_menu_info.pdirect_menu_ctrl, cur_obj, key_code[0]==DIVOOM_LVGL_KEY_PRE?DIVOOM_MENU_DIRECT_LEFT:DIVOOM_MENU_DIRECT_RIGHT);                            
                lv_group_focus_obj(cur_obj);
                for(int i = 0; i < 3; i++)
                {
                    if(cur_obj == phandle->psetting_menu_info.btn_list_obj[i])
                    {
                        if(phandle->psetting_menu_info.active_id != i)
                        {
                            divoom_menu_control_lv_img_set_src(phandle->psetting_menu_info.icon_list_obj[i], "14/14_up_arrow.png");
                            lv_obj_clear_flag(phandle->psetting_menu_info.icon_list_obj[i], LV_OBJ_FLAG_HIDDEN);
                            lv_obj_add_flag(phandle->psetting_menu_info.icon_list_obj[phandle->psetting_menu_info.active_id], LV_OBJ_FLAG_HIDDEN);
                            phandle->psetting_menu_info.active_id = i;
                        }
                        break;
                    }
                }
                break;
               
            case DIVOOM_LVGL_KEY_ESC:
                phandle->destory_top_btn_flag = 1;            
                break;
               
            case DIVOOM_LVGL_KEY_ENTER:
                divoom_menu_control_lv_img_set_src(phandle->psetting_menu_info.back_obj, "14/14_set_bg2.png");
                //进入配置菜单                  
                phandle->btn_hide_flag = 0;
                if(phandle->pcur_runing_slide && phandle->handle) phandle->pcur_runing_slide->pause(phandle->handle, 0);
                divoom_menu_control_lv_img_set_src(phandle->psetting_menu_info.icon_list_obj[phandle->psetting_menu_info.active_id], "14/14_down_arrow.png");
                if(phandle->psetting_menu_info.active_id == 0)
                {
                    lv_obj_clear_flag(phandle->peffect_menu_info.effect_cout_obj, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_add_flag(phandle->play_setting_info.play_setting_cout_obj, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_add_flag(phandle->pselect_album_info.select_album_cout_obj, LV_OBJ_FLAG_HIDDEN);
                    lv_group_focus_obj(phandle->peffect_menu_info.effect_btn_obj[phandle->peffect_menu_info.active_id]);
                }
                else if(phandle->psetting_menu_info.active_id == 1)
                {
                    lv_obj_add_flag(phandle->peffect_menu_info.effect_cout_obj, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_add_flag(phandle->play_setting_info.play_setting_cout_obj, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_clear_flag(phandle->pselect_album_info.select_album_cout_obj, LV_OBJ_FLAG_HIDDEN);
                    lv_group_focus_obj(phandle->pselect_album_info.select_album_btn_obj[phandle->pselect_album_info.active_id]);
                }
                else
                {
                    lv_obj_add_flag(phandle->peffect_menu_info.effect_cout_obj, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_clear_flag(phandle->play_setting_info.play_setting_cout_obj, LV_OBJ_FLAG_HIDDEN);
                    phandle->play_setting_info.active_id = 0;
                    phandle->play_setting_info.status = 0;
                    lv_obj_add_flag(phandle->pselect_album_info.select_album_cout_obj, LV_OBJ_FLAG_HIDDEN);
                    lv_group_focus_obj(phandle->play_setting_info.play_setting_btn_obj[phandle->play_setting_info.active_id]);
                }
  
                break;
               
            case DIVOOM_LVGL_KEY_HOME:
                break;
                          
        }
    }
}


void divoom_lvgl_photo_slide_effect_do_remove_photos(Pdivoom_lvgl_photo_slide_effect_ctrl phandle)
{
    Pdivoom_pic_album_manage_delete_info_item premove_info = divoom_pic_album_manage_get_remove_info();

    if(premove_info)
    {        
        if(phandle->pcur_runing_slide && phandle->handle &&
            phandle->pcur_runing_slide->premove_notice && phandle->pslide_config->album_id == premove_info->album_id)
        {
            phandle->pcur_runing_slide->premove_notice(phandle->handle, premove_info->pic_list, premove_info->pic_num);
        }
        divoom_pic_album_manage_do_remove_info(premove_info);
    }
}

void divoom_lvgl_photo_slide_effect_refresh_slide_type_time_handle(lv_timer_t *timer)
{
    int i;
    
    Pdivoom_lvgl_photo_slide_effect_ctrl phandle = (Pdivoom_lvgl_photo_slide_effect_ctrl)lv_timer_get_user_data(timer);

    if(phandle->video_volumn != phandle->pslide_config->video_volume)
    {
        divoom_local_movie_play_set_movie_volume(divoom_pic_phone_manage_convert_video_volume(phandle->pslide_config->video_volume));
        phandle->video_volumn = phandle->pslide_config->video_volume;
    }
    divoom_lvgl_photo_slide_effect_do_remove_photos(phandle);
    if(phandle->destory_top_btn_flag)
    {
        divoom_lvgl_photo_slide_effect_destory_btn(phandle);
        phandle->destory_top_btn_flag = 0;
    }
    if(divoom_pic_album_manage_get_refresh_album_info())
    {
        //发生变化后，故意触发更新
        
        phandle->update_effect_flag = 1;
        phandle->update_slide_info.album_id = phandle->pslide_config->album_id;
        phandle->update_slide_info.slide_type = phandle->pslide_config->cur_play_effect_type;
        phandle->slide_info.album_id = 0;
    }

    if(phandle->update_effect_flag == 0)
    {
        return;
    }
    if(phandle->main_obj != lv_scr_act())
    {
        return;
    }

    divoom_menu_control_change_slide_effect();
    
    phandle->update_effect_flag = 0;    
}

void divoom_lvgl_photo_slide_effect_change_slide_effect_mode(void *handle)
{
    Pdivoom_lvgl_photo_slide_effect_ctrl phandle = (Pdivoom_lvgl_photo_slide_effect_ctrl)handle;

    
    if(divoom_lvgl_photo_slide_effect_start_new_config(phandle, &phandle->update_slide_info))
    {        
        divoom_lvgl_photo_slide_effect_show_overlays(phandle);
    }
}

int divoom_lvgl_photo_slide_effect_is_empty_album(void *handle)
{
    Pdivoom_lvgl_photo_slide_effect_ctrl phandle = (Pdivoom_lvgl_photo_slide_effect_ctrl)handle;
    divoom_lvgl_photo_view_para_info pic_info;

    if(phandle == NULL || phandle->pcur_runing_slide == NULL || phandle->handle == NULL)
    {
        return 0;
    }
    if(phandle->pcur_runing_slide->pget_pic_info == NULL)
    {
        return 0;
    }
    memset(&pic_info, 0, sizeof(pic_info));
    phandle->pcur_runing_slide->pget_pic_info(phandle->handle, &pic_info);
    if(pic_info.pic_num == NULL)
    {
        return 0;
    }
    return *pic_info.pic_num == 0;
}

static void divoom_lvgl_photo_slide_effect_set_title_overlay_hidden(Pdivoom_lvgl_photo_slide_effect_ctrl phandle, int hidden)
{
    lv_obj_t *stack;

    if (phandle == NULL || phandle->title_label_obj == NULL)
    {
        return;
    }
    stack = lv_obj_get_parent(phandle->title_label_obj);
    if (stack == NULL)
    {
        return;
    }
    if (hidden)
    {
        lv_obj_add_flag(stack, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_clear_flag(stack, LV_OBJ_FLAG_HIDDEN);
    }
}

void divoom_lvgl_photo_slide_effect_refresh_title_overlay(Pdivoom_lvgl_photo_slide_effect_ctrl phandle)
{
    char title_buf[1024];
    divoom_lvgl_photo_view_para_info pic_info;

    if (phandle == NULL || phandle->title_label_obj == NULL)
    {
        return;
    }
    title_buf[0] = 0;
    if (phandle->pcur_runing_slide && phandle->pcur_runing_slide->pget_pic_info && phandle->handle)
    {
        memset(&pic_info, 0, sizeof(pic_info));
        phandle->pcur_runing_slide->pget_pic_info(phandle->handle, &pic_info);
        if (pic_info.pic_id)
        {
            divoom_database_manage_get_pic_title(pic_info.pic_id, title_buf);
        }
    }
    divoom_lvgl_photo_slide_effect_update_title_disp(phandle->title_label_obj, title_buf);
}

void divoom_lvgl_photo_slide_effect_refresh_top_display_time_handle(lv_timer_t *timer)
{
    Pdivoom_lvgl_photo_slide_effect_ctrl phandle = (Pdivoom_lvgl_photo_slide_effect_ctrl)lv_timer_get_user_data(timer);
    divoom_lvgl_photo_slide_effect_update_top_disp(phandle);
    divoom_lvgl_photo_slide_effect_refresh_title_overlay(phandle);
}

typedef struct  _divoom_lvgl_photo_slide_effect_btn_item
{
    char  *image_path;
    char  *btn_name;
}divoom_lvgl_photo_slide_effect_btn_item, *Pdivoom_lvgl_photo_slide_effect_btn_item;

static const char * btn_set_name_list[] =
{
    "SlideTopic",
    "Album",
    "Setting",
};

void divoom_lvgl_photo_slide_effect_add_setting_btn(Pdivoom_lvgl_photo_slide_effect_ctrl phandle)
{
    void *label_obj, *image_obj;    

    //后面这位置还要加图片
    image_obj = lv_img_create(phandle->top_btn_cout);
    lv_obj_set_pos(image_obj, 0, 0);
    lv_obj_set_size(image_obj, 800, 500);
    phandle->psetting_menu_info.back_obj = image_obj;
    divoom_menu_control_lv_img_set_src(phandle->psetting_menu_info.back_obj, "14/14_set_bg1.png");
    phandle->psetting_menu_info.pdirect_menu_ctrl = divoom_menu_direct_ctrl_create(3, 1);   

    divoom_menu_control_init_btn_stype(&phandle->psetting_menu_info.active_normal, NULL, 
        NULL, NULL, &phandle->psetting_menu_info.press, NULL, 19);
    lv_style_set_border_width(&phandle->psetting_menu_info.active_normal, 0); //没有边框
    lv_style_set_text_color(&phandle->psetting_menu_info.active_normal, lv_color_hex(DIVOOM_MENNU_TEXT_ACTIVE_COLOR));
    lv_style_set_border_color(&phandle->psetting_menu_info.press, lv_color_hex(0x464646));
    phandle->psetting_menu_info.active_id = 0;
    for(int i = 0; i < 3; i++)
    {
        phandle->psetting_menu_info.btn_list_obj[i] = lv_btn_create(phandle->psetting_menu_info.back_obj);
        lv_obj_add_style(phandle->psetting_menu_info.btn_list_obj[i], &phandle->psetting_menu_info.active_normal, 0);
        lv_obj_add_style(phandle->psetting_menu_info.btn_list_obj[i], &phandle->psetting_menu_info.press, LV_STATE_FOCUSED);
        lv_obj_add_style(phandle->psetting_menu_info.btn_list_obj[i], &phandle->psetting_menu_info.press, LV_STATE_FOCUS_KEY);
        divoom_menu_direct_ctrl_update(phandle->psetting_menu_info.pdirect_menu_ctrl, phandle->psetting_menu_info.btn_list_obj[i], i, 0);
        lv_obj_set_pos(phandle->psetting_menu_info.btn_list_obj[i], 44 + 240*i, 44);
    	lv_obj_set_size(phandle->psetting_menu_info.btn_list_obj[i], 232, 82);
        
        label_obj = lv_label_create(phandle->psetting_menu_info.btn_list_obj[i]);
        divoom_menu_control_disp_label_text(label_obj, divoom_menu_control_get_disp_str(btn_set_name_list[i]), 232, 82, 24, 0, LV_TEXT_ALIGN_CENTER, phandle->menu_id);
        lv_obj_center(label_obj);
        
        image_obj = lv_img_create(phandle->psetting_menu_info.back_obj);
        lv_obj_set_pos(image_obj, 148 + 240*i, 133);
        lv_obj_set_size(image_obj, 24, 24);
        phandle->psetting_menu_info.icon_list_obj[i] = image_obj;
        if(i == 0)
        {
            divoom_menu_control_lv_img_set_src(image_obj, "14/14_up_arrow.png");
        }
        else
        {
            divoom_menu_control_lv_img_set_src(image_obj, "14/14_down_arrow.png");
            lv_obj_add_flag(phandle->psetting_menu_info.icon_list_obj[i], LV_OBJ_FLAG_HIDDEN);
        }
    
        lv_obj_add_event_cb(phandle->psetting_menu_info.btn_list_obj[i], divoom_lvgl_photo_slide_effect_setting_event_cb, LV_EVENT_KEY, phandle);
    }
    
    lv_obj_add_flag(phandle->psetting_menu_info.back_obj, LV_OBJ_FLAG_HIDDEN);
    
}

void divoom_lvgl_photo_slide_effect_select_album_btn_no_photo_dialog_callback(void *handle, int ctrl_id)
{
    Pdivoom_lvgl_photo_slide_effect_ctrl phandle = (Pdivoom_lvgl_photo_slide_effect_ctrl)handle;
    
    divoom_lvgl_message_box_destroy(phandle->pselect_album_info.confirm_message_ctrl, 0);
    phandle->pselect_album_info.confirm_message_ctrl = NULL;
    lv_group_focus_obj(phandle->pselect_album_info.select_album_btn_obj[phandle->pselect_album_info.active_id]);
}

/*********************************************************************
  Function:        divoom_lvgl_photo_slide_effect_select_album_btn_no_photo_dialog
  Description:     提示这个相簿没有相片
  Input:           phandle
                   type  0：删除所选择照片; 1:删除所有照片；2：移除所有照片； 3：移除所选择的照片；
  Return:          NONE
**********************************************************************/

static void divoom_lvgl_photo_slide_effect_select_album_btn_no_photo_dialog(Pdivoom_lvgl_photo_slide_effect_ctrl phandle)
{
    char *title, *desc, *confirm;
    
    
    phandle->pselect_album_info.confirm_message_ctrl = divoom_lvgl_message_box_init(phandle->menu_id, phandle->main_obj, phandle, divoom_lvgl_photo_slide_effect_select_album_btn_no_photo_dialog_callback, 
        80, 320, 640, 408, "2/2_confirm_bg.png", lv_color_hex(0x222222), 1);
    
    title = divoom_menu_control_get_disp_str("NoPictureInAlbum");
    desc = divoom_menu_control_get_disp_str("NoPictureInAlbumDesc");
    
    divoom_lvgl_message_box_add_btn_item(phandle->pselect_album_info.confirm_message_ctrl, divoom_menu_control_get_disp_str("Cancel"), 32, lv_color_hex(0x444444), lv_color_hex(0xBBBBBB),lv_color_hex(0xFFFFFF), 0, 
        180, 294, 282, 90);
    divoom_lvgl_message_box_add_label_item(phandle->pselect_album_info.confirm_message_ctrl, title, -1, 26, lv_color_hex(0xFFFFFF), LV_TEXT_ALIGN_CENTER,
        0, 23, 640, 44);
    divoom_lvgl_message_box_add_label_item(phandle->pselect_album_info.confirm_message_ctrl, desc, -1, 26, lv_color_hex(0xFFFFFF), LV_TEXT_ALIGN_CENTER,
        20, 90, 600, 180);

    divoom_lvgl_message_box_set_active_item(phandle->pselect_album_info.confirm_message_ctrl, 0);
}


static void divoom_lvgl_photo_slide_effect_select_album_btn_event_cb(lv_event_t * e)
{
    lv_obj_t *cur_obj;
    lv_group_t * cur_group_obj = lv_obj_get_group(lv_event_get_target_obj(e));
    Pdivoom_lvgl_photo_slide_effect_ctrl phandle = (Pdivoom_lvgl_photo_slide_effect_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target_obj(e);
    if(code == LV_EVENT_KEY)
    {
        char *key_code = ((char *)lv_event_get_param(e));
        Pdivoom_pic_album_manage_head_data palbum_list = divoom_pic_album_manage_get_all_album_info();

        switch(key_code[0])
        {            
            case DIVOOM_LVGL_KEY_NEXT:
            case DIVOOM_LVGL_KEY_PRE:
                cur_obj = divoom_menu_direct_ctrl_get(phandle->pselect_album_info.pdirect_menu_ctrl, cur_obj, key_code[0]==DIVOOM_LVGL_KEY_PRE?DIVOOM_MENU_DIRECT_LEFT:DIVOOM_MENU_DIRECT_RIGHT);                            
                lv_group_focus_obj(cur_obj);
                for(int i = 0; i < palbum_list->album_num; i++)
                {
                    if(cur_obj == phandle->pselect_album_info.select_album_btn_obj[i])
                    {
                        if(phandle->pselect_album_info.active_id != i)
                        {
                            phandle->pselect_album_info.active_id = i;
                        }
                        break;
                    }
                }
                break;
               
            case DIVOOM_LVGL_KEY_ESC:
                divoom_menu_control_lv_img_set_src(phandle->psetting_menu_info.back_obj, "14/14_set_bg1.png");
                divoom_menu_control_lv_img_set_src(phandle->psetting_menu_info.icon_list_obj[phandle->psetting_menu_info.active_id], "14/14_up_arrow.png");
                lv_obj_add_flag(phandle->pselect_album_info.select_album_cout_obj, LV_OBJ_FLAG_HIDDEN);            
                lv_group_focus_obj(phandle->psetting_menu_info.btn_list_obj[phandle->psetting_menu_info.active_id]);
                break;
               
            case DIVOOM_LVGL_KEY_ENTER:
                if(phandle->pselect_album_info.active_id != phandle->pselect_album_info.old_id)
                {
                    int pic_num;
                    if(palbum_list->palbum_list[phandle->pselect_album_info.active_id].type)
                    {
                        pic_num = divoom_database_manage_get_pic_num_from_album_table(0, 0);
                    }
                    else
                    {
                        pic_num = divoom_database_manage_get_pic_num_from_album_table(palbum_list->palbum_list[phandle->pselect_album_info.active_id].id, 0);
                    }
                    if(pic_num == 0)
                    {
                        divoom_lvgl_photo_slide_effect_select_album_btn_no_photo_dialog(phandle);
                        break;
                    }
                    divoom_lvgl_photo_slide_effect_start_new_album_name(phandle, palbum_list->palbum_list[phandle->pselect_album_info.active_id].id);  
                    lv_obj_add_style(phandle->pselect_album_info.select_album_btn_obj[phandle->pselect_album_info.old_id], &phandle->pselect_album_info.unactive_normal, 0);
                    lv_obj_add_style(phandle->pselect_album_info.select_album_btn_obj[phandle->pselect_album_info.active_id], &phandle->pselect_album_info.active_normal, 0);
                    if(!divoom_lvgl_photo_slide_effect_start_new_config(phandle, &phandle->update_slide_info)) break;
                    phandle->update_effect_flag = 0;        
                    phandle->pselect_album_info.old_id = phandle->pselect_album_info.active_id;
                }
                if(phandle->pcur_runing_slide && phandle->handle) phandle->pcur_runing_slide->pdisp(phandle->handle, NULL);                
                phandle->destory_top_btn_flag = 1;
                break;
               
            case DIVOOM_LVGL_KEY_HOME:
                break;
                           
        }

    }
}

void divoom_lvgl_photo_slide_effect_select_album_btn(Pdivoom_lvgl_photo_slide_effect_ctrl phandle)
{
    void *label_obj, *image_obj;    
    Pdivoom_pic_album_manage_head_data palbum_list = divoom_pic_album_manage_get_all_album_info();
    
    //后面这位置还要加图片
    image_obj = lv_img_create(phandle->top_btn_cout);
    lv_obj_set_pos(image_obj, 0, 0);
    lv_obj_set_size(image_obj, 800, 500);
    divoom_menu_control_lv_img_set_src(image_obj, "14/14_set_bg3.png");
    phandle->pselect_album_info.select_album_cout_obj = image_obj;
    phandle->pselect_album_info.pdirect_menu_ctrl = divoom_menu_direct_ctrl_create(3, ( palbum_list->album_num + 2)/3);   

    divoom_menu_control_init_btn_stype(&phandle->pselect_album_info.active_normal, NULL, 
        &phandle->pselect_album_info.unactive_normal, NULL, &phandle->pselect_album_info.press, NULL, 19);
    phandle->pselect_album_info.active_id = 0;
    for(int i = 0; i < palbum_list->album_num; i++)
    {
        phandle->pselect_album_info.select_album_btn_obj[i] = lv_btn_create(phandle->pselect_album_info.select_album_cout_obj);
        if(palbum_list->palbum_list[i].id == phandle->slide_info.album_id)
        {
            phandle->pselect_album_info.active_id = i;
            phandle->pselect_album_info.old_id = i;
            lv_obj_add_style(phandle->pselect_album_info.select_album_btn_obj[i], &phandle->pselect_album_info.active_normal, 0);
        }
        else
        {
            lv_obj_add_style(phandle->pselect_album_info.select_album_btn_obj[i], &phandle->pselect_album_info.unactive_normal, 0);
        }
        lv_obj_add_style(phandle->pselect_album_info.select_album_btn_obj[i], &phandle->pselect_album_info.press, LV_STATE_FOCUSED);
        lv_obj_add_style(phandle->pselect_album_info.select_album_btn_obj[i], &phandle->pselect_album_info.press, LV_STATE_FOCUS_KEY);
        divoom_menu_direct_ctrl_update(phandle->pselect_album_info.pdirect_menu_ctrl, phandle->pselect_album_info.select_album_btn_obj[i], (i%3), (i/3));
        lv_obj_set_pos(phandle->pselect_album_info.select_album_btn_obj[i], 44 + 240*(i%3), 194 + 90*(i/3));
    	lv_obj_set_size(phandle->pselect_album_info.select_album_btn_obj[i], 232, 82);
        
        label_obj = lv_label_create(phandle->pselect_album_info.select_album_btn_obj[i]);
        divoom_menu_control_disp_label_text(label_obj, divoom_pic_album_manage_get_static_album_name(palbum_list->palbum_list[i].id), 232, 82, 24, 0, LV_TEXT_ALIGN_CENTER, phandle->menu_id);
        lv_obj_center(label_obj);
        lv_obj_add_event_cb(phandle->pselect_album_info.select_album_btn_obj[i], divoom_lvgl_photo_slide_effect_select_album_btn_event_cb, LV_EVENT_KEY, phandle);
    }
    
    lv_obj_add_flag(phandle->pselect_album_info.select_album_cout_obj, LV_OBJ_FLAG_HIDDEN);
    
}


static void divoom_lvgl_photo_slide_effect_add_play_setting_btn_event_cb(lv_event_t * e)
{
    lv_obj_t *cur_obj;
    lv_group_t * cur_group_obj = lv_obj_get_group(lv_event_get_target_obj(e));
    Pdivoom_lvgl_photo_slide_effect_ctrl phandle = (Pdivoom_lvgl_photo_slide_effect_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target_obj(e);
    if(code == LV_EVENT_KEY)
    {
        char *key_code = ((char *)lv_event_get_param(e));

        
        switch(key_code[0])
        {            
            case DIVOOM_LVGL_KEY_NEXT:
            case DIVOOM_LVGL_KEY_PRE:
                for(int i = 0; i < 2; i++)
                {
                    if(cur_obj == phandle->play_setting_info.play_setting_btn_obj[i])
                    {
                        #if 1
                        if(phandle->play_setting_info.status && i == 1)
                        {
                            lv_obj_add_style(phandle->play_setting_info.play_setting_btn_obj[i], &phandle->play_setting_info.press, LV_STATE_FOCUSED);
                            lv_obj_add_style(phandle->play_setting_info.play_setting_btn_obj[i], &phandle->play_setting_info.press, LV_STATE_FOCUS_KEY);
                            phandle->play_setting_info.status = 0;
                        }
                        if(phandle->play_setting_info.status)
                        {
                            //只有速度才响应
                            if(i == 0)
                            {
                                int value = lv_slider_get_value(phandle->play_setting_info.play_speed_obj);
                                
                                if(key_code[0] == DIVOOM_LVGL_KEY_NEXT)
                                {
                                    value += 5;
                                    if(value > 100)
                                    {
                                        value = 100;
                                    }
                                }
                                else
                                {
                                    if(value > 5)
                                    {
                                        value -= 5;
                                    }
                                    else
                                    {
                                        value = 0;
                                    }
                                }
                                lv_slider_set_value(phandle->play_setting_info.play_speed_obj, value, 0);
                            }
                        }
                        else
                        #endif
                        {
                            if(phandle->play_setting_info.active_id == i)
                            {
                                phandle->play_setting_info.active_id = i?0:1;
                                cur_obj = divoom_menu_direct_ctrl_get(phandle->play_setting_info.pdirect_menu_ctrl, cur_obj, key_code[0]==DIVOOM_LVGL_KEY_PRE?DIVOOM_MENU_DIRECT_UP:DIVOOM_MENU_DIRECT_DOWN);                            
                                lv_group_focus_obj(cur_obj);
                            }
                        }
                        break;
                    }
                }
                break;
               
            case DIVOOM_LVGL_KEY_ESC:
                if(phandle->play_setting_info.status == 0)
                {
                    divoom_menu_control_lv_img_set_src(phandle->psetting_menu_info.back_obj, "14/14_set_bg1.png");
                    divoom_menu_control_lv_img_set_src(phandle->psetting_menu_info.icon_list_obj[phandle->psetting_menu_info.active_id], "14/14_up_arrow.png");
                    lv_group_focus_obj(phandle->psetting_menu_info.btn_list_obj[phandle->psetting_menu_info.active_id]);
                    lv_obj_add_flag(phandle->play_setting_info.play_setting_cout_obj, LV_OBJ_FLAG_HIDDEN);            
                }
                else
                {
                    phandle->play_setting_info.status = 0;
                    lv_obj_add_style(phandle->play_setting_info.play_setting_btn_obj[phandle->play_setting_info.active_id], &phandle->play_setting_info.press, LV_STATE_FOCUSED);
                    lv_obj_add_style(phandle->play_setting_info.play_setting_btn_obj[phandle->play_setting_info.active_id], &phandle->play_setting_info.press, LV_STATE_FOCUS_KEY);
                    if(phandle->play_setting_info.active_id == 0)
                    {
                        int play_time;
                        play_time = divoom_pic_phone_manage_convert_percent_to_slide_speed(lv_slider_get_value(cur_obj));
                        if(play_time != phandle->pslide_config->play_time)
                        {
                            phandle->pslide_config->play_time = play_time;
                            divoom_pic_phone_manage_save_slide_config(1);
                        }
                    }
                
                }
                break;
               
            case DIVOOM_LVGL_KEY_ENTER:
                //进入配置菜单                  
                if(phandle->play_setting_info.status == 0)
                {
                    phandle->play_setting_info.status = 1;
                    
                    lv_obj_add_style(phandle->play_setting_info.play_setting_btn_obj[phandle->play_setting_info.active_id], &phandle->play_setting_info.active_normal, LV_STATE_FOCUSED);
                    lv_obj_add_style(phandle->play_setting_info.play_setting_btn_obj[phandle->play_setting_info.active_id], &phandle->play_setting_info.active_normal, LV_STATE_FOCUS_KEY);
                }
                else
                {
                    if(phandle->play_setting_info.active_id == 0)
                    {
                        int value = lv_slider_get_value(phandle->play_setting_info.play_speed_obj);
                        value += 5;
                        
                        if(value >= 105)
                        {
                            value = 0;
                        }
                        else if(value  > 100)
                        {
                            value = 100;
                        }
                        lv_slider_set_value(phandle->play_setting_info.play_speed_obj, value, 0);
                    }
                    else
                    {
                        divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SLIDE_SETING_MENU, NULL, 0, 0);
                    }
                }
  
                break;
               
            case DIVOOM_LVGL_KEY_HOME:
                //进入浏览照片模式
                break;
                          
        }

    }
}

const char *pdivoom_lvgl_photo_slide_effect_paly_setting_items[] =
{
    "PlaySpeed",
    "SlideSetting"
};

void divoom_lvgl_photo_slide_effect_add_play_setting_btn(Pdivoom_lvgl_photo_slide_effect_ctrl phandle)
{
    int items;
    void *label_obj, *image_obj;    
    
    //后面这位置还要加图片
    image_obj = lv_img_create(phandle->top_btn_cout);
    lv_obj_set_pos(image_obj, 0, 0);
    lv_obj_set_size(image_obj, 800, 500);
    divoom_menu_control_lv_img_set_src(image_obj, "14/14_set_bg3.png");
    items = sizeof(pdivoom_lvgl_photo_slide_effect_paly_setting_items)/sizeof(pdivoom_lvgl_photo_slide_effect_paly_setting_items[0]);
    phandle->play_setting_info.play_setting_cout_obj = image_obj;
    phandle->play_setting_info.pdirect_menu_ctrl = divoom_menu_direct_ctrl_create(1, 2);   

    divoom_menu_control_init_btn_stype(&phandle->play_setting_info.press, NULL, 
        &phandle->play_setting_info.unactive_normal, NULL, &phandle->play_setting_info.active_normal, NULL, 19);
    phandle->play_setting_info.active_id = 0;
    phandle->play_setting_info.status = 0;
    for(int i = 0; i < items; i++)
    {
        phandle->play_setting_info.play_setting_btn_obj[i] = lv_btn_create(phandle->play_setting_info.play_setting_cout_obj);
        lv_obj_add_style(phandle->play_setting_info.play_setting_btn_obj[i], &phandle->play_setting_info.unactive_normal, 0);
        lv_obj_add_style(phandle->play_setting_info.play_setting_btn_obj[i], &phandle->play_setting_info.press, LV_STATE_FOCUSED);
        lv_obj_add_style(phandle->play_setting_info.play_setting_btn_obj[i], &phandle->play_setting_info.press, LV_STATE_FOCUS_KEY);
        divoom_menu_direct_ctrl_update(phandle->play_setting_info.pdirect_menu_ctrl, phandle->play_setting_info.play_setting_btn_obj[i], 0, i);
        lv_obj_set_pos(phandle->play_setting_info.play_setting_btn_obj[i], 40 , 216 + 142 *i);
    	lv_obj_set_size(phandle->play_setting_info.play_setting_btn_obj[i], 720, 98);
        
        label_obj = lv_label_create(phandle->play_setting_info.play_setting_btn_obj[i]);
        divoom_menu_control_disp_label_text(label_obj, divoom_menu_control_get_disp_str(pdivoom_lvgl_photo_slide_effect_paly_setting_items[i]), 240, 40, 24, 0, LV_TEXT_ALIGN_LEFT, phandle->menu_id);
        lv_obj_set_style_align(label_obj, LV_ALIGN_LEFT_MID, 0);
        lv_obj_add_event_cb(phandle->play_setting_info.play_setting_btn_obj[i], divoom_lvgl_photo_slide_effect_add_play_setting_btn_event_cb, LV_EVENT_KEY, phandle);
        if(i == 0)
        {            
            void *slide_obj = lv_slider_create(phandle->play_setting_info.play_setting_btn_obj[i]);
            lv_slider_set_range(slide_obj, 0, 100);
            lv_obj_set_pos(slide_obj, 308, 47);
            lv_obj_set_size(slide_obj, 319, 4);
            lv_slider_set_value(slide_obj, divoom_pic_phone_manage_convert_slide_speed_to_percent(phandle->pslide_config->play_time), LV_ANIM_ON);
            lv_obj_set_style_bg_color(slide_obj, lv_color_hex(DIVOOM_MENNU_SLIDER_ACTIVE_COLOR), LV_PART_INDICATOR);
            lv_obj_set_style_bg_color(slide_obj, lv_color_hex(DIVOOM_MENNU_SLIDER_UNACTIVE_COLOR), LV_PART_MAIN);
            phandle->play_setting_info.play_speed_obj = slide_obj;
    
        }
        else
        {
            image_obj = lv_img_create(phandle->play_setting_info.play_setting_btn_obj[i]);
            lv_obj_set_pos(image_obj, 650, 31);
            lv_obj_set_size(image_obj, 36, 36);
            divoom_menu_control_lv_img_set_src(image_obj, "14/14_more.png");
        }
    }
    
    lv_obj_add_flag(phandle->play_setting_info.play_setting_cout_obj, LV_OBJ_FLAG_HIDDEN);
    
    
}

typedef struct _divoom_lvgl_photo_slide_effect_name_item
{
    int effect_type;
    char *effect_name;
}divoom_lvgl_photo_slide_effect_name_item, *Pdivoom_lvgl_photo_slide_effect_name_item;


const divoom_lvgl_photo_slide_effect_name_item pdivoom_lvgl_photo_slide_effect_menu_items[] =
{
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_NORMAL, "SlideNormal"},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_PUSH, "SlidePush"},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_FADE_OUT, "SlideFade"},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_ROTATE, "SlideRotate"},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_PHOTO_STEAM, "SlidePhotoStream"},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE0, "Creative0"},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE1, "Creative1"},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE2, "Creative2"},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE3, "Creative3"},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE4, "Creative4"},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE5, "Creative5"},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE6, "Creative6"},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE7, "Creative7"},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE8, "Creative8"},
    {DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE9, "Creative9"},
};

static void divoom_lvgl_photo_slide_effect_add_effect_event_cb(lv_event_t * e)
{
    lv_obj_t *cur_obj;
    lv_group_t * cur_group_obj = lv_obj_get_group(lv_event_get_target_obj(e));
    Pdivoom_lvgl_photo_slide_effect_ctrl phandle = (Pdivoom_lvgl_photo_slide_effect_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target_obj(e);
    if(code == LV_EVENT_KEY)
    {
        int items;
        char *key_code = ((char *)lv_event_get_param(e));

        items = sizeof(pdivoom_lvgl_photo_slide_effect_menu_items)/sizeof(pdivoom_lvgl_photo_slide_effect_menu_items[0]);
        switch(key_code[0])
        {            
            case DIVOOM_LVGL_KEY_NEXT:
            case DIVOOM_LVGL_KEY_PRE:
                cur_obj = divoom_menu_direct_ctrl_get(phandle->peffect_menu_info.pdirect_menu_ctrl, cur_obj, key_code[0]==DIVOOM_LVGL_KEY_PRE?DIVOOM_MENU_DIRECT_LEFT:DIVOOM_MENU_DIRECT_RIGHT);                            
                lv_group_focus_obj(cur_obj);
                for(int i = 0; i < items; i++)
                {
                    if(cur_obj == phandle->peffect_menu_info.effect_btn_obj[i])
                    {
                        if(phandle->peffect_menu_info.active_id != i)
                        {
                            phandle->peffect_menu_info.active_id = i;
                        }
                        break;
                    }
                }
                break;
               
            case DIVOOM_LVGL_KEY_ESC:
                divoom_menu_control_lv_img_set_src(phandle->psetting_menu_info.back_obj, "14/14_set_bg1.png");
                lv_obj_add_flag(phandle->peffect_menu_info.effect_cout_obj, LV_OBJ_FLAG_HIDDEN);     
                divoom_menu_control_lv_img_set_src(phandle->psetting_menu_info.icon_list_obj[phandle->psetting_menu_info.active_id], "14/14_up_arrow.png");
                lv_group_focus_obj(phandle->psetting_menu_info.btn_list_obj[phandle->psetting_menu_info.active_id]); 
                break;
               
            case DIVOOM_LVGL_KEY_ENTER:
                if(phandle->peffect_menu_info.active_id != phandle->peffect_menu_info.old_id)
                {
                    lv_obj_add_style(phandle->peffect_menu_info.effect_btn_obj[phandle->peffect_menu_info.active_id], &phandle->peffect_menu_info.active_normal, 0);
                    lv_obj_add_style(phandle->peffect_menu_info.effect_btn_obj[phandle->peffect_menu_info.old_id], &phandle->peffect_menu_info.unactive_normal, 0);
                    phandle->pslide_config->effect_type = pdivoom_lvgl_photo_slide_effect_menu_items[phandle->peffect_menu_info.active_id].effect_type;
                    divoom_pic_phone_manage_save_slide_config(1);
                    phandle->peffect_menu_info.old_id = phandle->peffect_menu_info.active_id;
                    divoom_lvgl_photo_slide_effect_start_new_slide_type(phandle, phandle->pslide_config->effect_type);
                }
                if(phandle->pcur_runing_slide && phandle->handle) phandle->pcur_runing_slide->pdisp(phandle->handle, NULL);                
                phandle->destory_top_btn_flag = 1;
                break;
               
            case DIVOOM_LVGL_KEY_HOME:
                break;
                           
        }

    }
}


void divoom_lvgl_photo_slide_effect_add_effect_btn(Pdivoom_lvgl_photo_slide_effect_ctrl phandle)
{    
    int items;
    void *label_obj, *image_obj;    
    
    //后面这位置还要加图片
    image_obj = lv_img_create(phandle->top_btn_cout);
    lv_obj_set_pos(image_obj, 0, 0);
    lv_obj_set_size(image_obj, 800, 680);
    divoom_menu_control_lv_img_set_src(image_obj, "14/14_set_bg3.png");
    items = sizeof(pdivoom_lvgl_photo_slide_effect_menu_items)/sizeof(pdivoom_lvgl_photo_slide_effect_menu_items[0]);
    phandle->peffect_menu_info.effect_cout_obj = image_obj;
    phandle->peffect_menu_info.pdirect_menu_ctrl = divoom_menu_direct_ctrl_create(3, (items + 2)/3);   

    divoom_menu_control_init_btn_stype(&phandle->peffect_menu_info.active_normal, NULL, 
        &phandle->peffect_menu_info.unactive_normal, NULL, &phandle->peffect_menu_info.press, NULL, 19);
    phandle->peffect_menu_info.active_id = 0;
    for(int i = 0; i < items; i++)
    {
        phandle->peffect_menu_info.effect_btn_obj[i] = lv_btn_create(phandle->peffect_menu_info.effect_cout_obj);
        if(phandle->pslide_config->effect_type == pdivoom_lvgl_photo_slide_effect_menu_items[i].effect_type)
        {
            lv_obj_add_style(phandle->peffect_menu_info.effect_btn_obj[i], &phandle->peffect_menu_info.active_normal, 0);
            phandle->peffect_menu_info.active_id = i;
            phandle->peffect_menu_info.old_id = i;
        }
        else
        {
            lv_obj_add_style(phandle->peffect_menu_info.effect_btn_obj[i], &phandle->peffect_menu_info.unactive_normal, 0);
        }
        lv_obj_add_style(phandle->peffect_menu_info.effect_btn_obj[i], &phandle->peffect_menu_info.press, LV_STATE_FOCUSED);
        lv_obj_add_style(phandle->peffect_menu_info.effect_btn_obj[i], &phandle->peffect_menu_info.press, LV_STATE_FOCUS_KEY);
        divoom_menu_direct_ctrl_update(phandle->peffect_menu_info.pdirect_menu_ctrl, phandle->peffect_menu_info.effect_btn_obj[i], i%3, i/3);
        lv_obj_set_pos(phandle->peffect_menu_info.effect_btn_obj[i], 44 + 240*(i%3), 194 + 90 *(i/3));
    	lv_obj_set_size(phandle->peffect_menu_info.effect_btn_obj[i], 232, 82);
        
        lv_obj_t *label_obj = lv_label_create(phandle->peffect_menu_info.effect_btn_obj[i]);
        divoom_menu_control_disp_label_text(label_obj, divoom_menu_control_get_disp_str(pdivoom_lvgl_photo_slide_effect_menu_items[i].effect_name), 232, 82, 24, 0, LV_TEXT_ALIGN_CENTER, phandle->menu_id);
        lv_obj_center(label_obj);
        lv_obj_add_event_cb(phandle->peffect_menu_info.effect_btn_obj[i], divoom_lvgl_photo_slide_effect_add_effect_event_cb, LV_EVENT_KEY, phandle);
    }
    
    lv_obj_add_flag(phandle->peffect_menu_info.effect_cout_obj, LV_OBJ_FLAG_HIDDEN);
    
    
}
static const char *weather_name_list[DIVOOM_SLIDE_WEATHER_IMAGE_COUNT] =
{
    
    "14/SUNNY_DAY.png",    //sunny
    "14/CLOUDY_DAY.png",    //Cloudy
    "14/RAINY_DAY.png",    //Rainy
    "14/SNOW_DAY.png",    //snow
    "14/FROG_DAY.png",    //Frog
    "14/SUNNY_NIGHT.png",    //sunny
    "14/CLOUDY_NIGHT.png",    //Cloudy
    "14/RAINY_NIGHT.png",    //Rainy
    "14/SNOW_NIGHT.png",    //snow
    "14/FROG_NIGHT.png",    //Frog
};

static void divoom_lvgl_photo_slide_effect_release_weather_images(Pdivoom_lvgl_photo_slide_effect_ctrl phandle)
{
#if !DIVOOM_SLIDE_WEATHER_LVGL_USE_PNG
    int i;
#endif

    if(phandle == NULL)
    {
        return;
    }

    if(phandle->weather_disp_obj && lv_obj_is_valid(phandle->weather_disp_obj))
    {
        lv_obj_add_flag(phandle->weather_disp_obj, LV_OBJ_FLAG_HIDDEN);
#if DIVOOM_SLIDE_WEATHER_LVGL_USE_PNG
        if(lv_img_get_src(phandle->weather_disp_obj))
        {
            lv_img_cache_invalidate_src(lv_img_get_src(phandle->weather_disp_obj));
            lv_img_set_src(phandle->weather_disp_obj, NULL);
        }
#endif
    }
    phandle->weather_images_loaded = 0;
#if !DIVOOM_SLIDE_WEATHER_LVGL_USE_PNG
    for(i = 0; i < DIVOOM_SLIDE_WEATHER_IMAGE_COUNT; i++)
    {
        if(phandle->weather_img_dsc[i].data)
        {
            lv_img_cache_invalidate_src(&phandle->weather_img_dsc[i]);
        }
        divoom_aw_png_decode_buf_release(&phandle->weather_img_decode[i]);
        memset(&phandle->weather_img_dsc[i], 0, sizeof(phandle->weather_img_dsc[i]));
    }
#endif
}

static void divoom_lvgl_photo_slide_effect_load_weather_images(Pdivoom_lvgl_photo_slide_effect_ctrl phandle)
{
#if !DIVOOM_SLIDE_WEATHER_LVGL_USE_PNG
    char image_path[256];
    uint8 *image_buf;
    int image_w;
    int image_h;
    int i;
#endif

    if(phandle == NULL)
    {
        return;
    }

    divoom_lvgl_photo_slide_effect_release_weather_images(phandle);
#if !DIVOOM_SLIDE_WEATHER_LVGL_USE_PNG
    for(i = 0; i < DIVOOM_SLIDE_WEATHER_IMAGE_COUNT; i++)
    {
        image_w = 0;
        image_h = 0;
#if defined(DIVOOM_SIMULATOR) && DIVOOM_SIMULATOR
        divoom_menu_control_get_img_path(weather_name_list[i], image_path);
#else
        divoom_menu_control_get_img_path2(weather_name_list[i], image_path);
#endif
        image_buf = divoom_aw_decode_png_file_to_lvfmt_reuse(image_path,
            &image_w, &image_h, &phandle->weather_img_decode[i],
            LV_COLOR_FORMAT_RGB565A8);
        if(image_buf == NULL || image_w <= 0 || image_h <= 0)
        {
            divoom_print("weather image decode fail: %s\n", image_path);
            continue;
        }

        divoom_lv_img_dsc_init_fmt(&phandle->weather_img_dsc[i],
            (uint32)image_w, (uint32)image_h, LV_COLOR_FORMAT_RGB565A8,
            image_buf);
        if(phandle->weather_img_dsc[i].data_size == 0 ||
            phandle->weather_img_dsc[i].data_size > phandle->weather_img_decode[i].out_buf_size)
        {
            divoom_aw_png_decode_buf_release(&phandle->weather_img_decode[i]);
            memset(&phandle->weather_img_dsc[i], 0, sizeof(phandle->weather_img_dsc[i]));
            divoom_print("weather image size invalid: %s\n", image_path);
        }
    }
#endif
    phandle->weather_images_loaded = 1;
}

#if !defined(DIVOOM_SIMULATOR) || !(DIVOOM_SIMULATOR)
static void divoom_lvgl_photo_slide_effect_disable_video_osd(Pdivoom_lvgl_photo_slide_effect_ctrl phandle)
{
    if(phandle == NULL || phandle->video_osd_enabled == 0)
    {
        return;
    }

    divoom_menu_control_disable_video_ui_overlay();
    phandle->video_osd_enabled = 0;
    divoom_print("slide TOP ARGB1555 OSD disabled\n");
}

static void divoom_lvgl_photo_slide_effect_release_video_osd(Pdivoom_lvgl_photo_slide_effect_ctrl phandle)
{
    int i;

    if(phandle == NULL)
    {
        return;
    }
    divoom_lvgl_photo_slide_effect_disable_video_osd(phandle);
    for(i = 0; i < 2; i++)
    {
        if(phandle->video_osd_buf[i])
        {
            sunxifb_free(&phandle->video_osd_buf[i],
                i == 0 ? "slide_video_osd0" : "slide_video_osd1");
        }
    }
    phandle->video_osd_front_index = 0;
    phandle->video_osd_buf_size = 0;
}

static void divoom_lvgl_photo_slide_effect_update_video_osd(Pdivoom_lvgl_photo_slide_effect_ctrl phandle)
{
    lv_draw_buf_t draw_buf;
    lv_area_t coords;
    void *back_buf;
    void *phy_addr;
    uint32 need_size;
    uint32 image_w;
    uint32 image_h;
    uint32 y;
    uint8 back_index;
    int i;
    int ret;

    if(phandle == NULL || phandle->top_disp_obj == NULL ||
        divoom_local_movie_play_is_playing() == 0)
    {
        divoom_lvgl_photo_slide_effect_disable_video_osd(phandle);
        return;
    }

    lv_obj_update_layout(phandle->top_disp_obj);
    image_w = (uint32)lv_obj_get_width(phandle->top_disp_obj);
    image_h = (uint32)lv_obj_get_height(phandle->top_disp_obj);
    if(image_w == 0 || image_h == 0 || image_w > DIVOOM_SCREEN_WIDTH || image_h > 128U)
    {
        divoom_print("video top osd object size invalid: %ux%u\n", image_w, image_h);
        return;
    }
    need_size = LV_DRAW_BUF_SIZE(image_w, image_h, LV_COLOR_FORMAT_ARGB8888);
    if(phandle->video_osd_buf[0] == NULL || phandle->video_osd_buf[1] == NULL ||
        phandle->video_osd_buf_size < need_size)
    {
        divoom_lvgl_photo_slide_effect_release_video_osd(phandle);
        for(i = 0; i < 2; i++)
        {
            phandle->video_osd_buf[i] = sunxifb_alloc(need_size,
                i == 0 ? "slide_video_osd0" : "slide_video_osd1");
            if(phandle->video_osd_buf[i] == NULL)
            {
                divoom_print("video top osd buffer:%d alloc fail: %u\n", i, need_size);
                divoom_lvgl_photo_slide_effect_release_video_osd(phandle);
                return;
            }
        }
        phandle->video_osd_buf_size = need_size;
    }

    /* Never render into the buffer currently scanned by the display engine. */
    back_index = phandle->video_osd_enabled ?
        (uint8)(phandle->video_osd_front_index ^ 1U) : 0;
    back_buf = phandle->video_osd_buf[back_index];
    memset(back_buf, 0, phandle->video_osd_buf_size);
    ret = lv_draw_buf_init(&draw_buf, image_w, image_h,
        LV_COLOR_FORMAT_ARGB8888, LV_STRIDE_AUTO,
        back_buf, phandle->video_osd_buf_size);
    if(ret != LV_RES_OK)
    {
        divoom_print("video top osd draw buffer init fail: %d\n", ret);
        return;
    }
    ret = lv_snapshot_take_to_draw_buf(phandle->top_disp_obj,
        LV_COLOR_FORMAT_ARGB8888, &draw_buf);
    if(ret != LV_RES_OK)
    {
        divoom_print("video top osd snapshot fail: %d\n", ret);
        return;
    }

    image_w = draw_buf.header.w;
    image_h = draw_buf.header.h;
    for(y = 0; y < image_h; y++)
    {
        lv_color32_t *src = (lv_color32_t *)((uint8_t *)draw_buf.data +
            y * draw_buf.header.stride);
        uint16_t *dst = (uint16_t *)draw_buf.data + y * image_w;
        uint32 x;

        for(x = 0; x < image_w; x++)
        {
            lv_color32_t pixel = src[x];
            dst[x] = (pixel.alpha >= 0x80U ? 0x8000U : 0U) |
                ((uint16_t)(pixel.red >> 3) << 10) |
                ((uint16_t)(pixel.green >> 3) << 5) |
                (uint16_t)(pixel.blue >> 3);
        }
    }

    lv_obj_get_coords(phandle->top_disp_obj, &coords);
    phy_addr = sunxifb_mem_get_phyaddr(draw_buf.data);
    if(image_w == 0 || image_h == 0 || phy_addr == NULL)
    {
        divoom_print("video top osd invalid: %ux%u phy:%p\n",
            image_w, image_h, phy_addr);
        return;
    }

    sunxifb_flush_cache(draw_buf.data,
        image_w * image_h * (uint32)sizeof(uint16_t));
    ret = divoom_menu_control_set_video_ui_overlay_argb1555(phy_addr,
        coords.x1, coords.y1, image_w, image_h);
    if(ret != 0)
    {
        divoom_print("slide TOP ARGB1555 OSD layer update fail: %d\n", ret);
        return;
    }
    phandle->video_osd_front_index = back_index;
    phandle->video_osd_enabled = 1;
    divoom_print("slide TOP ARGB1555 OSD updated: %ux%u at %d,%d index:%u\n",
        image_w, image_h, coords.x1, coords.y1, back_index);
}
#else
static void divoom_lvgl_photo_slide_effect_release_video_osd(Pdivoom_lvgl_photo_slide_effect_ctrl phandle)
{
    (void)phandle;
}

static void divoom_lvgl_photo_slide_effect_update_video_osd(Pdivoom_lvgl_photo_slide_effect_ctrl phandle)
{
    (void)phandle;
}
#endif

static void divoom_lvgl_photo_slide_effect_init_top_plate_style(lv_style_t *style)
{
    lv_style_init(style);
    lv_style_set_bg_color(style, lv_color_black());
    lv_style_set_bg_opa(style, DIVOOM_SLIDE_TOP_PLATE_OPA);
    lv_style_set_radius(style, DIVOOM_SLIDE_TOP_PLATE_RADIUS);
    lv_style_set_border_width(style, 0);
    lv_style_set_pad_all(style, 0);
}

static lv_coord_t divoom_lvgl_photo_slide_effect_measure_temp_w(lv_obj_t *label)
{
    const char *text;
    const lv_font_t *font;
    lv_point_t txt_size;

    if (!label)
    {
        return DIVOOM_SLIDE_TOP_TEMP_W;
    }
    text = lv_label_get_text(label);
    font = lv_obj_get_style_text_font(label, 0);
    if (!text || !font)
    {
        return DIVOOM_SLIDE_TOP_TEMP_W;
    }
    lv_txt_get_size(&txt_size, text, font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    if (txt_size.x <= 0)
    {
        return DIVOOM_SLIDE_TOP_TEMP_W;
    }
    return txt_size.x;
}

static int divoom_lvgl_photo_slide_effect_layout_top_disp(Pdivoom_lvgl_photo_slide_effect_ctrl phandle)
{
    int has_clock;
    int has_weather;
    lv_coord_t stack_w;
    lv_coord_t stack_h;
    lv_coord_t old_w;
    lv_coord_t old_h;
    int layout_changed;
    lv_coord_t weather_row_w;
    lv_coord_t weather_row_h;
    lv_coord_t content_w;
    lv_coord_t temp_w;
    lv_coord_t y;
    lv_coord_t row_x;

    has_clock = phandle->pslide_config->clock_flag;
    has_weather = phandle->pslide_config->weather_flag;
    temp_w = has_weather ? divoom_lvgl_photo_slide_effect_measure_temp_w(phandle->temp_disp_obj)
        : DIVOOM_SLIDE_TOP_TEMP_W;
    weather_row_h = DIVOOM_SLIDE_TOP_WEATHER_PAD_V * 2 + DIVOOM_SLIDE_TOP_WEATHER_ICON;
    if (has_weather && has_clock)
    {
        /* albumWidgetWeatherRow：icon + gap(6) + temp，共享背板内不再额外左右 padding */
        weather_row_w = DIVOOM_SLIDE_TOP_WEATHER_ICON + DIVOOM_SLIDE_TOP_WEATHER_GAP + temp_w;
    }
    else if (has_weather)
    {
        weather_row_w = DIVOOM_SLIDE_TOP_WEATHER_PAD_H * 2 + DIVOOM_SLIDE_TOP_WEATHER_ICON
            + DIVOOM_SLIDE_TOP_WEATHER_GAP + temp_w;
    }
    else
    {
        weather_row_w = 0;
    }

    if (!has_clock && !has_weather)
    {
        return 0;
    }

    old_w = lv_obj_get_width(phandle->top_disp_obj);
    old_h = lv_obj_get_height(phandle->top_disp_obj);

    if (has_clock && has_weather)
    {
        content_w = (weather_row_w > DIVOOM_SLIDE_TOP_TIME_W) ? weather_row_w : DIVOOM_SLIDE_TOP_TIME_W;
        stack_w = content_w + DIVOOM_SLIDE_TOP_PLATE_PAD_H * 2;
        stack_h = DIVOOM_SLIDE_TOP_PLATE_PAD_V * 2 + DIVOOM_SLIDE_TOP_LINE_H
            + DIVOOM_SLIDE_TOP_STACK_GAP + weather_row_h;
    }
    else if (has_clock)
    {
        content_w = DIVOOM_SLIDE_TOP_CLOCK_ONLY_W;
        stack_w = content_w;
        stack_h = DIVOOM_SLIDE_TOP_CLOCK_ONLY_H;
    }
    else
    {
        content_w = weather_row_w;
        stack_w = weather_row_w;
        stack_h = weather_row_h;
    }

    lv_obj_set_size(phandle->top_disp_obj, stack_w, stack_h);
    lv_obj_set_pos(phandle->top_disp_obj,
        DIVOOM_SCREEN_WIDTH - stack_w - DIVOOM_SLIDE_TOP_WIDGET_MARGIN,
        DIVOOM_SLIDE_TOP_WIDGET_MARGIN);

    if (phandle->top_plate_obj)
    {
        if (has_clock)
        {
            lv_obj_clear_flag(phandle->top_plate_obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_size(phandle->top_plate_obj, stack_w, stack_h);
            lv_obj_set_pos(phandle->top_plate_obj, 0, 0);
            lv_obj_move_background(phandle->top_plate_obj);
        }
        else
        {
            lv_obj_add_flag(phandle->top_plate_obj, LV_OBJ_FLAG_HIDDEN);
        }
    }

    y = has_clock ? (has_weather ? DIVOOM_SLIDE_TOP_PLATE_PAD_V
        : (stack_h - DIVOOM_SLIDE_TOP_LINE_H) / 2 + DIVOOM_SLIDE_TOP_CLOCK_OFFSET_Y) : 0;

    if (has_clock)
    {
        if (has_weather)
        {
            lv_obj_set_width(phandle->time_disp_obj, DIVOOM_SLIDE_TOP_TIME_W);
            lv_obj_set_style_text_align(phandle->time_disp_obj, LV_TEXT_ALIGN_RIGHT, 0);
            lv_obj_set_x(phandle->time_disp_obj,
                stack_w - DIVOOM_SLIDE_TOP_PLATE_PAD_H - DIVOOM_SLIDE_TOP_TIME_W);
        }
        else
        {
            /* 单时间模式使用整块背板作为文本槽，避免时间贴近右侧。 */
            lv_obj_set_width(phandle->time_disp_obj, stack_w);
            lv_obj_set_style_text_align(phandle->time_disp_obj, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_set_x(phandle->time_disp_obj, 0);
        }
        lv_obj_set_height(phandle->time_disp_obj, DIVOOM_SLIDE_TOP_LINE_H);
        lv_obj_set_y(phandle->time_disp_obj, y);
        y += DIVOOM_SLIDE_TOP_LINE_H;
        if (has_weather)
        {
            y += DIVOOM_SLIDE_TOP_STACK_GAP;
        }
    }

    if (has_weather)
    {
        lv_coord_t temp_y;
        lv_coord_t temp_x;
        lv_coord_t icon_x;

        temp_y = y + DIVOOM_SLIDE_TOP_WEATHER_PAD_V
            + (DIVOOM_SLIDE_TOP_WEATHER_ICON - DIVOOM_SLIDE_TOP_LINE_H) / 2
            + DIVOOM_SLIDE_TOP_TEMP_OFFSET_Y;

        if (has_clock)
        {
            lv_coord_t content_right;

            /* albumWidgetStackContent align-items:flex-end — 天气行右缘与时间右缘对齐 */
            content_right = stack_w - DIVOOM_SLIDE_TOP_PLATE_PAD_H;
            temp_x = content_right - temp_w;
            icon_x = temp_x - DIVOOM_SLIDE_TOP_WEATHER_GAP - DIVOOM_SLIDE_TOP_WEATHER_ICON;
            row_x = icon_x;
        }
        else
        {
            row_x = 0;
            icon_x = row_x + DIVOOM_SLIDE_TOP_WEATHER_PAD_H;
            temp_x = icon_x + DIVOOM_SLIDE_TOP_WEATHER_ICON + DIVOOM_SLIDE_TOP_WEATHER_GAP;
        }

        if (phandle->weather_plate_obj)
        {
            if (!has_clock)
            {
                lv_obj_clear_flag(phandle->weather_plate_obj, LV_OBJ_FLAG_HIDDEN);
                lv_obj_set_size(phandle->weather_plate_obj, weather_row_w, weather_row_h);
                lv_obj_set_pos(phandle->weather_plate_obj, row_x, y);
                lv_obj_move_background(phandle->weather_plate_obj);
            }
            else
            {
                lv_obj_add_flag(phandle->weather_plate_obj, LV_OBJ_FLAG_HIDDEN);
            }
        }

        lv_obj_set_pos(phandle->weather_disp_obj, icon_x, y + DIVOOM_SLIDE_TOP_WEATHER_PAD_V);
        lv_obj_set_size(phandle->weather_disp_obj,
            DIVOOM_SLIDE_TOP_WEATHER_ICON, DIVOOM_SLIDE_TOP_WEATHER_ICON);

        lv_obj_set_width(phandle->temp_disp_obj, temp_w);
        lv_obj_set_height(phandle->temp_disp_obj, DIVOOM_SLIDE_TOP_LINE_H);
        lv_obj_set_pos(phandle->temp_disp_obj, temp_x, temp_y);
        lv_obj_set_style_text_align(phandle->temp_disp_obj, LV_TEXT_ALIGN_LEFT, 0);
    }

    layout_changed = (old_w != stack_w) || (old_h != stack_h);
    return layout_changed;
}

void divoom_lvgl_photo_slide_effect_update_top_disp(Pdivoom_lvgl_photo_slide_effect_ctrl phandle)
{
    time_t cur_time;
	struct tm time_now; 
    char disp_buf[64];
    divoom_disp_weather_info weather_info;
    int was_hidden;
    int layout_changed;
    
    
    if(phandle->pslide_config->clock_flag == 0 && phandle->pslide_config->weather_flag == 0)
    {
        lv_obj_add_flag(phandle->top_disp_obj, LV_OBJ_FLAG_HIDDEN);
        divoom_lvgl_photo_slide_effect_release_video_osd(phandle);
        return;
    }
    was_hidden = lv_obj_has_flag(phandle->top_disp_obj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(phandle->top_disp_obj, LV_OBJ_FLAG_HIDDEN);
    
    
    divoom_main_get_weather_info(&weather_info);
    
    cur_time = time(0);
	localtime_r(&cur_time, &time_now);
    
    if(phandle->pslide_config->clock_flag == 0)
    {
        lv_obj_add_flag(phandle->time_disp_obj, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_clear_flag(phandle->time_disp_obj, LV_OBJ_FLAG_HIDDEN);
        divoom_astrootoo_time_pick_format_clock_label(disp_buf, (int)sizeof(disp_buf),
            time_now.tm_hour, time_now.tm_min);
        lv_label_set_text(phandle->time_disp_obj, disp_buf);
    }
    
    
    if(phandle->pslide_config->weather_flag == 0)
    {
        lv_obj_add_flag(phandle->weather_disp_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(phandle->temp_disp_obj, LV_OBJ_FLAG_HIDDEN);
        if (phandle->weather_plate_obj)
        {
            lv_obj_add_flag(phandle->weather_plate_obj, LV_OBJ_FLAG_HIDDEN);
        }
    }
    else
    {
        if(divoom_main_get_temp_flag() == 0)
        {
            sprintf(disp_buf, "%02d°", DIVOOM_WEATHER_ROUND_TEMP(weather_info.temp));
        }
        else
        {
            sprintf(disp_buf, "%02dF", DIVOOM_WEATHER_ROUND_TEMP(weather_info.temp *18/10 + 32));
        }
        lv_label_set_text(phandle->temp_disp_obj,disp_buf);
            
        
#if DIVOOM_SLIDE_WEATHER_LVGL_USE_PNG
        if(weather_info.weather_pic_id >= 0 &&
            weather_info.weather_pic_id < DIVOOM_SLIDE_WEATHER_IMAGE_COUNT)
        {
            divoom_menu_control_lv_img_set_src(phandle->weather_disp_obj,
                weather_name_list[weather_info.weather_pic_id]);
            lv_obj_clear_flag(phandle->weather_disp_obj, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(phandle->weather_disp_obj, LV_OBJ_FLAG_HIDDEN);
        }
#else
        if(weather_info.weather_pic_id >= 0 &&
            weather_info.weather_pic_id < DIVOOM_SLIDE_WEATHER_IMAGE_COUNT &&
            phandle->weather_img_dsc[weather_info.weather_pic_id].data)
        {
            lv_img_set_src(phandle->weather_disp_obj,
                &phandle->weather_img_dsc[weather_info.weather_pic_id]);
            lv_obj_clear_flag(phandle->weather_disp_obj, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(phandle->weather_disp_obj, LV_OBJ_FLAG_HIDDEN);
        }
#endif
        lv_obj_clear_flag(phandle->temp_disp_obj, LV_OBJ_FLAG_HIDDEN);
    }

    layout_changed = divoom_lvgl_photo_slide_effect_layout_top_disp(phandle);
    if (was_hidden || layout_changed)
    {
        divoom_lvgl_layer_top_raise(phandle->top_disp_obj);
    }
    divoom_lvgl_photo_slide_effect_update_video_osd(phandle);
}

static void divoom_lvgl_photo_slide_effect_apply_title_stack_style(lv_obj_t *stack)
{
    lv_obj_set_style_bg_opa(stack, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(stack, 0, 0);
    lv_obj_set_style_pad_all(stack, 0, 0);
    lv_obj_clear_flag(stack, LV_OBJ_FLAG_SCROLLABLE);
}

static void divoom_lvgl_photo_slide_effect_apply_title_plate_style(lv_obj_t *plate)
{
    lv_obj_set_style_bg_color(plate, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(plate, DIVOOM_SLIDE_TITLE_PLATE_OPA, 0);
    lv_obj_set_style_radius(plate, DIVOOM_SLIDE_TITLE_RADIUS, 0);
    lv_obj_set_style_border_width(plate, 0, 0);
    lv_obj_set_style_pad_all(plate, 0, 0);
    lv_obj_clear_flag(plate, LV_OBJ_FLAG_SCROLLABLE);
}

static void divoom_lvgl_photo_slide_effect_layout_title_disp(lv_obj_t *label)
{
    lv_obj_t *stack;
    lv_obj_t *plate;
    const lv_font_t *font;
    lv_point_t txt_size;
    lv_coord_t content_w;
    lv_coord_t stack_h;
    lv_coord_t line_h;
    const char *text;

    stack = lv_obj_get_parent(label);
    if (stack == NULL)
    {
        return;
    }
    plate = lv_obj_get_child(stack, 0);
    content_w = DIVOOM_SLIDE_TITLE_WIDTH - DIVOOM_SLIDE_TITLE_PAD_H * 2;
    lv_obj_set_width(label, content_w);
    text = lv_label_get_text(label);
    font = lv_obj_get_style_text_font(label, 0);
    if (font == NULL || text == NULL)
    {
        return;
    }
    lv_txt_get_size(&txt_size, text, font, 0, 0, content_w, LV_TEXT_FLAG_NONE);
    line_h = lv_font_get_line_height(font);
    stack_h = txt_size.y + DIVOOM_SLIDE_TITLE_PAD_V * 2;
    if (stack_h < DIVOOM_SLIDE_TITLE_PAD_V * 2 + line_h)
    {
        stack_h = DIVOOM_SLIDE_TITLE_PAD_V * 2 + line_h;
    }
    lv_obj_set_height(label, txt_size.y);
    lv_obj_set_pos(label, DIVOOM_SLIDE_TITLE_PAD_H, DIVOOM_SLIDE_TITLE_PAD_V);
    lv_obj_set_size(stack, DIVOOM_SLIDE_TITLE_WIDTH, stack_h);
    lv_obj_set_pos(stack, DIVOOM_SLIDE_TITLE_MARGIN,
        DIVOOM_SCREEN_HIGH - stack_h - DIVOOM_SLIDE_TITLE_MARGIN);
    if (plate)
    {
        lv_obj_set_size(plate, DIVOOM_SLIDE_TITLE_WIDTH, stack_h);
        lv_obj_set_pos(plate, 0, 0);
        lv_obj_move_background(plate);
    }
}

void *divoom_lvgl_photo_slide_effect_create_title_disp(void *main_obj, char *disp_title, int menu_id)
{
    Pdivoom_pic_phone_manage_slide_config pslide_config;
    lv_obj_t *stack;
    lv_obj_t *plate;
    lv_obj_t *label;
    lv_obj_t *parent;
    
    pslide_config = divoom_pic_phone_manage_get_slide_config();
    parent = main_obj ? main_obj : lv_layer_top();

    stack = lv_obj_create(parent);
    divoom_lvgl_photo_slide_effect_apply_title_stack_style(stack);
    lv_obj_set_width(stack, DIVOOM_SLIDE_TITLE_WIDTH);

    plate = lv_obj_create(stack);
    divoom_lvgl_photo_slide_effect_apply_title_plate_style(plate);

    label = lv_label_create(stack);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(DIVOOM_SLIDE_TOP_TEXT_COLOR), 0);
    lv_obj_set_style_text_font(label, divoom_menu_control_get_font_info(DIVOOM_SLIDE_TITLE_FONT, menu_id), 0);
    lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(label, 0, 0);
    lv_label_set_text(label, disp_title);
    divoom_lvgl_photo_slide_effect_layout_title_disp(label);
    divoom_lvgl_layer_top_raise(stack);
    
    if(pslide_config->title_flag == 0 || strlen(disp_title) == 0)
    {
        lv_obj_add_flag(stack, LV_OBJ_FLAG_HIDDEN);
    }
    return label;
}

void divoom_lvgl_photo_slide_effect_update_title_disp(void *title_obj, char *disp_title)
{
    Pdivoom_pic_phone_manage_slide_config pslide_config;
    lv_obj_t *label;
    lv_obj_t *stack;
    
    pslide_config = divoom_pic_phone_manage_get_slide_config();
    label = title_obj;
    stack = lv_obj_get_parent(label);
    if(pslide_config->title_flag == 0 || strlen(disp_title) == 0)
    {
        if (stack)
        {
            lv_obj_add_flag(stack, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);
        }
        return;
    }
    if (stack)
    {
        lv_obj_clear_flag(stack, LV_OBJ_FLAG_HIDDEN);
    }
    lv_label_set_text(label, disp_title);
    divoom_lvgl_photo_slide_effect_layout_title_disp(label);
}

void divoom_lvgl_photo_slide_effect_create_top_disp(Pdivoom_lvgl_photo_slide_effect_ctrl phandle)
{
    lv_style_init(&phandle->top_disp_style);
    lv_style_set_bg_opa(&phandle->top_disp_style, LV_OPA_TRANSP);
    lv_style_set_pad_all(&phandle->top_disp_style, 0);
    lv_style_set_pad_gap(&phandle->top_disp_style, 0);
    lv_style_set_border_width(&phandle->top_disp_style, 0);
    lv_style_set_border_side(&phandle->top_disp_style, 0);
    lv_style_set_outline_width(&phandle->top_disp_style, 0);
    lv_style_set_outline_pad(&phandle->top_disp_style, 0);

    divoom_lvgl_photo_slide_effect_init_top_plate_style(&phandle->top_plate_style);
    
    phandle->top_disp_obj = lv_obj_create(lv_layer_top());
    lv_obj_add_style(phandle->top_disp_obj, &phandle->top_disp_style, 0);
    lv_obj_clear_flag(phandle->top_disp_obj, LV_OBJ_FLAG_SCROLLABLE);
    /* Do not trigger weather PNG decode while the video pipeline is starting.
     * start_timer_cb shows and updates this overlay after video startup. */
    lv_obj_add_flag(phandle->top_disp_obj, LV_OBJ_FLAG_HIDDEN);

    phandle->top_plate_obj = lv_obj_create(phandle->top_disp_obj);
    lv_obj_add_style(phandle->top_plate_obj, &phandle->top_plate_style, 0);
    lv_obj_clear_flag(phandle->top_plate_obj, LV_OBJ_FLAG_SCROLLABLE);

    phandle->weather_plate_obj = lv_obj_create(phandle->top_disp_obj);
    lv_obj_add_style(phandle->weather_plate_obj, &phandle->top_plate_style, 0);
    lv_obj_clear_flag(phandle->weather_plate_obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(phandle->weather_plate_obj, LV_OBJ_FLAG_HIDDEN);
    
    phandle->time_disp_obj = lv_label_create(phandle->top_disp_obj);
    lv_obj_set_style_text_align(phandle->time_disp_obj, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_font(phandle->time_disp_obj, divoom_menu_control_get_font_info(DIVOOM_SLIDE_TOP_FONT, phandle->menu_id), 0);
    lv_obj_set_style_text_color(phandle->time_disp_obj, lv_color_hex(DIVOOM_SLIDE_TOP_TEXT_COLOR), 0);
    lv_label_set_text(phandle->time_disp_obj, " ");
    

    phandle->weather_disp_obj = lv_img_create(phandle->top_disp_obj);
    lv_obj_add_flag(phandle->weather_disp_obj, LV_OBJ_FLAG_HIDDEN);
    

    phandle->temp_disp_obj = lv_label_create(phandle->top_disp_obj);
    lv_obj_set_style_text_align(phandle->temp_disp_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(phandle->temp_disp_obj, divoom_menu_control_get_font_info(DIVOOM_SLIDE_TOP_FONT, phandle->menu_id), 0);
    lv_obj_set_style_text_color(phandle->temp_disp_obj, lv_color_hex(DIVOOM_SLIDE_TOP_TEXT_COLOR), 0);
    lv_label_set_text(phandle->temp_disp_obj, " ");
}

void divoom_lvgl_photo_slide_effect_create_btn(Pdivoom_lvgl_photo_slide_effect_ctrl phandle)
{
    
    phandle->top_btn_cout = lv_obj_create(lv_layer_top());
    lv_style_init(&phandle->top_menu_style);
    lv_style_set_bg_color(&phandle->top_menu_style, lv_color_black());
    lv_style_set_pad_all(&phandle->top_menu_style, 0);
    lv_style_set_pad_gap(&phandle->top_menu_style, 0);
    lv_style_set_border_width(&phandle->top_menu_style, 0);
    lv_style_set_border_side(&phandle->top_menu_style, 0);
    lv_style_set_outline_width(&phandle->top_menu_style, 0);
    lv_style_set_outline_pad(&phandle->top_menu_style, 0);
    lv_style_set_radius(&phandle->top_menu_style, 5);    
    //lv_style_set_opa(&phandle->top_disp_style, LV_OPA_40);  
    lv_style_set_bg_opa(&phandle->top_menu_style, LV_OPA_TRANSP);    
    
    lv_obj_add_style(phandle->top_btn_cout, &phandle->top_menu_style, 0);
    lv_obj_set_pos(phandle->top_btn_cout, 0, 0);
    lv_obj_set_size(phandle->top_btn_cout, 800, 680);
    divoom_lvgl_photo_slide_effect_add_setting_btn(phandle);    
    divoom_lvgl_photo_slide_effect_add_effect_btn(phandle);    
    divoom_lvgl_photo_slide_effect_select_album_btn(phandle);
    divoom_lvgl_photo_slide_effect_add_play_setting_btn(phandle);
    
    lv_obj_add_flag(phandle->top_disp_obj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(phandle->psetting_menu_info.back_obj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_opa(phandle->psetting_menu_info.back_obj, LV_OPA_COVER, 0);
    lv_obj_add_flag(phandle->peffect_menu_info.effect_cout_obj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(phandle->play_setting_info.play_setting_cout_obj, LV_OBJ_FLAG_HIDDEN);
    lv_group_focus_obj(phandle->psetting_menu_info.btn_list_obj[0]);
    divoom_menu_control_lv_img_set_src(phandle->psetting_menu_info.icon_list_obj[0], "14/14_up_arrow.png");
    lv_obj_clear_flag(phandle->psetting_menu_info.icon_list_obj[0], LV_OBJ_FLAG_HIDDEN);
    divoom_menu_control_lv_img_set_src(phandle->psetting_menu_info.icon_list_obj[1], "14/14_down_arrow.png");
    lv_obj_add_flag(phandle->psetting_menu_info.icon_list_obj[1], LV_OBJ_FLAG_HIDDEN);
    divoom_menu_control_lv_img_set_src(phandle->psetting_menu_info.icon_list_obj[2], "14/14_down_arrow.png");
    lv_obj_add_flag(phandle->psetting_menu_info.icon_list_obj[2], LV_OBJ_FLAG_HIDDEN);
    phandle->psetting_menu_info.active_id = 0;
    phandle->psetting_menu_info.old_id = 0;
}

void divoom_lvgl_photo_slide_effect_destory_btn(Pdivoom_lvgl_photo_slide_effect_ctrl phandle)
{
    if (phandle == NULL || phandle->top_btn_cout == NULL)
    {
        return;
    }
    lv_obj_del(phandle->top_btn_cout);
    phandle->top_btn_cout = NULL;
    if (phandle->pcur_runing_slide && phandle->pcur_runing_slide->pdisp)
    {
        if(phandle->pcur_runing_slide && phandle->handle) phandle->pcur_runing_slide->pdisp(phandle->handle, NULL);
    }
    divoom_lvgl_photo_slide_effect_show_overlays(phandle);
    if (phandle->main_obj)
    {
        lv_group_focus_obj(phandle->main_obj);
    }
    return;
    if(phandle->top_btn_cout)
    {
        lv_style_reset(&phandle->psetting_menu_info.active_normal);
        lv_style_reset(&phandle->psetting_menu_info.press);
        lv_style_reset(&phandle->peffect_menu_info.active_normal);
        lv_style_reset(&phandle->peffect_menu_info.unactive_normal);
        lv_style_reset(&phandle->peffect_menu_info.press);
        lv_style_reset(&phandle->pselect_album_info.active_normal);
        lv_style_reset(&phandle->pselect_album_info.unactive_normal);
        lv_style_reset(&phandle->pselect_album_info.press);
        lv_style_reset(&phandle->play_setting_info.active_normal);
        lv_style_reset(&phandle->play_setting_info.unactive_normal);
        lv_style_reset(&phandle->play_setting_info.press);
        divoom_menu_direct_ctrl_destory(phandle->peffect_menu_info.pdirect_menu_ctrl);
        divoom_menu_direct_ctrl_destory(phandle->psetting_menu_info.pdirect_menu_ctrl);
        divoom_menu_direct_ctrl_destory(phandle->pselect_album_info.pdirect_menu_ctrl);
        divoom_menu_direct_ctrl_destory(phandle->play_setting_info.pdirect_menu_ctrl);
        lv_style_reset(&phandle->top_menu_style);
        lv_group_focus_obj(phandle->main_obj);
        lv_obj_del(phandle->top_btn_cout);
        phandle->top_btn_cout = NULL; 
        if(phandle->pcur_runing_slide && phandle->handle) phandle->pcur_runing_slide->pdisp(phandle->handle, NULL);         
    }
}

void *divoom_lvgl_photo_slide_effect_init(void *user_data, int menu_id)
{
    Pdivoom_lvgl_photo_slide_effect_para album_info = (Pdivoom_lvgl_photo_slide_effect_para)user_data;
    Pdivoom_lvgl_photo_slide_effect_ctrl phandle;
    Pdivoom_pic_phone_manage_slide_config pslide_config;

    divoom_pic_album_manage_get_refresh_album_info();// 清除相簿变化标记
    pslide_config = divoom_pic_phone_manage_get_slide_config();


    phandle = divoom_system_malloc(sizeof(divoom_lvgl_photo_slide_effect_ctrl));
    if(phandle == NULL) { divoom_system_free(user_data); return NULL; }
    memset(phandle, 0, sizeof(*phandle));
    phandle->menu_id = menu_id;
    phandle->pslide_config = pslide_config;
    phandle->first_run_flag = 1;
    phandle->pre_menu_event = -1;
    phandle->slide_info.slide_type = -1;
    phandle->pcur_runing_slide = NULL;
    phandle->start_timer = NULL;
    phandle->puase_time = 0;
    phandle->lv_snapshot_obj = lv_img_create(NULL);    //只是为了修改菜单激活页面            
    if(phandle->lv_snapshot_obj == NULL) goto init_failed;
    phandle->main_obj = phandle->lv_snapshot_obj;
    lv_obj_set_size(phandle->lv_snapshot_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_set_style_bg_color(phandle->lv_snapshot_obj, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(phandle->lv_snapshot_obj, LV_OPA_COVER, 0);
    if(album_info == NULL)
    {
        
        album_info = divoom_system_malloc(sizeof(divoom_lvgl_photo_slide_effect_para));

        user_data = album_info;
        if(album_info == NULL) goto init_failed;
        album_info->album_id = pslide_config->album_id;
        album_info->slide_type = pslide_config->cur_play_effect_type;
    }
    
    if(album_info->album_id == 0)
    {
        album_info->album_id = divoom_pic_album_manage_get_album_id_from_type(DIVOOM_PIC_ALBUM_TYPE_ALL);
    }
    album_info->album_id = divoom_pic_album_manage_check_album_id(album_info->album_id);            
    /* Weather PNGs use the same global hardware decoder as every other
     * system and LVGL PNG request. */
    divoom_lvgl_photo_slide_effect_load_weather_images(phandle);
    memcpy(&phandle->update_slide_info, album_info, sizeof(phandle->update_slide_info));
    phandle->video_volumn = phandle->pslide_config->video_volume;
    phandle->refresh_timer = lv_timer_create(divoom_lvgl_photo_slide_effect_refresh_slide_type_time_handle, 16, phandle);
    if(phandle->refresh_timer == NULL) goto init_failed;
    lv_timer_pause(phandle->refresh_timer);

    divoom_lvgl_photo_slide_effect_create_top_disp(phandle);
    phandle->title_label_obj = divoom_lvgl_photo_slide_effect_create_title_disp(NULL, "", phandle->menu_id);
    divoom_lvgl_photo_slide_effect_refresh_title_overlay(phandle);
    phandle->refresh_top_timer = lv_timer_create(divoom_lvgl_photo_slide_effect_refresh_top_display_time_handle, 1000, phandle);
    if(phandle->refresh_top_timer == NULL) goto init_failed;
    lv_timer_pause(phandle->refresh_top_timer);
    divoom_system_free(user_data);
    divoom_print("create handle: %p\n", phandle);
    return phandle;
init_failed:
    divoom_system_free(user_data);
    divoom_lvgl_photo_slide_effect_exit(phandle);
    return NULL;
}

void divoom_lvgl_photo_slide_effect_update(void *handle, void *user_data)
{
}

static void divoom_lvgl_photo_slide_effect_start_timer_cb(lv_timer_t *timer)
{
    Pdivoom_lvgl_photo_slide_effect_ctrl phandle =
        (Pdivoom_lvgl_photo_slide_effect_ctrl)lv_timer_get_user_data(timer);

    if(phandle == NULL || phandle->start_timer != timer)
    {
        return;
    }
    phandle->start_timer = NULL;
    divoom_print("photo slide: start playback after menu cleanup\n");
    if(divoom_lvgl_photo_slide_effect_start_new_config(phandle, &phandle->update_slide_info) == 0)
    {
        return;
    }

    lv_timer_resume(phandle->refresh_top_timer);
    lv_timer_resume(phandle->refresh_timer);
    if(phandle->pcur_runing_slide && phandle->handle) phandle->pcur_runing_slide->pdisp(phandle->handle, NULL);
    divoom_lvgl_photo_slide_effect_show_overlays(phandle);
    lv_group_focus_obj(phandle->main_obj);
    divoom_lvgl_top_control_update_active();
}

void divoom_lvgl_photo_slide_effect_disp(void *handle, void *active_obj)
{
    Pdivoom_lvgl_photo_slide_effect_ctrl phandle = (Pdivoom_lvgl_photo_slide_effect_ctrl)handle;

    if(phandle == NULL)
    {
        return;
    }
    lv_img_cache_invalidate_src(NULL);
    if(phandle->weather_images_loaded == 0)
    {
        divoom_lvgl_photo_slide_effect_load_weather_images(phandle);
    }
    if(phandle->first_run_flag)
    {
        lv_scr_load(phandle->lv_snapshot_obj);
        if(phandle->start_timer == NULL)
        {
            phandle->start_timer = lv_timer_create(
                divoom_lvgl_photo_slide_effect_start_timer_cb, 1, phandle);
            if(phandle->start_timer)
            {
                phandle->first_run_flag = 0;
                lv_timer_set_repeat_count(phandle->start_timer, 1);
                divoom_print("photo slide: schedule playback in disp\n");
            }
        }
        return;
    }
    if(phandle->first_run_flag == 0)
    {       
        Pdivoom_pic_phone_manage_slide_config pslide_config;

        pslide_config = divoom_pic_phone_manage_get_slide_config();
        
        divoom_lvgl_photo_slide_effect_start_new_album_name(phandle, pslide_config->album_id);  
        
        if(phandle->update_effect_flag)
        {
            if(!divoom_lvgl_photo_slide_effect_start_new_config(phandle, &phandle->update_slide_info) &&
                (phandle->pcur_runing_slide == NULL || phandle->handle == NULL)) return;
            phandle->update_effect_flag = 0;            
            if(phandle->pcur_runing_slide && phandle->handle) phandle->pcur_runing_slide->pdisp(phandle->handle, active_obj);
            divoom_lvgl_photo_slide_effect_show_overlays(phandle);
            lv_group_focus_obj(phandle->main_obj);
            
            return;
        }
    }
    if(phandle->pcur_runing_slide == NULL || phandle->handle == NULL)
    {
        phandle->first_run_flag = 1;
        return;
    }
    lv_timer_resume(phandle->refresh_top_timer);
    lv_timer_resume(phandle->refresh_timer);
    if(phandle->pcur_runing_slide && phandle->handle) phandle->pcur_runing_slide->pdisp(phandle->handle, active_obj);
    divoom_lvgl_photo_slide_effect_show_overlays(phandle);
    lv_group_focus_obj(phandle->main_obj);
    divoom_lvgl_top_control_update_active();
}

void divoom_lvgl_photo_slide_effect_pause(void *handle)
{
    Pdivoom_lvgl_photo_slide_effect_ctrl phandle = (Pdivoom_lvgl_photo_slide_effect_ctrl)handle;

    if(phandle == NULL)
    {
        return;
    }
    divoom_print("pause handle: %p\n", phandle);
    if(phandle->start_timer)
    {
        lv_timer_del(phandle->start_timer);
        phandle->start_timer = NULL;
    }
    lv_timer_pause(phandle->refresh_timer);
    lv_timer_pause(phandle->refresh_top_timer);
    divoom_lvgl_photo_slide_effect_destory_btn(phandle);
    lv_obj_add_flag(phandle->top_disp_obj, LV_OBJ_FLAG_HIDDEN);
    divoom_lvgl_photo_slide_effect_release_video_osd(phandle);
    divoom_lvgl_photo_slide_effect_release_weather_images(phandle);
    divoom_lvgl_photo_slide_effect_set_title_overlay_hidden(phandle, 1);
    if(phandle->pcur_runing_slide && phandle->handle)
    {
        phandle->pcur_runing_slide->pause(phandle->handle, 1);
    }
    if(divoom_local_movie_play_is_playing())
    {
        divoom_menu_control_change_disp_mode(phandle->main_obj, 0);
    }
    
}

void divoom_lvgl_photo_slide_effect_exit(void *handle)
{
    Pdivoom_lvgl_photo_slide_effect_ctrl phandle = (Pdivoom_lvgl_photo_slide_effect_ctrl)handle;

    if(phandle == NULL)
    {
        return;
    }
    
    divoom_print("destory handle: %p\n", phandle);
    if(phandle->start_timer)
    {
        lv_timer_del(phandle->start_timer);
        phandle->start_timer = NULL;
    }
    divoom_lvgl_photo_slide_effect_destory_btn(phandle);
    if(phandle->refresh_timer) lv_timer_del(phandle->refresh_timer);
    if(phandle->refresh_top_timer) lv_timer_del(phandle->refresh_top_timer);
    if(phandle->pcur_runing_slide)
    {
        divoom_lvgl_photo_slide_effect_do_remove_photos(phandle);
    }
    if(phandle->pcur_runing_slide && phandle->handle)
    {
        phandle->pcur_runing_slide->pause(phandle->handle, 1);
    }
    divoom_lvgl_photo_slide_effect_release_video_osd(phandle);
    divoom_lvgl_photo_slide_effect_release_weather_images(phandle);
    if(phandle->top_disp_obj) lv_obj_del(phandle->top_disp_obj);
    if (phandle->title_label_obj)
    {
        lv_obj_t *title_stack = lv_obj_get_parent(phandle->title_label_obj);

        if (title_stack)
        {
            lv_obj_del(title_stack);
        }
        phandle->title_label_obj = NULL;
    }
    if(phandle->lv_snapshot_obj) lv_obj_del(phandle->lv_snapshot_obj);
    if(phandle->lv_snapshot_img)
    {
        lv_snapshot_free(phandle->lv_snapshot_img);
    }

    if(phandle->pselect_album_info.confirm_message_ctrl)
    {
        divoom_lvgl_message_box_destroy(phandle->pselect_album_info.confirm_message_ctrl, 1);
        phandle->pselect_album_info.confirm_message_ctrl = NULL;
    }  
    if(phandle->pcur_runing_slide && phandle->handle && phandle->pcur_runing_slide->pget_pic_info)
    {
        divoom_lvgl_photo_view_para_info pic_info;
        phandle->pcur_runing_slide->pget_pic_info(phandle->handle, &pic_info);
        divoom_pic_album_manage_update_last_play_album_pic_id(phandle->pcur_play_info.album_id, pic_info.pic_id);
    }
    if(phandle->pcur_runing_slide && phandle->handle)
    {
        phandle->pcur_runing_slide->pclean(phandle->handle);
    }

    lv_style_reset(&phandle->top_disp_style);
    lv_style_reset(&phandle->top_plate_style);
    divoom_system_free(phandle);
    lv_img_cache_invalidate_src(NULL);
    
    
}

