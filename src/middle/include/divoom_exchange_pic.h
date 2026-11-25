#ifndef _divoom_exchange_pic_h_
#define _divoom_exchange_pic_h_

/*********************************************************************
  Function:        divoom_exchange_pic_init
  Description:     模块初始化
  Input:           
                    
  Return:          none
**********************************************************************/


void divoom_exchange_pic_init(void);


/*********************************************************************
  Function:        divoom_exchange_pic_get_id
  Description:     得到汇率图片id
  Input:           exchage_flag  汇率标记
                    
  Return:          图片ＩＤ
**********************************************************************/

int divoom_exchange_pic_get_id(char *exchage_flag);

/*********************************************************************
  Function:        divoom_contry_pic_get_id
  Description:     得到国家图片id
  Input:           contry_flag  国家三个缩写标记
                    
  Return:          图片ＩＤ
**********************************************************************/

int divoom_contry_pic_get_id(char *contry_flag);


/*********************************************************************
  Function:        divoom_exchange_pic_update_list
  Description:     更新汇率列表
  Input:           plist
                   num
                    
  Return:          NONE
**********************************************************************/

void divoom_exchange_pic_update_list(Pdivoom_exchange_rate_pic_item plist, int num);


void divoom_exchange_pic_get_all_list(void);

void divoom_exchange_get_currency_name(char *exchage_flag, char *currency_name);

#endif


