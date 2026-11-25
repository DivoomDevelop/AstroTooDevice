#ifndef _divoom_weather_json_h_
#define _divoom_weather_json_h_

typedef struct _divoom_weather_json_setting_info
{
    int hourly;
    int specific;
    int time;
}divoom_weather_json_setting_info, *Pdivoom_weather_json_setting_info;

typedef struct _divoom_weather_json_info
{
    int8 weather[20];
    int temp;
    int pm25;
}divoom_weather_json_info, *Pdivoom_weather_json_info;


typedef struct _divoom_weather_position_info
{
    char lat[11];
    char lot[11];
}divoom_weather_position_info, *Pdivoom_weather_position_info;


void divoom_weather_json_set_setting_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_weather_json_set_setting_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_weather_json_get_setting_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_weather_json_get_setting_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_weather_json_set_current_weather_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_weather_json_set_future_weather_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_weather_json_set_current_weather_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_weather_json_set_future_weather_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

#endif

