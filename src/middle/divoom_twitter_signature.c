#include "divoom.h"
#include <limits.h>

#define TWITTER_HEAD_HTTPS  "OAuth oauth_consumer_key=%s, oauth_nonce=%s, oauth_signature_method=HMAC-SHA1, oauth_timestamp=%s, oauth_token=%s, oauth_version=1.0, oauth_signature=%s"

int hmac_sha1(const uint8 *key, int key_len, const uint8 *data, int data_len,
	       uint8 *mac);

typedef struct _divoom_array_header_info
{
	char name[36];
	char value[64];
}divoom_array_header_info, *Pdivoom_array_header_info;

static unsigned char hexchars[] = "0123456789ABCDEF";
/**
 * 16进制数转换成10进制数
 * 如：0xE4=14*16+4=228
 */
static int divoom_twitter_signature_htoi(char *s)
{
    int value;
    int c;

    c = ((unsigned char *)s)[0];
    if (isupper(c))
        c = tolower(c);
    value = (c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10) * 16;

    c = ((unsigned char *)s)[1];
    if (isupper(c))
        c = tolower(c);
    value += c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10;

    return (value);
}

char *divoom_twitter_signature_raw_url_encode(char const *s, int len, int *new_length)
{
    register unsigned char c;
    unsigned char *to, *start;
    unsigned char const *from, *end;
    
    if (new_length) *new_length = 0;
    if (s == NULL || len < 0 || len > (INT_MAX - 1) / 3)
        return NULL;
    from = (unsigned char *)s;
    end  = (unsigned char *)s + len;
    start = to = (unsigned char *)divoom_system_malloc(3*len+1);
    if (start == NULL)
        return NULL;

    while (from < end) 
    {
        c = *from++;
        if ((c < '0' && c != '-' && c != '.') ||(c < 'A' && c > '9') ||(c > 'Z' && c < 'a' && c != '_') ||(c > 'z')) 
        {
            to[0] = '%';
            to[1] = hexchars[c >> 4];//将2进制转换成16进制表示
            to[2] = hexchars[c & 15];//将2进制转换成16进制表示
            to += 3;
        }
        else 
        {
            *to++ = c;
        }
    }
    *to = 0;
    if (new_length) 
    {
		*new_length = to - start;
    }
    return (char *) start;
}

char *divoom_twitter_signature_url_encode(char const *s, int len, int *new_length)
{
    register unsigned char c;
    unsigned char *to, *start;
    unsigned char const *from, *end;
    
    if (new_length) *new_length = 0;
    if (s == NULL || len < 0 || len > (INT_MAX - 1) / 3)
        return NULL;
    from = (unsigned char *)s;
    end  = (unsigned char *)s + len;
    start = to = (unsigned char *)divoom_system_malloc(3*len+1);
    if (start == NULL)
        return NULL;

    while (from < end) 
    {
        c = *from++;

        if (c == ' ') 
        {
            *to++ = '+';
        } 
        else if ((c < '0' && c != '-' && c != '.') ||
                 (c < 'A' && c > '9') ||
                 (c > 'Z' && c < 'a' && c != '_') ||
                 (c > 'z')) 
        {
            to[0] = '%';
            to[1] = hexchars[c >> 4];//将2进制转换成16进制表示
            to[2] = hexchars[c & 15];//将2进制转换成16进制表示
            to += 3;
        }
        else 
        {
            *to++ = c;
        }
    }
    *to = 0;
    if (new_length) 
    {
        *new_length = to - start;
    }
    return (char *) start;
}
/*
  Function:       divoom_twitter_signature_url_decode
  Description:    对字符串进行ＵＲＬ解码
  Input:          NONE
  Return:         NONE
*/

int divoom_twitter_signature_url_decode(char *str, int len)
{
    char *dest = str;
    char *data = str;

    if(len <= 0)
    {
        return 0;
    }
    while (len--) 
    {
        if (*data == '+') 
        {
            *dest = ' ';
        }
        else if (*data == '%' && len >= 2 && isxdigit((int) *(data + 1)) && isxdigit((int) *(data + 2))) 
        {
            *dest = (char) divoom_twitter_signature_htoi(data + 1);
            data += 2;
            len -= 2;
        } 
        else 
        {
            *dest = *data;
        }
        data++;
        dest++;
    }
    *dest = '\0';
    return dest - str;
}

Pdivoom_array_header_info divoom_twitter_signature_deal_para(char *para, uint8 *para_num)
{
	char *p1, *p2, *p3;
	uint8 num = 0, i = 0;
	Pdivoom_array_header_info parray_info = NULL;

	if (para == NULL || para_num == NULL)
		return NULL;
	*para_num = 0;
	p1 = strchr(para, '?');
	if (p1 != NULL)
	{
		p1++;
		p2 = p1;
		num++;
		while ((p1 = strchr(p1, '&')))//先统计有多少个参数
		{
			/* The six OAuth fields share the uint8 iteration index. */
			if (num >= UINT8_MAX - 6)
				return NULL;
			p1++;
			num++;
		}
		parray_info = divoom_system_malloc(num * sizeof(divoom_array_header_info));
		if (parray_info == NULL)
			return NULL;
		memset(parray_info, 0, num * sizeof(*parray_info));

		p1 = p2;
		while ((p1 = strchr(p1, '&')))
		{
			p3 = strchr(p2, '=');
			if (p3 != NULL && p3 < p1 &&
			    (size_t)(p3 - p2) < sizeof(parray_info[i].name) &&
			    (size_t)(p1 - p3 - 1) < sizeof(parray_info[i].value))
			{
				strncpy(parray_info[i].name, p2, p3 - p2);
				p3++;
				strncpy(parray_info[i].value, p3, p1 - p3);
				divoom_twitter_signature_url_decode(parray_info[i].value, p1 - p3);
			}
			else
			{
				divoom_print("error\n");
				divoom_free(parray_info);
				return NULL;
			}
			p1++;
			p2 = p1;
			i++;
		}
		//上面的取完了，还剩最后一个
		p3 = strchr(p2, '=');
		if (p3 != NULL &&
		    (size_t)(p3 - p2) < sizeof(parray_info[i].name) &&
		    strlen(p3 + 1) < sizeof(parray_info[i].value))
		{
			strncpy(parray_info[i].name, p2, p3 - p2);
			p3++;
			strcpy(parray_info[i].value, p3); //最后一个
			divoom_twitter_signature_url_decode(parray_info[i].value, strlen(p3));
		}	
		else
		{
			divoom_print("error\n");
			divoom_free(parray_info);
			return NULL;
		}
	}
	*para_num = num;
	return parray_info;
}

void divoom_twitter_signature_sort_header(Pdivoom_array_header_info header_array, int size)
{
	//按照字母进行升序排序
	char name1[36], name2[36];
	divoom_array_header_info temp;
	//冒泡排序法
    int i, j, k, len1, len2;
    for (i = 0; i < size -1; i++) 
    {
        for ( j = 0; j < size -1 - i; j++) 
		{
			memset(name1, 0, sizeof(name1));
			memset(name2, 0, sizeof(name2));
			strcpy(name1, header_array[j].name);		
			strcpy(name2, header_array[j + 1].name);
			len1 = strlen(name1);
			len2 = strlen(name2);
		
			if (len1 < len2)		
			{
				for (k = 0; k < len1; k++)
				{
					if (name1[k] > name2[k])
					{
						memset(&temp, 0, sizeof(divoom_array_header_info));
						strcpy(temp.name, header_array[j].name);
						strcpy(temp.value, header_array[j].value);

						memset(&header_array[j], 0, sizeof(divoom_array_header_info));
						strcpy(header_array[j].name, header_array[j + 1].name);
						strcpy(header_array[j].value, header_array[j + 1].value);
						
						memset(&header_array[j + 1], 0, sizeof(divoom_array_header_info));
						strcpy(header_array[j + 1].name, temp.name);
						strcpy(header_array[j + 1].value, temp.value);
						break;
					}
					else if (name1[k] < name2[k])
					{
						break;				
					}
				}
			}
			else
			{
				for (k = 0; k < len2; k++)
				{
					if (name1[k] > name2[k])
					{
						memset(&temp, 0, sizeof(divoom_array_header_info));
						strcpy(temp.name, header_array[j].name);
						strcpy(temp.value, header_array[j].value);

						memset(&header_array[j], 0, sizeof(divoom_array_header_info));
						strcpy(header_array[j].name, header_array[j + 1].name);
						strcpy(header_array[j].value, header_array[j + 1].value);
						
						memset(&header_array[j + 1], 0, sizeof(divoom_array_header_info));
						strcpy(header_array[j + 1].name, temp.name);
						strcpy(header_array[j + 1].value, temp.value);
						break;
					}
					else if (name1[k] < name2[k])
					{
						break;				
					}
				}
				if (k == len2)
				{
					memset(&temp, 0, sizeof(divoom_array_header_info));
					strcpy(temp.name, header_array[j].name);
					strcpy(temp.value, header_array[j].value);

					memset(&header_array[j], 0, sizeof(divoom_array_header_info));
					strcpy(header_array[j].name, header_array[j + 1].name);
					strcpy(header_array[j].value, header_array[j + 1].value);
					
					memset(&header_array[j + 1], 0, sizeof(divoom_array_header_info));
					strcpy(header_array[j + 1].name, temp.name);
					strcpy(header_array[j + 1].value, temp.value);
				}
			}
        }
    }
}

Pdivoom_twitter_signature_header_info divoom_twitter_signature_get_authorization_header(char *consumer_key, char *consumer_secret, char *oauth_token, char *oauth_token_secret, char *http_method, char *request_url, char *para)
{
	Pdivoom_twitter_signature_header_info header = NULL;
	uint32 time_stamp = time(0);
	uint8 i = 0, array_size = 0, j = 0;
	Pdivoom_array_header_info parray_info = NULL;
	Pdivoom_array_header_info header_array = NULL;
	char *new_str = NULL, oauth_signature[512] = {0}, oauth_signature2[520] = {0}, signature[20] = {0}, key[128] = {0}, signature_base64[40] = {0};
	int new_length = 0, len = 0, written;

	if (consumer_key == NULL || consumer_secret == NULL || oauth_token == NULL ||
	    oauth_token_secret == NULL || http_method == NULL || request_url == NULL ||
	    para == NULL || strlen(consumer_key) >= STR_LEN || strlen(oauth_token) >= STR_LEN ||
	    strlen(request_url) > (INT_MAX - 1) / 3)
		return NULL;
	header = divoom_system_malloc(sizeof(divoom_twitter_signature_header_info));
	if (header == NULL)
		return NULL;
	//初始化头
	strcpy(header->oauth_consumer_key, consumer_key);
	strcpy(header->oauth_nonce, "UFMhInyzygl");
	strcpy(header->oauth_signature_method, "HMAC-SHA1");
	strcpy(header->oauth_token, oauth_token);
	sprintf(header->oauth_timestamp, "%d", time_stamp);
	strcpy(header->oauth_version, "1.0");

	//解析para
	parray_info = divoom_twitter_signature_deal_para(para, &array_size);
	if (strchr(para, '?') && parray_info == NULL)
		goto failed;
	header_array = divoom_system_malloc((6 + array_size) * sizeof(divoom_array_header_info));
	if (header_array == NULL)
		goto failed;
	i = 0;
	strcpy(header_array[i].name, "oauth_consumer_key");
	strcpy(header_array[i++].value, header->oauth_consumer_key);
	strcpy(header_array[i].name, "oauth_nonce");
	strcpy(header_array[i++].value, header->oauth_nonce);
	strcpy(header_array[i].name, "oauth_signature_method");
	strcpy(header_array[i++].value, header->oauth_signature_method);
	strcpy(header_array[i].name, "oauth_token");
	strcpy(header_array[i++].value, header->oauth_token);
	strcpy(header_array[i].name, "oauth_timestamp");
	strcpy(header_array[i++].value, header->oauth_timestamp);
	strcpy(header_array[i].name, "oauth_version");
	strcpy(header_array[i++].value, header->oauth_version);

	if (array_size > 0)
	{
		for (j = 0; j < array_size; j++)
		{
			strcpy(header_array[i].name, parray_info[j].name);
			strcpy(header_array[i].value, parray_info[j].value);
			i++;
		}
		divoom_free(parray_info);
		parray_info = NULL;
	}
	//对header_array进行排序，按字母进行排序
	divoom_twitter_signature_sort_header(header_array, 6 + array_size);
	//对排好序的header_array进行rawurlencode
	for (i = 0; i < 6 + array_size; i++)
	{
		if (i < 6 + array_size - 1)
		{
			written = snprintf(&oauth_signature[len], sizeof(oauth_signature) - len,
			    "%s=%s&", header_array[i].name, header_array[i].value);
		}
		else
		{
			written = snprintf(&oauth_signature[len], sizeof(oauth_signature) - len,
			    "%s=%s", header_array[i].name, header_array[i].value);
		}
		if (written < 0 || (size_t)written >= sizeof(oauth_signature) - len)
			goto failed;
		len += written;
	}
	new_str = divoom_twitter_signature_raw_url_encode(oauth_signature, strlen(oauth_signature), &new_length);
	if (new_str == NULL || (size_t)new_length >= sizeof(oauth_signature))
		goto failed;

	memcpy(oauth_signature, new_str, (size_t)new_length + 1);
	divoom_free(new_str);
	new_str = NULL;

	new_str = divoom_twitter_signature_url_encode(request_url, strlen(request_url), NULL);
	if (new_str == NULL)
		goto failed;
	written = snprintf(oauth_signature2, sizeof(oauth_signature2),
	    "%s&%s&%s", http_method, new_str, oauth_signature);
	if (written < 0 || (size_t)written >= sizeof(oauth_signature2))
		goto failed;
	divoom_free(new_str);
	new_str = NULL;

	written = snprintf(key, sizeof(key), "%s&%s", consumer_secret, oauth_token_secret);
	if (written < 0 || (size_t)written >= sizeof(key))
		goto failed;
	//hmac_sha1((uint8 *)key, strlen(key), (uint8 *)oauth_signature2, strlen(oauth_signature2), (uint8 *)signature);

	//对signature进行base_64编码
	divoom_base64_encode((uint8 *)signature, signature_base64, 20);
	new_str = divoom_twitter_signature_url_encode(signature_base64, strlen(signature_base64), &new_length);
	if (new_str == NULL || (size_t)new_length >= sizeof(header->oauth_signature))
		goto failed;

	memcpy(header->oauth_signature, new_str, (size_t)new_length + 1);
	divoom_free(new_str);
	divoom_free(header_array);
	return header;
failed:
	if (new_str) divoom_free(new_str);
	if (parray_info) divoom_free(parray_info);
	if (header_array) divoom_free(header_array);
	divoom_free(header);
	return NULL;
}

/*
  Function:       divoom_twitter_signature_request
  Description:    推特请求,带签名
  Input:          NONE
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
char *divoom_twitter_signature_request(char *url_addr, Pdivoom_http_client_url_head_info phead_info, int head_num, void *handle)
{
	Pdivoom_twitter_signature_header_info header_info = NULL;
	divoom_http_client_url_head_info head_info = {0};
	int i = 0;
	char consumer_key[] = "06SFlPIWjdrBCBxoCGNUR651l";
	char consumer_secret[] = "wxRT8JANSAz7fo2X82o81G8T3KD3uNylyLaCe0vwlvbQ9KzIk5";
	char http_method[] = "GET";
	char oauth_token[64] = {0};
	char oauth_token_secret[64] = {0};
	char url[520] = {0};
	char para[540] = {0};

	if (url_addr == NULL || head_num < 0 || (head_num && phead_info == NULL))
		return NULL;
	for (i = 0; i < head_num; i++)
	{
		if (strcmp(phead_info[i].key, "oauth_token") == 0)
		{
			if (strlen(phead_info[i].word) >= sizeof(oauth_token))
				return NULL;
			strcpy(oauth_token, phead_info[i].word);
		}

		if (strcmp(phead_info[i].key, "oauth_token_secret") == 0)
		{
			if (strlen(phead_info[i].word) >= sizeof(oauth_token_secret))
				return NULL;
			strcpy(oauth_token_secret, phead_info[i].word);
		}

		if (strcmp(phead_info[i].key, "user_id") == 0)
		{
			int n = snprintf(para, sizeof(para), "?user_id=%s", phead_info[i].word);
			if (n < 0 || (size_t)n >= sizeof(para))
				return NULL;
		}

		if (strcmp(phead_info[i].key, "id") == 0)
		{
			int n = snprintf(para, sizeof(para), "?id=%s", phead_info[i].word);
			if (n < 0 || (size_t)n >= sizeof(para))
				return NULL;
		}
	}
	i = snprintf(url, sizeof(url), "%s%s", url_addr, para);
	if (i < 0 || (size_t)i >= sizeof(url))
		return NULL;
	header_info = divoom_twitter_signature_get_authorization_header(consumer_key, consumer_secret, oauth_token, oauth_token_secret, http_method, url_addr, para);
	if (header_info)
	{
		i = snprintf(head_info.word, sizeof(head_info.word), TWITTER_HEAD_HTTPS, header_info->oauth_consumer_key, header_info->oauth_nonce, header_info->oauth_timestamp, header_info->oauth_token, header_info->oauth_signature);
		divoom_free(header_info);
		if (i < 0 || (size_t)i >= sizeof(head_info.word))
			return NULL;
		strcpy(head_info.key, "authorization");
		return divoom_http_client_url_request(url, &head_info, 1, divoom_clock_control_check_exit_flag, handle);
	}
	return NULL;
}

/*
  Function:       divoom_twitch_request
  Description:    推趣请求
  Input:          NONE
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
char *divoom_twitch_request(char *url_addr, Pdivoom_http_client_url_head_info phead_info, int head_num, void *handle)
{
	int i = 0, num = 0;
	divoom_http_client_url_head_info head_info[3] = {0};

	strcpy(head_info[0].key, "Accept");
	strcpy(head_info[0].word, "application/vnd.twitchtv.v5+json");

	for (i = 0; i < head_num; i++)
	{
		if (strcmp(phead_info[i].key, "Client-ID") == 0)
		{
			strcpy(head_info[1].key, "Client-ID");
			strcpy(head_info[1].word, phead_info[i].word);
		}	

		if (strcmp(phead_info[i].key, "Authorization") == 0)
		{
			strcpy(head_info[2].key, "Authorization");
			strcpy(head_info[2].word, phead_info[i].word);
		}	
	}	

	if (strlen(head_info[2].key) == 0)
		num = 2;
	else
		num = 3;

	return divoom_http_client_url_request(url_addr, head_info, num, divoom_clock_control_check_exit_flag, handle);
}

