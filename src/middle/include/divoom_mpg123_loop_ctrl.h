#ifndef _divoom_mpg123_loop_ctrl_h_
#define _divoom_mpg123_loop_ctrl_h_

/** 与 divoom_snd_pcm / mix_pcm 一致：16kHz mono S16LE */
#define DIVOOM_MPG123_LOOP_OUT_RATE 16000

void *divoom_mpg123_loop_open(const char *mp3_path, int vol_percent);

/** 读解码 PCM；文件末尾无缝回到开头。返回实际字节数。 */
int divoom_mpg123_loop_read(void *handle, uint8 *buf, int size);

void divoom_mpg123_loop_set_vol(void *handle, int vol_percent);

void divoom_mpg123_loop_close(void *handle);

#endif
