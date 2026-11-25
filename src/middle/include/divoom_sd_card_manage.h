#ifndef _DIVOOM_SD_CARD_MANAGE_H_
#define _DIVOOM_SD_CARD_MANAGE_H_

#include "divoom_type.h"

/*********************************************************************
 * SD卡状态枚举
 *********************************************************************/
typedef enum {
    DIVOOM_SD_CARD_STATE_NONE = 0,      // 未初始化
    DIVOOM_SD_CARD_STATE_INSERTED,      // SD卡已插入
    DIVOOM_SD_CARD_STATE_REMOVED,       // SD卡已拔出
    DIVOOM_SD_CARD_STATE_MOUNTED,       // SD卡已挂载
    DIVOOM_SD_CARD_STATE_UNMOUNTED,     // SD卡已卸载
    DIVOOM_SD_CARD_STATE_MOUNT_FAIL,    // SD卡挂载失败
} DIVOOM_SD_CARD_STATE;

/*********************************************************************
 * SD卡挂载状态回调函数类型
 * status: DIVOOM_SD_CARD_STATE 状态
 *********************************************************************/
typedef void (*DIVOOM_SD_CARD_CALLBACK)(DIVOOM_SD_CARD_STATE status);

/*********************************************************************
  Function:        divoom_sd_card_manage_init
  Description:     初始化SD卡管理模块，启动插拔检测线程
  Input:           callback - 状态变化回调函数
  Return:          1: 成功  0: 失败
**********************************************************************/
int divoom_sd_card_manage_init(DIVOOM_SD_CARD_CALLBACK callback);

/*********************************************************************
  Function:        divoom_sd_card_manage_deinit
  Description:     反初始化SD卡管理模块
  Input:           none
  Return:          none
**********************************************************************/
void divoom_sd_card_manage_deinit(void);

/*********************************************************************
  Function:        divoom_sd_card_manage_get_state
  Description:     获取当前SD卡状态
  Input:           none
  Return:          DIVOOM_SD_CARD_STATE
**********************************************************************/
DIVOOM_SD_CARD_STATE divoom_sd_card_manage_get_state(void);

/*********************************************************************
  Function:        divoom_sd_card_manage_is_mounted
  Description:     检查SD卡是否已挂载
  Input:           none
  Return:          1: 已挂载  0: 未挂载
**********************************************************************/
int divoom_sd_card_manage_is_mounted(void);

/*********************************************************************
  Function:        divoom_sd_card_manage_mount
  Description:     挂载SD卡
  Input:           none
  Return:          1: 成功  0: 失败
**********************************************************************/
int divoom_sd_card_manage_mount(void);

/*********************************************************************
  Function:        divoom_sd_card_manage_unmount
  Description:     卸载SD卡
  Input:           none
  Return:          1: 成功  0: 失败
**********************************************************************/
int divoom_sd_card_manage_unmount(void);

/*********************************************************************
  Function:        divoom_sd_card_manage_scan_music
  Description:     扫描SD卡中的音乐文件并添加到数据库
  Input:           none
  Return:          扫描到的音乐文件数量
**********************************************************************/
int divoom_sd_card_manage_scan_music(void);

/*********************************************************************
  Function:        divoom_sd_card_manage_get_mount_path
  Description:     获取SD卡挂载路径
  Input:           none
  Return:          挂载路径字符串
**********************************************************************/
const char* divoom_sd_card_manage_get_mount_path(void);

#endif /* _DIVOOM_SD_CARD_MANAGE_H_ */
