#ifndef _divoom_notify_json_h_
#define _divoom_notify_json_h_

enum
{
    DIVOOM_TIME_PLAN_STATUS_INFO_ADD,
    DIVOOM_TIME_PLAN_STATUS_INFO_DEL,
    DIVOOM_TIME_PLAN_STATUS_INFO_EDIT,
};

enum
{
    DIVOOM_NOTIFY_JSON_EVENT_SET_ALARM, //"SetAlarm"
    DIVOOM_NOTIFY_JSON_EVENT_DEL_ALARM, //"DelAlarm"
    DIVOOM_NOTIFY_JSON_EVENT_SET_MEMORIAL, //"SetMemorial"
    DIVOOM_NOTIFY_JSON_EVENT_DEL_MEMORIAL, //"DelMemorial"
    DIVOOM_NOTIFY_JSON_EVENT_VOICE, //"VoiceMessage"
    DIVOOM_NOTIFY_JSON_EVENT_TIME_PLAN, //"TimeManager"
    DIVOOM_NOTIFY_JSON_EVENT_TIME_PLAN_MOD, //"TimePlanItem"
    DIVOOM_NOTIFY_JSON_EVENT_SET_SLEEP, //"SleepMode"
    DIVOOM_NOTIFY_JSON_EVENT_GET_HOT_FILE,		//get hot file
    DIVOOM_NOTIFY_JSON_EVENT_END, //
};

typedef struct _divoom_notify_json_event_data
{
    int event_type;
    int other;
    int parm1;
    int parm2;
    int parm3;
    int parm4;
}divoom_notify_json_event_data, *Pdivoom_notify_json_event_data;


void divoom_notify_json_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_notify_json_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

#endif

