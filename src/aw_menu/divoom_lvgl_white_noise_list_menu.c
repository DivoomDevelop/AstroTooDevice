/**
 * 白噪音列表页面 - 模仿表盘预览菜单的九宫格布局
 * 支持旋钮/按键选择、选中高亮、确认播放，可选睡眠定时提示
 */
#include "divoom.h"
#include "divoom_menu_control.h"
#include "divoom_lvgl_dial_menu.h"
#include "divoom_lvgl_white_noise_list_menu.h"
#include "divoom_pixel_format.h"
#include "divoom_white_noise_player.h"
#include "divoom_user_key.h"

/* 与 MasterGo 设计稿一致: 画布 800×1280, 焦点框 238×366, 内卡 214×342, 圆角 26px */
#define DIVOOM_WHITE_NOISE_LIST_BG_COLOR  0xE4E4E4
#define DIVOOM_WHITE_NOISE_LIST_TOTAL	9
#define DIVOOM_WHITE_NOISE_ITEM_NUM      9
#define DIVOOM_WHITE_NOISE_PAGE_NUM      3   /* 分页容器数量，参考表盘预览 */
#define DIVOOM_WHITE_NOISE_MAX_ITEMS     32  /* 提示音最大数量，缓存数组上限（支持 20+ 时后台缓剩余） */
#define DIVOOM_WHITE_NOISE_FIRST_PAGE_CACHE  9   /* 首屏同步缓存张数，先显示再后台缓剩余 */
#define DIVOOM_WHITE_NOISE_COLS           3
#define DIVOOM_WHITE_NOISE_ROWS           3
#define DIVOOM_WHITE_NOISE_BTN_W          238
#define DIVOOM_WHITE_NOISE_BTN_H          366
#define DIVOOM_WHITE_NOISE_CARD_W         214
#define DIVOOM_WHITE_NOISE_CARD_H         342
#define DIVOOM_WHITE_NOISE_GRID_PAD       24
#define DIVOOM_WHITE_NOISE_GRID_GAP_H     19
#define DIVOOM_WHITE_NOISE_GRID_GAP_V     51
#define DIVOOM_WHITE_NOISE_FOCUS_RADIUS   26
#define DIVOOM_WHITE_NOISE_TIP_AUTO_CLOSE_MS  3000

/* 睡眠定时提示条（MasterGo：720×216，画布顶区居中） */
#define DIVOOM_SLEEP_TIP_POPUP_W           720
#define DIVOOM_SLEEP_TIP_POPUP_H           216
#define DIVOOM_SLEEP_TIP_POPUP_RADIUS      28
#define DIVOOM_SLEEP_TIP_TOP_OFFSET        40   /* 距屏顶 */
#define DIVOOM_SLEEP_TIP_ICON_SZ           54
#define DIVOOM_SLEEP_TIP_ICON_PAD          24
#define DIVOOM_SLEEP_TIP_ICON_TEXT_GAP     20
#define DIVOOM_SLEEP_TIP_ICON_PATH         "93/93_sleep.png"

/* 睡眠定时弹窗：关闭定时 + 7 个时长选项 */
#define DIVOOM_SLEEP_TIMER_OPTION_NUM     8
#define DIVOOM_SLEEP_TIMER_POPUP_W       560
#define DIVOOM_SLEEP_TIMER_POPUP_H       680
#define DIVOOM_SLEEP_TIMER_ITEM_H        64
#define DIVOOM_SLEEP_TIMER_OFF_MIN       0   /* 关闭定时 */

/* 预设时长（分钟），0 表示关闭，其余为 10/15/30/45/60/90/120 */
static const int sleep_timer_minutes[] = { 0, 10, 15, 30, 45, 60, 90, 120 };

typedef struct _divoom_lvgl_white_noise_list_menu_ctrl
{
	int menu_id;
	lv_coord_t col_dsc[6];  /* 238, 19, 238, 19, 238, LAST */
	lv_coord_t row_dsc[8];  /* 366,51,366,51,366,底部标题行,LAST */
	lv_obj_t *main_menu;
	lv_obj_t *cont;
	void *cout_obj;
	lv_style_t backgroud_type;
	lv_style_t grid_type;
	lv_style_t btn_type;
	lv_style_t btn_press_type;

	int list_type;  //列表类型 0所有列表 1收藏列表
	int current_item_idx;   /* 全局音源索引 0..total_items-1 */
	int current_effect_id;//当前正在播放的id
	int items_per_page;
	int total_items;
	int total_groups;
	int new_group;         /* 当前页码，用于分页与滚动 */
	lv_obj_t *grids[DIVOOM_WHITE_NOISE_PAGE_NUM];  /* 分页容器，参考表盘预览 */
	int wait_time;
	lv_obj_t *tip_popup;       /* 睡眠定时提示弹窗，3秒后自动消失 */
	lv_timer_t *tip_timer;

	lv_obj_t *sleep_timer_popup;   /* 睡眠定时选择弹窗（遮罩+内容） */
	lv_obj_t *sleep_timer_btns[DIVOOM_SLEEP_TIMER_OPTION_NUM];  /* 选项按钮，用于高亮 */
	int sleep_timer_focus_idx;    /* 当前选中项 0~7 */

	/* 提示音预览图缓存（参考表盘预览），避免翻页/刷新时解码卡顿 */
	lv_img_dsc_t *grid_img_cache[DIVOOM_WHITE_NOISE_MAX_ITEMS];
	int cache_run_flag;       /* 后台缓存任务运行标志，0 停止 */
	void *cache_lock;        /* 保护 grid_img_cache 的锁 */
	void *cache_sem;         /* 任务退出时 post，供 pause/exit 等待 */
} divoom_lvgl_white_noise_list_menu_ctrl, *Pdivoom_lvgl_white_noise_list_menu_ctrl;

/* 宫格 UI 图路径 */
const char * const divoom_white_noise_grid_img_paths[] = {
	"93/93_fire.webp",    /* 0 篝火 fire */
	"93/93_sea.webp",     /* 1 海浪 sea */
	"93/93_mist.webp",    /* 2 涟漪 mist */
	"93/93_ripple.webp",  /* 3 水波纹 ripple */
	"93/93_light.webp",   /* 4 光影 light */
	"93/93_quiet.webp",   /* 5 冥想 quiet */
	"93/93_sunrise.webp", /* 6 光影流动 sunrise */
	"93/93_rain.webp",    /* 7 雨滴 rain */
	"93/93_stream.webp",  /* 8 溪流 stream */
};
#define DIVOOM_WHITE_NOISE_GRID_IMG_PATHS_CNT   9

/* 白噪音项名称（与设计图一致），按系统语言选择中/英文 */
static const char *white_noise_name_list_en[] = {
	"fire",
	"sea",
	"mist",
	"ripple",
	"light",
	"quiet",
	"sunrise",
	"rain",
	"stream"
};
static const char *white_noise_name_list_cn[] = {
	"篝火",
	"海浪",
	"涟漪",
	"水波纹",
	"光影",
	"冥想",
	"光影流动",
	"雨滴",
	"溪流",
};
#define DIVOOM_WHITE_NOISE_NAME_LIST_CNT    9

static const char *divoom_lvgl_white_noise_get_name(int idx, int is_cn)
{
	if (idx < 0 || idx >= DIVOOM_WHITE_NOISE_NAME_LIST_CNT)
		return is_cn ? "白噪音" : "white";
	return is_cn ? white_noise_name_list_cn[idx] : white_noise_name_list_en[idx];
}

static void divoom_lvgl_white_noise_list_menu_event_cb(lv_event_t *e);
static void divoom_lvgl_white_noise_list_menu_next_item(void *handle);
static void divoom_lvgl_white_noise_list_menu_previous_item(void *handle);
static void divoom_lvgl_white_noise_list_menu_confirm_selection(void *handle);
static void divoom_lvgl_white_noise_list_menu_update_focus(void *handle);
static void divoom_lvgl_white_noise_list_create_dynamic_page(void *handle);
static void divoom_lvgl_white_noise_list_update_page_content(int page_idx, int start_item, void *handle);

static void divoom_lvgl_white_noise_list_close_sleep_timer_popup(void *handle);
static void divoom_lvgl_white_noise_list_sleep_timer_update_focus(void *handle);
static void divoom_lvgl_white_noise_list_show_sleep_timer_popup(void *handle);

/* 提示音预览图缓存（参考表盘预览 divoom_lvgl_dial_preview_menu） */
static int divoom_lvgl_white_noise_list_cache_png_by_index(int idx, lv_img_dsc_t *img_cache);
static void divoom_lvgl_white_noise_list_load_first_page_cache(void *handle);
static void divoom_lvgl_white_noise_list_load_rest_cache(void *handle);
static void divoom_lvgl_white_noise_list_free_img_cache(void *handle);
static void divoom_lvgl_white_noise_list_set_grid_img_src(lv_obj_t *img, int item_idx, void *handle);

static void divoom_lvgl_white_noise_list_tip_timer_cb(lv_timer_t *timer)
{
	Pdivoom_lvgl_white_noise_list_menu_ctrl phandle = (Pdivoom_lvgl_white_noise_list_menu_ctrl)lv_timer_get_user_data(timer);
	if (phandle == NULL || phandle->tip_popup == NULL)
		return;
	lv_obj_del(phandle->tip_popup);
	phandle->tip_popup = NULL;
	phandle->tip_timer = NULL;
}

int divoom_lvgl_white_noise_list_cache_image(char *file_name, lv_img_dsc_t *img_cache)
{
	int width, height, frame_time, resource_type, frame_num, flag = 0;
	void *pwebp_handle = NULL;
	uint8 *rgb_buf = NULL;

	pwebp_handle = divoom_lvgl_dial_menu_open_resource_handle(file_name, &resource_type, &width, &height, &frame_num);
	if (pwebp_handle == NULL)
	{
		return 0;
	}

	if(resource_type == DIVOOM_RESOURCE_TYPE_WEBP)
	{
		divoom_print("wft test DIVOOM_RESOURCE_TYPE_WEBP\n");
		img_cache->data = divoom_system_malloc(
			divoom_pixel_format_size(width, height, LV_COLOR_FORMAT_RGB565A8));
		if(img_cache->data)
			divoom_libwebp_decode_get_frame_rgb565a8_buffer(pwebp_handle,
				&frame_time, NULL, (uint8 *)img_cache->data);
        divoom_lv_img_dsc_init_fmt(img_cache, width, height,
            LV_COLOR_FORMAT_RGB565A8, img_cache->data);
	}
	divoom_lvgl_dial_menu_release_resource_handle(pwebp_handle, resource_type);
	return 0;
}

/* 将单个提示音预览图解码到内存（参考表盘预览 cache_png_image） */
static int divoom_lvgl_white_noise_list_cache_png_by_index(int idx, lv_img_dsc_t *img_cache)
{
	lv_img_decoder_dsc_t decoder_dsc;
	lv_img_header_t header;
	char path_buf[128];
	const char *rel_path;
	uint32_t size;

	if (idx < 0 || idx >= DIVOOM_WHITE_NOISE_GRID_IMG_PATHS_CNT || img_cache == NULL)
		return 0;
	rel_path = divoom_white_noise_grid_img_paths[idx];
	divoom_menu_control_get_img_path2(rel_path, path_buf);
	divoom_print("wft test path: %s\n", path_buf);

#if 1
	divoom_lvgl_white_noise_list_cache_image(path_buf, img_cache);
#else
    lv_img_dsc_t src_dsc;
	if (lv_img_decoder_open(&decoder_dsc, path_buf, lv_color_hex(0), 0) != LV_RES_OK)
		return 0;
	lv_img_decoder_get_info(path_buf, &header);
	size = divoom_pixel_format_size((uint32_t)header.w, (uint32_t)header.h, LV_COLOR_FORMAT_RGB565A8);
	img_cache->data = divoom_system_malloc(size);
	if (img_cache->data == NULL) 
	{
		lv_img_decoder_close(&decoder_dsc);
		return 0;
	}

	divoom_lv_img_dsc_init_fmt(&src_dsc, header.w, header.h,
        (lv_color_format_t)decoder_dsc.header.cf, decoder_dsc.img_data);
	if (!divoom_lv_img_dsc_convert_buf(&src_dsc, (uint8 *)img_cache->data, LV_COLOR_FORMAT_RGB565A8))
	{
		divoom_system_free((void *)img_cache->data);
		img_cache->data = NULL;
		lv_img_decoder_close(&decoder_dsc);
		return 0;
	}
	lv_img_decoder_close(&decoder_dsc);
	divoom_lv_img_dsc_init_fmt(img_cache, header.w, header.h, LV_COLOR_FORMAT_RGB565A8, img_cache->data);
#endif
	return 1;
}

/* 仅同步缓存首屏 9 张，用于立即显示，不阻塞界面 */
static void divoom_lvgl_white_noise_list_load_first_page_cache(void *handle)
{
    Pdivoom_lvgl_white_noise_list_menu_ctrl phandle = (Pdivoom_lvgl_white_noise_list_menu_ctrl)handle;
    int n = DIVOOM_WHITE_NOISE_FIRST_PAGE_CACHE;
    int i, idx;
    int start_idx = 0;

    // 获取当前播放的音效索引
    int cur_effect = divoom_white_noise_player_get_current_effect();

    // 计算当前页（分全量和收藏列表两种情况）
    int cur_item_idx = 0;
    if (phandle->list_type == 1) 
	{
        // 收藏列表，找到 cur_effect 在收藏列表中的位置
        int j;
        for (j = 0; j < phandle->total_items; j++) 
		{
            if (divoom_white_noise_player_get_favorite_at(j) == cur_effect) 
			{
                cur_item_idx = j;
                break;
            }
        }
    } 
	else 
	{
        // 全部列表
        if (cur_effect >= 0 && cur_effect < phandle->total_items) 
		{
            cur_item_idx = cur_effect;
        }
    }
    int items_per_page = DIVOOM_WHITE_NOISE_ITEM_NUM;
    int total_items = phandle->total_items;
    int total_paths = DIVOOM_WHITE_NOISE_GRID_IMG_PATHS_CNT;

    // 确定当前页的起始索引
    start_idx = (cur_item_idx / items_per_page) * items_per_page;

    // 缓存上限修正
    if (total_items < n)
        n = total_items;
    if (n > total_paths)
        n = total_paths;

    // 若从start_idx起还能取满一页，否则只缓存最后一页剩余部分
    if (start_idx + n > total_items)
        start_idx = total_items > n ? (total_items - n) : 0;

    for (i = 0; i < n; i++)
    {
        // 针对收藏列表要找宫格序号
        if (phandle->list_type == 1) 
		{
            idx = divoom_white_noise_player_get_favorite_at(start_idx + i);
            if (idx < 0 || idx >= total_paths) 
			{
                // 不合法，跳过
                continue;
            }
        } 
		else 
		{
            idx = start_idx + i;
            if (idx >= total_paths)
                break;
        }
        lv_img_dsc_t *dsc = divoom_system_malloc(sizeof(lv_img_dsc_t));
        if (dsc == NULL)
            continue;
        memset(dsc, 0, sizeof(*dsc));
        if (divoom_lvgl_white_noise_list_cache_png_by_index(idx, dsc))
            phandle->grid_img_cache[start_idx + i] = dsc;
        else
        {
            if (dsc->data)
                divoom_system_free((void *)dsc->data);
            divoom_system_free(dsc);
        }
    }
}

/* 后台任务：缓存剩余提示音预览图（从第 10 张起）
 * 需考虑缓存类型：list_type==0 全部列表，list_type==1 收藏列表 */
static void divoom_lvgl_white_noise_list_load_rest_cache(void *handle)
{
	Pdivoom_lvgl_white_noise_list_menu_ctrl phandle = (Pdivoom_lvgl_white_noise_list_menu_ctrl)handle;
	int total = phandle->total_items;
	int paths_cnt = DIVOOM_WHITE_NOISE_GRID_IMG_PATHS_CNT;
	int i;

	if (total <= DIVOOM_WHITE_NOISE_FIRST_PAGE_CACHE)
		return;

	for (i = DIVOOM_WHITE_NOISE_FIRST_PAGE_CACHE; i < total && i < DIVOOM_WHITE_NOISE_MAX_ITEMS && i < paths_cnt; i++) 
	{
		if (!phandle->cache_run_flag)
			break;

		int idx;

		if (phandle->list_type == 1) 
		{
			// 收藏列表, 找到第i项收藏对应的音效编号
			idx = divoom_white_noise_player_get_favorite_at(i);
			if (idx < 0 || idx >= paths_cnt) 
			{
				// 无效，跳过
				continue;
			}
		} 
		else 
		{
			// 全部列表
			idx = i;
			if (idx >= paths_cnt)
				break;
		}

		lv_img_dsc_t *dsc = divoom_system_malloc(sizeof(lv_img_dsc_t));
		if (dsc == NULL)
			continue;
		memset(dsc, 0, sizeof(*dsc));
		if (divoom_lvgl_white_noise_list_cache_png_by_index(idx, dsc)) 
		{
			divoom_system_get_lock(phandle->cache_lock);
			if (phandle->grid_img_cache[i]) 
			{
				if (phandle->grid_img_cache[i]->data)
					divoom_system_free((void *)phandle->grid_img_cache[i]->data);
				divoom_system_free(phandle->grid_img_cache[i]);
			}
			phandle->grid_img_cache[i] = dsc;
			divoom_system_free_lock(phandle->cache_lock);
		} 
		else 
		{
			if (dsc->data)
				divoom_system_free((void *)dsc->data);
			divoom_system_free(dsc);
		}
		divoom_system_delay_ms(5);
	}
}

static void *divoom_lvgl_white_noise_list_cache_task_handle(void *para)
{
	Pdivoom_lvgl_white_noise_list_menu_ctrl phandle = (Pdivoom_lvgl_white_noise_list_menu_ctrl)para;

	/* 只执行一次：缓存第 10 张及之后的预览图 */
	while (phandle->cache_run_flag) 
	{
		divoom_lvgl_white_noise_list_load_rest_cache(phandle);
		break;
	}
	divoom_system_put_sem(phandle->cache_sem);
	return NULL;
}

/* 释放预览图缓存 */
static void divoom_lvgl_white_noise_list_free_img_cache(void *handle)
{
	Pdivoom_lvgl_white_noise_list_menu_ctrl phandle = (Pdivoom_lvgl_white_noise_list_menu_ctrl)handle;
	int i;

	for (i = 0; i < DIVOOM_WHITE_NOISE_MAX_ITEMS; i++) {
		if (phandle->grid_img_cache[i]) {
			if (phandle->grid_img_cache[i]->data)
				divoom_system_free((void *)phandle->grid_img_cache[i]->data);
			divoom_system_free(phandle->grid_img_cache[i]);
			phandle->grid_img_cache[i] = NULL;
		}
	}
}

/* 设置宫格图片：优先用缓存，无缓存则走路径解码 */
static void divoom_lvgl_white_noise_list_set_grid_img_src(lv_obj_t *img, int item_idx, void *handle)
{
	Pdivoom_lvgl_white_noise_list_menu_ctrl phandle = (Pdivoom_lvgl_white_noise_list_menu_ctrl)handle;
	lv_img_dsc_t *cached = NULL;

	if (item_idx < 0 || item_idx >= DIVOOM_WHITE_NOISE_MAX_ITEMS || item_idx >= phandle->total_items)
		return;
	if (item_idx < DIVOOM_WHITE_NOISE_GRID_IMG_PATHS_CNT) 
	{
		if (phandle->cache_lock)
			divoom_system_get_lock(phandle->cache_lock);
		cached = phandle->grid_img_cache[item_idx];
		if (phandle->cache_lock)
			divoom_system_free_lock(phandle->cache_lock);
	}
	if (cached) 
	{
		lv_img_cache_invalidate_src(cached);
		lv_img_set_src(img, cached);
		lv_obj_invalidate(img);
	} 
	else if (item_idx < DIVOOM_WHITE_NOISE_GRID_IMG_PATHS_CNT) 
	{
//		divoom_menu_control_lv_img_set_src(img, divoom_white_noise_grid_img_paths[item_idx]);
	}
}

/* 创建并显示“睡眠定时”提示弹窗，3秒后自动关闭 */
static void divoom_lvgl_white_noise_list_show_sleep_tip(void *handle)
{
	Pdivoom_lvgl_white_noise_list_menu_ctrl phandle = (Pdivoom_lvgl_white_noise_list_menu_ctrl)handle;
	Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();
	int is_cn = (psys_config && psys_config->lang == DIVOOM_LVGL_MENU_LANG_SC);
	lv_obj_t *parent = phandle->main_menu;
	lv_obj_t *mask;
	lv_obj_t *box;
	lv_obj_t *title;
	lv_obj_t *body;

	if (phandle->tip_popup != NULL)
		return;

	/* 轻量全屏遮罩：仅略压暗背景，避免整屏浓黑 */
	mask = lv_obj_create(parent);
	lv_obj_set_size(mask, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_pos(mask, 0, 0);
	lv_obj_set_style_bg_color(mask, lv_color_hex(0x000000), 0);
	lv_obj_set_style_bg_opa(mask, LV_OPA_40, 0);
	lv_obj_set_style_border_width(mask, 0, 0);
	lv_obj_set_style_radius(mask, 0, 0);
	lv_obj_clear_flag(mask, LV_OBJ_FLAG_SCROLLABLE);
	phandle->tip_popup = mask;

	/* 顶部居中提示条 720×216 */
	box = lv_obj_create(mask);
	lv_obj_set_size(box, DIVOOM_SLEEP_TIP_POPUP_W, DIVOOM_SLEEP_TIP_POPUP_H);
	lv_obj_align(box, LV_ALIGN_TOP_MID, 0, DIVOOM_SLEEP_TIP_TOP_OFFSET);
	lv_obj_set_style_bg_color(box, lv_color_hex(0xF2F2F2), 0);
	lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
	lv_obj_set_style_radius(box, DIVOOM_SLEEP_TIP_POPUP_RADIUS, 0);
	lv_obj_set_style_border_width(box, 2, 0);
	lv_obj_set_style_border_color(box, lv_color_hex(0x333333), 0);
	lv_obj_set_style_pad_all(box, 0, 0);
	lv_obj_set_style_shadow_width(box, 8, 0);
	lv_obj_set_style_shadow_color(box, lv_color_hex(0x000000), 0);
	lv_obj_set_style_shadow_opa(box, LV_OPA_20, 0);
	lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t *icon = lv_img_create(box);
	lv_obj_set_size(icon, DIVOOM_SLEEP_TIP_ICON_SZ, DIVOOM_SLEEP_TIP_ICON_SZ);
	divoom_menu_control_lv_img_set_src(icon, DIVOOM_SLEEP_TIP_ICON_PATH);
	lv_obj_align(icon, LV_ALIGN_TOP_LEFT, DIVOOM_SLEEP_TIP_ICON_PAD, 20);

	lv_obj_t *text_col = lv_obj_create(box);
	lv_obj_set_size(text_col, DIVOOM_SLEEP_TIP_POPUP_W - DIVOOM_SLEEP_TIP_ICON_PAD - DIVOOM_SLEEP_TIP_ICON_SZ - DIVOOM_SLEEP_TIP_ICON_TEXT_GAP - 24, LV_SIZE_CONTENT);
	lv_obj_set_style_bg_opa(text_col, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(text_col, 0, 0);
	lv_obj_set_style_pad_all(text_col, 0, 0);
	lv_obj_clear_flag(text_col, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_align(text_col, LV_ALIGN_TOP_LEFT, DIVOOM_SLEEP_TIP_ICON_PAD + DIVOOM_SLEEP_TIP_ICON_SZ + DIVOOM_SLEEP_TIP_ICON_TEXT_GAP, 20);
	lv_obj_set_layout(text_col, LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(text_col, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(text_col, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
	lv_obj_set_style_pad_row(text_col, 10, 0);

	title = lv_label_create(text_col);
	lv_label_set_text(title, is_cn ? "睡眠定时" : "Sleep Timer");
	lv_obj_set_style_text_color(title, lv_color_hex(0x000000), 0);
	lv_obj_set_style_text_font(title, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);
	lv_obj_set_width(title, lv_pct(100));

	body = lv_label_create(text_col);
	lv_label_set_text(body, is_cn ? "按下大旋钮进入定时设置，根据预设时间停止播放当前的音效":"Press the large knob to set a sleep timer. Playback will stop when the preset time ends.");
	lv_obj_set_style_text_color(body, lv_color_hex(0x1A1A1A), 0);
	lv_obj_set_style_text_font(body, divoom_menu_control_get_font_info(18, phandle->menu_id), 0);
	lv_obj_set_width(body, lv_pct(100));
	lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(body, LV_TEXT_ALIGN_LEFT, 0);

	phandle->tip_timer = lv_timer_create(divoom_lvgl_white_noise_list_tip_timer_cb, DIVOOM_WHITE_NOISE_TIP_AUTO_CLOSE_MS, phandle);
	lv_timer_set_repeat_count(phandle->tip_timer, 1);
}

/* 关闭睡眠定时选择弹窗 */
static void divoom_lvgl_white_noise_list_close_sleep_timer_popup(void *handle)
{
	Pdivoom_lvgl_white_noise_list_menu_ctrl phandle = (Pdivoom_lvgl_white_noise_list_menu_ctrl)handle;
	if (phandle == NULL || phandle->sleep_timer_popup == NULL)
		return;
	lv_obj_del(phandle->sleep_timer_popup);
	phandle->sleep_timer_popup = NULL;
	memset(phandle->sleep_timer_btns, 0, sizeof(phandle->sleep_timer_btns));
}

/* 更新睡眠定时弹窗内选项的高亮 */
static void divoom_lvgl_white_noise_list_sleep_timer_update_focus(void *handle)
{
	Pdivoom_lvgl_white_noise_list_menu_ctrl phandle = (Pdivoom_lvgl_white_noise_list_menu_ctrl)handle;
	int i;
	for (i = 0; i < DIVOOM_SLEEP_TIMER_OPTION_NUM; i++) 
	{
		if (phandle->sleep_timer_btns[i]) 
		{
			if (i == phandle->sleep_timer_focus_idx) 
			{
				lv_obj_set_style_bg_color(phandle->sleep_timer_btns[i], lv_color_hex(0xE8E8E8), 0);
				lv_obj_set_style_border_width(phandle->sleep_timer_btns[i], 2, 0);
				lv_obj_set_style_border_color(phandle->sleep_timer_btns[i], lv_color_hex(0xFF6200), 0);
			} 
			else 
			{
				lv_obj_set_style_bg_color(phandle->sleep_timer_btns[i], lv_color_hex(0xF5F5F5), 0);
				lv_obj_set_style_border_width(phandle->sleep_timer_btns[i], 0, 0);
			}
		}
	}
}

/* 创建并显示“睡眠定时”选择弹窗（按大确认键进入）：关闭定时 + 10/15/30/45/60/90/120min */
static void divoom_lvgl_white_noise_list_show_sleep_timer_popup(void *handle)
{
	Pdivoom_lvgl_white_noise_list_menu_ctrl phandle = (Pdivoom_lvgl_white_noise_list_menu_ctrl)handle;
	Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();
	int is_cn = (psys_config && psys_config->lang == DIVOOM_LVGL_MENU_LANG_SC);
	lv_obj_t *parent = phandle->main_menu;
	lv_obj_t *mask, *box, *btn;
	lv_obj_t *label;
	static const char *time_texts_cn[] = { "关闭定时", "10分钟", "15分钟", "30分钟", "45分钟", "60分钟", "90分钟", "120分钟" };
	static const char *time_texts_en[] = { "Turn Off Timer", "10min", "15min", "30min", "45min", "60min", "90min", "120min" };
	const char **time_texts = is_cn ? time_texts_cn : time_texts_en;
	const int popup_pad = 16;
	const int first_item_h = 52;
	const int normal_item_h = DIVOOM_SLEEP_TIMER_ITEM_H;
	const int popup_h = popup_pad * 2 + first_item_h + (DIVOOM_SLEEP_TIMER_OPTION_NUM - 1) * normal_item_h;
	int timer_min = divoom_white_noise_player_get_sleep_timer_minutes();

	int i;

	if (phandle->sleep_timer_popup != NULL)
		return;

	/* 半透明遮罩 */
	mask = lv_obj_create(parent);
	lv_obj_set_size(mask, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_pos(mask, 0, 0);
	lv_obj_set_style_bg_color(mask, lv_color_hex(0x000000), 0);
	lv_obj_set_style_bg_opa(mask, LV_OPA_60, 0);
	lv_obj_set_style_border_width(mask, 0, 0);
	lv_obj_set_style_radius(mask, 0, 0);
	lv_obj_clear_flag(mask, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_scrollbar_mode(mask, LV_SCROLLBAR_MODE_OFF);
	phandle->sleep_timer_popup = mask;

	/* 白色圆角弹窗框 */
	box = lv_obj_create(mask);
	lv_obj_set_size(box, DIVOOM_SLEEP_TIMER_POPUP_W, popup_h);
	lv_obj_center(box);
	lv_obj_set_style_bg_color(box, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_style_radius(box, 20, 0);
	lv_obj_set_style_shadow_width(box, 12, 0);
	lv_obj_set_style_shadow_color(box, lv_color_hex(0x000000), 0);
	lv_obj_set_style_shadow_opa(box, LV_OPA_30, 0);
	lv_obj_set_style_pad_all(box, popup_pad, 0);
	lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_scrollbar_mode(box, LV_SCROLLBAR_MODE_OFF);

	for (i = 0; i < DIVOOM_SLEEP_TIMER_OPTION_NUM; i++) 
	{
		int item_h = (i == 0) ? first_item_h : normal_item_h;
		int y_off = popup_pad + (i == 0 ? 0 : first_item_h + (i - 1) * normal_item_h);
		btn = lv_btn_create(box);
		lv_obj_set_size(btn, DIVOOM_SLEEP_TIMER_POPUP_W - 32, item_h);
		lv_obj_set_pos(btn, 0, y_off);
		lv_obj_set_style_radius(btn, (i == 0) ? 12 : 8, 0);
		lv_obj_set_style_bg_color(btn, (i == 0) ? lv_color_hex(0xE8E8E8) : lv_color_hex(0xE8E8E8), 0);
		lv_obj_set_style_border_width(btn, (i == 0) ? 2 : 0, 0);
		lv_obj_set_style_border_color(btn, lv_color_hex(0xFF6200), 0);
		lv_obj_set_style_shadow_width(btn, 0, 0);
		lv_obj_set_style_pad_all(btn, 0, 0);
		label = lv_label_create(btn);
		lv_label_set_text(label, time_texts[i]);
		lv_obj_set_style_text_color(label, lv_color_hex(0x333333), 0);
		lv_obj_set_style_text_font(label, divoom_menu_control_get_font_info((i == 0) ? 20 : 18, phandle->menu_id), 0);
		lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
		lv_obj_center(label);
		phandle->sleep_timer_btns[i] = btn;
	}

	phandle->sleep_timer_focus_idx = 0;
	for (i = 0; i < DIVOOM_SLEEP_TIMER_OPTION_NUM; i++)
	{
		if (sleep_timer_minutes[i] == timer_min)
		{
			phandle->sleep_timer_focus_idx = i;
			break;
		}
	}
	divoom_lvgl_white_noise_list_sleep_timer_update_focus(handle);
}

void divoom_lvgl_white_noise_list_menu_update_focus(void *handle)
{
	Pdivoom_lvgl_white_noise_list_menu_ctrl phandle = (Pdivoom_lvgl_white_noise_list_menu_ctrl)handle;
	int current_page = phandle->new_group;
	int current_item_idx = phandle->current_item_idx;
    lv_obj_t * grid = phandle->grids[current_page % DIVOOM_WHITE_NOISE_PAGE_NUM];  // 获取当前页面
	lv_obj_t * cont = lv_obj_get_child(grid, 0);

    for (int i = 0; i < DIVOOM_WHITE_NOISE_ITEM_NUM; i++) 
	{
        lv_obj_t * btn = lv_obj_get_child(cont, i);
		if (btn)
		{
	        if (i == (current_item_idx % DIVOOM_WHITE_NOISE_ITEM_NUM)) 
			{
	            lv_obj_add_state(btn, LV_STATE_FOCUSED);  // 给当前选中的按钮加上焦点
	        }
			else 
			{
	            lv_obj_clear_state(btn, LV_STATE_FOCUSED);  // 移除其他项的焦点
	        }
		}
    }
}

/* 动态更新某一页的九宫格内容（参考表盘预览 update_page_content） */
static void divoom_lvgl_white_noise_list_update_page_content(int page_idx, int start_item, void *handle)
{
    Pdivoom_lvgl_white_noise_list_menu_ctrl phandle = (Pdivoom_lvgl_white_noise_list_menu_ctrl)handle;
	Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();
	int is_cn = (psys_config && psys_config->lang == DIVOOM_LVGL_MENU_LANG_SC);
    lv_obj_t *grid = phandle->grids[page_idx];  // 获取要更新的页面容器
	int i = 0, j = 0, flag = 0;
	uint8 clock_name[128] = {0};

	for (i = 0; i < DIVOOM_WHITE_NOISE_PAGE_NUM; i++)
	    lv_obj_clean(phandle->grids[i]);  // 清除页面上原有的项目,否则图片资源释放会导致系统崩溃

	lv_obj_t *cont = lv_btn_create(grid); //需要创建一个容器，把网格设置在容器上，不能直接在屏幕上设置网格
	lv_obj_set_size(cont, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	phandle->cout_obj = cont;
	lv_group_focus_obj(cont);
    lv_obj_add_event_cb(cont, divoom_lvgl_white_noise_list_menu_event_cb, LV_EVENT_KEY, phandle);

	lv_style_init(&phandle->btn_type);
	lv_style_set_pad_all(&phandle->btn_type, 0);
	lv_style_set_pad_gap(&phandle->btn_type, 0);
	lv_style_set_border_width(&phandle->btn_type, 4);
	lv_style_set_radius(&phandle->btn_type, 21);
	lv_style_set_bg_color(&phandle->btn_type, lv_color_hex(DIVOOM_WHITE_NOISE_LIST_BG_COLOR)); //0xF2F2F2
	lv_style_set_border_color(&phandle->btn_type, lv_color_hex(0xBEBEBE)); 
	lv_style_set_shadow_width(&phandle->btn_type, 0);

	lv_style_init(&phandle->btn_press_type);
	lv_style_set_pad_all(&phandle->btn_press_type, 0);
	lv_style_set_pad_gap(&phandle->btn_press_type, 0);
	lv_style_set_border_width(&phandle->btn_press_type, 4);
	lv_style_set_outline_width(&phandle->btn_press_type, 0);
	lv_style_set_radius(&phandle->btn_press_type, 21);
	lv_style_set_bg_color(&phandle->btn_press_type, lv_color_hex(DIVOOM_WHITE_NOISE_LIST_BG_COLOR)); //0xF2F2F2
	lv_style_set_border_color(&phandle->btn_press_type, lv_color_hex(0xFF6200)); //选中改变边框颜色

	for(i = 0; i < 3; i ++)
    {
        phandle->col_dsc[i] = 214 + 20;//LV_GRID_FR(1);
    }
    phandle->col_dsc[3] = LV_GRID_TEMPLATE_LAST;
    for(i = 0; i < 3; i ++)
    {
        phandle->row_dsc[i] = 342 + 20;//LV_GRID_FR(1);
    }
    phandle->row_dsc[3] = LV_GRID_TEMPLATE_LAST;

	lv_obj_add_style(cont, &phandle->grid_type, 0);
	lv_obj_set_style_pad_all(cont, 31 - 5, 0);
	lv_obj_set_style_bg_opa(cont, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(cont, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
	lv_obj_set_style_base_dir(cont, LV_BASE_DIR_AUTO, 0);

	lv_obj_set_grid_dsc_array(cont, phandle->col_dsc, phandle->row_dsc);	// 设置网格布局

	// 设置列间距和行间距
	lv_obj_set_style_pad_column(cont, 31 - 10, 0);  // 列间距 
	lv_obj_set_style_pad_row(cont, 63 - 10, 0);	 // 行间距 
	lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF); //隐藏滚动条

    /* 动态创建项目 */
    for (i = 0; i < DIVOOM_WHITE_NOISE_ITEM_NUM; i++) 
	{
        int item_idx = start_item + i;
        if (item_idx >= phandle->total_items) 
			break;  // 如果超出总项目数则停止

		/* list_type==1 收藏列表：item_idx 为列表位置，取对应的音效宫格索引用于预览图与名称 */
		int effect_idx = (phandle->list_type == 1)? divoom_white_noise_player_get_favorite_at(item_idx): item_idx;

        /* 创建按钮 */
        lv_obj_t * btn = lv_btn_create(cont);
		lv_obj_set_size(btn, 214+20, 342+20);
//		lv_obj_center(btn);
		lv_obj_add_style(btn, &phandle->btn_press_type, LV_STATE_FOCUSED);
		lv_obj_add_style(btn, &phandle->btn_type, 0);
		
		if (phandle->current_effect_id == effect_idx)
		{
			lv_obj_set_style_border_color(btn, lv_color_hex(0xDEB100), 0);
		}
		else
		{
			lv_obj_set_style_border_color(btn, lv_color_hex(0xBEBEBE), 0);
		}		

        lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, i % 3, 1, LV_GRID_ALIGN_STRETCH, i / 3, 1);

        /* 在按钮上添加图片 */
        lv_obj_t * img = lv_img_create(btn);
		lv_obj_set_align(img, LV_ALIGN_CENTER);
		divoom_lvgl_white_noise_list_set_grid_img_src(img, effect_idx, handle);
		/* 全部列表下已收藏的显示❤；收藏列表里每项都是收藏也画❤ */
		if (phandle->list_type == 1 || divoom_white_noise_player_is_favorite(effect_idx))
		{
			lv_obj_t *heart_img = lv_img_create(btn);
			divoom_menu_control_lv_img_set_src(heart_img, "93/93_heart.png");
			lv_obj_align(heart_img, LV_ALIGN_BOTTOM_LEFT, 20, -20);
		}

	    lv_obj_t *label = lv_label_create(grid);
		lv_obj_set_size(label, 226, 24);
		lv_obj_set_pos(label, 20 + (i%3)*263, 400 + (i/3)*417);

		lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
		lv_obj_set_style_text_color(label, lv_color_hex(0x333333), 0);
		lv_obj_set_style_text_font(label, divoom_menu_control_get_font_info(18, phandle->menu_id), 0);
		lv_label_set_text(label, divoom_lvgl_white_noise_get_name(effect_idx, is_cn));
    }

    /* 更新焦点 */
	divoom_lvgl_white_noise_list_menu_update_focus(handle);
}

/* 创建分页容器并初始化第一页（参考表盘预览 create_dynamic_page） */
static void divoom_lvgl_white_noise_list_create_dynamic_page(void *handle)
{
	Pdivoom_lvgl_white_noise_list_menu_ctrl phandle = (Pdivoom_lvgl_white_noise_list_menu_ctrl)handle;
	int i;

	lv_style_init(&phandle->grid_type);
	lv_style_set_pad_all(&phandle->grid_type, 0);
	lv_style_set_border_width(&phandle->grid_type, 0);
	lv_style_set_radius(&phandle->grid_type, 0);
	lv_style_set_bg_color(&phandle->grid_type, lv_color_hex(DIVOOM_WHITE_NOISE_LIST_BG_COLOR));
	lv_style_set_bg_opa(&phandle->grid_type, LV_OPA_COVER);

	for (i = 0; i < DIVOOM_WHITE_NOISE_PAGE_NUM; i++) 
	{
		phandle->grids[i] = lv_obj_create(phandle->main_menu);
		lv_obj_set_size(phandle->grids[i], DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
		lv_obj_set_pos(phandle->grids[i], 0, i * DIVOOM_SCREEN_HIGH);
		lv_obj_add_style(phandle->grids[i], &phandle->grid_type, 0);
		lv_obj_set_style_bg_opa(phandle->grids[i], LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT);
		lv_obj_set_style_bg_opa(phandle->grids[i], LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
		lv_obj_set_style_base_dir(phandle->grids[i], LV_BASE_DIR_AUTO, 0);
	}

	divoom_lvgl_white_noise_list_update_page_content(phandle->new_group % DIVOOM_WHITE_NOISE_PAGE_NUM, phandle->new_group * phandle->items_per_page, handle);
	lv_obj_scroll_to_view(phandle->grids[phandle->new_group % DIVOOM_WHITE_NOISE_PAGE_NUM], LV_ANIM_OFF);
}

/* 确认选择：按当前宫格播放对应提示音，按已选睡眠定时控制时长（0=一直播放），然后回到播放主页面 */
void divoom_lvgl_white_noise_list_menu_confirm_selection(void *handle)
{
	Pdivoom_lvgl_white_noise_list_menu_ctrl phandle = (Pdivoom_lvgl_white_noise_list_menu_ctrl)handle;
	int timer_min = divoom_white_noise_player_get_sleep_timer_minutes();
	int effect_idx = (phandle->list_type == 1)? divoom_white_noise_player_get_favorite_at(phandle->current_item_idx): phandle->current_item_idx;

	divoom_white_noise_player_start(effect_idx, timer_min);
	divoom_menu_control_return_pre();
}

void divoom_lvgl_white_noise_list_menu_next_item(void *handle)
{
	Pdivoom_lvgl_white_noise_list_menu_ctrl phandle = (Pdivoom_lvgl_white_noise_list_menu_ctrl)handle;
	int page_update = 0;

	phandle->current_item_idx++;
	if (phandle->current_item_idx >= phandle->total_items) 
	{
		phandle->current_item_idx = 0;
		phandle->new_group = 0;
		page_update = 1;
	} 
	else if (phandle->current_item_idx >= (phandle->new_group + 1) * phandle->items_per_page) 
	{
		if (phandle->new_group < phandle->total_groups - 1)
			phandle->new_group++;
		else
			phandle->new_group = 0;
		page_update = 1;
	}

	if (page_update) 
	{
		divoom_lvgl_white_noise_list_update_page_content(phandle->new_group % DIVOOM_WHITE_NOISE_PAGE_NUM,phandle->new_group * phandle->items_per_page, handle);
		lv_obj_scroll_to_view(phandle->grids[phandle->new_group % DIVOOM_WHITE_NOISE_PAGE_NUM], LV_ANIM_OFF);
	} 
	else 
	{
		divoom_lvgl_white_noise_list_menu_update_focus(handle);
	}
}

void divoom_lvgl_white_noise_list_menu_previous_item(void *handle)
{
	Pdivoom_lvgl_white_noise_list_menu_ctrl phandle = (Pdivoom_lvgl_white_noise_list_menu_ctrl)handle;
	int page_update = 0;

	if (phandle->current_item_idx > 0) 
	{
		phandle->current_item_idx--;
		if (phandle->current_item_idx < phandle->new_group * phandle->items_per_page) 
		{
			if (phandle->new_group > 0)
				phandle->new_group--;
			else
				phandle->new_group = phandle->total_groups - 1;
			page_update = 1;
		}
	} 
	else 
	{
		phandle->current_item_idx = phandle->total_items - 1;
		phandle->new_group = (phandle->total_items - 1) / phandle->items_per_page;
		page_update = 1;
	}

	if (page_update) 
	{
		divoom_lvgl_white_noise_list_update_page_content(phandle->new_group % DIVOOM_WHITE_NOISE_PAGE_NUM,phandle->new_group * phandle->items_per_page, handle);
		lv_obj_scroll_to_view(phandle->grids[phandle->new_group % DIVOOM_WHITE_NOISE_PAGE_NUM], LV_ANIM_OFF);
	} 
	else 
	{
		divoom_lvgl_white_noise_list_menu_update_focus(handle);
	}
}

static void divoom_lvgl_white_noise_list_menu_event_cb(lv_event_t *e)
{
	Pdivoom_lvgl_white_noise_list_menu_ctrl phandle = (Pdivoom_lvgl_white_noise_list_menu_ctrl)lv_event_get_user_data(e);
	lv_event_code_t code = lv_event_get_code(e);
	char *key_info = (char *)lv_event_get_param(e);
	char c = key_info ? (char)key_info[0] : 0;

	if (code != LV_EVENT_KEY)
		return;

	phandle->wait_time = divoom_system_get_ticks() + DIVOOM_EXIT_MENU_TIME_OUT;

	divoom_print("wft test c == 0x%x\n", c);
	/* 若睡眠定时选择弹窗已打开，仅处理弹窗内按键 */
	if (phandle->sleep_timer_popup != NULL) 
	{
		if (c == DIVOOM_LVGL_KEY_ESC) 
		{
			divoom_lvgl_white_noise_list_close_sleep_timer_popup(phandle);
			return;
		}
		if (c == DIVOOM_LVGL_KEY_PRE) 
		{
			phandle->sleep_timer_focus_idx--;
			if (phandle->sleep_timer_focus_idx < 0)
				phandle->sleep_timer_focus_idx = DIVOOM_SLEEP_TIMER_OPTION_NUM - 1;
			divoom_lvgl_white_noise_list_sleep_timer_update_focus(phandle);
			return;
		}
		if (c == DIVOOM_LVGL_KEY_NEXT) 
		{
			phandle->sleep_timer_focus_idx++;
			if (phandle->sleep_timer_focus_idx >= DIVOOM_SLEEP_TIMER_OPTION_NUM)
				phandle->sleep_timer_focus_idx = 0;
			divoom_lvgl_white_noise_list_sleep_timer_update_focus(phandle);
			return;
		}
		if (c == DIVOOM_LVGL_KEY_ENTER) 
		{
			int minutes = sleep_timer_minutes[phandle->sleep_timer_focus_idx];
			divoom_white_noise_player_set_sleep_timer_minutes(minutes);
			divoom_lvgl_white_noise_list_close_sleep_timer_popup(phandle);
			return;
		}
		return;
	}

	/* 大确认键：打开睡眠定时选择弹窗 */
	if (c == DIVOOM_KEY_LIGHT_ENTER) 
	{
		divoom_lvgl_white_noise_list_show_sleep_timer_popup(phandle);
		return;
	}

	if (c == DIVOOM_LVGL_KEY_ESC) 
	{
		divoom_menu_control_return_pre();
		return;
	}
	if (c == DIVOOM_LVGL_KEY_PRE) 
	{
		divoom_lvgl_white_noise_list_menu_previous_item(phandle);
		return;
	}
	if (c == DIVOOM_LVGL_KEY_NEXT) 
	{
		divoom_lvgl_white_noise_list_menu_next_item(phandle);
		return;
	}
	if (c == DIVOOM_LVGL_KEY_ENTER) 
	{
		divoom_lvgl_white_noise_list_menu_confirm_selection(phandle);
		return;
	}
	/* 其他按键交给主逻辑 */
	if (key_info)
		divoom_main_keypad_send_key_info(key_info[0], key_info[1]);
}

void *divoom_lvgl_white_noise_list_menu_init(void *user_data, int menu_id)
{
	Pdivoom_lvgl_white_noise_list_menu_ctrl phandle = divoom_system_malloc(sizeof(divoom_lvgl_white_noise_list_menu_ctrl));
	if (phandle == NULL)
		return NULL;

	memset(phandle, 0, sizeof(*phandle));
	phandle->menu_id = menu_id;
	phandle->items_per_page = DIVOOM_WHITE_NOISE_ITEM_NUM;
	if (user_data) 
	{
		phandle->list_type = *(int *)user_data;
		divoom_system_free(user_data);
	}
	/* list_type==0 全部列表，list_type==1 收藏列表（只显示收藏项的预览图） */

	divoom_print("wft test list_type: %d\n", phandle->list_type);

	if (phandle->list_type == 1) 
	{
		phandle->total_items = divoom_white_noise_player_get_favorite_count();
		if (phandle->total_items <= 0)
			phandle->total_items = 1;
	} 
	else 
	{
		phandle->total_items = DIVOOM_WHITE_NOISE_LIST_TOTAL;
	}
	phandle->total_groups = (phandle->total_items + phandle->items_per_page - 1) / phandle->items_per_page;
	/* 参考表盘预览：根据当前正在播放的提示音决定首屏显示的九宫格与选中项 */
	int cur = divoom_white_noise_player_get_current_effect();
	if (phandle->list_type == 1) 
	{
		int j, pos = 0;
		for (j = 0; j < phandle->total_items; j++) 
		{
			if (divoom_white_noise_player_get_favorite_at(j) == cur) 
			{
				pos = j;
				break;
			}
		}
		phandle->current_effect_id = pos;
	} 
	else 
	{
		if (cur < 0 || cur >= phandle->total_items)
			cur = 0;
		phandle->current_effect_id = cur;
	}

	phandle->current_item_idx = phandle->current_effect_id;
	phandle->new_group = phandle->current_item_idx / phandle->items_per_page;
	phandle->wait_time = divoom_system_get_ticks() + DIVOOM_EXIT_MENU_TIME_OUT;

	phandle->main_menu = lv_obj_create(NULL);
	lv_style_init(&phandle->backgroud_type);
	lv_style_set_bg_color(&phandle->backgroud_type, lv_color_hex(DIVOOM_WHITE_NOISE_LIST_BG_COLOR));
	lv_style_set_bg_opa(&phandle->backgroud_type, LV_OPA_COVER);
	lv_obj_add_style(phandle->main_menu, &phandle->backgroud_type, 0);
	lv_obj_set_size(phandle->main_menu, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_scrollbar_mode(phandle->main_menu, LV_SCROLLBAR_MODE_OFF);
	lv_obj_set_scroll_dir(phandle->main_menu, LV_DIR_VER);

	/* 先同步缓存首屏 9 张，立即显示；若总数 > 9 则后台任务缓存剩余 */
	phandle->cache_lock = divoom_system_create_lock();
	divoom_lvgl_white_noise_list_load_first_page_cache(phandle);
	divoom_lvgl_white_noise_list_create_dynamic_page(phandle);
	if (phandle->total_items > DIVOOM_WHITE_NOISE_FIRST_PAGE_CACHE) 
	{
		phandle->cache_run_flag = 1;
		phandle->cache_sem = divoom_system_create_sem(1, 0);
		divoom_system_create_task(divoom_lvgl_white_noise_list_cache_task_handle, "lv_wnoise", phandle, DIVOOM_TASK_STACK_LVGL_WNOISE);
	}
	return phandle;
}

void divoom_lvgl_white_noise_list_menu_disp(void *handle, void *active_obj)
{
	Pdivoom_lvgl_white_noise_list_menu_ctrl phandle = (Pdivoom_lvgl_white_noise_list_menu_ctrl)handle;
	if (phandle == NULL)
		return;
	lv_group_focus_obj(phandle->cout_obj);
	lv_scr_load(phandle->main_menu);
	/* 首次进入时显示睡眠定时提示，3 秒后自动消失 */
	divoom_lvgl_white_noise_list_show_sleep_tip(handle);
}

void divoom_lvgl_white_noise_list_menu_pause(void *handle)
{
	Pdivoom_lvgl_white_noise_list_menu_ctrl phandle = (Pdivoom_lvgl_white_noise_list_menu_ctrl)handle;
	if (phandle == NULL)
		return;
	if (phandle->tip_timer) 
	{
		lv_timer_del(phandle->tip_timer);
		phandle->tip_timer = NULL;
	}
	if (phandle->tip_popup) 
	{
		lv_obj_del(phandle->tip_popup);
		phandle->tip_popup = NULL;
	}
	divoom_lvgl_white_noise_list_close_sleep_timer_popup(phandle);
	phandle->cache_run_flag = 0;
	if (phandle->cache_sem)
		divoom_system_get_sem(phandle->cache_sem);
	lv_img_cache_invalidate_src(NULL);
	divoom_lvgl_white_noise_list_free_img_cache(phandle);
}

void divoom_lvgl_white_noise_list_menu_exit(void *handle)
{
	Pdivoom_lvgl_white_noise_list_menu_ctrl phandle = (Pdivoom_lvgl_white_noise_list_menu_ctrl)handle;
	if (phandle == NULL)
		return;
	if (phandle->tip_timer) 
	{
		lv_timer_del(phandle->tip_timer);
		phandle->tip_timer = NULL;
	}
	if (phandle->tip_popup) 
	{
		lv_obj_del(phandle->tip_popup);
		phandle->tip_popup = NULL;
	}
	divoom_lvgl_white_noise_list_close_sleep_timer_popup(phandle);
	phandle->cache_run_flag = 0;
	if (phandle->cache_sem)
		divoom_system_get_sem(phandle->cache_sem);
	lv_img_cache_invalidate_src(NULL);
	divoom_lvgl_white_noise_list_free_img_cache(phandle);
	if (phandle->cache_lock) 
	{
		divoom_system_destroy_lock(phandle->cache_lock);
		phandle->cache_lock = NULL;
	}
	if (phandle->cache_sem) 
	{
		divoom_system_destroy_sem(phandle->cache_sem);
		phandle->cache_sem = NULL;
	}
	lv_obj_del(phandle->main_menu);
	lv_style_reset(&phandle->backgroud_type);
	lv_style_reset(&phandle->grid_type);
	lv_style_reset(&phandle->btn_type);
	lv_style_reset(&phandle->btn_press_type);
	divoom_system_free(phandle);
}
