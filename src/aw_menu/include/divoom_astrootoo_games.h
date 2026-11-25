#ifndef _divoom_astrootoo_games_h_
#define _divoom_astrootoo_games_h_

void *divoom_astrootoo_games_init(void *user_data, int menu_id);
void divoom_astrootoo_games_disp(void *handle, void *active_obj);
void divoom_astrootoo_games_clean(void *handle);
void divoom_astrootoo_games_pause(void *handle);

int divoom_astrootoo_games_whitebox_get_catalog_count(void);
int divoom_astrootoo_games_whitebox_get_focus_row(void);
int divoom_astrootoo_games_whitebox_get_scroll_y(void);
int divoom_astrootoo_games_whitebox_focus_is_visible(void);
int divoom_astrootoo_games_whitebox_get_running(void);
int divoom_astrootoo_games_whitebox_get_page(void);

#endif
