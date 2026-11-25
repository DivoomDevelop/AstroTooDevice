#include "divoom.h"
#include "divoom_lvgl_photo_view.h"
#include "divoom_lvgl_photo_slide_effect.h"
#include "divoom_lvgl_select_album_play.h"
#include "divoom_menu_control.h"
#include "divoom_astrootoo_ui_base.h"
#include "divoom_astrootoo_theme.h"
#include "divoom_pixel_format.h"
#include "lvgl.h"
#ifdef DIVOOM_USE_RK_IC
#include "rk_mpi.h"
#include <rga/im2d.h>
#include <rga/rga.h>
#include <rga/RgaApi.h>
#endif


/*
 这个文件主要是对照片进行管理，可以删除照片，也可以将照片归类到某个相册中, 图片显示正方形
*/
#define DIVOOM_LVGL_PHOTO_MANAGE_PIC_IMAGE_SELECT_OPA  LV_OPA_70
#define DIVOOM_LVGL_PHOTO_MANAGE_PIC_IMAGE_HIDE_OPA    LV_OPA_70
/* GalleryPhotoGridScreen + GalleryScreens.module.css：3×144 cell，gap 4，140×140 thumb */
#define DIVOOM_LVGL_PHOTO_MANAGE_PIC_ROW_NUM          (3)
#define DIVOOM_LVGL_PHOTO_MANAGE_PIC_LIST_NUM         (9)
#define DIVOOM_LVGL_PHOTO_MANAGE_PIC_NUM_PRE_SCREEN   (DIVOOM_LVGL_PHOTO_MANAGE_PIC_ROW_NUM * DIVOOM_LVGL_PHOTO_MANAGE_PIC_LIST_NUM)
#define DIVOOM_LVGL_PHOTO_MANAGE_CELL_OUTER           (144)
#define DIVOOM_LVGL_PHOTO_MANAGE_PIC_WIDTH            (140)
#define DIVOOM_LVGL_PHOTO_MANAGE_PIC_HIGH             (140)
#define DIVOOM_LVGL_PHOTO_MANAGE_PIC_GAP              (4)
#define DIVOOM_LVGL_PHOTO_MANAGE_CELL_STEP            (DIVOOM_LVGL_PHOTO_MANAGE_CELL_OUTER + DIVOOM_LVGL_PHOTO_MANAGE_PIC_GAP)
#define DIVOOM_LVGL_PHOTO_MANAGE_GRID_W               (DIVOOM_LVGL_PHOTO_MANAGE_PIC_ROW_NUM * DIVOOM_LVGL_PHOTO_MANAGE_CELL_OUTER \
    + (DIVOOM_LVGL_PHOTO_MANAGE_PIC_ROW_NUM - 1) * DIVOOM_LVGL_PHOTO_MANAGE_PIC_GAP)
#define DIVOOM_LVGL_PHOTO_MANAGE_GRID_INNER_X         ((DIVOOM_SCREEN_WIDTH - DIVOOM_LVGL_PHOTO_MANAGE_SCROLL_PAD_X * 2 \
    - DIVOOM_LVGL_PHOTO_MANAGE_GRID_W) / 2)
#define DIVOOM_LVGL_PHOTO_MANAGE_SCROLL_PAD_TOP       (8)
#define DIVOOM_LVGL_PHOTO_MANAGE_SCROLL_PAD_BOTTOM    (12)
#define DIVOOM_LVGL_PHOTO_MANAGE_SCROLL_PAD_X         (4)
#define DIVOOM_LVGL_PHOTO_MANAGE_THUMB_RADIUS         (8)
#define DIVOOM_LVGL_PHOTO_MANAGE_CELL_RADIUS          (10)  /* photoCell · border-radius 10px */
#define DIVOOM_LVGL_PHOTO_MANAGE_CELL_FOCUS_BG        0xF1DEBF  /* surfaceFocus 12% on astroBg */
#define DIVOOM_LVGL_PHOTO_MANAGE_ACTION_CNT           (4)
#define DIVOOM_LVGL_PHOTO_MANAGE_ACTION_SHEET_W       (448)
#define DIVOOM_LVGL_PHOTO_MANAGE_ACTION_SHEET_PAD      (8)
#define DIVOOM_LVGL_PHOTO_MANAGE_ACTION_SHEET_BORDER  (2)
#define DIVOOM_LVGL_PHOTO_MANAGE_ACTION_SHEET_RADIUS  (16)
#define DIVOOM_LVGL_PHOTO_MANAGE_ACTION_ROW_H         (DIVOOM_ASTRO_CARD_ROW_H)
#define DIVOOM_LVGL_PHOTO_MANAGE_ACTION_ROW_GAP       (DIVOOM_ASTRO_CARD_ROW_GAP)
#define DIVOOM_LVGL_PHOTO_MANAGE_ACTION_ROW_PAD_X     (20)
#define DIVOOM_LVGL_PHOTO_MANAGE_ACTION_FONT          (28)
/* F101 RGB565：与 confirm_modal 相同，overlay 须透明才能透出下层网格（勿用 ink dim） */
#define DIVOOM_LVGL_PHOTO_MANAGE_DEFER_MS           50

enum
{
    DIVOOM_LVGL_PHOTO_MANAGE_PANE_GRID = 0,
    DIVOOM_LVGL_PHOTO_MANAGE_PANE_MENU,
    DIVOOM_LVGL_PHOTO_MANAGE_PANE_CONFIRM,
};

static const char *divoom_lvgl_photo_manage_action_keys[DIVOOM_LVGL_PHOTO_MANAGE_ACTION_CNT] =
{
    "galleryActionPlayFromHere",
    "galleryActionViewPhoto",
    "galleryActionDeletePhoto",
    "galleryActionDeleteAll",
};


typedef struct _divoom_lvgl_photo_manage_refresh_item
{
    int pos1;
    int pos2;
    int image_w;
    int image_h;
}divoom_lvgl_photo_manage_refresh_item, *Pdivoom_lvgl_photo_manage_refresh_item;

typedef struct  _divoom_lvgl_photo_manage_item_menu_info
{
    void *cout_obj;
    void *bg_obj;
    void *add_obj;
    void *play_obj;
    void *del_obj;
}divoom_lvgl_photo_manage_item_menu_info, *Pdivoom_lvgl_photo_manage_item_menu_info;

typedef struct  _divoom_lvgl_photo_manage_long_press_menu_info
{
    void *bg_obj;
    void *image_cout_obj;
    void *image_obj;
    void *cout_obj;
    void *btn_table_obj[5];
    void *btn_image_obj[5];
}divoom_lvgl_photo_manage_long_press_menu_info, *Pdivoom_lvgl_photo_manage_long_press_menu_info;

typedef struct _divoom_lvgl_photo_manage_delete_info
{
    int album_id;
    int pic_num;
    int *pic_list;
}divoom_lvgl_photo_manage_delete_info, *Pdivoom_lvgl_photo_manage_delete_info;

typedef struct  _divoom_lvgl_photo_manage_bottom_delete_menu_info
{
    void *cout_obj;
    void *bg_obj;
    void *delete_obj;
    void *cancle_obj;
}divoom_lvgl_photo_manage_bottom_delete_menu_info, *Pdivoom_lvgl_photo_manage_bottom_delete_menu_info;


typedef struct  _divoom_lvgl_photo_manage_top_btn_menu_info
{
    void *cout_obj;
    void *return_obj;
    void *select_obj;
    void *select_all_obj;
    void *unselect_all_obj;
    void *more_obj;
    void *cancle_obj;
    void *bottom_cout_obj;
    void *delete_obj;
    void *bottom_more_obj;
    void *select_lable_obj;
    void *bottom_play_obj;
}divoom_lvgl_photo_manage_top_btn_menu_info, *Pdivoom_lvgl_photo_manage_top_btn_menu_info;

typedef struct  _divoom_lvgl_photo_manage_add_picture_btn_menu_info
{
    void *cout_obj;
    void *finish_obj;
    void *cancle_obj;
    void *select_lable_obj;
}divoom_lvgl_photo_manage_add_picture_btn_menu_info, *Pdivoom_lvgl_photo_manage_add_picture_btn_menu_info;

typedef struct  _divoom_lvgl_photo_manage_add_picture_long_btn_menu_info
{
    void *image_obj;
    void *cout_obj;
}divoom_lvgl_photo_manage_add_picture_long_btn_menu_info, *Pdivoom_lvgl_photo_manage_add_picture_long_btn_menu_info;

typedef struct _divoom_lvgl_photo_manage_item
{
    int img_pos;  
    int img_id;
    lv_img_dsc_t *image_data;
    void *thumb_cont;
    void *thumb_clip;
    void *img_obj;
    void *btn_obj;
    void *video_obj;
}divoom_lvgl_photo_manage_item, *Pdivoom_lvgl_photo_manage_item;

typedef struct _divoom_lvgl_photo_manage_update_image_info
{
    int  hide_flag;
    int  select_flag;
    void *hide_flag_obj;
    void *image_obj;
    void *select_obj;
    void *image_data;
}divoom_lvgl_photo_manage_update_image_info, *Pdivoom_lvgl_photo_manage_update_image_info;

typedef struct _divoom_lvgl_photo_manage_ctrl
{
    divoom_astrootoo_ui_base ui;
    int menu_id;
    int select_mode_flag;   //是否处理选择状态
    int first_load_flag;
    int pic_num_cur_load;   //当前加载相片的数目
    int dir;
    int operate_type;
    int album_type;     //相簿类型
    int pic_num;
    uint8 *select_table;
    int read_pic_pos;
    int pane;
    int action_focus;
    lv_style_t btn_normal_type;
    lv_style_t btn_press_type;
    lv_style_t btn_select_type;
    lv_anim_t image_anim;    //照片放大动画
    lv_obj_t *main_obj;
    lv_obj_t *header_obj;
    lv_obj_t *scroll_obj;
    lv_obj_t *image_main_cout_obj;
    lv_obj_t *last_active_obj;
    lv_obj_t *restore_focus_obj;
    lv_timer_t *restore_input_timer;
    lv_timer_t *alloc_fail_return_timer;
    lv_timer_t *action_pick_timer;
    int action_pick_deferred_idx;
    uint8 esc_return_pending;
    lv_obj_t *action_overlay;
    lv_obj_t *action_sheet;
    lv_obj_t *action_btns[DIVOOM_LVGL_PHOTO_MANAGE_ACTION_CNT];
    lv_obj_t *confirm_overlay;
    lv_obj_t *confirm_cancel_btn;
    lv_obj_t *confirm_ok_btn;
    int confirm_focus;
    void *pdriect_menu_ctrl;
    void *action_direct;
    void *pget_pic_handle;
    divoom_lvgl_photo_view_para_info photo_view_info;
    divoom_lvgl_select_album_play_para padd_album_info;
    divoom_lvgl_photo_manage_para  para_info;
    divoom_lvgl_photo_manage_para  add_para_info;
    divoom_lvgl_photo_manage_item plist[DIVOOM_LVGL_PHOTO_MANAGE_PIC_NUM_PRE_SCREEN];
}divoom_lvgl_photo_manage_ctrl, *Pdivoom_lvgl_photo_manage_ctrl;

int divoom_lvgl_photo_manage_get_select_items(Pdivoom_lvgl_photo_manage_ctrl phandle);
void divoom_lvgl_photo_manage_update_max_pos(Pdivoom_lvgl_photo_manage_ctrl phandle);
int divoom_lvgl_photo_manage_add_more_to_albums(Pdivoom_lvgl_photo_manage_ctrl phandle);
int divoom_lvgl_photo_manage_remove_picture_from_album(Pdivoom_lvgl_photo_manage_ctrl phandle);
void divoom_lvgl_photo_manage_remove_all_picture_from_album(Pdivoom_lvgl_photo_manage_ctrl phandle);
int divoom_lvgl_photo_manage_delete_picture(Pdivoom_lvgl_photo_manage_ctrl phandle);
int divoom_lvgl_photo_manage_get_video_flag(int pic_id, int album_id);

static void divoom_lvgl_photo_manage_action_sheet_close(Pdivoom_lvgl_photo_manage_ctrl phandle, int restore_grid_focus);
static void divoom_lvgl_photo_manage_action_sheet_open(Pdivoom_lvgl_photo_manage_ctrl phandle);
static void divoom_lvgl_photo_manage_action_pick_execute(Pdivoom_lvgl_photo_manage_ctrl phandle, int action_idx);
static void divoom_lvgl_photo_manage_confirm_close(Pdivoom_lvgl_photo_manage_ctrl phandle);
static void divoom_lvgl_photo_manage_confirm_return_to_grid(Pdivoom_lvgl_photo_manage_ctrl phandle);
static void divoom_lvgl_photo_manage_create_confirm_dialog(Pdivoom_lvgl_photo_manage_ctrl phandle, int type);
static void divoom_lvgl_photo_manage_grid_focus(Pdivoom_lvgl_photo_manage_ctrl phandle, lv_obj_t *obj);
static void divoom_lvgl_photo_manage_fill_album_title(Pdivoom_lvgl_photo_manage_ctrl phandle, char *buf, int buf_len);
static int divoom_lvgl_photo_manage_btn_is_visible(Pdivoom_lvgl_photo_manage_ctrl phandle, lv_obj_t *btn);
static lv_obj_t *divoom_lvgl_photo_manage_grid_next(Pdivoom_lvgl_photo_manage_ctrl phandle, lv_obj_t *cur_obj, int direct);
static void divoom_lvgl_photo_manage_apply_cell_style(Pdivoom_lvgl_photo_manage_ctrl phandle, int index);
void divoom_lvgl_photo_manage_start_play_album(Pdivoom_lvgl_photo_manage_ctrl phandle);


int divoom_lvgl_photo_manage_get_current_item(Pdivoom_lvgl_photo_manage_ctrl phandle)
{
    lv_obj_t *active_obj;
    int i;

    if (!phandle)
    {
        return 0;
    }

    active_obj = phandle->last_active_obj;
    if (!active_obj || !lv_obj_is_valid(active_obj))
    {
        active_obj = lv_group_get_focused(lv_group_get_default());
    }
    for (i = 0; i < DIVOOM_LVGL_PHOTO_MANAGE_PIC_NUM_PRE_SCREEN; i++)
    {
        if (active_obj == phandle->plist[i].btn_obj && phandle->plist[i].img_id > 0)
        {
            return i;
        }
    }
    return 0;
}

/* 相簿记录可能仍在、原图文件却已不存在；此类记录不能创建可见或可聚焦的黑色空单元格。 */
static int divoom_lvgl_photo_manage_pic_file_exists(Pdivoom_lvgl_photo_manage_ctrl phandle, int pic_id)
{
    char file_name[256] = {0};

    if (!phandle || pic_id <= 0)
    {
        return 0;
    }
    if (phandle->para_info.album_id >= DIVOOM_CUSTOM_START_CLOCK_ID)
    {
        if (!divoom_database_manage_get_pic_path_name(pic_id, file_name))
        {
            return 0;
        }
    }
    else
    {
        if (!divoom_database_manage_get_custom_photo_name(pic_id, file_name))
        {
            return 0;
        }
    }
    return file_name[0] != 0 && divoom_app_com_check_file_exist(file_name);
}

int divoom_lvgl_photo_manage_read_pic(Pdivoom_lvgl_photo_manage_ctrl phandle, int pic_pos, int pic_id, int asyn_flag, uint8 *rgb_buf, DIVOOM_PIC_ALBUM_MANAGE_READ_OVER *pcallback, void *para)
{
    (void)phandle;
    (void)pic_pos;

    //缩图图已经处理旋转与镜像了，所以这里不用处理
    return divoom_thumb_cache_read_pic(0, pic_id, 1, rgb_buf,
        DIVOOM_LVGL_PHOTO_MANAGE_PIC_WIDTH, DIVOOM_LVGL_PHOTO_MANAGE_PIC_HIGH,
        pcallback, para);
}

void divoom_lvgl_photo_manage_update_finish_read_over_callback(void *para, uint8 *rgb_buf,int image_w, int image_h)
{
#if defined(DIVOOM_SIMULATOR)
    /* sim：缩略图回调运行在 worker 线程并需获取全局 lvgl 锁；
       退出/暂停照片网格时，menu 事件线程持有 lvgl 锁并在 divoom_thumb_cache_clear_all
       中忙等本 worker 完成（wait_flag==0），此时阻塞式 get_lvgl_lock 会与之死锁，
       导致 LVGL 渲染线程被饿死、退层卡死。改用 trylock：拿不到锁就跳过本次 invalidate
       （仅重绘提示，worker 得以及时归位清零 wait_flag），下一帧自然刷新。 */
    if (divoom_get_lvgl_try_lock() == 0)
    {
        lv_obj_invalidate(para);
        divoom_free_lvgl_lock();
    }
#else
    divoom_get_lvgl_lock();
    lv_obj_invalidate(para);
    divoom_free_lvgl_lock();
#endif
}

static void divoom_lvgl_photo_manage_cache_load(Pdivoom_lvgl_photo_manage_ctrl phandle, int start_pos)
{
    int j, img_id;

    if(start_pos == 0 || start_pos >= phandle->pic_num - 1)
    {
        return;
    }
    for(j = 0; j < DIVOOM_LVGL_PHOTO_MANAGE_PIC_NUM_PRE_SCREEN; j++)
    {
        if(start_pos + j >= phandle->pic_num)
        {
            break;
        }
        if(phandle->para_info.album_id >= DIVOOM_CUSTOM_START_CLOCK_ID)
        {
            divoom_thumb_cache_pre_read(0, divoom_pic_album_manage_get_disp_order_pic_id(phandle->pget_pic_handle,start_pos + j, NULL));
        }
        else
        {
            img_id = divoom_custom_photo_get_pic_id(phandle->pget_pic_handle,start_pos + j);
            divoom_thumb_cache_pre_read(0, img_id);
        }
    }
}

//重新加载一屏的数据，dir向上或向下
static void divoom_lvgl_photo_manage_reload_all_pic(Pdivoom_lvgl_photo_manage_ctrl phandle, int dir)
{
    int  i;
    Pdivoom_pic_phone_manage_item_offset pitem_info;

    if (!phandle || !phandle->plist[0].image_data)
    {
        return;
    }
    if(dir)
    {
        if(phandle->read_pic_pos >= phandle->pic_num)
        {
            divoom_print("err!: %d, %d\n",phandle->read_pic_pos, phandle->pic_num);
            return;
        }
    }
    else 
    {
        if(phandle->plist[0].img_pos)
        {
            phandle->read_pic_pos = phandle->plist[0].img_pos - DIVOOM_LVGL_PHOTO_MANAGE_PIC_NUM_PRE_SCREEN;
        }
        else
        {
            divoom_print("err!\n");
            return;
        }
    }
    
    for(i = 0; i < DIVOOM_LVGL_PHOTO_MANAGE_PIC_NUM_PRE_SCREEN; i++)
    {        
        if(phandle->read_pic_pos < phandle->pic_num)
        {                
            phandle->plist[i].img_pos = phandle->read_pic_pos;
            if(phandle->para_info.album_id >= DIVOOM_CUSTOM_START_CLOCK_ID)
            {
                phandle->plist[i].img_id = divoom_pic_album_manage_get_disp_order_pic_id(phandle->pget_pic_handle,phandle->read_pic_pos, NULL);
            }
            else
            {
                phandle->plist[i].img_id = divoom_custom_photo_get_pic_id(phandle->pget_pic_handle,phandle->read_pic_pos);
            }
            if(divoom_lvgl_photo_manage_pic_file_exists(phandle, phandle->plist[i].img_id)
                && divoom_lvgl_photo_manage_read_pic(phandle, phandle->plist[i].img_pos,phandle->plist[i].img_id, 0, (uint8 *)phandle->plist[i].image_data->data, divoom_lvgl_photo_manage_update_finish_read_over_callback, phandle->plist[i].img_obj))
            {
                lv_obj_invalidate(phandle->plist[i].img_obj);
                lv_obj_clear_flag(phandle->plist[i].btn_obj, LV_OBJ_FLAG_HIDDEN);
            }
            else
            {
                /* 缩略图没有有效来源时隐藏整个按钮，焦点导航也会跳过该单元格。 */
                lv_obj_add_flag(phandle->plist[i].btn_obj, LV_OBJ_FLAG_HIDDEN);
            }
            if (!lv_obj_has_flag(phandle->plist[i].btn_obj, LV_OBJ_FLAG_HIDDEN)
                && divoom_lvgl_photo_manage_get_video_flag(phandle->plist[i].img_id, phandle->para_info.album_id))
            {
                lv_obj_clear_flag(phandle->plist[i].video_obj, LV_OBJ_FLAG_HIDDEN);
            }
            else
            {
                lv_obj_add_flag(phandle->plist[i].video_obj, LV_OBJ_FLAG_HIDDEN);
            }
            divoom_lvgl_photo_manage_apply_cell_style(phandle, i);
            phandle->read_pic_pos ++;
            phandle->pic_num_cur_load = i+1;
        }
        else
        {
            divoom_print("over image and empty: %d\n", i );
            pitem_info = NULL;
            phandle->plist[i].img_id = 0;
            phandle->plist[i].img_pos = -1;
            lv_obj_add_flag(phandle->plist[i].btn_obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(phandle->plist[i].video_obj, LV_OBJ_FLAG_HIDDEN);
        }
    }
    divoom_lvgl_photo_manage_cache_load(phandle, dir?phandle->read_pic_pos + DIVOOM_LVGL_PHOTO_MANAGE_PIC_NUM_PRE_SCREEN:phandle->read_pic_pos - DIVOOM_LVGL_PHOTO_MANAGE_PIC_NUM_PRE_SCREEN);
}

static int divoom_lvgl_photo_manage_btn_is_visible(Pdivoom_lvgl_photo_manage_ctrl phandle, lv_obj_t *btn);

static void divoom_lvgl_photo_manage_cell_apply_focus_ring(lv_obj_t *cell_btn, int focused)
{
    if (!cell_btn)
    {
        return;
    }
    if (focused)
    {
        /* GalleryScreens · photoCellActive：2px 琥珀描边 + 12% focus 底，圆角 10px（画在 144 外层格） */
        lv_obj_set_style_radius(cell_btn, DIVOOM_LVGL_PHOTO_MANAGE_CELL_RADIUS, 0);
        lv_obj_set_style_border_width(cell_btn, DIVOOM_ASTRO_ITEM_BORDER_W_ACTIVE, 0);
        lv_obj_set_style_border_color(cell_btn, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), 0);
        lv_obj_set_style_border_opa(cell_btn, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(cell_btn, lv_color_hex(DIVOOM_LVGL_PHOTO_MANAGE_CELL_FOCUS_BG), 0);
        lv_obj_set_style_bg_opa(cell_btn, LV_OPA_COVER, 0);
        lv_obj_set_style_shadow_width(cell_btn, 0, 0);
    }
    else
    {
        lv_obj_set_style_border_width(cell_btn, 0, 0);
        lv_obj_set_style_bg_opa(cell_btn, LV_OPA_TRANSP, 0);
        lv_obj_set_style_shadow_width(cell_btn, 0, 0);
    }
}

static void divoom_lvgl_photo_manage_sync_focus_ring(Pdivoom_lvgl_photo_manage_ctrl phandle, lv_obj_t *focus_btn)
{
    int i;

    if (!phandle)
    {
        return;
    }
    for (i = 0; i < DIVOOM_LVGL_PHOTO_MANAGE_PIC_NUM_PRE_SCREEN; i++)
    {
        divoom_lvgl_photo_manage_cell_apply_focus_ring(phandle->plist[i].btn_obj,
            (focus_btn && phandle->plist[i].btn_obj == focus_btn) ? 1 : 0);
    }
}

static void divoom_lvgl_photo_manage_grid_focus(Pdivoom_lvgl_photo_manage_ctrl phandle, lv_obj_t *obj);
static void divoom_lvgl_photo_manage_rebind_grid_group(Pdivoom_lvgl_photo_manage_ctrl phandle)
{
    lv_group_t *grp;
    int i;

    if (!phandle)
    {
        return;
    }
    grp = lv_group_get_default();
    if (!grp)
    {
        return;
    }
    for (i = 0; i < phandle->pic_num_cur_load; i++)
    {
        lv_obj_t *btn = phandle->plist[i].btn_obj;

        if (btn && lv_obj_is_valid(btn) && divoom_lvgl_photo_manage_btn_is_visible(phandle, btn)
            && !lv_obj_get_group(btn))
        {
            lv_group_add_obj(grp, btn);
        }
    }
}

static void divoom_lvgl_photo_manage_remove_grid_from_group(Pdivoom_lvgl_photo_manage_ctrl phandle)
{
    lv_group_t *grp;
    int i;

    if (!phandle)
    {
        return;
    }
    grp = lv_group_get_default();
    if (!grp)
    {
        return;
    }
    for (i = 0; i < phandle->pic_num_cur_load; i++)
    {
        lv_obj_t *btn = phandle->plist[i].btn_obj;

        if (btn && lv_obj_is_valid(btn) && lv_obj_get_group(btn))
        {
            lv_group_remove_obj(btn);
        }
    }
}

static void divoom_lvgl_photo_manage_restore_grid_input(Pdivoom_lvgl_photo_manage_ctrl phandle, lv_obj_t *active_obj);

static void divoom_lvgl_photo_manage_schedule_esc_return(Pdivoom_lvgl_photo_manage_ctrl phandle)
{
    if (!phandle || phandle->esc_return_pending)
    {
        return;
    }
    if (divoom_lvgl_photo_manage_get_select_items(phandle))
    {
        divoom_lvgl_photo_manage_create_confirm_dialog(phandle, 4);
        return;
    }
    /* return_pre 仅入队，主线程/LVGL 均可调用；勿在此 lv_timer_create（主线程 ESC 会抢跑并锁死 pending） */
    phandle->esc_return_pending = 1;
    divoom_menu_control_return_pre();
}

static void divoom_lvgl_photo_manage_ensure_grid_focus(Pdivoom_lvgl_photo_manage_ctrl phandle)
{
    lv_obj_t *focused;

    if (!phandle || phandle->pane != DIVOOM_LVGL_PHOTO_MANAGE_PANE_GRID)
    {
        return;
    }
    focused = lv_group_get_focused(lv_group_get_default());
    if (divoom_lvgl_photo_manage_btn_is_visible(phandle, focused))
    {
        return;
    }
    divoom_lvgl_photo_manage_rebind_grid_group(phandle);
    divoom_lvgl_photo_manage_restore_grid_input(phandle, phandle->last_active_obj);
}

static void divoom_lvgl_photo_manage_deferred_restore_cb(lv_timer_t *timer)
{
    Pdivoom_lvgl_photo_manage_ctrl phandle;
    lv_obj_t *focus_obj;

    phandle = (Pdivoom_lvgl_photo_manage_ctrl)lv_timer_get_user_data(timer);
    if (!phandle)
    {
        return;
    }
    phandle->restore_input_timer = NULL;
    focus_obj = phandle->restore_focus_obj;
    phandle->restore_focus_obj = NULL;
    divoom_lvgl_photo_manage_restore_grid_input(phandle, focus_obj);
}

static void divoom_lvgl_photo_manage_deferred_alloc_fail_return_cb(lv_timer_t *timer)
{
    Pdivoom_lvgl_photo_manage_ctrl phandle;

    phandle = (Pdivoom_lvgl_photo_manage_ctrl)lv_timer_get_user_data(timer);
    if (phandle)
    {
        phandle->alloc_fail_return_timer = NULL;
    }
    lv_timer_del(timer);
    divoom_menu_control_return_pre();
}

static void divoom_lvgl_photo_manage_schedule_alloc_fail_return(Pdivoom_lvgl_photo_manage_ctrl phandle)
{
    lv_timer_t *timer;

    if (!phandle || phandle->alloc_fail_return_timer)
    {
        return;
    }
    timer = lv_timer_create(divoom_lvgl_photo_manage_deferred_alloc_fail_return_cb,
        DIVOOM_LVGL_PHOTO_MANAGE_DEFER_MS, phandle);
    if (timer)
    {
        phandle->alloc_fail_return_timer = timer;
        lv_timer_set_repeat_count(timer, 1);
    }
}

static void divoom_lvgl_photo_manage_restore_grid_input(Pdivoom_lvgl_photo_manage_ctrl phandle, lv_obj_t *active_obj)
{
    lv_obj_t *focus_obj;
    int i;

    if (!phandle)
    {
        return;
    }

    phandle->pane = DIVOOM_LVGL_PHOTO_MANAGE_PANE_GRID;
    divoom_lvgl_photo_manage_rebind_grid_group(phandle);
    focus_obj = active_obj;
    if (!focus_obj || !lv_obj_is_valid(focus_obj)
        || !divoom_lvgl_photo_manage_btn_is_visible(phandle, focus_obj))
    {
        focus_obj = phandle->last_active_obj;
    }
    if (!focus_obj || !lv_obj_is_valid(focus_obj)
        || !divoom_lvgl_photo_manage_btn_is_visible(phandle, focus_obj))
    {
        /* 首页首项也可能是失效照片，恢复时必须跳过全部隐藏单元格。 */
        focus_obj = NULL;
        for (i = 0; i < phandle->pic_num_cur_load; i++)
        {
            lv_obj_t *btn = phandle->plist[i].btn_obj;

            if (btn && divoom_lvgl_photo_manage_btn_is_visible(phandle, btn))
            {
                focus_obj = btn;
                break;
            }
        }
    }
    if (focus_obj && lv_obj_is_valid(focus_obj))
    {
        divoom_lvgl_photo_manage_grid_focus(phandle, focus_obj);
    }
}

static void divoom_lvgl_photo_manage_cell_focus_cb(lv_event_t *e)
{
    Pdivoom_lvgl_photo_manage_ctrl phandle;
    lv_obj_t *btn;
    lv_event_code_t code;

    code = lv_event_get_code(e);
    if (code != LV_EVENT_FOCUSED && code != LV_EVENT_DEFOCUSED)
    {
        return;
    }
    phandle = (Pdivoom_lvgl_photo_manage_ctrl)lv_event_get_user_data(e);
    btn = lv_event_get_target_obj(e);
    if (code == LV_EVENT_DEFOCUSED)
    {
        divoom_lvgl_photo_manage_cell_apply_focus_ring(btn, 0);
        return;
    }
    divoom_lvgl_photo_manage_cell_apply_focus_ring(btn, 1);
    divoom_lvgl_photo_manage_sync_focus_ring(phandle, btn);
    divoom_astrootoo_ui_focus_scroll(btn);
}

static void divoom_lvgl_photo_manage_grid_focus(Pdivoom_lvgl_photo_manage_ctrl phandle, lv_obj_t *obj)
{
    if (!phandle || !obj)
    {
        return;
    }
    if (!divoom_lvgl_photo_manage_btn_is_visible(phandle, obj))
    {
        return;
    }
    lv_group_focus_obj(obj);
    divoom_lvgl_photo_manage_sync_focus_ring(phandle, obj);
    divoom_astrootoo_ui_focus_scroll(obj);
    phandle->last_active_obj = obj;
}

static int divoom_lvgl_photo_manage_btn_is_visible(Pdivoom_lvgl_photo_manage_ctrl phandle, lv_obj_t *btn)
{
    int i;

    if (!phandle || !btn)
    {
        return 0;
    }
    for (i = 0; i < DIVOOM_LVGL_PHOTO_MANAGE_PIC_NUM_PRE_SCREEN; i++)
    {
        if (phandle->plist[i].btn_obj == btn && phandle->plist[i].img_id > 0
            && !lv_obj_has_flag(btn, LV_OBJ_FLAG_HIDDEN))
        {
            return 1;
        }
    }
    return 0;
}

static lv_obj_t *divoom_lvgl_photo_manage_grid_next(Pdivoom_lvgl_photo_manage_ctrl phandle, lv_obj_t *cur_obj, int direct)
{
    lv_obj_t *next_obj;
    int guard;

    if (!phandle || !cur_obj)
    {
        return NULL;
    }

    next_obj = cur_obj;
    for (guard = 0; guard < DIVOOM_LVGL_PHOTO_MANAGE_PIC_NUM_PRE_SCREEN; guard++)
    {
        next_obj = divoom_menu_direct_ctrl_get(phandle->pdriect_menu_ctrl, next_obj, direct);
        if (!next_obj || next_obj == cur_obj)
        {
            return NULL;
        }
        if (divoom_lvgl_photo_manage_btn_is_visible(phandle, next_obj))
        {
            return next_obj;
        }
    }
    return NULL;
}

static void divoom_lvgl_photo_manage_apply_cell_style(Pdivoom_lvgl_photo_manage_ctrl phandle, int index)
{
    lv_obj_t *btn;
    lv_obj_t *thumb_cont;
    int selected;

    if (!phandle || index < 0 || index >= DIVOOM_LVGL_PHOTO_MANAGE_PIC_NUM_PRE_SCREEN)
    {
        return;
    }

    btn = phandle->plist[index].btn_obj;
    thumb_cont = phandle->plist[index].thumb_cont;
    if (!btn || !thumb_cont)
    {
        return;
    }

    lv_obj_add_style(btn, &phandle->btn_normal_type, 0);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUS_KEY);

    selected = (phandle->plist[index].img_pos >= 0 && phandle->select_table
        && phandle->plist[index].img_pos < phandle->pic_num
        && phandle->select_table[phandle->plist[index].img_pos]);
    if (selected)
    {
        lv_obj_set_style_border_width(thumb_cont, DIVOOM_ASTRO_ITEM_BORDER_W_ACTIVE, 0);
        lv_obj_set_style_border_color(thumb_cont, lv_color_hex(0xDEB100), 0);
    }
    else if (!lv_obj_has_state(btn, LV_STATE_FOCUSED) && !lv_obj_has_state(btn, LV_STATE_FOCUS_KEY))
    {
        lv_obj_set_style_border_width(thumb_cont, 0, 0);
        lv_obj_set_style_shadow_width(thumb_cont, 0, 0);
    }
}

static void divoom_lvgl_photo_manage_fill_album_title(Pdivoom_lvgl_photo_manage_ctrl phandle, char *buf, int buf_len)
{
    if (phandle->para_info.album_id >= DIVOOM_CUSTOM_START_CLOCK_ID)
    {
        strncpy(buf, divoom_pic_album_manage_get_static_album_name(phandle->para_info.album_id), buf_len - 1);
        buf[buf_len - 1] = '\0';
    }
    else
    {
        Pdivoom_clock_manage_item pclock_info = divoom_clock_manage_get_clock_info(phandle->para_info.album_id,
            phandle->para_info.clock_pos, phandle->para_info.parent_id);

        if (pclock_info)
        {
            if (divoom_main_set_lang_flag(-1) == DIVOOM_LVGL_MENU_LANG_SC)
            {
                strncpy(buf, pclock_info->name_cn, buf_len - 1);
            }
            else
            {
                strncpy(buf, pclock_info->name_en, buf_len - 1);
            }
            buf[buf_len - 1] = '\0';
            divoom_clock_manage_free_clock(pclock_info);
        }
        else
        {
            strncpy(buf, divoom_pic_album_manage_get_static_album_name(phandle->para_info.album_id), buf_len - 1);
            buf[buf_len - 1] = '\0';
        }
    }
}

static void divoom_lvgl_photo_manage_confirm_refresh(Pdivoom_lvgl_photo_manage_ctrl phandle)
{
    int ok_focus;
    lv_obj_t *cancel_lbl;
    lv_obj_t *ok_lbl;

    if (!phandle || !phandle->confirm_cancel_btn || !phandle->confirm_ok_btn)
    {
        return;
    }
    ok_focus = (phandle->confirm_focus == 1);

    lv_obj_set_style_bg_opa(phandle->confirm_cancel_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(phandle->confirm_cancel_btn,
        lv_color_hex(ok_focus ? DIVOOM_ASTRO_SURFACE_IDLE : DIVOOM_ASTRO_SURFACE_FOCUS), 0);
    lv_obj_set_style_border_width(phandle->confirm_cancel_btn, 0, 0);

    if (ok_focus)
    {
        lv_obj_set_style_bg_opa(phandle->confirm_ok_btn, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(phandle->confirm_ok_btn, lv_color_hex(DIVOOM_ASTRO_DANGER), 0);
        lv_obj_set_style_border_width(phandle->confirm_ok_btn, 0, 0);
    }
    else
    {
        lv_obj_set_style_bg_opa(phandle->confirm_ok_btn, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(phandle->confirm_ok_btn, 2, 0);
        lv_obj_set_style_border_color(phandle->confirm_ok_btn, lv_color_hex(DIVOOM_ASTRO_DANGER), 0);
    }

    cancel_lbl = lv_obj_get_child(phandle->confirm_cancel_btn, 0);
    ok_lbl = lv_obj_get_child(phandle->confirm_ok_btn, 0);
    if (cancel_lbl)
    {
        lv_obj_set_style_text_color(cancel_lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    }
    if (ok_lbl)
    {
        lv_obj_set_style_text_color(ok_lbl,
            lv_color_hex(ok_focus ? DIVOOM_ASTRO_BG : DIVOOM_ASTRO_DANGER), 0);
    }
    lv_group_focus_obj(ok_focus ? phandle->confirm_ok_btn : phandle->confirm_cancel_btn);
    divoom_astrootoo_ui_confirm_modal_sync_btn_labels(phandle->confirm_cancel_btn,
        phandle->confirm_ok_btn, ok_focus);
}

static void divoom_lvgl_photo_manage_confirm_commit(Pdivoom_lvgl_photo_manage_ctrl phandle)
{
    if (phandle->operate_type == 0)
    {
        divoom_lvgl_photo_manage_delete_picture(phandle);
    }
    else if (phandle->operate_type == 1)
    {
        if (phandle->para_info.album_id >= DIVOOM_CUSTOM_START_CLOCK_ID)
        {
            divoom_pic_phone_manage_remove_all_pic();
        }
        else
        {
            divoom_clock_id_info clock_info = {0};

            clock_info.clock_id = phandle->para_info.album_id;
            clock_info.clock_pos = phandle->para_info.clock_pos;
            clock_info.parent_id = phandle->para_info.parent_id;
            divoom_custom_photo_clear_all(&clock_info);
        }
        divoom_menu_control_return_pre();
    }
    else if (phandle->operate_type == 2)
    {
        divoom_lvgl_photo_manage_remove_all_picture_from_album(phandle);
        divoom_menu_control_return_pre();
    }
    else if (phandle->operate_type == 3)
    {
        divoom_lvgl_photo_manage_remove_picture_from_album(phandle);
    }
    else if (phandle->operate_type == 4)
    {
        divoom_menu_control_return_pre();
    }
}

static void divoom_lvgl_photo_manage_confirm_key_cb(lv_event_t *e)
{
    Pdivoom_lvgl_photo_manage_ctrl phandle;
    uint8_t key_code;
    int op;
    int do_commit;

    if (lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }
    phandle = (Pdivoom_lvgl_photo_manage_ctrl)lv_event_get_user_data(e);
    key_code = divoom_astrootoo_ui_key_code_from_event(e);

    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        divoom_lvgl_photo_manage_confirm_close(phandle);
        divoom_lvgl_photo_manage_confirm_return_to_grid(phandle);
        return;
    }
    if (divoom_astrootoo_ui_key_is_left(key_code) || divoom_astrootoo_ui_key_is_right(key_code)
        || divoom_astrootoo_ui_key_is_prev(key_code) || divoom_astrootoo_ui_key_is_next(key_code))
    {
        phandle->confirm_focus = (phandle->confirm_focus == 0) ? 1 : 0;
        divoom_lvgl_photo_manage_confirm_refresh(phandle);
        return;
    }
    if (divoom_astrootoo_ui_key_is_enter(key_code))
    {
        op = phandle->operate_type;
        do_commit = (phandle->confirm_focus == 1);
        if (do_commit)
        {
            divoom_lvgl_photo_manage_confirm_commit(phandle);
        }
        divoom_lvgl_photo_manage_confirm_close(phandle);
        /* TimesFrame：确认框关闭后回到照片网格；仅删/移全部(1/2)与退出(4)确认会 return_pre */
        if (do_commit && (op == 1 || op == 2 || op == 4))
        {
            return;
        }
        divoom_lvgl_photo_manage_confirm_return_to_grid(phandle);
    }
}

static void divoom_lvgl_photo_manage_confirm_return_to_grid(Pdivoom_lvgl_photo_manage_ctrl phandle)
{
    if (!phandle)
    {
        return;
    }
    phandle->pane = DIVOOM_LVGL_PHOTO_MANAGE_PANE_GRID;
    divoom_lvgl_photo_manage_restore_grid_input(phandle, phandle->last_active_obj);
}

static void divoom_lvgl_photo_manage_confirm_close(Pdivoom_lvgl_photo_manage_ctrl phandle)
{
    if (!phandle || !phandle->confirm_overlay)
    {
        return;
    }
    lv_obj_del_async(phandle->confirm_overlay);
    phandle->confirm_overlay = NULL;
    phandle->confirm_cancel_btn = NULL;
    phandle->confirm_ok_btn = NULL;
    if (phandle->pane == DIVOOM_LVGL_PHOTO_MANAGE_PANE_CONFIRM)
    {
        phandle->pane = DIVOOM_LVGL_PHOTO_MANAGE_PANE_GRID;
    }
}

static lv_obj_t *divoom_lvgl_photo_manage_confirm_make_btn(Pdivoom_lvgl_photo_manage_ctrl phandle,
    lv_obj_t *parent, const char *label_key)
{
    return divoom_astrootoo_ui_confirm_modal_make_btn(parent,
        divoom_menu_control_get_disp_str(label_key), phandle->menu_id, 0,
        divoom_lvgl_photo_manage_confirm_key_cb, phandle, 1);
}

static void divoom_lvgl_photo_manage_create_confirm_dialog(Pdivoom_lvgl_photo_manage_ctrl phandle, int type)
{
    lv_obj_t *frame;
    lv_obj_t *title;
    lv_obj_t *body;
    lv_obj_t *actions;
    const char *title_key;
    const char *desc_key;
    const char *confirm_key;

    if (type > 4 || !phandle)
    {
        return;
    }

    divoom_lvgl_photo_manage_confirm_close(phandle);
    divoom_lvgl_photo_manage_remove_grid_from_group(phandle);
    phandle->operate_type = type;
    phandle->confirm_focus = 0;
    phandle->pane = DIVOOM_LVGL_PHOTO_MANAGE_PANE_CONFIRM;

    if (type == 0)
    {
        title_key = "DeleteSelectPic";
        desc_key = "DeleteSelectPicNoitce";
        confirm_key = "Delete";
    }
    else if (type == 1)
    {
        title_key = "DelAllPicture";
        desc_key = "DelAllPictureNoitce";
        confirm_key = "Delete";
    }
    else if (type == 2)
    {
        title_key = "DelAllPicFromAlbum";
        desc_key = "DelAllPicFromAlbumDesc";
        confirm_key = "Remove";
    }
    else if (type == 3)
    {
        title_key = "RemoveSelectPic";
        desc_key = "RemoveSelectPicNoitce";
        confirm_key = "Remove";
    }
    else
    {
        title_key = "IsExitConfirm";
        desc_key = "IsExitConfirmDesc";
        confirm_key = "Confirm";
    }

    phandle->confirm_overlay = divoom_astrootoo_ui_confirm_modal_create(
        phandle->main_obj, &frame);
    if (!phandle->confirm_overlay || !frame)
    {
        return;
    }

    title = lv_label_create(frame);
    lv_label_set_long_mode(title, LV_LABEL_LONG_WRAP);
    lv_label_set_text(title, divoom_menu_control_get_disp_str(title_key));
    lv_obj_set_width(title, LV_PCT(100));
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(title, divoom_menu_control_get_font_info(30, phandle->menu_id), 0);
    lv_obj_set_style_pad_bottom(title, 10, 0);
    lv_obj_set_style_border_side(title, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_width(title, 2, 0);
    lv_obj_set_style_border_color(title, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), 0);

    body = lv_label_create(frame);
    lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
    lv_label_set_text(body, divoom_menu_control_get_disp_str(desc_key));
    lv_obj_set_width(body, LV_PCT(100));
    lv_obj_set_style_text_align(body, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(body, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
    lv_obj_set_style_text_font(body, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);

    actions = lv_obj_create(frame);
    lv_obj_remove_style_all(actions);
    lv_obj_set_width(actions, LV_PCT(100));
    lv_obj_set_height(actions, LV_SIZE_CONTENT);
    lv_obj_clear_flag(actions, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(actions, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(actions, 8, 0);

    phandle->confirm_cancel_btn = divoom_lvgl_photo_manage_confirm_make_btn(phandle, actions, "Cancel");
    phandle->confirm_ok_btn = divoom_lvgl_photo_manage_confirm_make_btn(phandle, actions, confirm_key);
    divoom_astrootoo_ui_confirm_modal_layout(frame);
    divoom_lvgl_photo_manage_confirm_refresh(phandle);
}

static void divoom_lvgl_photo_manage_action_play_from_here(Pdivoom_lvgl_photo_manage_ctrl phandle)
{
    int select_item = divoom_lvgl_photo_manage_get_current_item(phandle);

    if (phandle->para_info.album_id >= DIVOOM_CUSTOM_START_CLOCK_ID)
    {
        divoom_pic_album_manage_update_last_play_album_pic_id(phandle->para_info.album_id, phandle->plist[select_item].img_id);
    }
    else
    {
        divoom_clock_id_info clock_info;

        clock_info.clock_id = phandle->para_info.album_id;
        clock_info.clock_pos = phandle->para_info.clock_pos;
        clock_info.parent_id = phandle->para_info.parent_id;
        divoom_custom_photo_update_pos_info(&clock_info, phandle->plist[select_item].img_id);
    }
    divoom_lvgl_photo_manage_start_play_album(phandle);
}

static void divoom_lvgl_photo_manage_action_view_photo(Pdivoom_lvgl_photo_manage_ctrl phandle)
{
    int select_item = divoom_lvgl_photo_manage_get_current_item(phandle);
    lv_obj_t *grid_btn;

    if (phandle->para_info.operate_type != DIVOOM_LVGL_PHOTO_MANAGE_CLOCK_VIEW_PICTURE)
    {
        phandle->photo_view_info.cur_pos = phandle->plist[select_item].img_pos;
        phandle->photo_view_info.pic_id = phandle->plist[select_item].img_id;
        phandle->photo_view_info.album_id = phandle->para_info.album_id;
        phandle->photo_view_info.clock_pos = phandle->para_info.clock_pos;
        phandle->photo_view_info.parent_id = phandle->para_info.parent_id;
        phandle->photo_view_info.pic_num = &phandle->pic_num;
        phandle->photo_view_info.pget_pic_handle = phandle->pget_pic_handle;
        phandle->photo_view_info.para = phandle;
        phandle->photo_view_info.pdel_callback = NULL;
        grid_btn = phandle->plist[select_item].btn_obj;
        if (grid_btn && lv_obj_is_valid(grid_btn))
        {
            phandle->last_active_obj = grid_btn;
            lv_group_focus_obj(grid_btn);
        }
        divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_PHOTO_VIEW, &phandle->photo_view_info, 0, 0);
        return;
    }

    if (phandle->para_info.album_id >= DIVOOM_CUSTOM_START_CLOCK_ID)
    {
        divoom_pic_album_manage_set_one_play_album_pic_id(phandle->para_info.album_id, phandle->plist[select_item].img_id);
    }
    else
    {
        divoom_clock_id_info clock_info;

        clock_info.clock_id = phandle->para_info.album_id;
        clock_info.clock_pos = phandle->para_info.clock_pos;
        clock_info.parent_id = phandle->para_info.parent_id;
        divoom_custom_photo_set_play_one(&clock_info, phandle->plist[select_item].img_id);
    }
    divoom_lvgl_photo_manage_start_play_album(phandle);
}

static int divoom_lvgl_photo_manage_get_delete_one_confirm_type(Pdivoom_lvgl_photo_manage_ctrl phandle)
{
    if (phandle->para_info.album_id >= DIVOOM_CUSTOM_START_CLOCK_ID && phandle->album_type != DIVOOM_PIC_ALBUM_TYPE_ALL)
    {
        return 3;
    }
    return 0;
}

static int divoom_lvgl_photo_manage_get_delete_all_confirm_type(Pdivoom_lvgl_photo_manage_ctrl phandle)
{
    if (phandle->para_info.album_id >= DIVOOM_CUSTOM_START_CLOCK_ID && phandle->album_type != DIVOOM_PIC_ALBUM_TYPE_ALL)
    {
        return 2;
    }
    return 1;
}

static void divoom_lvgl_photo_manage_action_pick_execute(Pdivoom_lvgl_photo_manage_ctrl phandle, int action_idx)
{
    if (!phandle)
    {
        return;
    }

    if (action_idx == 0)
    {
        divoom_lvgl_photo_manage_action_play_from_here(phandle);
        return;
    }
    if (action_idx == 1)
    {
        divoom_lvgl_photo_manage_action_view_photo(phandle);
        return;
    }
    if (action_idx == 2)
    {
        divoom_lvgl_photo_manage_create_confirm_dialog(phandle,
            divoom_lvgl_photo_manage_get_delete_one_confirm_type(phandle));
        return;
    }
    if (action_idx == 3)
    {
        divoom_lvgl_photo_manage_create_confirm_dialog(phandle,
            divoom_lvgl_photo_manage_get_delete_all_confirm_type(phandle));
        return;
    }
}

static void divoom_lvgl_photo_manage_action_pick_deferred_cb(lv_timer_t *timer)
{
    Pdivoom_lvgl_photo_manage_ctrl phandle;
    int action_idx;

    phandle = (Pdivoom_lvgl_photo_manage_ctrl)lv_timer_get_user_data(timer);
    if (!phandle)
    {
        return;
    }
    action_idx = phandle->action_pick_deferred_idx;
    phandle->action_pick_timer = NULL;
    lv_timer_del(timer);
    divoom_lvgl_photo_manage_action_pick_execute(phandle, action_idx);
}

static void divoom_lvgl_photo_manage_action_pick(Pdivoom_lvgl_photo_manage_ctrl phandle, int action_idx)
{
    if (!phandle)
    {
        return;
    }

    /* 勿在 LV_EVENT_KEY 回调内同步 lv_obj_del 当前按钮；延后执行具体操作 */
    divoom_lvgl_photo_manage_action_sheet_close(phandle, 0);
    phandle->action_pick_deferred_idx = action_idx;
    if (phandle->action_pick_timer)
    {
        lv_timer_del(phandle->action_pick_timer);
        phandle->action_pick_timer = NULL;
    }
    phandle->action_pick_timer = lv_timer_create(divoom_lvgl_photo_manage_action_pick_deferred_cb, 0, phandle);
    if (phandle->action_pick_timer)
    {
        lv_timer_set_repeat_count(phandle->action_pick_timer, 1);
    }
    else
    {
        divoom_lvgl_photo_manage_action_pick_execute(phandle, action_idx);
    }
}

static void divoom_lvgl_photo_manage_action_sheet_key_cb(lv_event_t *e)
{
    Pdivoom_lvgl_photo_manage_ctrl phandle;
    uint8_t key_code;
    lv_obj_t *next_obj;
    lv_obj_t *focused_obj;
    int i;
    int pick_idx;

    if (lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }
    phandle = (Pdivoom_lvgl_photo_manage_ctrl)lv_event_get_user_data(e);
    key_code = divoom_astrootoo_ui_key_code_from_event(e);

    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        divoom_lvgl_photo_manage_action_sheet_close(phandle, 1);
        return;
    }
    if (divoom_astrootoo_ui_key_is_up(key_code) || divoom_astrootoo_ui_key_is_down(key_code))
    {
        next_obj = divoom_menu_direct_ctrl_get(phandle->action_direct, lv_event_get_target_obj(e),
            divoom_astrootoo_ui_key_is_up(key_code) ? DIVOOM_MENU_DIRECT_UP : DIVOOM_MENU_DIRECT_DOWN);
        if (next_obj && next_obj != lv_event_get_target_obj(e))
        {
            lv_group_focus_obj(next_obj);
            for (i = 0; i < DIVOOM_LVGL_PHOTO_MANAGE_ACTION_CNT; i++)
            {
                if (next_obj == phandle->action_btns[i])
                {
                    phandle->action_focus = i;
                    break;
                }
            }
        }
        return;
    }
    if (divoom_astrootoo_ui_key_is_enter(key_code))
    {
        pick_idx = phandle->action_focus;
        focused_obj = lv_group_get_focused(lv_group_get_default());
        for (i = 0; i < DIVOOM_LVGL_PHOTO_MANAGE_ACTION_CNT; i++)
        {
            if (focused_obj == phandle->action_btns[i])
            {
                pick_idx = i;
                phandle->action_focus = i;
                break;
            }
        }
        divoom_lvgl_photo_manage_action_pick(phandle, pick_idx);
    }
}

static void divoom_lvgl_photo_manage_action_sheet_close(Pdivoom_lvgl_photo_manage_ctrl phandle, int restore_grid_focus)
{
    if (!phandle)
    {
        return;
    }
    if (phandle->action_direct)
    {
        divoom_menu_direct_ctrl_destory(phandle->action_direct);
        phandle->action_direct = NULL;
    }
    if (phandle->action_overlay)
    {
        int i;

        for (i = 0; i < DIVOOM_LVGL_PHOTO_MANAGE_ACTION_CNT; i++)
        {
            if (phandle->action_btns[i])
            {
                lv_group_remove_obj(phandle->action_btns[i]);
                phandle->action_btns[i] = NULL;
            }
        }
        lv_obj_del_async(phandle->action_overlay);
        phandle->action_overlay = NULL;
        phandle->action_sheet = NULL;
    }
    if (phandle->pane == DIVOOM_LVGL_PHOTO_MANAGE_PANE_MENU)
    {
        phandle->pane = DIVOOM_LVGL_PHOTO_MANAGE_PANE_GRID;
        if (restore_grid_focus)
        {
            divoom_lvgl_photo_manage_rebind_grid_group(phandle);
            if (phandle->last_active_obj && lv_obj_is_valid(phandle->last_active_obj)
                && divoom_lvgl_photo_manage_btn_is_visible(phandle, phandle->last_active_obj))
            {
                divoom_lvgl_photo_manage_grid_focus(phandle, phandle->last_active_obj);
            }
        }
    }
}

static void divoom_lvgl_photo_manage_action_sheet_open(Pdivoom_lvgl_photo_manage_ctrl phandle)
{
    lv_obj_t *sheet;
    lv_coord_t sheet_w;
    lv_coord_t sheet_h;
    int i;

    if (!phandle || phandle->pic_num == 0)
    {
        return;
    }

    divoom_lvgl_photo_manage_action_sheet_close(phandle, 0);
    phandle->last_active_obj = lv_group_get_focused(lv_group_get_default());
    divoom_lvgl_photo_manage_remove_grid_from_group(phandle);
    phandle->pane = DIVOOM_LVGL_PHOTO_MANAGE_PANE_MENU;
    phandle->action_focus = 0;
    phandle->action_direct = divoom_menu_direct_ctrl_create(1, DIVOOM_LVGL_PHOTO_MANAGE_ACTION_CNT);

    phandle->action_overlay = lv_obj_create(phandle->main_obj);
    lv_obj_remove_style_all(phandle->action_overlay);
    lv_obj_set_size(phandle->action_overlay, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_set_pos(phandle->action_overlay, 0, 0);
    lv_obj_set_style_bg_opa(phandle->action_overlay, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(phandle->action_overlay, 0, 0);
    lv_obj_set_style_pad_all(phandle->action_overlay, 0, 0);
    lv_obj_clear_flag(phandle->action_overlay, LV_OBJ_FLAG_SCROLLABLE);

    sheet_w = DIVOOM_LVGL_PHOTO_MANAGE_ACTION_SHEET_W;
    if (sheet_w > DIVOOM_SCREEN_WIDTH - 32)
    {
        sheet_w = DIVOOM_SCREEN_WIDTH - 32;
    }
    sheet_h = DIVOOM_LVGL_PHOTO_MANAGE_ACTION_SHEET_BORDER * 2
        + DIVOOM_LVGL_PHOTO_MANAGE_ACTION_SHEET_PAD * 2
        + DIVOOM_LVGL_PHOTO_MANAGE_ACTION_CNT * DIVOOM_LVGL_PHOTO_MANAGE_ACTION_ROW_H
        + (DIVOOM_LVGL_PHOTO_MANAGE_ACTION_CNT - 1) * DIVOOM_LVGL_PHOTO_MANAGE_ACTION_ROW_GAP;

    phandle->action_sheet = lv_obj_create(phandle->action_overlay);
    lv_obj_remove_style_all(phandle->action_sheet);
    lv_obj_set_size(phandle->action_sheet, sheet_w, sheet_h);
    lv_obj_center(phandle->action_sheet);
    lv_obj_set_style_bg_color(phandle->action_sheet, lv_color_hex(DIVOOM_ASTRO_MODAL_BG), 0);
    lv_obj_set_style_bg_opa(phandle->action_sheet, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(phandle->action_sheet, DIVOOM_LVGL_PHOTO_MANAGE_ACTION_SHEET_BORDER, 0);
    lv_obj_set_style_border_color(phandle->action_sheet, lv_color_hex(DIVOOM_ASTRO_STROKE), 0);
    lv_obj_set_style_radius(phandle->action_sheet, DIVOOM_LVGL_PHOTO_MANAGE_ACTION_SHEET_RADIUS, 0);
    lv_obj_set_style_pad_all(phandle->action_sheet, DIVOOM_LVGL_PHOTO_MANAGE_ACTION_SHEET_PAD, 0);
    lv_obj_set_style_pad_row(phandle->action_sheet, DIVOOM_LVGL_PHOTO_MANAGE_ACTION_ROW_GAP, 0);
    lv_obj_clear_flag(phandle->action_sheet, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(phandle->action_sheet, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(phandle->action_sheet, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    sheet = phandle->action_sheet;

    for (i = 0; i < DIVOOM_LVGL_PHOTO_MANAGE_ACTION_CNT; i++)
    {
        lv_obj_t *btn;
        lv_obj_t *lbl;

        btn = lv_btn_create(sheet);
        phandle->action_btns[i] = btn;
        lv_obj_add_style(btn, &phandle->ui.card_item_style, 0);
        lv_obj_add_style(btn, &phandle->ui.card_item_focus_style, LV_STATE_FOCUSED);
        lv_obj_add_style(btn, &phandle->ui.card_item_focus_style, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_border_width(btn, 2, 0);
        lv_obj_set_style_border_width(btn, 2, LV_STATE_FOCUSED);
        lv_obj_set_style_border_width(btn, 2, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUSED);
        lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUS_KEY);
        lv_obj_set_width(btn, LV_PCT(100));
        lv_obj_set_height(btn, DIVOOM_LVGL_PHOTO_MANAGE_ACTION_ROW_H);
        /* card_item_style 自带 pad=8；再叠 pad_hor/align 会重复缩进并撑破右边界 */
        lv_obj_set_style_pad_all(btn, 0, 0);
        lv_obj_set_style_pad_all(btn, 0, LV_STATE_FOCUSED);
        lv_obj_set_style_pad_all(btn, 0, LV_STATE_FOCUS_KEY);
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(btn, divoom_lvgl_photo_manage_action_sheet_key_cb, LV_EVENT_KEY, phandle);
        lv_group_add_obj(lv_group_get_default(), btn);
        divoom_menu_direct_ctrl_update(phandle->action_direct, btn, 0, i);

        lbl = lv_label_create(btn);
        lv_label_set_text(lbl, divoom_menu_control_get_disp_str(divoom_lvgl_photo_manage_action_keys[i]));
        lv_obj_set_style_text_color(lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
        lv_obj_set_style_text_font(lbl, divoom_menu_control_get_font_info(DIVOOM_LVGL_PHOTO_MANAGE_ACTION_FONT, phandle->menu_id), 0);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, DIVOOM_LVGL_PHOTO_MANAGE_ACTION_ROW_PAD_X, 0);
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_LEFT, 0);
        divoom_astrootoo_ui_list_label_prepare(lbl,
            divoom_astrootoo_ui_list_label_width(sheet_w
                    - DIVOOM_LVGL_PHOTO_MANAGE_ACTION_SHEET_PAD * 2,
                DIVOOM_LVGL_PHOTO_MANAGE_ACTION_ROW_PAD_X,
                DIVOOM_LVGL_PHOTO_MANAGE_ACTION_ROW_PAD_X, 0));
        divoom_astrootoo_ui_list_row_bind_marquee(btn, lbl);
    }

    phandle->action_focus = 0;
    lv_group_focus_obj(phandle->action_btns[0]);
    lv_obj_move_foreground(phandle->action_overlay);
}


void divoom_lvgl_photo_manage_start_play_album(Pdivoom_lvgl_photo_manage_ctrl phandle)
{
    if(phandle->para_info.operate_type == DIVOOM_LVGL_PHOTO_MANAGE_VIEW_PICTURE)
    {
        divoom_sys_msg_info model_msg= {0};
        int *clock_id = divoom_system_malloc(sizeof(int));

        *clock_id = phandle->para_info.album_id;
        model_msg.msg_type = 1;
    	model_msg.src_id = DIVOOM_MODEL_MAIN; 
    	model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
    	model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_SET_CLOCK_INDEX;
        model_msg.data = (uint8 *)clock_id;
    	divoom_sys_model_crl_msg_push(&model_msg);
    }
    else
    {
        divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_EVENT_START_UP_DISP_DIAL);
    }
}

static void divoom_lvgl_photo_manage_event_cb(lv_event_t * e)
{
    Pdivoom_lvgl_photo_manage_ctrl phandle;
    lv_obj_t *cur_obj;
    lv_obj_t *next_obj;
    uint8_t key_code;
    int cur_idx;
    int col;
    int row;
    int last_row;
    int focus_idx;

    if (lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    phandle = (Pdivoom_lvgl_photo_manage_ctrl)lv_event_get_user_data(e);
    {
        lv_obj_t *gf = lv_group_get_focused(lv_group_get_default());
        divoom_print("[grid_key] target=%p focused=%p pane=%d short=%d\n",
            (void *)lv_event_get_target_obj(e), (void *)gf, phandle->pane,
            divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)));
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }
    if (phandle->pane != DIVOOM_LVGL_PHOTO_MANAGE_PANE_GRID)
    {
        return;
    }
    cur_obj = lv_event_get_target_obj(e);
    key_code = divoom_astrootoo_ui_key_code_from_event(e);

    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        divoom_lvgl_photo_manage_schedule_esc_return(phandle);
        return;
    }

    if (divoom_astrootoo_ui_key_is_enter(key_code))
    {
        if (phandle->pic_num)
        {
            divoom_lvgl_photo_manage_action_sheet_open(phandle);
        }
        return;
    }

    if (phandle->pic_num == 0)
    {
        return;
    }

    cur_idx = divoom_lvgl_photo_manage_get_current_item(phandle);
    col = cur_idx % DIVOOM_LVGL_PHOTO_MANAGE_PIC_ROW_NUM;
    row = cur_idx / DIVOOM_LVGL_PHOTO_MANAGE_PIC_ROW_NUM;
    last_row = (phandle->pic_num_cur_load - 1) / DIVOOM_LVGL_PHOTO_MANAGE_PIC_ROW_NUM;

    if (divoom_astrootoo_ui_key_is_down(key_code))
    {
        if (phandle->pic_num_cur_load > 0
            && row == last_row
            && phandle->plist[phandle->pic_num_cur_load - 1].img_pos < phandle->pic_num - 1)
        {
            divoom_lvgl_photo_manage_reload_all_pic(phandle, 1);
            focus_idx = col;
            if (focus_idx >= phandle->pic_num_cur_load
                || !divoom_lvgl_photo_manage_btn_is_visible(phandle, phandle->plist[focus_idx].btn_obj))
            {
                for (focus_idx = 0; focus_idx < phandle->pic_num_cur_load; focus_idx++)
                {
                    if (divoom_lvgl_photo_manage_btn_is_visible(phandle, phandle->plist[focus_idx].btn_obj))
                    {
                        break;
                    }
                }
            }
            if (focus_idx < phandle->pic_num_cur_load
                && divoom_lvgl_photo_manage_btn_is_visible(phandle, phandle->plist[focus_idx].btn_obj))
            {
                divoom_lvgl_photo_manage_grid_focus(phandle, phandle->plist[focus_idx].btn_obj);
            }
            else
            {
                divoom_lvgl_photo_manage_ensure_grid_focus(phandle);
            }
            return;
        }
        next_obj = divoom_lvgl_photo_manage_grid_next(phandle, cur_obj, DIVOOM_MENU_DIRECT_DOWN);
    }
    else if (divoom_astrootoo_ui_key_is_up(key_code))
    {
        if (row == 0 && phandle->plist[0].img_pos > 0)
        {
            divoom_lvgl_photo_manage_reload_all_pic(phandle, 0);
            last_row = (phandle->pic_num_cur_load - 1) / DIVOOM_LVGL_PHOTO_MANAGE_PIC_ROW_NUM;
            focus_idx = last_row * DIVOOM_LVGL_PHOTO_MANAGE_PIC_ROW_NUM + col;
            if (focus_idx >= phandle->pic_num_cur_load)
            {
                focus_idx = phandle->pic_num_cur_load - 1;
            }
            divoom_lvgl_photo_manage_grid_focus(phandle, phandle->plist[focus_idx].btn_obj);
            return;
        }
        next_obj = divoom_lvgl_photo_manage_grid_next(phandle, cur_obj, DIVOOM_MENU_DIRECT_UP);
    }
    else if (divoom_astrootoo_ui_key_is_left(key_code))
    {
        next_obj = divoom_lvgl_photo_manage_grid_next(phandle, cur_obj, DIVOOM_MENU_DIRECT_LEFT);
    }
    else if (divoom_astrootoo_ui_key_is_right(key_code))
    {
        next_obj = divoom_lvgl_photo_manage_grid_next(phandle, cur_obj, DIVOOM_MENU_DIRECT_RIGHT);
    }
    else
    {
        return;
    }

    if (next_obj && next_obj != cur_obj)
    {
        divoom_lvgl_photo_manage_grid_focus(phandle, next_obj);
    }
    divoom_lvgl_photo_manage_ensure_grid_focus(phandle);
}

void divoom_lvgl_photo_manage_request_esc_return(void *handle)
{
    Pdivoom_lvgl_photo_manage_ctrl phandle;

    phandle = (Pdivoom_lvgl_photo_manage_ctrl)handle;
    if (!phandle)
    {
        return;
    }
    if (phandle->pane == DIVOOM_LVGL_PHOTO_MANAGE_PANE_MENU
        || phandle->pane == DIVOOM_LVGL_PHOTO_MANAGE_PANE_CONFIRM)
    {
        return;
    }
    divoom_lvgl_photo_manage_schedule_esc_return(phandle);
}

int divoom_lvgl_photo_manage_try_main_esc_return(void *handle)
{
    Pdivoom_lvgl_photo_manage_ctrl phandle;
    int select_num;

    phandle = (Pdivoom_lvgl_photo_manage_ctrl)handle;
    if (!phandle)
    {
        divoom_menu_control_return_pre();
        return 1;
    }
    select_num = divoom_lvgl_photo_manage_get_select_items(phandle);
    divoom_print("[photo_esc] pane=%d sel=%d pending=%d\n",
        phandle->pane, select_num, phandle->esc_return_pending);
    if (phandle->pane == DIVOOM_LVGL_PHOTO_MANAGE_PANE_MENU
        || phandle->pane == DIVOOM_LVGL_PHOTO_MANAGE_PANE_CONFIRM)
    {
        return 0;
    }
    if (select_num > 0)
    {
        return 0;
    }
    if (phandle->esc_return_pending)
    {
        return 1;
    }
    divoom_lvgl_photo_manage_schedule_esc_return(phandle);
    return 1;
}

void divoom_lvgl_photo_manage_update_all_pic_info(Pdivoom_lvgl_photo_manage_ctrl phandle)
{
    int i, j;

    if (!phandle || !phandle->plist[0].image_data)
    {
        return;
    }

    if(phandle->pic_num == 0)
    {
        //所有照片全部删除完了，将退出
        divoom_menu_control_return_pre();
        return ;
    }

    /* 以当前页首格为准重载，不要用读完一页后的 read_pic_pos 再减 27。
     * 旧逻辑：第 2 页未满时 read_pic_pos 对齐到 27，再走 >=27 分支减成 0，整页跳回第 1 页。 */
    {
        int page_size = DIVOOM_LVGL_PHOTO_MANAGE_PIC_NUM_PRE_SCREEN;
        int page_start = phandle->plist[0].img_pos;

        if (page_start < 0)
        {
            page_start = 0;
        }
        page_start = (page_start / page_size) * page_size;
        while (page_start >= phandle->pic_num && page_start >= page_size)
        {
            page_start -= page_size;
        }
        if (page_start < 0)
        {
            page_start = 0;
        }
        phandle->read_pic_pos = page_start;
    }
    divoom_print("load start pos:%d, %d\n", phandle->read_pic_pos, phandle->pic_num);     
    memset(phandle->select_table, 0, phandle->pic_num);
    phandle->pic_num_cur_load = 0;
    for(i = 0; i <DIVOOM_LVGL_PHOTO_MANAGE_PIC_NUM_PRE_SCREEN; i++)
    {
        //如果在这删除位置后面的图像，要全部重新加载
        if(phandle->read_pic_pos < phandle->pic_num)
        {    
            phandle->plist[i].img_pos = phandle->read_pic_pos;
            if(phandle->para_info.album_id >= DIVOOM_CUSTOM_START_CLOCK_ID)
            {
                phandle->plist[i].img_id = divoom_pic_album_manage_get_disp_order_pic_id(phandle->pget_pic_handle,phandle->plist[i].img_pos, NULL);
            }
            else
            {
                phandle->plist[i].img_id = divoom_custom_photo_get_pic_id(phandle->pget_pic_handle,phandle->plist[i].img_pos);
            }
            
            if(divoom_lvgl_photo_manage_pic_file_exists(phandle, phandle->plist[i].img_id)
                && divoom_lvgl_photo_manage_read_pic(phandle, phandle->plist[i].img_pos,phandle->plist[i].img_id, 0,
                (uint8 *)phandle->plist[i].image_data->data,  divoom_lvgl_photo_manage_update_finish_read_over_callback, phandle->plist[i].img_obj))
            {
                
                
                //divoom_print("update image!%d, %d, %d\n", i, j,phandle->plist[i].img_id[j]);
                lv_obj_invalidate(phandle->plist[i].img_obj);
                lv_obj_clear_flag(phandle->plist[i].btn_obj, LV_OBJ_FLAG_HIDDEN);
            }
            else
            {
                /* 删除或同步后残留的失效记录只占数据位置，不占可见网格位置。 */
                lv_obj_add_flag(phandle->plist[i].btn_obj, LV_OBJ_FLAG_HIDDEN);
            }
            lv_obj_set_style_img_opa(phandle->plist[i].img_obj, LV_OPA_100, 0);
            divoom_lvgl_photo_manage_apply_cell_style(phandle, i);
            if(!lv_obj_has_flag(phandle->plist[i].btn_obj, LV_OBJ_FLAG_HIDDEN)
                && divoom_lvgl_photo_manage_get_video_flag(phandle->plist[i].img_id, phandle->para_info.album_id))
            {
                lv_obj_clear_flag(phandle->plist[i].video_obj, LV_OBJ_FLAG_HIDDEN);
            }
            else
            {
                lv_obj_add_flag(phandle->plist[i].video_obj, LV_OBJ_FLAG_HIDDEN);
            }
            phandle->pic_num_cur_load ++;    
            phandle->read_pic_pos ++;
            
        }
        else
        {
            
            phandle->plist[i].img_id = 0;
            phandle->plist[i].img_pos = -1;
            lv_obj_add_flag(phandle->plist[i].btn_obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(phandle->plist[i].video_obj, LV_OBJ_FLAG_HIDDEN);
        }
    }
    
    
}

//删除当前相簿
static void divoom_lvgl_photo_manage_delete_album(Pdivoom_lvgl_photo_manage_ctrl phandle)
{
    Pdivoom_pic_phone_manage_slide_config pslide_config;
    divoom_lvgl_photo_slide_effect_para *play_info;

    pslide_config = divoom_pic_phone_manage_get_slide_config();
    if(pslide_config->album_id == phandle->para_info.album_id)
    {
        pslide_config->album_id = divoom_pic_album_manage_get_album_id_from_type(DIVOOM_PIC_ALBUM_TYPE_ALL);
    }
    divoom_pic_album_manage_del_type(phandle->para_info.album_id, 1);
    divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_PHOTO_SLIDE_EFFECT, NULL, 0, 0);   
}

//删除所有选择的照片
int divoom_lvgl_photo_manage_delete_picture(Pdivoom_lvgl_photo_manage_ctrl phandle)
{
    uint32 pic_id;
    int select_item = divoom_lvgl_photo_manage_get_current_item(phandle);

    pic_id = phandle->plist[select_item].img_id;
    
    //将相关的图片数据更新到列表中   
    if(phandle->para_info.album_id >= DIVOOM_CUSTOM_START_CLOCK_ID)
    {         
        divoom_pic_album_manage_remove_pic_id_from_disp_handle(phandle->pget_pic_handle, pic_id);
        divoom_pic_phone_manage_remove_pic_list(&pic_id, 1);
    }
    else
    {
        divoom_clock_id_info  clock_info ={0};

        clock_info.clock_id = phandle->para_info.album_id;
        clock_info.clock_pos = phandle->para_info.clock_pos;
        clock_info.parent_id = phandle->para_info.parent_id;
        divoom_custom_photo_delete_item(&clock_info, pic_id);
    }
    phandle->pic_num --;
    divoom_lvgl_photo_manage_update_all_pic_info(phandle);
        
    //修改激活项
    void *find_last_obj = NULL;
    
    for(int i = 0; i < DIVOOM_LVGL_PHOTO_MANAGE_PIC_NUM_PRE_SCREEN; i++)
    {
        if(phandle->plist[i].img_pos != -1)
        {
            find_last_obj = phandle->plist[i].btn_obj;
        }
        if(phandle->last_active_obj == phandle->plist[i].btn_obj)
        {
            if(phandle->plist[i].img_pos == -1)
            {
                phandle->last_active_obj = find_last_obj;
            }
            break;
        }
    
    }
    return 1;
}


//从相簿中移除选择照片
int divoom_lvgl_photo_manage_remove_picture_from_album(Pdivoom_lvgl_photo_manage_ctrl phandle)
{
    uint32 pic_id;
    int select_item = divoom_lvgl_photo_manage_get_current_item(phandle);
    void *find_last_obj = NULL;
    int i;

    pic_id = phandle->plist[select_item].img_id;
    if (pic_id == 0)
    {
        return 0;
    }

    divoom_pic_album_manage_remove_list_from_id(phandle->para_info.album_id, &pic_id, 1);
    divoom_pic_album_manage_remove_pic_id_from_disp_handle(phandle->pget_pic_handle, pic_id);
    phandle->pic_num = divoom_pic_album_manage_get_disp_order_pic_num(phandle->pget_pic_handle);
    divoom_lvgl_photo_manage_update_all_pic_info(phandle);

    for (i = 0; i < DIVOOM_LVGL_PHOTO_MANAGE_PIC_NUM_PRE_SCREEN; i++)
    {
        if (phandle->plist[i].img_pos != -1)
        {
            find_last_obj = phandle->plist[i].btn_obj;
        }
        if (phandle->last_active_obj == phandle->plist[i].btn_obj)
        {
            if (phandle->plist[i].img_pos == -1)
            {
                phandle->last_active_obj = find_last_obj;
            }
            break;
        }
    }
    return 1;
}

//从相簿中移除所有照片
void divoom_lvgl_photo_manage_remove_all_picture_from_album(Pdivoom_lvgl_photo_manage_ctrl phandle)
{
    int select_num = 0, i, *pic_list;

    //将相关的图片数据更新到列表中    
    pic_list  = divoom_system_malloc(sizeof(int) *phandle->pic_num);
    for(i = 0; i < phandle->pic_num; i++)
    {
        pic_list[select_num] = divoom_pic_album_manage_get_disp_order_pic_id(phandle->pget_pic_handle,i, NULL);
        select_num ++;    
    }
    
    
    for (i = 0; i < select_num; i++)
    {
        divoom_pic_album_manage_remove_pic_id_from_disp_handle(phandle->pget_pic_handle, pic_list[i]);
    }
    divoom_pic_album_manage_remove_list_from_id(phandle->para_info.album_id, pic_list, select_num);
    divoom_system_free(pic_list);
}

void divoom_lvgl_photo_manage_add_picture(Pdivoom_lvgl_photo_manage_ctrl phandle)
{        
    int select_num = 0, i, *pic_list;
    
    for(i = 0; i < phandle->pic_num; i++)
    {
        if(phandle->select_table[i])
        {
            select_num ++;
        }
    }
    if(select_num == 0)
    {
        return;
    }
    divoom_print("add picture num: %d, %d\n", phandle->para_info.album_id, select_num);

    
    pic_list = divoom_system_malloc(select_num * sizeof(uint32));
    for(i = 0, select_num = 0; i < phandle->pic_num; i++)
    {
        if(phandle->select_table[i])
        {
            pic_list[select_num] = divoom_pic_album_manage_get_disp_order_pic_id(phandle->pget_pic_handle,i, NULL);
            select_num ++;
        }
    }
    if(select_num)
    {
        divoom_pic_album_manage_add_list_to_name(phandle->para_info.album_id, pic_list, select_num);
    }
    divoom_system_free(pic_list);
}

int divoom_lvgl_photo_manage_get_select_items(Pdivoom_lvgl_photo_manage_ctrl phandle)
{   
    int select_num = 0, i;
    for(i = 0; i < phandle->pic_num; i++)
    {
        if(phandle->select_table[i])
        {
            select_num ++;
        }
    }
    return select_num;
}

int divoom_lvgl_photo_manage_add_more_to_albums(Pdivoom_lvgl_photo_manage_ctrl phandle)
{        
    int select_num = 0, i, *pic_list;
    
    for(i = 0; i < phandle->pic_num; i++)
    {
        if(phandle->select_table[i])
        {
            select_num ++;
        }
    }
    if(select_num == 0)
    {
        
        return 0;
    }
    divoom_print("add picture num: %d, %d\n", phandle->para_info.album_id, select_num);

    
    pic_list = divoom_system_malloc(select_num * sizeof(uint32));
    for(i = 0, select_num = 0; i < phandle->pic_num; i++)
    {
        if(phandle->select_table[i])
        {
            pic_list[select_num] = divoom_pic_album_manage_get_disp_order_pic_id(phandle->pget_pic_handle,i, NULL);
            select_num ++;
        }
    }
    
    if(select_num)
    {
        
        phandle->padd_album_info.pic_list = pic_list;
        phandle->padd_album_info.pic_num = select_num;
        phandle->padd_album_info.type = 1;
        divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_ADD_PICTURE_TO_ALBUM, &phandle->padd_album_info, 0, 0);
    }
    return 1;
}

int divoom_lvgl_photo_manage_get_video_flag(int pic_id, int album_id)
{
    char file_name[256];

    if(pic_id <= 0)
    {
        return 0;
    }
    if(album_id >= DIVOOM_CUSTOM_START_CLOCK_ID)
    {
        divoom_database_manage_get_pic_path_name(pic_id, file_name);
    }
    else
    {
        divoom_database_manage_get_custom_photo_name(pic_id, file_name);
    }

    return strstr(file_name, ".mp4") != NULL;
}


void *divoom_lvgl_photo_manage_init(void *user_data, int menu_id)
{
    char album_name[256];
    int i, j, off, read_pic_pos = 0;
    Pdivoom_lvgl_photo_manage_ctrl phandle;
    Pdivoom_pic_phone_manage_item_offset pitem_info;
    Pdivoom_pic_phone_manage_slide_config pslide_config = divoom_pic_phone_manage_get_slide_config();
    
    divoom_thumb_cache_set_dest_pic_size(DIVOOM_LVGL_PHOTO_MANAGE_PIC_WIDTH, DIVOOM_LVGL_PHOTO_MANAGE_PIC_HIGH);

    phandle = divoom_system_malloc(sizeof(divoom_lvgl_photo_manage_ctrl));
    memset(phandle, 0, sizeof(*phandle));
    phandle->menu_id = menu_id;
    phandle->pane = DIVOOM_LVGL_PHOTO_MANAGE_PANE_GRID;
    if(user_data)
    {
        memcpy(&phandle->para_info, user_data, sizeof(phandle->para_info));
        divoom_system_free(user_data);
    }
    else
    {
        phandle->para_info.album_id = divoom_pic_album_manage_get_album_id_from_type(DIVOOM_PIC_ALBUM_TYPE_ALL);
        phandle->para_info.operate_type = DIVOOM_LVGL_PHOTO_MANAGE_VIEW_PICTURE;
    }
    phandle->pdriect_menu_ctrl = divoom_menu_direct_ctrl_create(DIVOOM_LVGL_PHOTO_MANAGE_PIC_ROW_NUM, DIVOOM_LVGL_PHOTO_MANAGE_PIC_LIST_NUM);
    phandle->first_load_flag = 1;

    lv_style_init(&phandle->btn_press_type);
    lv_style_init(&phandle->btn_normal_type);
    lv_style_set_bg_opa(&phandle->btn_normal_type, LV_OPA_TRANSP);
    lv_style_set_border_width(&phandle->btn_normal_type, 0);
    lv_style_set_pad_all(&phandle->btn_normal_type, 0);
    lv_style_set_shadow_width(&phandle->btn_normal_type, 0);
    lv_style_set_outline_width(&phandle->btn_normal_type, 0);
    divoom_menu_control_init_btn_stype(&phandle->btn_select_type, NULL, NULL, NULL, NULL, NULL, 14);
    lv_style_set_border_color(&phandle->btn_select_type, lv_color_hex(0xDEB100));
    lv_style_set_border_width(&phandle->btn_select_type, DIVOOM_ASTRO_ITEM_BORDER_W_ACTIVE);

    phandle->main_obj = divoom_astrootoo_ui_create_root(&phandle->ui);

    divoom_lvgl_photo_manage_fill_album_title(phandle, album_name, sizeof(album_name));
    phandle->header_obj = divoom_astrootoo_ui_create_subpage_header_text(phandle->main_obj, &phandle->ui,
        album_name, phandle->menu_id);

    phandle->scroll_obj = divoom_astrootoo_ui_create_scroll_content(phandle->main_obj, &phandle->ui);
    lv_obj_set_scrollbar_mode(phandle->scroll_obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_top(phandle->scroll_obj, DIVOOM_LVGL_PHOTO_MANAGE_SCROLL_PAD_TOP, 0);
    lv_obj_set_style_pad_bottom(phandle->scroll_obj, DIVOOM_LVGL_PHOTO_MANAGE_SCROLL_PAD_BOTTOM, 0);
    lv_obj_set_style_pad_left(phandle->scroll_obj, DIVOOM_LVGL_PHOTO_MANAGE_SCROLL_PAD_X, 0);
    lv_obj_set_style_pad_right(phandle->scroll_obj, DIVOOM_LVGL_PHOTO_MANAGE_SCROLL_PAD_X, 0);

    phandle->image_main_cout_obj = lv_obj_create(phandle->scroll_obj);
    lv_obj_add_style(phandle->image_main_cout_obj, &phandle->ui.grid_cont_style, 0);
    lv_obj_set_size(phandle->image_main_cout_obj, DIVOOM_LVGL_PHOTO_MANAGE_GRID_W,
        DIVOOM_LVGL_PHOTO_MANAGE_PIC_LIST_NUM * DIVOOM_LVGL_PHOTO_MANAGE_CELL_STEP - DIVOOM_LVGL_PHOTO_MANAGE_PIC_GAP);
    lv_obj_set_pos(phandle->image_main_cout_obj, DIVOOM_LVGL_PHOTO_MANAGE_GRID_INNER_X, 0);
    lv_obj_clear_flag(phandle->image_main_cout_obj, LV_OBJ_FLAG_SCROLLABLE);

    if(phandle->para_info.album_id >= DIVOOM_CUSTOM_START_CLOCK_ID)
    {
    
        if(DIVOOM_LVGL_PHOTO_MANAGE_ADD_PICTURE == phandle->para_info.operate_type)
        {
            //添加照片默认是全部
            phandle->album_type = DIVOOM_PIC_ALBUM_TYPE_ALL;
            phandle->pget_pic_handle = divoom_pic_album_manage_create_disp_order_handle_from_start(divoom_pic_album_manage_get_album_id_from_type(DIVOOM_PIC_ALBUM_TYPE_ALL), DIVOOM_PIC_PHOTO_SLIDE_DISP_SAVE_TIME, 0, 1);
        }
        else
        {
            phandle->pget_pic_handle = divoom_pic_album_manage_create_disp_order_handle_from_start(phandle->para_info.album_id, pslide_config->disp_order!=DIVOOM_PIC_PHOTO_SLIDE_DISP_PHTOT_TIME?DIVOOM_PIC_PHOTO_SLIDE_DISP_SAVE_TIME:DIVOOM_PIC_PHOTO_SLIDE_DISP_PHTOT_TIME, pslide_config->disp_reverse, 1);
            if(phandle->para_info.album_id == divoom_pic_album_manage_get_album_id_from_type(DIVOOM_PIC_ALBUM_TYPE_ALL))
            {
                phandle->album_type = DIVOOM_PIC_ALBUM_TYPE_ALL;
            }
            else
            {
                phandle->album_type = DIVOOM_PIC_ALBUM_TYPE_NORMAL;
            }
        }
        
        //默认按照片时间的从新到旧排序
        
        phandle->pic_num = divoom_pic_album_manage_get_disp_order_pic_num(phandle->pget_pic_handle);
        divoom_thumb_cache_set_clock_info(NULL);
    }
    else
    {
        divoom_clock_id_info clock_info;

        clock_info.clock_id = phandle->para_info.album_id;
        clock_info.clock_pos = phandle->para_info.clock_pos;
        clock_info.parent_id = phandle->para_info.parent_id;
        phandle->pget_pic_handle = divoom_custom_photo_open(&clock_info, DIVOOM_PIC_PHOTO_SLIDE_DISP_SAVE_TIME, 0, NULL);
        phandle->pic_num = divoom_custom_photo_get_total_items(phandle->pget_pic_handle);
        divoom_thumb_cache_set_clock_info(&clock_info);
    }

    phandle->select_table = divoom_system_malloc(phandle->pic_num);
    if(phandle->para_info.pic_id != 0)
    {
        if(phandle->para_info.album_id >= DIVOOM_CUSTOM_START_CLOCK_ID)
        {
            divoom_pic_album_manage_get_pic_info_from_pic_id(phandle->pget_pic_handle, phandle->para_info.pic_id, &read_pic_pos, NULL);
        }
        else
        {
            read_pic_pos = divoom_custom_photo_get_pic_pos(phandle->pget_pic_handle, phandle->para_info.pic_id);
        }
        phandle->read_pic_pos = read_pic_pos / DIVOOM_LVGL_PHOTO_MANAGE_PIC_NUM_PRE_SCREEN *DIVOOM_LVGL_PHOTO_MANAGE_PIC_NUM_PRE_SCREEN;
        read_pic_pos = read_pic_pos % DIVOOM_LVGL_PHOTO_MANAGE_PIC_NUM_PRE_SCREEN; 
    }
    else
    {
        phandle->read_pic_pos = 0;
        read_pic_pos = 0;
    }
    
    lv_img_cache_invalidate_src(NULL);
    phandle->last_active_obj = NULL;

    for (i = 0, off = 0; i < DIVOOM_LVGL_PHOTO_MANAGE_PIC_LIST_NUM; i++)
    {
        for (j = 0; j < DIVOOM_LVGL_PHOTO_MANAGE_PIC_ROW_NUM; j++, off++)
        {
            phandle->plist[off].btn_obj = lv_btn_create(phandle->image_main_cout_obj);
            lv_obj_clear_flag(phandle->plist[off].btn_obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_pos(phandle->plist[off].btn_obj,
                j * DIVOOM_LVGL_PHOTO_MANAGE_CELL_STEP,
                i * DIVOOM_LVGL_PHOTO_MANAGE_CELL_STEP);
            lv_obj_set_size(phandle->plist[off].btn_obj,
                DIVOOM_LVGL_PHOTO_MANAGE_CELL_OUTER, DIVOOM_LVGL_PHOTO_MANAGE_CELL_OUTER);
            lv_obj_add_style(phandle->plist[off].btn_obj, &phandle->btn_normal_type, 0);
            lv_obj_set_style_bg_opa(phandle->plist[off].btn_obj, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(phandle->plist[off].btn_obj, 0, 0);
            lv_obj_set_style_radius(phandle->plist[off].btn_obj, DIVOOM_LVGL_PHOTO_MANAGE_CELL_RADIUS, 0);
            lv_obj_set_style_pad_all(phandle->plist[off].btn_obj, 0, 0);
            lv_obj_set_style_shadow_width(phandle->plist[off].btn_obj, 0, 0);
            lv_obj_set_style_outline_width(phandle->plist[off].btn_obj, 0, LV_STATE_FOCUSED);
            lv_obj_set_style_outline_width(phandle->plist[off].btn_obj, 0, LV_STATE_FOCUS_KEY);

            phandle->plist[off].thumb_cont = lv_obj_create(phandle->plist[off].btn_obj);
            lv_obj_add_style(phandle->plist[off].thumb_cont, &phandle->ui.content_style, 0);
            lv_obj_set_size(phandle->plist[off].thumb_cont,
                DIVOOM_LVGL_PHOTO_MANAGE_PIC_WIDTH, DIVOOM_LVGL_PHOTO_MANAGE_PIC_HIGH);
            lv_obj_center(phandle->plist[off].thumb_cont);
            lv_obj_set_style_bg_opa(phandle->plist[off].thumb_cont, LV_OPA_TRANSP, 0);
            lv_obj_set_style_radius(phandle->plist[off].thumb_cont, DIVOOM_LVGL_PHOTO_MANAGE_THUMB_RADIUS, 0);
            lv_obj_set_style_border_width(phandle->plist[off].thumb_cont, 0, 0);
            lv_obj_clear_flag(phandle->plist[off].thumb_cont, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_clear_flag(phandle->plist[off].thumb_cont, LV_OBJ_FLAG_CLICKABLE);

            phandle->plist[off].thumb_clip = lv_obj_create(phandle->plist[off].thumb_cont);
            lv_obj_add_style(phandle->plist[off].thumb_clip, &phandle->ui.content_style, 0);
            lv_obj_set_size(phandle->plist[off].thumb_clip,
                DIVOOM_LVGL_PHOTO_MANAGE_PIC_WIDTH, DIVOOM_LVGL_PHOTO_MANAGE_PIC_HIGH);
            lv_obj_set_style_bg_opa(phandle->plist[off].thumb_clip, LV_OPA_TRANSP, 0);
            lv_obj_set_style_radius(phandle->plist[off].thumb_clip, DIVOOM_LVGL_PHOTO_MANAGE_THUMB_RADIUS, 0);
            lv_obj_set_style_clip_corner(phandle->plist[off].thumb_clip, true, 0);
            lv_obj_set_style_border_width(phandle->plist[off].thumb_clip, 0, 0);
            lv_obj_set_style_pad_all(phandle->plist[off].thumb_clip, 0, 0);
            lv_obj_clear_flag(phandle->plist[off].thumb_clip, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_clear_flag(phandle->plist[off].thumb_clip, LV_OBJ_FLAG_CLICKABLE);

            phandle->plist[off].img_obj = lv_img_create(phandle->plist[off].thumb_clip);
            lv_obj_center(phandle->plist[off].img_obj);

            phandle->plist[off].image_data = divoom_system_malloc(sizeof(lv_img_dsc_t));
            divoom_lv_img_dsc_init_fmt(phandle->plist[off].image_data,
                DIVOOM_LVGL_PHOTO_MANAGE_PIC_WIDTH,
                DIVOOM_LVGL_PHOTO_MANAGE_PIC_HIGH,
                LV_COLOR_FORMAT_RGB565A8, NULL);
            phandle->plist[off].image_data->data = divoom_system_malloc(phandle->plist[off].image_data->data_size);
            lv_obj_set_size(phandle->plist[off].img_obj, DIVOOM_LVGL_PHOTO_MANAGE_PIC_WIDTH, DIVOOM_LVGL_PHOTO_MANAGE_PIC_HIGH);
            lv_img_set_src(phandle->plist[off].img_obj, phandle->plist[off].image_data);

            if (phandle->read_pic_pos < phandle->pic_num)
            {
                phandle->pic_num_cur_load = off + 1;
                phandle->plist[off].img_pos = phandle->read_pic_pos;
                if (phandle->para_info.album_id >= DIVOOM_CUSTOM_START_CLOCK_ID)
                {
                    phandle->plist[off].img_id = divoom_pic_album_manage_get_disp_order_pic_id(phandle->pget_pic_handle, phandle->read_pic_pos, NULL);
                }
                else
                {
                    phandle->plist[off].img_id = divoom_custom_photo_get_pic_id(phandle->pget_pic_handle, phandle->read_pic_pos);
                }
                phandle->read_pic_pos++;
                phandle->photo_view_info.pic_id = phandle->plist[off].img_id;
                if (!divoom_lvgl_photo_manage_pic_file_exists(phandle, phandle->plist[off].img_id)
                    || !divoom_lvgl_photo_manage_read_pic(phandle, phandle->plist[off].img_pos, phandle->plist[off].img_id, 1,
                        (uint8 *)phandle->plist[off].image_data->data, NULL, NULL))
                {
                    lv_obj_add_flag(phandle->plist[off].btn_obj, LV_OBJ_FLAG_HIDDEN);
                }
            }
            else if (phandle->pic_num == 0 && i == 0 && j == 0)
            {
                lv_obj_set_size(phandle->plist[off].img_obj, 44, 44);
                divoom_menu_control_lv_img_set_src(phandle->plist[off].img_obj, "1/1_add.png");
                lv_obj_center(phandle->plist[off].img_obj);
            }
            else
            {
                phandle->plist[off].img_pos = -1;
                divoom_print("over image and empty: %d, %d\n", i, j);
                pitem_info = NULL;
                phandle->plist[off].img_id = 0;
                lv_obj_add_flag(phandle->plist[off].btn_obj, LV_OBJ_FLAG_HIDDEN);
            }
            divoom_lvgl_photo_manage_apply_cell_style(phandle, off);
            divoom_menu_direct_ctrl_update(phandle->pdriect_menu_ctrl, phandle->plist[off].btn_obj, j, i);

            phandle->plist[off].video_obj = lv_img_create(phandle->plist[off].btn_obj);
            lv_obj_set_size(phandle->plist[off].video_obj, 32, 32);
            lv_obj_set_pos(phandle->plist[off].video_obj, 108, 108);
            divoom_menu_control_lv_img_set_src(phandle->plist[off].video_obj, "2/2_photo_video.png");
            if (divoom_lvgl_photo_manage_get_video_flag(phandle->plist[off].img_id, phandle->para_info.album_id))
            {
                lv_obj_clear_flag(phandle->plist[off].video_obj, LV_OBJ_FLAG_HIDDEN);
            }
            else
            {
                lv_obj_add_flag(phandle->plist[off].video_obj, LV_OBJ_FLAG_HIDDEN);
            }

            lv_obj_add_event_cb(phandle->plist[off].btn_obj, divoom_lvgl_photo_manage_event_cb, LV_EVENT_KEY, phandle);
            lv_obj_add_event_cb(phandle->plist[off].btn_obj, divoom_lvgl_photo_manage_cell_focus_cb, LV_EVENT_ALL, phandle);
            lv_group_add_obj(lv_group_get_default(), phandle->plist[off].btn_obj);
        }
    }
    phandle->last_active_obj = phandle->plist[read_pic_pos].btn_obj;
    if (phandle->pic_num_cur_load > 0
        && divoom_lvgl_photo_manage_btn_is_visible(phandle, phandle->plist[read_pic_pos].btn_obj))
    {
        divoom_lvgl_photo_manage_grid_focus(phandle, phandle->plist[read_pic_pos].btn_obj);
    }
    


    phandle->dir = 1;
    divoom_lvgl_photo_manage_cache_load(phandle, phandle->read_pic_pos + DIVOOM_LVGL_PHOTO_MANAGE_PIC_NUM_PRE_SCREEN);
    //phandle->update_image_timer = lv_timer_create(divoom_lvgl_photo_manage_update_image_time_task, 10, phandle);

    

    return phandle;
}

static void divoom_lvgl_photo_manage_release_grid_bufs(Pdivoom_lvgl_photo_manage_ctrl phandle)
{
    int i;

    if (!phandle)
    {
        return;
    }
    divoom_thumb_cache_clear_all();
    for (i = 0; i < DIVOOM_LVGL_PHOTO_MANAGE_PIC_NUM_PRE_SCREEN; i++)
    {
        if (phandle->plist[i].img_obj && lv_obj_is_valid(phandle->plist[i].img_obj))
        {
            lv_img_set_src(phandle->plist[i].img_obj, NULL);
        }
        if (phandle->plist[i].image_data)
        {
            if (phandle->plist[i].image_data->data)
            {
                divoom_system_free((uint8 *)phandle->plist[i].image_data->data);
                phandle->plist[i].image_data->data = NULL;
            }
            divoom_system_free(phandle->plist[i].image_data);
            phandle->plist[i].image_data = NULL;
        }
    }
}

static int divoom_lvgl_photo_manage_alloc_grid_bufs(Pdivoom_lvgl_photo_manage_ctrl phandle)
{
    int i;

    if (!phandle)
    {
        return 0;
    }
    for (i = 0; i < DIVOOM_LVGL_PHOTO_MANAGE_PIC_NUM_PRE_SCREEN; i++)
    {
        if (phandle->plist[i].image_data)
        {
            continue;
        }
        phandle->plist[i].image_data = divoom_system_malloc(sizeof(lv_img_dsc_t));
        if (!phandle->plist[i].image_data)
        {
            goto fail;
        }
        divoom_lv_img_dsc_init_fmt(phandle->plist[i].image_data,
            DIVOOM_LVGL_PHOTO_MANAGE_PIC_WIDTH,
            DIVOOM_LVGL_PHOTO_MANAGE_PIC_HIGH,
            LV_COLOR_FORMAT_RGB565A8, NULL);
        phandle->plist[i].image_data->data = divoom_system_malloc(phandle->plist[i].image_data->data_size);
        if (!phandle->plist[i].image_data->data)
        {
            divoom_system_free(phandle->plist[i].image_data);
            phandle->plist[i].image_data = NULL;
            goto fail;
        }
        if (phandle->plist[i].img_obj && lv_obj_is_valid(phandle->plist[i].img_obj))
        {
            lv_obj_set_size(phandle->plist[i].img_obj,
                DIVOOM_LVGL_PHOTO_MANAGE_PIC_WIDTH, DIVOOM_LVGL_PHOTO_MANAGE_PIC_HIGH);
            lv_img_set_src(phandle->plist[i].img_obj, phandle->plist[i].image_data);
        }
    }
    return 1;

fail:
    divoom_lvgl_photo_manage_release_grid_bufs(phandle);
    divoom_print("[pm] alloc grid bufs fail\n");
    return 0;
}

void divoom_lvgl_photo_manage_pause(void *handle)
{
    Pdivoom_lvgl_photo_manage_ctrl phandle = (Pdivoom_lvgl_photo_manage_ctrl)handle;
    lv_obj_t *focused;

    divoom_print("[pm] pause enter\n");
    divoom_lvgl_photo_manage_release_grid_bufs(phandle);
    divoom_print("[pm] pause after release grid bufs\n");
    if (phandle->action_pick_timer)
    {
        lv_timer_del(phandle->action_pick_timer);
        phandle->action_pick_timer = NULL;
    }
    divoom_lvgl_photo_manage_action_sheet_close(phandle, 0);
    divoom_lvgl_photo_manage_confirm_close(phandle);
    if (phandle->restore_input_timer)
    {
        lv_timer_del(phandle->restore_input_timer);
        phandle->restore_input_timer = NULL;
    }
    if (phandle->alloc_fail_return_timer)
    {
        lv_timer_del(phandle->alloc_fail_return_timer);
        phandle->alloc_fail_return_timer = NULL;
    }
    phandle->esc_return_pending = 0;
    phandle->restore_focus_obj = NULL;
    phandle->pane = DIVOOM_LVGL_PHOTO_MANAGE_PANE_GRID;
    focused = lv_group_get_focused(lv_group_get_default());
    if (focused && lv_obj_is_valid(focused)
        && divoom_lvgl_photo_manage_btn_is_visible(phandle, focused))
    {
        phandle->last_active_obj = focused;
    }
    /* 退出前先从 group 移除 grid 按钮，避免 del_queue 同步销毁时焦点悬空 */
    divoom_lvgl_photo_manage_remove_grid_from_group(phandle);
}

void divoom_lvgl_photo_manage_disp(void *handle, void *active_obj)
{
    Pdivoom_lvgl_photo_manage_ctrl phandle = (Pdivoom_lvgl_photo_manage_ctrl)handle;

    if (!divoom_lvgl_photo_manage_alloc_grid_bufs(phandle))
    {
        divoom_print("[pm] disp alloc grid bufs fail, return pre\n");
        divoom_lvgl_photo_manage_schedule_alloc_fail_return(phandle);
        return;
    }
    lv_scr_load(phandle->main_obj);
    if (phandle->action_pick_timer)
    {
        lv_timer_del(phandle->action_pick_timer);
        phandle->action_pick_timer = NULL;
    }
    divoom_lvgl_photo_manage_action_sheet_close(phandle, 0);
    divoom_lvgl_photo_manage_confirm_close(phandle);
    phandle->pane = DIVOOM_LVGL_PHOTO_MANAGE_PANE_GRID;
    phandle->restore_focus_obj = (lv_obj_t *)active_obj;
    if (phandle->restore_input_timer)
    {
        lv_timer_del(phandle->restore_input_timer);
        phandle->restore_input_timer = NULL;
    }
    /* pdisp 后 menu_control 会 top_control_update_active 抢焦点，必须延后恢复 grid */
    phandle->restore_input_timer = lv_timer_create(divoom_lvgl_photo_manage_deferred_restore_cb,
        DIVOOM_LVGL_PHOTO_MANAGE_DEFER_MS, phandle);
    if (phandle->restore_input_timer)
    {
        lv_timer_set_repeat_count(phandle->restore_input_timer, 1);
    }
    if(phandle->first_load_flag == 0)
    {
        if(phandle->para_info.album_id >= DIVOOM_CUSTOM_START_CLOCK_ID)
        {
            phandle->pic_num = divoom_pic_album_manage_get_disp_order_pic_num(phandle->pget_pic_handle);
        }
        else
        {
            phandle->pic_num = divoom_custom_photo_get_total_items(phandle->pget_pic_handle);
        }
        divoom_lvgl_photo_manage_update_all_pic_info(phandle);
    }
    phandle->first_load_flag = 0;
}

void divoom_lvgl_photo_manage_exit(void *handle)
{
    Pdivoom_lvgl_photo_manage_ctrl phandle = (Pdivoom_lvgl_photo_manage_ctrl)handle;

    divoom_thumb_cache_clear_all();
    divoom_thumb_cache_set_clock_info(NULL);
    divoom_lvgl_photo_manage_release_grid_bufs(phandle);
    lv_anim_del(phandle, NULL);
    if (phandle->action_pick_timer)
    {
        lv_timer_del(phandle->action_pick_timer);
        phandle->action_pick_timer = NULL;
    }
    divoom_lvgl_photo_manage_action_sheet_close(phandle, 0);
    divoom_lvgl_photo_manage_confirm_close(phandle);
    phandle->esc_return_pending = 0;
    if (phandle->restore_input_timer)
    {
        lv_timer_del(phandle->restore_input_timer);
        phandle->restore_input_timer = NULL;
    }
    if (phandle->alloc_fail_return_timer)
    {
        lv_timer_del(phandle->alloc_fail_return_timer);
        phandle->alloc_fail_return_timer = NULL;
    }
    phandle->restore_focus_obj = NULL;
    if (phandle->para_info.album_id >= DIVOOM_CUSTOM_START_CLOCK_ID)
    {
        divoom_pic_album_manage_destory_disp_order_handle(phandle->pget_pic_handle);
    }
    else
    {
        divoom_custom_photo_close(phandle->pget_pic_handle);
    }
    if (phandle->pdriect_menu_ctrl)
    {
        divoom_menu_direct_ctrl_destory(phandle->pdriect_menu_ctrl);
        phandle->pdriect_menu_ctrl = NULL;
    }
    divoom_lvgl_photo_manage_remove_grid_from_group(phandle);
    if (phandle->main_obj)
    {
        lv_obj_del(phandle->main_obj);
        phandle->main_obj = NULL;
    }
    divoom_astrootoo_ui_base_reset_styles(&phandle->ui);
    lv_style_reset(&phandle->btn_normal_type);
    lv_style_reset(&phandle->btn_press_type);
    lv_style_reset(&phandle->btn_select_type);
    if (phandle->select_table)
    {
        divoom_system_free(phandle->select_table);
    }
    divoom_system_free(phandle);
}
