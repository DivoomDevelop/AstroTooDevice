#ifndef __divoom_mix_pcm_ctrl_h_
#define __divoom_mix_pcm_ctrl_h_

void divoom_mix_pcm_ctrl_init(void);

/** 单 MP3 文件 libmpg123 无缝循环（16k/mono/S16LE），供助眠长时播放。 */
void divoom_mix_pcm_ctrl_init_mpg123(const char *mp3_path, int vol_percent);

/** mpg123 已在播时仅换源，不关 snd_pcm / 不拉低 JL 音量。成功返回 1。 */
int divoom_mix_pcm_ctrl_switch_mpg123(const char *mp3_path, int vol_percent);

/** DIY 白噪音混音 → mpg123：保留 snd_pcm，仅换解码源。成功返回 1。 */
int divoom_mix_pcm_ctrl_switch_diy_to_mpg123(const char *mp3_path, int vol_percent);

/** mpg123 → DIY 白噪音混音：同上，反向切换。成功返回 1。 */
int divoom_mix_pcm_ctrl_switch_mpg123_to_diy(void);

void divoom_mix_pcm_ctrl_close(void);

void divoom_mix_pcm_ctrl_update_volume(int *para);

int divoom_mix_pcm_ctrl_get_unmute_flag(void);
int divoom_mix_pcm_get_vol(void);
void divoom_mix_pcm_set_vol(int value);

void divoom_mix_pcm_ctrl_set_unmute_flag(int flag);

int divoom_mix_pcm_get_play_stat(void);

void divoom_mix_pcm_player_pp(void);
int divoom_mix_pcm_ring_get(uint8 *data,uint32 size);
int divoom_mix_pcm_ring_data_size(void);

#endif

