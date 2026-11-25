#ifndef __divoom_light_mixer_h
#define __divoom_light_mixer_h

enum{
   DIVOOM_LIGHT_MIXER_IN_OUT,//进入退出MIXER命令
   DIVOOM_LIGHT_MIXER_DRUM,
   DIVOOM_LIGHT_MIXER_PERCISSION,
   DIVOOM_LIGHT_MIXER_BASS,
   DIVOOM_LIGHT_MIXER_MELODIC,
   DIVOOM_LIGHT_MIXER_LOOP,
   DIVOOM_LIGHT_MIXER_LEAD,
   DIVOOM_LIGHT_MIXER_SYNTH,
   DIVOOM_LIGHT_MIXER_VACAL,
   DIVOOM_LIGHT_MIXER_END = 0xff,
};

void divoom_light_mixer_init(uint8 *data);
void divoom_light_mixer_close(void);
void divoom_light_mixer_play(uint8 flag);
void divoom_light_mixer_update_mode(uint8 *data);

#endif


