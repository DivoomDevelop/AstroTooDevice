#ifndef _divoom_cup2022_h_
#define _divoom_cup2022_h_
uint8 divoom_cup2022_get_data(char *url_info, Pdivoom_clock_control_disp_data pdisp_data, Pdivoom_http_client_url_head_info phead_info, int head_num, void *pdivoom_clock_control_handle);
uint8 divoom_cup2022_get_score(Pdivoom_clock_control_disp_data pdisp_data, Pdivoom_http_client_url_head_info phead_info, int head_num, void *pdivoom_clock_control_handle);

#endif

