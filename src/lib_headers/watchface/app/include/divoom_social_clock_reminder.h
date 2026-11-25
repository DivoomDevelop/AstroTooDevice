#ifndef _divoom_social_clock_reminder_
#define _divoom_social_clock_reminder_

typedef struct _divoom_social_reminder_info
{
	int reminder_id;	//提醒id
	int start_utc_time;
	int end_utc_time;
	char msg[2][64];
}divoom_social_reminder_info, *Pdivoom_social_reminder_info;

void divoom_social_clock_reminder_set_base_data(Pdivoom_clock_id_info clock_info, int value);

int divoom_social_clock_reminder_get_base_data(Pdivoom_clock_id_info clock_info);

void divoom_social_clock_reminder_add(Pdivoom_clock_id_info clock_info, int up_down_flag, int value);

void divoom_social_clock_reminder_init(void);

#endif
