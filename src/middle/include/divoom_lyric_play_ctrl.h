#ifndef _divoom_lyric_play_ctrl_h_
#define _divoom_lyric_play_ctrl_h_

void divoom_lyric_play_init(void);


int divoom_lyric_play_is_favorite(int index);


int divoom_lyric_play_get_favorite_at(int list_index);


int divoom_lyric_play_get_favorite_count(void);


void divoom_lyric_play_save_cfg(void);


void divoom_lyric_play_load_cfg(void);


int divoom_lyric_play_get_current_bg(void);
void divoom_lyric_play_set_current_bg(int bg_index);


int divoom_lyric_play_get_current_lyric_effect(void);
void divoom_lyric_play_set_current_lyric_effect(int lyric_effect);

#endif
