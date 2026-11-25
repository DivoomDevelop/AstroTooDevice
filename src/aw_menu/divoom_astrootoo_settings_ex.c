#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_menu_direct_ctrl.h"
#include "divoom_main.h"
#include "divoom_bt_music.h"
#include "divoom_power_control.h"
#include "divoom_astrootoo_ui_base.h"
#include "divoom_astrootoo_theme.h"
#include "divoom_astrootoo_settings_ex.h"
#include "divoom_astrootoo_time_pick.h"
#include "divoom_pixel_format.h"
#include <string.h>

#define DIVOOM_ASTRO_SET_UI_DIR         "26/"
#define DIVOOM_ASTRO_SET_HEADER         DIVOOM_ASTRO_SET_UI_DIR "at_settings_header.png"
#define DIVOOM_ASTRO_SET_SWITCH_ON      DIVOOM_ASTRO_SET_UI_DIR "at_settings_switch_on.png"
#define DIVOOM_ASTRO_SET_SWITCH_OFF     DIVOOM_ASTRO_SET_UI_DIR "at_settings_switch_off.png"
#define DIVOOM_ASTRO_SET_CHEVRON        DIVOOM_ASTRO_SET_UI_DIR "at_settings_chevron.png"
#define DIVOOM_ASTRO_SET_CHECK_ICON     "23/at_repeat_check.png"
#define DIVOOM_ASTRO_SP_ICON_ON         DIVOOM_ASTRO_SET_UI_DIR "at_scheduled_power_on.png"
#define DIVOOM_ASTRO_SP_ICON_OFF        DIVOOM_ASTRO_SET_UI_DIR "at_scheduled_power_off.png"

#define DIVOOM_ASTRO_SET_ROW_W          444
#define DIVOOM_ASTRO_SET_ROW_H          72
#define DIVOOM_ASTRO_SET_ROW_GAP        8
#define DIVOOM_ASTRO_SET_ROW_X          ((DIVOOM_SCREEN_WIDTH - DIVOOM_ASTRO_SET_ROW_W) / 2)
#define DIVOOM_ASTRO_SET_TOP_GAP        8

#define DIVOOM_ASTRO_MCP_CARD_H         284
/* 真源 DeviceSettingsPcWatchfaceMcpScreen：qrImageSlot 128×128 */
#define DIVOOM_ASTRO_MCP_QR_SIZE        128
#define DIVOOM_ASTRO_MCP_QR_QUIET       4
#define DIVOOM_ASTRO_MCP_SCROLL_STEP    56
#define DIVOOM_ASTRO_MCP_DOT_SIZE       8
#define DIVOOM_ASTRO_MCP_DOT_GAP        8
#define DIVOOM_ASTRO_MCP_CARD_PAD_X     20
#define DIVOOM_ASTRO_MCP_CARD_PAD_TOP   16
#define DIVOOM_ASTRO_MCP_CARD_PAD_BOT   20
#define DIVOOM_ASTRO_MCP_CARD_ROW_GAP   10
#define DIVOOM_ASTRO_MCP_PAGER_GAP      8
#define DIVOOM_ASTRO_MCP_PAGER_PAD_BOT  12
#define DIVOOM_ASTRO_MCP_HELP_ROW_GAP   8
#define DIVOOM_ASTRO_MCP_HELP_SEC_GAP   16
#define DIVOOM_ASTRO_MCP_CONTACT_GAP    6

#define DIVOOM_ASTRO_BT_PW_DIGIT_EMPTY  (-1)
#define DIVOOM_ASTRO_BT_PW_FOCUS_TOGGLE 0
#define DIVOOM_ASTRO_BT_PW_FOCUS_DIGIT0 1
#define DIVOOM_ASTRO_BT_PW_FOCUS_DIGIT3 4
#define DIVOOM_ASTRO_BT_PW_FOCUS_SAVE    5

#define DIVOOM_ASTRO_SP_EDIT_ROW_H       64
#define DIVOOM_ASTRO_SP_EDIT_ROW_GAP     8
#define DIVOOM_ASTRO_SP_EDIT_PAD_X       16

#define DIVOOM_ASTRO_SP_MODE_LIST       0
#define DIVOOM_ASTRO_SP_MODE_EDIT       1
#define DIVOOM_ASTRO_SP_MODE_TIME_PICK  2
#define DIVOOM_ASTRO_SP_MODE_REPEAT_PICK 3
#define DIVOOM_ASTRO_SP_MODE_ACTION_PICK 4
#define DIVOOM_ASTRO_SP_COL_BODY        0
#define DIVOOM_ASTRO_SP_COL_TOGGLE      1
#define DIVOOM_ASTRO_SP_REPEAT_NEVER    7
#define DIVOOM_ASTRO_SP_MENU_ID         DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_POWER_OFF_TIME
/* 列表行 body 内 grid（真源 DeviceSettingsScheduledPowerScreen.module.css scheduleListBodyGrid） */
#define DIVOOM_ASTRO_SP_ICON_X          12
#define DIVOOM_ASTRO_SP_ICON_W          20
#define DIVOOM_ASTRO_SP_GRID_GAP        8
#define DIVOOM_ASTRO_SP_TIME_X          (DIVOOM_ASTRO_SP_ICON_X + DIVOOM_ASTRO_SP_ICON_W \
    + DIVOOM_ASTRO_SP_GRID_GAP)
#define DIVOOM_ASTRO_SP_TIME_COL_W      72
#define DIVOOM_ASTRO_SP_REPEAT_ANCHOR_X (-(14 + 12 + 8))
#define DIVOOM_ASTRO_SP_REPEAT_W        (DIVOOM_ASTRO_ALARM_BODY_W + DIVOOM_ASTRO_SP_REPEAT_ANCHOR_X \
    - DIVOOM_ASTRO_SP_TIME_X - DIVOOM_ASTRO_SP_TIME_COL_W - DIVOOM_ASTRO_SP_GRID_GAP)
/* 真源 actionPickRowInner gap:10；编辑行右侧值区 icon+文案 gap:6 */
#define DIVOOM_ASTRO_SP_ACTION_PICK_ICON_TEXT_GAP  10
#define DIVOOM_ASTRO_SP_EDIT_ACTION_ICON_TEXT_GAP  6

static const char *const g_mcp_title_keys[2] =
{
    "McpToolDownload",
    "McpService",
};

static const char *const g_mcp_url_lines[2][3] =
{
    { "https://github.com/DivoomDevelop", "/divoom-watchface-visual-editor_AstroToo", NULL },
    { "https://github.com/DivoomDevelop", "/mcp-divoom-lan", NULL },
};

static const char *const g_sp_repeat_keys[DIVOOM_ASTRO_SP_REPEAT_ROWS] =
{
    "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun", "Never",
};

static const char *const g_sp_edit_keys[DIVOOM_ASTRO_SP_EDIT_FIELDS] =
{
    "SpTime", "SpRepeat", "SpPowerAction",
};

static const char *const g_sp_action_keys[2] =
{
    "SpPowerOn", "SpPowerOff",
};

int divoom_astrootoo_settings_ex_is_page(divoom_astrootoo_settings_page_t page)
{
    if (page == DIVOOM_ASTROTOO_SETTINGS_PAGE_MCP
        || page == DIVOOM_ASTROTOO_SETTINGS_PAGE_BT_PASSWORD
        || page == DIVOOM_ASTROTOO_SETTINGS_PAGE_SCHEDULED_POWER)
    {
        return 1;
    }
    return 0;
}

static void divoom_astrootoo_settings_ex_create_header(Pdivoom_astrootoo_settings_ctrl phandle, const char *title_key)
{
    divoom_astrootoo_ui_create_page_header_tab(phandle->root_obj, &phandle->ui,
        title_key, DIVOOM_ASTRO_SET_HEADER, phandle->menu_id);
}

static void divoom_astrootoo_settings_ex_sp_bind_edit_val(Pdivoom_astrootoo_settings_ctrl phandle, int i)
{
    lv_coord_t trail_w;

    if (!phandle || i < 0 || i >= DIVOOM_ASTRO_SP_EDIT_FIELDS
        || !phandle->sp_edit_row[i] || !phandle->sp_edit_lbl[i] || !phandle->sp_edit_val[i])
    {
        return;
    }
    trail_w = DIVOOM_ASTRO_SUBLEVEL_CHEVRON_W;
    if (i == 2)
    {
        trail_w += 20 + DIVOOM_ASTRO_SP_EDIT_ACTION_ICON_TEXT_GAP;
    }
    divoom_astrootoo_ui_list_kv_bind_row(phandle->sp_edit_lbl[i], phandle->sp_edit_val[i],
        DIVOOM_ASTRO_ALARM_LIST_W, 12, 12, trail_w,
        -(12 + DIVOOM_ASTRO_SUBLEVEL_CHEVRON_W + DIVOOM_ASTRO_LIST_LABEL_GAP), 0);
    divoom_astrootoo_ui_list_row_bind_marquee(phandle->sp_edit_row[i], phandle->sp_edit_val[i]);
}

static void divoom_astrootoo_settings_sp_layout_action_value(Pdivoom_astrootoo_settings_ctrl phandle)
{
    lv_obj_t *val;
    lv_obj_t *key;
    lv_coord_t trail_w;
    lv_coord_t val_align_ofs;
    lv_coord_t max_w;
    lv_coord_t text_w;
    const char *txt;
    const lv_font_t *font;
    lv_point_t size;

    if (!phandle || !phandle->sp_edit_val[2] || !phandle->sp_edit_lbl[2])
    {
        return;
    }

    val = phandle->sp_edit_val[2];
    key = phandle->sp_edit_lbl[2];
    val_align_ofs = -(12 + DIVOOM_ASTRO_SUBLEVEL_CHEVRON_W + DIVOOM_ASTRO_LIST_LABEL_GAP);
    trail_w = DIVOOM_ASTRO_SUBLEVEL_CHEVRON_W + 20 + DIVOOM_ASTRO_SP_EDIT_ACTION_ICON_TEXT_GAP;

    lv_obj_update_layout(key);
    max_w = divoom_astrootoo_ui_list_kv_value_width(DIVOOM_ASTRO_ALARM_LIST_W, 12, 12,
        lv_obj_get_width(key), DIVOOM_ASTRO_LIST_KV_GAP, trail_w);
    if (max_w <= 0)
    {
        max_w = 1;
    }

    txt = lv_label_get_text(val);
    font = lv_obj_get_style_text_font(val, 0);
    text_w = max_w;
    if (txt && txt[0] && font)
    {
        lv_txt_get_size(&size, txt, font, lv_obj_get_style_text_letter_space(val, 0), 0,
            LV_COORD_MAX, LV_TEXT_FLAG_NONE);
        text_w = size.x;
        if (text_w > max_w)
        {
            text_w = max_w;
        }
    }

    divoom_astrootoo_ui_list_label_prepare(val, text_w);
    lv_obj_set_style_text_align(val, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(val, LV_ALIGN_RIGHT_MID, val_align_ofs, 0);

    if (phandle->sp_edit_action_icon)
    {
        lv_obj_update_layout(val);
        lv_obj_align_to(phandle->sp_edit_action_icon, val,
            LV_ALIGN_OUT_LEFT_MID, -DIVOOM_ASTRO_SP_EDIT_ACTION_ICON_TEXT_GAP, 0);
    }
}

static void divoom_astrootoo_settings_ex_sp_bind_pick_lbl(lv_obj_t *row, lv_obj_t *lbl, int with_icon)
{
    lv_coord_t inset_l;

    if (!row || !lbl)
    {
        return;
    }
    inset_l = with_icon ? (12 + 20 + 8) : 12;
    divoom_astrootoo_ui_list_label_prepare(lbl,
        divoom_astrootoo_ui_list_label_width(DIVOOM_ASTRO_ALARM_LIST_W, inset_l, 16,
            DIVOOM_ASTRO_LIST_CHK_TRAIL_ALIGN(16)));
    divoom_astrootoo_ui_list_row_bind_marquee(row, lbl);
}

static void divoom_astrootoo_settings_ex_apply_list_slot(lv_obj_t *obj, Pdivoom_astrootoo_settings_ctrl phandle, int focused)
{
    if (!obj || !phandle)
    {
        return;
    }
    /*
     * 蓝牙密码 / 定时开关机共用：两页按键都由隐藏 key_btn 承接，
     * group_bind 后卡片 LV_STATE_FOCUSED 会被清掉，必须用本地 bg/border 画焦点。
     * 真源 FlowtooListCardRowV1 wrap / wrapFocus。
     */
    if (focused)
    {
        lv_obj_add_state(obj, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
        lv_obj_set_style_bg_color(obj, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), 0);
        lv_obj_set_style_border_color(obj, lv_color_hex(DIVOOM_ASTRO_TAB_INK), 0);
    }
    else
    {
        lv_obj_clear_state(obj, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
        lv_obj_set_style_bg_color(obj, lv_color_hex(DIVOOM_ASTRO_BG_MUTED), 0);
        lv_obj_set_style_border_color(obj, lv_color_hex(DIVOOM_ASTRO_STROKE), 0);
    }
    lv_obj_set_style_border_width(obj, 2, 0);
    lv_obj_set_style_radius(obj, DIVOOM_ASTRO_RADIUS_SM, 0);
    lv_obj_set_style_outline_width(obj, 0, 0);
    lv_obj_set_style_outline_width(obj, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(obj, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_pad_all(obj, 0, 0);
}

/* ===================== MCP ===================== */

static void divoom_astrootoo_settings_mcp_prepare_wrap_label(lv_obj_t *label, lv_coord_t width,
    int font_px, int menu_id, lv_color_t color, lv_text_align_t align)
{
    if (!label)
    {
        return;
    }
    lv_obj_set_width(label, width);
    lv_obj_set_style_min_width(label, width, 0);
    lv_obj_set_style_max_width(label, width, 0);
    lv_obj_set_style_pad_all(label, 0, 0);
    lv_obj_set_style_text_align(label, align, 0);
    lv_obj_set_style_text_color(label, color, 0);
    lv_obj_set_style_text_font(label, divoom_menu_control_get_font_info(font_px, menu_id), 0);
    lv_obj_set_style_text_line_space(label, 0, 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
}

/*
 * FreeType line_height 常大于字号；WRAP 折行后默认行距易叠字。
 * 按真源 helpSectionBody line-height≈1.5 校正，且基线步进不低于字号。
 */
static void divoom_astrootoo_settings_mcp_fit_wrap_label(lv_obj_t *label, int font_px)
{
    const lv_font_t *font;
    const char *txt;
    lv_point_t sz;
    lv_coord_t line_h;
    lv_coord_t letter_space;
    lv_coord_t box_w;
    lv_coord_t line_space;
    int n;

    if (!label)
    {
        return;
    }
    font = lv_obj_get_style_text_font(label, 0);
    txt = lv_label_get_text(label);
    if (!font || !txt || !txt[0])
    {
        return;
    }
    line_h = (lv_coord_t)lv_font_get_line_height(font);
    if (line_h <= 0)
    {
        line_h = (lv_coord_t)font_px;
    }
    letter_space = lv_obj_get_style_text_letter_space(label, 0);
    box_w = lv_obj_get_width(label);
    if (box_w <= 0)
    {
        return;
    }

    lv_txt_get_size(&sz, txt, font, letter_space, 0, box_w, LV_TEXT_FLAG_NONE);
    n = (int)((sz.y + line_h / 2) / line_h);
    if (n < 2)
    {
        lv_obj_set_style_text_line_space(label, 0, 0);
        return;
    }

    line_space = (lv_coord_t)(font_px + font_px / 2) - line_h;
    if (line_space < (lv_coord_t)font_px - line_h)
    {
        line_space = (lv_coord_t)font_px - line_h;
    }
    lv_obj_set_style_text_line_space(label, line_space, 0);
}

static void divoom_astrootoo_settings_mcp_set_wrap_text(lv_obj_t *label, const char *text, int font_px)
{
    if (!label)
    {
        return;
    }
    lv_label_set_text(label, text ? text : "");
    divoom_astrootoo_settings_mcp_fit_wrap_label(label, font_px);
}

static void divoom_astrootoo_settings_mcp_release_qr(Pdivoom_astrootoo_settings_ctrl phandle)
{
    if (!phandle || !phandle->mcp_qr_buf)
    {
        return;
    }
    if (phandle->mcp_qr_img && phandle->root_obj)
    {
        lv_img_set_src(phandle->mcp_qr_img, NULL);
    }
    lv_img_cache_invalidate_src(&phandle->mcp_qr_dsc);
    divoom_system_free(phandle->mcp_qr_buf);
    phandle->mcp_qr_buf = NULL;
    phandle->mcp_qr_rendered_index = -1;
    memset(&phandle->mcp_qr_dsc, 0, sizeof(phandle->mcp_qr_dsc));
}

static int divoom_astrootoo_settings_mcp_set_qr(Pdivoom_astrootoo_settings_ctrl phandle)
{
    Pdivoom_qrencode_data qr;
    const char *const *url_lines;
    uint8 *buf;
    char url[128];
    size_t url_len = 0;
    uint32 module_px;
    uint32 origin;
    uint32 x;
    uint32 y;
    uint32 dx;
    uint32 dy;
    uint32 src_offset;
    uint32 dst_offset;
    int i;
    int n;

    if (!phandle || !phandle->mcp_qr_img || phandle->mcp_qr_index < 0
        || phandle->mcp_qr_index >= 2)
    {
        return 0;
    }
    url_lines = g_mcp_url_lines[phandle->mcp_qr_index];
    url[0] = '\0';
    for (i = 0; i < 3 && url_lines[i]; i++)
    {
        n = snprintf(url + url_len, sizeof(url) - url_len, "%s", url_lines[i]);
        if (n < 0 || (size_t)n >= sizeof(url) - url_len)
        {
            return 0;
        }
        url_len += (size_t)n;
    }
    divoom_astrootoo_settings_mcp_release_qr(phandle);
    qr = divoom_qrencode_string(url);
    if (!qr || !qr->rgb_data || qr->pic_width == 0
        || qr->pic_width != qr->pic_high)
    {
        divoom_system_free(qr);
        return 0;
    }
    module_px = DIVOOM_ASTRO_MCP_QR_SIZE
        / ((uint32)qr->pic_width + DIVOOM_ASTRO_MCP_QR_QUIET * 2U);
    if (module_px < 2U)
    {
        divoom_system_free(qr);
        return 0;
    }
    buf = (uint8 *)divoom_system_malloc(
        DIVOOM_ASTRO_MCP_QR_SIZE * DIVOOM_ASTRO_MCP_QR_SIZE
        * DIVOOM_APP_RGB565_BYTES_PER_PX);
    if (!buf)
    {
        divoom_system_free(qr);
        return 0;
    }
    memset(buf, 0xff, DIVOOM_ASTRO_MCP_QR_SIZE * DIVOOM_ASTRO_MCP_QR_SIZE
        * DIVOOM_APP_RGB565_BYTES_PER_PX);
    origin = (DIVOOM_ASTRO_MCP_QR_SIZE
        - ((uint32)qr->pic_width + DIVOOM_ASTRO_MCP_QR_QUIET * 2U) * module_px) / 2U
        + DIVOOM_ASTRO_MCP_QR_QUIET * module_px;
    for (y = 0; y < qr->pic_high; y++)
    {
        for (x = 0; x < qr->pic_width; x++)
        {
            src_offset = (y * (uint32)qr->pic_width + x) * 3U;
            if (qr->rgb_data[src_offset] != 0U)
            {
                continue;
            }
            for (dy = 0; dy < module_px; dy++)
            {
                for (dx = 0; dx < module_px; dx++)
                {
                    dst_offset = (((origin + y * module_px + dy) * DIVOOM_ASTRO_MCP_QR_SIZE)
                        + origin + x * module_px + dx) * DIVOOM_APP_RGB565_BYTES_PER_PX;
                    divoom_store_rgb565_le(buf + dst_offset, 0);
                }
            }
        }
    }
    divoom_system_free(qr);
    phandle->mcp_qr_buf = buf;
    phandle->mcp_qr_rendered_index = phandle->mcp_qr_index;
    divoom_lv_img_dsc_init_fmt(&phandle->mcp_qr_dsc,
        DIVOOM_ASTRO_MCP_QR_SIZE, DIVOOM_ASTRO_MCP_QR_SIZE,
        LV_COLOR_FORMAT_RGB565, buf);
    lv_img_set_src(phandle->mcp_qr_img, &phandle->mcp_qr_dsc);
    lv_img_set_size_mode(phandle->mcp_qr_img, LV_IMG_SIZE_MODE_VIRTUAL);
    lv_img_set_antialias(phandle->mcp_qr_img, 0);
    lv_img_set_zoom(phandle->mcp_qr_img, LV_IMG_ZOOM_NONE);
    return 1;
}

static void divoom_astrootoo_settings_mcp_refresh(Pdivoom_astrootoo_settings_ctrl phandle)
{
    int i;
    const char *title;
    const char *const *url_lines;

    if (!phandle)
    {
        return;
    }
    title = divoom_menu_control_get_disp_str(g_mcp_title_keys[phandle->mcp_qr_index]);
    if (phandle->mcp_title_lbl)
    {
        divoom_astrootoo_settings_mcp_set_wrap_text(phandle->mcp_title_lbl, title, 28);
    }
    if (phandle->mcp_qr_img && phandle->mcp_qr_rendered_index != phandle->mcp_qr_index
        && !divoom_astrootoo_settings_mcp_set_qr(phandle))
    {
        divoom_astrootoo_settings_mcp_release_qr(phandle);
    }
    if (phandle->mcp_hint_lbl)
    {
        lv_label_set_text(phandle->mcp_hint_lbl, divoom_menu_control_get_disp_str("McpScanHint"));
    }
    url_lines = g_mcp_url_lines[phandle->mcp_qr_index];
    for (i = 0; i < 3; i++)
    {
        if (!phandle->mcp_url_lbl[i])
        {
            continue;
        }
        if (url_lines[i])
        {
            lv_label_set_text(phandle->mcp_url_lbl[i], url_lines[i]);
            lv_obj_clear_flag(phandle->mcp_url_lbl[i], LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(phandle->mcp_url_lbl[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
    for (i = 0; i < 2; i++)
    {
        if (!phandle->mcp_dot[i])
        {
            continue;
        }
        if (i == phandle->mcp_qr_index)
        {
            lv_obj_set_style_bg_color(phandle->mcp_dot[i], lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), 0);
        }
        else
        {
            lv_obj_set_style_bg_color(phandle->mcp_dot[i], lv_color_hex(DIVOOM_ASTRO_STROKE), 0);
        }
    }
    if (phandle->mcp_dot[0])
    {
        lv_obj_t *dots_row = lv_obj_get_parent(phandle->mcp_dot[0]);
        lv_obj_t *pager = dots_row ? lv_obj_get_parent(dots_row) : NULL;

        if (pager)
        {
            lv_obj_update_layout(pager);
        }
    }
}

static void divoom_astrootoo_settings_mcp_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_settings_ctrl phandle;
    uint8 key;
    lv_obj_t *scroll;

    phandle = (Pdivoom_astrootoo_settings_ctrl)lv_event_get_user_data(e);
    if (!phandle || !divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }
    key = divoom_astrootoo_ui_key_code_from_event(e);
    scroll = phandle->content;
    if (divoom_astrootoo_ui_key_is_esc(key))
    {
        divoom_menu_control_return_pre();
        return;
    }
    if (divoom_astrootoo_ui_key_is_left(key))
    {
        phandle->mcp_qr_index = (phandle->mcp_qr_index + 1) % 2;
        divoom_astrootoo_settings_mcp_refresh(phandle);
        return;
    }
    if (divoom_astrootoo_ui_key_is_right(key))
    {
        phandle->mcp_qr_index = (phandle->mcp_qr_index + 1) % 2;
        divoom_astrootoo_settings_mcp_refresh(phandle);
        return;
    }
    if (!scroll)
    {
        return;
    }
    if (divoom_astrootoo_ui_key_is_up(key))
    {
        lv_coord_t y;

        y = lv_obj_get_scroll_y(scroll);
        lv_obj_scroll_to_y(scroll, y - DIVOOM_ASTRO_MCP_SCROLL_STEP, LV_ANIM_ON);
        return;
    }
    if (divoom_astrootoo_ui_key_is_down(key))
    {
        lv_coord_t y;

        y = lv_obj_get_scroll_y(scroll);
        lv_obj_scroll_to_y(scroll, y + DIVOOM_ASTRO_MCP_SCROLL_STEP, LV_ANIM_ON);
    }
}

static void divoom_astrootoo_settings_build_mcp(Pdivoom_astrootoo_settings_ctrl phandle)
{
    lv_obj_t *pager;
    lv_obj_t *card;
    lv_obj_t *footer;
    lv_obj_t *lbl;
    lv_obj_t *dots;
    lv_obj_t *sec;
    lv_obj_t *spacer;
    lv_coord_t y;
    lv_coord_t card_inner_w;
    int i;

    phandle->mcp_qr_index = 0;
    phandle->mcp_qr_buf = NULL;
    phandle->mcp_qr_rendered_index = -1;
    divoom_astrootoo_settings_ex_create_header(phandle, "PcWatchfaceMcp");
    phandle->content = divoom_astrootoo_ui_create_scroll_content(phandle->root_obj, &phandle->ui);
    /* 与其它设置页一致：用 ROW_X 居中 444，勿再加左右 pad，否则卡片溢出出现拖动条 */
    lv_obj_set_scrollbar_mode(phandle->content, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_bottom(phandle->content, 24, 0);

    card_inner_w = DIVOOM_ASTRO_SET_ROW_W - DIVOOM_ASTRO_MCP_CARD_PAD_X * 2;
    y = DIVOOM_ASTRO_SET_TOP_GAP;
    pager = lv_obj_create(phandle->content);
    lv_obj_add_style(pager, &phandle->ui.content_style, 0);
    lv_obj_set_width(pager, DIVOOM_ASTRO_SET_ROW_W);
    lv_obj_set_height(pager, LV_SIZE_CONTENT);
    lv_obj_set_pos(pager, DIVOOM_ASTRO_SET_ROW_X, y);
    lv_obj_set_style_bg_opa(pager, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(pager, 0, 0);
    lv_obj_set_style_pad_all(pager, 0, 0);
    lv_obj_set_style_pad_bottom(pager, DIVOOM_ASTRO_MCP_PAGER_PAD_BOT, 0);
    lv_obj_clear_flag(pager, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(pager, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(pager, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(pager, DIVOOM_ASTRO_MCP_PAGER_GAP, 0);

    card = lv_obj_create(pager);
    lv_obj_add_style(card, &phandle->ui.content_style, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(DIVOOM_ASTRO_BG_MUTED), 0);
    lv_obj_set_style_border_color(card, lv_color_hex(DIVOOM_ASTRO_STROKE), 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_radius(card, DIVOOM_ASTRO_RADIUS_SM, 0);
    lv_obj_set_width(card, LV_PCT(100));
    lv_obj_set_height(card, LV_SIZE_CONTENT);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(card, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_top(card, DIVOOM_ASTRO_MCP_CARD_PAD_TOP, 0);
    lv_obj_set_style_pad_bottom(card, DIVOOM_ASTRO_MCP_CARD_PAD_BOT, 0);
    lv_obj_set_style_pad_left(card, DIVOOM_ASTRO_MCP_CARD_PAD_X, 0);
    lv_obj_set_style_pad_right(card, DIVOOM_ASTRO_MCP_CARD_PAD_X, 0);
    lv_obj_set_style_pad_row(card, DIVOOM_ASTRO_MCP_CARD_ROW_GAP, 0);

    phandle->mcp_title_lbl = lv_label_create(card);
    divoom_astrootoo_settings_mcp_prepare_wrap_label(phandle->mcp_title_lbl, card_inner_w, 28,
        phandle->menu_id, lv_color_hex(DIVOOM_ASTRO_INK), LV_TEXT_ALIGN_CENTER);

    phandle->mcp_qr_img = lv_img_create(card);
    lv_obj_set_size(phandle->mcp_qr_img, DIVOOM_ASTRO_MCP_QR_SIZE, DIVOOM_ASTRO_MCP_QR_SIZE);
    lv_obj_clear_flag(phandle->mcp_qr_img, LV_OBJ_FLAG_SCROLLABLE);

    footer = lv_obj_create(card);
    lv_obj_add_style(footer, &phandle->ui.content_style, 0);
    lv_obj_set_width(footer, card_inner_w);
    lv_obj_set_height(footer, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_set_style_pad_all(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(footer, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(footer, 4, 0);

    phandle->mcp_hint_lbl = lv_label_create(footer);
    lv_obj_set_width(phandle->mcp_hint_lbl, card_inner_w);
    lv_obj_set_style_text_align(phandle->mcp_hint_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(phandle->mcp_hint_lbl, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
    lv_obj_set_style_text_font(phandle->mcp_hint_lbl, divoom_menu_control_get_font_info(20, phandle->menu_id), 0);

    for (i = 0; i < 3; i++)
    {
        phandle->mcp_url_lbl[i] = lv_label_create(footer);
        lv_obj_set_width(phandle->mcp_url_lbl[i], card_inner_w);
        lv_obj_set_style_text_align(phandle->mcp_url_lbl[i], LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(phandle->mcp_url_lbl[i], lv_color_hex(DIVOOM_ASTRO_INK), 0);
        lv_obj_set_style_text_font(phandle->mcp_url_lbl[i], divoom_menu_control_get_font_info(16, phandle->menu_id), 0);
        lv_label_set_long_mode(phandle->mcp_url_lbl[i], LV_LABEL_LONG_CLIP);
    }

    dots = lv_obj_create(pager);
    lv_obj_add_style(dots, &phandle->ui.content_style, 0);
    lv_obj_set_width(dots, LV_PCT(100));
    lv_obj_set_height(dots, DIVOOM_ASTRO_MCP_DOT_SIZE);
    lv_obj_set_style_bg_opa(dots, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(dots, 0, 0);
    lv_obj_set_style_pad_all(dots, 0, 0);
    lv_obj_clear_flag(dots, LV_OBJ_FLAG_SCROLLABLE);
    for (i = 0; i < 2; i++)
    {
        phandle->mcp_dot[i] = lv_obj_create(dots);
        lv_obj_set_size(phandle->mcp_dot[i], DIVOOM_ASTRO_MCP_DOT_SIZE, DIVOOM_ASTRO_MCP_DOT_SIZE);
        lv_obj_set_style_radius(phandle->mcp_dot[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(phandle->mcp_dot[i], 0, 0);
        lv_obj_set_style_pad_all(phandle->mcp_dot[i], 0, 0);
        lv_obj_set_pos(phandle->mcp_dot[i],
            (DIVOOM_ASTRO_SET_ROW_W - (2 * DIVOOM_ASTRO_MCP_DOT_SIZE + DIVOOM_ASTRO_MCP_DOT_GAP)) / 2
            + i * (DIVOOM_ASTRO_MCP_DOT_SIZE + DIVOOM_ASTRO_MCP_DOT_GAP), 0);
    }

    phandle->mcp_key_btn = divoom_astrootoo_ui_create_key_capture_btn(phandle->root_obj, &phandle->ui,
        divoom_astrootoo_settings_mcp_key_cb, phandle);

    divoom_astrootoo_settings_mcp_refresh(phandle);
    lv_obj_update_layout(pager);
    y += lv_obj_get_height(pager);

    lbl = lv_label_create(phandle->content);
    lv_label_set_text(lbl, divoom_menu_control_get_disp_str("McpHelpHeading"));
    lv_obj_set_width(lbl, DIVOOM_ASTRO_SET_ROW_W);
    lv_obj_set_style_text_color(lbl, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
    lv_obj_set_style_text_font(lbl, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);
    lv_obj_set_pos(lbl, DIVOOM_ASTRO_SET_ROW_X, y + 4);
    lv_obj_update_layout(lbl);
    y += 4 + lv_obj_get_height(lbl) + 8;

    for (i = 0; i < 4; i++)
    {
        char tkey[24];
        char bkey[24];

        snprintf(tkey, sizeof(tkey), "McpHelp%dTitle", i + 1);
        snprintf(bkey, sizeof(bkey), "McpHelp%dBody", i + 1);
        sec = lv_obj_create(phandle->content);
        lv_obj_add_style(sec, &phandle->ui.content_style, 0);
        lv_obj_set_size(sec, DIVOOM_ASTRO_SET_ROW_W, LV_SIZE_CONTENT);
        lv_obj_set_pos(sec, DIVOOM_ASTRO_SET_ROW_X, y);
        lv_obj_set_style_bg_opa(sec, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(sec, 0, 0);
        lv_obj_set_style_pad_all(sec, 0, 0);
        lv_obj_set_flex_flow(sec, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(sec, DIVOOM_ASTRO_MCP_HELP_ROW_GAP, 0);

        lbl = lv_label_create(sec);
        divoom_astrootoo_settings_mcp_prepare_wrap_label(lbl, DIVOOM_ASTRO_SET_ROW_W, 22,
            phandle->menu_id, lv_color_hex(DIVOOM_ASTRO_INK), LV_TEXT_ALIGN_LEFT);
        divoom_astrootoo_settings_mcp_set_wrap_text(lbl, divoom_menu_control_get_disp_str(tkey), 22);

        lbl = lv_label_create(sec);
        divoom_astrootoo_settings_mcp_prepare_wrap_label(lbl, DIVOOM_ASTRO_SET_ROW_W, 20,
            phandle->menu_id, lv_color_hex(DIVOOM_ASTRO_INK), LV_TEXT_ALIGN_LEFT);
        divoom_astrootoo_settings_mcp_set_wrap_text(lbl, divoom_menu_control_get_disp_str(bkey), 20);
        lv_obj_update_layout(sec);
        y += lv_obj_get_height(sec) + DIVOOM_ASTRO_MCP_HELP_SEC_GAP;
    }

    y += DIVOOM_ASTRO_MCP_HELP_SEC_GAP;
    lbl = lv_label_create(phandle->content);
    lv_label_set_text(lbl, divoom_menu_control_get_disp_str("McpContactHeading"));
    lv_obj_set_width(lbl, DIVOOM_ASTRO_SET_ROW_W);
    lv_obj_set_style_text_color(lbl, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
    lv_obj_set_style_text_font(lbl, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);
    lv_obj_set_pos(lbl, DIVOOM_ASTRO_SET_ROW_X, y);
    lv_obj_update_layout(lbl);
    y += lv_obj_get_height(lbl) + DIVOOM_ASTRO_MCP_CONTACT_GAP;

    lbl = lv_label_create(phandle->content);
    lv_label_set_text(lbl, divoom_menu_control_get_disp_str("McpContactEmail"));
    lv_obj_set_width(lbl, DIVOOM_ASTRO_SET_ROW_W);
    lv_obj_set_style_text_color(lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(lbl, divoom_menu_control_get_font_info(22, phandle->menu_id), 0);
    lv_obj_set_pos(lbl, DIVOOM_ASTRO_SET_ROW_X, y);
    lv_obj_update_layout(lbl);
    y += lv_obj_get_height(lbl) + 24;

    spacer = lv_obj_create(phandle->content);
    lv_obj_set_size(spacer, 1, 1);
    lv_obj_set_pos(spacer, 0, y);
    lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer, 0, 0);
    lv_obj_clear_flag(spacer, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_update_layout(phandle->content);
}

/* ===================== 蓝牙密码 ===================== */

static int divoom_astrootoo_settings_bt_pw_char_to_digit(char c)
{
    if (c >= '0' && c <= '9')
    {
        return (int)(c - '0');
    }
    if (c >= '1' && c <= '9')
    {
        return (int)(c - '1');
    }
    return DIVOOM_ASTRO_BT_PW_DIGIT_EMPTY;
}

static char divoom_astrootoo_settings_bt_pw_digit_to_char(int8 d)
{
    if (d >= 0 && d <= 9)
    {
        return (char)('0' + d);
    }
    return '0';
}

static int divoom_astrootoo_settings_bt_pw_can_commit(Pdivoom_astrootoo_settings_ctrl phandle)
{
    int i;

    if (!phandle->bt_pw_draft_enabled)
    {
        return 1;
    }
    for (i = 0; i < 4; i++)
    {
        if (phandle->bt_pw_digits[i] < 0 || phandle->bt_pw_digits[i] > 9)
        {
            return 0;
        }
    }
    return 1;
}

static int divoom_astrootoo_settings_bt_pw_is_dirty(Pdivoom_astrootoo_settings_ctrl phandle)
{
    int i;

    if (!phandle)
    {
        return 0;
    }
    if (phandle->bt_pw_draft_enabled != phandle->bt_pw_baseline_enabled)
    {
        return 1;
    }
    if (!phandle->bt_pw_draft_enabled)
    {
        return 0;
    }
    for (i = 0; i < 4; i++)
    {
        if (phandle->bt_pw_digits[i] != phandle->bt_pw_baseline_digits[i])
        {
            return 1;
        }
    }
    return 0;
}

static int divoom_astrootoo_settings_bt_pw_save_ready(Pdivoom_astrootoo_settings_ctrl phandle)
{
    if (!phandle)
    {
        return 0;
    }
    return divoom_astrootoo_settings_bt_pw_is_dirty(phandle)
        && divoom_astrootoo_settings_bt_pw_can_commit(phandle);
}

static void divoom_astrootoo_settings_ex_obj_no_scroll(lv_obj_t *obj)
{
    if (!obj)
    {
        return;
    }
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

static void divoom_astrootoo_settings_bt_pw_refresh(Pdivoom_astrootoo_settings_ctrl phandle)
{
    int i;
    int focused;
    int save_ready;
    const char *sw_src;
    char ch_buf[2];

    if (!phandle)
    {
        return;
    }
    save_ready = divoom_astrootoo_settings_bt_pw_save_ready(phandle);
    sw_src = phandle->bt_pw_draft_enabled ? DIVOOM_ASTRO_SET_SWITCH_ON : DIVOOM_ASTRO_SET_SWITCH_OFF;
    if (phandle->bt_pw_toggle_sw)
    {
        divoom_menu_control_lv_img_set_src(phandle->bt_pw_toggle_sw, sw_src);
    }
    for (i = 0; i < 4; i++)
    {
        focused = (phandle->bt_pw_focus == DIVOOM_ASTRO_BT_PW_FOCUS_DIGIT0 + i);
        if (!phandle->bt_pw_digit_btn[i] || !phandle->bt_pw_digit_lbl[i])
        {
            continue;
        }
        if (!phandle->bt_pw_draft_enabled)
        {
            lv_label_set_text(phandle->bt_pw_digit_lbl[i], "-");
            lv_obj_set_style_bg_color(phandle->bt_pw_digit_btn[i], lv_color_hex(DIVOOM_ASTRO_SURFACE_IDLE), 0);
            lv_obj_set_style_text_color(phandle->bt_pw_digit_lbl[i], lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
        }
        else if (phandle->bt_pw_digits[i] >= 0 && phandle->bt_pw_digits[i] <= 9)
        {
            ch_buf[0] = divoom_astrootoo_settings_bt_pw_digit_to_char(phandle->bt_pw_digits[i]);
            ch_buf[1] = '\0';
            lv_label_set_text(phandle->bt_pw_digit_lbl[i], ch_buf);
            if (focused)
            {
                lv_obj_set_style_bg_color(phandle->bt_pw_digit_btn[i], lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), 0);
                lv_obj_set_style_text_color(phandle->bt_pw_digit_lbl[i], lv_color_hex(DIVOOM_ASTRO_BG), 0);
            }
            else
            {
                lv_obj_set_style_bg_color(phandle->bt_pw_digit_btn[i], lv_color_hex(DIVOOM_ASTRO_SURFACE_IDLE), 0);
                lv_obj_set_style_text_color(phandle->bt_pw_digit_lbl[i], lv_color_hex(DIVOOM_ASTRO_INK), 0);
            }
        }
        else
        {
            lv_label_set_text(phandle->bt_pw_digit_lbl[i], focused ? "0" : "-");
            if (focused)
            {
                lv_obj_set_style_bg_color(phandle->bt_pw_digit_btn[i], lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), 0);
                lv_obj_set_style_text_color(phandle->bt_pw_digit_lbl[i], lv_color_hex(DIVOOM_ASTRO_BG), 0);
            }
            else
            {
                lv_obj_set_style_bg_color(phandle->bt_pw_digit_btn[i], lv_color_hex(DIVOOM_ASTRO_SURFACE_IDLE), 0);
                lv_obj_set_style_text_color(phandle->bt_pw_digit_lbl[i], lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
            }
        }
    }
    if (phandle->bt_pw_save_btn)
    {
        if (phandle->bt_pw_focus == DIVOOM_ASTRO_BT_PW_FOCUS_SAVE)
        {
            lv_obj_set_style_bg_color(phandle->bt_pw_save_btn, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), 0);
            lv_obj_set_style_text_color(phandle->bt_pw_save_lbl, lv_color_hex(DIVOOM_ASTRO_BG), 0);
            lv_obj_set_style_opa(phandle->bt_pw_save_btn, LV_OPA_COVER, 0);
        }
        else if (save_ready)
        {
            lv_obj_set_style_bg_color(phandle->bt_pw_save_btn, lv_color_hex(0x6BA876), 0);
            lv_obj_set_style_text_color(phandle->bt_pw_save_lbl, lv_color_hex(DIVOOM_ASTRO_BG), 0);
            lv_obj_set_style_opa(phandle->bt_pw_save_btn, LV_OPA_COVER, 0);
        }
        else
        {
            lv_obj_set_style_bg_color(phandle->bt_pw_save_btn, lv_color_hex(DIVOOM_ASTRO_BG_MUTED), 0);
            lv_obj_set_style_text_color(phandle->bt_pw_save_lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
            lv_obj_set_style_opa(phandle->bt_pw_save_btn, LV_OPA_50, 0);
        }
    }
    if (phandle->bt_pw_toggle_card)
    {
        divoom_astrootoo_settings_ex_apply_list_slot(phandle->bt_pw_toggle_card, phandle,
            phandle->bt_pw_focus == DIVOOM_ASTRO_BT_PW_FOCUS_TOGGLE);
    }
}

static int divoom_astrootoo_settings_bt_pw_move_focus(Pdivoom_astrootoo_settings_ctrl phandle, int delta)
{
    int order[6];
    int cnt;
    int i;
    int idx;

    cnt = 0;
    order[cnt++] = DIVOOM_ASTRO_BT_PW_FOCUS_TOGGLE;
    if (phandle->bt_pw_draft_enabled)
    {
        order[cnt++] = DIVOOM_ASTRO_BT_PW_FOCUS_DIGIT0;
        order[cnt++] = DIVOOM_ASTRO_BT_PW_FOCUS_DIGIT0 + 1;
        order[cnt++] = DIVOOM_ASTRO_BT_PW_FOCUS_DIGIT0 + 2;
        order[cnt++] = DIVOOM_ASTRO_BT_PW_FOCUS_DIGIT0 + 3;
    }
    order[cnt++] = DIVOOM_ASTRO_BT_PW_FOCUS_SAVE;
    idx = 0;
    for (i = 0; i < cnt; i++)
    {
        if (order[i] == phandle->bt_pw_focus)
        {
            idx = i;
            break;
        }
    }
    idx = (idx + delta + cnt) % cnt;
    return order[idx];
}

static void divoom_astrootoo_settings_bt_pw_commit_save(Pdivoom_astrootoo_settings_ctrl phandle)
{
    Pdivoom_sys_setting_info cfg;
    int i;

    if (!divoom_astrootoo_settings_bt_pw_save_ready(phandle))
    {
        return;
    }
    cfg = divoom_main_get_system_config();
    if (!cfg)
    {
        return;
    }
    cfg->bt_secret_switch = phandle->bt_pw_draft_enabled ? 1 : 0;
    for (i = 0; i < 4; i++)
    {
        if (phandle->bt_pw_draft_enabled && phandle->bt_pw_digits[i] >= 0)
        {
            cfg->bt_password[i] = divoom_astrootoo_settings_bt_pw_digit_to_char(phandle->bt_pw_digits[i]);
        }
        else
        {
            cfg->bt_password[i] = '0';
        }
    }
    divoom_main_save_config();
    divoom_bt_music_set_pair_pin(cfg->bt_secret_switch, cfg->bt_password);
    divoom_menu_control_return_pre();
}

static void divoom_astrootoo_settings_bt_pw_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_settings_ctrl phandle;
    uint8 key;
    int di;

    if (lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }
    phandle = (Pdivoom_astrootoo_settings_ctrl)lv_event_get_user_data(e);
    if (!phandle)
    {
        return;
    }
    key = divoom_astrootoo_ui_key_code_from_event(e);
    if (divoom_astrootoo_ui_key_is_esc(key))
    {
        divoom_menu_control_return_pre();
        return;
    }
    if (divoom_astrootoo_ui_key_is_left(key) || divoom_astrootoo_ui_key_is_right(key))
    {
        int delta;

        delta = divoom_astrootoo_ui_key_is_right(key) ? 1 : -1;
        phandle->bt_pw_focus = divoom_astrootoo_settings_bt_pw_move_focus(phandle, delta);
        divoom_astrootoo_settings_bt_pw_refresh(phandle);
        return;
    }
    if (divoom_astrootoo_ui_key_is_up(key) || divoom_astrootoo_ui_key_is_down(key))
    {
        if (phandle->bt_pw_focus < DIVOOM_ASTRO_BT_PW_FOCUS_DIGIT0
            || phandle->bt_pw_focus > DIVOOM_ASTRO_BT_PW_FOCUS_DIGIT3
            || !phandle->bt_pw_draft_enabled)
        {
            int delta;

            delta = divoom_astrootoo_ui_key_is_down(key) ? 1 : -1;
            phandle->bt_pw_focus = divoom_astrootoo_settings_bt_pw_move_focus(phandle, delta);
            divoom_astrootoo_settings_bt_pw_refresh(phandle);
            return;
        }
        di = phandle->bt_pw_focus - DIVOOM_ASTRO_BT_PW_FOCUS_DIGIT0;
        if (phandle->bt_pw_digits[di] < 0)
        {
            phandle->bt_pw_digits[di] = 0;
        }
        if (divoom_astrootoo_ui_key_is_up(key))
        {
            phandle->bt_pw_digits[di] = (int8)((phandle->bt_pw_digits[di] + 1) % 10);
        }
        else
        {
            phandle->bt_pw_digits[di] = (int8)((phandle->bt_pw_digits[di] + 9) % 10);
        }
        divoom_astrootoo_settings_bt_pw_refresh(phandle);
        return;
    }
    if (!divoom_astrootoo_ui_key_is_enter(key))
    {
        return;
    }
    if (phandle->bt_pw_focus == DIVOOM_ASTRO_BT_PW_FOCUS_TOGGLE)
    {
        phandle->bt_pw_draft_enabled = phandle->bt_pw_draft_enabled ? 0 : 1;
        if (!phandle->bt_pw_draft_enabled)
        {
            if (phandle->bt_pw_focus > DIVOOM_ASTRO_BT_PW_FOCUS_TOGGLE)
            {
                phandle->bt_pw_focus = DIVOOM_ASTRO_BT_PW_FOCUS_SAVE;
            }
        }
        else
        {
            phandle->bt_pw_focus = DIVOOM_ASTRO_BT_PW_FOCUS_DIGIT0;
        }
        divoom_astrootoo_settings_bt_pw_refresh(phandle);
        return;
    }
    if (divoom_astrootoo_settings_bt_pw_save_ready(phandle))
    {
        divoom_astrootoo_settings_bt_pw_commit_save(phandle);
        return;
    }
    if (phandle->bt_pw_focus >= DIVOOM_ASTRO_BT_PW_FOCUS_DIGIT0
        && phandle->bt_pw_focus <= DIVOOM_ASTRO_BT_PW_FOCUS_DIGIT3
        && phandle->bt_pw_draft_enabled)
    {
        di = phandle->bt_pw_focus - DIVOOM_ASTRO_BT_PW_FOCUS_DIGIT0;
        if (phandle->bt_pw_digits[di] < 0)
        {
            phandle->bt_pw_digits[di] = 0;
        }
        else
        {
            phandle->bt_pw_focus = divoom_astrootoo_settings_bt_pw_move_focus(phandle, 1);
        }
        divoom_astrootoo_settings_bt_pw_refresh(phandle);
    }
}

static void divoom_astrootoo_settings_build_bt_password(Pdivoom_astrootoo_settings_ctrl phandle)
{
    Pdivoom_sys_setting_info cfg;
    lv_obj_t *zone;
    lv_obj_t *lbl;
    lv_obj_t *grid;
    lv_coord_t y;
    int i;
    int cell_w;

    cfg = divoom_main_get_system_config();
    phandle->bt_pw_draft_enabled = (cfg && cfg->bt_secret_switch) ? 1 : 0;
    phandle->bt_pw_baseline_enabled = phandle->bt_pw_draft_enabled;
    for (i = 0; i < 4; i++)
    {
        phandle->bt_pw_digits[i] = cfg ? divoom_astrootoo_settings_bt_pw_char_to_digit(cfg->bt_password[i]) : DIVOOM_ASTRO_BT_PW_DIGIT_EMPTY;
        phandle->bt_pw_baseline_digits[i] = phandle->bt_pw_digits[i];
    }
    phandle->bt_pw_focus = DIVOOM_ASTRO_BT_PW_FOCUS_TOGGLE;

    divoom_astrootoo_settings_ex_create_header(phandle, "BTPassword");
    phandle->content = divoom_astrootoo_ui_create_scroll_content(phandle->root_obj, &phandle->ui);
    divoom_astrootoo_settings_ex_obj_no_scroll(phandle->content);
    lv_obj_set_style_pad_left(phandle->content, 2, 0);
    lv_obj_set_style_pad_right(phandle->content, 2, 0);

    y = DIVOOM_ASTRO_SET_TOP_GAP;
    phandle->bt_pw_toggle_card = lv_btn_create(phandle->content);
    lv_obj_add_style(phandle->bt_pw_toggle_card, &phandle->ui.card_item_style, 0);
    lv_obj_add_style(phandle->bt_pw_toggle_card, &phandle->ui.card_item_focus_style, LV_STATE_FOCUSED);
    lv_obj_add_style(phandle->bt_pw_toggle_card, &phandle->ui.card_item_focus_style, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(phandle->bt_pw_toggle_card, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(phandle->bt_pw_toggle_card, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_size(phandle->bt_pw_toggle_card, DIVOOM_ASTRO_SET_ROW_W, DIVOOM_ASTRO_SET_ROW_H);
    lv_obj_set_pos(phandle->bt_pw_toggle_card, DIVOOM_ASTRO_SET_ROW_X, y);
    divoom_astrootoo_settings_ex_obj_no_scroll(phandle->bt_pw_toggle_card);
    lbl = lv_label_create(phandle->bt_pw_toggle_card);
    lv_label_set_text(lbl, divoom_menu_control_get_disp_str("BtPwEnable"));
    lv_obj_set_style_text_font(lbl, divoom_menu_control_get_font_info(28, phandle->menu_id), 0);
    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 12, 0);
    divoom_astrootoo_ui_list_label_prepare(lbl,
        divoom_astrootoo_ui_list_label_width(DIVOOM_ASTRO_SET_ROW_W,
            12 + DIVOOM_ASTRO_SET_CARD_PAD_H, 16 + DIVOOM_ASTRO_SET_CARD_PAD_H,
            DIVOOM_ASTRO_LIST_TOGGLE_W));
    divoom_astrootoo_ui_list_row_bind_marquee(phandle->bt_pw_toggle_card, lbl);
    phandle->bt_pw_toggle_sw = lv_img_create(phandle->bt_pw_toggle_card);
    lv_obj_align(phandle->bt_pw_toggle_sw, LV_ALIGN_RIGHT_MID, -16, 0);
    y += DIVOOM_ASTRO_SET_ROW_H + 10;

    zone = lv_obj_create(phandle->content);
    lv_obj_add_style(zone, &phandle->ui.content_style, 0);
    lv_obj_set_style_bg_color(zone, lv_color_hex(DIVOOM_ASTRO_BG_MUTED), 0);
    lv_obj_set_style_border_color(zone, lv_color_hex(DIVOOM_ASTRO_STROKE), 0);
    lv_obj_set_style_border_width(zone, 2, 0);
    lv_obj_set_style_radius(zone, DIVOOM_ASTRO_RADIUS_SM, 0);
    lv_obj_set_style_pad_all(zone, 12, 0);
    lv_obj_set_size(zone, DIVOOM_ASTRO_SET_ROW_W, 120);
    lv_obj_set_pos(zone, DIVOOM_ASTRO_SET_ROW_X, y);
    divoom_astrootoo_settings_ex_obj_no_scroll(zone);
    lbl = lv_label_create(zone);
    lv_label_set_text(lbl, divoom_menu_control_get_disp_str("BtPwSetPassword"));
    lv_obj_set_style_text_color(lbl, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
    lv_obj_set_style_text_font(lbl, divoom_menu_control_get_font_info(22, phandle->menu_id), 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 0, 0);
    grid = lv_obj_create(zone);
    lv_obj_add_style(grid, &phandle->ui.content_style, 0);
    lv_obj_set_size(grid, DIVOOM_ASTRO_SET_ROW_W - 24, 64);
    lv_obj_align(grid, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    divoom_astrootoo_settings_ex_obj_no_scroll(grid);
    cell_w = (DIVOOM_ASTRO_SET_ROW_W - 24 - 3 * 8) / 4;
    for (i = 0; i < 4; i++)
    {
        phandle->bt_pw_digit_btn[i] = lv_obj_create(grid);
        lv_obj_set_size(phandle->bt_pw_digit_btn[i], cell_w, 64);
        lv_obj_set_pos(phandle->bt_pw_digit_btn[i], i * (cell_w + 8), 0);
        lv_obj_set_style_radius(phandle->bt_pw_digit_btn[i], DIVOOM_ASTRO_RADIUS_SM, 0);
        lv_obj_set_style_border_width(phandle->bt_pw_digit_btn[i], 2, 0);
        lv_obj_set_style_border_color(phandle->bt_pw_digit_btn[i], lv_color_hex(DIVOOM_ASTRO_STROKE), 0);
        divoom_astrootoo_settings_ex_obj_no_scroll(phandle->bt_pw_digit_btn[i]);
        phandle->bt_pw_digit_lbl[i] = lv_label_create(phandle->bt_pw_digit_btn[i]);
        lv_obj_set_style_text_font(phandle->bt_pw_digit_lbl[i], divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
        lv_obj_center(phandle->bt_pw_digit_lbl[i]);
    }
    y += 130;

    phandle->bt_pw_save_btn = lv_btn_create(phandle->content);
    lv_obj_set_size(phandle->bt_pw_save_btn, DIVOOM_ASTRO_SET_ROW_W, 56);
    lv_obj_set_pos(phandle->bt_pw_save_btn, DIVOOM_ASTRO_SET_ROW_X, y);
    lv_obj_set_style_radius(phandle->bt_pw_save_btn, 14, 0);
    lv_obj_set_style_border_width(phandle->bt_pw_save_btn, 2, 0);
    lv_obj_set_style_border_color(phandle->bt_pw_save_btn, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    divoom_astrootoo_settings_ex_obj_no_scroll(phandle->bt_pw_save_btn);
    phandle->bt_pw_save_lbl = lv_label_create(phandle->bt_pw_save_btn);
    lv_label_set_text(phandle->bt_pw_save_lbl, divoom_menu_control_get_disp_str("BtPwSave"));
    lv_obj_set_style_text_font(phandle->bt_pw_save_lbl, divoom_menu_control_get_font_info(28, phandle->menu_id), 0);
    lv_obj_center(phandle->bt_pw_save_lbl);

    phandle->bt_pw_key_btn = divoom_astrootoo_ui_create_key_capture_btn(phandle->root_obj, &phandle->ui,
        divoom_astrootoo_settings_bt_pw_key_cb, phandle);
    divoom_astrootoo_settings_ex_obj_no_scroll(phandle->root_obj);
    divoom_astrootoo_settings_bt_pw_refresh(phandle);
}

/* ===================== 定时开关机（续下） ===================== */

/* weekday_mask bit0=周一 … bit6=周日；week 与闹钟相同：bit0=周日, bit1=周一 … bit6=周六 */
static uint8 divoom_astrootoo_settings_sp_mask_from_week(uint8 week)
{
    uint8 mask;
    int i;

    mask = 0;
    for (i = 0; i < 7; i++)
    {
        if (week & (uint8)(1 << ((i + 1) % 7)))
        {
            mask |= (uint8)(1 << i);
        }
    }
    return mask;
}

static uint8 divoom_astrootoo_settings_sp_week_from_mask(uint8 mask)
{
    uint8 week;
    int i;

    week = 0;
    for (i = 0; i < 7; i++)
    {
        if (mask & (1 << i))
        {
            week |= (uint8)(1 << ((i + 1) % 7));
        }
    }
    return week;
}

static void divoom_astrootoo_settings_sp_load(Pdivoom_astrootoo_settings_ctrl phandle)
{
    Pdivoom_power_control_item items;
    int i;

    items = (Pdivoom_power_control_item)divoom_main_model_get_power_on_item();
    for (i = 0; i < DIVOOM_ASTRO_SP_SCHEDULE_COUNT; i++)
    {
        if (items)
        {
            phandle->sp_items[i].enabled = items[i].on_off ? 1 : 0;
            phandle->sp_items[i].hour = items[i].hour;
            phandle->sp_items[i].minute = items[i].minute;
            phandle->sp_items[i].weekday_mask = divoom_astrootoo_settings_sp_mask_from_week(items[i].week);
            phandle->sp_items[i].action = items[i].mode ? 1 : 0;
        }
        else
        {
            memset(&phandle->sp_items[i], 0, sizeof(phandle->sp_items[i]));
        }
    }
}

static void divoom_astrootoo_settings_sp_save_slot(Pdivoom_astrootoo_settings_ctrl phandle, int index)
{
    Pdivoom_power_control_item items;
    divoom_astrootoo_settings_sp_item_t *it;

    if (index < 0 || index >= DIVOOM_ASTRO_SP_SCHEDULE_COUNT)
    {
        return;
    }
    items = (Pdivoom_power_control_item)divoom_main_model_get_power_on_item();
    if (!items)
    {
        return;
    }
    it = &phandle->sp_items[index];
    items[index].on_off = it->enabled ? 1 : 0;
    items[index].hour = it->hour;
    items[index].minute = it->minute;
    items[index].week = divoom_astrootoo_settings_sp_week_from_mask(it->weekday_mask);
    items[index].mode = it->action ? 1 : 0;
    divoom_power_control_send(&items[index]);
    divoom_power_control_update(NULL, 2);
    divoom_main_save_config();
}

static void divoom_astrootoo_settings_sp_format_time_parts(char *tbuf, int tlen, char *mbuf, int mlen,
    uint8 hour, uint8 minute)
{
    if (tbuf && tlen > 0)
    {
        tbuf[0] = '\0';
    }
    if (mbuf && mlen > 0)
    {
        mbuf[0] = '\0';
    }
    if (!tbuf || tlen < 8)
    {
        return;
    }
    if (divoom_astrootoo_time_pick_use_hour24())
    {
        snprintf(tbuf, (size_t)tlen, "%02u:%02u", (unsigned)hour, (unsigned)minute);
        return;
    }
    {
        int h12;
        const char *ap;

        h12 = hour % 12;
        if (h12 == 0)
        {
            h12 = 12;
        }
        snprintf(tbuf, (size_t)tlen, "%02d:%02u", h12, (unsigned)minute);
        if (mbuf && mlen > 0)
        {
            ap = divoom_menu_control_get_disp_str(hour < 12 ? "AM" : "PM");
            if (ap)
            {
                snprintf(mbuf, (size_t)mlen, "%s", ap);
            }
        }
    }
}

static void divoom_astrootoo_settings_sp_format_time(char *buf, int len, uint8 hour, uint8 minute)
{
    char tbuf[16];
    char mbuf[16];

    if (!buf || len <= 0)
    {
        return;
    }
    divoom_astrootoo_settings_sp_format_time_parts(tbuf, (int)sizeof(tbuf), mbuf, (int)sizeof(mbuf),
        hour, minute);
    if (mbuf[0] != '\0')
    {
        snprintf(buf, (size_t)len, "%s %s", tbuf, mbuf);
    }
    else
    {
        snprintf(buf, (size_t)len, "%s", tbuf);
    }
}

static void divoom_astrootoo_settings_sp_layout_list_row(Pdivoom_astrootoo_settings_ctrl phandle, int row)
{
    lv_coord_t time_w;
    lv_coord_t mer_w;
    lv_coord_t repeat_x;
    lv_coord_t repeat_w;
    const lv_coord_t repeat_right = DIVOOM_ASTRO_ALARM_BODY_W + DIVOOM_ASTRO_SP_REPEAT_ANCHOR_X;

    if (!phandle || row < 0 || row >= DIVOOM_ASTRO_SP_SCHEDULE_COUNT
        || !phandle->sp_row_time[row] || !phandle->sp_row_sub[row])
    {
        return;
    }

    lv_obj_update_layout(phandle->sp_row_time[row]);
    time_w = lv_obj_get_width(phandle->sp_row_time[row]);
    mer_w = 0;
    if (phandle->sp_row_meridiem[row]
        && !lv_obj_has_flag(phandle->sp_row_meridiem[row], LV_OBJ_FLAG_HIDDEN))
    {
        lv_obj_align_to(phandle->sp_row_meridiem[row], phandle->sp_row_time[row],
            LV_ALIGN_OUT_RIGHT_MID, DIVOOM_ASTRO_TIME_MERIDIEM_GAP, 0);
        lv_obj_update_layout(phandle->sp_row_meridiem[row]);
        mer_w = DIVOOM_ASTRO_TIME_MERIDIEM_GAP + lv_obj_get_width(phandle->sp_row_meridiem[row]);
    }
    repeat_x = DIVOOM_ASTRO_SP_TIME_X + time_w + mer_w + DIVOOM_ASTRO_SP_GRID_GAP;
    repeat_w = repeat_right - repeat_x;
    if (repeat_w < 48)
    {
        repeat_w = 48;
    }
    lv_obj_set_width(phandle->sp_row_sub[row], repeat_w);
    lv_obj_align(phandle->sp_row_sub[row], LV_ALIGN_LEFT_MID, repeat_x, 0);
}

static void divoom_astrootoo_settings_sp_format_repeat(char *buf, int len, uint8 mask)
{
    static const char * const week_abbr[7] = { "Mo", "Tu", "We", "Th", "Fr", "Sa", "Su" };
    int i;
    int idx;

    if (!buf || len <= 0)
    {
        return;
    }
    buf[0] = '\0';

    /* 真源 ScheduledPowerRepeatSummary / getFlowtooWeekdaySummaryForListUi；mask bit0=Mon … bit6=Sun */
    if (mask == 0)
    {
        return;
    }
    if (mask == 0x7F)
    {
        const char *daily;

        daily = divoom_menu_control_get_disp_str("every day");
        if (daily)
        {
            snprintf(buf, (size_t)len, "%s", daily);
        }
        return;
    }

    idx = 0;
    for (i = 0; i < 7; i++)
    {
        if (mask & (uint8)(1 << i))
        {
            if (idx > 0)
            {
                idx += snprintf(buf + idx, (size_t)(len - idx), " ");
            }
            idx += snprintf(buf + idx, (size_t)(len - idx), "%s", week_abbr[i]);
        }
    }
}

static void divoom_astrootoo_settings_sp_refresh_list_row(Pdivoom_astrootoo_settings_ctrl phandle, int row)
{
    char tbuf[16];
    char mbuf[16];
    char rbuf[64];
    divoom_astrootoo_settings_sp_item_t *it;
    const char *sw;
    const char *icon;
    lv_opa_t row_opa;

    if (row < 0 || row >= DIVOOM_ASTRO_SP_SCHEDULE_COUNT)
    {
        return;
    }
    it = &phandle->sp_items[row];
    divoom_astrootoo_settings_sp_format_time_parts(tbuf, (int)sizeof(tbuf), mbuf, (int)sizeof(mbuf),
        it->hour, it->minute);
    divoom_astrootoo_settings_sp_format_repeat(rbuf, sizeof(rbuf), it->weekday_mask);
    if (phandle->sp_row_time[row])
    {
        lv_label_set_text(phandle->sp_row_time[row], tbuf);
    }
    if (phandle->sp_row_meridiem[row])
    {
        if (mbuf[0] != '\0')
        {
            lv_label_set_text(phandle->sp_row_meridiem[row], mbuf);
            lv_obj_clear_flag(phandle->sp_row_meridiem[row], LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_label_set_text(phandle->sp_row_meridiem[row], "");
            lv_obj_add_flag(phandle->sp_row_meridiem[row], LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (phandle->sp_row_sub[row])
    {
        lv_label_set_text(phandle->sp_row_sub[row], rbuf);
    }
    icon = it->action ? DIVOOM_ASTRO_SP_ICON_OFF : DIVOOM_ASTRO_SP_ICON_ON;
    if (phandle->sp_row_icon[row])
    {
        divoom_menu_control_lv_img_set_src(phandle->sp_row_icon[row], icon);
    }
    sw = it->enabled ? DIVOOM_ASTRO_SET_SWITCH_ON : DIVOOM_ASTRO_SET_SWITCH_OFF;
    if (phandle->sp_row_sw[row])
    {
        divoom_menu_control_lv_img_set_src(phandle->sp_row_sw[row], sw);
    }
    row_opa = it->enabled ? LV_OPA_COVER : LV_OPA_50;
    if (phandle->sp_row_icon[row])
    {
        lv_obj_set_style_opa(phandle->sp_row_icon[row], row_opa, 0);
    }
    if (phandle->sp_row_time[row])
    {
        lv_obj_set_style_text_color(phandle->sp_row_time[row],
            lv_color_hex(it->enabled ? DIVOOM_ASTRO_INK : DIVOOM_ASTRO_INK_MUTED), 0);
    }
    if (phandle->sp_row_meridiem[row])
    {
        lv_obj_set_style_text_color(phandle->sp_row_meridiem[row],
            lv_color_hex(it->enabled ? DIVOOM_ASTRO_INK : DIVOOM_ASTRO_INK_MUTED), 0);
    }
    if (phandle->sp_row_sub[row])
    {
        lv_obj_set_style_text_color(phandle->sp_row_sub[row],
            lv_color_hex(it->enabled ? DIVOOM_ASTRO_INK : DIVOOM_ASTRO_INK_MUTED), 0);
    }
    if (phandle->sp_row_body[row])
    {
        divoom_astrootoo_settings_ex_apply_list_slot(phandle->sp_row_body[row], phandle,
            phandle->sp_mode == DIVOOM_ASTRO_SP_MODE_LIST
            && phandle->sp_list_col == DIVOOM_ASTRO_SP_COL_BODY
            && phandle->sp_list_focus == row);
        lv_obj_set_style_opa(phandle->sp_row_body[row], row_opa, 0);
    }
    if (phandle->sp_row_toggle[row])
    {
        divoom_astrootoo_settings_ex_apply_list_slot(phandle->sp_row_toggle[row], phandle,
            phandle->sp_mode == DIVOOM_ASTRO_SP_MODE_LIST
            && phandle->sp_list_col == DIVOOM_ASTRO_SP_COL_TOGGLE
            && phandle->sp_list_focus == row);
        lv_obj_set_style_opa(phandle->sp_row_toggle[row], row_opa, 0);
    }
    divoom_astrootoo_settings_sp_layout_list_row(phandle, row);
}

static void divoom_astrootoo_settings_sp_refresh_all(Pdivoom_astrootoo_settings_ctrl phandle)
{
    int i;

    for (i = 0; i < DIVOOM_ASTRO_SP_SCHEDULE_COUNT; i++)
    {
        divoom_astrootoo_settings_sp_refresh_list_row(phandle, i);
    }
}

static void divoom_astrootoo_settings_sp_set_obj_hidden(lv_obj_t *obj, int hidden)
{
    if (!obj)
    {
        return;
    }
    if (hidden)
    {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
}

static void divoom_astrootoo_settings_sp_set_header(Pdivoom_astrootoo_settings_ctrl phandle, const char *key)
{
    if (phandle->sp_header_lbl)
    {
        lv_label_set_text(phandle->sp_header_lbl, divoom_menu_control_get_disp_str(key));
        lv_obj_center(phandle->sp_header_lbl);
    }
}

static void divoom_astrootoo_settings_sp_show_mode(Pdivoom_astrootoo_settings_ctrl phandle)
{
    int mode;
    int show_edit_rows;
    int show_pick;
    int show_time;
    int i;
    const char *header_key;

    mode = phandle->sp_mode;
    show_edit_rows = (mode == DIVOOM_ASTRO_SP_MODE_EDIT);
    show_pick = (mode == DIVOOM_ASTRO_SP_MODE_REPEAT_PICK
        || mode == DIVOOM_ASTRO_SP_MODE_ACTION_PICK);
    show_time = (mode == DIVOOM_ASTRO_SP_MODE_TIME_PICK);

    divoom_astrootoo_settings_sp_set_obj_hidden(phandle->sp_list_cont,
        mode != DIVOOM_ASTRO_SP_MODE_LIST);
    divoom_astrootoo_settings_sp_set_obj_hidden(phandle->sp_edit_cont,
        mode == DIVOOM_ASTRO_SP_MODE_LIST);

    for (i = 0; i < DIVOOM_ASTRO_SP_EDIT_FIELDS; i++)
    {
        divoom_astrootoo_settings_sp_set_obj_hidden(phandle->sp_edit_row[i], !show_edit_rows);
    }
    divoom_astrootoo_settings_sp_set_obj_hidden(phandle->sp_pick_cont, !show_pick);
    divoom_astrootoo_settings_sp_set_obj_hidden(phandle->sp_time_cont, !show_time);

    switch (mode)
    {
    case DIVOOM_ASTRO_SP_MODE_REPEAT_PICK:
        header_key = "SpRepeat";
        break;
    case DIVOOM_ASTRO_SP_MODE_ACTION_PICK:
        header_key = "SpPowerAction";
        break;
    case DIVOOM_ASTRO_SP_MODE_TIME_PICK:
        header_key = "SpTime";
        break;
    default:
        header_key = "ScheduledPower";
        break;
    }
    divoom_astrootoo_settings_sp_set_header(phandle, header_key);

    if (phandle->content)
    {
        lv_obj_scroll_to_y(phandle->content, 0, LV_ANIM_OFF);
    }
    if (mode == DIVOOM_ASTRO_SP_MODE_LIST
        && phandle->sp_row_btn[phandle->sp_list_focus])
    {
        lv_obj_scroll_to_view_recursive(phandle->sp_row_btn[phandle->sp_list_focus], LV_ANIM_OFF);
    }
}

static void divoom_astrootoo_settings_sp_refresh_edit(Pdivoom_astrootoo_settings_ctrl phandle)
{
    char tbuf[16];
    char rbuf[64];
    int i;

    divoom_astrootoo_settings_sp_format_time(tbuf, sizeof(tbuf), phandle->sp_draft.hour, phandle->sp_draft.minute);
    divoom_astrootoo_settings_sp_format_repeat(rbuf, sizeof(rbuf), phandle->sp_draft.weekday_mask);
    for (i = 0; i < DIVOOM_ASTRO_SP_EDIT_FIELDS; i++)
    {
        if (!phandle->sp_edit_row[i])
        {
            continue;
        }
        divoom_astrootoo_settings_ex_apply_list_slot(phandle->sp_edit_row[i], phandle,
            phandle->sp_mode == DIVOOM_ASTRO_SP_MODE_EDIT && phandle->sp_edit_field == i);
    }
    if (phandle->sp_edit_val[0])
    {
        lv_label_set_text(phandle->sp_edit_val[0], tbuf);
    }
    if (phandle->sp_edit_val[1])
    {
        lv_label_set_text(phandle->sp_edit_val[1], rbuf);
    }
    if (phandle->sp_edit_val[2])
    {
        lv_label_set_text(phandle->sp_edit_val[2],
            divoom_menu_control_get_disp_str(phandle->sp_draft.action ? "SpPowerOff" : "SpPowerOn"));
        if (phandle->sp_edit_action_icon)
        {
            divoom_menu_control_lv_img_set_src(phandle->sp_edit_action_icon,
                phandle->sp_draft.action ? DIVOOM_ASTRO_SP_ICON_OFF : DIVOOM_ASTRO_SP_ICON_ON);
        }
        divoom_astrootoo_settings_sp_layout_action_value(phandle);
    }
    for (i = 0; i < DIVOOM_ASTRO_SP_EDIT_FIELDS; i++)
    {
        if (phandle->sp_edit_row[i] && phandle->sp_edit_val[i])
        {
            divoom_astrootoo_ui_list_label_refresh_marquee(phandle->sp_edit_row[i],
                phandle->sp_edit_val[i]);
        }
    }
}

static void divoom_astrootoo_settings_sp_open_edit(Pdivoom_astrootoo_settings_ctrl phandle, int index)
{
    phandle->sp_editing_index = index;
    phandle->sp_draft = phandle->sp_items[index];
    phandle->sp_edit_touched = 0;
    phandle->sp_mode = DIVOOM_ASTRO_SP_MODE_EDIT;
    phandle->sp_edit_field = 0;
    divoom_astrootoo_settings_sp_show_mode(phandle);
    divoom_astrootoo_settings_sp_refresh_edit(phandle);
}

static void divoom_astrootoo_settings_sp_commit_edit(Pdivoom_astrootoo_settings_ctrl phandle)
{
    if (phandle->sp_editing_index >= 0 && phandle->sp_editing_index < DIVOOM_ASTRO_SP_SCHEDULE_COUNT)
    {
        if (phandle->sp_edit_touched)
        {
            phandle->sp_draft.enabled = 1;
        }
        phandle->sp_items[phandle->sp_editing_index] = phandle->sp_draft;
        divoom_astrootoo_settings_sp_save_slot(phandle, phandle->sp_editing_index);
        divoom_astrootoo_settings_sp_refresh_list_row(phandle, phandle->sp_editing_index);
    }
    phandle->sp_mode = DIVOOM_ASTRO_SP_MODE_LIST;
    phandle->sp_editing_index = -1;
    divoom_astrootoo_settings_sp_show_mode(phandle);
    divoom_astrootoo_settings_sp_refresh_all(phandle);
}

static void divoom_astrootoo_settings_sp_refresh_action_pick(Pdivoom_astrootoo_settings_ctrl phandle)
{
    int i;

    for (i = 0; i < 2; i++)
    {
        if (!phandle->sp_pick_row[i])
        {
            continue;
        }
        lv_obj_clear_flag(phandle->sp_pick_row[i], LV_OBJ_FLAG_HIDDEN);
        divoom_astrootoo_settings_ex_apply_list_slot(phandle->sp_pick_row[i], phandle,
            phandle->sp_mode == DIVOOM_ASTRO_SP_MODE_ACTION_PICK && phandle->sp_action_focus == i);
        if (phandle->sp_pick_icon[i])
        {
            divoom_menu_control_lv_img_set_src(phandle->sp_pick_icon[i],
                i == 0 ? DIVOOM_ASTRO_SP_ICON_ON : DIVOOM_ASTRO_SP_ICON_OFF);
            lv_obj_clear_flag(phandle->sp_pick_icon[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_align(phandle->sp_pick_icon[i], LV_ALIGN_LEFT_MID, 12, 0);
        }
        if (phandle->sp_pick_lbl[i])
        {
            lv_label_set_text(phandle->sp_pick_lbl[i], divoom_menu_control_get_disp_str(g_sp_action_keys[i]));
            lv_obj_set_style_text_font(phandle->sp_pick_lbl[i], divoom_menu_control_get_font_info(28, phandle->menu_id), 0);
            if (phandle->sp_pick_icon[i]
                && !lv_obj_has_flag(phandle->sp_pick_icon[i], LV_OBJ_FLAG_HIDDEN))
            {
                lv_obj_align_to(phandle->sp_pick_lbl[i], phandle->sp_pick_icon[i],
                    LV_ALIGN_OUT_RIGHT_MID, DIVOOM_ASTRO_SP_ACTION_PICK_ICON_TEXT_GAP, 0);
            }
            else
            {
                lv_obj_align(phandle->sp_pick_lbl[i], LV_ALIGN_LEFT_MID, 12, 0);
            }
            divoom_astrootoo_settings_ex_sp_bind_pick_lbl(phandle->sp_pick_row[i], phandle->sp_pick_lbl[i],
                phandle->sp_pick_icon[i]
                && !lv_obj_has_flag(phandle->sp_pick_icon[i], LV_OBJ_FLAG_HIDDEN));
            divoom_astrootoo_ui_list_label_refresh_marquee(phandle->sp_pick_row[i], phandle->sp_pick_lbl[i]);
        }
        if (phandle->sp_pick_chk[i])
        {
            if (i == (phandle->sp_draft.action ? 1 : 0))
            {
                lv_obj_clear_flag(phandle->sp_pick_chk[i], LV_OBJ_FLAG_HIDDEN);
            }
            else
            {
                lv_obj_add_flag(phandle->sp_pick_chk[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
    if (phandle->sp_pick_row[phandle->sp_action_focus])
    {
        lv_obj_scroll_to_view_recursive(phandle->sp_pick_row[phandle->sp_action_focus], LV_ANIM_OFF);
    }
    for (i = 2; i < DIVOOM_ASTRO_SP_REPEAT_ROWS; i++)
    {
        if (phandle->sp_pick_row[i])
        {
            lv_obj_add_flag(phandle->sp_pick_row[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void divoom_astrootoo_settings_sp_refresh_repeat_pick(Pdivoom_astrootoo_settings_ctrl phandle)
{
    int i;

    for (i = 0; i < DIVOOM_ASTRO_SP_REPEAT_ROWS; i++)
    {
        if (phandle->sp_pick_row[i])
        {
            lv_obj_clear_flag(phandle->sp_pick_row[i], LV_OBJ_FLAG_HIDDEN);
        }
        if (phandle->sp_pick_icon[i])
        {
            lv_obj_add_flag(phandle->sp_pick_icon[i], LV_OBJ_FLAG_HIDDEN);
        }
        if (phandle->sp_pick_lbl[i])
        {
            lv_label_set_text(phandle->sp_pick_lbl[i], divoom_menu_control_get_disp_str(g_sp_repeat_keys[i]));
            lv_obj_align(phandle->sp_pick_lbl[i], LV_ALIGN_LEFT_MID, 12, 0);
            divoom_astrootoo_settings_ex_sp_bind_pick_lbl(phandle->sp_pick_row[i],
                phandle->sp_pick_lbl[i], 0);
        }
    }
    for (i = 0; i < DIVOOM_ASTRO_SP_REPEAT_ROWS; i++)
    {
        if (!phandle->sp_pick_row[i])
        {
            continue;
        }
        divoom_astrootoo_settings_ex_apply_list_slot(phandle->sp_pick_row[i], phandle,
            phandle->sp_mode == DIVOOM_ASTRO_SP_MODE_REPEAT_PICK && phandle->sp_repeat_focus == i);
        if (phandle->sp_pick_chk[i])
        {
            int on;

            if (i == DIVOOM_ASTRO_SP_REPEAT_NEVER)
            {
                on = (phandle->sp_repeat_mask == 0);
            }
            else
            {
                on = (phandle->sp_repeat_mask & (1 << i)) != 0;
            }
            if (on)
            {
                lv_obj_clear_flag(phandle->sp_pick_chk[i], LV_OBJ_FLAG_HIDDEN);
            }
            else
            {
                lv_obj_add_flag(phandle->sp_pick_chk[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
        if (phandle->sp_pick_lbl[i])
        {
            divoom_astrootoo_ui_list_label_refresh_marquee(phandle->sp_pick_row[i],
                phandle->sp_pick_lbl[i]);
        }
    }
    if (phandle->sp_pick_row[phandle->sp_repeat_focus])
    {
        lv_obj_scroll_to_view_recursive(phandle->sp_pick_row[phandle->sp_repeat_focus], LV_ANIM_OFF);
    }
}

static void divoom_astrootoo_settings_sp_refresh_time(Pdivoom_astrootoo_settings_ctrl phandle)
{
    divoom_astrootoo_time_pick_widgets_t widgets;

    widgets.ampm_box = phandle->sp_time_ampm_box;
    widgets.ampm_lbl = phandle->sp_time_ampm_lbl;
    widgets.h_box = phandle->sp_time_h_box;
    widgets.m_box = phandle->sp_time_m_box;
    widgets.h_lbl = phandle->sp_time_h_lbl;
    widgets.m_lbl = phandle->sp_time_m_lbl;
    widgets.h_cap = phandle->sp_time_h_cap;
    widgets.m_cap = phandle->sp_time_m_cap;
    widgets.cancel = phandle->sp_time_cancel;
    widgets.confirm = phandle->sp_time_confirm;
    widgets.use_hour24 = divoom_astrootoo_time_pick_use_hour24();
    divoom_astrootoo_time_pick_refresh_widgets(&widgets, phandle->sp_time_field,
        phandle->sp_time_hour, phandle->sp_time_min, widgets.use_hour24);
}

static void divoom_astrootoo_settings_sp_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_settings_ctrl phandle;
    uint8 key;
    int i;

    phandle = (Pdivoom_astrootoo_settings_ctrl)lv_event_get_user_data(e);
    if (!phandle || !divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }
    key = divoom_astrootoo_ui_key_code_from_event(e);
    if (divoom_astrootoo_ui_key_is_esc(key))
    {
        if (phandle->sp_mode == DIVOOM_ASTRO_SP_MODE_LIST)
        {
            divoom_menu_control_return_pre();
        }
        else if (phandle->sp_mode == DIVOOM_ASTRO_SP_MODE_EDIT)
        {
            divoom_astrootoo_settings_sp_commit_edit(phandle);
        }
        else if (phandle->sp_mode == DIVOOM_ASTRO_SP_MODE_TIME_PICK)
        {
            phandle->sp_mode = DIVOOM_ASTRO_SP_MODE_EDIT;
            divoom_astrootoo_settings_sp_show_mode(phandle);
            divoom_astrootoo_settings_sp_refresh_edit(phandle);
        }
        else
        {
            phandle->sp_mode = DIVOOM_ASTRO_SP_MODE_EDIT;
            divoom_astrootoo_settings_sp_show_mode(phandle);
            divoom_astrootoo_settings_sp_refresh_edit(phandle);
        }
        return;
    }

    if (phandle->sp_mode == DIVOOM_ASTRO_SP_MODE_LIST)
    {
        if (divoom_astrootoo_ui_key_is_up(key) || divoom_astrootoo_ui_key_is_down(key))
        {
            int next;

            next = phandle->sp_list_focus;
            if (divoom_astrootoo_ui_key_is_up(key))
            {
                next = (next - 1 + DIVOOM_ASTRO_SP_SCHEDULE_COUNT) % DIVOOM_ASTRO_SP_SCHEDULE_COUNT;
            }
            else
            {
                next = (next + 1) % DIVOOM_ASTRO_SP_SCHEDULE_COUNT;
            }
            phandle->sp_list_focus = next;
            phandle->sp_list_col = DIVOOM_ASTRO_SP_COL_BODY;
            if (phandle->sp_row_btn[next])
            {
                lv_obj_scroll_to_view_recursive(phandle->sp_row_btn[next], LV_ANIM_OFF);
            }
            divoom_astrootoo_settings_sp_refresh_all(phandle);
            return;
        }
        if (divoom_astrootoo_ui_key_is_left(key))
        {
            phandle->sp_list_col = DIVOOM_ASTRO_SP_COL_BODY;
            divoom_astrootoo_settings_sp_refresh_list_row(phandle, phandle->sp_list_focus);
            return;
        }
        if (divoom_astrootoo_ui_key_is_right(key))
        {
            phandle->sp_list_col = DIVOOM_ASTRO_SP_COL_TOGGLE;
            divoom_astrootoo_settings_sp_refresh_list_row(phandle, phandle->sp_list_focus);
            return;
        }
        if (!divoom_astrootoo_ui_key_is_enter(key))
        {
            return;
        }
        i = phandle->sp_list_focus;
        if (phandle->sp_list_col == DIVOOM_ASTRO_SP_COL_BODY)
        {
            divoom_astrootoo_settings_sp_open_edit(phandle, i);
        }
        else
        {
            phandle->sp_items[i].enabled = phandle->sp_items[i].enabled ? 0 : 1;
            divoom_astrootoo_settings_sp_save_slot(phandle, i);
            divoom_astrootoo_settings_sp_refresh_list_row(phandle, i);
        }
        return;
    }

    if (phandle->sp_mode == DIVOOM_ASTRO_SP_MODE_EDIT)
    {
        if (divoom_astrootoo_ui_key_is_up(key) || divoom_astrootoo_ui_key_is_down(key))
        {
            if (divoom_astrootoo_ui_key_is_up(key))
            {
                phandle->sp_edit_field = (phandle->sp_edit_field + DIVOOM_ASTRO_SP_EDIT_FIELDS - 1) % DIVOOM_ASTRO_SP_EDIT_FIELDS;
            }
            else
            {
                phandle->sp_edit_field = (phandle->sp_edit_field + 1) % DIVOOM_ASTRO_SP_EDIT_FIELDS;
            }
            divoom_astrootoo_settings_sp_refresh_edit(phandle);
            return;
        }
        if (!divoom_astrootoo_ui_key_is_enter(key))
        {
            return;
        }
        if (phandle->sp_edit_field == 0)
        {
            phandle->sp_edit_touched = 1;
            phandle->sp_time_hour = phandle->sp_draft.hour;
            phandle->sp_time_min = phandle->sp_draft.minute;
            phandle->sp_time_field = divoom_astrootoo_time_pick_initial_field(
                divoom_astrootoo_time_pick_use_hour24());
            phandle->sp_mode = DIVOOM_ASTRO_SP_MODE_TIME_PICK;
            divoom_astrootoo_settings_sp_show_mode(phandle);
            divoom_astrootoo_settings_sp_refresh_time(phandle);
            return;
        }
        if (phandle->sp_edit_field == 1)
        {
            phandle->sp_edit_touched = 1;
            phandle->sp_repeat_mask = phandle->sp_draft.weekday_mask;
            phandle->sp_repeat_focus = 0;
            phandle->sp_mode = DIVOOM_ASTRO_SP_MODE_REPEAT_PICK;
            divoom_astrootoo_settings_sp_show_mode(phandle);
            divoom_astrootoo_settings_sp_refresh_repeat_pick(phandle);
            return;
        }
        phandle->sp_edit_touched = 1;
        phandle->sp_action_focus = phandle->sp_draft.action ? 1 : 0;
        phandle->sp_mode = DIVOOM_ASTRO_SP_MODE_ACTION_PICK;
        divoom_astrootoo_settings_sp_show_mode(phandle);
        divoom_astrootoo_settings_sp_refresh_action_pick(phandle);
        return;
    }

    if (phandle->sp_mode == DIVOOM_ASTRO_SP_MODE_TIME_PICK)
    {
        divoom_astrootoo_time_pick_state_t st;
        divoom_astrootoo_time_pick_key_action_t action;

        st.use_hour24 = divoom_astrootoo_time_pick_use_hour24();
        st.field = phandle->sp_time_field;
        st.hour24 = phandle->sp_time_hour;
        st.minute = phandle->sp_time_min;
        action = divoom_astrootoo_time_pick_handle_key(&st, key, 0);
        phandle->sp_time_field = st.field;
        phandle->sp_time_hour = st.hour24;
        phandle->sp_time_min = st.minute;
        if (action == DIVOOM_ASTRO_TP_KEY_CANCEL)
        {
            phandle->sp_mode = DIVOOM_ASTRO_SP_MODE_EDIT;
            divoom_astrootoo_settings_sp_show_mode(phandle);
            divoom_astrootoo_settings_sp_refresh_edit(phandle);
            return;
        }
        if (action == DIVOOM_ASTRO_TP_KEY_CONFIRM)
        {
            phandle->sp_draft.hour = (uint8)phandle->sp_time_hour;
            phandle->sp_draft.minute = (uint8)phandle->sp_time_min;
            phandle->sp_mode = DIVOOM_ASTRO_SP_MODE_EDIT;
            divoom_astrootoo_settings_sp_show_mode(phandle);
            divoom_astrootoo_settings_sp_refresh_edit(phandle);
            return;
        }
        divoom_astrootoo_settings_sp_refresh_time(phandle);
        return;
    }

    if (phandle->sp_mode == DIVOOM_ASTRO_SP_MODE_REPEAT_PICK)
    {
        if (divoom_astrootoo_ui_key_is_up(key) || divoom_astrootoo_ui_key_is_down(key))
        {
            if (divoom_astrootoo_ui_key_is_up(key))
            {
                phandle->sp_repeat_focus = (phandle->sp_repeat_focus + DIVOOM_ASTRO_SP_REPEAT_ROWS - 1) % DIVOOM_ASTRO_SP_REPEAT_ROWS;
            }
            else
            {
                phandle->sp_repeat_focus = (phandle->sp_repeat_focus + 1) % DIVOOM_ASTRO_SP_REPEAT_ROWS;
            }
            divoom_astrootoo_settings_sp_refresh_repeat_pick(phandle);
            return;
        }
        if (!divoom_astrootoo_ui_key_is_enter(key))
        {
            return;
        }
        if (phandle->sp_repeat_focus == DIVOOM_ASTRO_SP_REPEAT_NEVER)
        {
            phandle->sp_repeat_mask = 0;
        }
        else
        {
            phandle->sp_repeat_mask ^= (uint8)(1 << phandle->sp_repeat_focus);
        }
        phandle->sp_draft.weekday_mask = phandle->sp_repeat_mask;
        divoom_astrootoo_settings_sp_refresh_repeat_pick(phandle);
        return;
    }

    if (phandle->sp_mode == DIVOOM_ASTRO_SP_MODE_ACTION_PICK)
    {
        if (divoom_astrootoo_ui_key_is_up(key) || divoom_astrootoo_ui_key_is_down(key))
        {
            phandle->sp_action_focus = phandle->sp_action_focus ? 0 : 1;
            divoom_astrootoo_settings_sp_refresh_action_pick(phandle);
            return;
        }
        if (divoom_astrootoo_ui_key_is_enter(key))
        {
            phandle->sp_draft.action = (uint8)phandle->sp_action_focus;
            phandle->sp_mode = DIVOOM_ASTRO_SP_MODE_EDIT;
            divoom_astrootoo_settings_sp_show_mode(phandle);
            divoom_astrootoo_settings_sp_refresh_edit(phandle);
        }
    }
}

static void divoom_astrootoo_settings_build_scheduled_power(Pdivoom_astrootoo_settings_ctrl phandle)
{
    lv_obj_t *row;
    lv_obj_t *body;
    lv_obj_t *toggle;
    lv_obj_t *lbl;
    lv_obj_t *img;
    lv_coord_t y;
    int i;
    int menu_id;

    menu_id = DIVOOM_ASTRO_SP_MENU_ID;

    divoom_astrootoo_settings_sp_load(phandle);
    phandle->sp_mode = DIVOOM_ASTRO_SP_MODE_LIST;
    phandle->sp_list_focus = 0;
    phandle->sp_list_col = DIVOOM_ASTRO_SP_COL_BODY;
    phandle->sp_editing_index = -1;

    phandle->sp_header = divoom_astrootoo_ui_create_page_header_tab(phandle->root_obj, &phandle->ui,
        "ScheduledPower", DIVOOM_ASTRO_SET_HEADER, phandle->menu_id);
    phandle->sp_header_lbl = lv_obj_get_child(phandle->sp_header,
        (int)lv_obj_get_child_cnt(phandle->sp_header) - 1);
    phandle->content = divoom_astrootoo_ui_create_scroll_content(phandle->root_obj, &phandle->ui);
    lv_obj_set_scrollbar_mode(phandle->content, LV_SCROLLBAR_MODE_OFF);

    phandle->sp_list_cont = lv_obj_create(phandle->content);
    lv_obj_add_style(phandle->sp_list_cont, &phandle->ui.content_style, 0);
    lv_obj_set_size(phandle->sp_list_cont, DIVOOM_SCREEN_WIDTH,
        DIVOOM_ASTRO_SP_SCHEDULE_COUNT * (DIVOOM_ASTRO_ALARM_ROW_H + DIVOOM_ASTRO_ALARM_ROW_GAP) + 8);
    lv_obj_set_pos(phandle->sp_list_cont, 0, DIVOOM_ASTRO_SET_TOP_GAP);
    lv_obj_set_layout(phandle->sp_list_cont, 0);
    lv_obj_set_style_bg_opa(phandle->sp_list_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(phandle->sp_list_cont, 0, 0);
    lv_obj_clear_flag(phandle->sp_list_cont, LV_OBJ_FLAG_SCROLLABLE);

    for (i = 0; i < DIVOOM_ASTRO_SP_SCHEDULE_COUNT; i++)
    {
        y = i * (DIVOOM_ASTRO_ALARM_ROW_H + DIVOOM_ASTRO_ALARM_ROW_GAP);
        row = lv_btn_create(phandle->sp_list_cont);
        lv_obj_add_style(row, &phandle->ui.content_style, 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_size(row, DIVOOM_ASTRO_ALARM_LIST_W, DIVOOM_ASTRO_ALARM_ROW_H);
        lv_obj_set_pos(row, DIVOOM_ASTRO_LIST_PAD_X, y);
        lv_obj_set_style_outline_width(row, 0, LV_STATE_FOCUSED);
        lv_obj_set_style_outline_width(row, 0, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_shadow_width(row, 0, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_layout(row, 0);
        phandle->sp_row_btn[i] = row;

        body = lv_obj_create(row);
        divoom_astrootoo_settings_ex_apply_list_slot(body, phandle, 0);
        lv_obj_set_size(body, DIVOOM_ASTRO_ALARM_BODY_W, DIVOOM_ASTRO_ALARM_ROW_H);
        lv_obj_set_pos(body, 0, 0);
        lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_layout(body, 0);
        phandle->sp_row_body[i] = body;

        phandle->sp_row_icon[i] = lv_img_create(body);
        lv_obj_set_size(phandle->sp_row_icon[i], DIVOOM_ASTRO_SP_ICON_W, 22);
        lv_obj_align(phandle->sp_row_icon[i], LV_ALIGN_LEFT_MID, DIVOOM_ASTRO_SP_ICON_X, 0);

        phandle->sp_row_time[i] = lv_label_create(body);
        lv_obj_set_style_text_font(phandle->sp_row_time[i],
            divoom_menu_control_get_font_info(DIVOOM_ASTRO_ALARM_TIME_FONT_PX, menu_id), 0);
        lv_obj_set_style_text_color(phandle->sp_row_time[i], lv_color_hex(DIVOOM_ASTRO_INK), 0);
        lv_obj_align(phandle->sp_row_time[i], LV_ALIGN_LEFT_MID, DIVOOM_ASTRO_SP_TIME_X, 0);

        phandle->sp_row_meridiem[i] = lv_label_create(body);
        lv_obj_set_style_text_font(phandle->sp_row_meridiem[i],
            divoom_menu_control_get_font_info(DIVOOM_ASTRO_ALARM_MERIDIEM_FONT_PX, menu_id), 0);
        lv_obj_set_style_text_color(phandle->sp_row_meridiem[i], lv_color_hex(DIVOOM_ASTRO_INK), 0);
        lv_obj_add_flag(phandle->sp_row_meridiem[i], LV_OBJ_FLAG_HIDDEN);

        phandle->sp_row_sub[i] = lv_label_create(body);
        lv_obj_set_style_text_font(phandle->sp_row_sub[i],
            divoom_menu_control_get_font_info(DIVOOM_ASTRO_ALARM_REPEAT_FONT_PX, menu_id), 0);
        lv_obj_set_style_text_color(phandle->sp_row_sub[i], lv_color_hex(DIVOOM_ASTRO_INK), 0);
        lv_obj_set_width(phandle->sp_row_sub[i], DIVOOM_ASTRO_SP_REPEAT_W);
        lv_label_set_long_mode(phandle->sp_row_sub[i], LV_LABEL_LONG_CLIP);
        lv_obj_set_style_text_align(phandle->sp_row_sub[i], LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_align(phandle->sp_row_sub[i], LV_ALIGN_LEFT_MID, DIVOOM_ASTRO_SP_TIME_X, 0);

        img = lv_img_create(body);
        divoom_astrootoo_ui_apply_sublevel_chevron_size(img);
        divoom_menu_control_lv_img_set_src(img, DIVOOM_ASTRO_SET_CHEVRON);
        lv_obj_align(img, LV_ALIGN_RIGHT_MID, -14, 0);

        toggle = lv_obj_create(row);
        divoom_astrootoo_settings_ex_apply_list_slot(toggle, phandle, 0);
        lv_obj_set_size(toggle, DIVOOM_ASTRO_ALARM_TOGGLE_W, DIVOOM_ASTRO_ALARM_ROW_H);
        lv_obj_set_pos(toggle, DIVOOM_ASTRO_ALARM_BODY_W + DIVOOM_ASTRO_ALARM_COL_GAP, 0);
        lv_obj_clear_flag(toggle, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_layout(toggle, 0);
        phandle->sp_row_toggle[i] = toggle;
        phandle->sp_row_sw[i] = lv_img_create(toggle);
        lv_obj_set_size(phandle->sp_row_sw[i], 44, 24);
        lv_obj_center(phandle->sp_row_sw[i]);

    }

    phandle->sp_edit_cont = lv_obj_create(phandle->content);
    lv_obj_add_flag(phandle->sp_edit_cont, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_style(phandle->sp_edit_cont, &phandle->ui.content_style, 0);
    lv_obj_set_size(phandle->sp_edit_cont, DIVOOM_SCREEN_WIDTH, 600);
    lv_obj_set_pos(phandle->sp_edit_cont, 0, DIVOOM_ASTRO_SET_TOP_GAP);
    lv_obj_set_style_bg_opa(phandle->sp_edit_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(phandle->sp_edit_cont, 0, 0);
    lv_obj_clear_flag(phandle->sp_edit_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(phandle->sp_edit_cont, 0);
    y = 0;
    for (i = 0; i < DIVOOM_ASTRO_SP_EDIT_FIELDS; i++)
    {
        phandle->sp_edit_row[i] = lv_obj_create(phandle->sp_edit_cont);
        divoom_astrootoo_settings_ex_apply_list_slot(phandle->sp_edit_row[i], phandle, 0);
        lv_obj_set_size(phandle->sp_edit_row[i], DIVOOM_ASTRO_ALARM_LIST_W, DIVOOM_ASTRO_SP_EDIT_ROW_H);
        lv_obj_set_pos(phandle->sp_edit_row[i], DIVOOM_ASTRO_LIST_PAD_X, y);
        lv_obj_clear_flag(phandle->sp_edit_row[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_layout(phandle->sp_edit_row[i], 0);
        phandle->sp_edit_lbl[i] = lv_label_create(phandle->sp_edit_row[i]);
        lv_label_set_text(phandle->sp_edit_lbl[i], divoom_menu_control_get_disp_str(g_sp_edit_keys[i]));
        lv_obj_set_style_text_font(phandle->sp_edit_lbl[i], divoom_menu_control_get_font_info(28, menu_id), 0);
        lv_obj_align(phandle->sp_edit_lbl[i], LV_ALIGN_LEFT_MID, 12, 0);
        phandle->sp_edit_val[i] = lv_label_create(phandle->sp_edit_row[i]);
        lv_obj_set_style_text_font(phandle->sp_edit_val[i], divoom_menu_control_get_font_info(24, menu_id), 0);
        lv_obj_set_style_text_color(phandle->sp_edit_val[i], lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
        lv_obj_align(phandle->sp_edit_val[i], LV_ALIGN_RIGHT_MID, -28, 0);
        if (i == 2)
        {
            phandle->sp_edit_action_icon = lv_img_create(phandle->sp_edit_row[i]);
            lv_obj_set_size(phandle->sp_edit_action_icon, 20, 22);
        }
        img = lv_img_create(phandle->sp_edit_row[i]);
        divoom_astrootoo_ui_apply_sublevel_chevron_size(img);
        divoom_menu_control_lv_img_set_src(img, DIVOOM_ASTRO_SET_CHEVRON);
        lv_obj_align(img, LV_ALIGN_RIGHT_MID, -12, 0);
        divoom_astrootoo_settings_ex_sp_bind_edit_val(phandle, i);
        y += DIVOOM_ASTRO_SP_EDIT_ROW_H + DIVOOM_ASTRO_SP_EDIT_ROW_GAP;
    }

    phandle->sp_pick_cont = lv_obj_create(phandle->sp_edit_cont);
    lv_obj_add_flag(phandle->sp_pick_cont, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_style(phandle->sp_pick_cont, &phandle->ui.content_style, 0);
    lv_obj_set_size(phandle->sp_pick_cont, DIVOOM_SCREEN_WIDTH,
        DIVOOM_ASTRO_SP_REPEAT_ROWS * (DIVOOM_ASTRO_SP_EDIT_ROW_H + DIVOOM_ASTRO_SP_EDIT_ROW_GAP) + 8);
    lv_obj_set_pos(phandle->sp_pick_cont, 0, 0);
    lv_obj_set_style_bg_opa(phandle->sp_pick_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(phandle->sp_pick_cont, 0, 0);
    lv_obj_clear_flag(phandle->sp_pick_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(phandle->sp_pick_cont, 0);
    y = 0;
    for (i = 0; i < DIVOOM_ASTRO_SP_REPEAT_ROWS; i++)
    {
        const char *key;

        key = (phandle->sp_mode == DIVOOM_ASTRO_SP_MODE_ACTION_PICK && i < 2)
            ? g_sp_action_keys[i] : g_sp_repeat_keys[i];
        phandle->sp_pick_row[i] = lv_obj_create(phandle->sp_pick_cont);
        divoom_astrootoo_settings_ex_apply_list_slot(phandle->sp_pick_row[i], phandle, 0);
        lv_obj_set_size(phandle->sp_pick_row[i], DIVOOM_ASTRO_ALARM_LIST_W, DIVOOM_ASTRO_SP_EDIT_ROW_H);
        lv_obj_set_pos(phandle->sp_pick_row[i], DIVOOM_ASTRO_LIST_PAD_X, y);
        lv_obj_clear_flag(phandle->sp_pick_row[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_layout(phandle->sp_pick_row[i], 0);
        phandle->sp_pick_icon[i] = lv_img_create(phandle->sp_pick_row[i]);
        lv_obj_set_size(phandle->sp_pick_icon[i], 20, 22);
        lv_obj_align(phandle->sp_pick_icon[i], LV_ALIGN_LEFT_MID, 12, 0);
        lv_obj_add_flag(phandle->sp_pick_icon[i], LV_OBJ_FLAG_HIDDEN);
        phandle->sp_pick_lbl[i] = lv_label_create(phandle->sp_pick_row[i]);
        lv_label_set_text(phandle->sp_pick_lbl[i], divoom_menu_control_get_disp_str(key));
        lv_obj_set_style_text_font(phandle->sp_pick_lbl[i], divoom_menu_control_get_font_info(28, menu_id), 0);
        lv_obj_align(phandle->sp_pick_lbl[i], LV_ALIGN_LEFT_MID, 12, 0);
        phandle->sp_pick_chk[i] = lv_img_create(phandle->sp_pick_row[i]);
        divoom_menu_control_lv_img_set_src(phandle->sp_pick_chk[i], DIVOOM_ASTRO_SET_CHECK_ICON);
        lv_obj_align(phandle->sp_pick_chk[i], LV_ALIGN_RIGHT_MID, -16, 0);
        lv_obj_add_flag(phandle->sp_pick_chk[i], LV_OBJ_FLAG_HIDDEN);
        divoom_astrootoo_settings_ex_sp_bind_pick_lbl(phandle->sp_pick_row[i],
            phandle->sp_pick_lbl[i], 0);
        y += DIVOOM_ASTRO_SP_EDIT_ROW_H + DIVOOM_ASTRO_SP_EDIT_ROW_GAP;
    }

    phandle->sp_time_cont = lv_obj_create(phandle->sp_edit_cont);
    lv_obj_add_flag(phandle->sp_time_cont, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_style(phandle->sp_time_cont, &phandle->ui.content_style, 0);
    lv_obj_set_size(phandle->sp_time_cont, DIVOOM_SCREEN_WIDTH,
        DIVOOM_ASTRO_CONTENT_H - DIVOOM_ASTRO_SET_TOP_GAP);
    lv_obj_set_pos(phandle->sp_time_cont, 0, 0);
    lv_obj_set_style_bg_opa(phandle->sp_time_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(phandle->sp_time_cont, 0, 0);
    lv_obj_clear_flag(phandle->sp_time_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(phandle->sp_time_cont, 0);

    {
        divoom_astrootoo_time_pick_widgets_t widgets;
        lv_coord_t stack_y;
        lv_coord_t area_h;

        area_h = DIVOOM_ASTRO_CONTENT_H - DIVOOM_ASTRO_SET_TOP_GAP;
        stack_y = divoom_astrootoo_settings_time_pick_stack_y(area_h);
        divoom_astrootoo_settings_create_time_pick_stack(phandle->sp_time_cont, phandle->menu_id,
            &phandle->ui.content_style, stack_y, &widgets);
        phandle->sp_time_ampm_box = widgets.ampm_box;
        phandle->sp_time_ampm_lbl = widgets.ampm_lbl;
        phandle->sp_time_h_box = widgets.h_box;
        phandle->sp_time_m_box = widgets.m_box;
        phandle->sp_time_h_lbl = widgets.h_lbl;
        phandle->sp_time_m_lbl = widgets.m_lbl;
        phandle->sp_time_h_cap = widgets.h_cap;
        phandle->sp_time_m_cap = widgets.m_cap;
        phandle->sp_time_cancel = widgets.cancel;
        phandle->sp_time_confirm = widgets.confirm;
    }

    phandle->sp_key_btn = divoom_astrootoo_ui_create_key_capture_btn(phandle->root_obj, &phandle->ui,
        divoom_astrootoo_settings_sp_key_cb, phandle);
    divoom_astrootoo_settings_sp_show_mode(phandle);
    divoom_astrootoo_settings_sp_refresh_all(phandle);
}

void divoom_astrootoo_settings_ex_build(Pdivoom_astrootoo_settings_ctrl phandle)
{
    if (!phandle)
    {
        return;
    }
    switch (phandle->page)
    {
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_MCP:
    {
        divoom_astrootoo_settings_build_mcp(phandle);
        break;
    }
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_BT_PASSWORD:
    {
        divoom_astrootoo_settings_build_bt_password(phandle);
        break;
    }
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_SCHEDULED_POWER:
    {
        divoom_astrootoo_settings_build_scheduled_power(phandle);
        break;
    }
    default:
    {
        break;
    }
    }
}

void divoom_astrootoo_settings_ex_release(Pdivoom_astrootoo_settings_ctrl phandle)
{
    if (phandle && phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_MCP)
    {
        divoom_astrootoo_settings_mcp_release_qr(phandle);
    }
}

void divoom_astrootoo_settings_ex_group_bind(Pdivoom_astrootoo_settings_ctrl phandle, lv_obj_t *focus_target)
{
    lv_group_t *g;
    lv_obj_t *focus;

    if (!phandle)
    {
        return;
    }
    g = lv_group_get_default();
    lv_group_remove_all_objs(g);
    focus = NULL;
    if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_MCP && phandle->mcp_key_btn)
    {
        lv_group_add_obj(g, phandle->mcp_key_btn);
        focus = phandle->mcp_key_btn;
    }
    else if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_BT_PASSWORD && phandle->bt_pw_key_btn)
    {
        lv_group_add_obj(g, phandle->bt_pw_key_btn);
        focus = phandle->bt_pw_key_btn;
    }
    else if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_SCHEDULED_POWER && phandle->sp_key_btn)
    {
        lv_group_add_obj(g, phandle->sp_key_btn);
        focus = phandle->sp_key_btn;
    }
    if (focus_target && lv_obj_is_valid(focus_target))
    {
        focus = focus_target;
    }
    if (focus)
    {
        lv_group_focus_obj(focus);
    }
}
