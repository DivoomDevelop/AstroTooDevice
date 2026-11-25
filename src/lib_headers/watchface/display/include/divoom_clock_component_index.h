#ifndef _divoom_clock_component_index_h_
#define _divoom_clock_component_index_h_

#include "divoom_type.h"
#include "divoom_disp_clock_ai_component.h"

/*
 * 表盘组件索引模块：把“主表盘 / 主表盘+位置+子表盘”关系及调度属性持久化，
 * 网络配置变更后只比较并替换对应主表盘片段，显示层只读取索引文件。
 */

/*
  Function:         divoom_clock_component_index_init
  Description:      初始化索引模块的串行化锁；LIGHT_APP 初始化完成后调用一次
  Return:           1=成功，0=创建锁失败
*/
int divoom_clock_component_index_init(void);

/*
  Function:         divoom_clock_component_index_refresh
  Description:      只生成 clock_id 对应的主/子表盘片段；先比较，变化后才替换
  Input:            clock_id 主表盘 ID
  Return:           1=文件已更新，0=内容未变化，-1=生成或写入失败
*/
int divoom_clock_component_index_refresh(int32 clock_id);

/*
  Function:         divoom_clock_component_index_remove_clock
  Description:      只删除 clock_id 的索引片段；片段不存在时不写盘
  Input:            clock_id 主表盘 ID
  Return:           1=文件已更新，0=内容未变化，-1=读写失败
*/
int divoom_clock_component_index_remove_clock(int32 clock_id);

/*
  Function:         divoom_clock_component_index_bootstrap
  Description:      仅在索引缺失或损坏时扫描“我的表盘”建立首份快照
  Return:           1=文件已重建，0=原索引有效，-1=重建失败
*/
int divoom_clock_component_index_bootstrap(void);

/*
  Function:         divoom_clock_component_index_load
  Description:      从索引文件加载完整组件列表；文件缺失、版本不符或损坏时自动重建
  Output:           component_list 由调用方使用 divoom_system_free 释放；component_num 为记录数
  Return:           记录数，失败返回 -1
*/
int divoom_clock_component_index_load(
    Pdivoom_disp_clock_main_compent_info *component_list, int *component_num);

#endif
