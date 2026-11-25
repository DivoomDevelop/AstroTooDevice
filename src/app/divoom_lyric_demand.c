/*
 * 联网歌词的界面需求管理：汇总当前可见表盘及其子表盘是否需要歌词。
 * 本模块只维护需求状态并通知蓝牙取词入口重新调度，不直接发送网络请求。
 *
 * 使用顺序：
 * 1. 每个主/子表盘显示实例持有一个清零的 source，配置加载或更新后调用
 *    set_source。专用歌词表盘或包含歌词元素时 needed=1，暂停时置 0。
 *    子表盘使用所属主界面的 menu_id；同一可见菜单任一节点需要歌词即生效。
 * 2. 界面显示后调用 set_visible(owner, menu_id, 1)，离开或暂停时传 0。
 *    owner 是界面实例标识，用于防止旧界面的退出通知关闭新界面的需求。
 * 3. 显示实例关闭时先 remove_source，再按原生命周期等待使用者退出并释放。
 *    临时暂停只设置 needed=0；remove_source 会永久关闭该节点。
 * 4. 歌词任务调度时保存 snapshot() 返回的需求代次，0 表示无需联网取词。
 *    每次实际发送前及处理响应前调用 valid(保存的代次)，失效则停止后续请求，
 *    丢弃过期结果。退出后快速重进会产生新代次，旧任务不会重新生效。
 *
 * 网络侧只读取代次，不访问表盘对象；代次不替代歌曲代次、区域及串口优先检查，
 * 也不重置每首歌的请求预算。检查无法撤回已经发出的网络请求。
 */
#include "divoom.h"
#include "divoom_port_os.h"
#include "divoom_lyric_demand.h"
#include "divoom_lyric_region.h"

static divoom_lyric_demand_source *s_lyric_sources;
static void *s_lyric_visible_owner;
static int s_lyric_visible_menu;
static int s_lyric_needed;
static unsigned int s_lyric_generation = 1;

/* 临界区内只更新整数和节点，不分配内存、不打印、不获取其它锁。 */
static int lyric_demand_update_locked(int owner_changed)
{
    divoom_lyric_demand_source *source;
    int needed = 0;
    if(s_lyric_visible_owner)
    {
        for(source = s_lyric_sources; source; source = source->next)
        {
            if(source->needed && source->menu_id == s_lyric_visible_menu)
            {
                needed = 1;
                break;
            }
        }
    }
    if(needed == s_lyric_needed && !(needed && owner_changed))
    {
        return 0;
    }
    s_lyric_needed = needed;
    if(++s_lyric_generation == 0)
    {
        s_lyric_generation = 1;
    }
    return 1;
}

/* 临界区外通知，允许回调读取需求并调度任务，避免持锁执行后续处理。 */
static void lyric_demand_notify(int changed)
{
    if(changed)
    {
        unsigned int generation = divoom_lyric_demand_snapshot();
        divoom_print("[lyric_demand] needed=%d generation=%u\n", generation != 0, generation);
        divoom_bt_music_lyric_demand_changed();
    }
}

/* 返回当前有效需求代次；调用者保存此值，后续不能用新快照替换旧任务的代次。 */
unsigned int divoom_lyric_demand_snapshot(void)
{
    unsigned int generation;
    divoom_port_critical_enter();
    generation = s_lyric_needed ? s_lyric_generation : 0;
    divoom_port_critical_exit();
    return generation;
}

/* 仅当仍需歌词且需求代次未变时有效，用于取消排队任务及拒绝过期响应。 */
int divoom_lyric_demand_valid(unsigned int generation)
{
    return generation != 0 && generation == divoom_lyric_demand_snapshot();
}

/* 首次调用注册节点，后续更新需求；已关闭节点忽略迟到的更新。 */
void divoom_lyric_demand_set_source(divoom_lyric_demand_source *source, int menu_id, int needed)
{
    int changed = 0;
    divoom_port_critical_enter();
    if(!source->closed)
    {
        if(!source->registered)
        {
            source->next = s_lyric_sources;
            s_lyric_sources = source;
            source->registered = 1;
        }
        source->menu_id = menu_id;
        source->needed = needed != 0;
        changed = lyric_demand_update_locked(0);
    }
    divoom_port_critical_exit();
    lyric_demand_notify(changed);
}

/* 关闭并摘除节点；不释放内存，也不保证其它调用者已经退出。 */
void divoom_lyric_demand_remove_source(divoom_lyric_demand_source *source)
{
    divoom_lyric_demand_source **link;
    int changed;
    divoom_port_critical_enter();
    source->closed = 1;
    for(link = &s_lyric_sources; *link; link = &(*link)->next)
    {
        if(*link == source)
        {
            *link = source->next;
            break;
        }
    }
    source->registered = 0;
    source->next = NULL;
    changed = lyric_demand_update_locked(0);
    divoom_port_critical_exit();
    lyric_demand_notify(changed);
}

/* 只汇总当前可见菜单的节点；隐藏通知只对当前 owner 生效。 */
void divoom_lyric_demand_set_visible(void *owner, int menu_id, int visible)
{
    int changed = 0;
    int owner_changed;
    divoom_port_critical_enter();
    owner_changed = s_lyric_visible_owner != owner || s_lyric_visible_menu != menu_id;
    if(visible)
    {
        s_lyric_visible_owner = owner;
        s_lyric_visible_menu = menu_id;
        changed = lyric_demand_update_locked(owner_changed);
    }
    else if(s_lyric_visible_owner == owner)
    {
        s_lyric_visible_owner = NULL;
        changed = lyric_demand_update_locked(0);
    }
    divoom_port_critical_exit();
    lyric_demand_notify(changed);
}

/* Invalidate old demand before notifying the scheduler outside the critical section. */
void divoom_lyric_demand_region_changed(void)
{
    divoom_port_critical_enter();
    if (++s_lyric_generation == 0) s_lyric_generation = 1;
    divoom_port_critical_exit();
    divoom_bt_music_lyric_region_changed();
}
