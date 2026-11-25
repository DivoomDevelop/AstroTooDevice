
#include "divoom.h"

#if 0
#define DIVOOM_DECODE_LOL_BUF_LEN  1024

typedef struct _divoom_decode_lol_item
{
	int summonerLevel;
	int timePlayed;
	int win;
	int kills;
	int deaths;
	int assists;
}divoom_decode_lol_item, *Pdivoom_decode_lol_item;

typedef struct _divoom_decode_lol_ctrl
{
	int find_puuid;
	int find_num;
	int have_result;
	int  flag_len;
	int  result_start_record;
	int  result_len;
	char  puuid[256];
	char flag_buf[DIVOOM_DECODE_LOL_BUF_LEN];
	char result_buf[DIVOOM_DECODE_LOL_BUF_LEN];

}divoom_decode_lol_ctrl, *Pdivoom_decode_lol_ctrl;

void *divoom_decode_lol_init(char *puuid)
{
	Pdivoom_decode_lol_ctrl phandle;

	phandle = (Pdivoom_decode_lol_ctrl)divoom_system_malloc(sizeof(divoom_decode_lol_ctrl));
	if (phandle == NULL)
	{
		return NULL;
	}
	memset(phandle, 0, sizeof(divoom_decode_lol_ctrl));
	strcpy(phandle->puuid, puuid);
	phandle->flag_len = -1;
	phandle->result_len = -1;

	return (void *)phandle;
}


int divoom_decode_lol_get_result(Pdivoom_decode_lol_ctrl phandle, uint8 *buf, int len, int offset, Pdivoom_decode_lol_item plol_item)
{
	int str_off;
	char *data_ptr;

	if (phandle->result_start_record == 0)
	{
		//第一个为[,{我们就认为这个结果结束了，是数组，如果为子母或数字或"，我们认为开始
		while (offset < len)
		{
			if (buf[offset] == '[' || buf[offset] == '{' || buf[offset] == ']' || buf[offset] == '}' || buf[offset] == ',')
			{
				offset++;
				phandle->result_len = -1;
				phandle->flag_len = -1;
				return offset;
			}
			else if (buf[offset] == '\"' || (buf[offset] >= '0' && buf[offset] <= '9') ||
				(buf[offset] >= 'a' && buf[offset] <= 'z') || (buf[offset] >= 'A' && buf[offset] <= 'Z'))
			{
				break;
			}
			offset++;
		}

		if (offset >= len)
		{
			return len;
		}
		phandle->result_start_record = 1;
		phandle->result_len = 0;
	}
	//结束都以','

	while (offset < len && buf[offset] != ',')
	{
		phandle->result_buf[phandle->result_len] = buf[offset];
		phandle->result_len++;
		offset++;
	}
	if (offset < len)
	{
		//找到结果了
		//printf("find: %s: result: %s\n", phandle->flag_buf, phandle->result_buf);
		phandle->result_buf[phandle->result_len] = 0;
		data_ptr = phandle->result_buf;
		str_off = 0;
		while (*data_ptr)
		{
			if (data_ptr[0] == '\"')
			{
				data_ptr++;
				str_off++;
				phandle->result_len--;
				//将最后"去掉"
				while (phandle->result_buf[phandle->result_len] != '"' && phandle->result_len > 0)
				{
					phandle->result_len--;
				}
				phandle->result_buf[phandle->result_len] = 0;
				break;
			}
			if ((data_ptr[0] >= '0' && data_ptr[0] >= '9'))
			{
				break;
			}
			if ((data_ptr[0] >= 'a' && data_ptr[0] >= 'z') || (data_ptr[0] >= 'A' && data_ptr[0] >= 'Z'))
			{
				break;
			}
			data_ptr++;
			str_off++;
		}
		if (strcmp(phandle->flag_buf, "assists")==0)
		{
			plol_item->assists = atoi(phandle->result_buf);
			phandle->find_num = 1;
		}
		else if (strcmp(phandle->flag_buf, "puuid") == 0)
		{
			if (strcmp(&phandle->result_buf[str_off], phandle->puuid))
			{
                divoom_print("pass: %s\n", phandle->puuid);
				phandle->find_puuid = 0;
			}
			else
			{
                divoom_print("find: %s\n", phandle->puuid);
				phandle->find_puuid = 1;
			}
		}
		else if (strcmp(phandle->flag_buf, "summonerLevel") == 0)
		{
			plol_item->summonerLevel = atoi(phandle->result_buf);
			phandle->find_num++;
		}
		else if (strcmp(phandle->flag_buf, "deaths") == 0)
		{
			plol_item->deaths = atoi(phandle->result_buf);
			phandle->find_num++;
		}
		else if (strcmp(phandle->flag_buf, "timePlayed") == 0)
		{
			plol_item->timePlayed = atoi(phandle->result_buf);
				phandle->find_num++;
		}
		else if (strcmp(phandle->flag_buf, "win") == 0)
		{
			if (memcmp(phandle->result_buf, "true", 4))
			{
				plol_item->win = 0;
			}
			else
			{
				plol_item->win = 1;
			}
				phandle->find_num++;
		}
		else if (strcmp(phandle->flag_buf, "kills") == 0)
		{
			plol_item->kills = atoi(phandle->result_buf);
			phandle->find_num++;
		}
		phandle->result_start_record = 0;
		phandle->result_len = -1;
		phandle->flag_len = -1;
		return offset;
	}
	else
	{
		return len;
	}

}

uint8 divoom_decode_lol_write(void *handle, uint8 *buf, int len, Pdivoom_decode_lol_item plol_item)
{
	int offset = 0, pass_num = 0;;
	Pdivoom_decode_lol_ctrl phandle = (Pdivoom_decode_lol_ctrl)handle;

	if (handle == NULL)
	{
		return 0;
	}

	if (phandle->result_len != -1)
	{
		offset = divoom_decode_lol_get_result(phandle, buf, len, 0, plol_item);
		if (offset == len)
		{
			return 0;
		}
		if (phandle->find_num >= 6 && phandle->find_puuid)
		{
			return 1;
		}
	}

	while (offset < len)
	{
		if (phandle->flag_len == -1)
		{
			//跳过前面一些字符
			while (buf[offset] != '"' && offset < len)
			{
				offset++;
			}
			phandle->flag_len = 0;
			offset++;
		}

		while (buf[offset] != '"' && offset < len)
		{
			phandle->flag_buf[phandle->flag_len] = buf[offset];
			phandle->flag_len++;
			offset++;
		}
		if (offset >= len)
		{
			break;
		}
		offset++;
		phandle->flag_buf[phandle->flag_len] = 0;
		phandle->result_len = 0;
		phandle->have_result = 0;
		phandle->result_start_record = 0;

		offset = divoom_decode_lol_get_result(phandle, buf, len, offset, plol_item);
		if (offset == len)
		{
			return 0;
		}
		pass_num++;
		if (phandle->find_num >= 6 && phandle->find_puuid)
		{
			return 1;
		}
	}

	return 0;
}


void divoom_decode_lol_exit(void *handle)
{
	divoom_system_free(handle);
}

int divoom_http_client_event_handler(esp_http_client_event_t *evt);

char *divoom_decode_lol_url_request(char *url_addr, char *puuid)
{
    uint8 result = 0;
	char *buffer = NULL;
	esp_err_t err;
	int content_length, read_len = 0;
    void *divoom_handle = NULL;
    divoom_decode_lol_item plol_info;
	
	esp_http_client_config_t config = {	
		.url = url_addr,
        .timeout_ms = 20000,
        .method = HTTP_METHOD_GET,
		.event_handler = divoom_http_client_event_handler,
		//.skip_cert_common_name_check = true,
	};
 	if (url_addr == NULL || strlen(url_addr) == 0)
 	{
		divoom_print("url_addr is null!\n");
		return NULL;
	}
	esp_http_client_handle_t client = esp_http_client_init(&config);
 	if (client == NULL)
 	{
		divoom_print("client is null maybe the url is wrong");
		return NULL;
	}
	
    esp_http_client_set_header(client, "Content-Type", "application/json");
	if ((err = esp_http_client_open(client, 0)) != ESP_OK) 
	{
		divoom_print("Failed to open HTTP connection: %x", err);
        esp_http_client_cleanup(client);
		return NULL;
	}
    
    do
    {
	    content_length =  esp_http_client_fetch_headers(client);
        if(content_length == 0)
        {
            if(esp_http_client_is_chunked_response(client))
            {
                divoom_print("check!\n");
                content_length = 200*1024;
                break;
            }
            ESP_LOGI("HTTP", "retry\n");
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
    }while(content_length == 0);
        
    
	if (content_length > 0) 
	{
        int cur_read = 0, read_times = 0;

        divoom_handle = divoom_decode_lol_init(puuid);
        if(divoom_handle == NULL)
        {
            esp_http_client_cleanup(client);
            return NULL;
        }
		buffer = (char *)divoom_malloc(DIVOOM_DECODE_LOL_BUF_LEN, DIVOOM_MODEL_NET, __LINE__, __FILE__);
		DIVOOM_MEM_CHECK("HTTP_CLIENT", buffer, goto Error);
        while(cur_read < content_length )
        {
            read_times ++;
            if(cur_read + read_len < content_length)
            {
                read_len = DIVOOM_DECODE_LOL_BUF_LEN;
            }
            else
            {
                read_len = content_length - cur_read;
            }
    		read_len = esp_http_client_read(client, buffer, read_len);
    		if (read_len < 0) 
    		{
    			divoom_print("Error read data");
    			divoom_free(buffer);
    			goto Error;
    		}
            else if(read_len == 0)
            {
                break;
            }
            result = divoom_decode_lol_write(divoom_handle, (uint8 *)buffer, read_len, &plol_info);
            if(result == 1)
            {
                break;
            }
            if(read_times >= 20)
            {
                divoom_system_delay_ms(10);
                read_times = 0;
            }
            cur_read += read_len;
        }
		divoom_print("http read data :%d, %d\n", cur_read, content_length);
	}
    else{
			ESP_LOGE("divoom_http","Error url: %s read data : %d\n", url_addr, content_length);
    }
    divoom_decode_lol_exit(divoom_handle);
	esp_http_client_cleanup(client);
    divoom_system_free(buffer);
	if(result)
	{
        divoom_cJSON *root = NULL;

        root = divoom_cJSON_CreateObject();
        divoom_cJSON_AddNumberToObject(root , "summonerLevel", plol_info.summonerLevel);
        divoom_cJSON_AddNumberToObject(root , "timePlayed", plol_info.timePlayed);
        divoom_cJSON_AddNumberToObject(root , "win", plol_info.win);
        divoom_cJSON_AddNumberToObject(root , "kills", plol_info.kills);
        divoom_cJSON_AddNumberToObject(root , "deaths", plol_info.deaths);
        divoom_cJSON_AddNumberToObject(root , "assists", plol_info.assists);
        buffer = divoom_cJSON_Print(root);
        divoom_cJSON_Delete(root);
    	return buffer;
	}
    return NULL;
Error:
    if(divoom_handle)
    {
        divoom_decode_lol_exit(divoom_handle);
    }
	esp_http_client_cleanup(client);
	return NULL;
}


#if 0

int _tmain(int argc, _TCHAR* argv[])
{
	int read_len, ret;
	void *divoom_handle;
	uint8 file_buf[10 * 1024];
	FILE *file_handle;
	divoom_decode_lol_item plol_item;

	fopen_s(&file_handle, "lol.bin", "rb");
	if (file_handle == NULL)
	{
		return 0;
	}
	divoom_handle = divoom_decode_lol_init("vQV-JEneOnU7XoW_ihbSxKj6DVmpZOVDqEpmqW6298nBzYOqnh50T3CoMKpsJLCuJqOx-K1boZYZGw");
	while (1)
	{
		read_len = fread_s(file_buf, 10 * 1024, 1, 1024, file_handle);
		if (read_len == 0)
		{
			break;
		}
		ret = divoom_decode_lol_write(divoom_handle, file_buf, read_len, &plol_item);
		if (ret)
		{
			printf("find !\n");
			break;
		}
		if (read_len < 1024)
		{

			break;
		}
	}
	divoom_port_file_close(file_handle);
	divoom_decode_lol_exit(divoom_handle);
	return 0;
}
#endif

#endif


