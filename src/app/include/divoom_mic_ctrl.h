#ifndef _divoom_mic_ctrl_h_
#define _divoom_mic_ctrl_h_


enum
{
    DIVOOM_MIC_CTRL_RAW_TYPE,
    DIVOOM_MIC_CTRL_AMR_TYPE,
    DIVOOM_MIC_CTRL_NULL_TYPE,
};


typedef void DIVOOM_MIC_CTRL_SEND_DATA(uint8 *data, uint16 len);


/*********************************************************************
  Function:        divoom_mic_ctrl_init
  Description:    初始化相关信息
  Input:            NONE
  Return:           none
  Author:           albert
**********************************************************************/

void divoom_mic_ctrl_init(void);

/*********************************************************************
  Function:        divoom_mic_ctrl_request
  Description:    增加一路MIC数据的请求，如果为编码方式，将停止向其它非编码通路送数据
  Input:            pcallback    数据输出回调函数
                       encode_flag    是否需要编码
  Return:           none
  Author:           albert
**********************************************************************/

void divoom_mic_ctrl_request(DIVOOM_MIC_CTRL_SEND_DATA pcallback);

/*********************************************************************
  Function:        divoom_mic_ctrl_release
  Description:    取消之前申请的MIC数据通路
  Input:            pcallback    数据输出回调函数
                       
  Return:           none
  Author:           albert
**********************************************************************/

void divoom_mic_ctrl_release(void);

/*********************************************************************
  Function:        divoom_mic_ctrl_exit
  Description:    退出相关模块
  Input:            pcallback    数据输出回调函数
                       
  Return:           none
  Author:           albert
**********************************************************************/

void divoom_mic_ctrl_exit(void);


#endif


