#ifndef __divoom_dj_xylophone_h
#define __divoom_dj_xylophone_h

enum{
    DIVOOM_LIGHT_DJ_XYLOPHONE_MODE1,
    DIVOOM_LIGHT_DJ_XYLOPHONE_MODE2,
    DIVOOM_LIGHT_DJ_XYLOPHONE_MODE3,
    DIVOOM_LIGHT_DJ_XYLOPHONE_MODE4,
    DIVOOM_LIGHT_DJ_XYLOPHONE_MODE5,
    DIVOOM_LIGHT_DJ_XYLOPHONE_MODE6,
    DIVOOM_LIGHT_DJ_XYLOPHONE_MODE7,
    DIVOOM_LIGHT_DJ_XYLOPHONE_MODE8,
    DIVOOM_LIGHT_DJ_XYLOPHONE_MODE_END
};

void divoom_light_dj_xylophone_init(uint8 *data);
void divoom_light_dj_xylophone_close(void);
void divoom_light_dj_xylophone_play(uint8 flag);
void divoom_light_dj_xylophone_update(uint8 mode);

#endif

