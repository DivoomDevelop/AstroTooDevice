#ifndef DIVOOM_LYRIC_DEMAND_H
#define DIVOOM_LYRIC_DEMAND_H

/*
 * 界面需求节点：由主/子表盘显示实例持有，首次注册前必须整体清零。
 * 注册期间节点地址必须稳定，不能使用短期栈变量；字段由本模块维护。
 * 子表盘与所属主界面使用相同 menu_id，任一有效节点需要歌词即可启用需求。
 * 释放实例前必须注销并确保无其它使用者；注销后不可再次注册同一生命周期节点。
 * 网络任务只读需求代次，不持有节点或表盘对象。完整调用顺序见 .c 文件头部。
 */
typedef struct divoom_lyric_demand_source
{
    struct divoom_lyric_demand_source *next;
    int menu_id;
    int needed;
    int registered;
    int closed;
} divoom_lyric_demand_source;

/* 任务调度时保存：0=当前无需歌词，非 0=本次界面需求代次。 */
unsigned int divoom_lyric_demand_snapshot(void);
/* 实际发送及处理响应前校验保存的代次，返回 0 时取消该任务后续处理。 */
int divoom_lyric_demand_valid(unsigned int generation);
/* 配置加载/更新、暂停/恢复时注册或更新节点；source 必须非空。 */
void divoom_lyric_demand_set_source(divoom_lyric_demand_source *source, int menu_id, int needed);
/* 实例关闭时永久注销，source 必须非空；临时暂停请用 set_source(..., 0)。 */
void divoom_lyric_demand_remove_source(divoom_lyric_demand_source *source);
/* 界面显示/离开时调用，owner 使用非空、稳定的界面实例标识。 */
void divoom_lyric_demand_set_visible(void *owner, int menu_id, int visible);
/* 蓝牙模块实现的需求变化回调：重新检查当前歌曲是否需要调度取词。 */
void divoom_bt_music_lyric_demand_changed(void);
void divoom_lyric_demand_region_changed(void);

#endif
