#ifndef __divoom_light_musical_instruments_h_
#define __divoom_light_musical_instruments_h_

enum{
	DIVOOM_LIGHT_DJ_XYLOPHONE_MODE = 1,		//木琴
	DIVOOM_LIGHT_DJ_DRUM_MODE = 2,			//鼓
	DIVOOM_LIGHT_DJ_PIANO_MODE = 3,			//钢琴
	DIVOOM_LIGHT_DJ_BELL_MODE = 4,			//铃铛
	DIVOOM_LIGHT_DJ_STRINGED_MODE = 5,		//弦乐器
	DIVOOM_LIGHT_DJ_METRONOME_MODE = 6,		//节拍器
	DIVOOM_LIGHT_DJ_PIPE_MODE = 7,			//管乐器
    DIVOOM_LIHGT_DJ_MODE_END,

};

void divoom_light_musical_instruments_init(uint8 *data);
void divoom_light_musical_instruments_update(uint8 *data);
void divoom_light_musical_instruments_close(void);
void divoom_light_musical_instruments_play(uint8 flag);
uint8 divoom_light_musical_instruments_get_cur_mode(void);

#endif

