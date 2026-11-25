#ifndef _divoom_astrootoo_watch_h_
#define _divoom_astrootoo_watch_h_

void *divoom_astrootoo_watch_init(void *user_data, int menu_id);
void divoom_astrootoo_watch_disp(void *handle, void *active_obj);
void divoom_astrootoo_watch_clean(void *handle);
void divoom_astrootoo_watch_pause(void *handle);

#endif
