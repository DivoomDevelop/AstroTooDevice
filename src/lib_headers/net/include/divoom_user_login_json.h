#ifndef _DIVOOM_USER_LOGIN_JSON_H_
#define _DIVOOM_USER_LOGIN_JSON_H_

typedef struct _divoom_get_accesstoken_info_
{
	char email_mac[128];
	//32位小写md5
	char password[33];
}divoom_get_accesstoken_info, *Pdivoom_get_accesstoken_info;

typedef struct _divoom_login_info_
{
	uint32 userid;
	uint32 accesstoken;
}divoom_login_info, *Pdivoom_login_info;

typedef struct _divoom_aliyun_info_
{
	char aliyunid[64];
	char product_key[20];
	char product_secret[64];
}divoom_aliyun_info, *Pdivoom_aliyun_info;


void divoom_get_accesstoken_info_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_get_accesstoken_info_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_get_aliyun_info_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_get_aliyun_info_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);


#endif




