#ifndef _divoom_clock_control_h_
#define _divoom_clock_control_h_

#define DIVOOM_CLOCK_CONTROL_MAX_DISP_NUM     128
#define DIVOOM_CLOCK_CONTROL_MAX_URL_HEAD_NUM     20
#define DIVOOM_CLOCK_CONTROL_MAX_DISP_SUB_NUM     40
#define DIVOOM_CLOCK_CONTROL_HISOTYR_DAYS  79
#define DIVOOM_CLOCK_CONTROL_DISPLAY_MAX_LEN   128
#define DIVOOM_CLOCK_CONTROL_IEX_TIME_ZONE "EST5EDT,M3.2.0,M11.1.0"


typedef struct _divoom_clock_control_disp_array_item
{
    uint8   num;       //pdisp_data ,pdisp_name的有效个数
    int     image_id;        //APP显示的图片,如果小于DIVOOM_MANAGE_IMAGE_ID_BASE_APP_PIC这个是本地的ＩＤ,否则使用应用ＩＤ
    int     image_id2;        //APP显示的图片,如果小于DIVOOM_MANAGE_IMAGE_ID_BASE_APP_PIC这个是本地的ＩＤ,否则使用应用ＩＤ
    int     image_id3;        //APP显示的图片,如果小于DIVOOM_MANAGE_IMAGE_ID_BASE_APP_PIC这个是本地的ＩＤ,否则使用应用ＩＤ
    int     image_id4;        //APP显示的图片,如果小于DIVOOM_MANAGE_IMAGE_ID_BASE_APP_PIC这个是本地的ＩＤ,否则使用应用ＩＤ
	int 	image_url_id; 	//外部url地址获取的图片
    uint8   attr_flag;      //用来储存股票上涨还是下跌的,0:上涨，1:下跌
    uint8 	bg_color[3]; //针对google 日历
	uint8 	pdisp_data_roll_flag[DIVOOM_CLOCK_CONTROL_MAX_DISP_SUB_NUM]; //数据滚动标志
	char    app_name[DIVOOM_CLOCK_CONTROL_DISPLAY_MAX_LEN*2]; //网络项的名字                DIVOOM_CLOCK_DISP_SUPPORT_APP_TITLE
    char    pdisp_name[DIVOOM_CLOCK_CONTROL_MAX_DISP_SUB_NUM][DIVOOM_CLOCK_CONTROL_DISPLAY_MAX_LEN]; //网络项的名字            DIVOOM_CLOCK_DISP_SUPPORT_APP_ITEM_NAMEX
    char    pdisp_data[DIVOOM_CLOCK_CONTROL_MAX_DISP_SUB_NUM][DIVOOM_CLOCK_CONTROL_DISPLAY_MAX_LEN]; //网络项的显示数据           DIVOOM_CLOCK_DISP_SUPPORT_APP_ITEM_DATAx
    float   open_value[DIVOOM_CLOCK_CONTROL_HISOTYR_DAYS];
    float   close_value[DIVOOM_CLOCK_CONTROL_HISOTYR_DAYS];
	float	max_price;
	float	min_price;
	int 	hour_value[DIVOOM_CLOCK_CONTROL_HISOTYR_DAYS];
	float	price_value[DIVOOM_CLOCK_CONTROL_HISOTYR_DAYS];
}divoom_clock_control_disp_array_item, *Pdivoom_clock_control_disp_array_item;

typedef struct _divoom_clock_control_disp_data
{
    uint8 num;
    uint8 update;        //数据是否更新
    Pdivoom_clock_control_disp_array_item pdisp_list[DIVOOM_CLOCK_CONTROL_MAX_DISP_NUM];       //显示列表，按需申请，降低常驻内存
}divoom_clock_control_disp_data, *Pdivoom_clock_control_disp_data;

typedef struct _divoom_clock_control_rule_data
{
    char prule_data[64];
}divoom_clock_control_rule_data, *Pdivoom_clock_control_rule_data;

typedef void DIVOOM_CLOCK_CONTROL_SEND_DATA_CALLBACK(void *para,Pdivoom_clock_id_info clock_info, Pdivoom_clock_control_disp_data pdisp_data);

/* 判断名称是否为比特币（BTC / BTC-USD / BTCUSD / bitcoin，忽略大小写） */
int divoom_clock_control_is_btc_symbol(const char *name);
void divoom_clock_control_disp_data_init(Pdivoom_clock_control_disp_data pdisp_data);
void divoom_clock_control_disp_data_deinit(Pdivoom_clock_control_disp_data pdisp_data);
Pdivoom_clock_control_disp_array_item divoom_clock_control_disp_data_get_item(Pdivoom_clock_control_disp_data pdisp_data, int index);
void divoom_clock_control_disp_data_clear_item(Pdivoom_clock_control_disp_data pdisp_data, int index);
void divoom_clock_control_disp_data_reset_item(Pdivoom_clock_control_disp_data pdisp_data);
int divoom_clock_control_disp_data_copy(Pdivoom_clock_control_disp_data pdst, Pdivoom_clock_control_disp_data psrc);

#define DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(clock_disp_data_ptr, clock_disp_data_index) (*divoom_clock_control_disp_data_get_item((clock_disp_data_ptr), (clock_disp_data_index)))


/*********************************************************************
  Function:         divoom_clock_control_init
  Description:      启动表盘获取网络数据
  Input:            pclock_info
                    clock_num
                    need_net_request  是否需要网络请求
  Return:           none
  Author:           albert
**********************************************************************/

void *divoom_clock_control_init(Pdivoom_clock_id_info clock_info, Pdivoom_clock_attr_info pclock_info, int clock_num, DIVOOM_CLOCK_CONTROL_SEND_DATA_CALLBACK *pcallback, int *need_net_request, void *para);



/*********************************************************************
  Function:         divoom_clock_control_exit
  Description:      退出表盘获取网络数据模块
  Input:            none
  Return:           none
  Author:           albert
**********************************************************************/


void divoom_clock_control_exit(void *handle);

/*********************************************************************
  Function:         divoom_clock_control_exit_notice
  Description:      退出表盘通知
  Input:            none
  Return:           none
  Author:           albert
**********************************************************************/


void divoom_clock_control_exit_notice(void *handle);

/*********************************************************************
  Function:         divoom_clock_control_request_quick
  Description:      快速激活一次请求
  Input:            none
  Return:           none
  Author:           albert
**********************************************************************/


void divoom_clock_control_request_quick(Pdivoom_clock_id_info clock_id);
int divoom_clock_control_url_same(void *handle, const char *url_info, const char *url_config, int app_type);

/*********************************************************************
  Function:        divoom_clock_control_do_with_data
  Description:     通过规则串，解析出所要的数据
  Input:            root        
                    rule_string      规则字符串
                    out_str          如果是字符串，将输出这个，返回1有效
                    out_int          如果是数字将输出这个,返回0有效
  Return:           1：输出数字；2：输出的字符串;0:失败
  Author:           albert
**********************************************************************/

uint8 divoom_clock_control_do_with_data(divoom_cJSON *root,char *rule_string, char *out_str, long long *out_int);
void divoom_disp_clock_get_update_disp_data(void *para, Pdivoom_clock_id_info clock_info, Pdivoom_clock_control_disp_data pdisp_data);

/*********************************************************************
  Function:         divoom_clock_control_update_pc_data
  Description:      通过本地网域发过来的表盘显示数据
  Input:            ppc_clock_data   网络数据
  Return:           none
  Author:           albert
**********************************************************************/
void divoom_clock_control_update_pc_data(Pdivoom_device_pc_update_clock_para ppc_clock_data);

/*********************************************************************
  Function:         divoom_clock_control_global_init
  Description:      初始化全局管理句柄
  Input:            none
  Return:           none
  Author:           albert
**********************************************************************/

void divoom_clock_control_global_init(void);

/*********************************************************************
  Function:         divoom_clock_control_update
  Description:      更新表盘获取网络数据属性
  Input:            pclock_info
                    clock_num
                    need_net_request  是否需要网络请求
  Return:           none
  Author:           albert
**********************************************************************/

int divoom_clock_control_update(void *handle, Pdivoom_clock_id_info clock_info, Pdivoom_clock_attr_info pclock_info, int clock_num, int *need_net_request);

/*********************************************************************
  Function:         divoom_clock_control_check_exit_flag
  Description:      检查是否能出标记
  Input:            handle
  Return:           1：退出，0：正常
  Author:           albert
**********************************************************************/

int divoom_clock_control_check_exit_flag(void *handle);

#endif


