#include "divoom.h"
#include "divoom_sd_card_manage.h"
#include "divoom_tf_music.h"
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*********************************************************************
 * 宏定义
 *********************************************************************/
// SD卡设备节点 (T113平台TF卡一般是mmcblk1)
#define DIVOOM_SD_CARD_DEVICE           "/dev/mmcblk1p1"
#define DIVOOM_SD_CARD_DEVICE_BLOCK     "/dev/mmcblk1"

// SD卡挂载点
#define DIVOOM_SD_CARD_MOUNT_PATH       "/mnt/SDCARD"
#define DIVOOM_SD_CARD_MUSIC_PATH       "/mnt/SDCARD"

// 检测间隔时间 (毫秒)
#define DIVOOM_SD_CARD_CHECK_INTERVAL   1000

// 最大路径长度
#define DIVOOM_SD_CARD_MAX_PATH         512

// 支持的音乐文件扩展名
static const char* g_music_ext_list[] = {
    "mp3", "MP3",
    "flac", "FLAC",
    "ogg", "OGG",
    "wav", "WAV",
    "aac", "AAC",
    "wma", "WMA",
    NULL
};

/*********************************************************************
 * 模块私有数据结构
 *********************************************************************/
typedef struct _divoom_sd_card_manage_ctrl
{
    DIVOOM_SD_CARD_STATE state;         // 当前状态
    DIVOOM_SD_CARD_CALLBACK callback;   // 状态回调
    void *task_handle;                  // 检测任务句柄
    void *lock;                         // 互斥锁
    int running;                        // 运行标志
    int music_count;                    // 扫描到的音乐数量
} divoom_sd_card_manage_ctrl, *Pdivoom_sd_card_manage_ctrl;

static Pdivoom_sd_card_manage_ctrl g_sd_card_handle = NULL;

/*********************************************************************
 * 内部函数声明
 *********************************************************************/
static int divoom_sd_card_is_inserted(void);
static int divoom_sd_card_do_mount(void);
static int divoom_sd_card_do_unmount(void);
static void divoom_sd_card_scan_dir(const char *dir_path);
static int divoom_sd_card_is_music_file(const char *file_name);
static void *divoom_sd_card_detect_task(void *param);

/*********************************************************************
  Function:        divoom_sd_card_is_inserted
  Description:     检测SD卡是否插入
  Input:           none
  Return:          1: 已插入  0: 未插入
**********************************************************************/
static int divoom_sd_card_is_inserted(void)
{
    struct stat st;
    
    // 方法1: 检查分区设备节点是否存在
    if (divoom_port_file_exists(DIVOOM_SD_CARD_DEVICE))
    {
        return 1;
    }
    
    // 方法2: 检查块设备是否存在
    if (stat(DIVOOM_SD_CARD_DEVICE_BLOCK, &st) == 0 && S_ISBLK(st.st_mode))
    {
        return 1;
    }
    
    // 方法3: 检查 /sys/block 中是否存在对应设备 (更可靠)
    // TF卡通常是 mmcblk1，内部emmc是 mmcblk0
    if (divoom_port_file_exists("/sys/block/mmcblk1"))
    {
        return 1;
    }
    
    return 0;
}

/*********************************************************************
  Function:        divoom_sd_card_do_mount
  Description:     执行SD卡挂载
  Input:           none
  Return:          1: 成功  0: 失败
**********************************************************************/
/* 检查指定设备是否已挂载到指定路径（如开机已被 udev/init 挂载则不再重复 mount，避免 EBUSY） */
static int divoom_sd_card_is_mounted_at(const char *device, const char *mount_path)
{
    FILE *fp;
    char line[512];
    char dev[128], path[256], rest[128];
    if (!device || !mount_path) return 0;
    fp = divoom_port_file_open("/proc/mounts", "r");
    if (!fp) return 0;
    while (fgets(line, sizeof(line), fp))
    {
        if (sscanf(line, "%127s %255s %127s", dev, path, rest) >= 2)
        {
            if (strcmp(dev, device) == 0 && strcmp(path, mount_path) == 0)
            {
                divoom_port_file_close(fp);
                return 1;
            }
        }
    }
    divoom_port_file_close(fp);
    return 0;
}

static int divoom_sd_card_do_mount(void)
{
    int ret;
    struct stat st;
    
    /* 若已被系统/udev 挂载到目标路径，直接视为成功，避免重复 mount 报 Device or resource busy */
    if (divoom_sd_card_is_mounted_at(DIVOOM_SD_CARD_DEVICE, DIVOOM_SD_CARD_MOUNT_PATH))
    {
        divoom_print("SD card: already mounted at %s (skip mount)\n", DIVOOM_SD_CARD_MOUNT_PATH);
        return 1;
    }
    
    // 检查挂载点是否存在，不存在则创建
    if (stat(DIVOOM_SD_CARD_MOUNT_PATH, &st) != 0)
    {
        ret = divoom_port_mkdir_p(DIVOOM_SD_CARD_MOUNT_PATH);
        if (ret != 0 && errno != EEXIST)
        {
            divoom_print("SD card: create mount point failed: %s\n", strerror(errno));
            return 0;
        }
    }
    
    // 尝试挂载：先 vfat（不加 iocharset=utf8，嵌入式内核常无此 charset），再 exfat/ext4
    ret = mount(DIVOOM_SD_CARD_DEVICE, DIVOOM_SD_CARD_MOUNT_PATH, "vfat",
                MS_NOATIME | MS_NODIRATIME, NULL);
    if (ret != 0)
    {
        ret = mount(DIVOOM_SD_CARD_DEVICE, DIVOOM_SD_CARD_MOUNT_PATH, "exfat",
                    MS_NOATIME | MS_NODIRATIME, NULL);
        if (ret != 0)
        {
            ret = mount(DIVOOM_SD_CARD_DEVICE, DIVOOM_SD_CARD_MOUNT_PATH, "ext4",
                        MS_NOATIME | MS_NODIRATIME, NULL);
            if (ret != 0)
            {
                divoom_print("SD card: mount failed: %s\n", strerror(errno));
                return 0;
            }
        }
    }
    divoom_print("SD card: mounted successfully at %s\n", DIVOOM_SD_CARD_MOUNT_PATH);
    return 1;
}

/*********************************************************************
  Function:        divoom_sd_card_do_unmount
  Description:     执行SD卡卸载
  Input:           none
  Return:          1: 成功  0: 失败
**********************************************************************/
static int divoom_sd_card_do_unmount(void)
{
    int ret;
    int device_gone = 0;
    
    /* 若设备节点已消失（用户已拔卡），不要 sync()，否则可能卡死或触发内核异常 */
    if (!divoom_port_file_exists(DIVOOM_SD_CARD_DEVICE_BLOCK))
        device_gone = 1;
    
    if (!device_gone)
    {
        sync();
        ret = umount(DIVOOM_SD_CARD_MOUNT_PATH);
    }
    else
    {
        /* 卡已拔：先尝试懒卸载，避免对已消失设备做 I/O */
        ret = umount2(DIVOOM_SD_CARD_MOUNT_PATH, MNT_DETACH);
    }
    
    if (ret != 0)
    {
        ret = umount2(DIVOOM_SD_CARD_MOUNT_PATH, MNT_FORCE);
        if (ret != 0)
        {
            divoom_print("SD card: unmount failed: %s\n", strerror(errno));
            return 0;
        }
    }
    
    divoom_print("SD card: unmounted successfully\n");
    return 1;
}

/*********************************************************************
  Function:        divoom_sd_card_is_music_file
  Description:     判断文件是否为音乐文件
  Input:           file_name - 文件名
  Return:          1: 是音乐文件  0: 不是音乐文件
**********************************************************************/
static int divoom_sd_card_is_music_file(const char *file_name)
{
    char ext_name[16] = {0};
    const char **ext_ptr;
    
    if (file_name == NULL)
    {
        return 0;
    }
    
    // 获取文件扩展名
    divoom_app_com_get_file_ex_name((char *)file_name, ext_name);
    
    // 检查是否为支持的音乐格式
    ext_ptr = g_music_ext_list;
    while (*ext_ptr != NULL)
    {
        if (strcasecmp(ext_name, *ext_ptr) == 0)
        {
            return 1;
        }
        ext_ptr++;
    }
    
    return 0;
}

/*********************************************************************
  Function:        divoom_sd_card_scan_dir
  Description:     递归扫描目录中的音乐文件
  Input:           dir_path - 目录路径
  Return:          none
**********************************************************************/
static void divoom_sd_card_scan_dir(const char *dir_path)
{
    DIR *dp;
    const divoom_port_dirent_t *dirp;
    char full_path[DIVOOM_SD_CARD_MAX_PATH];
    int path_len;
    
    if (g_sd_card_handle == NULL || dir_path == NULL)
    {
        return;
    }
    
    dp = divoom_port_dir_open(dir_path);
    if (dp == NULL)
    {
        divoom_print("SD card: cannot open directory: %s\n", dir_path);
        return;
    }
    
    path_len = strlen(dir_path);
    
    while ((dirp = divoom_port_dir_read(dp)) != NULL)
    {
        // 跳过 . 和 ..
        if (strcmp(dirp->name, ".") == 0 || strcmp(dirp->name, "..") == 0)
        {
            continue;
        }
        
        // 跳过隐藏文件和系统目录
        if (dirp->name[0] == '.')
        {
            continue;
        }
        
        // 构建完整路径
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, dirp->name);
        
        if (dirp->is_dir)
        {
            // 目录 - 递归扫描
            // 跳过一些系统目录
            if (strcmp(dirp->name, "LOST.DIR") == 0 ||
                strcmp(dirp->name, "System Volume Information") == 0 ||
                strcmp(dirp->name, "Android") == 0 ||
                strcmp(dirp->name, "$RECYCLE.BIN") == 0)
            {
                continue;
            }
            
            divoom_print("SD card: scanning directory: %s\n", full_path);
            divoom_sd_card_scan_dir(full_path);
        }
        else if (!dirp->is_dir)
        {
            // 普通文件 - 检查是否为音乐文件
            if (divoom_sd_card_is_music_file(dirp->name))
            {
                divoom_print("SD card: found music file: %s\n", full_path);
                
                // 使用 divoom_tf_music_add_music_item 添加到数据库
                divoom_tf_music_add_music_item(full_path);
                
                g_sd_card_handle->music_count++;
            }
        }
    }
    
    divoom_port_dir_close(dp);
}

/*********************************************************************
  Function:        divoom_sd_card_detect_task
  Description:     SD卡插拔检测任务
  Input:           param - 参数
  Return:          none
**********************************************************************/
static void *divoom_sd_card_detect_task(void *param)
{
    int inserted;
    DIVOOM_SD_CARD_STATE new_state;
    DIVOOM_SD_CARD_STATE old_state;
    
    divoom_print("SD card: detect task started\n");
    
    while (g_sd_card_handle != NULL && g_sd_card_handle->running)
    {
        inserted = divoom_sd_card_is_inserted();
        
        divoom_system_get_lock(g_sd_card_handle->lock);
        old_state = g_sd_card_handle->state;
        
        if (inserted)
        {
            // SD卡已插入
            if (old_state == DIVOOM_SD_CARD_STATE_NONE || 
                old_state == DIVOOM_SD_CARD_STATE_REMOVED ||
                old_state == DIVOOM_SD_CARD_STATE_UNMOUNTED)
            {
                // 尝试挂载
                new_state = DIVOOM_SD_CARD_STATE_INSERTED;
                g_sd_card_handle->state = new_state;
                divoom_system_free_lock(g_sd_card_handle->lock);
                
                divoom_print("SD card: detected card inserted\n");
                
//                // 回调通知
//                if (g_sd_card_handle->callback)
//                {
//                    g_sd_card_handle->callback(new_state);
//                }
//                
                // 尝试挂载
                if (divoom_sd_card_do_mount())
                {
                    divoom_system_get_lock(g_sd_card_handle->lock);
                    g_sd_card_handle->state = DIVOOM_SD_CARD_STATE_MOUNTED;
                    divoom_system_free_lock(g_sd_card_handle->lock);

                    // 挂载成功后先扫描音乐文件，再回调上层切模式，避免切到 TF 时歌单尚未入库
                    divoom_print("SD card: start scanning music files...\n");
                    divoom_sd_card_manage_scan_music();
                    divoom_print("SD card: scan complete, found %d music files\n", g_sd_card_handle->music_count);

                    if (g_sd_card_handle->callback)
                    {
                        g_sd_card_handle->callback(DIVOOM_SD_CARD_STATE_MOUNTED);
                    }
                }
                else
                {
                    divoom_system_get_lock(g_sd_card_handle->lock);
                    g_sd_card_handle->state = DIVOOM_SD_CARD_STATE_MOUNT_FAIL;
                    divoom_system_free_lock(g_sd_card_handle->lock);
                    
                    if (g_sd_card_handle->callback)
                    {
                        g_sd_card_handle->callback(DIVOOM_SD_CARD_STATE_MOUNT_FAIL);
                    }
                }
                
                divoom_system_get_lock(g_sd_card_handle->lock);
            }
        }
        else
        {
            // SD卡已拔出
            if (old_state == DIVOOM_SD_CARD_STATE_INSERTED ||
                old_state == DIVOOM_SD_CARD_STATE_MOUNTED ||
                old_state == DIVOOM_SD_CARD_STATE_MOUNT_FAIL)
            {
                // 如果之前是挂载状态，先卸载（短延时再卸，避免拔卡瞬间与内核竞争导致异常/重启）
                if (old_state == DIVOOM_SD_CARD_STATE_MOUNTED)
                {
                    divoom_system_free_lock(g_sd_card_handle->lock);
                    divoom_system_delay_ms(150);
                    divoom_sd_card_do_unmount();
                    divoom_system_get_lock(g_sd_card_handle->lock);
                }
                
                new_state = DIVOOM_SD_CARD_STATE_REMOVED;
                g_sd_card_handle->state = new_state;
                g_sd_card_handle->music_count = 0;
                divoom_system_free_lock(g_sd_card_handle->lock);
                
                divoom_print("SD card: detected card removed\n");
                
                // 回调通知
                if (g_sd_card_handle->callback)
                {
                    g_sd_card_handle->callback(new_state);
                }
                
                divoom_system_get_lock(g_sd_card_handle->lock);
            }
        }
        
        divoom_system_free_lock(g_sd_card_handle->lock);
        
        // 检测间隔
        divoom_system_delay_ms(DIVOOM_SD_CARD_CHECK_INTERVAL);
    }
    
    divoom_print("SD card: detect task stopped\n");
    return NULL;
}

/*********************************************************************
  Function:        divoom_sd_card_manage_init
  Description:     初始化SD卡管理模块，启动插拔检测线程
  Input:           callback - 状态变化回调函数
  Return:          1: 成功  0: 失败
**********************************************************************/
int divoom_sd_card_manage_init(DIVOOM_SD_CARD_CALLBACK callback)
{
    if (g_sd_card_handle != NULL)
    {
        divoom_print("SD card: already initialized\n");
        return 1;
    }
    
    g_sd_card_handle = divoom_system_malloc(sizeof(divoom_sd_card_manage_ctrl));
    if (g_sd_card_handle == NULL)
    {
        divoom_print("SD card: malloc failed\n");
        return 0;
    }
    
    memset(g_sd_card_handle, 0, sizeof(divoom_sd_card_manage_ctrl));
    
    g_sd_card_handle->state = DIVOOM_SD_CARD_STATE_NONE;
    g_sd_card_handle->callback = callback;
    g_sd_card_handle->running = 1;
    g_sd_card_handle->music_count = 0;
    
    // 创建互斥锁
    g_sd_card_handle->lock = divoom_system_create_lock();
    if (g_sd_card_handle->lock == NULL)
    {
        divoom_print("SD card: create lock failed\n");
        divoom_system_free(g_sd_card_handle);
        g_sd_card_handle = NULL;
        return 0;
    }
    
    // 启动检测任务
    g_sd_card_handle->task_handle = divoom_system_create_task(divoom_sd_card_detect_task, "sd_detect", NULL, DIVOOM_TASK_STACK_SD_DETECT);
    if (g_sd_card_handle->task_handle == NULL)
    {
        divoom_print("SD card: create task failed\n");
        divoom_system_destroy_lock(g_sd_card_handle->lock);
        divoom_system_free(g_sd_card_handle);
        g_sd_card_handle = NULL;
        return 0;
    }
    
    divoom_print("SD card: initialized successfully\n");
    return 1;
}

/*********************************************************************
  Function:        divoom_sd_card_manage_deinit
  Description:     反初始化SD卡管理模块
  Input:           none
  Return:          none
**********************************************************************/
void divoom_sd_card_manage_deinit(void)
{
    if (g_sd_card_handle == NULL)
    {
        return;
    }
    
    // 停止检测任务
    g_sd_card_handle->running = 0;
    
    // 等待任务结束
    divoom_system_delay_ms(DIVOOM_SD_CARD_CHECK_INTERVAL * 2);
    
    // 如果已挂载，先卸载
    if (g_sd_card_handle->state == DIVOOM_SD_CARD_STATE_MOUNTED)
    {
        divoom_sd_card_do_unmount();
    }
    
    // 销毁任务
    if (g_sd_card_handle->task_handle)
    {
        divoom_system_destroy_task(g_sd_card_handle->task_handle);
    }
    
    // 销毁锁
    if (g_sd_card_handle->lock)
    {
        divoom_system_destroy_lock(g_sd_card_handle->lock);
    }
    
    divoom_system_free(g_sd_card_handle);
    g_sd_card_handle = NULL;
    
    divoom_print("SD card: deinitialized\n");
}

/*********************************************************************
  Function:        divoom_sd_card_manage_get_state
  Description:     获取当前SD卡状态
  Input:           none
  Return:          DIVOOM_SD_CARD_STATE
**********************************************************************/
DIVOOM_SD_CARD_STATE divoom_sd_card_manage_get_state(void)
{
    DIVOOM_SD_CARD_STATE state;
    
    if (g_sd_card_handle == NULL)
    {
        return DIVOOM_SD_CARD_STATE_NONE;
    }
    
    divoom_system_get_lock(g_sd_card_handle->lock);
    state = g_sd_card_handle->state;
    divoom_system_free_lock(g_sd_card_handle->lock);
    
    return state;
}

/*********************************************************************
  Function:        divoom_sd_card_manage_is_mounted
  Description:     检查SD卡是否已挂载
  Input:           none
  Return:          1: 已挂载  0: 未挂载
**********************************************************************/
int divoom_sd_card_manage_is_mounted(void)
{
    return (divoom_sd_card_manage_get_state() == DIVOOM_SD_CARD_STATE_MOUNTED) ? 1 : 0;
}

/*********************************************************************
  Function:        divoom_sd_card_manage_mount
  Description:     手动挂载SD卡
  Input:           none
  Return:          1: 成功  0: 失败
**********************************************************************/
int divoom_sd_card_manage_mount(void)
{
    if (g_sd_card_handle == NULL)
    {
        return 0;
    }
    
    if (!divoom_sd_card_is_inserted())
    {
        divoom_print("SD card: no card inserted\n");
        return 0;
    }
    
    if (divoom_sd_card_manage_is_mounted())
    {
        divoom_print("SD card: already mounted\n");
        return 1;
    }
    
    if (divoom_sd_card_do_mount())
    {
        divoom_system_get_lock(g_sd_card_handle->lock);
        g_sd_card_handle->state = DIVOOM_SD_CARD_STATE_MOUNTED;
        divoom_system_free_lock(g_sd_card_handle->lock);

        /* 手动挂载同样先扫描再回调，保证回调切 TF 模式时可立即取到曲库 */
        divoom_print("SD card: start scanning music files...\n");
        divoom_sd_card_manage_scan_music();
        divoom_print("SD card: scan complete, found %d music files\n", g_sd_card_handle->music_count);

        if (g_sd_card_handle->callback)
        {
            g_sd_card_handle->callback(DIVOOM_SD_CARD_STATE_MOUNTED);
        }
        
        return 1;
    }
    
    return 0;
}

/*********************************************************************
  Function:        divoom_sd_card_manage_unmount
  Description:     手动卸载SD卡
  Input:           none
  Return:          1: 成功  0: 失败
**********************************************************************/
int divoom_sd_card_manage_unmount(void)
{
    if (g_sd_card_handle == NULL)
    {
        return 0;
    }
    
    if (!divoom_sd_card_manage_is_mounted())
    {
        divoom_print("SD card: not mounted\n");
        return 1;
    }
    
    if (divoom_sd_card_do_unmount())
    {
        divoom_system_get_lock(g_sd_card_handle->lock);
        g_sd_card_handle->state = DIVOOM_SD_CARD_STATE_UNMOUNTED;
        g_sd_card_handle->music_count = 0;
        divoom_system_free_lock(g_sd_card_handle->lock);
        
        if (g_sd_card_handle->callback)
        {
            g_sd_card_handle->callback(DIVOOM_SD_CARD_STATE_UNMOUNTED);
        }
        
        return 1;
    }
    
    return 0;
}

/*********************************************************************
  Function:        divoom_sd_card_manage_scan_music
  Description:     扫描SD卡中的音乐文件并添加到数据库
  Input:           none
  Return:          扫描到的音乐文件数量
**********************************************************************/
int divoom_sd_card_manage_scan_music(void)
{
    if (g_sd_card_handle == NULL)
    {
        return 0;
    }
    
    if (!divoom_sd_card_manage_is_mounted())
    {
        divoom_print("SD card: not mounted, cannot scan\n");
        return 0;
    }
    
    // 重置计数
    g_sd_card_handle->music_count = 0;
    
    // 开始扫描（整树一次事务，写入 music.db）
    divoom_database_manage_local_music_batch_begin();
    divoom_sd_card_scan_dir(DIVOOM_SD_CARD_MUSIC_PATH);
    divoom_database_manage_local_music_batch_end();
    
    return g_sd_card_handle->music_count;
}

/*********************************************************************
  Function:        divoom_sd_card_manage_get_mount_path
  Description:     获取SD卡挂载路径
  Input:           none
  Return:          挂载路径字符串
**********************************************************************/
const char* divoom_sd_card_manage_get_mount_path(void)
{
    return DIVOOM_SD_CARD_MOUNT_PATH;
}
