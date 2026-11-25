#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_lvgl_mcp_tool.h"

#define DIVOOM_MCP_TOOL_SCROLL_STEP 80

typedef struct _divoom_lvgl_mcp_tool_ctrl
{
	int menu_id;
	int lang;
	lv_style_t normal_style;
	void *main_obj;
	void *scroll_obj;
	void *scroll_btn;
	void *title_label;
	void *faq_label;
}divoom_lvgl_mcp_tool_ctrl, *Pdivoom_lvgl_mcp_tool_ctrl;

static void divoom_lvgl_mcp_tool_create_qr_panel(void *parent, int x, int y, const char *img_src)
{
	lv_obj_t *frame;

	frame = lv_obj_create(parent);
	lv_obj_set_pos(frame, x, y);
	lv_obj_set_size(frame, 141, 141);
	lv_obj_set_style_bg_color(frame, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_style_border_width(frame, 0, 0);
	lv_obj_set_style_pad_all(frame, 0, 0);
	lv_obj_set_style_radius(frame, 0, 0);
	lv_obj_clear_flag(frame, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t *img = lv_img_create(frame);
	lv_obj_set_size(img, 141, 141);
	lv_obj_center(img);
	divoom_menu_control_lv_img_set_src(img, img_src);
}

static void divoom_lvgl_mcp_tool_menu_event_cb(lv_event_t * e)
{
	Pdivoom_lvgl_mcp_tool_ctrl phandle = (Pdivoom_lvgl_mcp_tool_ctrl)lv_event_get_user_data(e);
	lv_event_code_t code = lv_event_get_code(e);

	if(code != LV_EVENT_KEY)
	{
		return;
	}
    divoom_menu_control_return_pre();
}

void *divoom_lvgl_mcp_tool_init(void *user_data, int menu_id)
{
	Pdivoom_lvgl_mcp_tool_ctrl phandle;
	lv_obj_t *label_obj;
	lv_obj_t *scroll_obj;
	lv_obj_t *content_obj;
	lv_obj_t *btn_obj;
	Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();

	phandle = divoom_system_malloc(sizeof(divoom_lvgl_mcp_tool_ctrl));
	phandle->menu_id = menu_id;
	phandle->lang = psys_config->lang;

	divoom_menu_control_init_lable_style(&phandle->normal_style);
	phandle->main_obj = lv_obj_create(NULL);
	lv_obj_set_style_bg_color(phandle->main_obj, lv_color_hex(0xE4E4E4), LV_PART_MAIN);
	lv_obj_set_pos(phandle->main_obj, 0, 0);
	lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_add_style(phandle->main_obj, &phandle->normal_style, 0);
	lv_obj_set_scrollbar_mode(phandle->main_obj, LV_SCROLLBAR_MODE_OFF);
	lv_obj_clear_flag(phandle->main_obj, LV_OBJ_FLAG_SCROLLABLE);

	label_obj = lv_label_create(phandle->main_obj);
	phandle->title_label = label_obj;
	lv_obj_set_pos(label_obj, 0, 50);
	lv_obj_set_size(label_obj, DIVOOM_SCREEN_WIDTH, 78);
	lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("McpToolTitle"));
	lv_obj_set_style_text_color(label_obj, lv_color_hex(0x000000), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);

	scroll_obj = lv_obj_create(phandle->main_obj);
	phandle->scroll_obj = scroll_obj;
	lv_obj_set_pos(scroll_obj, 0, 128);
	lv_obj_set_size(scroll_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH - 128);
	lv_obj_set_style_bg_color(scroll_obj, lv_color_hex(0xE4E4E4), 0);
	lv_obj_set_style_border_width(scroll_obj, 0, 0);
	lv_obj_set_style_pad_all(scroll_obj, 0, 0);
	lv_obj_set_scrollbar_mode(scroll_obj, LV_SCROLLBAR_MODE_OFF);
	lv_obj_set_scroll_dir(scroll_obj, LV_DIR_VER);

	content_obj = lv_obj_create(scroll_obj);
	lv_obj_set_size(content_obj, DIVOOM_SCREEN_WIDTH, 1240);
	lv_obj_set_pos(content_obj, 0, 0);
	lv_obj_set_style_bg_color(content_obj, lv_color_hex(0xE4E4E4), 0);
	lv_obj_set_style_border_width(content_obj, 0, 0);
	lv_obj_set_style_pad_all(content_obj, 0, 0);
	lv_obj_clear_flag(content_obj, LV_OBJ_FLAG_SCROLLABLE);

	label_obj = lv_label_create(content_obj);
	lv_obj_set_pos(label_obj, 32, 32);
	lv_obj_set_size(label_obj, 352, 48);
	lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("McpToolDownloadLink"));
	lv_obj_set_style_text_color(label_obj, lv_color_hex(0x000000), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);

	label_obj = lv_label_create(content_obj);
	lv_obj_set_pos(label_obj, 416, 40);
	lv_obj_set_size(label_obj, 352, 32);
	lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("McpMcpServices"));
	lv_obj_set_style_text_color(label_obj, lv_color_hex(0x000000), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);

	divoom_lvgl_mcp_tool_create_qr_panel(content_obj, 141, 89, "26/26_tool.png");
	divoom_lvgl_mcp_tool_create_qr_panel(content_obj, 517, 89, "26/26_mcp.png");

	label_obj = lv_label_create(content_obj);
	lv_obj_set_pos(label_obj, 32, 230);
	lv_obj_set_size(label_obj, 352, 48);
	lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("McpScanToViewLink"));
	lv_obj_set_style_text_color(label_obj, lv_color_hex(0x000000), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(20, phandle->menu_id), 0);

	label_obj = lv_label_create(content_obj);
	lv_obj_set_pos(label_obj, 416, 230);
	lv_obj_set_size(label_obj, 352, 48);
	lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("McpScanToViewLink"));
	lv_obj_set_style_text_color(label_obj, lv_color_hex(0x000000), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(20, phandle->menu_id), 0);

	label_obj = lv_label_create(content_obj);
	lv_obj_set_pos(label_obj, 44, 278);
	lv_obj_set_size(label_obj, 328, 72);
	lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("McpEditorUrl"));
	lv_label_set_long_mode(label_obj, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_color(label_obj, lv_color_hex(0x128CE4), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(18, phandle->menu_id), 0);

	label_obj = lv_label_create(content_obj);
	lv_obj_set_pos(label_obj, 428, 278);
	lv_obj_set_size(label_obj, 328, 72);
	lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("McpServicesUrl"));
	lv_label_set_long_mode(label_obj, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_color(label_obj, lv_color_hex(0x128CE4), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(18, phandle->menu_id), 0);

	label_obj = lv_label_create(content_obj);
	phandle->faq_label = label_obj;
	lv_obj_set_pos(label_obj, 48, 370);
	lv_obj_set_size(label_obj, 705, 820);
	lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("McpToolFaq"));
	lv_label_set_long_mode(label_obj, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_color(label_obj, lv_color_hex(0x000000), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(20, phandle->menu_id), 0);

	btn_obj = lv_btn_create(phandle->main_obj);
	phandle->scroll_btn = btn_obj;
	lv_obj_set_size(btn_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH - 128);
	lv_obj_set_pos(btn_obj, 0, 128);
	lv_obj_set_style_bg_opa(btn_obj, LV_OPA_0, 0);
	lv_obj_set_style_border_width(btn_obj, 0, 0);
	lv_obj_set_style_shadow_width(btn_obj, 0, 0);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_mcp_tool_menu_event_cb, LV_EVENT_KEY, phandle);

	return phandle;
}

void divoom_lvgl_mcp_tool_disp(void *handle, void *active_obj)
{
	Pdivoom_lvgl_mcp_tool_ctrl phandle = (Pdivoom_lvgl_mcp_tool_ctrl)handle;
	Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();

	if(psys_config->lang != phandle->lang)
	{
		phandle->lang = psys_config->lang;
		lv_label_set_text(phandle->title_label, divoom_menu_control_get_disp_str("McpToolTitle"));
		lv_obj_set_style_text_font(phandle->title_label, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
		lv_label_set_text(phandle->faq_label, divoom_menu_control_get_disp_str("McpToolFaq"));
		lv_obj_set_style_text_font(phandle->faq_label, divoom_menu_control_get_font_info(20, phandle->menu_id), 0);
	}

	lv_group_focus_obj(phandle->scroll_btn);
	lv_scr_load(phandle->main_obj);
}

void divoom_lvgl_mcp_tool_exit(void *handle)
{
	Pdivoom_lvgl_mcp_tool_ctrl phandle = (Pdivoom_lvgl_mcp_tool_ctrl)handle;

	lv_obj_del(phandle->main_obj);
	lv_style_reset(&phandle->normal_style);
	divoom_system_free(phandle);
}
