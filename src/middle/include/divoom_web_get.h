#ifndef __divoom_web_get_h
#define __divoom_web_get_h



typedef enum{
	//获取字符串BUF
	DIVOOM_WEB_GET_TYPE_TEXT_BUF,
	//获取DIVOOM服务器文件BUF
	DIVOOM_WEB_GET_TYPE_DIVOOM_SERVER_FILE_BUF,	
	//下载DIVOOM服务器文件到本地指定位置	
	DIVOOM_WEB_GET_TYPE_DIVOOM_SERVER_DOWN_TO_FILE,	
	//下载非DIVOOM服务器文件到本地指定位置
	DIVOOM_WEB_GET_TYPE_DOWN_TO_FILE,	
	//下载DIVOOM服务器图片文件打包后到本地指定位置	
	DIVOOM_WEB_GET_TYPE_DIVOOM_PIC_SERVER_DOWN_TO_FILE,	
	DIVOOM_WEB_GET_TYPE_GZIP_TEXT_BUF,
}DIVOOM_WEB_GET_TYPE;



//得到的数据
typedef struct __divoom_web_get_msg_info
{
	uint32 len;
	void *data;
}divoom_web_get_msg_info,*Pdivoom_web_get_msg_info;

/*
网络文件获取的CALLBACK
1.当pmsg为空时表示失败下载
2.当pmsg不为空
	获取字符串buf时，pmsg->data

	获取字符串buf时，pmsg->data,pmsg->len
	
	当下载文件时，pmsg不含值，只是通知下载成功
*/
typedef void (*DIVOOM_WEBGET_MODEL_CALLBACK)(Pdivoom_web_get_msg_info pmsg, void *para);

//获取数据的请求信息，会被压入队列中待处理
typedef struct __divoom_web_get_request_info
{
    void *para;
    DIVOOM_WEB_GET_TYPE mode;
    DIVOOM_WEBGET_MODEL_CALLBACK pcallback;
	char file_id[128];
	char url[512];
}divoom_web_get_request_info,*Pdivoom_web_get_request_info;


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
int divoom_web_get_push(Pdivoom_web_get_request_info pinfo);

void divoom_web_get_init(void);

int divoom_web_get_clear_data(void *para);

#endif

