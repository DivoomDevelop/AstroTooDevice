#ifndef _divoom_sleep_aid_json_h_
#define _divoom_sleep_aid_json_h_

#include "divoom_type.h"
#include "cJSON.h"

/** cmd_data for AidSleep/GetAllList request packing (MQTT/HTTP). */
typedef struct _divoom_sleep_aid_get_all_list_req
{
    int type;
    int update_time;
} divoom_sleep_aid_get_all_list_req, *Pdivoom_sleep_aid_get_all_list_req;

/** One row of AidSleep/GetAllList response after JSON unpack only (`language_json` = Print(Language), malloc'd or NULL). */
typedef struct _divoom_sleep_aid_list_row_parsed
{
    uint8 add_flag;
    uint32 voice_id;
    int update_time;
    int video_type;
    char file_id[56];
    char *language_json;
} divoom_sleep_aid_list_row_parsed;

/** cmd_data from AidSleep/GetAllList response_unpack; freed by divoom_sleep_aid_get_all_list_response_unpack_free_handle after handling. */
typedef struct _divoom_sleep_aid_get_all_list_resp
{
    int list_type;
    int row_num;
    divoom_sleep_aid_list_row_parsed *rows;
} divoom_sleep_aid_get_all_list_resp, *Pdivoom_sleep_aid_get_all_list_resp;

/** cmd_data for AidSleep/Play after JSON unpack. */
typedef struct _divoom_sleep_aid_play_req
{
	int type;
	uint32 sleep_id;
} divoom_sleep_aid_play_req, *Pdivoom_sleep_aid_play_req;

/** cmd_data for AidSleep/Add after JSON unpack (`language_json` malloc'd or NULL). */
typedef struct _divoom_sleep_aid_add_req
{
	int type;
	int video_type;
	uint32 sleep_id;
	char file_id[56];
	char *language_json;
} divoom_sleep_aid_add_req, *Pdivoom_sleep_aid_add_req;

void divoom_sleep_aid_get_all_list_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_sleep_aid_get_all_list_response_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_sleep_aid_play_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_sleep_aid_add_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_sleep_aid_handle_play_req(void *para);

void divoom_sleep_aid_handle_add_req(void *para);

void divoom_sleep_aid_handle_exit_play(void);

#endif


