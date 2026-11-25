#ifndef _divoom_audio_policy_h_
#define _divoom_audio_policy_h_

#include "divoom_build_config.h"
#include "divoom_audio_session.h"

/* 助眠 DIY / mpg 循环播放占用 CH_SLEEP 时由 sleep_aid 同步状态 */
void divoom_audio_policy_set_sleep_playback_active(int on);

int divoom_audio_policy_is_sleep_playback_active(void);

void divoom_audio_policy_set_voice_clip_recording(int on);
int divoom_audio_policy_is_voice_clip_recording(void);

#define DIVOOM_AUDIO_POLICY_TONE_DUCK_SLEEP_PCT  25u

int divoom_audio_policy_tone_play_allowed(void);

int divoom_audio_policy_keep_jl_after_tone(void);

void divoom_audio_policy_amp_tip_hold(int jl_level_0_100);
void divoom_audio_policy_amp_tip_release(void);

/* mix_pcm 总音量变化后刷新 JL IISIN EQ（0x71）；tip 占用时仍以 tip 为准 */
void divoom_audio_policy_iisin_eq_on_mix_vol(void);
void divoom_audio_policy_amp_prepare_i2s_clock(void);
void divoom_audio_policy_amp_force_mute(void);
void divoom_audio_policy_amp_restore(void);

void divoom_audio_policy_on_tone_begin(unsigned int sleep_duck_pct);
void divoom_audio_policy_on_tone_end(void);

/**
 * tip/sleep/record 占用期间压 JL 音乐总开关（DM_BTCORE_MEDIA_VOLUME 0↔16）。
 * tip/sleep 播法不变；勿动 MUSIC_VOLUME。tag 幂等（tone/sleep/record 位闩）。
 */
void divoom_audio_policy_media_duck_hold(const char *tag);
void divoom_audio_policy_media_duck_release(const char *tag);

#if DIVOOM_SHOW_TF_NAND
/* 提示音/助眠/录音 hold 期间本地 TF MUSIC 是否已 mute（sink 复用 ring 时勿把 duck 0 当未初始化） */
int divoom_audio_policy_show_music_muted(void);
#endif

/** Phase0 P5：duck / 录音 / 助眠 policy 快照（供 audio_diag） */
void divoom_audio_policy_diag_print(void);

#ifdef CONFIG_DIVOOM_MPP_NET_AUDIO
void divoom_audio_policy_network_active(int on);
int divoom_audio_policy_network_muted(void);
/* 电台音量 0–100。会话占用时按音量档 0–16 下发 IISIN EQ；响度仍走数字 PCM。 */
void divoom_audio_policy_network_set_volume(unsigned int vol_0_100);
#endif

#endif
