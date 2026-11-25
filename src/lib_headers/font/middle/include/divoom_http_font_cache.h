#ifndef _divoom_http_font_cache_h_
#define _divoom_http_font_cache_h_
void *divoom_http_font_cache_create(void);
void divoom_http_font_cache_add_font(void * handle, uint8 font_id, uint16 *unicode_list, int num);
void divoom_http_font_cache_get_font_data(void *handle, Pdivoom_image_word_font_info pword_info);
void divoom_http_font_cache_destroy(void * handle);

#endif


