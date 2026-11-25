#ifndef DIVOOM_HA_H
#define DIVOOM_HA_H
/* 云端登录入口：仅接受当前固件支持的 http://主机[:端口]。
 * 返回 0 表示登录配置已保存，后台换取 access_token 后连接；不代表认证成功。
 * https/WSS 未实现时返回 -1，禁止静默降级。 */
int divoom_ha_login(const char *url, const char *client_id, const char *refresh_token);
#define DIVOOM_HA_TOKEN_MAX 2048
#define DIVOOM_HA_ENTITY_MAX 16
#define DIVOOM_HA_CLOCK_SLOT_COUNT 4
#define DIVOOM_HA_ENTITY_ID_MAX 128
#define DIVOOM_HA_STATE_MAX 256
#define DIVOOM_HA_REQUIREMENT_MAX 8

/* 云端下发的 HA 配置；Divoom 账号 Token 仅由服务器校验，不作为 HA 凭据。 */
typedef struct
{
    char ha_url[256];
    char ha_client_id[512];
    char ha_refresh_token[2048];
}divoom_home_assistant_login_info;

/* 云端命令框架回调；具体解析、应答和凭据释放由 HA 模块负责。 */
struct divoom_cJSON;
struct _divoom_net_command;
int divoom_ha_login_unpack(struct divoom_cJSON *root, struct _divoom_net_command *pcmd_info);
void divoom_ha_result_pack(struct divoom_cJSON *root, struct _divoom_net_command *pcmd_info);

/* Firmware watchface requirements; not supplied by the phone App.
 * Empty device_class matches all entities in the specified domain. */
struct divoom_ha_requirement
{
    char domain[32];
    char device_class[32];
};
struct divoom_ha_watchface
{
    unsigned requirement_count;
    struct divoom_ha_requirement requirements[DIVOOM_HA_REQUIREMENT_MAX];
};
/* Empty requirements stop watchface entity reads; the HA connection stays online. */
/*********************************************************************
  Function:        divoom_ha_set_watchface
  Description:     替换表盘需求并使旧缓存失效，后台只更新实体订阅，保留 HA 连接。
  Input:           watchface：domain/device_class 规则；空需求表示普通表盘无需实体。
  Return:          0：需求已接收；-1：未初始化或规则非法。
  调用约束:         内部加锁并复制规则；只改变表盘版本，不改变连接版本。
**********************************************************************/
int divoom_ha_set_watchface(const struct divoom_ha_watchface *watchface);
/* Enumerate matched IDs on the device without copying credentials. */
/*********************************************************************
  Function:        divoom_ha_get_entity_id
  Description:     按索引枚举匹配实体，供表盘再查询对应状态。
  Input:           index：从 0 开始；entity_id：输出；capacity：含结束符容量。
  Return:          0：成功；-1：越界、容量不足或参数错误，输出为空。
  调用约束:         内部加锁，不返回任何连接凭据。
**********************************************************************/
int divoom_ha_get_entity_id(unsigned index, char *entity_id, unsigned capacity);

struct divoom_ha_config
{
    char host[128];
    unsigned port;
    char access_token[DIVOOM_HA_TOKEN_MAX];
    /* Optional engineering overrides. App leaves these zeroed. */
    char temperature_entity[128];
    char scene_entity[128];
    unsigned entity_count;
    char entity_ids[DIVOOM_HA_ENTITY_MAX][DIVOOM_HA_ENTITY_ID_MAX];
};
struct divoom_ha_status
{
    int configured, online, temperature_valid, scene_pending;
    unsigned matched_count;
    unsigned temperature_candidates;
    int match_overflow;
    char entity_state[48];
    double temperature;
    char unit[16];
    char state[40];
    char action[48];
    char scene_entity[128];
};
/* Snapshot of one subscribed entity. State is kept as text for all domains.
 * received=0 after disconnect/removal; available=0 for unknown/unavailable.
 * truncated=1 means the original state exceeds the fixed cache capacity. */
struct divoom_ha_clock_bindings
{
    char entity_ids[DIVOOM_HA_CLOCK_SLOT_COUNT][DIVOOM_HA_ENTITY_ID_MAX];
    unsigned aux_slots; /* 由固件根据各槽的显示数据 2 元素设置。 */
};

enum divoom_ha_clock_action
{
    DIVOOM_HA_CLOCK_DECREASE = -1,
    DIVOOM_HA_CLOCK_TOGGLE,
    DIVOOM_HA_CLOCK_INCREASE
};

/* 读取子表盘 AppList[0].Config 的主实体绑定。 */
int divoom_ha_parse_clock_entity(const char *config, char entity_id[DIVOOM_HA_ENTITY_ID_MAX]);

/* NULL restores the normal entity policy. Slots are numbered 1 to 4. */
int divoom_ha_set_clock_bindings(const struct divoom_ha_clock_bindings *bindings);
void divoom_ha_set_clock_control(int enabled);
int divoom_ha_clock_control_enabled(void);
int divoom_ha_request_clock_action(unsigned slot, enum divoom_ha_clock_action action);
struct divoom_ha_entity_state;
int divoom_ha_get_clock_entity(unsigned slot, struct divoom_ha_entity_state *state,
    struct divoom_ha_entity_state *aux);

#define DIVOOM_HA_CURRENT_TEMP 1u
#define DIVOOM_HA_TARGET_TEMP 2u
#define DIVOOM_HA_FAN_PERCENTAGE 64u
#define DIVOOM_HA_COLOR_TEMP 128u
#define DIVOOM_HA_COVER_POSITION 256u

enum divoom_ha_fan_oscillation
{
    DIVOOM_HA_FAN_OSCILLATION_UNKNOWN,
    DIVOOM_HA_FAN_OSCILLATION_OFF,
    DIVOOM_HA_FAN_OSCILLATION_ON
};

struct divoom_ha_entity_state
{
    char entity_id[DIVOOM_HA_ENTITY_ID_MAX];
    char state[DIVOOM_HA_STATE_MAX];
    char unit[16];
    char name[128];
    char hvac_action[20];
    char device_class[32];
    char fan_preset[32];
    double current_temperature, target_temperature, min_temp, max_temp, target_temp_step;
    double fan_percentage, color_temp_kelvin, cover_position;
    unsigned numeric_attributes;
    unsigned supported_features;
    unsigned epoch;
    unsigned finish_time_utc;
    enum divoom_ha_fan_oscillation fan_oscillation;
    int brightness;
    int dimmable;
    int received;
    int available;
    int truncated;
};
/* Returns 0 for a configured entity, including one not received yet. */
/*********************************************************************
  Function:        divoom_ha_get_entity_state
  Description:     按实体 ID 取得缓存快照，保留字符串状态、单位及有效性标志。
  Input:           entity_id：实体 ID；state：输出结构。
  Return:          0：在匹配列表中；-1：参数错误、未初始化或未匹配。
  调用约束:         内部加锁；成功后仍须检查 received、available、truncated，不能把未知值当零。
**********************************************************************/
int divoom_ha_get_entity_state(const char *entity_id, struct divoom_ha_entity_state *state);
/* App sets host/port/access_token only: all entity fields zero means auto discovery.
 * entity_count=0 with temperature_entity preserves the legacy test configuration.
 * With an explicit list, temperature_entity is optional and must be in that list if set. */
/*********************************************************************
  Function:        divoom_ha_init
  Description:     分配核心状态、创建系统锁、设置默认温度需求并启动后台任务。
  Input:           无。
  Return:          0：成功或已初始化；-1：内存、锁或任务创建失败。
  调用约束:         启动阶段串行调用；任务栈使用配置宏，失败释放初始化资源。
**********************************************************************/
int divoom_ha_init(void);
/* Phone provisioning entry: no entity IDs or watchface rules required. */
/*********************************************************************
  Function:        divoom_ha_configure_connection
  Description:     App 简化配置入口，只传 HA 地址、端口和令牌，实体由设备自动发现。
  Input:           host：不带协议前缀的主机；port：端口；access_token：访问令牌。
  Return:          0：配置已接收；-1：参数、内存或配置校验失败。
  调用约束:         复制参数后调用方可释放缓冲；不打印或持久化令牌。
**********************************************************************/
int divoom_ha_configure_connection(const char *host, unsigned port, const char *access_token);
/*********************************************************************
  Function:        divoom_ha_configure
  Description:     校验并复制完整工程配置，清空旧实体缓存，通知后台按新凭据重连。
  Input:           c：连接配置，可带实体列表、温度和场景测试覆盖项。
  Return:          0：配置已接收；-1：未初始化或参数错误；不代表网络已连接。
  调用约束:         任意业务线程可调用，内部加锁；令牌仅保存在 RAM。
**********************************************************************/
int divoom_ha_configure(const struct divoom_ha_config *config);
/*********************************************************************
  Function:        divoom_ha_clear
  Description:     删除保存的登录配置并清空 RAM 凭据和实体缓存。
  Input:           无。
  Return:          0：清除成功；-1：未初始化或删除失败。
  调用约束:         内部加锁；后台关闭连接；不撤销 HA 服务端令牌。
**********************************************************************/
int divoom_ha_clear(void);
/*********************************************************************
  Function:        divoom_ha_get_status
  Description:     复制连接、实体订阅和场景状态，供界面或串口查询。
  Input:           s：输出快照，可为 NULL。
  Return:          无；未初始化时输出清零，快照不含令牌。
  调用约束:         内部加锁；调用方不可通过快照修改内部状态。
**********************************************************************/
void divoom_ha_get_status(struct divoom_ha_status *status);
/*********************************************************************
  Function:        divoom_ha_execute_scene
  Description:     把已绑定场景的执行请求交给后台任务，不在调用线程发送网络数据。
  Input:           无；使用配置中的 scene_entity。
  Return:          0：已排队；-1：未在线、未绑定、未初始化或已有请求未结束。
  调用约束:         内部加锁；结果从 status.action 查询，超时后不自动补发。
**********************************************************************/
int divoom_ha_execute_scene(void);
#endif
