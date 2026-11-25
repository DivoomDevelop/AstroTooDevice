#include "divoom.h"
/*
 * Line-in / USB / SD卡(TF) / 充电满 检测：以普通 GPIO 轮询方式实现。
 * 开一个线程循环读取各检测脚电平，状态变化时触发回调。
 * DTS 中需将对应引脚从 gpio-keys 移除或禁用，由本模块独占使用。
 * T113: PA=0,PB=1,PC=2,PD=3,PE=4,PF=5,PG=6 -> gpio_num = bank*32 + pin
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define DIVOOM_INPUT_HOTPLUG_MAX         8
#define DIVOOM_GPIO_POLL_INTERVAL_MS     200

#define DIVOOM_USB_KEY_CODE              183
#define DIVOOM_LINEIN_KEY_CODE           184
#define DIVOOM_TF_KEY_CODE               185
#define DIVOOM_CHG_FULL_KEY_CODE         186
#define DIVOOM_FAST_CHG_KEY_CODE         187

/* key_code -> { gpio_num, active_low }，与 board.dts 中 gpios 一致；active_low: 1=低电平有效，0=高电平有效 */
typedef struct {
    int key_code;
    int gpio_num;
    int active_low; /* 1: 低电平=插入/有效；0: 高电平=插入/有效 */
} divoom_det_gpio_t;

static const divoom_det_gpio_t g_det_gpio_table[] = {
    { DIVOOM_LINEIN_KEY_CODE,   4 * 32 + 10,  1 },  /* PE10 linein-det */
    { DIVOOM_USB_KEY_CODE,      6 * 32 + 11,  1 },  /* PG11 usb-det */
    { DIVOOM_TF_KEY_CODE,       5 * 32 + 6,   1 },  /* PF6  sd-card-det */
    { DIVOOM_CHG_FULL_KEY_CODE, 3 * 32 + 21,  0 },  /* PD21 chg-stat */
    { DIVOOM_FAST_CHG_KEY_CODE, 5 * 32 + 5,  0 },  /* PF5  fast-chg-det (USB12V-DET, 高电平=快充) */
};
#define DIVOOM_DET_GPIO_NUM     (sizeof(g_det_gpio_table) / sizeof(g_det_gpio_table[0]))

typedef void (*DIVOOM_INPUT_HOTPLUG_CB)(int key_code, int state, void *user);
typedef void (*DIVOOM_DET_HOTPLUG_CB)(int inserted);
typedef DIVOOM_DET_HOTPLUG_CB DIVOOM_SIMPLE_HOTPLUG_CB;

typedef struct _divoom_input_hotplug_item {
    int used;
    int key_code;
    int last_state; /* 0: removed, 1: inserted, -1: unknown */
    DIVOOM_INPUT_HOTPLUG_CB cb;
    void *user;
    DIVOOM_SIMPLE_HOTPLUG_CB simple_cb; /* 给“简单回调”模式使用 */
} divoom_input_hotplug_item;

typedef struct _divoom_input_hotplug_ctrl {
    int inited;
    int run_flag;
    void *task_handle;
    void *lock_handle;
    divoom_input_hotplug_item items[DIVOOM_INPUT_HOTPLUG_MAX];
} divoom_input_hotplug_ctrl;

static divoom_input_hotplug_ctrl g_hp;

/* 根据 key_code 查 GPIO 配置，未找到返回 -1 */
static const divoom_det_gpio_t *divoom_det_gpio_find(int key_code)
{
    for (size_t i = 0; i < DIVOOM_DET_GPIO_NUM; i++) {
        if (g_det_gpio_table[i].key_code == key_code) {
            return &g_det_gpio_table[i];
        }
    }
    return NULL;
}

static int divoom_gpio_export(int gpio_num)
{
    char path[64];
    char buf[8];

    if (gpio_num < 0) return 0;
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d", gpio_num);
    if (divoom_port_file_exists(path)) return 0;
    snprintf(buf, sizeof(buf), "%d", gpio_num);
    if (divoom_port_hw_sysfs_write("/sys/class/gpio/export", buf) != 0)
        return -1;
    divoom_port_delay_ms((50000) / 1000U);
    return 0;
}

static int divoom_gpio_set_direction_in(int gpio_num)
{
    char path[64];

    if (gpio_num < 0) return -1;
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", gpio_num);
    return divoom_port_hw_sysfs_write(path, "in");
}

/* 读 GPIO 电平 0/1，失败返回 -1 */
static int divoom_gpio_read_value(int gpio_num)
{
    char path[64];
    char buf[4];
    int n;

    if (gpio_num < 0) return -1;
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", gpio_num);
    n = divoom_port_hw_sysfs_read(path, buf, sizeof(buf));
    if (n <= 0) return -1;
    return (buf[0] == '1') ? 1 : 0;
}

static int divoom_input_used_count(void)
{
    int c = 0;
    for (int i = 0; i < DIVOOM_INPUT_HOTPLUG_MAX; i++) {
        if (g_hp.items[i].used) c++;
    }
    return c;
}

/* GPIO 轮询线程：循环读 linein/usb/tf/chg_full 电平，状态变化时回调 */
static void *divoom_input_hotplug_gpio_poll_task(void *param)
{
    (void)param;
    divoom_print("input hotplug gpio poll start\n");

    while (g_hp.run_flag) {
        /* 两趟扫描：先更新 PF5 快充插入，再更新 PD21 充满等。
         * 否则同一周期内先读到“满电”毛刺、divoom_in_charge_sta 仍为上一周期“在充”，会误弹充满 UI。 */
        for (int pass = 0; pass < 2; pass++) {
        for (int i = 0; i < DIVOOM_INPUT_HOTPLUG_MAX; i++) {
            divoom_system_get_lock(g_hp.lock_handle);
            if (!g_hp.items[i].used) {
                divoom_system_free_lock(g_hp.lock_handle);
                continue;
            }
            int key_code = g_hp.items[i].key_code;
            if (pass == 0 && key_code != DIVOOM_FAST_CHG_KEY_CODE)
                { divoom_system_free_lock(g_hp.lock_handle); continue; }
            if (pass == 1 && key_code == DIVOOM_FAST_CHG_KEY_CODE)
                { divoom_system_free_lock(g_hp.lock_handle); continue; }
            const divoom_det_gpio_t *gp = divoom_det_gpio_find(key_code);
            if (!gp) {
                divoom_system_free_lock(g_hp.lock_handle);
                continue;
            }
            DIVOOM_SIMPLE_HOTPLUG_CB cb = g_hp.items[i].simple_cb;
            divoom_system_free_lock(g_hp.lock_handle);

            int val = divoom_gpio_read_value(gp->gpio_num);
            if (val < 0) continue;
            int st = gp->active_low ? (val == 0 ? 1 : 0) : val;

            divoom_system_get_lock(g_hp.lock_handle);
            if (g_hp.items[i].last_state != st) {
                g_hp.items[i].last_state = st;
                divoom_system_free_lock(g_hp.lock_handle);
                divoom_print("hotplug key=%d state=%d (gpio poll)\n", key_code, st);
                if (cb) cb(st);
            } else {
                divoom_system_free_lock(g_hp.lock_handle);
            }
        }
        }
        divoom_system_delay_ms(DIVOOM_GPIO_POLL_INTERVAL_MS);
    }
    divoom_print("input hotplug gpio poll stop\n");
    return NULL;
}

int divoom_input_hotplug_register(int key_code, DIVOOM_INPUT_HOTPLUG_CB cb, void *user)
{
    if (key_code <= 0) {
        return -1;
    }

    if (!g_hp.lock_handle) {
        g_hp.lock_handle = divoom_system_create_lock();
    }

    divoom_system_get_lock(g_hp.lock_handle);

    /* 已存在则更新 */
    for (int i = 0; i < DIVOOM_INPUT_HOTPLUG_MAX; i++) {
        if (g_hp.items[i].used && g_hp.items[i].key_code == key_code) {
            g_hp.items[i].cb = cb;
            g_hp.items[i].user = user;
            divoom_system_free_lock(g_hp.lock_handle);
            return 0;
        }
    }

    /* 新增 */
    for (int i = 0; i < DIVOOM_INPUT_HOTPLUG_MAX; i++) {
        if (!g_hp.items[i].used) {
            g_hp.items[i].used = 1;
            g_hp.items[i].key_code = key_code;
            g_hp.items[i].last_state = -1;
            g_hp.items[i].cb = cb;
            g_hp.items[i].user = user;
            divoom_system_free_lock(g_hp.lock_handle);
            return 0;
        }
    }

    divoom_system_free_lock(g_hp.lock_handle);
    return -1;
}

int divoom_input_hotplug_unregister(int key_code)
{
    if (!g_hp.lock_handle) {
        return -1;
    }

    divoom_system_get_lock(g_hp.lock_handle);
    for (int i = 0; i < DIVOOM_INPUT_HOTPLUG_MAX; i++) {
        if (g_hp.items[i].used && g_hp.items[i].key_code == key_code) {
            memset(&g_hp.items[i], 0, sizeof(g_hp.items[i]));
            break;
        }
    }
    int left = divoom_input_used_count();
    divoom_system_free_lock(g_hp.lock_handle);

    if (left == 0 && g_hp.inited) {
        g_hp.run_flag = 0;
        divoom_system_delay_ms(DIVOOM_GPIO_POLL_INTERVAL_MS + 50);
        g_hp.inited = 0;
    }

    return 0;
}

int divoom_input_hotplug_get_state(int key_code)
{
    if (!g_hp.lock_handle) {
        return -1;
    }

    int st = -1;
    divoom_system_get_lock(g_hp.lock_handle);
    for (int i = 0; i < DIVOOM_INPUT_HOTPLUG_MAX; i++) {
        if (g_hp.items[i].used && g_hp.items[i].key_code == key_code) {
            st = g_hp.items[i].last_state;
            break;
        }
    }
    divoom_system_free_lock(g_hp.lock_handle);
    return st;
}

int divoom_input_hotplug_start(void)
{
    if (!g_hp.lock_handle) {
        g_hp.lock_handle = divoom_system_create_lock();
    }

    /* 无论是否已初始化，都先为当前已注册的检测脚 export 并设为输入，避免后注册的检测口被漏掉 */
    for (int i = 0; i < DIVOOM_INPUT_HOTPLUG_MAX; i++) {
        if (!g_hp.items[i].used) continue;
        const divoom_det_gpio_t *gp = divoom_det_gpio_find(g_hp.items[i].key_code);
        if (!gp) continue;
        if (divoom_gpio_export(gp->gpio_num) == 0) {
            divoom_gpio_set_direction_in(gp->gpio_num);
        } else {
            divoom_print("input hotplug gpio%d export failed (errno=%d)\n", gp->gpio_num, errno);
        }
    }

    if (g_hp.inited) {
        return 0;  /* 轮询任务已在跑，只补充了新口的 GPIO 配置 */
    }

    g_hp.run_flag = 1;
    g_hp.inited = 1;
    g_hp.task_handle = divoom_system_create_task(divoom_input_hotplug_gpio_poll_task, "hotplug", NULL, DIVOOM_TASK_STACK_INPUT_HOTPLUG);
    return 0;
}

void divoom_input_hotplug_stop(void)
{
    if (!g_hp.inited) return;
    g_hp.run_flag = 0;
    divoom_system_delay_ms(DIVOOM_GPIO_POLL_INTERVAL_MS + 50);
    g_hp.inited = 0;
}

/* -------------------- 通用 wrapper：给 USB/Line-in/TF 等复用 -------------------- */

static void divoom_simple_wrapper_cb(int key_code, int state, void *user)
{
    /* user 指向对应的 item，避免把函数指针塞进 void*（更稳） */
    divoom_input_hotplug_item *item = (divoom_input_hotplug_item *)user;
    if (!item) {
        return;
    }
    if (item->key_code != key_code) {
        return;
    }
    if (item->simple_cb) {
        item->simple_cb(state);
    }
}

int divoom_input_hotplug_register_simple(int key_code, DIVOOM_SIMPLE_HOTPLUG_CB cb)
{
    if (key_code <= 0) {
        return -1;
    }

    if (!g_hp.lock_handle) {
        g_hp.lock_handle = divoom_system_create_lock();
    }

    divoom_system_get_lock(g_hp.lock_handle);

    /* 已存在则更新 */
    for (int i = 0; i < DIVOOM_INPUT_HOTPLUG_MAX; i++) {
        if (g_hp.items[i].used && g_hp.items[i].key_code == key_code) {
            g_hp.items[i].cb = divoom_simple_wrapper_cb;
            g_hp.items[i].user = &g_hp.items[i];
            g_hp.items[i].simple_cb = cb;
            divoom_system_free_lock(g_hp.lock_handle);
            return 0;
        }
    }

    /* 新增 */
    for (int i = 0; i < DIVOOM_INPUT_HOTPLUG_MAX; i++) {
        if (!g_hp.items[i].used) {
            g_hp.items[i].used = 1;
            g_hp.items[i].key_code = key_code;
            g_hp.items[i].last_state = -1;
            g_hp.items[i].cb = divoom_simple_wrapper_cb;
            g_hp.items[i].user = &g_hp.items[i];
            g_hp.items[i].simple_cb = cb;
            divoom_system_free_lock(g_hp.lock_handle);
            return 0;
        }
    }

    divoom_system_free_lock(g_hp.lock_handle);
    return -1;
}

/* -------------------- USB 兼容接口（保持你之前的调用方式） -------------------- */

#define DIVOOM_USB_STATE_PATH    "/sys/class/android_usb/android0/state"
#define DIVOOM_USB_STATE_CONFIGURED  "CONFIGURED"

/* 读取内核 USB gadget 状态：接 PC 时会变为 CONFIGURED，接充电器不会。用于区分是否可进入 UAC。 */
int divoom_usb_is_configured(void)
{
	char path[] = DIVOOM_USB_STATE_PATH;
	char buf[32];
	int fd, n;
	size_t len = sizeof(DIVOOM_USB_STATE_CONFIGURED) - 1;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return 0;
	n = (int)read(fd, buf, sizeof(buf) - 1);
	close(fd);
	if (n <= 0)
		return 0;
	buf[n] = '\0';
	/* 去掉末尾换行 */
	if (n > 0 && buf[n - 1] == '\n')
		buf[--n] = '\0';
	if ((size_t)n != len)
		return 0;
	return (memcmp(buf, DIVOOM_USB_STATE_CONFIGURED, len) == 0) ? 1 : 0;
}

int divoom_usb_hotplug_init(DIVOOM_DET_HOTPLUG_CB cb)
{
    /* 注册 USB 的 KEY_F13，然后启动监听 */
    if (divoom_input_hotplug_register_simple(DIVOOM_USB_KEY_CODE, cb) != 0) {
        return -1;
    }
    return divoom_input_hotplug_start();
}

int divoom_usb_hotplug_get_state(void)
{
    return divoom_input_hotplug_get_state(DIVOOM_USB_KEY_CODE);
}

void divoom_usb_hotplug_deinit(void)
{
    (void)divoom_input_hotplug_unregister(DIVOOM_USB_KEY_CODE);
}

/* -------------------- Line-in 兼容接口 -------------------- */

int divoom_linein_hotplug_init(DIVOOM_DET_HOTPLUG_CB cb)
{
    if (divoom_input_hotplug_register_simple(DIVOOM_LINEIN_KEY_CODE, cb) != 0) {
        return -1;
    }
    return divoom_input_hotplug_start();
}

int divoom_linein_hotplug_get_state(void)
{
    return divoom_input_hotplug_get_state(DIVOOM_LINEIN_KEY_CODE);
}

void divoom_linein_hotplug_deinit(void)
{
    (void)divoom_input_hotplug_unregister(DIVOOM_LINEIN_KEY_CODE);
}

/* -------------------- TF 兼容接口 -------------------- */

int divoom_tf_hotplug_init(DIVOOM_DET_HOTPLUG_CB cb)
{
    if (divoom_input_hotplug_register_simple(DIVOOM_TF_KEY_CODE, cb) != 0) {
        return -1;
    }
    return divoom_input_hotplug_start();
}

int divoom_tf_hotplug_get_state(void)
{
    return divoom_input_hotplug_get_state(DIVOOM_TF_KEY_CODE);
}

void divoom_tf_hotplug_deinit(void)
{
    (void)divoom_input_hotplug_unregister(DIVOOM_TF_KEY_CODE);
}

/* -------------------- CHG-FULL 兼容接口 -------------------- */

int divoom_chg_full_init(DIVOOM_DET_HOTPLUG_CB cb)
{
    if (divoom_input_hotplug_register_simple(DIVOOM_CHG_FULL_KEY_CODE, cb) != 0) {
        return -1;
    }
    return divoom_input_hotplug_start();
}

int divoom_chg_full_get_state(void)
{
    return divoom_input_hotplug_get_state(DIVOOM_CHG_FULL_KEY_CODE);
}

void divoom_chg_full_deinit(void)
{
    (void)divoom_input_hotplug_unregister(DIVOOM_CHG_FULL_KEY_CODE);
}

/* -------------------- 快充口检测（PF5）兼容接口 -------------------- */

int divoom_fast_chg_hotplug_init(DIVOOM_DET_HOTPLUG_CB cb)
{
    if (divoom_input_hotplug_register_simple(DIVOOM_FAST_CHG_KEY_CODE, cb) != 0) {
        return -1;
    }
    return divoom_input_hotplug_start();
}

int divoom_fast_chg_hotplug_get_state(void)
{
    return divoom_input_hotplug_get_state(DIVOOM_FAST_CHG_KEY_CODE);
}

void divoom_fast_chg_hotplug_deinit(void)
{
    (void)divoom_input_hotplug_unregister(DIVOOM_FAST_CHG_KEY_CODE);
}

