#include "divoom.h"

#if !DIVOOM_BUILD_SIMULATOR
#define tinfl_decompress divoom_ota_tinfl_decompress
#include "game_runtime/miniz.h"
#undef tinfl_decompress

#define DIVOOM_WEB_GZIP_INPUT_MAX (16U * 1024U)
#define DIVOOM_WEB_GZIP_TEXT_MAX (32U * 1024U)
#define DIVOOM_WEB_GZIP_TIMEOUT_MS 8000U

typedef struct
{
    uint8 *data;
    size_t length;
    uint32 start_ms;
    int too_large;
} divoom_web_gzip_buffer;

static int divoom_web_gzip_exit(void *arg)
{
    const divoom_web_gzip_buffer *buffer = arg;

    return buffer->too_large ||
           divoom_system_get_tickms() - buffer->start_ms >= DIVOOM_WEB_GZIP_TIMEOUT_MS;
}

static void divoom_web_gzip_read(void *arg, uint8 *data, int length, int total)
{
    divoom_web_gzip_buffer *buffer = arg;

    if (total > (int)DIVOOM_WEB_GZIP_INPUT_MAX || length < 0 ||
        (size_t)length > DIVOOM_WEB_GZIP_INPUT_MAX - buffer->length) {
        buffer->too_large = 1;
        return;
    }
    memcpy(buffer->data + buffer->length, data, (size_t)length);
    buffer->length += (size_t)length;
}

/* Decode one RFC 1952 member with a bounded output and validate its trailer. */
static char *divoom_web_gzip_decode(const uint8 *data, size_t length, uint32 *text_length)
{
    const uint8 *input;
    const uint8 *trailer;
    size_t input_size, output_size;
    uint32 expected_size, expected_crc;
    tinfl_decompressor *inflater;
    tinfl_status status;
    char *text;
    unsigned flags;
    unsigned field;

    if (length < 18 || data[0] != 0x1f || data[1] != 0x8b || data[2] != 8 ||
        (data[3] & 0xe0)) {
        return NULL;
    }
    flags = data[3];
    input = data + 10;
    trailer = data + length - 8;
    if (flags & 4) {
        if ((size_t)(trailer - input) < 2) {
            return NULL;
        }
        input_size = MZ_READ_LE16(input);
        input += 2;
        if (input_size > (size_t)(trailer - input)) {
            return NULL;
        }
        input += input_size;
    }
    for (field = 8; field <= 16; field <<= 1) {
        if (flags & field) {
            input = memchr(input, 0, (size_t)(trailer - input));
            if (input == NULL) {
                return NULL;
            }
            ++input;
        }
    }
    if (flags & 2) {
        if ((size_t)(trailer - input) < 2 ||
            (uint16)mz_crc32(0, data, (size_t)(input - data)) != MZ_READ_LE16(input)) {
            return NULL;
        }
        input += 2;
    }

    expected_crc = MZ_READ_LE32(trailer);
    expected_size = MZ_READ_LE32(trailer + 4);
    if (input == trailer || expected_size == 0 || expected_size > DIVOOM_WEB_GZIP_TEXT_MAX) {
        return NULL;
    }

    text = divoom_system_malloc(expected_size + 1);
    inflater = divoom_system_malloc(sizeof(*inflater));
    if (text == NULL || inflater == NULL) {
        divoom_system_free(text);
        divoom_system_free(inflater);
        return NULL;
    }
    tinfl_init(inflater);
    input_size = (size_t)(trailer - input);
    output_size = expected_size;
    status = divoom_ota_tinfl_decompress(inflater, input, &input_size,
        (uint8 *)text, (uint8 *)text, &output_size, TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
    divoom_system_free(inflater);

    if (status != TINFL_STATUS_DONE || input_size != (size_t)(trailer - input) ||
        output_size != expected_size || mz_crc32(0, (const uint8 *)text, output_size) != expected_crc ||
        memchr(text, 0, output_size) != NULL) {
        divoom_system_free(text);
        return NULL;
    }
    text[output_size] = '\0';
    *text_length = expected_size;
    return text;
}

static int divoom_web_get_gzip_text(char *url, Pdivoom_web_get_msg_info msg)
{
    divoom_http_client_url_head_info *header = divoom_system_malloc(sizeof(*header));
    divoom_web_gzip_buffer buffer = {0};

    if (header == NULL) {
        return 0;
    }
    buffer.data = divoom_system_malloc(DIVOOM_WEB_GZIP_INPUT_MAX);
    if (buffer.data != NULL) {
        strcpy(header->key, "Accept-Encoding");
        strcpy(header->word, "gzip");
        buffer.start_ms = divoom_system_get_tickms();
        if (divoom_http_client_get_net_tofile(url, divoom_web_gzip_read, &buffer,
                header, 1, divoom_web_gzip_exit, &buffer) == 0 && !divoom_web_gzip_exit(&buffer)) {
            msg->data = divoom_web_gzip_decode(buffer.data, buffer.length, &msg->len);
        }
        divoom_system_free(buffer.data);
    }
    divoom_system_free(header);
    return msg->data != NULL;
}
#endif

//队列最大个数
#define DIVOOM_WEB_GET_QUEUE_MAX_NUM	20


typedef struct __divoom_web_get_ctrl
{
    Pdivoom_web_get_request_info pcur_use;
    void *lock;
	void *request_queue;
    void *request_sem;
}divoom_web_get_ctrl,*Pdivoom_web_get_ctrl;


static Pdivoom_web_get_ctrl pdivoom_web_get_ctrl_handle = NULL;


void *divoom_web_get_main(void *param)
{
	while(1)
	{    
        divoom_system_get_sem(pdivoom_web_get_ctrl_handle->request_sem);
	    while(divoom_queue_not_empty(pdivoom_web_get_ctrl_handle->request_queue))
	    {
			Pdivoom_web_get_request_info request_info = NULL;
			divoom_web_get_msg_info msg_info = {0};
			int ret = 0,len;
            divoom_system_get_lock(pdivoom_web_get_ctrl_handle->lock);
	        request_info = (Pdivoom_web_get_request_info)divoom_queue_pop(pdivoom_web_get_ctrl_handle->request_queue);
            pdivoom_web_get_ctrl_handle->pcur_use = request_info;
            divoom_system_free_lock(pdivoom_web_get_ctrl_handle->lock);
	        if(request_info == NULL)
	        {
	            break;
	        }
			switch(request_info->mode)
			{
				case DIVOOM_WEB_GET_TYPE_TEXT_BUF:
					msg_info.data = (void*)divoom_http_client_get_srv(request_info->url);
					if(msg_info.data)
					{
					    msg_info.len = strlen((char *)msg_info.data)+1;
						ret = 1;
					}
                    else{
                        msg_info.len  = 0;
                    }
					break;
#if !DIVOOM_BUILD_SIMULATOR
                case DIVOOM_WEB_GET_TYPE_GZIP_TEXT_BUF:
                    ret = divoom_web_get_gzip_text(request_info->url, &msg_info);
                    break;
#endif
				case DIVOOM_WEB_GET_TYPE_DIVOOM_SERVER_FILE_BUF:				
					msg_info.data = (void *)divoom_http_client_get_file(request_info->file_id,&len, NULL, 0, NULL, NULL);
					msg_info.len = len;
					if(msg_info.data)
						ret = 1;
					break;
				case DIVOOM_WEB_GET_TYPE_DOWN_TO_FILE:
				{
					ret = divoom_http_client_down_file(request_info->file_id,request_info->url,0, NULL);
                    if(ret  < 0)
                    {
                        divoom_print("it fails to download file: %s\n", request_info->url);
                        ret = 0;
                    }

				}
					break;
                #if 0
                case DIVOOM_WEB_GET_TYPE_DIVOOM_PIC_SERVER_DOWN_TO_FILE:
                    msg_info.data = divoom_format_srv_gif_file_to_buf(request_info->url, &len);
                    if(msg_info.data)
                    {
                        ret  =1;
                        divoom_app_com_write_file(request_info->file_id, msg_info.data, len);
                    }
                    break;
                #endif
                
				case DIVOOM_WEB_GET_TYPE_DIVOOM_SERVER_DOWN_TO_FILE:
					ret = divoom_http_client_down_file(request_info->file_id,request_info->url,1, NULL);
                    if(ret < 0)
                    {
                        divoom_print("it fails to download file: %s\n", request_info->url);
                        ret = 0;
                    }
					break;
				default:
					break;
			}
            divoom_system_get_lock(pdivoom_web_get_ctrl_handle->lock);
			if(request_info->pcallback)
			{
				if(ret)
	            	request_info->pcallback(&msg_info, request_info->para);	//成功返回数据
				else
					request_info->pcallback(NULL, request_info->para);		//失败了返回空
			}
            pdivoom_web_get_ctrl_handle->pcur_use = NULL;
            divoom_system_free_lock(pdivoom_web_get_ctrl_handle->lock);
			if(msg_info.data)
				divoom_system_free(msg_info.data);
            divoom_system_free(request_info);
		    divoom_system_delay_ms(10);
	    }
	}

    return  NULL;
}
int divoom_web_get_clear_data_compare(void *item1, void *item2)
{    
    Pdivoom_web_get_request_info puse_info = (Pdivoom_web_get_request_info)item2;

    return puse_info->para != item1;
}

int divoom_web_get_clear_data(void *para)
{
    int ret = 0;
    Pdivoom_web_get_request_info puse_info;
    
    divoom_system_get_lock(pdivoom_web_get_ctrl_handle->lock);
    if(pdivoom_web_get_ctrl_handle->pcur_use && pdivoom_web_get_ctrl_handle->pcur_use->para == para)
    {
        pdivoom_web_get_ctrl_handle->pcur_use->pcallback = NULL;
        ret = 1;
    }
    while(1)
    {
        puse_info = divoom_queue_pop_cmp(pdivoom_web_get_ctrl_handle->request_queue, para, divoom_web_get_clear_data_compare);
        if(puse_info == NULL)
        {
            ret = 1;
            break;
        }
        divoom_system_free(puse_info);
    }
    divoom_system_free_lock(pdivoom_web_get_ctrl_handle->lock);
    return ret;
}

/*****************************************************************************
 Prototype    : divoom_web_get_push
 Description  : 将下载需求加入列表
 Input        : Pdivoom_web_get_request_info pinfo	
				获取字符串buf及文件buf时pinfo->pcallback必须不为空
 Output       : None
 Return Value : 返回0：入列表失败，需要free掉pinfo, 1:入列成功
 
  History        :
  1.Date         : 2020/8/28
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
int divoom_web_get_push(Pdivoom_web_get_request_info pinfo)
{
    Pdivoom_web_get_request_info puse_info;
	if(NULL == pinfo || NULL == pdivoom_web_get_ctrl_handle)
	{
		return 0;
	}
    puse_info = divoom_system_malloc(sizeof(divoom_web_get_request_info));
    if(puse_info == NULL)
    {
        return 0;
    }
    memcpy(puse_info, pinfo, sizeof(divoom_web_get_request_info));
	switch(puse_info->mode)
	{
		case DIVOOM_WEB_GET_TYPE_TEXT_BUF:
        case DIVOOM_WEB_GET_TYPE_GZIP_TEXT_BUF:
		case DIVOOM_WEB_GET_TYPE_DIVOOM_SERVER_FILE_BUF:				
			if(puse_info->pcallback == NULL)	//没有callback不处理
			{
                divoom_system_free(puse_info);
				return 0;
			}
			break;
//		case DIVOOM_WEB_GET_TYPE_DOWN_TO_FILE:
//		case DIVOOM_WEB_GET_TYPE_DIVOOM_SERVER_DOWN_TO_FILE:
//			break;
		default:
			break;
	}
    if (!divoom_queue_push(pdivoom_web_get_ctrl_handle->request_queue, puse_info)) {
        divoom_system_free(puse_info);
        return 0;
    }
    divoom_system_put_sem(pdivoom_web_get_ctrl_handle->request_sem);
    return 1;
}

void divoom_web_get_init(void)
{
	if(pdivoom_web_get_ctrl_handle)
		return;
	pdivoom_web_get_ctrl_handle = (Pdivoom_web_get_ctrl)divoom_system_malloc(sizeof(divoom_web_get_ctrl));
	if(NULL == pdivoom_web_get_ctrl_handle)
	{
		divoom_print("malloc fail\n");
	}
	pdivoom_web_get_ctrl_handle->request_queue = divoom_queue_init(DIVOOM_WEB_GET_QUEUE_MAX_NUM, __func__, __LINE__);
    pdivoom_web_get_ctrl_handle->request_sem = divoom_system_create_sem(1, 0);
    pdivoom_web_get_ctrl_handle->lock = divoom_system_create_lock();
	divoom_system_create_task(divoom_web_get_main, "web_get", NULL, DIVOOM_TASK_STACK_WEB_GET);
//	xTaskCreatePinnedToCore(divoom_web_get_main, "divoom web get", 6*1024, NULL, 2, NULL, 1);
}

