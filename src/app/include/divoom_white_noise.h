#ifndef _divoom_white_noise_h_
#define _divoom_white_noise_h_
typedef struct _divoom_white_noise_info
{
    int on_off;
    int power_off;
    int time;
	int idle_time;
	int fresh_flag;
    int volume[8];
}divoom_white_noise_info, *Pdivoom_white_noise_info;

enum{
	DIVOOM_WHITE_NOISE_SET,
	DIVOOM_WHITE_NOISE_GET,
};

enum
{
    DIVOOM_WHITE_NOISE_ACTIVE,
};

void *divoom_white_noise_get_base_info(void);
void divoom_white_noise_init(void);
void divoom_white_noise_start(void);
void divoom_white_noise_stop(void);
void divoom_white_noise_exit(void);

#endif
