/*
 * Buddy 表情配置加载：按需读取 buddy/ 下小 JSON，解析结果缓存。
 */
#include "divoom_buddy_config.h"
#include "divoom_clock_ato_resources.h"
#include "divoom.h"
#include "divoom_app_com.h"
#include "divoom_log.h"
#include "cJSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DIVOOM_BUDDY_CFG_BASE_PATH          "/usr/share/divoom_app/ui/"
#define DIVOOM_BUDDY_CLIP_CACHE_MAX         64
#define DIVOOM_BUDDY_STATE_CACHE_MAX        32

static char g_buddy_cfg_dir[96] = DIVOOM_BUDDY_CFG_DIR_DEFAULT;
static divoom_buddy_layers_cfg_t g_buddy_layers_cache;
static uint8 g_buddy_layers_loaded;

typedef struct
{
    char id[DIVOOM_BUDDY_ID_MAX];
    divoom_buddy_clip_cfg_t clip;
} divoom_buddy_clip_cache_entry_t;

typedef struct
{
    char id[DIVOOM_BUDDY_ID_MAX];
    divoom_buddy_state_cfg_t state;
} divoom_buddy_state_cache_entry_t;

/* Cache access and widget lifecycle are serialized by the LVGL lock. */
static unsigned int g_buddy_cache_users;
static divoom_buddy_clip_cache_entry_t *g_buddy_clip_cache[DIVOOM_BUDDY_CLIP_CACHE_MAX];
static unsigned int g_buddy_clip_cache_count;
static divoom_buddy_state_cache_entry_t *g_buddy_state_cache[DIVOOM_BUDDY_STATE_CACHE_MAX];
static unsigned int g_buddy_state_cache_count;

static void divoom_buddy_source_sit_set_layer(divoom_buddy_layers_cfg_t *out, int idx,
    const char *id, int z, int x, int y)
{
    if (!out || idx < 0 || idx >= DIVOOM_BUDDY_MAX_LAYERS)
    {
        return;
    }

    strncpy(out->layers[idx].id, id, DIVOOM_BUDDY_ID_MAX - 1);
    out->layers[idx].id[DIVOOM_BUDDY_ID_MAX - 1] = '\0';
    out->layers[idx].z = z;
    out->layers[idx].x = x;
    out->layers[idx].y = y;
}

static void divoom_buddy_source_sit_load_layers(divoom_buddy_layers_cfg_t *out)
{
    memset(out, 0, sizeof(*out));
    out->region_w = 193;
    out->region_h = 230;
    out->layer_count = 4;
    divoom_buddy_source_sit_set_layer(out, 0, "tail", 0, -10, 94);
    divoom_buddy_source_sit_set_layer(out, 1, "body", 1, -21, -26);
    divoom_buddy_source_sit_set_layer(out, 2, "ears", 2, 25, -4);
    divoom_buddy_source_sit_set_layer(out, 3, "face", 3, 70, 48);
}

static void divoom_buddy_source_sit_set_clip_base(divoom_buddy_clip_cfg_t *out,
    const char *id, const char *layer_id, uint8 loop)
{
    memset(out, 0, sizeof(*out));
    strncpy(out->id, id, DIVOOM_BUDDY_ID_MAX - 1);
    strncpy(out->layer_id, layer_id, DIVOOM_BUDDY_ID_MAX - 1);
    strncpy(out->pose_id, "sit_pose", DIVOOM_BUDDY_ID_MAX - 1);
    out->loop = loop;
}

static void divoom_buddy_source_sit_add_frame(divoom_buddy_clip_cfg_t *out,
    const char *frame, int delay_ms)
{
    int idx = out->frame_count;
    if (idx >= DIVOOM_BUDDY_MAX_FRAMES)
    {
        return;
    }

    strncpy(out->frames[idx], frame, DIVOOM_BUDDY_FRAME_NAME_MAX - 1);
    out->frames[idx][DIVOOM_BUDDY_FRAME_NAME_MAX - 1] = '\0';
    out->frame_delays_ms[idx] = delay_ms;
    out->frame_count++;
    out->has_frame_delays = 1;
}

static int divoom_buddy_source_sit_get_clip(const char *clip_id, divoom_buddy_clip_cfg_t *out)
{
    const char *face = NULL;

    if (strcmp(clip_id, "act_sit_body") == 0 || strcmp(clip_id, "act_body") == 0)
    {
        divoom_buddy_source_sit_set_clip_base(out, clip_id, "body", 0);
        divoom_buddy_source_sit_add_frame(out, "body/sit_body_base.png", 1000);
        return 0;
    }

    if (strcmp(clip_id, "act_ears_flop") == 0 || strcmp(clip_id, "act_sit_ears_flop") == 0)
    {
        divoom_buddy_source_sit_set_clip_base(out, clip_id, "ears", 1);
        divoom_buddy_source_sit_add_frame(out, "ears/sit_ears_flop_01.png", 280);
        divoom_buddy_source_sit_add_frame(out, "ears/sit_ears_flop_02.png", 80);
        divoom_buddy_source_sit_add_frame(out, "ears/sit_ears_flop_03.png", 120);
        divoom_buddy_source_sit_add_frame(out, "ears/sit_ears_flop_04.png", 80);
        divoom_buddy_source_sit_add_frame(out, "ears/sit_ears_flop_05.png", 420);
        return 0;
    }

    if (strcmp(clip_id, "act_tail_wag") == 0 || strcmp(clip_id, "act_sit_tail_wag") == 0)
    {
        divoom_buddy_source_sit_set_clip_base(out, clip_id, "tail", 1);
        divoom_buddy_source_sit_add_frame(out, "tail/sit_tail_wag_01.png", 80);
        divoom_buddy_source_sit_add_frame(out, "tail/sit_tail_wag_02.png", 70);
        divoom_buddy_source_sit_add_frame(out, "tail/sit_tail_wag_03.png", 60);
        divoom_buddy_source_sit_add_frame(out, "tail/sit_tail_wag_04.png", 70);
        divoom_buddy_source_sit_add_frame(out, "tail/sit_tail_wag_05.png", 80);
        divoom_buddy_source_sit_add_frame(out, "tail/sit_tail_wag_04.png", 70);
        divoom_buddy_source_sit_add_frame(out, "tail/sit_tail_wag_03.png", 60);
        divoom_buddy_source_sit_add_frame(out, "tail/sit_tail_wag_02.png", 70);
        return 0;
    }

    if (strcmp(clip_id, "expr_happy") == 0 || strcmp(clip_id, "expr_sit_happy") == 0)
    {
        face = "happy";
    }
    else if (strcmp(clip_id, "expr_curious") == 0 || strcmp(clip_id, "expr_sit_curious") == 0)
    {
        face = "curious";
    }
    else if (strcmp(clip_id, "expr_sleepy") == 0 || strcmp(clip_id, "expr_sit_sleepy") == 0)
    {
        face = "sleepy";
    }
    else if (strcmp(clip_id, "expr_neutral") == 0 || strcmp(clip_id, "expr_sit_neutral") == 0)
    {
        face = "neutral";
    }

    if (!face)
    {
        return -1;
    }

    divoom_buddy_source_sit_set_clip_base(out, clip_id, "face", 1);
    if (strcmp(face, "curious") == 0)
    {
        divoom_buddy_source_sit_add_frame(out, "face/sit_face_curious_01.png", 250);
        divoom_buddy_source_sit_add_frame(out, "face/sit_face_curious_02.png", 160);
        divoom_buddy_source_sit_add_frame(out, "face/sit_face_curious_03.png", 700);
        divoom_buddy_source_sit_add_frame(out, "face/sit_face_curious_04.png", 160);
        divoom_buddy_source_sit_add_frame(out, "face/sit_face_curious_05.png", 350);
    }
    else if (strcmp(face, "happy") == 0)
    {
        divoom_buddy_source_sit_add_frame(out, "face/sit_face_happy_01.png", 250);
        divoom_buddy_source_sit_add_frame(out, "face/sit_face_happy_02.png", 500);
        divoom_buddy_source_sit_add_frame(out, "face/sit_face_happy_03.png", 350);
    }
    else if (strcmp(face, "sleepy") == 0)
    {
        divoom_buddy_source_sit_add_frame(out, "face/sit_face_sleepy_01.png", 700);
        divoom_buddy_source_sit_add_frame(out, "face/sit_face_sleepy_02.png", 240);
        divoom_buddy_source_sit_add_frame(out, "face/sit_face_sleepy_03.png", 900);
    }
    else
    {
        divoom_buddy_source_sit_add_frame(out, "face/sit_face_neutral_01.png", 1200);
        divoom_buddy_source_sit_add_frame(out, "face/sit_face_neutral_02.png", 80);
        divoom_buddy_source_sit_add_frame(out, "face/sit_face_neutral_03.png", 1000);
    }

    return 0;
}

static void divoom_buddy_source_sit_add_pool(divoom_buddy_pool_opt_t *opt, const char *clip, int weight)
{
    strncpy(opt->clip, clip, DIVOOM_BUDDY_ID_MAX - 1);
    opt->clip[DIVOOM_BUDDY_ID_MAX - 1] = '\0';
    opt->weight = weight;
}

static void divoom_buddy_cfg_add_pool(divoom_buddy_pool_opt_t *opts, int *cnt, const char *clip, int weight)
{
    if (*cnt >= DIVOOM_BUDDY_MAX_POOL_OPTS)
    {
        return;
    }

    strncpy(opts[*cnt].clip, clip, DIVOOM_BUDDY_ID_MAX - 1);
    opts[*cnt].clip[DIVOOM_BUDDY_ID_MAX - 1] = '\0';
    opts[*cnt].weight = (weight > 0) ? weight : 1;
    (*cnt)++;
}

static void divoom_buddy_cfg_add_micro(divoom_buddy_state_cfg_t *out, const char *clip,
    int interval_min_ms, int interval_max_ms, float chance)
{
    divoom_buddy_micro_def_t *micro;

    if (out->micro_action_count >= DIVOOM_BUDDY_MAX_MICRO)
    {
        return;
    }

    micro = &out->micro_actions[out->micro_action_count++];
    strncpy(micro->clip, clip, DIVOOM_BUDDY_ID_MAX - 1);
    micro->clip[DIVOOM_BUDDY_ID_MAX - 1] = '\0';
    micro->interval_min_ms = interval_min_ms;
    micro->interval_max_ms = interval_max_ms;
    micro->chance = chance;
}

static int divoom_buddy_cfg_id_eq(const char *state_id, const char *a, const char *b, const char *c)
{
    return (a && strcmp(state_id, a) == 0) ||
        (b && strcmp(state_id, b) == 0) ||
        (c && strcmp(state_id, c) == 0);
}

static int divoom_buddy_v2_get_builtin_state(const char *state_id, divoom_buddy_state_cfg_t *out)
{
    const char *base_clip = "act_sit_body";
    const char *ear_clip = "act_sit_ears_flop";
    const char *tail_clip = "act_sit_tail_wag";
    const char *expr_a = "expr_sit_neutral";
    const char *expr_b = NULL;
    const char *expr_c = NULL;
    int w_a = 100;
    int w_b = 0;
    int w_c = 0;
    int prone = 0;
    int music_only = 0;

    if (!state_id || !out)
    {
        return -1;
    }

    memset(out, 0, sizeof(*out));
    strncpy(out->id, state_id, DIVOOM_BUDDY_ID_MAX - 1);
    out->id[DIVOOM_BUDDY_ID_MAX - 1] = '\0';
    out->expression_refresh_min_ms = 8000;
    out->expression_refresh_max_ms = 18000;
    out->base_action_refresh_min_ms = 60000;
    out->base_action_refresh_max_ms = 120000;

    if (divoom_buddy_cfg_id_eq(state_id, "idle_life", "default_life", "neutral") ||
        divoom_buddy_cfg_id_eq(state_id, "calm", "sit_idle", NULL))
    {
        expr_a = "expr_sit_neutral"; w_a = 70;
        expr_b = "expr_sit_happy"; w_b = 15;
        expr_c = "expr_sit_curious"; w_c = 15;
        divoom_buddy_cfg_add_micro(out, "act_stretch", 35000, 90000, 0.35f);
        divoom_buddy_cfg_add_micro(out, "act_turn_back", 45000, 120000, 0.30f);
    }
    else if (divoom_buddy_cfg_id_eq(state_id, "sunny", "happy", "scene_sunny"))
    {
        expr_a = "expr_sit_happy"; w_a = 75;
        expr_b = "expr_sit_neutral"; w_b = 25;
        divoom_buddy_cfg_add_micro(out, "fx_sit_star", 18000, 36000, 0.35f);
    }
    else if (divoom_buddy_cfg_id_eq(state_id, "rainy", "lazy", "scene_rainy") ||
        divoom_buddy_cfg_id_eq(state_id, "hot", "scene_hot", NULL))
    {
        prone = 1;
        expr_a = "expr_prone_sleepy"; w_a = 70;
        expr_b = "expr_prone_neutral"; w_b = 30;
        divoom_buddy_cfg_add_micro(out, "fx_prone_sleep_zzz", 12000, 26000, 0.45f);
    }
    else if (divoom_buddy_cfg_id_eq(state_id, "cold", "scene_cold", NULL))
    {
        prone = 1;
        expr_a = "expr_prone_sad"; w_a = 65;
        expr_b = "expr_prone_sleepy"; w_b = 35;
    }
    else if (divoom_buddy_cfg_id_eq(state_id, "photo", "look_photo", "scene_photo") ||
        divoom_buddy_cfg_id_eq(state_id, "pixel", "look_pixel", "scene_pixel") ||
        divoom_buddy_cfg_id_eq(state_id, "curious", "content_view", NULL))
    {
        expr_a = "expr_sit_curious"; w_a = 85;
        expr_b = "expr_sit_happy"; w_b = 15;
        divoom_buddy_cfg_add_micro(out, "fx_sit_question", 8000, 18000, 0.60f);
    }
    /* 唱歌动作只允许由明确的音乐状态构造，普通兴奋/开心状态不得复用。 */
    else if (divoom_buddy_cfg_id_eq(state_id, "music", "singing", "scene_music"))
    {
        music_only = 1;
        divoom_buddy_cfg_add_micro(out, "act_dog_sing", 0, 0, 1.0f);
    }
    else if (divoom_buddy_cfg_id_eq(state_id, "stock_up", "proud", "scene_stock_up"))
    {
        expr_a = "expr_sit_happy"; w_a = 80;
        expr_b = "expr_sit_neutral"; w_b = 20;
        divoom_buddy_cfg_add_micro(out, "fx_sit_star", 9000, 22000, 0.55f);
    }
    else if (divoom_buddy_cfg_id_eq(state_id, "stock_down", "confused", "scene_stock_down"))
    {
        prone = 1;
        expr_a = "expr_prone_sad"; w_a = 80;
        expr_b = "expr_prone_neutral"; w_b = 20;
    }
    else if (divoom_buddy_cfg_id_eq(state_id, "low_battery", "worried", "alert") ||
        divoom_buddy_cfg_id_eq(state_id, "scene_worried", NULL, NULL))
    {
        expr_a = "expr_sit_curious"; w_a = 70;
        expr_b = "expr_sit_sleepy"; w_b = 30;
        divoom_buddy_cfg_add_micro(out, "fx_sit_question", 5000, 12000, 0.70f);
    }
    else if (divoom_buddy_cfg_id_eq(state_id, "night", "sleepy", "scene_night"))
    {
        prone = 1;
        expr_a = "expr_prone_sleepy"; w_a = 100;
        out->expression_refresh_min_ms = 12000;
        out->expression_refresh_max_ms = 30000;
        divoom_buddy_cfg_add_micro(out, "fx_prone_sleep_zzz", 6000, 14000, 0.70f);
    }
    else
    {
        return -1;
    }

    if (music_only)
    {
        /* 音乐状态由全帧唱歌动画独占，不能再启动耳朵、尾巴、身体或表情通道。 */
        return 0;
    }

    if (prone)
    {
        base_clip = "act_prone_body";
        ear_clip = "act_prone_ears_flop";
        tail_clip = "act_prone_tail_wag";
    }

    divoom_buddy_cfg_add_pool(out->expression_opts, &out->expression_opt_count, expr_a, w_a);
    if (expr_b)
    {
        divoom_buddy_cfg_add_pool(out->expression_opts, &out->expression_opt_count, expr_b, w_b);
    }
    if (expr_c)
    {
        divoom_buddy_cfg_add_pool(out->expression_opts, &out->expression_opt_count, expr_c, w_c);
    }

    divoom_buddy_cfg_add_pool(out->base_action_opts, &out->base_action_opt_count, base_clip, 100);
    divoom_buddy_cfg_add_micro(out, ear_clip, 0, 0, 1.0f);
    divoom_buddy_cfg_add_micro(out, tail_clip, 0, 0, 1.0f);
    return 0;
}

static int divoom_buddy_source_sit_get_state(const char *state_id, divoom_buddy_state_cfg_t *out)
{
    const char *expr = "expr_neutral";

    memset(out, 0, sizeof(*out));
    strncpy(out->id, state_id, DIVOOM_BUDDY_ID_MAX - 1);
    out->id[DIVOOM_BUDDY_ID_MAX - 1] = '\0';

    if (strstr(state_id, "happy") || strstr(state_id, "sunny") ||
        strstr(state_id, "music") || strstr(state_id, "stock_up"))
    {
        expr = "expr_happy";
    }
    else if (strstr(state_id, "curious") || strstr(state_id, "photo") ||
        strstr(state_id, "pixel") || strstr(state_id, "worried"))
    {
        expr = "expr_curious";
    }
    else if (strstr(state_id, "sleep") || strstr(state_id, "night") ||
        strstr(state_id, "hot") || strstr(state_id, "rainy"))
    {
        expr = "expr_sleepy";
    }

    out->expression_opt_count = 1;
    out->expression_refresh_min_ms = 8000;
    out->expression_refresh_max_ms = 16000;
    divoom_buddy_source_sit_add_pool(&out->expression_opts[0], expr, 1);

    out->base_action_opt_count = 1;
    out->base_action_refresh_min_ms = 60000;
    out->base_action_refresh_max_ms = 60000;
    divoom_buddy_source_sit_add_pool(&out->base_action_opts[0], "act_sit_body", 1);

    out->micro_action_count = 2;
    strncpy(out->micro_actions[0].clip, "act_ears_flop", DIVOOM_BUDDY_ID_MAX - 1);
    out->micro_actions[0].chance = 1.0f;
    strncpy(out->micro_actions[1].clip, "act_tail_wag", DIVOOM_BUDDY_ID_MAX - 1);
    out->micro_actions[1].chance = 1.0f;
    return 0;
}

static char *divoom_buddy_config_read_text(const char *rel_path)
{
    char path[256];
    char *buf;

    if (!rel_path)
    {
        return NULL;
    }

    if (strncmp(rel_path, "ato/", 4) == 0)
    {
        if (!divoom_clock_ato_resources_path(rel_path, path, sizeof(path), 0))
        {
            return NULL;
        }
    }
    else
    {
        snprintf(path, sizeof(path), "%s%s", DIVOOM_BUDDY_CFG_BASE_PATH, rel_path);
    }
    buf = divoom_app_com_read_file(path, NULL);
    return buf;
}

static void divoom_buddy_config_build_rel(char *out, int out_sz, const char *sub_path)
{
    snprintf(out, (size_t)out_sz, "%s%s", g_buddy_cfg_dir, sub_path);
}

static int divoom_buddy_config_json_is_array(divoom_cJSON *node)
{
    if (!node)
    {
        return 0;
    }

    return (node->type & divoom_cJSON_Array) ? 1 : 0;
}

static int divoom_buddy_config_json_is_number(divoom_cJSON *node)
{
    if (!node)
    {
        return 0;
    }

    return (node->type & divoom_cJSON_Number) ? 1 : 0;
}

static int divoom_buddy_config_json_is_string(divoom_cJSON *node)
{
    if (!node)
    {
        return 0;
    }

    return (node->type & divoom_cJSON_String) ? 1 : 0;
}

static int divoom_buddy_config_is_source_sit(void)
{
    return strcmp(g_buddy_cfg_dir, "buddy/source/sit/") == 0;
}

static int divoom_buddy_config_is_default_dir(void)
{
    return strcmp(g_buddy_cfg_dir, DIVOOM_BUDDY_CFG_DIR_DEFAULT) == 0;
}

static int divoom_buddy_config_layer_cache_index(const char *layer_id)
{
    int i;

    if (!layer_id || !g_buddy_layers_loaded)
    {
        return -1;
    }

    for (i = 0; i < g_buddy_layers_cache.layer_count; i++)
    {
        if (strcmp(g_buddy_layers_cache.layers[i].id, layer_id) == 0)
        {
            return i;
        }
    }

    return -1;
}

static void divoom_buddy_config_set_pose_layer_pos(divoom_buddy_clip_cfg_t *out,
    const char *layer_id, int x, int y)
{
    int idx;

    idx = divoom_buddy_config_layer_cache_index(layer_id);
    if (idx < 0 || idx >= DIVOOM_BUDDY_MAX_LAYERS)
    {
        return;
    }

    out->has_layer_pos = 1;
    out->layer_pos_x[idx] = x;
    out->layer_pos_y[idx] = y;
    out->layer_pos_mask |= (uint8)(1u << idx);
}

static void divoom_buddy_config_apply_pose_layer_pos(divoom_buddy_clip_cfg_t *out, const char *pose)
{
    if (!out || !pose)
    {
        return;
    }

    if (strcmp(pose, "sit_pose") == 0)
    {
        divoom_buddy_config_set_pose_layer_pos(out, "tail", 21, 120);
        divoom_buddy_config_set_pose_layer_pos(out, "body", 0, 0);
        divoom_buddy_config_set_pose_layer_pos(out, "ears", 56, 22);
        divoom_buddy_config_set_pose_layer_pos(out, "face", 101, 74);
        divoom_buddy_config_set_pose_layer_pos(out, "extra", 0, 13);
    }
    else if (strcmp(pose, "prone_pose") == 0)
    {
        divoom_buddy_config_set_pose_layer_pos(out, "tail", 9, 136);
        divoom_buddy_config_set_pose_layer_pos(out, "body", 0, 0);
        divoom_buddy_config_set_pose_layer_pos(out, "ears", 65, 118);
        divoom_buddy_config_set_pose_layer_pos(out, "zzz_effect", 143, 115);
        divoom_buddy_config_set_pose_layer_pos(out, "face", 111, 169);
    }
}

static int divoom_buddy_config_parse_int_range(divoom_cJSON *node, int *out_min, int *out_max)
{
    divoom_cJSON *a0;
    divoom_cJSON *a1;

    if (divoom_buddy_config_json_is_number(node))
    {
        *out_min = node->valueint;
        *out_max = node->valueint;
        return 0;
    }

    if (!divoom_buddy_config_json_is_array(node))
    {
        return -1;
    }

    if (divoom_cJSON_GetArraySize(node) < 2)
    {
        return -1;
    }

    a0 = divoom_cJSON_GetArrayItem(node, 0);
    a1 = divoom_cJSON_GetArrayItem(node, 1);
    if (!divoom_buddy_config_json_is_number(a0) || !divoom_buddy_config_json_is_number(a1))
    {
        return -1;
    }

    *out_min = a0->valueint;
    *out_max = a1->valueint;
    if (*out_min > *out_max)
    {
        int t = *out_min;
        *out_min = *out_max;
        *out_max = t;
    }

    return 0;
}

static int divoom_buddy_config_parse_float_range(divoom_cJSON *node, float *out_min, float *out_max, uint8 *has)
{
    divoom_cJSON *a0;
    divoom_cJSON *a1;

    *has = 0;
    if (!node)
    {
        return 0;
    }

    if (divoom_buddy_config_json_is_number(node))
    {
        *out_min = (float)node->valuedouble;
        *out_max = (float)node->valuedouble;
        *has = 1;
        return 0;
    }

    if (!divoom_buddy_config_json_is_array(node))
    {
        return -1;
    }

    if (divoom_cJSON_GetArraySize(node) < 2)
    {
        return -1;
    }

    a0 = divoom_cJSON_GetArrayItem(node, 0);
    a1 = divoom_cJSON_GetArrayItem(node, 1);
    if (!divoom_buddy_config_json_is_number(a0) || !divoom_buddy_config_json_is_number(a1))
    {
        return -1;
    }

    *out_min = (float)a0->valuedouble;
    *out_max = (float)a1->valuedouble;
    if (*out_min > *out_max)
    {
        float t = *out_min;
        *out_min = *out_max;
        *out_max = t;
    }

    *has = 1;
    return 0;
}

static int divoom_buddy_config_parse_string_array(divoom_cJSON *arr, char out[][DIVOOM_BUDDY_FRAME_NAME_MAX], int max_cnt)
{
    int i;
    int n;
    divoom_cJSON *el;

    if (!divoom_buddy_config_json_is_array(arr))
    {
        return 0;
    }

    n = divoom_cJSON_GetArraySize(arr);
    if (n > max_cnt)
    {
        n = max_cnt;
    }

    for (i = 0; i < n; i++)
    {
        el = divoom_cJSON_GetArrayItem(arr, i);
        if (!divoom_buddy_config_json_is_string(el))
        {
            continue;
        }

        strncpy(out[i], el->valuestring, DIVOOM_BUDDY_FRAME_NAME_MAX - 1);
        out[i][DIVOOM_BUDDY_FRAME_NAME_MAX - 1] = '\0';
    }

    return n;
}

static int divoom_buddy_config_parse_int_array(divoom_cJSON *arr, int *out, int max_cnt)
{
    int i;
    int n;
    divoom_cJSON *el;

    if (!divoom_buddy_config_json_is_array(arr))
    {
        return 0;
    }

    n = divoom_cJSON_GetArraySize(arr);
    if (n > max_cnt)
    {
        n = max_cnt;
    }

    for (i = 0; i < n; i++)
    {
        el = divoom_cJSON_GetArrayItem(arr, i);
        if (!divoom_buddy_config_json_is_number(el))
        {
            out[i] = 0;
        }
        else
        {
            out[i] = el->valueint;
        }
    }

    return n;
}

static int divoom_buddy_config_parse_pool(divoom_cJSON *obj, divoom_buddy_pool_opt_t *opts, int max_cnt,
    int *out_cnt, int *refresh_min, int *refresh_max)
{
    divoom_cJSON *options;
    divoom_cJSON *refresh;
    divoom_cJSON *item;
    divoom_cJSON *clip;
    divoom_cJSON *w;
    int i;
    int n;

    *out_cnt = 0;
    *refresh_min = 15000;
    *refresh_max = 45000;

    if (!obj || !(obj->type & divoom_cJSON_Object))
    {
        return -1;
    }

    refresh = divoom_cJSON_GetObjectItem(obj, "refresh_ms");
    if (refresh)
    {
        divoom_buddy_config_parse_int_range(refresh, refresh_min, refresh_max);
    }

    options = divoom_cJSON_GetObjectItem(obj, "options");
    if (!divoom_buddy_config_json_is_array(options))
    {
        return -1;
    }

    n = divoom_cJSON_GetArraySize(options);
    if (n > max_cnt)
    {
        n = max_cnt;
    }

    for (i = 0; i < n; i++)
    {
        item = divoom_cJSON_GetArrayItem(options, i);
        if (!item || !(item->type & divoom_cJSON_Object))
        {
            continue;
        }

        clip = divoom_cJSON_GetObjectItem(item, "clip");
        w = divoom_cJSON_GetObjectItem(item, "w");
        if (!divoom_buddy_config_json_is_string(clip))
        {
            continue;
        }

        strncpy(opts[*out_cnt].clip, clip->valuestring, DIVOOM_BUDDY_ID_MAX - 1);
        opts[*out_cnt].clip[DIVOOM_BUDDY_ID_MAX - 1] = '\0';
        opts[*out_cnt].weight = divoom_buddy_config_json_is_number(w) ? w->valueint : 1;
        if (opts[*out_cnt].weight <= 0)
        {
            opts[*out_cnt].weight = 1;
        }

        divoom_buddy_config_parse_float_range(divoom_cJSON_GetObjectItem(item, "speed"),
            &opts[*out_cnt].speed_min, &opts[*out_cnt].speed_max, &opts[*out_cnt].has_speed);
        (*out_cnt)++;
    }

    return (*out_cnt > 0) ? 0 : -1;
}

static int divoom_buddy_config_parse_pool_or_single(divoom_cJSON *obj, divoom_buddy_pool_opt_t *opts, int max_cnt,
    int *out_cnt, int *refresh_min, int *refresh_max, int default_min, int default_max)
{
    divoom_cJSON *clip;
    divoom_cJSON *refresh;

    if (divoom_buddy_config_parse_pool(obj, opts, max_cnt, out_cnt, refresh_min, refresh_max) == 0)
    {
        return 0;
    }

    *out_cnt = 0;
    *refresh_min = default_min;
    *refresh_max = default_max;

    if (!obj || !(obj->type & divoom_cJSON_Object))
    {
        return -1;
    }

    refresh = divoom_cJSON_GetObjectItem(obj, "refresh_ms");
    if (refresh)
    {
        divoom_buddy_config_parse_int_range(refresh, refresh_min, refresh_max);
    }

    clip = divoom_cJSON_GetObjectItem(obj, "clip");
    if (!divoom_buddy_config_json_is_string(clip))
    {
        return -1;
    }

    strncpy(opts[0].clip, clip->valuestring, DIVOOM_BUDDY_ID_MAX - 1);
    opts[0].clip[DIVOOM_BUDDY_ID_MAX - 1] = '\0';
    opts[0].weight = 1;
    *out_cnt = 1;
    return 0;
}

static int divoom_buddy_config_parse_clip_json(divoom_cJSON *root, divoom_buddy_clip_cfg_t *out)
{
    divoom_cJSON *node;
    divoom_cJSON *layer_pos;
    divoom_cJSON *pos_item;
    divoom_cJSON *x;
    divoom_cJSON *y;
    divoom_cJSON *pose;
    int i;

    memset(out, 0, sizeof(*out));

    node = divoom_cJSON_GetObjectItem(root, "id");
    if (divoom_buddy_config_json_is_string(node))
    {
        strncpy(out->id, node->valuestring, DIVOOM_BUDDY_ID_MAX - 1);
    }

    node = divoom_cJSON_GetObjectItem(root, "layer");
    if (!divoom_buddy_config_json_is_string(node))
    {
        return -1;
    }

    strncpy(out->layer_id, node->valuestring, DIVOOM_BUDDY_ID_MAX - 1);

    node = divoom_cJSON_GetObjectItem(root, "frames");
    out->frame_count = divoom_buddy_config_parse_string_array(node, out->frames, DIVOOM_BUDDY_MAX_FRAMES);
    if (out->frame_count <= 0)
    {
        return -1;
    }

    node = divoom_cJSON_GetObjectItem(root, "frame_delays_ms");
    if (node)
    {
        int n = divoom_buddy_config_parse_int_array(node, out->frame_delays_ms, DIVOOM_BUDDY_MAX_FRAMES);
        if (n == out->frame_count)
        {
            out->has_frame_delays = 1;
        }
    }

    node = divoom_cJSON_GetObjectItem(root, "fps");
    if (node)
    {
        if (divoom_buddy_config_json_is_number(node))
        {
            out->fps = node->valueint;
        }
        else if (divoom_buddy_config_json_is_array(node))
        {
            out->fps_is_range = 1;
            divoom_buddy_config_parse_int_range(node, &out->fps_min, &out->fps_max);
        }
    }

    node = divoom_cJSON_GetObjectItem(root, "loop");
    if (node && (node->type & divoom_cJSON_True))
    {
        out->loop = 1;
    }

    pose = divoom_cJSON_GetObjectItem(root, "pose");
    if (divoom_buddy_config_json_is_string(pose))
    {
        strncpy(out->pose_id, pose->valuestring, DIVOOM_BUDDY_ID_MAX - 1);
        out->pose_id[DIVOOM_BUDDY_ID_MAX - 1] = '\0';
        divoom_buddy_config_apply_pose_layer_pos(out, out->pose_id);
    }

    layer_pos = divoom_cJSON_GetObjectItem(root, "layer_pos");
    if (layer_pos && (layer_pos->type & divoom_cJSON_Object))
    {
        out->has_layer_pos = 1;
        for (i = 0; i < g_buddy_layers_cache.layer_count && i < DIVOOM_BUDDY_MAX_LAYERS; i++)
        {
            if (g_buddy_layers_loaded == 0)
            {
                break;
            }

            pos_item = divoom_cJSON_GetObjectItem(layer_pos, g_buddy_layers_cache.layers[i].id);
            if (!pos_item || !(pos_item->type & divoom_cJSON_Object))
            {
                continue;
            }

            x = divoom_cJSON_GetObjectItem(pos_item, "x");
            y = divoom_cJSON_GetObjectItem(pos_item, "y");
            if (!divoom_buddy_config_json_is_number(x) || !divoom_buddy_config_json_is_number(y))
            {
                continue;
            }

            out->layer_pos_x[i] = x->valueint;
            out->layer_pos_y[i] = y->valueint;
            out->layer_pos_mask |= (uint8)(1u << i);
        }
    }

    return 0;
}

static divoom_buddy_clip_cache_entry_t *divoom_buddy_config_clip_cache_find(const char *clip_id)
{
    for (unsigned int i = 0; i < g_buddy_clip_cache_count; i++)
    {
        if (strcmp(g_buddy_clip_cache[i]->id, clip_id) == 0)
        {
            return g_buddy_clip_cache[i];
        }
    }

    return NULL;
}

static divoom_buddy_clip_cache_entry_t *divoom_buddy_config_clip_cache_alloc(const char *clip_id)
{
    if (!g_buddy_cache_users || g_buddy_clip_cache_count >= DIVOOM_BUDDY_CLIP_CACHE_MAX)
    {
        return NULL;
    }

    divoom_buddy_clip_cache_entry_t *slot = divoom_system_malloc(sizeof(*slot));
    if (slot)
    {
        snprintf(slot->id, sizeof(slot->id), "%s", clip_id);
        g_buddy_clip_cache[g_buddy_clip_cache_count++] = slot;
    }
    return slot;
}

static divoom_buddy_state_cache_entry_t *divoom_buddy_config_state_cache_find(const char *state_id)
{
    for (unsigned int i = 0; i < g_buddy_state_cache_count; i++)
    {
        if (strcmp(g_buddy_state_cache[i]->id, state_id) == 0)
        {
            return g_buddy_state_cache[i];
        }
    }

    return NULL;
}

static divoom_buddy_state_cache_entry_t *divoom_buddy_config_state_cache_alloc(const char *state_id)
{
    if (!g_buddy_cache_users || g_buddy_state_cache_count >= DIVOOM_BUDDY_STATE_CACHE_MAX)
    {
        return NULL;
    }

    divoom_buddy_state_cache_entry_t *slot = divoom_system_malloc(sizeof(*slot));
    if (slot)
    {
        snprintf(slot->id, sizeof(slot->id), "%s", state_id);
        g_buddy_state_cache[g_buddy_state_cache_count++] = slot;
    }
    return slot;
}

void divoom_buddy_config_retain(void)
{
    g_buddy_cache_users++;
}

void divoom_buddy_config_release(void)
{
    if (g_buddy_cache_users && --g_buddy_cache_users == 0)
    {
        divoom_buddy_config_invalidate();
    }
}

void divoom_buddy_config_invalidate(void)
{
    while (g_buddy_clip_cache_count)
    {
        divoom_system_free(g_buddy_clip_cache[--g_buddy_clip_cache_count]);
        g_buddy_clip_cache[g_buddy_clip_cache_count] = NULL;
    }
    while (g_buddy_state_cache_count)
    {
        divoom_system_free(g_buddy_state_cache[--g_buddy_state_cache_count]);
        g_buddy_state_cache[g_buddy_state_cache_count] = NULL;
    }
    g_buddy_layers_loaded = 0;
}

void divoom_buddy_config_set_dir(const char *cfg_dir)
{
    char old_dir[sizeof(g_buddy_cfg_dir)];

    strncpy(old_dir, g_buddy_cfg_dir, sizeof(old_dir) - 1);
    old_dir[sizeof(old_dir) - 1] = '\0';

    if (!cfg_dir || cfg_dir[0] == '\0')
    {
        strncpy(g_buddy_cfg_dir, DIVOOM_BUDDY_CFG_DIR_DEFAULT, sizeof(g_buddy_cfg_dir) - 1);
    }
    else
    {
        strncpy(g_buddy_cfg_dir, cfg_dir, sizeof(g_buddy_cfg_dir) - 1);
    }
    g_buddy_cfg_dir[sizeof(g_buddy_cfg_dir) - 1] = '\0';

    if (strcmp(old_dir, g_buddy_cfg_dir) != 0)
    {
        divoom_buddy_config_invalidate();
    }
}

const char *divoom_buddy_config_get_dir(void)
{
    return g_buddy_cfg_dir;
}

int divoom_buddy_config_find_layer_index(const divoom_buddy_layers_cfg_t *layers, const char *layer_id)
{
    int i;

    if (!layers || !layer_id)
    {
        return -1;
    }

    for (i = 0; i < layers->layer_count; i++)
    {
        if (strcmp(layers->layers[i].id, layer_id) == 0)
        {
            return i;
        }
    }

    return -1;
}

int divoom_buddy_config_load_layers(divoom_buddy_layers_cfg_t *out)
{
    char rel[128];
    char *txt;
    divoom_cJSON *root;
    divoom_cJSON *region;
    divoom_cJSON *layers;
    divoom_cJSON *item;
    divoom_cJSON *node;
    int i;
    int n;

    if (!out)
    {
        return -1;
    }

    if (g_buddy_layers_loaded)
    {
        memcpy(out, &g_buddy_layers_cache, sizeof(*out));
        return 0;
    }

    if (divoom_buddy_config_is_source_sit())
    {
        divoom_buddy_source_sit_load_layers(out);
        memcpy(&g_buddy_layers_cache, out, sizeof(g_buddy_layers_cache));
        g_buddy_layers_loaded = 1;
        return 0;
    }

    divoom_buddy_config_build_rel(rel, sizeof(rel), "buddy_layers.json");
    txt = divoom_buddy_config_read_text(rel);
    if (!txt)
    {
        divoom_print("[buddy_cfg] missing layers json: %s\n", rel);
        return -1;
    }

    root = divoom_cJSON_Parse(txt);
    divoom_system_free(txt);
    if (!root)
    {
        return -1;
    }

    memset(out, 0, sizeof(*out));
    region = divoom_cJSON_GetObjectItem(root, "region");
    if (region && (region->type & divoom_cJSON_Object))
    {
        node = divoom_cJSON_GetObjectItem(region, "w");
        if (divoom_buddy_config_json_is_number(node))
        {
            out->region_w = node->valueint;
        }

        node = divoom_cJSON_GetObjectItem(region, "h");
        if (divoom_buddy_config_json_is_number(node))
        {
            out->region_h = node->valueint;
        }
    }

    layers = divoom_cJSON_GetObjectItem(root, "layers");
    if (!divoom_buddy_config_json_is_array(layers))
    {
        divoom_cJSON_Delete(root);
        return -1;
    }

    n = divoom_cJSON_GetArraySize(layers);
    if (n > DIVOOM_BUDDY_MAX_LAYERS)
    {
        n = DIVOOM_BUDDY_MAX_LAYERS;
    }

    for (i = 0; i < n; i++)
    {
        item = divoom_cJSON_GetArrayItem(layers, i);
        if (!item || !(item->type & divoom_cJSON_Object))
        {
            continue;
        }

        node = divoom_cJSON_GetObjectItem(item, "id");
        if (divoom_buddy_config_json_is_string(node))
        {
            strncpy(out->layers[out->layer_count].id, node->valuestring, DIVOOM_BUDDY_ID_MAX - 1);
        }

        node = divoom_cJSON_GetObjectItem(item, "z");
        out->layers[out->layer_count].z = divoom_buddy_config_json_is_number(node) ? node->valueint : 0;

        node = divoom_cJSON_GetObjectItem(item, "x");
        out->layers[out->layer_count].x = divoom_buddy_config_json_is_number(node) ? node->valueint : 0;

        node = divoom_cJSON_GetObjectItem(item, "y");
        out->layers[out->layer_count].y = divoom_buddy_config_json_is_number(node) ? node->valueint : 0;
        out->layer_count++;
    }

    divoom_cJSON_Delete(root);
    memcpy(&g_buddy_layers_cache, out, sizeof(g_buddy_layers_cache));
    g_buddy_layers_loaded = 1;
    return 0;
}

int divoom_buddy_config_get_clip(const char *clip_id, divoom_buddy_clip_cfg_t *out)
{
    char rel[160];
    char *txt;
    divoom_cJSON *root;
    divoom_buddy_clip_cache_entry_t *slot;

    if (!clip_id || !out || clip_id[0] == '\0')
    {
        return -1;
    }

    if (divoom_buddy_config_is_source_sit())
    {
        return divoom_buddy_source_sit_get_clip(clip_id, out);
    }

    slot = divoom_buddy_config_clip_cache_find(clip_id);
    if (slot)
    {
        memcpy(out, &slot->clip, sizeof(*out));
        return 0;
    }

    divoom_buddy_config_build_rel(rel, sizeof(rel), "clips/");
    strncat(rel, clip_id, sizeof(rel) - strlen(rel) - 1);
    strncat(rel, ".json", sizeof(rel) - strlen(rel) - 1);

    txt = divoom_buddy_config_read_text(rel);
    if (!txt)
    {
        divoom_print("[buddy_cfg] missing clip json: %s\n", rel);
        return -1;
    }

    root = divoom_cJSON_Parse(txt);
    divoom_system_free(txt);
    if (!root)
    {
        return -1;
    }

    if (divoom_buddy_config_parse_clip_json(root, out) != 0)
    {
        divoom_cJSON_Delete(root);
        return -1;
    }

    divoom_cJSON_Delete(root);

    slot = divoom_buddy_config_clip_cache_alloc(clip_id);
    if (!slot)
    {
        return -1;
    }

    memcpy(&slot->clip, out, sizeof(slot->clip));
    return 0;
}

int divoom_buddy_config_get_state(const char *state_id, divoom_buddy_state_cfg_t *out)
{
    char rel[160];
    char *txt;
    divoom_cJSON *root;
    divoom_cJSON *node;
    divoom_cJSON *micro_arr;
    divoom_cJSON *micro_item;
    divoom_cJSON *clip;
    divoom_cJSON *chance;
    divoom_cJSON *once;
    divoom_buddy_state_cache_entry_t *slot;
    int i;
    int n;

    if (!state_id || !out || state_id[0] == '\0')
    {
        return -1;
    }

    if (divoom_buddy_config_is_source_sit())
    {
        return divoom_buddy_source_sit_get_state(state_id, out);
    }

    slot = divoom_buddy_config_state_cache_find(state_id);
    if (slot)
    {
        memcpy(out, &slot->state, sizeof(*out));
        return 0;
    }

    divoom_buddy_config_build_rel(rel, sizeof(rel), "states/");
    strncat(rel, state_id, sizeof(rel) - strlen(rel) - 1);
    strncat(rel, ".json", sizeof(rel) - strlen(rel) - 1);

    txt = divoom_buddy_config_read_text(rel);
    if (!txt)
    {
        /*
         * 旧狗狗表盘保留内置 state 兜底；外部角色目录必须读取自己的 JSON。
         * 否则 1721 妈妈会拿到狗狗 idle_life，随后去 mom/clips 查狗狗 clip 而显示失败。
         */
        if (divoom_buddy_config_is_default_dir() &&
            divoom_buddy_v2_get_builtin_state(state_id, out) == 0)
        {
            return 0;
        }

        divoom_print("[buddy_cfg] missing state json: %s\n", rel);
        return -1;
    }

    root = divoom_cJSON_Parse(txt);
    divoom_system_free(txt);
    if (!root)
    {
        return -1;
    }

    memset(out, 0, sizeof(*out));
    strncpy(out->id, state_id, DIVOOM_BUDDY_ID_MAX - 1);

    node = divoom_cJSON_GetObjectItem(root, "expression");
    if (!node)
    {
        node = divoom_cJSON_GetObjectItem(root, "runtime_expression");
    }
    if (divoom_buddy_config_parse_pool_or_single(node, out->expression_opts, DIVOOM_BUDDY_MAX_POOL_OPTS,
            &out->expression_opt_count, &out->expression_refresh_min_ms, &out->expression_refresh_max_ms,
            15000, 45000) != 0)
    {
        divoom_cJSON_Delete(root);
        return -1;
    }

    node = divoom_cJSON_GetObjectItem(root, "base_action");
    if (divoom_buddy_config_parse_pool_or_single(node, out->base_action_opts, DIVOOM_BUDDY_MAX_POOL_OPTS,
            &out->base_action_opt_count, &out->base_action_refresh_min_ms, &out->base_action_refresh_max_ms,
            60000, 120000) != 0)
    {
        divoom_cJSON_Delete(root);
        return -1;
    }

    micro_arr = divoom_cJSON_GetObjectItem(root, "micro_actions");
    if (divoom_buddy_config_json_is_array(micro_arr))
    {
        n = divoom_cJSON_GetArraySize(micro_arr);
        if (n > DIVOOM_BUDDY_MAX_MICRO)
        {
            n = DIVOOM_BUDDY_MAX_MICRO;
        }

        for (i = 0; i < n; i++)
        {
            micro_item = divoom_cJSON_GetArrayItem(micro_arr, i);
            if (!micro_item || !(micro_item->type & divoom_cJSON_Object))
            {
                continue;
            }

            clip = divoom_cJSON_GetObjectItem(micro_item, "clip");
            if (!divoom_buddy_config_json_is_string(clip))
            {
                continue;
            }

            strncpy(out->micro_actions[out->micro_action_count].clip, clip->valuestring, DIVOOM_BUDDY_ID_MAX - 1);
            divoom_buddy_config_parse_int_range(divoom_cJSON_GetObjectItem(micro_item, "interval_ms"),
                &out->micro_actions[out->micro_action_count].interval_min_ms,
                &out->micro_actions[out->micro_action_count].interval_max_ms);
            chance = divoom_cJSON_GetObjectItem(micro_item, "chance");
            out->micro_actions[out->micro_action_count].chance =
                divoom_buddy_config_json_is_number(chance) ? (float)chance->valuedouble : 1.0f;
            once = divoom_cJSON_GetObjectItem(micro_item, "once");
            out->micro_actions[out->micro_action_count].once_per_mode =
                (once && (once->type & divoom_cJSON_True)) ? 1 : 0;
            divoom_buddy_config_parse_float_range(divoom_cJSON_GetObjectItem(micro_item, "speed"),
                &out->micro_actions[out->micro_action_count].speed_min,
                &out->micro_actions[out->micro_action_count].speed_max,
                &out->micro_actions[out->micro_action_count].has_speed);
            out->micro_action_count++;
        }
    }

    divoom_cJSON_Delete(root);

    slot = divoom_buddy_config_state_cache_alloc(state_id);
    if (!slot)
    {
        return -1;
    }

    memcpy(&slot->state, out, sizeof(slot->state));
    return 0;
}
