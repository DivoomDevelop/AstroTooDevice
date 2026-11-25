#ifndef _divoom_led_edit_json_
#define _divoom_led_edit_json_

#define DIVOOM_LED_EDIT_WORD_MAX_NUM	64*16

typedef struct _divoom_led_edit_info
{	
	int led_len;
    int user_id;
	uint8 *led_data;
}divoom_led_edit_info, *Pdivoom_led_edit_info;
 

typedef struct _divoom_led_edit_set_text_info
{
	//char text[DIVOOM_LED_EDIT_WORD_MAX_NUM];
	int user_id;
	int text_num;
	uint16 text_list[64];
	int speed;
}divoom_led_edit_set_text_info, *Pdivoom_led_edit_set_text_info;

typedef struct _divoom_led_edit_set_speed
{
	int speed;
}divoom_led_edit_set_speed, *Pdioom_led_edit_set_speed;

typedef struct _divoom_led_edit_send_text_info
{
	int     light;
    int     speed;
	int     text_num;
	uint16 text_list[64];
    uint8   word_color[3];
    uint8   back_color[3];
}divoom_led_edit_send_text_info, *Pdivoom_led_edit_send_text_info;

int divoom_led_edit_json_send_data_resquest_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_led_edit_json_send_data_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_led_edit_json_set_speed_resquest_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_led_edit_json_set_speed_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_led_edit_json_set_text_resquest_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_led_edit_json_set_text_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_led_edit_json_text_stop_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_led_edit_json_text_stop_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_led_edit_json_text_stop_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_led_edit_json_text_stop_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_led_edit_json_send_text_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_led_edit_json_send_text_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_led_edit_json_send_data_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

#endif

