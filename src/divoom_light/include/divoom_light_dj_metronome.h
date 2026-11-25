#ifndef __divoom_dj_metronome_h
#define __divoom_dj_metronome_h

enum{
    DIVOOM_LIGHT_DJ_METRONOME_MODE1,
    DIVOOM_LIGHT_DJ_METRONOME_MODE2,
    DIVOOM_LIGHT_DJ_METRONOME_MODE3,
    DIVOOM_LIGHT_DJ_METRONOME_MODE4,
    DIVOOM_LIGHT_DJ_METRONOME_MODE5,
    DIVOOM_LIGHT_DJ_METRONOME_MODE_END
};

void divoom_light_dj_metronome_init(uint8 *data);
void divoom_light_dj_metronome_close(void);
void divoom_light_dj_metronome_play(uint8 flag);
void divoom_light_dj_metronome_update(uint8 mode);

#endif

