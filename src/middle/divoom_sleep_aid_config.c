#include "divoom.h"
#include "divoom_sleep_aid_ctrl.h"
#include "divoom_main.h"
#include "divoom_voice_read.h"
#include "divoom_menu_control.h"

typedef struct
{
    divoom_sleep_aid_config_item *buf;
    int n;
    int cap;
} divoom_sleep_aid_vec;

/** Persisted cfg slots: noise / white / music — indices match GetAllList Type and sleep_aid_%02d.cfg. */
#define DIVOOM_SLEEP_AID_STORED_TYPE_NUM 3

static int sleep_aid_load_vec_from_file(int type, divoom_sleep_aid_vec *out_vec);
static int sleep_aid_save_vec_to_file(int type, const divoom_sleep_aid_vec *v);

static int sleep_aid_vec_reserve(divoom_sleep_aid_vec *v, int need)
{
    divoom_sleep_aid_config_item *nb;
    int new_cap;

    if (need <= v->cap)
        return 0;
    new_cap = v->cap ? v->cap : 8;
    while (new_cap < need)
        new_cap *= 2;
    nb = (divoom_sleep_aid_config_item *)divoom_system_malloc((unsigned)new_cap * sizeof(divoom_sleep_aid_config_item));
    if (!nb)
        return -1;
    if (v->n > 0 && v->buf)
        memcpy(nb, v->buf, (unsigned)v->n * sizeof(divoom_sleep_aid_config_item));
    if (v->buf)
        divoom_system_free(v->buf);
    v->buf = nb;
    v->cap = new_cap;
    return 0;
}

static void sleep_aid_vec_free(divoom_sleep_aid_vec *v)
{
    if (v->buf)
        divoom_system_free(v->buf);
    v->buf = NULL;
    v->n = 0;
    v->cap = 0;
}

static int sleep_aid_vec_find(const divoom_sleep_aid_vec *v, uint32 voice_id)
{
    int i;

    for (i = 0; i < v->n; i++)
    {
        if (v->buf[i].voice_id == voice_id)
            return i;
    }
    return -1;
}

static int sleep_aid_vec_append(divoom_sleep_aid_vec *v, const divoom_sleep_aid_config_item *it)
{
    if (sleep_aid_vec_reserve(v, v->n + 1) != 0)
        return -1;
    v->buf[v->n] = *it;
    v->n++;
    return 0;
}

static int sleep_aid_vec_max_update_time(const divoom_sleep_aid_vec *v)
{
    int i;
    int mx = 0;

    if (!v || !v->buf || v->n <= 0)
        return 0;
    for (i = 0; i < v->n; i++)
    {
        if (v->buf[i].update_time > mx)
            mx = v->buf[i].update_time;
    }
    return mx;
}

/**
 * Central JSON on DIVOOM_SLEEP_AID_UPDATE_TIME_PATH:
 *   {"SleepUpdateList":[{"AUDIO_TYPE":0,"UPDATE_TIME":123}, ...]}
 * Only types that exist locally are stored; omitted AUDIO_TYPE => UpdateTime 0 on GetAllList.
 */
#define SLEEP_AID_UT_LIST_KEY "SleepUpdateList"

typedef struct
{
    int audio_type;
    int update_time;
} sleep_aid_ut_pair;

#define SLEEP_AID_UT_PAIR_MAX 32

/* 完整读取主配置，失败时回退到同名出厂备份；调用方负责释放返回缓冲区。 */
static char *sleep_aid_read_file_with_backup(const char *path, const char *backup_path, int *file_len)
{
    char *read_buf;

    read_buf = divoom_port_file_read_all(path, file_len);
    if (!read_buf && backup_path)
    {
        read_buf = divoom_port_file_read_all(backup_path, file_len);
    }
    return read_buf;
}

static int sleep_aid_ut_read_json_file(char **out_txt)
{
    *out_txt = NULL;
    *out_txt = sleep_aid_read_file_with_backup(DIVOOM_SLEEP_AID_UPDATE_TIME_PATH,
                                                DIVOOM_SLEEP_AID_UPDATE_TIME_BAK_PATH, NULL);
    return *out_txt ? 0 : -1;
}

/** Parse one row: AUDIO_TYPE / audio_type + UPDATE_TIME / update_time */
static int sleep_aid_ut_obj_get_pair(divoom_cJSON *obj, int *audio_type, int *update_time)
{
    divoom_cJSON *t;
    divoom_cJSON *u;

    if (!obj || !(obj->type & divoom_cJSON_Object))
    {
        return -1;
    }

    t = divoom_cJSON_GetObjectItem(obj, "AUDIO_TYPE");
    if (!t || !(t->type & divoom_cJSON_Number))
    {
        t = divoom_cJSON_GetObjectItem(obj, "audio_type");
    }
    if (!t || !(t->type & divoom_cJSON_Number))
    {
        return -1;
    }

    u = divoom_cJSON_GetObjectItem(obj, "UPDATE_TIME");
    if (!u || !(u->type & divoom_cJSON_Number))
    {
        u = divoom_cJSON_GetObjectItem(obj, "update_time");
    }
    if (!u || !(u->type & divoom_cJSON_Number))
    {
        return -1;
    }

    *audio_type = t->valueint;
    *update_time = u->valueint;
    return 0;
}

static void sleep_aid_load_ut_pairs_from_file(sleep_aid_ut_pair *pairs, int *pair_count, int pair_cap)
{
    char *read_txt = NULL;
    divoom_cJSON *root = NULL;
    divoom_cJSON *arr = NULL;
    divoom_cJSON *el = NULL;
    int i;
    int at;
    int ut;

    if (pair_count)
    {
        *pair_count = 0;
    }

    if (!pairs || !pair_count || pair_cap <= 0)
    {
        return;
    }

    if (sleep_aid_ut_read_json_file(&read_txt) != 0 || !read_txt)
    {
        return;
    }

    root = divoom_cJSON_Parse(read_txt);
    divoom_system_free(read_txt);
    if (!root)
    {
        return;
    }

    arr = divoom_cJSON_GetObjectItem(root, SLEEP_AID_UT_LIST_KEY);
    if (arr && (arr->type & divoom_cJSON_Array))
    {
        for (i = 0; i < divoom_cJSON_GetArraySize(arr); i++)
        {
            el = divoom_cJSON_GetArrayItem(arr, i);
            if (sleep_aid_ut_obj_get_pair(el, &at, &ut) != 0)
            {
                continue;
            }
            if (*pair_count >= pair_cap)
            {
                break;
            }
            pairs[*pair_count].audio_type = at;
            pairs[*pair_count].update_time = ut;
            (*pair_count)++;
        }
    }

    divoom_cJSON_Delete(root);
}

static int sleep_aid_ut_pair_cmp_for_sort(const void *a, const void *b)
{
    const sleep_aid_ut_pair *pa = (const sleep_aid_ut_pair *)a;
    const sleep_aid_ut_pair *pb = (const sleep_aid_ut_pair *)b;

    if (pa->audio_type < pb->audio_type)
    {
        return -1;
    }
    if (pa->audio_type > pb->audio_type)
    {
        return 1;
    }
    return 0;
}

/** Sort by AUDIO_TYPE; merge duplicates keeping max UPDATE_TIME */
static void sleep_aid_ut_pairs_sort_dedupe_max(sleep_aid_ut_pair *pairs, int *pair_count)
{
    int i;
    int w;

    if (!pairs || !pair_count || *pair_count <= 1)
    {
        return;
    }

    qsort(pairs, (unsigned)*pair_count, sizeof(sleep_aid_ut_pair), sleep_aid_ut_pair_cmp_for_sort);

    w = 0;
    for (i = 1; i < *pair_count; i++)
    {
        if (pairs[i].audio_type == pairs[w].audio_type)
        {
            if (pairs[i].update_time > pairs[w].update_time)
            {
                pairs[w].update_time = pairs[i].update_time;
            }
        }
        else
        {
            w++;
            pairs[w] = pairs[i];
        }
    }
    *pair_count = w + 1;
}

static int sleep_aid_save_ut_pairs_to_file(sleep_aid_ut_pair *pairs, int pair_count)
{
    divoom_cJSON *root;
    divoom_cJSON *arr;
    divoom_cJSON *obj;
    char *txt;
    FILE *fp;
    int i;

    if (!pairs && pair_count > 0)
    {
        return -1;
    }

    sleep_aid_ut_pairs_sort_dedupe_max(pairs, &pair_count);

    root = divoom_cJSON_CreateObject();
    arr = divoom_cJSON_CreateArray();
    if (!root || !arr)
    {
        if (root)
        {
            divoom_cJSON_Delete(root);
        }
        return -1;
    }

    for (i = 0; i < pair_count; i++)
    {
        obj = divoom_cJSON_CreateObject();
        if (!obj)
        {
            divoom_cJSON_Delete(root);
            return -1;
        }
        divoom_cJSON_AddNumberToObject(obj, "AUDIO_TYPE", (double)pairs[i].audio_type);
        divoom_cJSON_AddNumberToObject(obj, "UPDATE_TIME", (double)pairs[i].update_time);
        divoom_cJSON_AddItemToArray(arr, obj);
    }

    divoom_cJSON_AddItemToObject(root, SLEEP_AID_UT_LIST_KEY, arr);

    txt = divoom_cJSON_Print(root);
    divoom_cJSON_Delete(root);
    if (!txt)
    {
        return -1;
    }

    fp = divoom_port_file_open(DIVOOM_SLEEP_AID_UPDATE_TIME_PATH, "w+");
    if (!fp)
    {
        divoom_system_free(txt);
        return -1;
    }
    divoom_port_file_write(txt, strlen(txt), 1, fp);
    divoom_port_file_close(fp);
    divoom_system_free(txt);
    return 0;
}

/** Dense [0..n): missing AUDIO_TYPE in file => 0 */
static void sleep_aid_load_max_update_times_from_file(int *out, int n)
{
    sleep_aid_ut_pair pairs[SLEEP_AID_UT_PAIR_MAX];
    int pc = 0;
    int j;

    for (j = 0; j < n; j++)
    {
        out[j] = 0;
    }

    sleep_aid_load_ut_pairs_from_file(pairs, &pc, SLEEP_AID_UT_PAIR_MAX);

    for (j = 0; j < pc; j++)
    {
        if (pairs[j].audio_type >= 0 && pairs[j].audio_type < n)
        {
            out[pairs[j].audio_type] = pairs[j].update_time;
        }
    }
}

/** After saving sleep_aid_%02d.cfg, upsert/remove row in SleepUpdateList for this AUDIO_TYPE. */
static void sleep_aid_update_global_max_update_time(int type, int max_ut)
{
    sleep_aid_ut_pair pairs[SLEEP_AID_UT_PAIR_MAX];
    int pc = 0;
    int i;
    int found_idx;

    if (type < 0)
    {
        return;
    }

    sleep_aid_load_ut_pairs_from_file(pairs, &pc, SLEEP_AID_UT_PAIR_MAX);
    sleep_aid_ut_pairs_sort_dedupe_max(pairs, &pc);

    found_idx = -1;
    for (i = 0; i < pc; i++)
    {
        if (pairs[i].audio_type == type)
        {
            found_idx = i;
            break;
        }
    }

    if (max_ut <= 0)
    {
        if (found_idx >= 0)
        {
            pairs[found_idx] = pairs[pc - 1];
            pc--;
        }
    }
    else
    {
        if (found_idx >= 0)
        {
            pairs[found_idx].update_time = max_ut;
        }
        else if (pc < SLEEP_AID_UT_PAIR_MAX)
        {
            pairs[pc].audio_type = type;
            pairs[pc].update_time = max_ut;
            pc++;
        }
        else
        {
            divoom_print("%s: pair cap %d full, drop type=%d\n", __func__, SLEEP_AID_UT_PAIR_MAX, type);
            return;
        }
    }

    if (sleep_aid_save_ut_pairs_to_file(pairs, pc) != 0)
    {
        divoom_print("%s: write %s failed type=%d max_ut=%d\n", __func__, DIVOOM_SLEEP_AID_UPDATE_TIME_PATH, type, max_ut);
    }
}

static void sleep_aid_build_cfg_path(char *out, int out_sz, int type)
{
    snprintf(out, (unsigned)out_sz, DIVOOM_SLEEP_AID_CONFIG_PATH, type);
}

static void sleep_aid_build_cfg_backup_path(char *out, int out_sz, int type)
{
    snprintf(out, (unsigned)out_sz, DIVOOM_SLEEP_AID_CONFIG_BAK_PATH, type);
}

static void sleep_aid_build_type_lang_path(char *out, int out_sz, int audio_type, const char *locale_key)
{
    snprintf(out, (unsigned)out_sz, DIVOOM_SLEEP_AID_LANGUAGE_PATH, audio_type,
             locale_key ? locale_key : "en");
}

static void sleep_aid_build_type_lang_backup_path(char *out, int out_sz, int audio_type, const char *locale_key)
{
    snprintf(out, (unsigned)out_sz, DIVOOM_SLEEP_AID_LANGUAGE_BAK_PATH, audio_type,
             locale_key ? locale_key : "en");
}

static void sleep_aid_save_language_for_type(int list_type, uint32 audio_id, const char *language_json);
static void sleep_aid_flush_languages_for_rows(int list_type, divoom_sleep_aid_list_row_parsed *rows, int srv_n);
static void sleep_aid_voice_list_fill_names(divoom_sleep_aid_voice_list_item *out, int n, int list_aid_type);
static int sleep_aid_lookup_name_in_type_lang_file(int audio_type, int lang_idx, uint32 audio_id,
                                                   char *out, int out_sz);

static void sleep_aid_copy_voice_name_out(char *out, int out_sz, const char *src);

/** 收藏列表中录音/混合无 sleep_lang 条目：用 Tab 同名文案（录音 / DIY） */
static void sleep_aid_copy_special_voice_display_name(int audio_type, char *out, int out_sz)
{
    const char *key = NULL;
    const char *label;

    if (!out || out_sz <= 0)
    {
        return;
    }
    out[0] = '\0';

    if (audio_type == DIVOOM_SLEEP_AID_TYPE_RECORD)
    {
        key = "SleepAidTabRecord";
    }
    else if (audio_type == DIVOOM_SLEEP_AID_TYPE_DIY)
    {
        key = "SleepAidTabDiy";
    }
    else
    {
        return;
    }

    label = divoom_menu_control_get_disp_str(key);
    if (label && label[0])
    {
        sleep_aid_copy_voice_name_out(out, out_sz, label);
    }
}

static void sleep_aid_try_download(uint32 voice_id, const char *file_id)
{
    divoom_manage_image_item_info image_info;

    if (!file_id || !file_id[0])
        return;
    memset(&image_info, 0, sizeof(image_info));
    image_info.flag = 1;
    image_info.image_id = voice_id;
    strncpy(image_info.file_addr, file_id, DIVOOM_MANAGE_FILE_ADDR_LEN - 1);
    image_info.file_addr[DIVOOM_MANAGE_FILE_ADDR_LEN - 1] = '\0';
    divoom_manage_image_add_font(DIVOOM_MANAGE_IMAGE_SLEEP_AID_PIC, &image_info, 1);
}

static void sleep_aid_del_cached(uint32 voice_id)
{
    uint32 id = voice_id;

    divoom_manage_image_del(DIVOOM_MANAGE_IMAGE_SLEEP_AID_PIC, &id, 1, 0);
}

static int sleep_aid_voice_preset_file_exists(uint32 voice_id)
{
    char path[256];

    if (voice_id == 0)
    {
        return 0;
    }
    divoom_manage_image_download_get_file_name(DIVOOM_MANAGE_IMAGE_SLEEP_AID_PIC, voice_id, path);
    return divoom_port_file_exists(path) ? 1 : 0;
}

static int sleep_aid_merge_app_add_row(int list_type, Pdivoom_sleep_aid_add_req req)
{
    divoom_sleep_aid_vec vec;
    divoom_sleep_aid_config_item incoming;
    divoom_sleep_aid_list_row_parsed lang_row;
    int idx;
    int file_exists;

    if (!req || req->sleep_id == 0)
    {
        return -1;
    }
    if (list_type < 0 || list_type >= DIVOOM_SLEEP_AID_STORED_TYPE_NUM)
    {
        return -1;
    }

    memset(&vec, 0, sizeof(vec));
    if (sleep_aid_load_vec_from_file(list_type, &vec) != 0)
    {
        return -1;
    }

    memset(&incoming, 0, sizeof(incoming));
    incoming.voice_id = req->sleep_id;
    incoming.add_flag = 1;
    incoming.video_type = req->video_type;
    strncpy(incoming.file_id, req->file_id, sizeof(incoming.file_id) - 1);
    incoming.file_id[sizeof(incoming.file_id) - 1] = '\0';

    idx = sleep_aid_vec_find(&vec, incoming.voice_id);
    if (idx < 0)
    {
        if (sleep_aid_vec_append(&vec, &incoming) != 0)
        {
            sleep_aid_vec_free(&vec);
            return -1;
        }
    }
    else
    {
        if (incoming.file_id[0] && strcmp(vec.buf[idx].file_id, incoming.file_id) != 0)
        {
            sleep_aid_del_cached(incoming.voice_id);
            strncpy(vec.buf[idx].file_id, incoming.file_id, sizeof(vec.buf[idx].file_id) - 1);
            vec.buf[idx].file_id[sizeof(vec.buf[idx].file_id) - 1] = '\0';
        }
        vec.buf[idx].add_flag = 1;
        vec.buf[idx].video_type = incoming.video_type;
    }

    if (req->language_json && req->language_json[0])
    {
        memset(&lang_row, 0, sizeof(lang_row));
        lang_row.voice_id = req->sleep_id;
        lang_row.language_json = req->language_json;
        sleep_aid_flush_languages_for_rows(list_type, &lang_row, 1);
    }

    if (sleep_aid_save_vec_to_file(list_type, &vec) != 0)
    {
        sleep_aid_vec_free(&vec);
        return -1;
    }
    sleep_aid_vec_free(&vec);

    {
        Pdivoom_sleep_aid_ctrl c = divoom_sleep_aid_ctrl_instance();

        if (c)
        {
            c->voice_list_cfg_revision++;
        }
    }

    file_exists = sleep_aid_voice_preset_file_exists(req->sleep_id);
    if (file_exists)
    {
        divoom_print("%s: preset exists id=%u type=%d, play now\n", __func__,
                     (unsigned)req->sleep_id, list_type);
        divoom_sleep_aid_ctrl_app_preview_begin();
        divoom_sleep_aid_ctrl_select_voice(list_type, (int)req->sleep_id);
    }
    else if (incoming.file_id[0])
    {
        divoom_print("%s: no preset id=%u, download file_id=%s\n", __func__,
                     (unsigned)req->sleep_id, incoming.file_id);
        sleep_aid_try_download(incoming.voice_id, incoming.file_id);
    }
    return file_exists ? 1 : 0;
}


static int sleep_aid_load_vec_from_file(int type, divoom_sleep_aid_vec *out_vec)
{
    divoom_cJSON *root = NULL;
    divoom_cJSON *list = NULL;
    divoom_cJSON *item = NULL;
    uint8 *read_buf = NULL;
    char path[160];
    char backup_path[160];
    int i;
    int n;

    memset(out_vec, 0, sizeof(*out_vec));
    sleep_aid_build_cfg_path(path, (int)sizeof(path), type);
    sleep_aid_build_cfg_backup_path(backup_path, (int)sizeof(backup_path), type);
    read_buf = (uint8 *)sleep_aid_read_file_with_backup(path, backup_path, NULL);
    if (!read_buf)
    {
        return 0;
    }

    root = divoom_cJSON_Parse((char *)read_buf);
    divoom_system_free(read_buf);
    if (!root)
        return -1;

    list = divoom_cJSON_GetObjectItem(root, "SleepList");
    if (!list || !(list->type & divoom_cJSON_Array))
    {
        divoom_cJSON_Delete(root);
        return 0;
    }

    n = divoom_cJSON_GetArraySize(list);
    for (i = 0; i < n; i++)
    {
        divoom_sleep_aid_config_item one;

        item = divoom_cJSON_GetArrayItem(list, i);
        if (!item)
            continue;
        memset(&one, 0, sizeof(one));
        one.voice_id = (uint32)divoom_cJSON_GetObjectInt(item, "voice_id");
        if (one.voice_id == 0)
            one.voice_id = (uint32)divoom_cJSON_GetObjectInt(item, "SleepId");
        one.add_flag = (uint8)divoom_cJSON_GetObjectInt(item, "add_flag");
        if (divoom_cJSON_GetObjectItem(item, "AddFlag"))
            one.add_flag = (uint8)divoom_cJSON_GetObjectInt(item, "AddFlag");
        one.video_type = divoom_cJSON_GetObjectInt(item, "video_type");
        if (divoom_cJSON_GetObjectItem(item, "VideoType"))
            one.video_type = divoom_cJSON_GetObjectInt(item, "VideoType");
        one.update_time = divoom_cJSON_GetObjectInt(item, "update_time");
        if (divoom_cJSON_GetObjectItem(item, "UpdateTime"))
            one.update_time = divoom_cJSON_GetObjectInt(item, "UpdateTime");
        divoom_cJSON_GetObjectString(item, "file_id", one.file_id);
        if (!one.file_id[0])
            divoom_cJSON_GetObjectString(item, "FileId", one.file_id);

        if (one.voice_id == 0)
            continue;
        if (sleep_aid_vec_append(out_vec, &one) != 0)
        {
            sleep_aid_vec_free(out_vec);
            divoom_cJSON_Delete(root);
            return -1;
        }
    }

    divoom_cJSON_Delete(root);
    return 0;
}

static int sleep_aid_save_vec_to_file(int type, const divoom_sleep_aid_vec *v)
{
    divoom_cJSON *root;
    divoom_cJSON *arr;
    FILE *fp;
    char path[160];
    char *txt;
    int i;

    sleep_aid_build_cfg_path(path, (int)sizeof(path), type);
    root = divoom_cJSON_CreateObject();
    if (!root)
        return -1;
    arr = divoom_cJSON_CreateArray();
    if (!arr)
    {
        divoom_cJSON_Delete(root);
        return -1;
    }
    divoom_cJSON_AddItemToObject(root, "SleepList", arr);

    for (i = 0; i < v->n; i++)
    {
        divoom_cJSON *obj = divoom_cJSON_CreateObject();
        if (!obj)
        {
            divoom_cJSON_Delete(root);
            return -1;
        }
        divoom_cJSON_AddNumberToObject(obj, "voice_id", (double)v->buf[i].voice_id);
        divoom_cJSON_AddNumberToObject(obj, "add_flag", v->buf[i].add_flag);
        divoom_cJSON_AddNumberToObject(obj, "video_type", v->buf[i].video_type);
        divoom_cJSON_AddNumberToObject(obj, "update_time", v->buf[i].update_time);
        divoom_cJSON_AddStringToObject(obj, "file_id", v->buf[i].file_id);
        divoom_cJSON_AddItemToArray(arr, obj);
    }

    divoom_cJSON_AddNumberToObject(root, "max_update_time", sleep_aid_vec_max_update_time(v));

    txt = divoom_cJSON_Print(root);
    divoom_cJSON_Delete(root);
    if (!txt)
        return -1;

    fp = divoom_port_file_open(path, "w+");
    if (!fp)
    {
        divoom_system_free(txt);
        return -1;
    }
    divoom_port_file_write(txt, strlen(txt), 1, fp);
    divoom_port_file_close(fp);
    divoom_system_free(txt);

    sleep_aid_update_global_max_update_time(type, sleep_aid_vec_max_update_time(v));

    return 0;
}

static int sleep_aid_merge_and_store_rows(int list_type, divoom_sleep_aid_list_row_parsed *rows, int srv_n)
{
    divoom_sleep_aid_vec vec;
    int ni;

    memset(&vec, 0, sizeof(vec));
    if (sleep_aid_load_vec_from_file(list_type, &vec) != 0)
        return -1;

    if (!rows)
        srv_n = 0;

    for (ni = 0; ni < srv_n; ni++)
    {
        divoom_sleep_aid_list_row_parsed *sr = &rows[ni];
        divoom_sleep_aid_config_item incoming;
        int idx;

        memset(&incoming, 0, sizeof(incoming));
        incoming.voice_id = sr->voice_id;
        incoming.add_flag = sr->add_flag;
        incoming.video_type = sr->video_type;
        incoming.update_time = sr->update_time;
        strncpy(incoming.file_id, sr->file_id, sizeof(incoming.file_id) - 1);
        incoming.file_id[sizeof(incoming.file_id) - 1] = '\0';

        if (incoming.voice_id == 0)
            continue;

        idx = sleep_aid_vec_find(&vec, incoming.voice_id);
        if (idx < 0)
        {
            if (sleep_aid_vec_append(&vec, &incoming) != 0)
            {
                sleep_aid_vec_free(&vec);
                return -1;
            }
            if (incoming.add_flag == 1)
                sleep_aid_try_download(incoming.voice_id, incoming.file_id);
        }
        else
        {
            if (strcmp(vec.buf[idx].file_id, incoming.file_id) != 0)
            {
                sleep_aid_del_cached(incoming.voice_id);
                strncpy(vec.buf[idx].file_id, incoming.file_id, sizeof(vec.buf[idx].file_id) - 1);
                vec.buf[idx].file_id[sizeof(vec.buf[idx].file_id) - 1] = '\0';
            }
            vec.buf[idx].add_flag = incoming.add_flag;
            vec.buf[idx].video_type = incoming.video_type;
            vec.buf[idx].update_time = incoming.update_time;

            if (incoming.add_flag == 1)
                sleep_aid_try_download(incoming.voice_id, vec.buf[idx].file_id);
        }
    }

    sleep_aid_flush_languages_for_rows(list_type, rows, srv_n);

    if (sleep_aid_save_vec_to_file(list_type, &vec) != 0)
    {
        sleep_aid_vec_free(&vec);
        return -1;
    }
    sleep_aid_vec_free(&vec);
    return 0;
}

void divoom_sleep_aid_handle_get_all_list_resp(void *para)
{
    Pdivoom_sleep_aid_get_all_list_resp resp = (Pdivoom_sleep_aid_get_all_list_resp)para;

    if (!resp)
        return;

    if (sleep_aid_merge_and_store_rows(resp->list_type, resp->rows, resp->row_num) != 0)
    {
        divoom_print("%s: merge sleep aid list failed type=%d rows=%d\n", __func__, resp->list_type, resp->row_num);
    }
    else
    {
        Pdivoom_sleep_aid_ctrl c = divoom_sleep_aid_ctrl_instance();

        if (c)
        {
            c->voice_list_cfg_revision++;
        }
    }
    /* resp / rows / language_json released by divoom_sleep_aid_get_all_list_response_unpack_free_handle */
}

void divoom_sleep_aid_handle_play_req(void *para)
{
    Pdivoom_sleep_aid_play_req req = (Pdivoom_sleep_aid_play_req)para;

    if (!req)
    {
        return;
    }
    divoom_print("%s: app play type=%d sleep_id=%u\n", __func__, req->type, (unsigned)req->sleep_id);
    divoom_sleep_aid_ctrl_app_preview_begin();
    divoom_sleep_aid_ctrl_select_voice(req->type, (int)req->sleep_id);
}

void divoom_sleep_aid_handle_add_req(void *para)
{
    Pdivoom_sleep_aid_add_req req = (Pdivoom_sleep_aid_add_req)para;

    if (!req)
    {
        return;
    }
    divoom_print("%s: app add type=%d sleep_id=%u video_type=%d\n", __func__,
                 req->type, (unsigned)req->sleep_id, req->video_type);
    sleep_aid_merge_app_add_row(req->type, req);
}

void divoom_sleep_aid_handle_exit_play(void)
{
    divoom_sleep_aid_ctrl_app_preview_stop();
}

/* ---------- sleep aid favorites (DIVOOM_SLEEP_AID_FAV_PATH JSON) ---------- */

typedef struct
{
    divoom_sleep_aid_fav_item *buf;
    int n;
    int cap;
} sleep_aid_fav_vec;

static void sleep_aid_fav_vec_free(sleep_aid_fav_vec *v);
static int sleep_aid_fav_load(sleep_aid_fav_vec *out_vec);

/** 进程内收藏缓存：避免 save 后立刻 load 失败导致 UI 不刷新、下次 add 覆盖整条列表 */
static sleep_aid_fav_vec s_fav_mem;
static int s_fav_mem_ready;

static void sleep_aid_fav_mem_reset(void)
{
    sleep_aid_fav_vec_free(&s_fav_mem);
    memset(&s_fav_mem, 0, sizeof(s_fav_mem));
    s_fav_mem_ready = 0;
}

static int sleep_aid_fav_mem_ensure(void)
{
    if (s_fav_mem_ready)
    {
        return 0;
    }
    memset(&s_fav_mem, 0, sizeof(s_fav_mem));
    if (sleep_aid_fav_load(&s_fav_mem) != 0)
    {
        sleep_aid_fav_vec_free(&s_fav_mem);
        memset(&s_fav_mem, 0, sizeof(s_fav_mem));
        return -1;
    }
    s_fav_mem_ready = 1;
    return 0;
}

static uint32 sleep_aid_fav_json_uint(const divoom_cJSON *item, const char *key)
{
    divoom_cJSON *node;

    if (!item || !key)
    {
        return 0;
    }
    node = divoom_cJSON_GetObjectItem(item, key);
    if (!node || !(node->type & divoom_cJSON_Number))
    {
        return 0;
    }
    return (uint32)node->valuedouble;
}

static int sleep_aid_fav_vec_reserve(sleep_aid_fav_vec *v, int need)
{
    divoom_sleep_aid_fav_item *nb;
    int new_cap;

    if (need <= v->cap)
        return 0;
    new_cap = v->cap ? v->cap : 8;
    while (new_cap < need)
        new_cap *= 2;
    nb = (divoom_sleep_aid_fav_item *)divoom_system_malloc((unsigned)new_cap * sizeof(divoom_sleep_aid_fav_item));
    if (!nb)
        return -1;
    if (v->n > 0 && v->buf)
        memcpy(nb, v->buf, (unsigned)v->n * sizeof(divoom_sleep_aid_fav_item));
    if (v->buf)
        divoom_system_free(v->buf);
    v->buf = nb;
    v->cap = new_cap;
    return 0;
}

static void sleep_aid_fav_vec_free(sleep_aid_fav_vec *v)
{
    if (v->buf)
        divoom_system_free(v->buf);
    v->buf = NULL;
    v->n = 0;
    v->cap = 0;
}

static int sleep_aid_fav_vec_find(const sleep_aid_fav_vec *v, uint32 audio_id, int audio_type)
{
    int i;

    for (i = 0; i < v->n; i++)
    {
        if (v->buf[i].audio_id == audio_id && v->buf[i].audio_type == audio_type)
        {
            return i;
        }
    }
    return -1;
}

static int sleep_aid_fav_vec_append(sleep_aid_fav_vec *v, const divoom_sleep_aid_fav_item *it)
{
    if (sleep_aid_fav_vec_reserve(v, v->n + 1) != 0)
    {
        return -1;
    }
    v->buf[v->n] = *it;
    v->n++;
    return 0;
}

/** 插入到列表头部：最新收藏在前（时间倒序展示）。 */
static int sleep_aid_fav_vec_insert_front(sleep_aid_fav_vec *v, const divoom_sleep_aid_fav_item *it)
{
    if (!it)
    {
        return -1;
    }
    if (sleep_aid_fav_vec_reserve(v, v->n + 1) != 0)
    {
        return -1;
    }
    if (v->n > 0)
    {
        memmove(&v->buf[1], &v->buf[0], (unsigned)v->n * sizeof(divoom_sleep_aid_fav_item));
    }
    v->buf[0] = *it;
    v->n++;
    return 0;
}

static int sleep_aid_fav_vec_remove_at(sleep_aid_fav_vec *v, int idx)
{
    if (idx < 0 || idx >= v->n)
    {
        return -1;
    }
    /* 删除后保持原有顺序，后续项前移（禁止与末项交换，否则 UI 只刷新首行时会显示「末项顶上」）。 */
    if (idx < v->n - 1)
    {
        memmove(&v->buf[idx], &v->buf[idx + 1],
                (unsigned)(v->n - idx - 1) * sizeof(divoom_sleep_aid_fav_item));
    }
    v->n--;
    return 0;
}

static int sleep_aid_fav_load(sleep_aid_fav_vec *out_vec)
{
    divoom_cJSON *root = NULL;
    divoom_cJSON *list = NULL;
    divoom_cJSON *item = NULL;
    uint8 *read_buf = NULL;
    int i;
    int n;

    memset(out_vec, 0, sizeof(*out_vec));
    read_buf = (uint8 *)sleep_aid_read_file_with_backup(DIVOOM_SLEEP_AID_FAV_PATH,
                                                        DIVOOM_SLEEP_AID_FAV_BAK_PATH, NULL);
    if (!read_buf)
    {
        return 0;
    }

    root = divoom_cJSON_Parse((char *)read_buf);
    divoom_system_free(read_buf);
    if (!root)
        return -1;

    list = divoom_cJSON_GetObjectItem(root, "FavList");
    if (!list || !(list->type & divoom_cJSON_Array))
    {
        divoom_cJSON_Delete(root);
        return 0;
    }

    n = divoom_cJSON_GetArraySize(list);
    for (i = 0; i < n; i++)
    {
        divoom_sleep_aid_fav_item one;

        item = divoom_cJSON_GetArrayItem(list, i);
        if (!item)
            continue;
        memset(&one, 0, sizeof(one));
        one.audio_id = sleep_aid_fav_json_uint(item, "audio_id");
        if (one.audio_id == 0)
        {
            one.audio_id = sleep_aid_fav_json_uint(item, "voice_id");
        }
        if (one.audio_id == 0)
        {
            one.audio_id = sleep_aid_fav_json_uint(item, "VoiceId");
        }
        if (divoom_cJSON_GetObjectItem(item, "audio_type"))
        {
            one.audio_type = divoom_cJSON_GetObjectInt(item, "audio_type");
        }
        else if (divoom_cJSON_GetObjectItem(item, "VideoType"))
        {
            one.audio_type = divoom_cJSON_GetObjectInt(item, "VideoType");
        }
        one.video_type = divoom_cJSON_GetObjectInt(item, "video_type");
        if (one.video_type == 0 && !divoom_cJSON_GetObjectItem(item, "video_type"))
        {
            one.video_type = -1;
        }

        if (one.audio_id == 0)
        {
            continue;
        }
        if (sleep_aid_fav_vec_append(out_vec, &one) != 0)
        {
            sleep_aid_fav_vec_free(out_vec);
            divoom_cJSON_Delete(root);
            return -1;
        }
    }

    divoom_cJSON_Delete(root);
    return 0;
}

static int sleep_aid_fav_save(const sleep_aid_fav_vec *v)
{
    divoom_cJSON *root;
    divoom_cJSON *arr;
    FILE *fp;
    char *txt;
    int i;

    root = divoom_cJSON_CreateObject();
    if (!root)
        return -1;
    arr = divoom_cJSON_CreateArray();
    if (!arr)
    {
        divoom_cJSON_Delete(root);
        return -1;
    }
    divoom_cJSON_AddItemToObject(root, "FavList", arr);

    for (i = 0; i < v->n; i++)
    {
        divoom_cJSON *obj = divoom_cJSON_CreateObject();
        if (!obj)
        {
            divoom_cJSON_Delete(root);
            return -1;
        }
        divoom_cJSON_AddNumberToObject(obj, "audio_id", (double)v->buf[i].audio_id);
        divoom_cJSON_AddNumberToObject(obj, "audio_type", v->buf[i].audio_type);
        if (v->buf[i].video_type >= 0)
        {
            divoom_cJSON_AddNumberToObject(obj, "video_type", v->buf[i].video_type);
        }
        divoom_cJSON_AddItemToArray(arr, obj);
    }

    txt = divoom_cJSON_Print(root);
    divoom_cJSON_Delete(root);
    if (!txt)
        return -1;

    fp = divoom_port_file_open(DIVOOM_SLEEP_AID_FAV_PATH, "wb");
    if (!fp)
    {
        divoom_system_free(txt);
        return -1;
    }
    if (divoom_port_file_write(txt, 1, strlen(txt), fp) != strlen(txt))
    {
        divoom_port_file_close(fp);
        divoom_system_free(txt);
        return -1;
    }
    divoom_port_file_close(fp);
    divoom_system_free(txt);
    return 0;
}

divoom_sleep_aid_fav_item *divoom_sleep_aid_fav_get_list(int *items)
{
    divoom_sleep_aid_fav_item *out;

    if (items)
        *items = 0;

    if (sleep_aid_fav_mem_ensure() != 0 || s_fav_mem.n <= 0)
    {
        return NULL;
    }

    out = (divoom_sleep_aid_fav_item *)divoom_system_malloc((unsigned)s_fav_mem.n * sizeof(divoom_sleep_aid_fav_item));
    if (!out)
    {
        return NULL;
    }

    memcpy(out, s_fav_mem.buf, (unsigned)s_fav_mem.n * sizeof(divoom_sleep_aid_fav_item));
    if (items)
        *items = s_fav_mem.n;

    return out;
}

int divoom_sleep_aid_fav_add(uint32 audio_id, int audio_type, int video_type)
{
    int idx;
    divoom_sleep_aid_fav_item it;

    if (audio_id == 0)
    {
        return -1;
    }

    if (!s_fav_mem_ready && sleep_aid_fav_load(&s_fav_mem) != 0)
    {
        return -1;
    }
    s_fav_mem_ready = 1;

    idx = sleep_aid_fav_vec_find(&s_fav_mem, audio_id, audio_type);
    if (idx >= 0)
    {
        it = s_fav_mem.buf[idx];
        if (video_type >= 0)
        {
            it.video_type = video_type;
        }
        if (sleep_aid_fav_vec_remove_at(&s_fav_mem, idx) != 0)
        {
            return -1;
        }
        if (sleep_aid_fav_vec_insert_front(&s_fav_mem, &it) != 0)
        {
            return -1;
        }
    }
    else
    {
        memset(&it, 0, sizeof(it));
        it.audio_id = audio_id;
        it.audio_type = audio_type;
        it.video_type = video_type;
        if (sleep_aid_fav_vec_insert_front(&s_fav_mem, &it) != 0)
        {
            return -1;
        }
    }

    if (sleep_aid_fav_save(&s_fav_mem) != 0)
    {
        return -1;
    }
    return 0;
}

int divoom_sleep_aid_fav_remove(uint32 audio_id, int audio_type)
{
    int idx;

    if (audio_id == 0)
    {
        return -1;
    }

    if (sleep_aid_fav_mem_ensure() != 0)
    {
        return -1;
    }

    idx = sleep_aid_fav_vec_find(&s_fav_mem, audio_id, audio_type);
    if (idx >= 0)
    {
        if (sleep_aid_fav_vec_remove_at(&s_fav_mem, idx) != 0)
        {
            return -1;
        }
        if (sleep_aid_fav_save(&s_fav_mem) != 0)
        {
            return -1;
        }
    }

    return 0;
}

static int sleep_aid_cfg_item_passes_add_flag(const divoom_sleep_aid_config_item *it, int add_flag)
{
    if (add_flag >= 0 && it->add_flag != (uint8)add_flag)
    {
        return 0;
    }
    return 1;
}
static int sleep_aid_cfg_video_type_for_voice(int audio_type, uint32 audio_id)
{
    divoom_sleep_aid_vec vec;
    int idx;
    int vt;

    if (audio_id == 0)
    {
        return -1;
    }
    if (audio_type != DIVOOM_SLEEP_AID_TYPE_NOISE
        && audio_type != DIVOOM_SLEEP_AID_TYPE_WHITE
        && audio_type != DIVOOM_SLEEP_AID_TYPE_MUSIC)
    {
        return -1;
    }

    memset(&vec, 0, sizeof(vec));
    if (sleep_aid_load_vec_from_file(audio_type, &vec) != 0)
    {
        return -1;
    }
    idx = sleep_aid_vec_find(&vec, audio_id);
    if (idx < 0)
    {
        sleep_aid_vec_free(&vec);
        return -1;
    }
    vt = vec.buf[idx].video_type;
    sleep_aid_vec_free(&vec);
    return vt;
}

int divoom_sleep_aid_voice_get_video_type(int aid_type, uint32 audio_id)
{
    if (audio_id == 0)
    {
        return -1;
    }

    if (aid_type == DIVOOM_SLEEP_AID_TYPE_FAV)
    {
        divoom_sleep_aid_fav_item *a;
        int n;
        int i;

        a = divoom_sleep_aid_fav_get_list(&n);
        if (!a || n <= 0)
        {
            if (a)
            {
                divoom_system_free(a);
            }
            return -1;
        }
        for (i = 0; i < n; i++)
        {
            if (a[i].audio_id == audio_id)
            {
                int vt;

                vt = a[i].video_type;
                divoom_system_free(a);
                if (vt >= 0)
                {
                    return vt;
                }
                return sleep_aid_cfg_video_type_for_voice(a[i].audio_type, audio_id);
            }
        }
        divoom_system_free(a);
        return -1;
    }

    if (aid_type == DIVOOM_SLEEP_AID_TYPE_RECORD)
    {
        return DIVOOM_SLEEP_AID_TYPE_RECORD;
    }

    if (aid_type == DIVOOM_SLEEP_AID_TYPE_DIY)
    {
        return DIVOOM_SLEEP_AID_TYPE_DIY;
    }

    return sleep_aid_cfg_video_type_for_voice(aid_type, audio_id);
}

divoom_sleep_aid_voice_list_item *divoom_sleep_aid_voice_get_list(int aid_type, int *items, int add_flag)
{
    if (items)
    {
        *items = 0;
    }

    if (aid_type == DIVOOM_SLEEP_AID_TYPE_FAV)
    {
        divoom_sleep_aid_fav_item *a;
        int n;
        divoom_sleep_aid_voice_list_item *out;
        int i;

        a = divoom_sleep_aid_fav_get_list(&n);
        if (!a || n <= 0)
        {
            if (a)
            {
                divoom_system_free(a);
            }
            return NULL;
        }

        out = (divoom_sleep_aid_voice_list_item *)divoom_system_malloc((unsigned)n * sizeof(divoom_sleep_aid_voice_list_item));
        if (!out)
        {
            divoom_system_free(a);
            return NULL;
        }
        for (i = 0; i < n; i++)
        {
            out[i].audio_id = a[i].audio_id;
            out[i].audio_type = a[i].audio_type;
            out[i].video_type = a[i].video_type;
            out[i].name[0] = '\0';
            if (out[i].video_type < 0)
            {
                out[i].video_type = sleep_aid_cfg_video_type_for_voice(out[i].audio_type, out[i].audio_id);
            }
        }
        divoom_system_free(a);
        sleep_aid_voice_list_fill_names(out, n, DIVOOM_SLEEP_AID_TYPE_FAV);
        if (items)
        {
            *items = n;
        }
        return out;
    }

    if (aid_type == DIVOOM_SLEEP_AID_TYPE_RECORD)
    {
        int cnt = divoom_sleep_aid_ctrl_record_clip_count();
        divoom_sleep_aid_voice_list_item *buf;
        int i;

        if (cnt <= 0)
        {
            return NULL;
        }

        buf = (divoom_sleep_aid_voice_list_item *)divoom_system_malloc((unsigned)cnt * sizeof(divoom_sleep_aid_voice_list_item));
        if (!buf)
        {
            return NULL;
        }

        for (i = 0; i < cnt; i++)
        {
            int vid = divoom_sleep_aid_ctrl_record_clip_id(i);

            if (vid < 0)
            {
                divoom_system_free(buf);
                return NULL;
            }
            buf[i].audio_id = (uint32)vid;
            buf[i].audio_type = DIVOOM_SLEEP_AID_TYPE_RECORD;
            buf[i].video_type = DIVOOM_SLEEP_AID_TYPE_RECORD;
            buf[i].name[0] = '\0';
        }

        if (items)
        {
            *items = cnt;
        }
        return buf;
    }

    if (aid_type == DIVOOM_SLEEP_AID_TYPE_DIY)
    {
        divoom_sleep_aid_voice_list_item *one;

        one = (divoom_sleep_aid_voice_list_item *)divoom_system_malloc(sizeof(divoom_sleep_aid_voice_list_item));
        if (!one)
        {
            return NULL;
        }
        one[0].audio_id = 1;
        one[0].audio_type = DIVOOM_SLEEP_AID_TYPE_DIY;
        one[0].video_type = DIVOOM_SLEEP_AID_TYPE_DIY;
        one[0].name[0] = '\0';
        if (items)
        {
            *items = 1;
        }
        return one;
    }

    if (aid_type != DIVOOM_SLEEP_AID_TYPE_NOISE && aid_type != DIVOOM_SLEEP_AID_TYPE_WHITE && aid_type != DIVOOM_SLEEP_AID_TYPE_MUSIC)
    {
        return NULL;
    }

    {
        divoom_sleep_aid_vec vec;
        divoom_sleep_aid_voice_list_item *out;
        int i;
        int n;
        int j;

        memset(&vec, 0, sizeof(vec));
        if (sleep_aid_load_vec_from_file(aid_type, &vec) != 0)
        {
            return NULL;
        }

        n = 0;
        for (i = 0; i < vec.n; i++)
        {
            if (vec.buf[i].voice_id == 0)
            {
                continue;
            }
            if (!sleep_aid_cfg_item_passes_add_flag(&vec.buf[i], add_flag))
            {
                continue;
            }
            n++;
        }

        if (n <= 0)
        {
            sleep_aid_vec_free(&vec);
            return NULL;
        }

        out = (divoom_sleep_aid_voice_list_item *)divoom_system_malloc((unsigned)n * sizeof(divoom_sleep_aid_voice_list_item));
        if (!out)
        {
            sleep_aid_vec_free(&vec);
            return NULL;
        }

        j = 0;
        for (i = 0; i < vec.n; i++)
        {
            if (vec.buf[i].voice_id == 0)
            {
                continue;
            }
            if (!sleep_aid_cfg_item_passes_add_flag(&vec.buf[i], add_flag))
            {
                continue;
            }
            out[j].audio_id = vec.buf[i].voice_id;
            out[j].audio_type = aid_type;
            out[j].video_type = vec.buf[i].video_type;
            out[j].name[0] = '\0';
            j++;
        }

        sleep_aid_vec_free(&vec);
        sleep_aid_voice_list_fill_names(out, n, aid_type);

        if (items)
        {
            *items = n;
        }
        return out;
    }
}

divoom_sleep_aid_voice_list_item *divoom_sleep_aid_voice_get_list_page(int aid_type, int offset, int count,
    int add_flag, int *out_n, int *total)
{
    divoom_sleep_aid_vec vec;
    divoom_sleep_aid_voice_list_item *out;
    int matched;
    int i;
    int j;
    int got;

    if (out_n)
    {
        *out_n = 0;
    }
    if (total)
    {
        *total = 0;
    }

    if (aid_type != DIVOOM_SLEEP_AID_TYPE_NOISE && aid_type != DIVOOM_SLEEP_AID_TYPE_WHITE
        && aid_type != DIVOOM_SLEEP_AID_TYPE_MUSIC)
    {
        return NULL;
    }
    if (count <= 0)
    {
        return NULL;
    }
    if (offset < 0)
    {
        offset = 0;
    }

    memset(&vec, 0, sizeof(vec));
    if (sleep_aid_load_vec_from_file(aid_type, &vec) != 0)
    {
        return NULL;
    }

    /* 过滤后总条数 */
    matched = 0;
    for (i = 0; i < vec.n; i++)
    {
        if (vec.buf[i].voice_id == 0)
        {
            continue;
        }
        if (!sleep_aid_cfg_item_passes_add_flag(&vec.buf[i], add_flag))
        {
            continue;
        }
        matched++;
    }
    if (total)
    {
        *total = matched;
    }

    if (matched <= 0 || offset >= matched)
    {
        sleep_aid_vec_free(&vec);
        return NULL;
    }

    out = (divoom_sleep_aid_voice_list_item *)divoom_system_malloc((unsigned)count * sizeof(divoom_sleep_aid_voice_list_item));
    if (!out)
    {
        sleep_aid_vec_free(&vec);
        return NULL;
    }

    /* 仅收集 [offset, offset+count) 区间 */
    j = 0;
    got = 0;
    for (i = 0; i < vec.n && got < count; i++)
    {
        if (vec.buf[i].voice_id == 0)
        {
            continue;
        }
        if (!sleep_aid_cfg_item_passes_add_flag(&vec.buf[i], add_flag))
        {
            continue;
        }
        if (j < offset)
        {
            j++;
            continue;
        }
        out[got].audio_id = vec.buf[i].voice_id;
        out[got].audio_type = aid_type;
        out[got].video_type = vec.buf[i].video_type;
        out[got].name[0] = '\0';
        got++;
        j++;
    }

    sleep_aid_vec_free(&vec);

    if (got <= 0)
    {
        divoom_system_free(out);
        return NULL;
    }

    /* 仅为本批次填充显示名（只加载一次语言缓存） */
    sleep_aid_voice_list_fill_names(out, got, aid_type);

    if (out_n)
    {
        *out_n = got;
    }
    return out;
}

void divoom_sleep_aid_voice_change_add_flag(int video_type, uint32 voice_id, int add_flag)
{
    divoom_sleep_aid_vec vec;
    int idx;

    if (voice_id == 0)
    {
        return;
    }
    if (video_type < 0 || video_type >= DIVOOM_SLEEP_AID_STORED_TYPE_NUM)
    {
        return;
    }

    memset(&vec, 0, sizeof(vec));
    if (sleep_aid_load_vec_from_file(video_type, &vec) != 0)
    {
        return;
    }

    idx = sleep_aid_vec_find(&vec, voice_id);
    if (idx < 0)
    {
        sleep_aid_vec_free(&vec);
        return;
    }

    vec.buf[idx].add_flag = (uint8)add_flag;
    sleep_aid_save_vec_to_file(video_type, &vec);
    sleep_aid_vec_free(&vec);
}

int divoom_sleep_aid_voice_pick_pending_add(int aid_type, uint32 *voice_ids, char file_ids[][56], int max_pick)
{
    divoom_sleep_aid_vec vec;
    int i;
    int picked;

    if (!voice_ids || !file_ids || max_pick <= 0)
    {
        return 0;
    }
    if (aid_type != DIVOOM_SLEEP_AID_TYPE_NOISE && aid_type != DIVOOM_SLEEP_AID_TYPE_WHITE
        && aid_type != DIVOOM_SLEEP_AID_TYPE_MUSIC)
    {
        return 0;
    }

    memset(&vec, 0, sizeof(vec));
    if (sleep_aid_load_vec_from_file(aid_type, &vec) != 0)
    {
        divoom_print("sleep_aid_add: pick_pending type=%d load cfg failed\n", aid_type);
        return 0;
    }

    {
        int skip_added = 0;
        int skip_no_file = 0;

        picked = 0;
        for (i = 0; i < vec.n && picked < max_pick; i++)
        {
            if (vec.buf[i].voice_id == 0 || vec.buf[i].add_flag != 0)
            {
                skip_added++;
                continue;
            }
            if (!vec.buf[i].file_id[0])
            {
                skip_no_file++;
                continue;
            }

            voice_ids[picked] = vec.buf[i].voice_id;
            strncpy(file_ids[picked], vec.buf[i].file_id, 55);
            file_ids[picked][55] = '\0';
            vec.buf[i].add_flag = 1;
            picked++;
        }

        divoom_print("sleep_aid_add: pick_pending type=%d cfg_rows=%d picked=%d skip_added=%d skip_no_file=%d\n",
                     aid_type, vec.n, picked, skip_added, skip_no_file);
    }

    if (picked > 0)
    {
        sleep_aid_save_vec_to_file(aid_type, &vec);
    }
    sleep_aid_vec_free(&vec);
    return picked;
}

void divoom_sleep_aid_send_get_all_list(void)
{
    int t;
    divoom_sleep_aid_get_all_list_req req;
    int times[DIVOOM_SLEEP_AID_STORED_TYPE_NUM];

    sleep_aid_load_max_update_times_from_file(times, DIVOOM_SLEEP_AID_STORED_TYPE_NUM);

    for (t = 0; t < DIVOOM_SLEEP_AID_STORED_TYPE_NUM; t++)
    {
        memset(&req, 0, sizeof(req));
        req.type = t;
        req.update_time = times[t];
        divoom_net_app_send_packet(DIVOOM_NET_COMM_AID_SLEEP_GET_ALL_LIST, DIVOOM_SERVER_TOPIC, (void *)&req);
    }
}


static void sleep_aid_copy_voice_name_out(char *out, int out_sz, const char *src)
{
    if (!out || out_sz <= 0)
    {
        return;
    }
    out[0] = '\0';
    if (!src || !src[0])
    {
        return;
    }
    strncpy(out, src, (unsigned)out_sz - 1);
    out[out_sz - 1] = '\0';
}

/** 与 `DIVOOM_LVGL_MENU_LANG_*` 下标一致（见 divoom_menu_control.h）。 */
static const char *const s_sleep_aid_locale_keys[] =
{
	"en",
	"zh-hans",
	"zh-hant",
	"ja",
	"de",
	"th",
	"ru",
	"ko",
	"vi",
	"fr",
	"tr",
	"es",
	"pt",
	"id",
	"it",
	"uk",
	"sw",
	"da",
	"pl",
};

static const char *sleep_aid_locale_key_for_lang(int lang_idx)
{
	if (lang_idx < 0 || lang_idx >= (int)(sizeof(s_sleep_aid_locale_keys) / sizeof(s_sleep_aid_locale_keys[0])))
	{
		return "en";
	}
	return s_sleep_aid_locale_keys[lang_idx];
}

/** 部分旧数据或目录名仍用 sc/tc/in。 */
static const char *sleep_aid_locale_alt_key_for_lang(int lang_idx)
{
	switch (lang_idx)
	{
	case 1:
	{
		return "sc";
	}
	case 2:
	{
		return "tc";
	}
	case 13:
	{
		return "in";
	}
	default:
	{
		return NULL;
	}
	}
}

static int sleep_aid_try_copy_lang_string_node(divoom_cJSON *node, char *out, int out_sz)
{
	if (!node)
	{
		return -1;
	}
	if ((node->type & divoom_cJSON_String) && node->valuestring && node->valuestring[0])
	{
		sleep_aid_copy_voice_name_out(out, out_sz, node->valuestring);
		return 0;
	}
	return -1;
}

static int sleep_aid_pick_name_from_language_object(divoom_cJSON *lang_node, int lang_idx, char *out, int out_sz)
{
	const char *primary;
	const char *alt;
	divoom_cJSON *item;
	divoom_cJSON *ch;

	primary = sleep_aid_locale_key_for_lang(lang_idx);
	item = divoom_cJSON_GetObjectItem(lang_node, primary);
	if (sleep_aid_try_copy_lang_string_node(item, out, out_sz) == 0)
	{
		return 0;
	}

	alt = sleep_aid_locale_alt_key_for_lang(lang_idx);
	if (alt)
	{
		item = divoom_cJSON_GetObjectItem(lang_node, alt);
		if (sleep_aid_try_copy_lang_string_node(item, out, out_sz) == 0)
		{
			return 0;
		}
	}

	item = divoom_cJSON_GetObjectItem(lang_node, "en");
	if (sleep_aid_try_copy_lang_string_node(item, out, out_sz) == 0)
	{
		return 0;
	}

	for (ch = lang_node->child; ch; ch = ch->next)
	{
		if (sleep_aid_try_copy_lang_string_node(ch, out, out_sz) == 0)
		{
			return 0;
		}
	}

	return -1;
}

static int sleep_aid_lang_entry_get_id(divoom_cJSON *obj)
{
    divoom_cJSON *t;

    if (!obj)
    {
        return -1;
    }

    t = divoom_cJSON_GetObjectItem(obj, "Language");
    if (t && (t->type & divoom_cJSON_Number))
    {
        return t->valueint;
    }
    t = divoom_cJSON_GetObjectItem(obj, "Lang");
    if (t && (t->type & divoom_cJSON_Number))
    {
        return t->valueint;
    }
    t = divoom_cJSON_GetObjectItem(obj, "lang");
    if (t && (t->type & divoom_cJSON_Number))
    {
        return t->valueint;
    }
    t = divoom_cJSON_GetObjectItem(obj, "LangId");
    if (t && (t->type & divoom_cJSON_Number))
    {
        return t->valueint;
    }
    return -1;
}

static int sleep_aid_lang_entry_get_name(divoom_cJSON *obj, char *out, int out_sz)
{
    divoom_cJSON *n;

    if (!obj || !out || out_sz <= 0)
    {
        return -1;
    }

    n = divoom_cJSON_GetObjectItem(obj, "Name");
    if (n && n->valuestring && n->valuestring[0])
    {
        sleep_aid_copy_voice_name_out(out, out_sz, n->valuestring);
        return 0;
    }
    n = divoom_cJSON_GetObjectItem(obj, "name");
    if (n && n->valuestring && n->valuestring[0])
    {
        sleep_aid_copy_voice_name_out(out, out_sz, n->valuestring);
        return 0;
    }
    n = divoom_cJSON_GetObjectItem(obj, "Title");
    if (n && n->valuestring && n->valuestring[0])
    {
        sleep_aid_copy_voice_name_out(out, out_sz, n->valuestring);
        return 0;
    }
    return -1;
}

/** Language：字符串 / 对象(locale→name，如 sleep_lang_468.cfg) / 数组(Lang+Name)。 */
static int sleep_aid_pick_name_from_language(divoom_cJSON *lang_node, int lang_idx, char *out, int out_sz)
{
    int i;
    int n;
    char tmp[DIVOOM_SLEEP_AID_VOICE_NAME_LEN];
    int got_fallback = 0;

    if (!lang_node || !out || out_sz <= 0)
    {
        return -1;
    }

    out[0] = '\0';

    if (lang_node->type & divoom_cJSON_String)
    {
        return sleep_aid_try_copy_lang_string_node(lang_node, out, out_sz);
    }

    if (lang_node->type & divoom_cJSON_Object)
    {
        return sleep_aid_pick_name_from_language_object(lang_node, lang_idx, out, out_sz);
    }

    if (lang_node->type & divoom_cJSON_Array)
    {
        n = divoom_cJSON_GetArraySize(lang_node);
        for (i = 0; i < n; i++)
        {
            divoom_cJSON *el = divoom_cJSON_GetArrayItem(lang_node, i);
            int lid;

            if (!el)
            {
                continue;
            }
            if (sleep_aid_lang_entry_get_name(el, tmp, (int)sizeof(tmp)) != 0)
            {
                continue;
            }
            lid = sleep_aid_lang_entry_get_id(el);
            if (lid == lang_idx)
            {
                sleep_aid_copy_voice_name_out(out, out_sz, tmp);
                return 0;
            }
            if (!got_fallback && (lid < 0 || lid == 0))
            {
                sleep_aid_copy_voice_name_out(out, out_sz, tmp);
                got_fallback = 1;
            }
        }
        if (got_fallback && out[0])
        {
            return 0;
        }
        for (i = 0; i < n; i++)
        {
            divoom_cJSON *el = divoom_cJSON_GetArrayItem(lang_node, i);

            if (el && sleep_aid_lang_entry_get_name(el, tmp, (int)sizeof(tmp)) == 0)
            {
                sleep_aid_copy_voice_name_out(out, out_sz, tmp);
                return 0;
            }
        }
        return -1;
    }

    return -1;
}

typedef struct
{
    uint32 *ids;
    char *names;
    int n;
} sleep_aid_type_lang_cache_t;

static void sleep_aid_type_lang_cache_clear(sleep_aid_type_lang_cache_t *cache);
static int sleep_aid_type_lang_cache_load(sleep_aid_type_lang_cache_t *cache, int audio_type, int lang_idx);
static int sleep_aid_type_lang_cache_lookup(const sleep_aid_type_lang_cache_t *cache, uint32 audio_id,
                                            char *out, int out_sz);

#define SLEEP_AID_LANG_RUNTIME_OK      1
#define SLEEP_AID_LANG_RUNTIME_FAILED  2

/** 进程内语言名缓存：每种 audio_type 在当前 lang_idx 下只读一次 sleep_lang_*.cfg */
static sleep_aid_type_lang_cache_t s_lang_runtime[DIVOOM_SLEEP_AID_STORED_TYPE_NUM];
static uint8 s_lang_runtime_state[DIVOOM_SLEEP_AID_STORED_TYPE_NUM];
static int s_lang_runtime_lang_idx = -1;

static void sleep_aid_lang_runtime_reset(void)
{
    int t;

    for (t = 0; t < DIVOOM_SLEEP_AID_STORED_TYPE_NUM; t++)
    {
        sleep_aid_type_lang_cache_clear(&s_lang_runtime[t]);
        s_lang_runtime_state[t] = 0;
    }
    s_lang_runtime_lang_idx = -1;
}

static void sleep_aid_lang_runtime_invalidate_type(int audio_type)
{
    if (audio_type < 0 || audio_type >= DIVOOM_SLEEP_AID_STORED_TYPE_NUM)
    {
        return;
    }
    sleep_aid_type_lang_cache_clear(&s_lang_runtime[audio_type]);
    s_lang_runtime_state[audio_type] = 0;
}

static int sleep_aid_lang_runtime_ensure(int audio_type, int lang_idx)
{
    if (audio_type < 0 || audio_type >= DIVOOM_SLEEP_AID_STORED_TYPE_NUM)
    {
        return -1;
    }

    if (lang_idx != s_lang_runtime_lang_idx)
    {
        sleep_aid_lang_runtime_reset();
        s_lang_runtime_lang_idx = lang_idx;
    }

    if (s_lang_runtime_state[audio_type] == SLEEP_AID_LANG_RUNTIME_OK)
    {
        return 0;
    }
    if (s_lang_runtime_state[audio_type] == SLEEP_AID_LANG_RUNTIME_FAILED)
    {
        return -1;
    }

    if (sleep_aid_type_lang_cache_load(&s_lang_runtime[audio_type], audio_type, lang_idx) == 0
        && s_lang_runtime[audio_type].n > 0)
    {
        s_lang_runtime_state[audio_type] = SLEEP_AID_LANG_RUNTIME_OK;
        return 0;
    }

    sleep_aid_type_lang_cache_clear(&s_lang_runtime[audio_type]);
    s_lang_runtime_state[audio_type] = SLEEP_AID_LANG_RUNTIME_FAILED;
    return -1;
}

static uint32 sleep_aid_lang_json_uint(const divoom_cJSON *node)
{
    if (!node || !(node->type & divoom_cJSON_Number))
    {
        return 0;
    }
    return (uint32)node->valuedouble;
}

static uint32 sleep_aid_lang_row_get_audio_id(divoom_cJSON *obj)
{
    divoom_cJSON *t;
    uint32 id;

    if (!obj)
    {
        return 0;
    }

    t = divoom_cJSON_GetObjectItem(obj, "AUDIO_ID");
    id = sleep_aid_lang_json_uint(t);
    if (id != 0)
    {
        return id;
    }
    t = divoom_cJSON_GetObjectItem(obj, "AudioId");
    id = sleep_aid_lang_json_uint(t);
    if (id != 0)
    {
        return id;
    }
    t = divoom_cJSON_GetObjectItem(obj, "audio_id");
    id = sleep_aid_lang_json_uint(t);
    if (id != 0)
    {
        return id;
    }
    t = divoom_cJSON_GetObjectItem(obj, "SleepId");
    id = sleep_aid_lang_json_uint(t);
    if (id != 0)
    {
        return id;
    }
    t = divoom_cJSON_GetObjectItem(obj, "voice_id");
    id = sleep_aid_lang_json_uint(t);
    if (id != 0)
    {
        return id;
    }
    t = divoom_cJSON_GetObjectItem(obj, "VoiceId");
    return sleep_aid_lang_json_uint(t);
}

/** Names 行：Name 字段或 Language 字符串/对象/数组（与 GetAllList 落盘格式一致）。 */
static int sleep_aid_lang_row_pick_display_name(divoom_cJSON *row, int lang_idx, char *out, int out_sz)
{
    divoom_cJSON *lang_node;

    if (!row || !out || out_sz <= 0)
    {
        return -1;
    }

    if (sleep_aid_lang_entry_get_name(row, out, out_sz) == 0 && out[0])
    {
        return 0;
    }

    lang_node = divoom_cJSON_GetObjectItem(row, "Language");
    if (!lang_node)
    {
        lang_node = divoom_cJSON_GetObjectItem(row, "language");
    }
    if (lang_node)
    {
        return sleep_aid_pick_name_from_language(lang_node, lang_idx, out, out_sz);
    }

    return -1;
}

static void sleep_aid_type_lang_cache_clear(sleep_aid_type_lang_cache_t *cache)
{
    if (!cache)
    {
        return;
    }
    if (cache->ids)
    {
        divoom_system_free(cache->ids);
    }
    if (cache->names)
    {
        divoom_system_free(cache->names);
    }
    cache->ids = NULL;
    cache->names = NULL;
    cache->n = 0;
}

static int sleep_aid_type_lang_cache_load(sleep_aid_type_lang_cache_t *cache, int audio_type, int lang_idx)
{
    uint8 *read_buf = NULL;
    divoom_cJSON *root = NULL;
    divoom_cJSON *names_arr = NULL;
    char path[192];
    char backup_path[192];
    int i;
    int n;
    int got;

    if (!cache || audio_type < 0 || audio_type >= DIVOOM_SLEEP_AID_STORED_TYPE_NUM)
    {
        return -1;
    }

    const char *locale_key;
    const char *alt_key;

    sleep_aid_type_lang_cache_clear(cache);
    locale_key = sleep_aid_locale_key_for_lang(lang_idx);
    alt_key = sleep_aid_locale_alt_key_for_lang(lang_idx);
    sleep_aid_build_type_lang_path(path, (int)sizeof(path), audio_type, locale_key);
    read_buf = (uint8 *)divoom_port_file_read_all(path, NULL);
    if (!read_buf && alt_key)
    {
        sleep_aid_build_type_lang_path(path, (int)sizeof(path), audio_type, alt_key);
        read_buf = (uint8 *)divoom_port_file_read_all(path, NULL);
    }
    if (!read_buf)
    {
        /* 所有正常语言文件都读取失败后，再按相同 locale 顺序尝试备份。 */
        sleep_aid_build_type_lang_backup_path(backup_path, (int)sizeof(backup_path), audio_type, locale_key);
        read_buf = (uint8 *)divoom_port_file_read_all(backup_path, NULL);
    }
    if (!read_buf && alt_key)
    {
        sleep_aid_build_type_lang_backup_path(backup_path, (int)sizeof(backup_path), audio_type, alt_key);
        read_buf = (uint8 *)divoom_port_file_read_all(backup_path, NULL);
    }
    if (!read_buf)
    {
        return -1;
    }

    root = divoom_cJSON_Parse((char *)read_buf);
    divoom_system_free(read_buf);
    if (!root)
    {
        return -1;
    }

    names_arr = divoom_cJSON_GetObjectItem(root, "Names");
    if (!names_arr || !(names_arr->type & divoom_cJSON_Array))
    {
        divoom_cJSON_Delete(root);
        return -1;
    }

    n = divoom_cJSON_GetArraySize(names_arr);
    if (n <= 0)
    {
        divoom_cJSON_Delete(root);
        return 0;
    }

    cache->ids = (uint32 *)divoom_system_malloc((unsigned)n * sizeof(uint32));
    cache->names = (char *)divoom_system_malloc((unsigned)n * DIVOOM_SLEEP_AID_VOICE_NAME_LEN);
    if (!cache->ids || !cache->names)
    {
        sleep_aid_type_lang_cache_clear(cache);
        divoom_cJSON_Delete(root);
        return -1;
    }

    got = 0;
    for (i = 0; i < n; i++)
    {
        divoom_cJSON *row = divoom_cJSON_GetArrayItem(names_arr, i);
        uint32 aid;
        char tmp[DIVOOM_SLEEP_AID_VOICE_NAME_LEN];

        if (!row)
        {
            continue;
        }
        aid = sleep_aid_lang_row_get_audio_id(row);
        if (aid == 0)
        {
            continue;
        }
        if (sleep_aid_lang_row_pick_display_name(row, lang_idx, tmp, (int)sizeof(tmp)) != 0 || !tmp[0])
        {
            continue;
        }
        cache->ids[got] = aid;
        sleep_aid_copy_voice_name_out(cache->names + got * DIVOOM_SLEEP_AID_VOICE_NAME_LEN,
                                      DIVOOM_SLEEP_AID_VOICE_NAME_LEN, tmp);
        got++;
    }
    cache->n = got;
    divoom_cJSON_Delete(root);
    return got > 0 ? 0 : -1;
}

static int sleep_aid_type_lang_cache_lookup(const sleep_aid_type_lang_cache_t *cache, uint32 audio_id,
                                            char *out, int out_sz)
{
    int i;

    if (!cache || !out || out_sz <= 0 || audio_id == 0)
    {
        return -1;
    }

    out[0] = '\0';
    for (i = 0; i < cache->n; i++)
    {
        if (cache->ids[i] == audio_id)
        {
            sleep_aid_copy_voice_name_out(out, out_sz, cache->names + i * DIVOOM_SLEEP_AID_VOICE_NAME_LEN);
            return out[0] ? 0 : -1;
        }
    }
    return -1;
}

static int sleep_aid_lang_cache_upsert_entry(sleep_aid_type_lang_cache_t *cache, uint32 audio_id,
                                             const char *name)
{
    uint32 *nids;
    char *nnames;
    int i;
    int nn;

    if (!cache || audio_id == 0 || !name || !name[0])
    {
        return -1;
    }

    for (i = 0; i < cache->n; i++)
    {
        if (cache->ids[i] == audio_id)
        {
            sleep_aid_copy_voice_name_out(cache->names + i * DIVOOM_SLEEP_AID_VOICE_NAME_LEN,
                                          DIVOOM_SLEEP_AID_VOICE_NAME_LEN, name);
            return 0;
        }
    }

    nn = cache->n + 1;
    nids = (uint32 *)divoom_system_malloc((unsigned)nn * sizeof(uint32));
    nnames = (char *)divoom_system_malloc((unsigned)nn * DIVOOM_SLEEP_AID_VOICE_NAME_LEN);
    if (!nids || !nnames)
    {
        if (nids)
        {
            divoom_system_free(nids);
        }
        if (nnames)
        {
            divoom_system_free(nnames);
        }
        return -1;
    }
    for (i = 0; i < cache->n; i++)
    {
        nids[i] = cache->ids[i];
        sleep_aid_copy_voice_name_out(nnames + i * DIVOOM_SLEEP_AID_VOICE_NAME_LEN,
                                      DIVOOM_SLEEP_AID_VOICE_NAME_LEN,
                                      cache->names + i * DIVOOM_SLEEP_AID_VOICE_NAME_LEN);
    }
    nids[cache->n] = audio_id;
    sleep_aid_copy_voice_name_out(nnames + cache->n * DIVOOM_SLEEP_AID_VOICE_NAME_LEN,
                                  DIVOOM_SLEEP_AID_VOICE_NAME_LEN, name);
    sleep_aid_type_lang_cache_clear(cache);
    cache->ids = nids;
    cache->names = nnames;
    cache->n = nn;
    return 0;
}

static int sleep_aid_type_lang_cache_save(int audio_type, int lang_idx,
                                          const sleep_aid_type_lang_cache_t *cache)
{
    divoom_cJSON *root;
    divoom_cJSON *names_arr;
    divoom_cJSON *row;
    char path[192];
    const char *locale_key;
    char *txt;
    FILE *fp;
    int i;

    if (!cache || cache->n <= 0)
    {
        return 0;
    }
    if (audio_type < 0 || audio_type >= DIVOOM_SLEEP_AID_STORED_TYPE_NUM)
    {
        return -1;
    }

    root = divoom_cJSON_CreateObject();
    names_arr = divoom_cJSON_CreateArray();
    if (!root || !names_arr)
    {
        if (root)
        {
            divoom_cJSON_Delete(root);
        }
        return -1;
    }
    divoom_cJSON_AddItemToObject(root, "Names", names_arr);
    for (i = 0; i < cache->n; i++)
    {
        row = divoom_cJSON_CreateObject();
        if (!row)
        {
            continue;
        }
        divoom_cJSON_AddNumberToObject(row, "AUDIO_ID", (int)cache->ids[i]);
        divoom_cJSON_AddStringToObject(row, "Name", cache->names + i * DIVOOM_SLEEP_AID_VOICE_NAME_LEN);
        divoom_cJSON_AddItemToArray(names_arr, row);
    }

    txt = divoom_cJSON_Print(root);
    divoom_cJSON_Delete(root);
    if (!txt)
    {
        return -1;
    }

    locale_key = sleep_aid_locale_key_for_lang(lang_idx);
    sleep_aid_build_type_lang_path(path, (int)sizeof(path), audio_type, locale_key);
    fp = divoom_port_file_open(path, "w+");
    if (!fp)
    {
        divoom_system_free(txt);
        return -1;
    }
    divoom_port_file_write(txt, strlen(txt), 1, fp);
    divoom_port_file_close(fp);
    divoom_system_free(txt);
    return 0;
}

static int sleep_aid_upsert_name_in_type_lang_file(int audio_type, int lang_idx, uint32 audio_id,
                                                   const char *name)
{
    sleep_aid_type_lang_cache_t cache;
    int ret;

    if (audio_id == 0 || !name || !name[0])
    {
        return -1;
    }
    if (audio_type < 0 || audio_type >= DIVOOM_SLEEP_AID_STORED_TYPE_NUM)
    {
        return -1;
    }

    memset(&cache, 0, sizeof(cache));
    (void)sleep_aid_type_lang_cache_load(&cache, audio_type, lang_idx);
    if (sleep_aid_lang_cache_upsert_entry(&cache, audio_id, name) != 0)
    {
        sleep_aid_type_lang_cache_clear(&cache);
        return -1;
    }
    ret = sleep_aid_type_lang_cache_save(audio_type, lang_idx, &cache);
    sleep_aid_type_lang_cache_clear(&cache);
    if (ret == 0)
    {
        sleep_aid_lang_runtime_invalidate_type(audio_type);
    }
    return ret;
}

static void sleep_aid_flush_languages_for_rows(int list_type, divoom_sleep_aid_list_row_parsed *rows, int srv_n)
{
    divoom_cJSON **parsed;
    sleep_aid_type_lang_cache_t merged;
    int locale_n;
    int li;
    int ni;
    char name[DIVOOM_SLEEP_AID_VOICE_NAME_LEN];

    if (!rows || srv_n <= 0)
    {
        return;
    }
    if (list_type < 0 || list_type >= DIVOOM_SLEEP_AID_STORED_TYPE_NUM)
    {
        return;
    }

    parsed = (divoom_cJSON **)divoom_system_malloc((unsigned)srv_n * sizeof(divoom_cJSON *));
    if (!parsed)
    {
        return;
    }
    memset(parsed, 0, (unsigned)srv_n * sizeof(divoom_cJSON *));
    for (ni = 0; ni < srv_n; ni++)
    {
        if (rows[ni].language_json && rows[ni].language_json[0])
        {
            parsed[ni] = divoom_cJSON_Parse(rows[ni].language_json);
        }
    }

    locale_n = (int)(sizeof(s_sleep_aid_locale_keys) / sizeof(s_sleep_aid_locale_keys[0]));
    for (li = 0; li < locale_n; li++)
    {
        memset(&merged, 0, sizeof(merged));
        (void)sleep_aid_type_lang_cache_load(&merged, list_type, li);
        for (ni = 0; ni < srv_n; ni++)
        {
            if (!parsed[ni] || rows[ni].voice_id == 0)
            {
                continue;
            }
            name[0] = '\0';
            if (sleep_aid_pick_name_from_language(parsed[ni], li, name, (int)sizeof(name)) != 0 || !name[0])
            {
                continue;
            }
            (void)sleep_aid_lang_cache_upsert_entry(&merged, rows[ni].voice_id, name);
        }
        if (merged.n > 0)
        {
            (void)sleep_aid_type_lang_cache_save(list_type, li, &merged);
        }
        sleep_aid_type_lang_cache_clear(&merged);
    }
    sleep_aid_lang_runtime_invalidate_type(list_type);

    for (ni = 0; ni < srv_n; ni++)
    {
        if (parsed[ni])
        {
            divoom_cJSON_Delete(parsed[ni]);
        }
    }
    divoom_system_free(parsed);
}

static void sleep_aid_save_language_for_type(int list_type, uint32 audio_id, const char *language_json)
{
    divoom_cJSON *lang_node = NULL;
    int li;
    int locale_n;
    char name[DIVOOM_SLEEP_AID_VOICE_NAME_LEN];

    if (!language_json || !language_json[0] || audio_id == 0)
    {
        return;
    }
    if (list_type < 0 || list_type >= DIVOOM_SLEEP_AID_STORED_TYPE_NUM)
    {
        return;
    }

    lang_node = divoom_cJSON_Parse(language_json);
    if (!lang_node)
    {
        return;
    }

    locale_n = (int)(sizeof(s_sleep_aid_locale_keys) / sizeof(s_sleep_aid_locale_keys[0]));
    for (li = 0; li < locale_n; li++)
    {
        name[0] = '\0';
        if (sleep_aid_pick_name_from_language(lang_node, li, name, (int)sizeof(name)) != 0 || !name[0])
        {
            continue;
        }
        sleep_aid_upsert_name_in_type_lang_file(list_type, li, audio_id, name);
    }

    divoom_cJSON_Delete(lang_node);
}

static int sleep_aid_resolve_name_audio_type(int voice_type, uint32 voice_id)
{
    int t;

    if (voice_type == DIVOOM_SLEEP_AID_TYPE_NOISE || voice_type == DIVOOM_SLEEP_AID_TYPE_WHITE
        || voice_type == DIVOOM_SLEEP_AID_TYPE_MUSIC)
    {
        return voice_type;
    }
    if (voice_type == DIVOOM_SLEEP_AID_TYPE_FAV)
    {
        divoom_sleep_aid_fav_item *a;
        int n;
        int i;

        a = divoom_sleep_aid_fav_get_list(&n);
        if (a)
        {
            for (i = 0; i < n; i++)
            {
                if (a[i].audio_id == voice_id)
                {
                    t = a[i].audio_type;
                    divoom_system_free(a);
                    if (t >= 0 && t < DIVOOM_SLEEP_AID_STORED_TYPE_NUM)
                    {
                        return t;
                    }
                    return -1;
                }
            }
            divoom_system_free(a);
        }
        return -1;
    }
    return -1;
}

static int sleep_aid_lookup_name_in_type_lang_file(int audio_type, int lang_idx, uint32 audio_id,
                                                   char *out, int out_sz)
{
    if (!out || out_sz <= 0 || audio_id == 0)
    {
        return -1;
    }
    if (audio_type < 0 || audio_type >= DIVOOM_SLEEP_AID_STORED_TYPE_NUM)
    {
        return -1;
    }

    out[0] = '\0';
    if (sleep_aid_lang_runtime_ensure(audio_type, lang_idx) != 0)
    {
        return -1;
    }
    return sleep_aid_type_lang_cache_lookup(&s_lang_runtime[audio_type], audio_id, out, out_sz);
}

static void sleep_aid_voice_list_fill_names(divoom_sleep_aid_voice_list_item *out, int n, int list_aid_type)
{
    int lang_idx;
    int i;
    int t;

    if (!out || n <= 0)
    {
        return;
    }

    if (list_aid_type == DIVOOM_SLEEP_AID_TYPE_RECORD || list_aid_type == DIVOOM_SLEEP_AID_TYPE_DIY)
    {
        return;
    }

    lang_idx = divoom_main_set_lang_flag(-1);
    if (lang_idx < 0)
    {
        lang_idx = 0;
    }

    for (i = 0; i < n; i++)
    {
        out[i].name[0] = '\0';

        if (list_aid_type == DIVOOM_SLEEP_AID_TYPE_FAV
            && (out[i].audio_type == DIVOOM_SLEEP_AID_TYPE_RECORD
                || out[i].audio_type == DIVOOM_SLEEP_AID_TYPE_DIY))
        {
            sleep_aid_copy_special_voice_display_name(out[i].audio_type, out[i].name,
                (int)sizeof(out[i].name));
            continue;
        }

        t = out[i].audio_type;
        if (list_aid_type != DIVOOM_SLEEP_AID_TYPE_FAV)
        {
            t = list_aid_type;
        }
        if (t < 0 || t >= DIVOOM_SLEEP_AID_STORED_TYPE_NUM)
        {
            continue;
        }
        if (sleep_aid_lang_runtime_ensure(t, lang_idx) == 0)
        {
            (void)sleep_aid_type_lang_cache_lookup(&s_lang_runtime[t], out[i].audio_id, out[i].name,
                                                   (int)sizeof(out[i].name));
        }
    }
}

void divoom_sleep_aid_send_get_voice_name(int voice_type, uint32 voice_id, char *name_out)
{
    int lang_idx;
    int audio_type;

    if (!name_out)
    {
        return;
    }

    name_out[0] = '\0';

    if (voice_id == 0)
    {
        return;
    }

    if (voice_type == DIVOOM_SLEEP_AID_TYPE_RECORD || voice_type == DIVOOM_SLEEP_AID_TYPE_DIY)
    {
        sleep_aid_copy_special_voice_display_name(voice_type, name_out, DIVOOM_SLEEP_AID_VOICE_NAME_LEN);
        return;
    }

    if (voice_id > DIVOOM_VOICE_ID_SLEEP_AID_RECORD_BASE && voice_id < DIVOOM_VOICE_ID_END_SLEEP_AID_RECORD)
    {
        sleep_aid_copy_special_voice_display_name(DIVOOM_SLEEP_AID_TYPE_RECORD, name_out,
            DIVOOM_SLEEP_AID_VOICE_NAME_LEN);
        return;
    }

    audio_type = sleep_aid_resolve_name_audio_type(voice_type, voice_id);
    if (audio_type < 0)
    {
        return;
    }

    lang_idx = divoom_main_set_lang_flag(-1);
    if (lang_idx < 0)
    {
        lang_idx = 0;
    }

    sleep_aid_lookup_name_in_type_lang_file(audio_type, lang_idx, voice_id, name_out, DIVOOM_SLEEP_AID_VOICE_NAME_LEN);
}

/**
 * 按 voice_id 在本地 sleep_aid_%02d.cfg 中查找 file_id，供图片下载模块作 URL。
 * @return 找到且 file_id 非空返回 1，否则返回 0。
 */
int divoom_sleep_aid_get_url_addr(uint32 voice_id, char *url_addr)
{
    int t;
    divoom_sleep_aid_vec vec;
    int idx;

    if (!url_addr || voice_id == 0)
    {
        return 0;
    }
    url_addr[0] = '\0';

    for (t = 0; t < DIVOOM_SLEEP_AID_STORED_TYPE_NUM; t++)
    {
        memset(&vec, 0, sizeof(vec));
        if (sleep_aid_load_vec_from_file(t, &vec) != 0)
        {
            continue;
        }
        idx = sleep_aid_vec_find(&vec, voice_id);
        if (idx >= 0 && vec.buf[idx].file_id[0] != '\0')
        {
            strncpy(url_addr, vec.buf[idx].file_id, DIVOOM_MANAGE_FILE_ADDR_LEN - 1);
            url_addr[DIVOOM_MANAGE_FILE_ADDR_LEN - 1] = '\0';
            sleep_aid_vec_free(&vec);
            return 1;
        }
        sleep_aid_vec_free(&vec);
    }
    return 0;
}
