/* HA LAN client: OAuth configuration, entity subscriptions and device actions. */
#include "divoom.h"
#include "divoom_ha.h"
#include "divoom_ha_wire.h"
#include "divoom_ha_http.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <stdint.h>

#define DIVOOM_HA_LOGIN_FILE DIVOOM_USER_SYSTEM_DIR "/ha_login.cfg"
#define DIVOOM_HA_LOGIN_TEMP DIVOOM_HA_LOGIN_FILE ".tmp"
#define DIVOOM_HA_LOGIN_BACKUP DIVOOM_HA_LOGIN_FILE ".bak"
#define DIVOOM_HA_LOGIN_FORMAT "HA01"

struct divoom_ha_login_record
{
    char format[sizeof(DIVOOM_HA_LOGIN_FORMAT) - 1];
    divoom_home_assistant_login_info login;
};

struct divoom_ha_oauth_config
{
    char url[256];
    char client_id[512];
    char refresh_token[DIVOOM_HA_TOKEN_MAX];
    unsigned refresh_at;
    int enabled;
};

struct divoom_ha_ctrl
{
    struct divoom_ha_oauth_config oauth;
    void *lock;
    void *task_handle;
    struct divoom_ha_config config;
    struct divoom_ha_status status;
    char temperature_unit[4];
    struct divoom_ha_watchface watchface;
    struct divoom_ha_config matched_config;
    struct divoom_ha_entity_state entities[DIVOOM_HA_ENTITY_MAX];
    unsigned revision;
    unsigned watchface_revision;
    int scene_requested;
    int clock_active;
    int clock_control;
    unsigned clock_action_slot;
    enum divoom_ha_clock_action clock_action;
    unsigned entity_epoch;
    struct divoom_ha_clock_bindings clock_bindings;
    char clock_aux_ids[DIVOOM_HA_CLOCK_SLOT_COUNT][DIVOOM_HA_ENTITY_ID_MAX];
};

static struct divoom_ha_ctrl *pdivoom_ha_handle = NULL;

static int divoom_ha_remove_login_file(const char *path)
{
    return divoom_port_file_exists(path) ? divoom_port_remove_file(path) : 0;
}

/* Called with the HA lock held. FatFs rename cannot replace an existing file. */
static int divoom_ha_save_login(const char *url, const char *client_id, const char *refresh_token)
{
    struct divoom_ha_login_record *record;
    void *file;
    int result = -1;

    if(divoom_port_mkdir_p(DIVOOM_USER_SYSTEM_DIR))
        return -1;
    record = divoom_system_malloc(sizeof(*record));
    if(!record)
        return -1;
    memset(record, 0, sizeof(*record));
    memcpy(record->format, DIVOOM_HA_LOGIN_FORMAT, sizeof(record->format));
    strcpy(record->login.ha_url, url);
    strcpy(record->login.ha_client_id, client_id);
    strcpy(record->login.ha_refresh_token, refresh_token);
    file = divoom_port_file_open(DIVOOM_HA_LOGIN_TEMP, "wb");
    if(file)
    {
        if(divoom_port_file_write(record, 1, sizeof(*record), file) == sizeof(*record) &&
            divoom_port_file_sync(file) == 0)
            result = 0;
        if(divoom_port_file_close(file))
            result = -1;
    }
    memset(record, 0, sizeof(*record));
    divoom_system_free(record);
    if(!result && divoom_port_rename_file(DIVOOM_HA_LOGIN_TEMP, DIVOOM_HA_LOGIN_FILE))
    {
        result = -1;
        if(divoom_port_file_exists(DIVOOM_HA_LOGIN_FILE) &&
            !divoom_ha_remove_login_file(DIVOOM_HA_LOGIN_BACKUP) &&
            !divoom_port_rename_file(DIVOOM_HA_LOGIN_FILE, DIVOOM_HA_LOGIN_BACKUP))
        {
            result = divoom_port_rename_file(DIVOOM_HA_LOGIN_TEMP, DIVOOM_HA_LOGIN_FILE);
            if(result)
                divoom_port_rename_file(DIVOOM_HA_LOGIN_BACKUP, DIVOOM_HA_LOGIN_FILE);
        }
    }
    if(result)
        divoom_ha_remove_login_file(DIVOOM_HA_LOGIN_TEMP);
    else
        divoom_ha_remove_login_file(DIVOOM_HA_LOGIN_BACKUP);
    return result;
}

/*********************************************************************
  Function:        divoom_ha_json_get_string
  Description:     读取 JSON 字符串字段，不分配或复制字符串。
  Input:           r：JSON 对象；key：字段名。
  Return:          字段正确时返回内部字符串指针，缺失或类型错误返回 NULL；指针随 JSON 释放失效。
  调用约束:         调用方保证 JSON 对象在使用期间有效。
**********************************************************************/
static const char *divoom_ha_json_get_string(divoom_cJSON *r, const char *key)
{
    divoom_cJSON *v = divoom_cJSON_GetObjectItem(r, key);
    return v && (v->type & 255) == divoom_cJSON_String ? v->valuestring : NULL;
}
/*********************************************************************
  Function:        divoom_ha_json_string_equals
  Description:     比较 JSON 字符串字段与指定文本是否完全相同。
  Input:           r：JSON 对象；key：字段名；s：比较文本。
  Return:          1：相等；0：缺失、类型错误或不相等。
  调用约束:         不修改共享状态。
**********************************************************************/
static int divoom_ha_json_string_equals(divoom_cJSON *r, const char *key, const char *s)
{
    const char *v = divoom_ha_json_get_string(r, key);
    return v && !strcmp(v, s);
}
/*********************************************************************
  Function:        divoom_ha_lock
  Description:     取得 HA 状态锁，保护连接配置、实体缓存及对外状态。
  Input:           无。
  Return:          无。
  调用约束:         须与 divoom_ha_unlock 配对，不允许重复取得同一把锁。
**********************************************************************/
static void divoom_ha_lock(void)
{
    divoom_system_get_lock(pdivoom_ha_handle->lock);
}
/*********************************************************************
  Function:        divoom_ha_unlock
  Description:     释放 HA 控制结构的状态锁。
  Input:           无。
  Return:          无。
  调用约束:         由取得该锁的线程调用。
**********************************************************************/
static void divoom_ha_unlock(void)
{
    divoom_system_free_lock(pdivoom_ha_handle->lock);
}
/*********************************************************************
  Function:        divoom_ha_set_connection_state
  Description:     更新连接状态，离线时使缓存失效并清除待执行场景。
  Input:           s：状态说明；online：是否已认证在线。
  Return:          无。
  调用约束:         内部加锁；在线不等于实体已订阅成功。
**********************************************************************/
static void divoom_ha_set_connection_state(const char *s, int online)
{
    divoom_ha_lock();
    snprintf(pdivoom_ha_handle->status.state, sizeof(pdivoom_ha_handle->status.state), "%s", s);
    pdivoom_ha_handle->status.online = online;
    if(!online)
    {
        pdivoom_ha_handle->temperature_unit[0] = 0;
        memset(pdivoom_ha_handle->entities, 0, sizeof(pdivoom_ha_handle->entities));
        memset(pdivoom_ha_handle->clock_aux_ids, 0, sizeof(pdivoom_ha_handle->clock_aux_ids));
        pdivoom_ha_handle->status.temperature_valid = 0;
        pdivoom_ha_handle->status.scene_pending = 0;
        pdivoom_ha_handle->scene_requested = 0;
        pdivoom_ha_handle->clock_action_slot = 0;
    }
    divoom_ha_unlock();
}
/*********************************************************************
  Function:        divoom_ha_set_action_state
  Description:     更新最近一次场景请求的状态及是否仍在等待结果。
  Input:           s：动作状态说明；pending：是否待处理。
  Return:          无。
  调用约束:         内部加锁；HA 接受请求不等于真实设备已达到目标。
**********************************************************************/
static void divoom_ha_set_action_state(const char *s, int pending)
{
    divoom_ha_lock();
    snprintf(pdivoom_ha_handle->status.action, sizeof(pdivoom_ha_handle->status.action), "%s", s);
    pdivoom_ha_handle->status.scene_pending = pending;
    divoom_ha_unlock();
}
/*********************************************************************
  Function:        divoom_ha_config_is_current
  Description:     判断当前连接使用的配置是否仍有效，避免改配后继续使用旧连接。
  Input:           rev：建立连接时保存的凭据配置版本。
  Return:          1：仍启用且版本一致；0：已清除或替换。
  调用约束:         内部加锁；表盘版本变化不影响此连接版本。
**********************************************************************/
static int divoom_ha_config_is_current(unsigned rev)
{
    int same;
    divoom_ha_lock();
    same = pdivoom_ha_handle->status.configured && rev == pdivoom_ha_handle->revision;
    divoom_ha_unlock();
    return same;
}
/*********************************************************************
  Function:        divoom_ha_validate_entity
  Description:     校验实体是否属于指定 domain，且长度和名称字符合法。
  Input:           s：实体 ID；domain：要求的域，例如 scene。
  Return:          1：合法；0：不合法。
  调用约束:         调用方先保证 s 是正确终止的字符串。
**********************************************************************/
static int divoom_ha_validate_entity(const char *s, const char *domain)
{
    size_t i, n = strlen(s), d = strlen(domain);
    if(n <= d + 1 || n >= 128 || strncmp(s, domain, d) || s[d] != '.')
        return 0;
    for(i = d + 1; i < n; i++)
        if(!islower((unsigned char)s[i]) && !isdigit((unsigned char)s[i]) && s[i] != '_')
            return 0;
    return 1;
}
/*********************************************************************
  Function:        divoom_ha_validate_entity_id
  Description:     校验通用 domain.object_id 格式，不限制设备类型。
  Input:           entity：实体 ID。
  Return:          1：合法；0：空值、分隔符或字符错误。
  调用约束:         字符串须正确终止；字段容量由配置入口检查。
**********************************************************************/
static int divoom_ha_validate_entity_id(const char *entity)
{
    size_t i;
    int separator = 0;
    if(!entity || !entity[0])
        return 0;
    for(i = 0; entity[i]; i++)
    {
        if(entity[i] == '.' && i && entity[i + 1] && !separator)
            separator = 1;
        else if(!(entity[i] >= 'a' && entity[i] <= 'z') && !(entity[i] >= '0' && entity[i] <= '9') && entity[i] != '_')
            return 0;
    }
    return separator;
}

/*********************************************************************
  Function:        divoom_ha_configure
  Description:     校验并复制完整工程配置，清空旧实体缓存，通知后台按新凭据重连。
  Input:           c：连接配置，可带实体列表、温度和场景测试覆盖项。
  Return:          0：配置已接收；-1：未初始化或参数错误；不代表网络已连接。
  调用约束:         任意业务线程可调用，内部加锁；令牌仅保存在 RAM。
**********************************************************************/
int divoom_ha_configure(const struct divoom_ha_config *c)
{
    size_t i, j, n;
    int temperature_found = 0;
    if(!pdivoom_ha_handle || !pdivoom_ha_handle->lock || !c || !memchr(c->host, 0, sizeof(c->host)) || !memchr(c->access_token, 0, sizeof(c->access_token)) || !memchr(c->temperature_entity, 0, sizeof(c->temperature_entity)) || !memchr(c->scene_entity, 0, sizeof(c->scene_entity)))
        return -1;
    n = strlen(c->host);
    if(!n || !c->port || c->port > 65535 || !c->access_token[0])
        return -1;
    for(i = 0; i < n; i++)
        if(!isalnum((unsigned char)c->host[i]) && c->host[i] != '.' && c->host[i] != '-')
            return -1;
    if(c->entity_count > DIVOOM_HA_ENTITY_MAX)
        return -1;
    for(i = 0; i < c->entity_count; i++)
    {
        if(!memchr(c->entity_ids[i], 0, sizeof(c->entity_ids[i])) || !divoom_ha_validate_entity_id(c->entity_ids[i]))
            return -1;
        for(j = 0; j < i; j++)
            if(!strcmp(c->entity_ids[i], c->entity_ids[j]))
                return -1;
        if(!strcmp(c->entity_ids[i], c->temperature_entity))
            temperature_found = 1;
    }
    if(c->temperature_entity[0] && !divoom_ha_validate_entity(c->temperature_entity, "sensor") && !divoom_ha_validate_entity(c->temperature_entity, "input_number"))
        return -1;
    if(c->entity_count && c->temperature_entity[0] && !temperature_found)
        return -1;
    if(c->scene_entity[0] && !divoom_ha_validate_entity(c->scene_entity, "scene"))
        return -1;
    divoom_ha_lock();
    memset(&pdivoom_ha_handle->oauth, 0, sizeof(pdivoom_ha_handle->oauth));
    memcpy(&pdivoom_ha_handle->config, c, sizeof(*c));
    if(!pdivoom_ha_handle->config.entity_count && c->temperature_entity[0])
    {
        pdivoom_ha_handle->config.entity_count = 1;
        strcpy(pdivoom_ha_handle->config.entity_ids[0], c->temperature_entity);
    }
    memset(pdivoom_ha_handle->entities, 0, sizeof(pdivoom_ha_handle->entities));
    memset(&pdivoom_ha_handle->matched_config, 0, sizeof(pdivoom_ha_handle->matched_config));
    pdivoom_ha_handle->revision++;
    pdivoom_ha_handle->watchface_revision++;
    pdivoom_ha_handle->scene_requested = 0;
    pdivoom_ha_handle->matched_config = pdivoom_ha_handle->config;
    memset(&pdivoom_ha_handle->status, 0, sizeof(pdivoom_ha_handle->status));
    pdivoom_ha_handle->status.matched_count = pdivoom_ha_handle->matched_config.entity_count;
    pdivoom_ha_handle->status.configured = 1;
    snprintf(pdivoom_ha_handle->status.scene_entity, sizeof(pdivoom_ha_handle->status.scene_entity), "%s", c->scene_entity);
    strcpy(pdivoom_ha_handle->status.state, "Waiting for Wi-Fi");
    divoom_ha_unlock();
    return 0;
}
/*********************************************************************
  Function:        divoom_ha_configure_connection
  Description:     App 简化配置入口，只传 HA 地址、端口和令牌，实体由设备自动发现。
  Input:           host：不带协议前缀的主机；port：端口；access_token：访问令牌。
  Return:          0：配置已接收；-1：参数、内存或配置校验失败。
  调用约束:         复制参数后调用方可释放缓冲；不打印或持久化令牌。
**********************************************************************/
int divoom_ha_configure_connection(const char *host, unsigned port, const char *access_token)
{
    struct divoom_ha_config *config;
    int rc;
    if(!host || !access_token || strlen(host) >= sizeof(config->host) || strlen(access_token) >= sizeof(config->access_token))
        return -1;
    config = divoom_system_malloc(sizeof(*config));
    if(!config)
        return -1;
    memset(config, 0, sizeof(*config));
    strcpy(config->host, host);
    config->port = port;
    strcpy(config->access_token, access_token);
    rc = divoom_ha_configure(config);
    memset(config, 0, sizeof(*config));
    divoom_system_free(config);
    return rc;
}

/*********************************************************************
  Function:        divoom_ha_clear
  Description:     删除保存的登录配置并清空 RAM 凭据和实体缓存。
  Input:           无。
  Return:          0：清除成功；-1：未初始化或删除失败。
  调用约束:         内部加锁；后台关闭连接；不撤销 HA 服务端令牌。
**********************************************************************/
int divoom_ha_clear(void)
{
    if(!pdivoom_ha_handle || !pdivoom_ha_handle->lock)
        return -1;
    divoom_ha_lock();
    /* Remove recovery files first so logout cannot restore an older login. */
    if(divoom_ha_remove_login_file(DIVOOM_HA_LOGIN_TEMP) ||
        divoom_ha_remove_login_file(DIVOOM_HA_LOGIN_BACKUP) ||
        divoom_ha_remove_login_file(DIVOOM_HA_LOGIN_FILE))
    {
        divoom_ha_unlock();
        divoom_print("ha: cannot remove saved login\n");
        return -1;
    }
    pdivoom_ha_handle->temperature_unit[0] = 0;
    memset(&pdivoom_ha_handle->oauth, 0, sizeof(pdivoom_ha_handle->oauth));
    memset(&pdivoom_ha_handle->config, 0, sizeof(pdivoom_ha_handle->config));
    memset(&pdivoom_ha_handle->matched_config, 0, sizeof(pdivoom_ha_handle->matched_config));
    memset(pdivoom_ha_handle->entities, 0, sizeof(pdivoom_ha_handle->entities));
    memset(&pdivoom_ha_handle->status, 0, sizeof(pdivoom_ha_handle->status));
    pdivoom_ha_handle->scene_requested = 0;
    pdivoom_ha_handle->clock_action_slot = 0;
    pdivoom_ha_handle->revision++;
    pdivoom_ha_handle->watchface_revision++;
    strcpy(pdivoom_ha_handle->status.state, "Not configured");
    divoom_ha_unlock();
    return 0;
}
/*********************************************************************
  Function:        divoom_ha_get_status
  Description:     复制连接、实体订阅和场景状态，供界面或串口查询。
  Input:           s：输出快照，可为 NULL。
  Return:          无；未初始化时输出清零，快照不含令牌。
  调用约束:         内部加锁；调用方不可通过快照修改内部状态。
**********************************************************************/
void divoom_ha_get_status(struct divoom_ha_status *s)
{
    if(!s)
        return;
    memset(s, 0, sizeof(*s));
    if(!pdivoom_ha_handle || !pdivoom_ha_handle->lock)
        return;
    divoom_ha_lock();
    *s = pdivoom_ha_handle->status;
    divoom_ha_unlock();
}
/*********************************************************************
  Function:        divoom_ha_execute_scene
  Description:     把已绑定场景的执行请求交给后台任务，不在调用线程发送网络数据。
  Input:           无；使用配置中的 scene_entity。
  Return:          0：已排队；-1：未在线、未绑定、未初始化或已有请求未结束。
  调用约束:         内部加锁；结果从 status.action 查询，超时后不自动补发。
**********************************************************************/
int divoom_ha_execute_scene(void)
{
    int rc = -1;
    if(!pdivoom_ha_handle || !pdivoom_ha_handle->lock)
        return rc;
    divoom_ha_lock();
    if(pdivoom_ha_handle->status.online && pdivoom_ha_handle->config.scene_entity[0] && !pdivoom_ha_handle->status.scene_pending && !pdivoom_ha_handle->scene_requested)
    {
        pdivoom_ha_handle->scene_requested = 1;
        pdivoom_ha_handle->status.scene_pending = 1;
        strcpy(pdivoom_ha_handle->status.action, "Queued");
        rc = 0;
    }
    divoom_ha_unlock();
    return rc;
}
/*********************************************************************
  Function:        divoom_ha_get_entity_state
  Description:     按实体 ID 取得缓存快照，保留字符串状态、单位及有效性标志。
  Input:           entity_id：实体 ID；state：输出结构。
  Return:          0：在匹配列表中；-1：参数错误、未初始化或未匹配。
  调用约束:         内部加锁；成功后仍须检查 received、available、truncated，不能把未知值当零。
**********************************************************************/
int divoom_ha_get_entity_state(const char *entity_id, struct divoom_ha_entity_state *state)
{
    unsigned i;
    int rc = -1;
    if(!state)
        return -1;
    memset(state, 0, sizeof(*state));
    if(!pdivoom_ha_handle || !entity_id)
        return -1;
    divoom_ha_lock();
    for(i = 0; i < pdivoom_ha_handle->matched_config.entity_count; i++)
    {
        if(strcmp(entity_id, pdivoom_ha_handle->matched_config.entity_ids[i]))
            continue;
        *state = pdivoom_ha_handle->entities[i];
        snprintf(state->entity_id, sizeof(state->entity_id), "%s", entity_id);
        rc = 0;
        break;
    }
    divoom_ha_unlock();
    return rc;
}

/*********************************************************************
  Function:        divoom_ha_cache_entity_state
  Description:     合并紧凑状态的 s/a 字段，属性增量不会覆盖已有状态。
  Input:           index：缓存槽；root：紧凑状态对象；snapshot：1 为完整快照，0 为增量。
  Return:          无。
  调用约束:         调用方必须已持有 HA 锁；超长状态被截断后标记为不可用。
**********************************************************************/
enum divoom_ha_numeric_attribute
{
    DIVOOM_HA_MIN_TEMP = 4,
    DIVOOM_HA_MAX_TEMP = 8,
    DIVOOM_HA_TEMP_STEP = 16,
    DIVOOM_HA_BRIGHTNESS = 32
};

static void divoom_ha_cache_number(divoom_cJSON *attrs, const char *key,
    struct divoom_ha_entity_state *state, unsigned flag, double *out)
{
    divoom_cJSON *value = divoom_cJSON_GetObjectItem(attrs, key);
    if(!value)
        return;
    state->numeric_attributes &= ~flag;
    if((value->type & 255) == divoom_cJSON_Number && isfinite(value->valuedouble))
    {
        *out = value->valuedouble;
        state->numeric_attributes |= flag;
    }
}

static void divoom_ha_cache_attributes(struct divoom_ha_entity_state *state, divoom_cJSON *attrs)
{
    const char *name = divoom_ha_json_get_string(attrs, "friendly_name");
    const char *action = divoom_ha_json_get_string(attrs, "hvac_action");
    const char *unit = divoom_ha_json_get_string(attrs, "unit_of_measurement");
    const char *device_class = divoom_ha_json_get_string(attrs, "device_class");
    divoom_cJSON *value, *modes;
    double brightness = 0;
    int i;

    if(name || divoom_cJSON_GetObjectItem(attrs, "friendly_name"))
        snprintf(state->name, sizeof(state->name), "%s", name ? name : "");
    if(action || divoom_cJSON_GetObjectItem(attrs, "hvac_action"))
        snprintf(state->hvac_action, sizeof(state->hvac_action), "%s", action ? action : "");
    if(unit || divoom_cJSON_GetObjectItem(attrs, "unit_of_measurement"))
        snprintf(state->unit, sizeof(state->unit), "%s", unit ? unit : "");
    divoom_ha_cache_number(attrs, "current_temperature", state, DIVOOM_HA_CURRENT_TEMP, &state->current_temperature);
    if(divoom_cJSON_GetObjectItem(attrs, "temperature"))
    {
        divoom_ha_cache_number(attrs, "temperature", state, DIVOOM_HA_TARGET_TEMP, &state->target_temperature);
        if(state->numeric_attributes & DIVOOM_HA_TARGET_TEMP)
            divoom_print("ha action: reported target=%.2f\n", state->target_temperature);
    }
    divoom_ha_cache_number(attrs, "min_temp", state, DIVOOM_HA_MIN_TEMP, &state->min_temp);
    divoom_ha_cache_number(attrs, "max_temp", state, DIVOOM_HA_MAX_TEMP, &state->max_temp);
    divoom_ha_cache_number(attrs, "target_temp_step", state, DIVOOM_HA_TEMP_STEP, &state->target_temp_step);
    if(device_class || divoom_cJSON_GetObjectItem(attrs, "device_class"))
        snprintf(state->device_class, sizeof(state->device_class), "%s", device_class ? device_class : "");
    divoom_ha_cache_number(attrs, "percentage", state, DIVOOM_HA_FAN_PERCENTAGE, &state->fan_percentage);
    if(state->fan_percentage < 0 || state->fan_percentage > 100)
        state->numeric_attributes &= ~DIVOOM_HA_FAN_PERCENTAGE;
    divoom_ha_cache_number(attrs, "color_temp_kelvin", state, DIVOOM_HA_COLOR_TEMP, &state->color_temp_kelvin);
    divoom_ha_cache_number(attrs, "current_position", state, DIVOOM_HA_COVER_POSITION, &state->cover_position);
    value = divoom_cJSON_GetObjectItem(attrs, "oscillating");
    if(value)
    {
        int type = value->type & 255;
        state->fan_oscillation = type == divoom_cJSON_True ? DIVOOM_HA_FAN_OSCILLATION_ON :
            type == divoom_cJSON_False ? DIVOOM_HA_FAN_OSCILLATION_OFF : DIVOOM_HA_FAN_OSCILLATION_UNKNOWN;
    }
    value = divoom_cJSON_GetObjectItem(attrs, "preset_mode");
    if(value)
    {
        const char *preset = (value->type & 255) == divoom_cJSON_String ? value->valuestring : NULL;
        snprintf(state->fan_preset, sizeof(state->fan_preset), "%s",
            preset && strlen(preset) < sizeof(state->fan_preset) &&
            strcmp(preset, "unknown") && strcmp(preset, "unavailable") ? preset : "");
    }
    if(divoom_cJSON_GetObjectItem(attrs, "brightness"))
    {
        divoom_ha_cache_number(attrs, "brightness", state, DIVOOM_HA_BRIGHTNESS, &brightness);
        state->brightness = brightness >= 1 && brightness <= 255 ? (int)brightness : 0;
    }
    value = divoom_cJSON_GetObjectItem(attrs, "supported_features");
    if(value)
        state->supported_features = (value->type & 255) == divoom_cJSON_Number && value->valueint >= 0 ? (unsigned)value->valueint : 0;
    modes = divoom_cJSON_GetObjectItem(attrs, "supported_color_modes");
    if(modes)
    {
        state->dimmable = 0;
        for(i = 0; i < divoom_cJSON_GetArraySize(modes); i++)
        {
            value = divoom_cJSON_GetArrayItem(modes, i);
            if(value && (value->type & 255) == divoom_cJSON_String && value->valuestring &&
                strcmp(value->valuestring, "onoff") && strcmp(value->valuestring, "unknown"))
                state->dimmable = 1;
        }
    }
}

/* timestamp 传感器用 ISO 8601，转换一次供表盘按分钟倒计时。 */
static unsigned divoom_ha_parse_timestamp(const char *text)
{
    static const int month_days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    struct tm utc = {0};
    const char *zone;
    int year, month, day, length = 0, offset = 0;
    int64_t seconds;

    if(sscanf(text, "%4d-%2d-%2dT%2d:%2d:%2d%n", &year, &month, &day,
        &utc.tm_hour, &utc.tm_min, &utc.tm_sec, &length) != 6 || length != 19 ||
        year < 1970 || year > 2106 || month < 1 || month > 12 || day < 1 ||
        day > month_days[month - 1] + (month == 2 && year % 4 == 0 && (year % 100 || year % 400 == 0)) ||
        utc.tm_hour < 0 || utc.tm_hour > 23 || utc.tm_min < 0 || utc.tm_min > 59 ||
        utc.tm_sec < 0 || utc.tm_sec > 59)
        return 0;
    zone = text + length;
    if(*zone == '.')
    {
        size_t fraction = strspn(++zone, "0123456789");
        if(!fraction)
            return 0;
        zone += fraction;
    }
    if(*zone == '+' || *zone == '-')
    {
        int hour, minute;
        length = 0;
        if(sscanf(zone + 1, "%2d:%2d%n", &hour, &minute, &length) != 2 ||
            length != 5 || zone[6] || hour < 0 || hour > 23 || minute < 0 || minute > 59)
            return 0;
        offset = (hour * 60 + minute) * 60 * (*zone == '+' ? 1 : -1);
    }
    else if(strcmp(zone, "Z"))
        return 0;
    utc.tm_year = year - 1900;
    utc.tm_mon = month - 1;
    utc.tm_mday = day;
    seconds = (int64_t)divoom_timelib_timegm(&utc) - offset;
    return seconds > 0 && seconds <= UINT32_MAX ? (unsigned)seconds : 0;
}

static void divoom_ha_cache_entity_state(unsigned index, divoom_cJSON *root, int snapshot)
{
    struct divoom_ha_entity_state *state = &pdivoom_ha_handle->entities[index];
    divoom_cJSON *attrs = divoom_cJSON_GetObjectItem(root, "a");
    const char *raw = divoom_ha_json_get_string(root, "s");
    if(snapshot)
    {
        /* 已保留的实体接收新订阅快照时，保持现有播放进度。 */
        unsigned epoch = state->available ? state->epoch : ++pdivoom_ha_handle->entity_epoch;
        memset(state, 0, sizeof(*state));
        state->epoch = epoch;
    }
    if(raw)
    {
        if(!snapshot && !state->available)
            state->epoch = ++pdivoom_ha_handle->entity_epoch;
        snprintf(state->state, sizeof(state->state), "%s", raw);
        state->received = 1;
        state->truncated = strlen(raw) >= sizeof(state->state);
        state->available = !state->truncated && strcmp(raw, "unknown") && strcmp(raw, "unavailable");
    }
    else if(divoom_cJSON_GetObjectItem(root, "s"))
    {
        state->received = 0;
        state->available = 0;
    }
    if(attrs)
        divoom_ha_cache_attributes(state, attrs);
    if(raw || attrs)
        state->finish_time_utc = state->available && !strcmp(state->device_class, "timestamp") ?
            divoom_ha_parse_timestamp(state->state) : 0;
}

/*********************************************************************
  Function:        divoom_ha_handle_entity_event
  Description:     处理实体新增 a、变化 c、删除 r，并同步温度测试值。
  Input:           r：完整 WebSocket event；config 和 revision：订阅配置与版本。
  Return:          无。
  调用约束:         后台任务调用，内部加锁；忽略已被替换的订阅消息。
**********************************************************************/
static void divoom_ha_handle_entity_event(divoom_cJSON *r, const struct divoom_ha_config *config, unsigned revision)
{
    divoom_cJSON *event = divoom_cJSON_GetObjectItem(r, "event");
    divoom_cJSON *added = divoom_cJSON_GetObjectItem(event, "a");
    divoom_cJSON *changed = divoom_cJSON_GetObjectItem(event, "c");
    divoom_cJSON *removed = divoom_cJSON_GetObjectItem(event, "r");
    divoom_cJSON *value, *plus, *deleted_attrs;
    unsigned i;
    int j;
    /* Ignore messages from a connection whose configuration was replaced. */
    divoom_ha_lock();
    if(revision != pdivoom_ha_handle->watchface_revision ||
        memcmp(config, &pdivoom_ha_handle->matched_config, sizeof(*config)))
    {
        divoom_ha_unlock();
        return;
    }
    for(i = 0; i < config->entity_count; i++)
    {
        const char *entity = config->entity_ids[i];
        value = divoom_cJSON_GetObjectItem(added, entity);
        if(value)
            divoom_ha_cache_entity_state(i, value, 1);
        value = divoom_cJSON_GetObjectItem(changed, entity);
        plus = divoom_cJSON_GetObjectItem(value, "+");
        if(plus)
            divoom_ha_cache_entity_state(i, plus, 0);
        deleted_attrs = divoom_cJSON_GetObjectItem(divoom_cJSON_GetObjectItem(value, "-"), "a");
        for(j = 0; j < divoom_cJSON_GetArraySize(deleted_attrs); j++)
        {
            value = divoom_cJSON_GetArrayItem(deleted_attrs, j);
            struct divoom_ha_entity_state *state = &pdivoom_ha_handle->entities[i];
            const char *key = value && (value->type & 255) == divoom_cJSON_String ? value->valuestring : NULL;
            if(!key)
                continue;
            if(!strcmp(key, "unit_of_measurement")) state->unit[0] = 0;
            else if(!strcmp(key, "friendly_name")) state->name[0] = 0;
            else if(!strcmp(key, "hvac_action")) state->hvac_action[0] = 0;
            else if(!strcmp(key, "device_class")) state->device_class[0] = 0;
            else if(!strcmp(key, "percentage")) state->numeric_attributes &= ~DIVOOM_HA_FAN_PERCENTAGE;
            else if(!strcmp(key, "oscillating")) state->fan_oscillation = DIVOOM_HA_FAN_OSCILLATION_UNKNOWN;
            else if(!strcmp(key, "preset_mode")) state->fan_preset[0] = 0;
            else if(!strcmp(key, "color_temp_kelvin")) state->numeric_attributes &= ~DIVOOM_HA_COLOR_TEMP;
            else if(!strcmp(key, "current_position")) state->numeric_attributes &= ~DIVOOM_HA_COVER_POSITION;
            else if(!strcmp(key, "current_temperature")) state->numeric_attributes &= ~DIVOOM_HA_CURRENT_TEMP;
            else if(!strcmp(key, "temperature")) state->numeric_attributes &= ~DIVOOM_HA_TARGET_TEMP;
            else if(!strcmp(key, "min_temp")) state->numeric_attributes &= ~DIVOOM_HA_MIN_TEMP;
            else if(!strcmp(key, "max_temp")) state->numeric_attributes &= ~DIVOOM_HA_MAX_TEMP;
            else if(!strcmp(key, "target_temp_step")) state->numeric_attributes &= ~DIVOOM_HA_TEMP_STEP;
            else if(!strcmp(key, "brightness")) { state->numeric_attributes &= ~DIVOOM_HA_BRIGHTNESS; state->brightness = 0; }
            else if(!strcmp(key, "supported_color_modes")) state->dimmable = 0;
            else if(!strcmp(key, "supported_features")) state->supported_features = 0;
        }
        for(j = 0; j < divoom_cJSON_GetArraySize(removed); j++)
        {
            value = divoom_cJSON_GetArrayItem(removed, j);
            if(value && (value->type & 255) == divoom_cJSON_String && value->valuestring && !strcmp(value->valuestring, entity))
                memset(&pdivoom_ha_handle->entities[i], 0, sizeof(pdivoom_ha_handle->entities[i]));
        }
        if(config->temperature_entity[0] && !strcmp(entity, config->temperature_entity))
        {
            struct divoom_ha_entity_state *state = &pdivoom_ha_handle->entities[i];
            char *end;
            double temperature = strtod(state->state, &end);
            pdivoom_ha_handle->status.temperature_valid = state->received && state->available && end != state->state && !*end && isfinite(temperature);
            if(pdivoom_ha_handle->status.temperature_valid)
                pdivoom_ha_handle->status.temperature = temperature;
            snprintf(pdivoom_ha_handle->status.unit, sizeof(pdivoom_ha_handle->status.unit), "%s", state->unit);
        }
    }
    divoom_ha_unlock();
}
/*********************************************************************
  Function:        divoom_ha_get_entity_id
  Description:     按索引枚举匹配实体，供表盘再查询对应状态。
  Input:           index：从 0 开始；entity_id：输出；capacity：含结束符容量。
  Return:          0：成功；-1：越界、容量不足或参数错误，输出为空。
  调用约束:         内部加锁，不返回任何连接凭据。
**********************************************************************/
int divoom_ha_get_entity_id(unsigned index, char *entity_id, unsigned capacity)
{
    int rc = -1;
    if(!entity_id || !capacity)
        return -1;
    entity_id[0] = 0;
    if(!pdivoom_ha_handle)
        return -1;
    divoom_ha_lock();
    if(index < pdivoom_ha_handle->matched_config.entity_count && strlen(pdivoom_ha_handle->matched_config.entity_ids[index]) < capacity)
    {
        strcpy(entity_id, pdivoom_ha_handle->matched_config.entity_ids[index]);
        rc = 0;
    }
    divoom_ha_unlock();
    return rc;
}

/*********************************************************************
  Function:        divoom_ha_set_watchface
  Description:     替换表盘需求并使旧缓存失效，后台只更新实体订阅，保留通知连接。
  Input:           watchface：domain/device_class 规则；空需求表示普通表盘无需实体。
  Return:          0：需求已接收；-1：未初始化或规则非法。
  调用约束:         内部加锁并复制规则；只改变表盘版本，不改变连接版本。
**********************************************************************/
int divoom_ha_set_watchface(const struct divoom_ha_watchface *watchface)
{
    unsigned i;
    size_t j;
    if(!pdivoom_ha_handle || !watchface || watchface->requirement_count > DIVOOM_HA_REQUIREMENT_MAX)
        return -1;
    for(i = 0; i < watchface->requirement_count; i++)
    {
        const struct divoom_ha_requirement *requirement = &watchface->requirements[i];
        if(!memchr(requirement->domain, 0, sizeof(requirement->domain)) || !requirement->domain[0] || !memchr(requirement->device_class, 0, sizeof(requirement->device_class)))
            return -1;
        for(j = 0; requirement->domain[j]; j++)
            if(!(requirement->domain[j] >= 'a' && requirement->domain[j] <= 'z') && requirement->domain[j] != '_')
                return -1;
    }
    divoom_ha_lock();
    pdivoom_ha_handle->watchface = *watchface;
    /* A watchface change replaces engineering overrides with automatic matching. */
    pdivoom_ha_handle->config.entity_count = 0;
    pdivoom_ha_handle->config.temperature_entity[0] = 0;
    memset(pdivoom_ha_handle->config.entity_ids, 0, sizeof(pdivoom_ha_handle->config.entity_ids));
    memset(&pdivoom_ha_handle->matched_config, 0, sizeof(pdivoom_ha_handle->matched_config));
    memset(pdivoom_ha_handle->entities, 0, sizeof(pdivoom_ha_handle->entities));
    pdivoom_ha_handle->status.matched_count = 0;
    pdivoom_ha_handle->status.temperature_candidates = 0;
    pdivoom_ha_handle->status.match_overflow = 0;
    pdivoom_ha_handle->status.temperature_valid = 0;
    pdivoom_ha_handle->watchface_revision++;
    strcpy(pdivoom_ha_handle->status.entity_state, "Watchface changed");
    divoom_ha_unlock();
    return 0;
}

/*********************************************************************
  Function:        divoom_ha_entity_matches
  Description:     逐条匹配表盘规则，命中任意一条即通过；温度可在缺少 device_class 时按 °C/°F 识别。
  Input:           entity：实体 ID；attrs：HA attributes；watchface：表盘需求。
  Return:          1：匹配；0：不匹配。
  调用约束:         不按名称或房间猜测，不会擅自选择某个房间。
**********************************************************************/
static int divoom_ha_entity_matches(const char *entity, divoom_cJSON *attrs, const struct divoom_ha_watchface *watchface)
{
    unsigned i;
    const char *device_class = divoom_ha_json_get_string(attrs, "device_class");
    for(i = 0; i < watchface->requirement_count; i++)
    {
        const struct divoom_ha_requirement *requirement = &watchface->requirements[i];
        size_t length = strlen(requirement->domain);
        if(strncmp(entity, requirement->domain, length) || entity[length] != '.')
            continue;
        if(!requirement->device_class[0] || (device_class && !strcmp(device_class, requirement->device_class)))
            return 1;
        /* Numeric test helpers may only expose a temperature unit. */
        if(!device_class && !strcmp(requirement->device_class, "temperature"))
        {
            const char *unit = divoom_ha_json_get_string(attrs, "unit_of_measurement");
            if(unit && (!strcmp(unit, "\xc2\xb0"
                                      "C") ||
                        !strcmp(unit, "\xc2\xb0"
                                      "F")))
                return 1;
        }
    }
    return 0;
}

/*********************************************************************
  Function:        divoom_ha_discover_entities
  Description:     遍历 get_states 结果，匹配、去重、排序并生成有界的订阅列表。
  Input:           states：HA 状态数组；config：写入匹配结果的会话配置；revision：表盘版本。
  Return:          0：有匹配；1：无匹配；-1：数据或版本无效；-2：超过数量上限。
  调用约束:         内部加锁；多个温度保留候选，只有唯一候选时用于单温度测试显示。
**********************************************************************/
static int divoom_ha_discover_entities(divoom_cJSON *states, struct divoom_ha_config *config, unsigned revision)
{
    divoom_cJSON *item, *attrs;
    unsigned i, j;
    int overflow = 0;
    unsigned temperature_candidates = 0;
    if(!states || (states->type & 255) != divoom_cJSON_Array)
        return -1;
    divoom_ha_lock();
    if(revision != pdivoom_ha_handle->watchface_revision)
    {
        divoom_ha_unlock();
        return -1;
    }
    config->entity_count = 0;
    config->temperature_entity[0] = 0;
    /* Keep a deterministic lexical order, independent of HA registry ordering. */
    for(item = states->child; item; item = item->next)
    {
        const char *entity = divoom_ha_json_get_string(item, "entity_id");
        attrs = divoom_cJSON_GetObjectItem(item, "attributes");
        if(!entity || strlen(entity) >= DIVOOM_HA_ENTITY_ID_MAX || !divoom_ha_validate_entity_id(entity) || !divoom_ha_entity_matches(entity, attrs, &pdivoom_ha_handle->watchface))
            continue;
        for(i = 0; i < config->entity_count; i++)
            if(!strcmp(config->entity_ids[i], entity))
                break;
        if(i < config->entity_count)
            continue;
        if(config->entity_count >= DIVOOM_HA_ENTITY_MAX)
        {
            overflow = 1;
            continue;
        }
        i = config->entity_count++;
        while(i && strcmp(config->entity_ids[i - 1], entity) > 0)
        {
            strcpy(config->entity_ids[i], config->entity_ids[i - 1]);
            i--;
        }
        strcpy(config->entity_ids[i], entity);
    }
    pdivoom_ha_handle->status.match_overflow = overflow;
    if(overflow)
    {
        config->entity_count = 0;
        pdivoom_ha_handle->status.matched_count = 0;
        memset(&pdivoom_ha_handle->matched_config, 0, sizeof(pdivoom_ha_handle->matched_config));
        divoom_ha_unlock();
        return -2;
    }
    /* Never silently choose one room's temperature when multiple candidates match. */
    for(item = states->child; item; item = item->next)
    {
        const char *entity = divoom_ha_json_get_string(item, "entity_id");
        const char *device_class;
        attrs = divoom_cJSON_GetObjectItem(item, "attributes");
        device_class = divoom_ha_json_get_string(attrs, "device_class");
        if(!entity)
            continue;
        if(!device_class || strcmp(device_class, "temperature"))
        {
            const char *unit = divoom_ha_json_get_string(attrs, "unit_of_measurement");
            if(device_class || !unit ||
               (strcmp(unit, "\xc2\xb0"
                             "C") &&
                strcmp(unit, "\xc2\xb0"
                             "F")))
                continue;
        }
        for(j = 0; j < config->entity_count; j++)
        {
            if(strcmp(config->entity_ids[j], entity))
                continue;
            temperature_candidates++;
            snprintf(config->temperature_entity, sizeof(config->temperature_entity), "%s", entity);
            break;
        }
    }
    if(temperature_candidates != 1)
        config->temperature_entity[0] = 0;
    pdivoom_ha_handle->status.temperature_candidates = temperature_candidates;
    pdivoom_ha_handle->status.matched_count = config->entity_count;
    pdivoom_ha_handle->matched_config = *config;
    memset(pdivoom_ha_handle->entities, 0, sizeof(pdivoom_ha_handle->entities));
    divoom_ha_unlock();
    return config->entity_count ? 0 : 1;
}

/*********************************************************************
  Function:        divoom_ha_send_json
  Description:     序列化并发送 JSON，在成功和失败路径都释放传入对象。
  Input:           w：连接；r：转交所有权的 JSON 对象。
  Return:          0：发送成功；-1：对象、内存或传输失败。
  调用约束:         后台任务调用；返回后不能再访问或释放 r。
**********************************************************************/
static int divoom_ha_send_json(struct divoom_ha_wire *w, divoom_cJSON *r)
{
    char *s;
    int rc = -1;
    if(!r)
        return -1;
    s = divoom_cJSON_Print(r);
    if(s)
    {
        rc = divoom_ha_wire_send_text(w, s);
        divoom_system_free(s);
    }
    divoom_cJSON_Delete(r);
    return rc;
}
/*********************************************************************
  Function:        divoom_ha_send_auth
  Description:     构造 auth 消息，发送令牌以完成 HA WebSocket 认证。
  Input:           w：连接；token：当前连接令牌。
  Return:          0：已发送；-1：内存或传输失败。
  调用约束:         后台任务调用；仍需等待 auth_ok，禁止打印令牌。
**********************************************************************/
static int divoom_ha_send_auth(struct divoom_ha_wire *w, const char *token)
{
    divoom_cJSON *r = divoom_cJSON_CreateObject();
    if(!r)
        return -1;
    divoom_cJSON_AddStringToObject(r, "type", "auth");
    divoom_cJSON_AddStringToObject(r, "access_token", token);
    return divoom_ha_send_json(w, r);
}
/*********************************************************************
  Function:        divoom_ha_subscribe_entities
  Description:     发送 subscribe_entities，一次订阅匹配列表中的多个实体。
  Input:           w：连接；config：非空匹配列表；id：请求及后续事件使用的订阅 ID。
  Return:          0：请求已发送；-1：内存或传输失败。
  调用约束:         后台任务调用；收到 result 成功后才视为订阅建立。
**********************************************************************/
static int divoom_ha_subscribe_entities(struct divoom_ha_wire *w, const struct divoom_ha_config *config, unsigned id)
{
    unsigned i;
    divoom_cJSON *r = divoom_cJSON_CreateObject(), *a, *entity;
    if(!r)
        return -1;
    a = divoom_cJSON_CreateArray();
    if(!a)
    {
        divoom_cJSON_Delete(r);
        return -1;
    }
    divoom_cJSON_AddNumberToObject(r, "id", id);
    divoom_cJSON_AddStringToObject(r, "type", "subscribe_entities");
    for(i = 0; i < config->entity_count; i++)
    {
        entity = divoom_cJSON_CreateString(config->entity_ids[i]);
        if(!entity)
        {
            divoom_cJSON_Delete(a);
            divoom_cJSON_Delete(r);
            return -1;
        }
        divoom_cJSON_AddItemToArray(a, entity);
    }
    divoom_cJSON_AddItemToObject(r, "entity_ids", a);
    return divoom_ha_send_json(w, r);
}
/*********************************************************************
  Function:        divoom_ha_send_scene
  Description:     发送 scene.turn_on，执行明确绑定的场景。
  Input:           w：连接；scene：scene.* 实体；id：关联结果的请求 ID。
  Return:          0：请求已发送；-1：内存或传输失败。
  调用约束:         后台任务调用；结果未知时不得自动补发，以免重复操作。
**********************************************************************/
static int divoom_ha_send_scene(struct divoom_ha_wire *w, const char *scene, unsigned id)
{
    divoom_cJSON *r = divoom_cJSON_CreateObject(), *target;
    if(!r)
        return -1;
    target = divoom_cJSON_CreateObject();
    if(!target)
    {
        divoom_cJSON_Delete(r);
        return -1;
    }
    divoom_cJSON_AddNumberToObject(r, "id", id);
    divoom_cJSON_AddStringToObject(r, "type", "call_service");
    divoom_cJSON_AddStringToObject(r, "domain", "scene");
    divoom_cJSON_AddStringToObject(r, "service", "turn_on");
    divoom_cJSON_AddStringToObject(target, "entity_id", scene);
    divoom_cJSON_AddItemToObject(r, "target", target);
    return divoom_ha_send_json(w, r);
}
/* 每个子表盘绑定一个实体。支持 domain=domain.object_id 及 entity_id=domain.object_id。 */
int divoom_ha_parse_clock_entity(const char *config, char entity_id[DIVOOM_HA_ENTITY_ID_MAX])
{
    char value[DIVOOM_HA_ENTITY_ID_MAX] = {0};
    const char *field, *end;

    if(!entity_id)
        return -1;
    entity_id[0] = 0;
    for(field = config; field && *field; field = *end ? end + 1 : end)
    {
        const char *equal, *id, *dot;
        size_t key_len, id_len, domain_len;

        end = field + strcspn(field, "&");
        equal = memchr(field, '=', end - field);
        if(!equal || equal == field)
            continue;
        key_len = (size_t)(equal - field);
        id = equal + 1;
        id_len = (size_t)(end - id);
        dot = memchr(id, '.', id_len);
        if(!dot || dot == id)
            continue;
        domain_len = (size_t)(dot - id);
        if(!(key_len == domain_len && !memcmp(field, id, key_len)) &&
            !(key_len == sizeof("entity_id") - 1 && !memcmp(field, "entity_id", key_len)) &&
            !(key_len == sizeof("temperature") - 1 && !memcmp(field, "temperature", key_len) &&
              domain_len == sizeof("sensor") - 1 && !memcmp(id, "sensor", domain_len)))
            continue;
        if(value[0] || id_len >= sizeof(value))
            return -1;
        memcpy(value, id, id_len);
    }
    if(!divoom_ha_validate_entity_id(value))
        return -1;
    strcpy(entity_id, value);
    return 0;
}

/* 3518 的显式槽位独立于登录凭据及原有自动发现策略。 */
int divoom_ha_set_clock_bindings(const struct divoom_ha_clock_bindings *bindings)
{
    struct divoom_ha_clock_bindings valid = {0};
    unsigned i;

    if(!pdivoom_ha_handle)
        return -1;
    for(i = 0; bindings && i < DIVOOM_HA_CLOCK_SLOT_COUNT; i++)
    {
        const char *id = bindings->entity_ids[i];
        if(memchr(id, 0, sizeof(valid.entity_ids[i])) && divoom_ha_validate_entity_id(id))
        {
            strcpy(valid.entity_ids[i], id);
            valid.aux_slots |= bindings->aux_slots & (1u << i);
        }
    }
    divoom_ha_lock();
    if(pdivoom_ha_handle->clock_active != (bindings != NULL) ||
        memcmp(&pdivoom_ha_handle->clock_bindings, &valid, sizeof(valid)))
    {
        if(!bindings || !pdivoom_ha_handle->clock_active)
        {
            memset(&pdivoom_ha_handle->matched_config, 0, sizeof(pdivoom_ha_handle->matched_config));
            memset(pdivoom_ha_handle->entities, 0, sizeof(pdivoom_ha_handle->entities));
            pdivoom_ha_handle->status.matched_count = 0;
        }
        pdivoom_ha_handle->clock_active = bindings != NULL;
        if(!bindings)
            pdivoom_ha_handle->clock_control = 0;
        for(i = 0; i < DIVOOM_HA_CLOCK_SLOT_COUNT; i++)
        {
            if(!(valid.aux_slots & (1u << i)) || strcmp(valid.entity_ids[i], pdivoom_ha_handle->clock_bindings.entity_ids[i]))
                memset(pdivoom_ha_handle->clock_aux_ids[i], 0, sizeof(pdivoom_ha_handle->clock_aux_ids[i]));
        }
        pdivoom_ha_handle->clock_bindings = valid;
        pdivoom_ha_handle->clock_action_slot = 0;
        pdivoom_ha_handle->watchface_revision++;
    }
    divoom_ha_unlock();
    return 0;
}

/* 仅供按键输入分流，页面退出时丢弃还未发送的控制请求。 */
void divoom_ha_set_clock_control(int enabled)
{
    if(!pdivoom_ha_handle)
        return;
    divoom_ha_lock();
    pdivoom_ha_handle->clock_control = enabled && pdivoom_ha_handle->clock_active;
    if(!pdivoom_ha_handle->clock_control)
        pdivoom_ha_handle->clock_action_slot = 0;
    divoom_ha_unlock();
}

int divoom_ha_clock_control_enabled(void)
{
    int enabled;
    if(!pdivoom_ha_handle)
        return 0;
    divoom_ha_lock();
    enabled = pdivoom_ha_handle->clock_control;
    divoom_ha_unlock();
    return enabled;
}

/* 主实体与辅助实体共用同一次槽位快照，避免热改绑时串到其它设备。 */
static void divoom_ha_copy_clock_entity(const char *id, struct divoom_ha_entity_state *state)
{
    unsigned i;
    if(!id[0])
        return;
    for(i = 0; i < pdivoom_ha_handle->matched_config.entity_count; i++)
    {
        if(strcmp(id, pdivoom_ha_handle->matched_config.entity_ids[i]))
            continue;
        *state = pdivoom_ha_handle->entities[i];
        if(!state->unit[0] && !strncmp(id, "climate.", 8))
            strcpy(state->unit, pdivoom_ha_handle->temperature_unit);
        break;
    }
    strcpy(state->entity_id, id);
}

int divoom_ha_get_clock_entity(unsigned slot, struct divoom_ha_entity_state *state,
    struct divoom_ha_entity_state *aux)
{
    if(!state)
        return -1;
    memset(state, 0, sizeof(*state));
    if(aux)
        memset(aux, 0, sizeof(*aux));
    if(!pdivoom_ha_handle || !slot || slot > DIVOOM_HA_CLOCK_SLOT_COUNT)
        return -1;
    divoom_ha_lock();
    if(pdivoom_ha_handle->clock_active)
    {
        divoom_ha_copy_clock_entity(pdivoom_ha_handle->clock_bindings.entity_ids[slot - 1], state);
        if(aux)
            divoom_ha_copy_clock_entity(pdivoom_ha_handle->clock_aux_ids[slot - 1], aux);
    }
    divoom_ha_unlock();
    return state->entity_id[0] ? 0 : -1;
}

/* 页面只提交槽位，实际发送前用最新已确认状态检查设备能力。 */
int divoom_ha_request_clock_action(unsigned slot, enum divoom_ha_clock_action action)
{
    int result = -1;
    if(!pdivoom_ha_handle || !slot || slot > DIVOOM_HA_CLOCK_SLOT_COUNT ||
        action < DIVOOM_HA_CLOCK_DECREASE || action > DIVOOM_HA_CLOCK_INCREASE)
        return -1;
    divoom_ha_lock();
    if(pdivoom_ha_handle->clock_active && pdivoom_ha_handle->status.online &&
        pdivoom_ha_handle->clock_bindings.entity_ids[slot - 1][0] && !pdivoom_ha_handle->clock_action_slot)
    {
        pdivoom_ha_handle->clock_action_slot = slot;
        pdivoom_ha_handle->clock_action = action;
        result = 0;
    }
    divoom_ha_unlock();
    return result;
}

enum divoom_ha_entity_operation
{
    DIVOOM_HA_ENTITY_IDLE,
    DIVOOM_HA_ENTITY_DISCOVER,
    DIVOOM_HA_ENTITY_SUBSCRIBE,
    DIVOOM_HA_ENTITY_UNSUBSCRIBE
};

enum divoom_ha_aux_query
{
    DIVOOM_HA_AUX_PRIMARY,
    DIVOOM_HA_AUX_RELATED,
    DIVOOM_HA_AUX_SENSORS
};

struct divoom_ha_aux_lookup
{
    struct divoom_ha_clock_bindings bindings;
    char device_ids[DIVOOM_HA_CLOCK_SLOT_COUNT][64];
    char sensor_ids[DIVOOM_HA_CLOCK_SLOT_COUNT][DIVOOM_HA_ENTITY_ID_MAX];
    unsigned id, sent_at, revision, slot;
    enum divoom_ha_aux_query query;
};

struct divoom_ha_session
{
    struct divoom_ha_config config;
    struct divoom_ha_config active_config;
    unsigned connection_revision;
    unsigned entity_revision;
    unsigned processed_revision;
    unsigned next_id;
    unsigned config_id;
    unsigned entity_id;
    unsigned pending_id;
    unsigned pending_at;
    unsigned retry_at;
    unsigned scene_id;
    unsigned action_at;
    unsigned ping_id;
    unsigned ping_at;
    unsigned last_ping;
    int authenticated;
    int retry_entities;
    int aux_disabled;
    enum divoom_ha_entity_operation operation;
    struct divoom_ha_aux_lookup aux;
};

/*********************************************************************
  Function:        divoom_ha_set_entity_state
  Description:     单独更新实体发现或订阅状态，不影响通知订阅。
  Input:           state：状态文本。
  Return:          无。
  调用约束:         内部加锁，不改变连接在线标志。
**********************************************************************/
static void divoom_ha_set_entity_state(const char *state)
{
    divoom_ha_lock();
    snprintf(pdivoom_ha_handle->status.entity_state, sizeof(pdivoom_ha_handle->status.entity_state), "%s", state);
    divoom_ha_unlock();
}

/*********************************************************************
  Function:        divoom_ha_send_command
  Description:     发送实体查询、取消订阅或心跳控制命令。
  Input:           wire：连接；type：命令类型；id：请求 ID；subscription：取消时的旧订阅 ID。
  Return:          0：已发送；-1：内存或传输失败。
  调用约束:         仅由 HA 后台任务调用。
**********************************************************************/
static int divoom_ha_send_command(struct divoom_ha_wire *wire, const char *type, unsigned id, unsigned subscription)
{
    divoom_cJSON *request = divoom_cJSON_CreateObject();
    if(!request)
        return -1;
    divoom_cJSON_AddNumberToObject(request, "id", id);
    divoom_cJSON_AddStringToObject(request, "type", type);
    if(!strcmp(type, "unsubscribe_events"))
        divoom_cJSON_AddNumberToObject(request, "subscription", subscription);
    return divoom_ha_send_json(wire, request);
}

static const char *divoom_ha_aux_device_class(const char *id)
{
    if(!strncmp(id, "switch.", 7))
        return "power";
    if(!strncmp(id, "vacuum.", 7))
        return "battery";
    if(!strncmp(id, "sensor.", 7) || !strncmp(id, "binary_sensor.", 14) ||
        !strncmp(id, "select.", 7) || !strncmp(id, "washer.", 7) ||
        !strncmp(id, "dishwasher.", 11) || !strncmp(id, "dryer.", 6))
        return "timestamp";
    return NULL;
}

/* 仅发送当前槽所需的查询，设备值仍由已有 subscribe_entities 回传。 */
static int divoom_ha_aux_send(struct divoom_ha_wire *wire, struct divoom_ha_session *session,
    enum divoom_ha_aux_query query, divoom_cJSON *entity_ids, unsigned now)
{
    struct divoom_ha_aux_lookup *aux = &session->aux;
    divoom_cJSON *request = divoom_cJSON_CreateObject();
    if(!request)
    {
        divoom_cJSON_Delete(entity_ids);
        return 0;
    }
    aux->query = query;
    aux->id = session->next_id++;
    aux->sent_at = now;
    divoom_cJSON_AddNumberToObject(request, "id", aux->id);
    if(entity_ids)
    {
        divoom_cJSON_AddStringToObject(request, "type", "config/entity_registry/get_entries");
        divoom_cJSON_AddItemToObjectCS(request, "entity_ids", entity_ids);
    }
    else
    {
        divoom_cJSON_AddStringToObject(request, "type", "search/related");
        divoom_cJSON_AddStringToObject(request, "item_type", "device");
        divoom_cJSON_AddStringToObject(request, "item_id", aux->device_ids[aux->slot]);
    }
    return divoom_ha_send_json(wire, request);
}

static int divoom_ha_aux_finish(struct divoom_ha_aux_lookup *aux)
{
    divoom_ha_lock();
    if(aux->revision == pdivoom_ha_handle->watchface_revision &&
        memcmp(aux->sensor_ids, pdivoom_ha_handle->clock_aux_ids, sizeof(aux->sensor_ids)))
    {
        memcpy(pdivoom_ha_handle->clock_aux_ids, aux->sensor_ids, sizeof(aux->sensor_ids));
        /* 订阅去重并更新，记录新版本以免再次触发同一轮查找。 */
        aux->revision = ++pdivoom_ha_handle->watchface_revision;
    }
    divoom_ha_unlock();
    return 0;
}

static int divoom_ha_aux_next_device(struct divoom_ha_wire *wire, struct divoom_ha_session *session, unsigned now)
{
    struct divoom_ha_aux_lookup *aux = &session->aux;
    while(aux->slot < DIVOOM_HA_CLOCK_SLOT_COUNT && !aux->device_ids[aux->slot][0])
        aux->slot++;
    if(aux->slot == DIVOOM_HA_CLOCK_SLOT_COUNT)
        return divoom_ha_aux_finish(aux);
    return divoom_ha_aux_send(wire, session, DIVOOM_HA_AUX_RELATED, NULL, now);
}

/* 相关实体查询也可能返回子设备，最终按注册表 device_id 复核归属。 */
static void divoom_ha_aux_select_sensor(struct divoom_ha_aux_lookup *aux, divoom_cJSON *result)
{
    const char *primary = aux->bindings.entity_ids[aux->slot];
    const char *wanted = divoom_ha_aux_device_class(primary);
    const char *selected = NULL;
    divoom_cJSON *entry;

    if(!wanted)
        return;
    for(entry = result ? result->child : NULL; entry; entry = entry->next)
    {
        const char *id = entry->string;
        const char *device = divoom_ha_json_get_string(entry, "device_id");
        const char *device_class = divoom_ha_json_get_string(entry, "device_class");
        if(!id || strncmp(id, "sensor.", 7) || !strcmp(id, primary) ||
            strlen(id) >= DIVOOM_HA_ENTITY_ID_MAX || !divoom_ha_validate_entity_id(id) ||
            !device || strcmp(device, aux->device_ids[aux->slot]) ||
            divoom_ha_json_get_string(entry, "disabled_by"))
            continue;
        if(!device_class)
            device_class = divoom_ha_json_get_string(entry, "original_device_class");
        if(!device_class || strcmp(device_class, wanted))
            continue;
        if(!strcmp(wanted, "timestamp"))
        {
            const char *key = divoom_ha_json_get_string(entry, "translation_key");
            if(!key || (strcmp(key, "program_finish_time") && strcmp(key, "completion_time")))
                continue;
        }
        if(selected)
            return;
        selected = id;
    }
    if(selected)
        strcpy(aux->sensor_ids[aux->slot], selected);
}

static int divoom_ha_aux_result(struct divoom_ha_wire *wire, struct divoom_ha_session *session,
    divoom_cJSON *result, unsigned now)
{
    struct divoom_ha_aux_lookup *aux = &session->aux;
    unsigned revision;
    aux->id = 0;
    divoom_ha_lock();
    revision = pdivoom_ha_handle->watchface_revision;
    divoom_ha_unlock();
    if(aux->revision != revision)
        return 0;
    if(aux->query == DIVOOM_HA_AUX_PRIMARY)
    {
        unsigned slot;
        for(slot = 0; slot < DIVOOM_HA_CLOCK_SLOT_COUNT; slot++)
        {
            const char *id = aux->bindings.entity_ids[slot];
            const char *device = divoom_ha_json_get_string(divoom_cJSON_GetObjectItem(result, id), "device_id");
            if((aux->bindings.aux_slots & (1u << slot)) && divoom_ha_aux_device_class(id) &&
                device && strlen(device) < sizeof(aux->device_ids[slot]))
                strcpy(aux->device_ids[slot], device);
        }
        return divoom_ha_aux_next_device(wire, session, now);
    }
    if(aux->query == DIVOOM_HA_AUX_RELATED)
    {
        divoom_cJSON *entities = divoom_cJSON_GetObjectItem(result, "entity");
        divoom_cJSON *item, *ids = divoom_cJSON_CreateArray();
        unsigned count = 0;
        if(ids)
        {
            for(item = entities ? entities->child : NULL; item; item = item->next)
            {
                const char *id = (item->type & 255) == divoom_cJSON_String ? item->valuestring : NULL;
                divoom_cJSON *name;
                if(!id || strncmp(id, "sensor.", 7) || !strcmp(id, aux->bindings.entity_ids[aux->slot]) ||
                    strlen(id) >= DIVOOM_HA_ENTITY_ID_MAX || !divoom_ha_validate_entity_id(id))
                    continue;
                if(++count > DIVOOM_HA_ENTITY_MAX || !(name = divoom_cJSON_CreateString(id)))
                    break;
                divoom_cJSON_AddItemToArray(ids, name);
            }
            /* 候选过多或分配失败时跳过该槽，不能从不完整的列表选传感器。 */
            if(!item && count)
                return divoom_ha_aux_send(wire, session, DIVOOM_HA_AUX_SENSORS, ids, now);
            divoom_cJSON_Delete(ids);
        }
    }
    else
        divoom_ha_aux_select_sensor(aux, result);
    aux->slot++;
    return divoom_ha_aux_next_device(wire, session, now);
}

static int divoom_ha_aux_tick(struct divoom_ha_wire *wire, struct divoom_ha_session *session, unsigned now)
{
    struct divoom_ha_aux_lookup *aux = &session->aux;
    divoom_cJSON *ids;
    unsigned revision, slot;

    if(session->aux_disabled || !session->entity_id || session->pending_id)
        return 0;
    divoom_ha_lock();
    revision = pdivoom_ha_handle->watchface_revision;
    if(!pdivoom_ha_handle->clock_active || revision != session->entity_revision)
    {
        divoom_ha_unlock();
        return 0;
    }
    if(revision == aux->revision)
    {
        divoom_ha_unlock();
        if(aux->id && now - aux->sent_at > 3000)
            return divoom_ha_aux_result(wire, session, NULL, now);
        return 0;
    }
    memset(aux, 0, sizeof(*aux));
    aux->revision = revision;
    aux->bindings = pdivoom_ha_handle->clock_bindings;
    divoom_ha_unlock();

    if(!aux->bindings.aux_slots)
        return 0;
    ids = divoom_cJSON_CreateArray();
    if(!ids)
        return 0;
    for(slot = 0; slot < DIVOOM_HA_CLOCK_SLOT_COUNT; slot++)
    {
        const char *id = aux->bindings.entity_ids[slot];
        divoom_cJSON *name;
        if(!(aux->bindings.aux_slots & (1u << slot)) || !divoom_ha_aux_device_class(id))
            continue;
        name = divoom_cJSON_CreateString(id);
        if(!name)
        {
            divoom_cJSON_Delete(ids);
            return 0;
        }
        divoom_cJSON_AddItemToArray(ids, name);
    }
    if(ids->child)
        return divoom_ha_aux_send(wire, session, DIVOOM_HA_AUX_PRIMARY, ids, now);
    divoom_cJSON_Delete(ids);
    return 0;
}

/*********************************************************************
  Function:        divoom_ha_session_authenticate
  Description:     收到 auth_ok 后标记连接在线，开始心跳计时并允许实体发现。
  Input:           session：当前会话；now：毫秒时钟。
  Return:          无。
  调用约束:         仅由 HA 后台任务在认证成功后调用。
**********************************************************************/
static void divoom_ha_session_authenticate(struct divoom_ha_session *session, unsigned now)
{
    session->authenticated = 1;
    session->last_ping = now;
    divoom_ha_set_connection_state("Online", 1);
}

static void divoom_ha_add_clock_subscription(struct divoom_ha_config *config, const char *id)
{
    unsigned i;
    if(!id[0])
        return;
    for(i = 0; i < config->entity_count; i++)
    {
        if(!strcmp(config->entity_ids[i], id))
            return;
    }
    strcpy(config->entity_ids[config->entity_count++], id);
}

/*********************************************************************
  Function:        divoom_ha_session_update_entities
  Description:     根据表盘版本取消旧实体订阅、发现新实体并重新订阅。
  Input:           wire：连接；session：会话；now：毫秒时钟。
  Return:          0：无需处理、等待或已发请求；-1：命令超时或发送失败，应重连清理。
  调用约束:         后台任务调用；无需求时不订阅实体，无匹配按间隔重试。
**********************************************************************/
static int divoom_ha_session_update_entities(struct divoom_ha_wire *wire, struct divoom_ha_session *session, unsigned now)
{
    unsigned revision;
    divoom_ha_lock();
    revision = pdivoom_ha_handle->watchface_revision;
    divoom_ha_unlock();
    if(session->pending_id)
        return now - session->pending_at > 10000 ? -1 : 0;
    if(revision == session->processed_revision && (!session->retry_entities || now - session->retry_at < 30000))
        return 0;
    session->pending_id = session->next_id++;
    session->pending_at = now;
    if(session->entity_id)
    {
        session->operation = DIVOOM_HA_ENTITY_UNSUBSCRIBE;
        divoom_ha_set_entity_state("Updating subscription");
        return divoom_ha_send_command(wire, "unsubscribe_events", session->pending_id, session->entity_id);
    }
    divoom_ha_lock();
    session->config = pdivoom_ha_handle->config;
    session->entity_revision = pdivoom_ha_handle->watchface_revision;
    if(pdivoom_ha_handle->clock_active)
    {
        /* 主、辅实体按 ID 保留缓存，改绑一槽时不打断其它槽的动画。 */
        struct divoom_ha_entity_state retained[DIVOOM_HA_CLOCK_SLOT_COUNT * 2] = {0};
        unsigned slot, i, j;
        session->config.entity_count = 0;
        session->config.temperature_entity[0] = 0;
        memset(session->config.entity_ids, 0, sizeof(session->config.entity_ids));
        for(slot = 0; slot < DIVOOM_HA_CLOCK_SLOT_COUNT; slot++)
        {
            divoom_ha_add_clock_subscription(&session->config, pdivoom_ha_handle->clock_bindings.entity_ids[slot]);
            divoom_ha_add_clock_subscription(&session->config, pdivoom_ha_handle->clock_aux_ids[slot]);
        }
        for(i = 0; i < session->config.entity_count; i++)
        {
            for(j = 0; j < pdivoom_ha_handle->matched_config.entity_count; j++)
            {
                if(!strcmp(session->config.entity_ids[i], pdivoom_ha_handle->matched_config.entity_ids[j]))
                {
                    retained[i] = pdivoom_ha_handle->entities[j];
                    break;
                }
            }
        }
        memset(pdivoom_ha_handle->entities, 0, sizeof(pdivoom_ha_handle->entities));
        memcpy(pdivoom_ha_handle->entities, retained, sizeof(retained));
        pdivoom_ha_handle->matched_config = session->config;
        pdivoom_ha_handle->status.matched_count = session->config.entity_count;
    }
    else if(session->config.entity_count)
    {
        pdivoom_ha_handle->matched_config = session->config;
        memset(pdivoom_ha_handle->entities, 0, sizeof(pdivoom_ha_handle->entities));
        pdivoom_ha_handle->status.matched_count = session->config.entity_count;
    }
    if(!session->config.entity_count && (pdivoom_ha_handle->clock_active || !pdivoom_ha_handle->watchface.requirement_count))
    {
        session->pending_id = 0;
        session->processed_revision = session->entity_revision;
        session->retry_entities = 0;
        strcpy(pdivoom_ha_handle->status.entity_state, "No watchface data required");
        divoom_ha_unlock();
        return 0;
    }
    divoom_ha_unlock();
    if(session->config.entity_count)
    {
        session->operation = DIVOOM_HA_ENTITY_SUBSCRIBE;
        divoom_ha_set_entity_state("Subscribing");
        return divoom_ha_subscribe_entities(wire, &session->config, session->pending_id);
    }
    session->operation = DIVOOM_HA_ENTITY_DISCOVER;
    divoom_ha_set_entity_state("Discovering entities");
    return divoom_ha_send_command(wire, "get_states", session->pending_id, 0);
}

/*********************************************************************
  Function:        divoom_ha_session_result
  Description:     根据请求 ID 分发 result，推进场景和实体订阅流程。
  Input:           wire：连接；session：会话；root：result；id：响应 ID；now：毫秒时钟。
  Return:          0：处理完成或无关响应；-1：发送失败或旧订阅无法清理，应重连。
  调用约束:         后台任务调用；无匹配及匹配超限均不直接断开连接。
**********************************************************************/
static int divoom_ha_session_result(struct divoom_ha_wire *wire, struct divoom_ha_session *session, divoom_cJSON *root, unsigned id, unsigned now)
{
    divoom_cJSON *success = divoom_cJSON_GetObjectItem(root, "success");
    int ok = success && (success->type & 255) == divoom_cJSON_True;
    if(id && id == session->config_id)
    {
        session->config_id = 0;
        if(ok)
        {
            divoom_cJSON *config = divoom_cJSON_GetObjectItem(root, "result");
            divoom_cJSON *unit_system = divoom_cJSON_GetObjectItem(config, "unit_system");
            const char *unit = divoom_ha_json_get_string(unit_system, "temperature");

            if(unit && (!strcmp(unit, "\xc2\xb0" "C") || !strcmp(unit, "\xc2\xb0" "F")))
            {
                divoom_ha_lock();
                if(session->connection_revision == pdivoom_ha_handle->revision)
                    strcpy(pdivoom_ha_handle->temperature_unit, unit);
                divoom_ha_unlock();
            }
        }
    }
    else if(id && id == session->scene_id)
    {
        divoom_cJSON *error = divoom_cJSON_GetObjectItem(root, "error");
        const char *code = divoom_ha_json_get_string(error, "code");
        divoom_print("ha action: reply id=%u success=%d code=%.48s\n", id, ok, code ? code : "-");
        divoom_ha_set_action_state(ok ? "Request accepted" : "Request rejected", 0);
        session->scene_id = 0;
    }
    else if(id && id == session->aux.id)
        return divoom_ha_aux_result(wire, session, ok ? divoom_cJSON_GetObjectItem(root, "result") : NULL, now);
    else if(id && id == session->pending_id)
    {
        unsigned revision;
        enum divoom_ha_entity_operation operation = session->operation;
        session->pending_id = 0;
        divoom_ha_lock();
        revision = pdivoom_ha_handle->watchface_revision;
        divoom_ha_unlock();
        if(operation == DIVOOM_HA_ENTITY_UNSUBSCRIBE)
        {
            /* Rejection leaves the old subscription live: reconnect to clean it. */
            if(!ok)
                return -1;
            session->entity_id = 0;
            return 0;
        }
        if(operation == DIVOOM_HA_ENTITY_SUBSCRIBE)
        {
            if(ok)
            {
                session->entity_id = id;
                session->active_config = session->config;
            }
            session->processed_revision = session->entity_revision;
            session->retry_entities = !ok;
            session->retry_at = now;
            divoom_ha_set_entity_state(ok ? "Subscribed" : "Entity subscription rejected");
            return 0;
        }
        if(operation == DIVOOM_HA_ENTITY_DISCOVER)
        {
            int matched;
            if(revision != session->entity_revision)
                return 0;
            matched = ok ? divoom_ha_discover_entities(divoom_cJSON_GetObjectItem(root, "result"), &session->config, session->entity_revision) : -1;
            if(matched)
            {
                session->processed_revision = session->entity_revision;
                session->retry_entities = matched != -2;
                session->retry_at = now;
                divoom_ha_set_entity_state(matched == 1 ? "No matching entities" : matched == -2 ? "Too many matches (max 16)" : "Entity discovery failed");
                return 0;
            }
            session->pending_id = session->next_id++;
            session->pending_at = now;
            session->operation = DIVOOM_HA_ENTITY_SUBSCRIBE;
            divoom_ha_set_entity_state("Subscribing");
            return divoom_ha_subscribe_entities(wire, &session->config, session->pending_id);
        }
    }
    return 0;
}

/*********************************************************************
  Function:        divoom_ha_session_tick
  Description:     处理已认证会话的场景请求、心跳及表盘订阅更新。
  Input:           wire：连接；session：会话状态；now：毫秒时钟。
  Return:          0：继续接收；-1：心跳、命令或传输失败，应退出连接。
  调用约束:         后台任务周期调用，不操作界面。
**********************************************************************/
/* 构造针对已订阅实体的 call_service。没有有效状态或能力时返回空。 */
static divoom_cJSON *divoom_ha_clock_service(const char *id, const struct divoom_ha_entity_state *state,
    enum divoom_ha_clock_action action, unsigned request_id)
{
    const char *domain, *service, *argument = NULL;
    const char *trigger_domain = NULL;
    double value = 0;
    divoom_cJSON *request, *target, *data;
    int off = !strcmp(state->state, "off");

    if(!strncmp(id, "scene.", 6))
        trigger_domain = "scene";
    else if(!strncmp(id, "button.", 7))
        trigger_domain = "button";
    else if(!strncmp(id, "input_button.", 13))
        trigger_domain = "input_button";

    /* 场景和按钮首次触发前可能是 unknown，收到该状态后仍可执行。 */
    if(!state->received || (!state->available && !(trigger_domain && !strcmp(state->state, "unknown"))))
    {
        divoom_print("ha action: entity state not ready received=%d available=%d\n",
            (int)state->received, (int)state->available);
        return NULL;
    }
    service = off ? "turn_on" : "turn_off";
    if(trigger_domain || !strncmp(id, "script.", 7))
    {
        if(action != DIVOOM_HA_CLOCK_TOGGLE)
            return NULL;
        domain = trigger_domain ? trigger_domain : "script";
        service = !strcmp(domain, "scene") || !strcmp(domain, "script") ? "turn_on" : "press";
    }
    else if(!strncmp(id, "climate.", 8))
    {
        domain = "climate";
        if(action == DIVOOM_HA_CLOCK_TOGGLE)
        {
            /* HA ClimateEntityFeature: TURN_OFF=128, TURN_ON=256. */
            if(!(state->supported_features & (off ? 256u : 128u)))
                return NULL;
        }
        else
        {
            unsigned required = DIVOOM_HA_TARGET_TEMP | DIVOOM_HA_MIN_TEMP | DIVOOM_HA_MAX_TEMP;
            double step = state->numeric_attributes & DIVOOM_HA_TEMP_STEP ? state->target_temp_step : 1;
            const char *unit = state->unit[0] ? state->unit : pdivoom_ha_handle->temperature_unit;
            if(off || !(state->supported_features & 1u) || (state->numeric_attributes & required) != required ||
                state->min_temp > state->max_temp || step <= 0)
            {
                divoom_print("ha action: temperature unavailable off=%d features=0x%x attrs=0x%x required=0x%x min=%.1f max=%.1f step=%.2f\n",
                    off, (unsigned)state->supported_features, (unsigned)state->numeric_attributes, required,
                    state->min_temp, state->max_temp, step);
                return NULL;
            }
            /* HA can report Fahrenheit in whole degrees, hiding a half-degree adjustment. */
            if(step < 1.0 && !strcmp(unit, "\xc2\xb0" "F"))
                step = 1.0;
            value = fmax(state->min_temp, fmin(state->max_temp, state->target_temperature + action * step));
            if(value == state->target_temperature)
            {
                divoom_print("ha action: target at limit current=%.1f direction=%d\n", value, (int)action);
                return NULL;
            }
            service = "set_temperature";
            argument = "temperature";
        }
    }
    else if(!strncmp(id, "light.", 6))
    {
        domain = "light";
        if(!off && strcmp(state->state, "on"))
            return NULL;
        if(action != DIVOOM_HA_CLOCK_TOGGLE)
        {
            if(off || !state->dimmable || !state->brightness)
                return NULL;
            value = fmax(1, fmin(255, state->brightness + action * 26));
            if(value == state->brightness)
                return NULL;
            service = "turn_on";
            argument = "brightness";
        }
    }
    else if(!strncmp(id, "fan.", 4))
    {
        domain = "fan";
        if(!off && strcmp(state->state, "on"))
            return NULL;
        if(action == DIVOOM_HA_CLOCK_TOGGLE)
        {
            /* HA FanEntityFeature: TURN_OFF=16, TURN_ON=32. */
            if(!(state->supported_features & (off ? 32u : 16u)))
                return NULL;
        }
        else
        {
            /* HA 按风扇自己的档位增减速度，SET_SPEED=1。 */
            if(!(state->supported_features & 1u) || (off && action == DIVOOM_HA_CLOCK_DECREASE))
                return NULL;
            service = action == DIVOOM_HA_CLOCK_INCREASE ? "increase_speed" : "decrease_speed";
        }
    }
    else if(!strncmp(id, "cover.", 6))
    {
        int closed = !strcmp(state->state, "closed");
        int opened = !strcmp(state->state, "open");
        int opening = !strcmp(state->state, "opening");
        int closing = !strcmp(state->state, "closing");
        domain = "cover";
        /* HA CoverEntityFeature: OPEN=1, CLOSE=2, STOP=8。 */
        if(action == DIVOOM_HA_CLOCK_TOGGLE && (opening || closing) && (state->supported_features & 8u))
            service = "stop_cover";
        else if(((closed && action != DIVOOM_HA_CLOCK_DECREASE) || (closing && action == DIVOOM_HA_CLOCK_INCREASE)) &&
            (state->supported_features & 1u))
            service = "open_cover";
        else if(((opened && action != DIVOOM_HA_CLOCK_INCREASE) || (opening && action == DIVOOM_HA_CLOCK_DECREASE)) &&
            (state->supported_features & 2u))
            service = "close_cover";
        else
            return NULL;
    }
    else if(!strncmp(id, "vacuum.", 7))
    {
        int cleaning = !strcmp(state->state, "cleaning");
        int paused = !strcmp(state->state, "paused");
        int idle = !strcmp(state->state, "idle");
        int docked = !strcmp(state->state, "docked");
        domain = "vacuum";
        /* HA VacuumEntityFeature: PAUSE=4, STOP=8, RETURN_HOME=16, START=8192。 */
        if(action == DIVOOM_HA_CLOCK_DECREASE && (cleaning || paused || idle) && (state->supported_features & 16u))
            service = "return_to_base";
        else if(action != DIVOOM_HA_CLOCK_DECREASE && (paused || idle || docked) && (state->supported_features & 8192u))
            service = "start";
        else if(action == DIVOOM_HA_CLOCK_TOGGLE && cleaning && (state->supported_features & 4u))
            service = "pause";
        else if(action == DIVOOM_HA_CLOCK_TOGGLE && cleaning && (state->supported_features & 8u))
            service = "stop";
        else
            return NULL;
    }
    else if(!strncmp(id, "lock.", 5) && action == DIVOOM_HA_CLOCK_TOGGLE)
    {
        domain = "lock";
        if(!strcmp(state->state, "locked"))
            service = "unlock";
        else if(!strcmp(state->state, "unlocked"))
            service = "lock";
        else
            return NULL;
    }
    else if((!strncmp(id, "switch.", 7) || !strncmp(id, "input_boolean.", 14)) && action == DIVOOM_HA_CLOCK_TOGGLE &&
        (off || !strcmp(state->state, "on")))
        domain = !strncmp(id, "switch.", 7) ? "switch" : "input_boolean";
    else
        return NULL;

    request = divoom_cJSON_CreateObject();
    target = divoom_cJSON_CreateObject();
    data = argument ? divoom_cJSON_CreateObject() : NULL;
    if(!request || !target || (argument && !data))
    {
        divoom_cJSON_Delete(request);
        divoom_cJSON_Delete(target);
        divoom_cJSON_Delete(data);
        return NULL;
    }
    divoom_cJSON_AddNumberToObject(request, "id", request_id);
    divoom_cJSON_AddStringToObject(request, "type", "call_service");
    divoom_cJSON_AddStringToObject(request, "domain", domain);
    divoom_cJSON_AddStringToObject(request, "service", service);
    divoom_cJSON_AddStringToObject(target, "entity_id", id);
    divoom_cJSON_AddItemToObject(request, "target", target);
    if(data)
    {
        divoom_cJSON_AddNumberToObject(data, argument, value);
        divoom_cJSON_AddItemToObject(request, "service_data", data);
    }
    if(argument && !strcmp(argument, "temperature"))
        divoom_print("ha action: set_temperature id=%u current=%.1f target=%.2f\n",
            request_id, state->target_temperature, value);
    return request;
}

static int divoom_ha_session_tick(struct divoom_ha_wire *wire, struct divoom_ha_session *session, unsigned now)
{
    int requested;
    divoom_cJSON *clock_request = NULL;
    if(session->scene_id && now - session->action_at > 5000)
    {
        divoom_ha_set_action_state("Result unknown; not retried", 0);
        session->scene_id = 0;
    }
    if(session->ping_id && now - session->ping_at > 10000)
        return -1;
    if(!session->ping_id && now - session->last_ping > 15000)
    {
        session->ping_id = session->next_id++;
        session->ping_at = session->last_ping = now;
        if(divoom_ha_send_command(wire, "ping", session->ping_id, 0))
            return -1;
    }
    divoom_ha_lock();
    requested = pdivoom_ha_handle->scene_requested && !session->scene_id;
    if(requested)
        pdivoom_ha_handle->scene_requested = 0;
    if(!requested && !session->scene_id && pdivoom_ha_handle->clock_action_slot)
    {
        unsigned slot = pdivoom_ha_handle->clock_action_slot;
        const char *id = pdivoom_ha_handle->clock_bindings.entity_ids[slot - 1];
        unsigned i;
        pdivoom_ha_handle->clock_action_slot = 0;
        if(session->entity_id && session->entity_revision == pdivoom_ha_handle->watchface_revision &&
            session->connection_revision == pdivoom_ha_handle->revision)
        {
            for(i = 0; i < pdivoom_ha_handle->matched_config.entity_count; i++)
            {
                if(strcmp(id, pdivoom_ha_handle->matched_config.entity_ids[i]))
                    continue;
                clock_request = divoom_ha_clock_service(id, &pdivoom_ha_handle->entities[i],
                    pdivoom_ha_handle->clock_action, session->next_id);
                break;
            }
        }
        if(!clock_request)
            divoom_print("ha action: no request slot=%u subscription=%u same_binding=%d same_connection=%d\n",
                slot, session->entity_id, session->entity_revision == pdivoom_ha_handle->watchface_revision,
                session->connection_revision == pdivoom_ha_handle->revision);
    }
    divoom_ha_unlock();
    if(clock_request)
    {
        session->scene_id = session->next_id++;
        session->action_at = now;
        divoom_ha_set_action_state("Sending", 1);
        if(divoom_ha_send_json(wire, clock_request))
        {
            divoom_print("ha action: send failed id=%u\n", session->scene_id);
            return -1;
        }
    }
    if(requested)
    {
        session->scene_id = session->next_id++;
        session->action_at = now;
        divoom_ha_set_action_state("Sending", 1);
        if(divoom_ha_send_scene(wire, session->config.scene_entity, session->scene_id))
            return -1;
    }
    if(divoom_ha_session_update_entities(wire, session, now))
        return -1;
    return divoom_ha_aux_tick(wire, session, now);
}

/*********************************************************************
  Function:        divoom_ha_apply_login
  Description:     校验 OAuth 配置，按调用来源保存后交由后台换取访问令牌。
  Input:           URL、客户端标识、刷新令牌；save 为 1 时保存到设备。
  Return:          0：配置已应用；-1：校验、内存或写入失败。
  调用约束:         内部加锁，HTTPS、子路径和 IPv6 当前明确拒绝，不降级为明文。
**********************************************************************/
static int divoom_ha_apply_login(const char *url, const char *client_id, const char *refresh_token, int save)
{
    struct divoom_ha_config *config;
    const char *host, *end, *colon;
    size_t n, i, url_len;
    unsigned port = 80;
    if(!pdivoom_ha_handle || !url || !client_id || !refresh_token || strncmp(url, "http://", 7) || !client_id[0] || strlen(client_id) >= sizeof(pdivoom_ha_handle->oauth.client_id) || !refresh_token[0] || strlen(refresh_token) >= sizeof(pdivoom_ha_handle->oauth.refresh_token))
        return -1;
    url_len = strlen(url);
    if(url_len >= sizeof(pdivoom_ha_handle->oauth.url))
        return -1;
    host = url + 7;
    end = url + url_len;
    if(end > host && end[-1] == '/')
        end--;
    if(end == host)
        return -1;
    colon = memchr(host, ':', (size_t)(end - host));
    n = colon ? (size_t)(colon - host) : (size_t)(end - host);
    if(!n || n >= sizeof(config->host))
        return -1;
    for(i = 0; i < n; i++)
        if(!isalnum((unsigned char)host[i]) && host[i] != '.' && host[i] != '-')
            return -1;
    if(colon)
    {
        const char *digit;
        port = 0;
        if(colon + 1 == end)
            return -1;
        for(digit = colon + 1; digit < end; digit++)
        {
            if(*digit < '0' || *digit > '9')
                return -1;
            port = port * 10 + (unsigned)(*digit - '0');
            if(port > 65535)
                return -1;
        }
        if(!port)
            return -1;
    }
    config = divoom_system_malloc(sizeof(*config));
    if(!config)
        return -1;
    memset(config, 0, sizeof(*config));
    memcpy(config->host, host, n);
    config->port = port;
    divoom_ha_lock();
    if(save && divoom_ha_save_login(url, client_id, refresh_token))
    {
        divoom_ha_unlock();
        divoom_system_free(config);
        divoom_print("ha: cannot save login\n");
        return -1;
    }
    pdivoom_ha_handle->temperature_unit[0] = 0;
    memset(&pdivoom_ha_handle->oauth, 0, sizeof(pdivoom_ha_handle->oauth));
    memcpy(pdivoom_ha_handle->oauth.url, url, (size_t)(end - url));
    strcpy(pdivoom_ha_handle->oauth.client_id, client_id);
    strcpy(pdivoom_ha_handle->oauth.refresh_token, refresh_token);
    pdivoom_ha_handle->oauth.enabled = 1;
    pdivoom_ha_handle->config = *config;
    memset(&pdivoom_ha_handle->matched_config, 0, sizeof(pdivoom_ha_handle->matched_config));
    memset(pdivoom_ha_handle->entities, 0, sizeof(pdivoom_ha_handle->entities));
    memset(&pdivoom_ha_handle->status, 0, sizeof(pdivoom_ha_handle->status));
    pdivoom_ha_handle->status.configured = 1;
    strcpy(pdivoom_ha_handle->status.state, "Waiting for token");
    pdivoom_ha_handle->scene_requested = 0;
    pdivoom_ha_handle->clock_action_slot = 0;
    pdivoom_ha_handle->revision++;
    pdivoom_ha_handle->watchface_revision++;
    divoom_ha_unlock();
    divoom_system_free(config);
    return 0;
}

int divoom_ha_login(const char *url, const char *client_id, const char *refresh_token)
{
    return divoom_ha_apply_login(url, client_id, refresh_token, 1);
}

static int divoom_ha_read_login(const char *path, struct divoom_ha_login_record *record)
{
    divoom_home_assistant_login_info *login = &record->login;
    void *file = divoom_port_file_open(path, "rb");
    unsigned char extra;
    int valid;

    if(!file)
        return -1;
    valid = divoom_port_file_read(record, 1, sizeof(*record), file) == sizeof(*record) &&
        divoom_port_file_read(&extra, 1, 1, file) == 0;
    divoom_port_file_close(file);
    if(!valid || memcmp(record->format, DIVOOM_HA_LOGIN_FORMAT, sizeof(record->format)) ||
        !memchr(login->ha_url, 0, sizeof(login->ha_url)) ||
        !memchr(login->ha_client_id, 0, sizeof(login->ha_client_id)) ||
        !memchr(login->ha_refresh_token, 0, sizeof(login->ha_refresh_token)))
        return -1;
    return divoom_ha_apply_login(login->ha_url, login->ha_client_id, login->ha_refresh_token, 0);
}

static void divoom_ha_restore_login(void)
{
    struct divoom_ha_login_record *record;

    if(!divoom_port_file_exists(DIVOOM_HA_LOGIN_FILE) && !divoom_port_file_exists(DIVOOM_HA_LOGIN_BACKUP))
        return;
    record = divoom_system_malloc(sizeof(*record));
    if(!record)
        return;
    if(divoom_ha_read_login(DIVOOM_HA_LOGIN_FILE, record) &&
        divoom_ha_read_login(DIVOOM_HA_LOGIN_BACKUP, record))
        divoom_print("ha: cannot restore saved login\n");
    else
        divoom_print("ha: restored saved login\n");
    memset(record, 0, sizeof(*record));
    divoom_system_free(record);
}

/*********************************************************************
  Function:        divoom_ha_oauth_form_encode
  Description:     将 UTF-8 字节编码为 OAuth 表单参数，避免 &、+、= 改变参数含义。
  Input:           dst：至少 strlen(src)*3+1 字节；src：以零结束的文本。
  Return:          结束符位置，可继续追加字段。
**********************************************************************/
static char *divoom_ha_oauth_form_encode(char *dst, const char *src)
{
    const char *hex = "0123456789ABCDEF";
    while(*src)
    {
        unsigned char c = (unsigned char)*src++;
        if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~')
            *dst++ = (char)c;
        else
        {
            *dst++ = '%';
            *dst++ = hex[c >> 4];
            *dst++ = hex[c & 15];
        }
    }
    *dst = 0;
    return dst;
}

/*********************************************************************
  Function:        divoom_ha_oauth_refresh
  Description:     HA 任务换取或更新访问令牌，网络期间不持有状态锁；旧配置响应丢弃。
  Input:           revision：当前连接配置版本。
  Return:          0：令牌可用；-1：刷新失败或配置已改变；失败由原任务延时重试。
**********************************************************************/
static int divoom_ha_oauth_refresh(unsigned revision)
{
    struct divoom_ha_oauth_config *oauth;
    char *form = NULL, *response = NULL, *cursor;
    char token_url[272];
    divoom_cJSON *root = NULL, *expiry;
    const char *token;
    const char *failure = "form allocation failed";
    int result = -1;
    int http_status = 0;
    oauth = divoom_system_malloc(sizeof(*oauth));
    if(!oauth)
    {
        divoom_print("ha oauth: configuration snapshot allocation failed\n");
        return -1;
    }
    divoom_ha_lock();
    *oauth = pdivoom_ha_handle->oauth;
    if(revision != pdivoom_ha_handle->revision)
        oauth->enabled = 0;
    if(revision == pdivoom_ha_handle->revision && (!oauth->enabled || (pdivoom_ha_handle->config.access_token[0] && (int)(oauth->refresh_at - divoom_port_tick_ms()) > 0)))
        result = 0;
    divoom_ha_unlock();
    if(!result || !oauth->enabled)
        goto DONE;
    form = divoom_system_malloc((strlen(oauth->client_id) + strlen(oauth->refresh_token)) * 3 + 80);
    if(!form)
        goto DONE;
    strcpy(form, "grant_type=refresh_token&client_id=");
    cursor = divoom_ha_oauth_form_encode(form + strlen(form), oauth->client_id);
    strcpy(cursor, "&refresh_token=");
    divoom_ha_oauth_form_encode(cursor + strlen(cursor), oauth->refresh_token);
    snprintf(token_url, sizeof(token_url), "%s/auth/token", oauth->url);
    failure = "token HTTP request failed or response exceeds limit";
    response = divoom_ha_http_post_form(token_url, form, &http_status);
    if(!response)
        goto DONE;
    if(http_status != 200)
    {
        divoom_print("ha oauth: token request rejected; HTTP status=%d\n", http_status);
        failure = "token endpoint returned an unsuccessful HTTP status";
        goto DONE;
    }
    failure = "token response is not valid JSON";
    root = divoom_cJSON_Parse(response);
    if(!root)
        goto DONE;
    failure = "access_token or expires_in missing, invalid or exceeds limit";
    token = divoom_ha_json_get_string(root, "access_token");
    expiry = divoom_cJSON_GetObjectItem(root, "expires_in");
    if(!token || !token[0] || strlen(token) >= DIVOOM_HA_TOKEN_MAX || !expiry || (expiry->type & 255) != divoom_cJSON_Number || !isfinite(expiry->valuedouble) || expiry->valuedouble < 2 || expiry->valuedouble > 86400 || expiry->valuedouble != expiry->valueint)
        goto DONE;
    divoom_ha_lock();
    if(revision == pdivoom_ha_handle->revision && pdivoom_ha_handle->oauth.enabled)
    {
        unsigned seconds = (unsigned)expiry->valueint;
        strcpy(pdivoom_ha_handle->config.access_token, token);
        pdivoom_ha_handle->oauth.refresh_at = divoom_port_tick_ms() + (seconds > 60 ? seconds - 30 : seconds / 2) * 1000;
        result = 0;
    }
    divoom_ha_unlock();
DONE:
    if(result && divoom_ha_config_is_current(revision))
        divoom_print("ha oauth: %s\n", failure);
    if(root)
        divoom_cJSON_Delete(root);
    if(response)
    {
        memset(response, 0, strlen(response));
        divoom_system_free(response);
    }
    if(form)
    {
        memset(form, 0, strlen(form));
        divoom_system_free(form);
    }
    memset(oauth, 0, sizeof(*oauth));
    divoom_system_free(oauth);
    return result;
}

/*********************************************************************
  Function:        divoom_ha_oauth_expired
  Description:     检查续期时间，令 HA 任务关闭旧连接并在下一轮刷新后恢复订阅。
  Input:           无。
  Return:          1：需要续期；0：工程访问令牌模式或尚未到期。
**********************************************************************/
static int divoom_ha_oauth_expired(void)
{
    int expired;
    divoom_ha_lock();
    expired = pdivoom_ha_handle->oauth.enabled && (int)(divoom_port_tick_ms() - pdivoom_ha_handle->oauth.refresh_at) >= 0;
    divoom_ha_unlock();
    return expired;
}

/*********************************************************************
  Function:        divoom_ha_task
  Description:     HA 常驻任务，负责连接、认证、消息分发、心跳及真实断线后的重连。
  Input:           unused：任务接口保留参数，当前不用。
  Return:          正常长期循环不返回；启动内存不足时返回 NULL。
  调用约束:         由系统任务接口创建，唯一持有并操作 HA WebSocket 的任务。
**********************************************************************/
static void *divoom_ha_task(void *unused)
{
    struct divoom_ha_session *session = divoom_system_malloc(sizeof(*session));
    unsigned blocked = 0;
    unsigned aux_blocked = 0;
    (void)unused;
    if(!session)
    {
        divoom_ha_set_connection_state("No memory", 0);
        return NULL;
    }
    for(;;)
    {
        unsigned revision, start;
        int enabled, auth_sent = 0, fatal_auth = 0, oversized = 0;
        struct divoom_ha_wire *wire = NULL;
        memset(session, 0, sizeof(*session));
        session->next_id = 1;
        divoom_ha_lock();
        enabled = pdivoom_ha_handle->status.configured;
        revision = pdivoom_ha_handle->revision;
        if(enabled && revision != blocked)
            session->config = pdivoom_ha_handle->config;
        divoom_ha_unlock();
        session->connection_revision = revision;
        session->aux_disabled = aux_blocked && revision == aux_blocked;
        if(!enabled || revision == blocked || !divoom_main_get_wifi_con_status() || divoom_onoff_is_poweroff_active())
        {
            divoom_port_delay_ms(500);
            continue;
        }
        divoom_ha_set_connection_state("Refreshing token", 0);
        if(divoom_ha_oauth_refresh(revision))
        {
            if(divoom_ha_config_is_current(revision))
                divoom_ha_set_connection_state("Token refresh failed", 0);
            goto RETRY;
        }
        divoom_ha_lock();
        session->config = pdivoom_ha_handle->config;
        divoom_ha_unlock();
        if(!divoom_ha_config_is_current(revision))
            goto RETRY;
        divoom_ha_set_connection_state("Connecting", 0);
        if(divoom_ha_wire_open(&wire, session->config.host, session->config.port))
        {
            divoom_ha_set_connection_state("Connect failed", 0);
            goto RETRY;
        }
        start = divoom_port_tick_ms();
        divoom_ha_set_connection_state("Authenticating", 0);
        while(divoom_ha_config_is_current(revision) && divoom_main_get_wifi_con_status() && !divoom_onoff_is_poweroff_active())
        {
            const char *text = NULL;
            const char *parse_end = NULL;
            divoom_cJSON *root, *id;
            unsigned request_id = 0, now = divoom_port_tick_ms();
            int received, failed = 0;
            if(divoom_ha_oauth_expired())
                break;
            if(!session->authenticated && now - start > 10000)
                break;
            if(session->authenticated && divoom_ha_session_tick(wire, session, now))
                break;
            received = divoom_ha_wire_receive_text(wire, &text);
            if(received < 0)
            {
                oversized = received == -2;
                if(oversized && session->aux.id)
                {
                    /* 可选查找超限时重连，只恢复主实体订阅。 */
                    aux_blocked = revision;
                    oversized = 0;
                }
                break;
            }
            if(!received)
                continue;
            root = divoom_cJSON_ParseWithOpts(text, &parse_end, 1);
            if(!root)
            {
                if(session->aux.id)
                    aux_blocked = revision;
                break;
            }
            id = divoom_cJSON_GetObjectItem(root, "id");
            if(id && (id->type & 255) == divoom_cJSON_Number && id->valueint > 0)
                request_id = (unsigned)id->valueint;
            if(divoom_ha_json_string_equals(root, "type", "auth_invalid"))
                fatal_auth = 1;
            else if(!auth_sent && !session->authenticated && divoom_ha_json_string_equals(root, "type", "auth_required"))
            {
                failed = divoom_ha_send_auth(wire, session->config.access_token);
                auth_sent = 1;
            }
            else if(auth_sent && !session->authenticated && divoom_ha_json_string_equals(root, "type", "auth_ok"))
            {
                divoom_ha_session_authenticate(session, now);
                session->config_id = session->next_id++;
                failed = divoom_ha_send_command(wire, "get_config", session->config_id, 0);
            }
            else if(session->authenticated && divoom_ha_json_string_equals(root, "type", "result"))
                failed = divoom_ha_session_result(wire, session, root, request_id, now);
            else if(session->authenticated && divoom_ha_json_string_equals(root, "type", "pong") && request_id == session->ping_id)
                session->ping_id = 0;
            else if(session->authenticated && divoom_ha_json_string_equals(root, "type", "event"))
            {
                if(session->entity_id && request_id == session->entity_id)
                    divoom_ha_handle_entity_event(root, &session->active_config, session->entity_revision);
            }
            divoom_cJSON_Delete(root);
            if(failed || fatal_auth)
                break;
        }
        if(session->scene_id)
            divoom_ha_set_action_state("Result unknown; not retried", 0);
        divoom_ha_wire_close(wire);
        if(oversized)
        {
            blocked = revision;
            divoom_ha_set_connection_state("HA message exceeds 128 KiB", 0);
        }
        else if(fatal_auth)
        {
            divoom_ha_lock();
            if(revision == pdivoom_ha_handle->revision && pdivoom_ha_handle->oauth.enabled)
                pdivoom_ha_handle->config.access_token[0] = 0;
            else
                blocked = revision;
            divoom_ha_unlock();
            divoom_ha_set_connection_state("Check token / permissions", 0);
        }
        else
            divoom_ha_set_connection_state("Disconnected", 0);
    RETRY:
        memset(session, 0, sizeof(*session));
        {
            unsigned i;
            for(i = 0; i < 30 && divoom_ha_config_is_current(revision); i++)
                divoom_port_delay_ms(100);
        }
    }
    return NULL;
}
/*********************************************************************
  Function:        divoom_ha_init
  Description:     分配核心状态、创建系统锁、设置默认温度需求并启动后台任务。
  Input:           无。
  Return:          0：成功或已初始化；-1：内存、锁或任务创建失败。
  调用约束:         启动阶段串行调用；任务栈使用配置宏，失败释放初始化资源。
**********************************************************************/
int divoom_ha_init(void)
{
    if(pdivoom_ha_handle)
        return 0;
    pdivoom_ha_handle = divoom_system_malloc(sizeof(*pdivoom_ha_handle));
    if(!pdivoom_ha_handle)
        return -1;
    memset(pdivoom_ha_handle, 0, sizeof(*pdivoom_ha_handle));
    pdivoom_ha_handle->lock = divoom_system_create_lock();
    if(!pdivoom_ha_handle->lock)
        goto ERROR_RETURN;
    strcpy(pdivoom_ha_handle->status.state, "Not configured");
    pdivoom_ha_handle->watchface.requirement_count = 2;
    strcpy(pdivoom_ha_handle->watchface.requirements[0].domain, "sensor");
    strcpy(pdivoom_ha_handle->watchface.requirements[0].device_class, "temperature");
    strcpy(pdivoom_ha_handle->watchface.requirements[1].domain, "input_number");
    strcpy(pdivoom_ha_handle->watchface.requirements[1].device_class, "temperature");
    divoom_ha_restore_login();
    pdivoom_ha_handle->task_handle = divoom_system_create_task(divoom_ha_task, "HA_TASK", NULL, DIVOOM_TASK_STACK_HA_CLIENT);
    if(!pdivoom_ha_handle->task_handle)
        goto ERROR_RETURN;
    return 0;

ERROR_RETURN:
    if(pdivoom_ha_handle->lock)
        divoom_system_destroy_lock(pdivoom_ha_handle->lock);
    divoom_system_free(pdivoom_ha_handle);
    pdivoom_ha_handle = NULL;
    return -1;
}

/*********************************************************************
  Function:        divoom_ha_login_free
  Description:     命令完成或被丢弃时清零刷新令牌，再按框架约定释放参数及命令。
  Input:           pcmd_info：命令独占对象。
  Return:          无，调用后对象失效。
**********************************************************************/
static void divoom_ha_login_free(Pdivoom_net_command pcmd_info)
{
    if(pcmd_info->cmd_data)
    {
        memset(pcmd_info->cmd_data, 0, sizeof(divoom_home_assistant_login_info));
        divoom_system_free(pcmd_info->cmd_data);
    }
    divoom_system_free(pcmd_info);
}

/*********************************************************************
  Function:        divoom_ha_login_unpack
  Description:     校验云端 HA 配置并复制到命令独占的动态内存，禁止记录令牌。
  Input:           root：命令 JSON；pcmd_info：命令对象。
  Return:          1：成功；0：字段非法或内存不足，框架统一释放命令。
**********************************************************************/
int divoom_ha_login_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info)
{
    divoom_home_assistant_login_info *info;
    divoom_cJSON *url = divoom_cJSON_GetObjectItem(root, "HaUrl");
    divoom_cJSON *client = divoom_cJSON_GetObjectItem(root, "HaClientId");
    divoom_cJSON *token = divoom_cJSON_GetObjectItem(root, "HaRefreshToken");
    if(!url || (url->type & 255) != divoom_cJSON_String || !url->valuestring || !url->valuestring[0] || strlen(url->valuestring) >= sizeof(info->ha_url) || !client || (client->type & 255) != divoom_cJSON_String || !client->valuestring || !client->valuestring[0] || strlen(client->valuestring) >= sizeof(info->ha_client_id) || !token || (token->type & 255) != divoom_cJSON_String || !token->valuestring || !token->valuestring[0] || strlen(token->valuestring) >= sizeof(info->ha_refresh_token))
        return 0;
    info = divoom_system_malloc(sizeof(*info));
    if(!info)
        return 0;
    memset(info, 0, sizeof(*info));
    strcpy(info->ha_url, url->valuestring);
    strcpy(info->ha_client_id, client->valuestring);
    strcpy(info->ha_refresh_token, token->valuestring);
    pcmd_info->cmd_data = info;
    pcmd_info->pfreefunc = divoom_ha_login_free;
    return 1;
}

/*********************************************************************
  Function:        divoom_ha_result_pack
  Description:     打包设备配置接收结果，不回传任何 HA 凭据；成功不代表 HA 已连接。
  Input:           root：应答 JSON；pcmd_info：cmd_data 指向同步返回码。
  Return:          无。
**********************************************************************/
void divoom_ha_result_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info)
{
    int result = pcmd_info->cmd_data ? *(int *)pcmd_info->cmd_data : 1;
    divoom_cJSON_DeleteItemFromObject(root, "ReturnCode");
    divoom_cJSON_DeleteItemFromObject(root, "ReturnMessage");
    divoom_cJSON_AddNumberToObject(root, "ReturnCode", result);
    divoom_cJSON_AddStringToObject(root, "ReturnMessage", result ? "Invalid HA configuration or unsupported URL" : "");
}
