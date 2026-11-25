#ifndef _divoom_calendar_today_watch_layout_h_
#define _divoom_calendar_today_watch_layout_h_

#include <stdio.h>
#include <string.h>

/* Today 全表盘：5 行时间(0-4) + 5 行标题(5-9) + footer(10) */
#define DIVOOM_CALENDAR_TODAY_WATCH_MAX_ROWS        5
#define DIVOOM_CALENDAR_TODAY_WATCH_DATA_NUM_FULL   11
#define DIVOOM_CALENDAR_TODAY_ACTIVITIES_INDEX      11
#define DIVOOM_CALENDAR_TODAY_ACTIVITIES_DEFAULT    "00 EVENTS TODAY"

static inline void divoom_calendar_format_today_activities(char *buf, int count)
{
    if (buf == NULL)
        return;
    if (count <= 0)
        strcpy(buf, DIVOOM_CALENDAR_TODAY_ACTIVITIES_DEFAULT);
    else
        sprintf(buf, "%02d EVENTS TODAY", count);
}

typedef struct _divoom_calendar_today_watch_layout
{
    int max_rows;
    int data_num;
    int has_footer;
} divoom_calendar_today_watch_layout;

divoom_calendar_today_watch_layout divoom_calendar_today_watch_get_layout(int clock_id);

int divoom_calendar_today_watch_is_event_watch(int clock_id);

#endif
