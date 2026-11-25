#ifndef _divoom_music_fft_h_
#define _divoom_music_fft_h_

/**
 * @Author      coronaa
 * @DateTime    2018-10-17
 * @Input       [input]
 * @Output      [output]
 * @Description [更新播放状态]
 * @param       flag          [description]
 */
void divoom_music_fft_update_music_play_status(bool flag);
/**
 * @Author      coronaa
 * @DateTime    2018-10-17
 * @Input       [input]
 * @Output      [output]
 * @Description [更新显示与否的标志，有显示的地方才进行FFT]
 * @param       flag          [description]
 */
void divoom_music_fft_update_onoff_status(bool flag);
/**
 * @Author      coronaa
 * @DateTime    2018-10-17
 * @Input       [input]
 * @Output      [output]
 * @Description [开始进行FFT运算]
 * @param       func          [description]
 */
void divoom_music_fft_start(void * func);
/**
 * @Author      coronaa
 * @DateTime    2018-10-17
 * @Input       [input]
 * @Output      [output]
 * @Description [结束FFT计算]
 */
void divoom_music_fft_stop(void);

void divoom_music_fft_update_mic_data(uint8 *data, uint16 len);
void divoom_music_fft_main_loop(void *param);
void divoom_music_fft_update_music_data(void *data, int len);
void divoom_music_fft_init(void);
void divoom_music_fft_exit(void);
#endif

