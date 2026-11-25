/**
 * @file aic_wifi.h
 * @brief Allwinner WiFi 统一对外 API(对标 esp_wifi.h)
 *
 * 设计目标:
 *   应用 / WifiManager / Amazon-FreRTOS 只看这一份头文件,
 *   底下 AIC8800 驱动怎么改、甚至换芯片,都不影响上层。
 *
 * 和 esp_wifi 的三个关键差异(适配层负责抹平):
 *   1. init / set_mode / start 拆开 —— AIC 原来 aic_wifi_init 一把梭
 *   2. connect 异步,立即返回,结果走事件 —— AIC 原 wlan_sta_connect 阻塞 30s
 *   3. 事件回调在适配层自己的任务里调,不在 wpa 任务里
 *
 * 命名对齐 esp_wifi,前缀用 aw_wifi_。要直接移植 esp 代码,
 * 再加一层 esp_wifi_* 的薄 alias 即可。
 */
#ifndef _AIC_WIFI_H_
#define _AIC_WIFI_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================ *
 * 常量
 * ============================================================ */
#define AW_WIFI_SSID_LEN   32
#define AW_WIFI_PASS_LEN   64
#define AW_WIFI_MAC_LEN    6

/* ============================================================ *
 * 类型定义
 * ============================================================ */

/** WiFi 工作模式 */
typedef enum {
    AW_WIFI_MODE_NULL = 0,   /* 未配置 */
    AW_WIFI_MODE_STA,        /* 只做 STA(连别人) */
    AW_WIFI_MODE_AP,         /* 只做 AP(热点) */
    AW_WIFI_MODE_STAAP,      /* STA + AP 同时(预留,AIC8800 暂不支持) */
    AW_WIFI_MODE_P2P,        /* P2P GO(WiFi Direct, CarPlay/Miracast) */
} aw_wifi_mode_t;

/** 接口(STA 或 AP) */
typedef enum {
    AW_WIFI_IF_STA = 0,
    AW_WIFI_IF_AP,
} aw_wifi_interface_t;

/** 频段 */
typedef enum {
    AW_WIFI_BAND_2G = 0,     /* 2.4 GHz */
    AW_WIFI_BAND_5G = 1,     /* 5 GHz */
} aw_wifi_band_t;

/** 带宽 */
typedef enum {
    AW_WIFI_BW_20M = 0,     /* 20 MHz */
    AW_WIFI_BW_40M = 1,     /* 40 MHz */
} aw_wifi_bandwidth_t;

/** 认证 / 加密方式 */
typedef enum {
    AW_WIFI_AUTH_OPEN = 0,
    AW_WIFI_AUTH_WEP,
    AW_WIFI_AUTH_WPA_PSK,
    AW_WIFI_AUTH_WPA2_PSK,
    AW_WIFI_AUTH_WPA_WPA2_PSK,
    AW_WIFI_AUTH_WPA3_PSK,
} aw_wifi_auth_mode_t;

/** STA 侧配置(连哪个 AP) */
typedef struct {
    uint8_t  ssid[AW_WIFI_SSID_LEN];     /* 要连的 SSID */
    uint8_t  password[AW_WIFI_PASS_LEN]; /* 密码,OPEN 方式留空 */
    aw_wifi_auth_mode_t authmode;        /* 认证方式 */
    aw_wifi_band_t band;                 /* 频段: AW_WIFI_BAND_2G / AW_WIFI_BAND_5G */
    uint8_t  channel;                     /* 0 = 不指定,自动选 */
    bool     bssid_set;                   /* 是否锁定到具体 BSSID */
    uint8_t  bssid[AW_WIFI_MAC_LEN];     /* 锁定的 BSSID */
} aw_wifi_sta_config_t;

/** AP 侧配置(开什么热点) */
typedef struct {
    uint8_t  ssid[AW_WIFI_SSID_LEN];     /* 热点 SSID */
    uint8_t  password[AW_WIFI_PASS_LEN]; /* 密码,OPEN 方式留空 */
    aw_wifi_auth_mode_t authmode;        /* 认证方式 */
    aw_wifi_band_t band;                 /* 频段: AW_WIFI_BAND_2G / AW_WIFI_BAND_5G */
    aw_wifi_bandwidth_t bandwidth;       /* 带宽: AW_WIFI_BW_20M / AW_WIFI_BW_40M */
    uint8_t  channel;                     /* 信道,2.4G(1~13),5G(36/40/44/48/149/153/157/161/165) */
    bool     ssid_hidden;                 /* 是否隐藏 SSID */
    uint8_t  max_connection;              /* 最多接几个 STA,默认 4 */
    uint16_t max_inactivity;              /* STA 最大无活动超时(秒),默认 60 */
    uint16_t beacon_interval;             /* 信标间隔,0 = 用默认(100) */
    bool     enable_he;                   /* WiFi 6 HE 速率开关 */
    bool     enable_acs;                  /* 自动信道选择 */
    uint8_t  acs_scan_num;               /* ACS 扫描次数,默认 0 */
} aw_wifi_ap_config_t;

/** P2P 侧配置(开什么 P2P GO) */
typedef struct {
    uint8_t  ssid[AW_WIFI_SSID_LEN];     /* P2P 设备名 */
    uint8_t  password[AW_WIFI_PASS_LEN]; /* 密码 */
    aw_wifi_auth_mode_t authmode;        /* 认证方式 */
    aw_wifi_band_t band;                 /* 频段 */
    aw_wifi_bandwidth_t bandwidth;       /* 带宽 */
    uint8_t  channel;                     /* 信道 */
    bool     enable_he;                   /* WiFi 6 HE 速率开关 */
    bool     enable_acs;                  /* 自动信道选择 */
    uint8_t  acs_scan_num;               /* ACS 扫描次数 */
} aw_wifi_p2p_config_t;

/** 配置 union,按接口取 sta 或 ap 或 p2p */
typedef union {
    aw_wifi_sta_config_t sta;
    aw_wifi_ap_config_t  ap;
    aw_wifi_p2p_config_t p2p;
} aw_wifi_config_t;

/** init 配置,目前预留,可传 NULL */
typedef struct {
    int reserved;
} aw_wifi_init_config_t;

/** 扫描参数 */
typedef struct {
    uint8_t *ssid;        /* 只扫指定 SSID,NULL = 全扫 */
    uint8_t *bssid;       /* 只扫指定 BSSID,NULL = 不限 */
    aw_wifi_band_t band;  /* 频段过滤,AW_WIFI_BAND_2G / AW_WIFI_BAND_5G */
    uint8_t  channel;     /* 0 = 全信道 */
    bool     show_hidden; /* 是否扫隐藏 SSID */
} aw_wifi_scan_config_t;

/** 单条扫描结果 */
typedef struct {
    uint8_t  bssid[AW_WIFI_MAC_LEN];
    uint8_t  ssid[AW_WIFI_SSID_LEN];
    aw_wifi_band_t band;                 /* 频段 */
    uint8_t  channel;
    int8_t   rssi;
    /* TODO: authmode 待 AIC 原厂在 wifi_ap_info_t 中补上后启用
     * 当前原厂 wifi_ap_info_t 只有 ssid/bssid/channel/rssi,无 authmode
     */
    aw_wifi_auth_mode_t authmode;
} aw_wifi_ap_record_t;

/** AP 侧单个已接入 STA 的信息 */
typedef struct {
    uint8_t mac[AW_WIFI_MAC_LEN];
    int8_t  rssi;
} aw_wifi_sta_info_t;

/** AP 侧已接入 STA 列表 */
typedef struct {
    uint8_t num;                          /* 实际数量 */
    aw_wifi_sta_info_t sta[8];
} aw_wifi_sta_list_t;

/* ============================================================ *
 * 事件
 * ============================================================ */

/** STA 连接结果；数值与底层 wlan_sta_connect_result_t 保持一致。 */
typedef enum {
    AW_WIFI_STA_CONNECT_OK                       = 0,
    AW_WIFI_STA_CONNECT_ERR_PASSWORD_TOO_SHORT   = -1,
    AW_WIFI_STA_CONNECT_ERR_VIF_CONFIG           = -2,
    AW_WIFI_STA_CONNECT_ERR_NETIF_NOT_FOUND      = -3,
    AW_WIFI_STA_CONNECT_ERR_NET_INIT             = -4,
    AW_WIFI_STA_CONNECT_ERR_AUTH                 = -5,
    AW_WIFI_STA_CONNECT_ERR_DHCP                 = -6,
    AW_WIFI_STA_CONNECT_ERR_INVALID_SSID         = -7,
    AW_WIFI_STA_CONNECT_ERR_NETIF_ADD            = -8,
    AW_WIFI_STA_CONNECT_ERR_CONTROL_LINK         = -9,
    AW_WIFI_STA_CONNECT_ERR_CONNECT              = -10,
    AW_WIFI_STA_CONNECT_ERR_INVALID_PASSWORD     = -11,
    AW_WIFI_STA_CONNECT_ERR_SCAN_LIMIT_REACHED   = -12,
    AW_WIFI_STA_CONNECT_ERR_WRONG_KEY            = -13,
    AW_WIFI_STA_CONNECT_ERR_MISC                 = -14,
} aw_wifi_sta_connect_result_t;

/** 事件 id */
typedef enum {
    AW_WIFI_EVENT_WIFI_READY = 0,    /* 驱动就绪 */
    AW_WIFI_EVENT_SCAN_DONE,         /* 扫描完成,可以取结果了 */
    AW_WIFI_EVENT_STA_START,         /* STA 已启动 */
    AW_WIFI_EVENT_STA_STOP,
    AW_WIFI_EVENT_STA_CONNECTED,     /* STA 关联成功 */
    AW_WIFI_EVENT_STA_DISCONNECTED,  /* STA 断开(密码错/超时/被踢) */
    AW_WIFI_EVENT_AP_START,
    AW_WIFI_EVENT_AP_STOP,
    AW_WIFI_EVENT_AP_STACONNECTED,   /* 有 STA 接入热点 */
    AW_WIFI_EVENT_AP_STADISCONNECTED,
    AW_WIFI_EVENT_P2P_START,         /* P2P GO 已启动 */
    AW_WIFI_EVENT_P2P_STOP,
    AW_WIFI_EVENT_P2P_STACONNECTED,  /* P2P 对端接入 */
    AW_WIFI_EVENT_P2P_STADISCONNECTED,
    AW_WIFI_EVENT_P2P_PRO_DISC_REQ,  /* 收到 P2P Provisioning Discovery 请求 */
    AW_IP_EVENT_STA_GOT_IP,          /* DHCP 拿到 IP */
    AW_IP_EVENT_STA_LOST_IP,
    AW_WIFI_EVENT_MAX,
} aw_wifi_event_id_t;

/** 事件附加信息,不同事件按需取用 */
typedef struct {
    uint8_t  mac[AW_WIFI_MAC_LEN]; /* AP/P2P 侧 STA 接入/断开时的 mac */
    int32_t  reason;                /* 连接失败时为 aw_wifi_sta_connect_result_t */
    uint32_t ip;                    /* GOT_IP 时的 IP(网络字节序) */
    uint32_t gateway;               /* 网关(网络字节序) */
    uint32_t netmask;               /* 子网掩码(网络字节序) */
    uint32_t sta_session;           /* Nonzero for session-bound STA events. */
} aw_wifi_event_info_t;

/** 事件回调。调用线程取决于事件来源，回调中不要执行长时间阻塞操作。 */
typedef void (*aw_wifi_event_cb_t)(aw_wifi_event_id_t id,
                                   const aw_wifi_event_info_t *info,
                                   void *arg);

/* ============================================================ *
 * 核心接口
 * ============================================================ */

/**
 * 初始化 WiFi 驱动(只初始化,不启动)。
 * 底层对应:platform_config_init / aic_wifi_init / wifi_drv_event_set_cbk
 * @param cfg  配置,目前可传 NULL
 * @return 0 成功,<0 失败
 */
int aw_wifi_init(const aw_wifi_init_config_t *cfg);

/**
 * 反初始化,释放资源。
 * 底层对应:aic_wifi_deinit
 * @return 0 成功,<0 失败
 */
int aw_wifi_deinit(void);

/**
 * 设置工作模式。必须先于 aw_wifi_start 调用。
 * 底层对应:aic_wifi_set_mode
 * @param mode  工作模式
 * @return 0 成功,<0 失败
 */
int aw_wifi_set_mode(aw_wifi_mode_t mode);

/**
 * 取当前工作模式。
 * @param mode  输出当前模式
 * @return 0 成功,<0 失败
 */
int aw_wifi_get_mode(aw_wifi_mode_t *mode);

/**
 * 按 set_mode / set_config 的设置启动 WiFi。
 * 成功后会抛 STA_START / AP_START / P2P_START 事件。
 * 底层对应:wlan_start_sta / wlan_start_ap / wlan_start_p2p
 * @return 0 成功,<0 失败
 */
int aw_wifi_start(void);

/**
 * 一步到位:设置模式并启动 WiFi。
 * 等价于 aw_wifi_set_mode(mode) + aw_wifi_start()。
 * @param mode 工作模式
 * @return 0 成功,<0 失败
 */
int aw_wifi_start_mode(aw_wifi_mode_t mode);

/**
 * 停止 WiFi。会抛 STA_STOP / AP_STOP / P2P_STOP 事件。
 * 底层对应:wlan_stop_ap / wlan_stop_p2p / STA 侧 stop
 * @return 0 成功,<0 失败
 */
int aw_wifi_stop(void);

/**
 * 设置 STA 或 AP 或 P2P 的配置。建议在 aw_wifi_start 之前调。
 * 底层对应:wlan_start_ap(aic_ap_cfg*) / wlan_sta_cfg / wlan_start_p2p(aic_p2p_cfg*)
 * @param iface  选 STA / AP / P2P 接口
 * @param conf   配置内容
 * @return 0 成功,<0 失败
 */
int aw_wifi_set_config(aw_wifi_interface_t iface, const aw_wifi_config_t *conf);

/**
 * 取当前 STA/AP 的配置。
 * @param iface  选 STA / AP 接口
 * @param conf   输出配置内容
 * @return 0 成功,<0 失败
 */
int aw_wifi_get_config(aw_wifi_interface_t iface, aw_wifi_config_t *conf);

/* ============================================================ *
 * STA 接口
 * ============================================================ */

/**
 * 发起连接(用 set_config(STA) 设的 SSID/密码)。
 * 异步:立即返回,连上/连不上/超时都走 STA_CONNECTED / STA_DISCONNECTED 事件。
 * 底层对应:wlan_sta_connect(适配层起一个 task 包住它原来的 30s 阻塞)
 * @return 0 表示已发起；立即失败时返回 aw_wifi_sta_connect_result_t。
 *         异步连接失败通过 STA_DISCONNECTED 的 info->reason 返回同一错误码。
 */
int aw_wifi_connect(void);

/**
 * 一步到位:设置STA配置 + 连接 + 连上后自动DHCP。
 * 等价于 set_config + connect + 等待STA_CONNECTED + dhcpc_start。
 * @param ssid     路由器 SSID
 * @param password 密码
 * @return aw_wifi_sta_connect_result_t；DHCP 失败返回 AW_WIFI_STA_CONNECT_ERR_DHCP。
 */
int aw_wifi_sta_join(const char *ssid, const char *password);

/**
 * 断开当前连接，并清理掉线后残留的 WPA/DHCP 会话。
 * 底层对应:wlan_disconnect_sta
 * @return 0 成功,<0 失败
 */
int aw_wifi_disconnect(void);

/**
 * 查是否已连上(同步,直接看状态)。
 * 底层对应:wlan_get_connect_status
 * @return true 已连接,false 未连接
 */
bool aw_wifi_is_connected(void);

/* ============================================================ *
 * 扫描接口
 * ============================================================ */

/**
 * 启动扫描。
 * @param cfg   扫描参数,NULL = 全信道全扫
 * @param block true = 阻塞到扫完再返回;false = 立即返回,扫完抛 SCAN_DONE
 * 底层对应:wlan_if_scan_open / scan / getscan / close 四步
 * @return 0 成功,<0 失败
 */
int aw_wifi_scan_start(const aw_wifi_scan_config_t *cfg, bool block);

/* Nonblocking memory snapshot during the current scan; at most 24 distinct SSIDs per call.
 * Returns no records before scan acceptance or after completion. */
int aw_wifi_scan_get_ap_progress(uint16_t *number, aw_wifi_ap_record_t *aps);

/**
 * 中止正在进行的扫描。
 * 底层对应:wlan_if_scan_close
 * @return 0 成功,<0 失败
 */
int aw_wifi_scan_stop(void);

/**
 * 取扫描到的 AP 数量。在 SCAN_DONE 之后调。
 * 底层对应:wifi_ap_list_t.ap_count
 * @param number  输出 AP 数量
 * @return 0 成功,<0 失败
 */
int aw_wifi_scan_get_ap_num(uint16_t *number);

/**
 * 取扫描结果。
 * @param number  传入缓冲区能装多少条,返回实际写入多少条
 * @param aps     结果缓冲区
 * 底层对应:wifi_ap_list_t -> aw_wifi_ap_record_t 翻译
 * @return 0 成功,<0 失败
 */
int aw_wifi_scan_get_ap_records(uint16_t *number, aw_wifi_ap_record_t *aps);

/**
 * 打印扫描结果（表格格式）。
 * 在 SCAN_DONE 之后调,将结果以表格打印到控制台。
 * @return 0 成功,<0 无扫描结果
 */
int aw_wifi_scan_print_results(void);

/* ============================================================ *
 * AP 接口
 * ============================================================ */

/**
 * 取 AP 侧当前接入的 STA 列表。
 * 底层对应:wlan_ap_get_associated_sta_list
 * @param sta  输出 STA 列表
 * @return 0 成功,<0 失败
 */
int aw_wifi_ap_get_sta_list(aw_wifi_sta_list_t *sta);

/**
 * 把某个 STA 踢下线。
 * 底层对应:wlan_ap_disassociate_sta
 * @param mac  要踢的 STA 的 MAC 地址
 * @return 0 成功,<0 失败
 */
int aw_wifi_ap_deauth_sta(const uint8_t mac[AW_WIFI_MAC_LEN]);

/* ============================================================ *
 * P2P 接口
 * ============================================================ */

/**
 * 设置 P2P 设备名(CarPlay/Miracast 场景)。
 * 底层对应:user_p2p_setDN
 * @param device_name  设备名,如 "DIRECT-xx"
 * @return 0 成功,<0 失败
 */
int aw_wifi_p2p_set_device_name(const char *device_name);

/**
 * 设置 WFD(WiFi Display)类型。
 * 底层对应:user_set_wfd_type
 * @param type  0=source, 1=sink
 * @return 0 成功,<0 失败
 */
int aw_wifi_p2p_set_wfd_type(int type);

/**
 * 启用/禁用 WFD。
 * 底层对应:user_set_wfd_enable
 * @param enable  true=启用, false=禁用
 * @return 0 成功,<0 失败
 */
int aw_wifi_p2p_set_wfd_enable(bool enable);

/**
 * 触发 WPS PBC 按键(CarPlay 配对场景)。
 * 底层对应:user_wps_button_pushed
 * @return 0 成功,<0 失败
 */
int aw_wifi_p2p_wps_pbc(void);

/* ============================================================ *
 * 通用接口
 * ============================================================ */

/**
 * 取本机 MAC 地址。
 * 底层对应:驱动初始化时从 Wi-Fi 模组读取的地址
 * @param iface  选 STA / AP 接口
 * @param mac    输出 MAC 地址
 * @return 0 成功,<0 失败
 */
int aw_wifi_get_mac(aw_wifi_interface_t iface, uint8_t mac[AW_WIFI_MAC_LEN]);

/**
 * 取当前 IP 地址 / 子网掩码 / 网关。
 * 底层对应:netif_find("wl1") → netif_ip4_addr / netmask / gw
 * @param iface   选 STA / AP 接口(当前忽略,统一走 wl1)
 * @param ip      输出 IP 地址(网络字节序),传 NULL 不取
 * @param netmask 输出子网掩码(网络字节序),传 NULL 不取
 * @param gw      输出网关(网络字节序),传 NULL 不取
 * @return 0 成功,<0 失败(如 netif 不存在或未拿到 IP)
 */
int aw_wifi_get_ip(aw_wifi_interface_t iface, uint32_t *ip, uint32_t *netmask, uint32_t *gw);

/**
 * 注册事件回调(整个系统只注册一个,内部由适配层统一分发)。
 * 回调线程取决于事件来源；GOT_IP 由 aw_dhcp 监控任务上报。
 * 回调中不要执行长时间阻塞操作。
 * 底层对应:wifi_drv_event_set_cbk(适配层作为唯一注册者,把 ON_ASSOC /
 *           ON_ASSOC_FAIL / ON_DISASSOC / GOT_IP 翻译成上面的事件 id)
 * @param cb   回调函数,传 NULL 取消
 * @param arg  回调时原样回传的私有参数
 * @return 0 成功,<0 失败
 */
int aw_wifi_set_event_cb(aw_wifi_event_cb_t cb, void *arg);

/* ============================================================ *
 * DHCP / IP 管理接口
 * ============================================================ */

/**
 * STA 侧启动 DHCP 客户端,获取 IP。
 * 成功后抛 AW_IP_EVENT_STA_GOT_IP 事件。
 * 底层对应:dhcp_start(netif)
 * @return 0 成功,<0 失败
 */
/* Token-bound variants reject work from an earlier STA session. */
uint32_t aw_wifi_sta_session_token(void);
/* update must be short and must not block or call back into WiFi. */
bool aw_wifi_sta_session_update(uint32_t token, void (*update)(void *), void *arg);
int aw_wifi_sta_dhcpc_start_for_session(uint32_t token);
int aw_wifi_sta_dhcpc_stop_for_session(uint32_t token);

int aw_wifi_sta_dhcpc_start(void);

/**
 * STA 侧停止 DHCP 客户端。
 * 底层对应:dhcp_stop(netif)
 * @return 0 成功,<0 失败
 */
int aw_wifi_sta_dhcpc_stop(void);

/**
 * AP/P2P 侧设置本机 IP(必须在 dhcpd_start 之前调)。
 * 底层对应:netif_set_ipaddr / netif_set_netmask / netif_set_gw
 * @param ip       IP 地址,网络字节序(与 lwip ip4_addr_t 一致)
 * @param gateway  网关,网络字节序
 * @param netmask  子网掩码,网络字节序
 * @return 0 成功,<0 失败
 */
int aw_wifi_ap_set_ip(uint32_t ip, uint32_t gateway, uint32_t netmask);

/**
 * AP/P2P 侧启动 DHCP 服务器,给接入的 STA 分配 IP。
 * 底层对应:dhcpd_start
 * @return 0 成功,<0 失败
 */
int aw_wifi_ap_dhcpd_start(void);

/**
 * AP/P2P 侧停止 DHCP 服务器。
 * @return 0 成功,<0 失败
 */
int aw_wifi_ap_dhcpd_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* _AIC_WIFI_H_ */
