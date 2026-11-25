#ifndef _divoom_lvgl_mcp_tool_h_
#define _divoom_lvgl_mcp_tool_h_

void *divoom_lvgl_mcp_tool_init(void *user_data, int menu_id);

void divoom_lvgl_mcp_tool_disp(void *handle, void *active_obj);

void divoom_lvgl_mcp_tool_exit(void *handle);

#endif
