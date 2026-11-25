#if !defined(_WIN32)
#include <sys/errno.h>
#endif
#include "divoom.h"
#include "cJSON.h"
#include "divoom_wifi_con.h"
#include "divoom_boot_health.h"
#include "divoom_product_test_userdata_wifi.h"
#include "divoom_recovery_journal.h"
#include "divoom_product_test_repair.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>

#define WIFI_JSON_LIMIT (64 * 1024)

static char *userdata_read_text(const char *path, int limit)
{
    void *fp = divoom_port_file_open(path, "rb");
    if (!fp) return NULL;
    divoom_port_stat_t st;
    char *text = NULL;
    int ok = divoom_port_file_stat(fp, &st) == 0;
    if (ok && (st.size <= 0 || st.size > limit)) { errno = EINVAL; ok = 0; }
    if (ok) {
        text = divoom_system_malloc((size_t)st.size + 1);
        if (!text) { errno = ENOMEM; ok = 0; }
    }
    if (ok) {
        unsigned char extra;
        ok = divoom_port_file_read(text, 1, st.size, fp) == (size_t)st.size &&
            divoom_port_file_read(&extra, 1, 1, fp) == 0 && !ferror((FILE *)fp) &&
            !memchr(text, 0, (size_t)st.size);
        if (!ok && !errno) errno = EIO;
        text[st.size] = 0;
    }
    int saved_errno = errno;
    if (divoom_port_file_close(fp) != 0) { ok = 0; saved_errno = errno; }
    if (!ok) { divoom_system_free(text); text = NULL; }
    errno = saved_errno;
    return text;
}

static divoom_cJSON *userdata_wifi_read(const char *path)
{
    char *text = userdata_read_text(path, WIFI_JSON_LIMIT);
    if (!text) return NULL;
    divoom_cJSON *root = divoom_cJSON_ParseWithOpts(text, NULL, 1);
    divoom_system_free(text);
    if (!root) errno = EINVAL;
    return root;
}

static int userdata_wifi_valid(divoom_cJSON *config)
{
    divoom_cJSON *list = divoom_cJSON_GetObjectItem(config, "wifi"), *item;
    if (!config || config->type != divoom_cJSON_Object || !list ||
        list->type != divoom_cJSON_Array || divoom_cJSON_GetArraySize(list) > 256) return 0;
    for (item = list->child; item; item = item->next) {
        divoom_cJSON *ssid = divoom_cJSON_GetObjectItem(item, "ssid");
        divoom_cJSON *password = divoom_cJSON_GetObjectItem(item, "password");
        if (item->type != divoom_cJSON_Object || !ssid || !password ||
            ssid->type != divoom_cJSON_String || password->type != divoom_cJSON_String ||
            !ssid->valuestring || !password->valuestring || !ssid->valuestring[0] ||
            strlen(ssid->valuestring) > 32 || strlen(password->valuestring) > 64) return 0;
    }
    return 1;
}

static int userdata_config_write(const char *path, const char *text)
{
    size_t size = strlen(text);
    void *fp = divoom_port_file_open(path, "wb");
    if (!fp) {divoom_product_test_repair_record_io(path,"userdata_config.open_write",errno);return 0;}
    errno=0;
    int ok = divoom_port_file_write(text, 1, size, fp) == size && divoom_port_file_sync(fp) == 0;
    if (!ok) divoom_product_test_repair_record_io(path,"userdata_config.write_sync",errno?errno:EIO);
    if (divoom_port_file_close(fp) != 0) {divoom_product_test_repair_record_io(path,"userdata_config.close",errno);ok = 0;}
    if (ok && divoom_port_sync()!=0) {divoom_product_test_repair_record_io(path,"userdata_config.sync",errno);ok=0;}
    return ok;
}

static int userdata_config_matches(const char *path,const char *text)
{
    unsigned char buffer[512];size_t offset=0,size=strlen(text);
    errno=0;void *file=divoom_port_file_open(path,"rb");
    if(!file){divoom_product_test_repair_record_io(path,"userdata_config.verify_open",errno);return 0;}
    int ok=1;
    while(offset<size){
        size_t want=size-offset;if(want>sizeof(buffer))want=sizeof(buffer);
        errno=0;size_t got=divoom_port_file_read(buffer,1,want,file);
        if(got!=want||memcmp(buffer,text+offset,want)){
            divoom_product_test_repair_record_io(path,"userdata_config.verify_content",errno?errno:EIO);ok=0;break;
        }
        offset+=want;
    }
    if(ok){errno=0;size_t extra=divoom_port_file_read(buffer,1,1,file);
        if(extra||ferror((FILE*)file)){divoom_product_test_repair_record_io(path,"userdata_config.verify_end",errno?errno:EIO);ok=0;}}
    if(divoom_port_file_close(file)!=0){divoom_product_test_repair_record_io(path,"userdata_config.verify_close",errno);ok=0;}
    return ok;
}

int divoom_product_test_userdata_wifi_pending(void)
{
    int pending = divoom_recovery_journal_pending();
    if (pending < 0) return 0; /* Main recovery entry handles unread raw state. */
    return !!(divoom_recovery_journal_get()->pending_mask & 1U);
}

static divoom_cJSON *userdata_wifi_memory(int *count)
{
    Pdivoom_wifi_menu_wifi_info records = divoom_wifi_userdata_format_snapshot(count);
    if (*count < 0) return NULL;
    divoom_cJSON *root = divoom_cJSON_CreateObject(), *list = divoom_cJSON_CreateArray();
    if (!root || !list) {
        divoom_cJSON_Delete(root); divoom_cJSON_Delete(list); divoom_system_free(records);
        errno = ENOMEM; return NULL;
    }
    divoom_cJSON_AddItemToObject(root, "wifi", list);
    if (divoom_cJSON_GetObjectItem(root, "wifi") != list) {
        divoom_cJSON_Delete(root); divoom_system_free(records);
        errno = ENOMEM; return NULL;
    }
    for (int i = 0; i < *count; ++i) {
        char ssid[33] = {0}, password[65] = {0};
        memcpy(ssid, records[i].ssid, 32); memcpy(password, records[i].pswd, 64);
        divoom_cJSON *item = divoom_cJSON_CreateObject();
        if (item) {
            divoom_cJSON_AddStringToObject(item, "ssid", ssid);
            divoom_cJSON_AddStringToObject(item, "password", password);
        }
        if (!item || !divoom_cJSON_GetObjectItem(item, "ssid") || !divoom_cJSON_GetObjectItem(item, "password")) {
            divoom_cJSON_Delete(item); divoom_cJSON_Delete(root); divoom_system_free(records);
            errno = ENOMEM; return NULL;
        }
        divoom_cJSON_AddItemToArray(list, item);
    }
    divoom_system_free(records);
    return root;
}

int divoom_product_test_userdata_wifi_prepare(void)
{
    divoom_recovery_record *record = divoom_system_malloc(sizeof(*record));
    if (!record) return 0;
    memcpy(record, divoom_recovery_journal_get(), sizeof(*record));
    int capture_config = record->phase == DIVOOM_RECOVERY_FORMAT && !record->target_usr &&
        !record->plan_count && !(record->flags & DIVOOM_RECOVERY_SYSTEM_CONFIG);
    if (record->wifi_size && !capture_config) { divoom_system_free(record); return 1; }
    if (capture_config) {
        char *snapshot = divoom_product_test_userdata_config_snapshot();
        if (snapshot) {
            record->system_config.size = (uint32_t)strlen(snapshot);
            memcpy(record->system_config.json, snapshot, record->system_config.size + 1);
            record->flags |= DIVOOM_RECOVERY_SYSTEM_CONFIG;
            divoom_system_free(snapshot);
        }
    }
    if (!record->wifi_size) {
        errno = 0;
        divoom_cJSON *config = userdata_wifi_read(DIVOOM_WPA_SUPPLICANT_CONF);
        if (!userdata_wifi_valid(config)) {
            divoom_cJSON_Delete(config);
            int count = 0;
            config = userdata_wifi_memory(&count);
        }
        char *text = userdata_wifi_valid(config) &&
            divoom_cJSON_GetArraySize(divoom_cJSON_GetObjectItem(config, "wifi")) > 0 ?
            divoom_cJSON_PrintUnformatted(config) : NULL;
        if (text && strlen(text) < sizeof(record->wifi)) {
            record->wifi_size = (uint32_t)strlen(text);
            memcpy(record->wifi, text, record->wifi_size + 1);
        } else {
            record->wifi_size = 0;
            record->wifi[0] = 0;
            record->flags |= 1U; /* Best effort: credentials unavailable, continue. */
        }
        divoom_system_free(text); divoom_cJSON_Delete(config);
    }
    int ok = divoom_recovery_journal_save(record);
    divoom_print("userdata_wifi: snapshot saved=%d available=%d\n", ok, record->wifi_size != 0);
    if (capture_config)
        divoom_print("userdata_system_config: snapshot saved=%d available=%d\n", ok,
            !!(record->flags & DIVOOM_RECOVERY_SYSTEM_CONFIG));
    divoom_system_free(record);
    return ok;
}

/* No allocation and no global cJSON hooks: malformed optional snapshots may
 * be discarded, but a syntactically valid document that cannot allocate waits. */
static void wifi_json_space(const char **p){while(**p==' '||**p=='\t'||**p=='\r'||**p=='\n')++*p;}
static int wifi_json_hex(const char **p,unsigned *out)
{
    unsigned value=0;
    for(int i=0;i<4;i++){
        unsigned char c=(unsigned char)**p;unsigned digit;
        if(c>='0'&&c<='9')digit=c-'0';
        else if(c>='a'&&c<='f')digit=c-'a'+10;
        else if(c>='A'&&c<='F')digit=c-'A'+10;
        else return 0;
        value=value*16+digit;++*p;
    }
    *out=value;return 1;
}
static int wifi_json_string(const char **p)
{
    if(*(*p)++!='"')return 0;
    while(**p && **p!='"'){
        unsigned char c=*(*p)++;if(c<32)return 0;
        if(c=='\\'){
            c=*(*p)++;if(!c)return 0;
            if(c=='u'){
                unsigned code;if(!wifi_json_hex(p,&code))return 0;
                if(code>=0xdc00&&code<=0xdfff)return 0;
                if(code>=0xd800&&code<=0xdbff){
                    if((*p)[0]!='\\'||(*p)[1]!='u')return 0;
                    *p+=2;
                    if(!wifi_json_hex(p,&code)||code<0xdc00||code>0xdfff)return 0;
                }
            }
            else if(!strchr("\"\\/bfnrt",c))return 0;
        }
    }
    if(**p!='"')return 0;
    ++*p;return 1;
}
static int userdata_json_value(const char **p,int depth,int bounded_numbers)
{
    if(depth>32)return 0;
    wifi_json_space(p);
    char c=**p;
    if(c=='"')return wifi_json_string(p);
    if(c=='{'||c=='['){
        char close=c=='{'?'}':']';++*p;wifi_json_space(p);
        if(**p==close){++*p;return 1;}
        for(;;){
            if(c=='{'){if(**p!='"'||!wifi_json_string(p))return 0;wifi_json_space(p);if(**p!=':')return 0;++*p;}
            if(!userdata_json_value(p,depth+1,bounded_numbers))return 0;
            wifi_json_space(p);
            if(**p==close){++*p;return 1;}if(**p!=',')return 0;++*p;wifi_json_space(p);
        }
    }
    const char *literal=c=='t'?"true":c=='f'?"false":c=='n'?"null":NULL;
    if(literal){size_t n=strlen(literal);if(strncmp(*p,literal,n))return 0;*p+=n;return 1;}
    const char *number=*p;
    if(c=='-')++*p;
    if(**p=='0')++*p;
    else{if(**p<'1'||**p>'9')return 0;while(**p>='0'&&**p<='9')++*p;}
    if(**p=='.'){++*p;if(**p<'0'||**p>'9')return 0;while(**p>='0'&&**p<='9')++*p;}
    if(**p=='e'||**p=='E'){
        ++*p;
        if(**p=='+'||**p=='-')++*p;
        const char *exponent=*p;
        while(**p>='0'&&**p<='9')++*p;
        if(*p==exponent)return 0;
        if(bounded_numbers&&(*p-exponent>3||atoi(exponent)>308))return 0;
    }
    if(bounded_numbers){
        if(*p-number>32)return 0;
        char *end;
        double value=strtod(number,&end);
        /* Leave headroom for the legacy parser's digit-by-digit rounding. */
        if(end!=*p||!isfinite(value)||value<INT_MIN||value>=0x1p53)return 0;
    }
    return 1;
}

static int userdata_config_string_valid(const divoom_cJSON *object, const char *key, size_t capacity)
{
    divoom_cJSON *item = divoom_cJSON_GetObjectItem(object, key);
    return !item || (item->type == divoom_cJSON_String && item->valuestring &&
        strlen(item->valuestring) < capacity);
}

static divoom_cJSON *userdata_config_unique_item(const divoom_cJSON *object, const char *key, int *valid)
{
    divoom_cJSON *item = divoom_cJSON_GetObjectItem(object, key);
    if (item && item->next) {
        divoom_cJSON remaining = {.child = item->next};
        if (divoom_cJSON_GetObjectItem(&remaining, key)) *valid = 0;
    }
    return item;
}

/* cJSON preserves member order. Use that order to locate the original
 * value token, including keys written with Unicode escapes. */
static char *userdata_config_value_start(char *object, const divoom_cJSON *parent, const divoom_cJSON *item)
{
    const char *p = object;
    wifi_json_space(&p);
    if (*p++ != '{') return NULL;
    for (const divoom_cJSON *child = parent->child; child; child = child->next) {
        wifi_json_space(&p);
        if (*p != '"' || !wifi_json_string(&p)) return NULL;
        wifi_json_space(&p);
        if (*p++ != ':') return NULL;
        wifi_json_space(&p);
        if (child == item) return (char *)p;
        if (!userdata_json_value(&p, 0, 1)) return NULL;
        wifi_json_space(&p);
        if (*p++ != ',') break;
    }
    return NULL;
}

static char *userdata_config_reset_id(char *source, const divoom_cJSON *root,
    const divoom_cJSON *settings, const divoom_cJSON *device_id)
{
    char *object = userdata_config_value_start(source, root, settings);
    if (!object) return NULL;
    if (device_id) {
        char *id = userdata_config_value_start(object, settings, device_id);
        if (!id) return NULL;
        const char *end = id;
        if (!userdata_json_value(&end, 0, 1)) return NULL;
        *id = '0';
        memmove(id + 1, end, strlen(end) + 1);
        return source;
    }
    const char *field = settings->child ? "\"device_id\":0," : "\"device_id\":0";
    size_t prefix = (size_t)(object + 1 - source), length = strlen(source), added = strlen(field);
    if (length + added >= DIVOOM_RECOVERY_SYSTEM_CONFIG_MAX) return NULL;
    char *text = divoom_system_malloc(length + added + 1);
    if (text) {
        memcpy(text, source, prefix);
        memcpy(text + prefix, field, added);
        memcpy(text + prefix + added, source + prefix, length - prefix + 1);
    }
    return text;
}

char *divoom_product_test_userdata_config_snapshot(void)
{
    char *source = userdata_read_text(DIVOOM_SYS_CFG_SAVE_PATH, DIVOOM_RECOVERY_SYSTEM_CONFIG_MAX - 1);
    if (!source) return NULL;
    const char *end = source;
    int valid = userdata_json_value(&end, 0, 1);
    if (valid) wifi_json_space(&end);
    divoom_cJSON *root = valid && !*end ? divoom_cJSON_ParseWithOpts(source, NULL, 1) : NULL;
    divoom_cJSON *settings = userdata_config_unique_item(root, "sys_setting", &valid);
    divoom_cJSON *blue = divoom_cJSON_GetObjectItem(root, "blue_info");
    divoom_cJSON *power = divoom_cJSON_GetObjectItem(root, "PowerOnInfo");
    divoom_cJSON *device_id = userdata_config_unique_item(settings, "device_id", &valid);
    char *text = NULL;
    if (valid && root && root->type == divoom_cJSON_Object && settings && settings->type == divoom_cJSON_Object &&
        (!blue || blue->type == divoom_cJSON_Object) && (!power || power->type == divoom_cJSON_Array) &&
        (!device_id || device_id->type == divoom_cJSON_Number) &&
        userdata_config_string_valid(settings, "power_on_id", 64) &&
        userdata_config_string_valid(settings, "lat", 11) &&
        userdata_config_string_valid(settings, "lot", 11) &&
        userdata_config_string_valid(settings, "time_zone", 64) &&
        userdata_config_string_valid(blue, "spp_secret", 6) &&
        userdata_config_string_valid(blue, "blue_name", 32))
        text = userdata_config_reset_id(source, root, settings, device_id);
    divoom_cJSON_Delete(root);
    if (text != source) divoom_system_free(source);
    return text;
}
static int userdata_wifi_drop_invalid(void)
{
    divoom_recovery_record *copy=divoom_system_malloc(sizeof(*copy));if(!copy)return 0;
    memcpy(copy,divoom_recovery_journal_get(),sizeof(*copy));copy->wifi_size=0;copy->wifi[0]=0;copy->flags|=1U;
    int ok=divoom_recovery_journal_save(copy);divoom_system_free(copy);return ok;
}

static int userdata_config_restore(const char *path, const char *temp, const char *text)
{
    int ok = divoom_port_mkdir_p(DIVOOM_USER_SYSTEM_DIR) == 0 &&
        userdata_config_write(temp, text);
    if (!ok) divoom_product_test_repair_record_io(path,"userdata_config.mkdir_or_write",errno);
    if (ok) ok = userdata_config_matches(temp, text);
    if (ok) {
        errno = 0;
        ok = (divoom_port_remove_file(path) == 0 || errno == ENOENT) &&
            divoom_port_rename_file(temp, path) == 0 && divoom_port_sync() == 0;
        if (!ok) divoom_product_test_repair_record_io(path,"userdata_config.rename_sync",errno);
    }
    return ok && userdata_config_matches(path, text);
}

static int userdata_system_config_restore(void)
{
    const divoom_recovery_record *record = divoom_recovery_journal_get();
    if (!(record->format_mask & 1U) || !(record->flags & DIVOOM_RECOVERY_SYSTEM_CONFIG) ||
        !(record->pending_mask & 1U) || record->target_usr ||
        record->plan_count || !record->system_config.size ||
        record->system_config.size >= sizeof(record->system_config.json) ||
        record->system_config.json[record->system_config.size] ||
        memchr(record->system_config.json, 0, record->system_config.size)) return 1;
    int ok = userdata_config_restore(DIVOOM_SYS_CFG_SAVE_PATH, DIVOOM_SYS_CFG_SAVE_PATH ".restore",
        record->system_config.json);
    divoom_print("userdata_system_config: restore %s\n", ok ? "ok" : "failed");
    /* Keep the durable snapshot for a retry without requesting another erase. */
    divoom_product_test_repair_io_error_target(1);
    return ok;
}

static int userdata_wifi_restore(void)
{
    const divoom_recovery_record *record = divoom_recovery_journal_get();
    if (!record->wifi_size) return 1;
    divoom_product_test_repair_io_error_target(1);
    const char *end=record->wifi;
    if(!userdata_json_value(&end,0,0))return userdata_wifi_drop_invalid();
    wifi_json_space(&end);if(*end)return userdata_wifi_drop_invalid();
    divoom_cJSON *config = divoom_cJSON_ParseWithOpts(record->wifi, NULL, 1);
    if (!config) return 0; /* Syntax is valid, allocation may have failed. */
    if (!userdata_wifi_valid(config)) { divoom_cJSON_Delete(config); return userdata_wifi_drop_invalid(); }
    char *text = divoom_cJSON_PrintUnformatted(config);
    int ok = text && userdata_config_restore(DIVOOM_WPA_SUPPLICANT_CONF,
        DIVOOM_WPA_SUPPLICANT_CONF ".restore", text);
    divoom_system_free(text); divoom_cJSON_Delete(config);
    if (ok) divoom_product_test_repair_io_error_target(1);
    divoom_print("userdata_wifi: restore %s\n", ok ? "ok" : "failed");
    return ok;
}

int divoom_product_test_userdata_wifi_restore(void)
{
    return userdata_wifi_restore() && userdata_system_config_restore();
}

int divoom_product_test_userdata_wifi_finish(void)
{
    divoom_recovery_record *record = divoom_system_malloc(sizeof(*record));
    if (!record) return 0;
    memcpy(record, divoom_recovery_journal_get(), sizeof(*record));
    record->wifi_size = 0; memset(record->wifi, 0, sizeof(record->wifi));
    divoom_recovery_record_clear_system_config(record);
    int ok = divoom_recovery_journal_save(record); divoom_system_free(record); return ok;
}
