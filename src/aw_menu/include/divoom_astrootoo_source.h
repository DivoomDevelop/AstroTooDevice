#ifndef _divoom_astrootoo_source_h_
#define _divoom_astrootoo_source_h_

void *divoom_astrootoo_source_init(void *user_data, int menu_id);
void divoom_astrootoo_source_disp(void *handle, void *active_obj);
void divoom_astrootoo_source_clean(void *handle);
void divoom_astrootoo_source_pause(void *handle);

/** M 键切音源后：若当前就在音源菜单，投递到 LVGL 再切播放页。 */
void divoom_astrootoo_source_goto_music_mode(uint8 mode);

int divoom_astrootoo_source_whitebox_get_page(void);

/* Radio page key ownership and read-only verification. */
int divoom_astrootoo_source_net_radio_is_open(void);
int divoom_astrootoo_source_whitebox_radio_probe(const char *name, long *out);

#endif
