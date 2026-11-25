#ifndef _divoom_twitter_signature_
#define _divoom_twitter_signature_

#define STR_LEN 64
typedef struct _divoom_twitter_signature_header_info
{
	char oauth_consumer_key[STR_LEN];
	char oauth_nonce[STR_LEN];
	char oauth_signature_method[STR_LEN];
	char oauth_timestamp[STR_LEN];
	char oauth_token[STR_LEN];
	char oauth_version[STR_LEN];
	char oauth_signature[STR_LEN];
	char oauth_requeset_url[STR_LEN];
	char oauth_requeset_para[STR_LEN];
}divoom_twitter_signature_header_info, *Pdivoom_twitter_signature_header_info;

/*
  Function:       divoom_twitter_signature_request
  Description:    推特请求,带签名
  Input:          NONE
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
char *divoom_twitter_signature_request(char *url_addr, Pdivoom_http_client_url_head_info phead_info, int head_num, void *handle);
/*
  Function:       divoom_twitch_request
  Description:    推趣请求
  Input:          NONE
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
char *divoom_twitch_request(char *url_addr, Pdivoom_http_client_url_head_info phead_info, int head_num, void *handle);

/*
  Function:       divoom_twitter_signature_url_decode
  Description:    对字符串进行ＵＲＬ解码
  Input:          NONE
  Return:         NONE
*/

int divoom_twitter_signature_url_decode(char *str, int len);
char *divoom_twitter_signature_url_encode(char const *s, int len, int *new_length);

#endif
