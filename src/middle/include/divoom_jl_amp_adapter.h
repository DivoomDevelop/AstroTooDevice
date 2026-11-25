#ifndef _divoom_jl_amp_adapter_h_
#define _divoom_jl_amp_adapter_h_

/* JL TONE_VOLUME UART 适配层（TimesVibe）；policy 经此 prime/hold，不直接 include divoom_main */

void divoom_jl_amp_adapter_prime_if_needed(void);
void divoom_jl_amp_adapter_on_session_active(void);
void divoom_jl_amp_adapter_on_session_idle(int keep_open_for_movie);

/*
 * JL 音乐总开关（DM_BTCORE_MEDIA_VOLUME 0~16）与 tip amp prime。
 * 真机实现；模拟器为空 stub。勿再套 SPEAKER/VIBE 产品宏。
 */
#define DIVOOM_JL_MEDIA_VOL_MAX  16

void divoom_jl_media_volume_set(int level_0_16);
void divoom_jl_media_volume_ramp_to(int level_0_16);
int divoom_jl_media_volume_get_current(void);

/*
 * JL 媒体 Vol_Media（0x6C）+ 通话 VolCallGate（0x6E）mute 同步下发。
 * mute：1=静音，0=开声。
 */
void divoom_jl_audio_gates_mute_set(int mute);

/** 开机门控结束：解 JL 媒体/通话 mute，每 boot 一次 */
void divoom_jl_release_boot_audio_gates_once(void);

#endif
