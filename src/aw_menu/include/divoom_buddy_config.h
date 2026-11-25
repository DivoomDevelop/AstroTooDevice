#ifndef _divoom_buddy_config_h_
#define _divoom_buddy_config_h_

#include "divoom_type.h"

#define DIVOOM_BUDDY_CFG_DIR_DEFAULT        "buddy/"
#define DIVOOM_BUDDY_MAX_LAYERS             8
#define DIVOOM_BUDDY_MAX_FRAMES             16
#define DIVOOM_BUDDY_MAX_POOL_OPTS          8
#define DIVOOM_BUDDY_MAX_MICRO              8
#define DIVOOM_BUDDY_ID_MAX                 48
#define DIVOOM_BUDDY_FRAME_NAME_MAX         128

typedef struct
{
    char id[DIVOOM_BUDDY_ID_MAX];
    int z;
    int x;
    int y;
} divoom_buddy_layer_def_t;

typedef struct
{
    int region_w;
    int region_h;
    int layer_count;
    divoom_buddy_layer_def_t layers[DIVOOM_BUDDY_MAX_LAYERS];
} divoom_buddy_layers_cfg_t;

typedef struct
{
    char id[DIVOOM_BUDDY_ID_MAX];
    char layer_id[DIVOOM_BUDDY_ID_MAX];
    char pose_id[DIVOOM_BUDDY_ID_MAX];
    char frames[DIVOOM_BUDDY_MAX_FRAMES][DIVOOM_BUDDY_FRAME_NAME_MAX];
    int frame_count;
    int frame_delays_ms[DIVOOM_BUDDY_MAX_FRAMES];
    uint8 has_frame_delays;
    int fps;
    int fps_min;
    int fps_max;
    uint8 fps_is_range;
    uint8 loop;
    uint8 has_layer_pos;
    int layer_pos_x[DIVOOM_BUDDY_MAX_LAYERS];
    int layer_pos_y[DIVOOM_BUDDY_MAX_LAYERS];
    uint8 layer_pos_mask;
} divoom_buddy_clip_cfg_t;

typedef struct
{
    char clip[DIVOOM_BUDDY_ID_MAX];
    int weight;
    float speed_min;
    float speed_max;
    uint8 has_speed;
} divoom_buddy_pool_opt_t;

typedef struct
{
    char clip[DIVOOM_BUDDY_ID_MAX];
    int interval_min_ms;
    int interval_max_ms;
    float chance;
    float speed_min;
    float speed_max;
    uint8 has_speed;
    uint8 once_per_mode; /* 场景进入后只执行一次，不在本 mode 内重新排期。 */
} divoom_buddy_micro_def_t;

typedef struct
{
    char id[DIVOOM_BUDDY_ID_MAX];
    divoom_buddy_pool_opt_t expression_opts[DIVOOM_BUDDY_MAX_POOL_OPTS];
    int expression_opt_count;
    int expression_refresh_min_ms;
    int expression_refresh_max_ms;
    divoom_buddy_pool_opt_t base_action_opts[DIVOOM_BUDDY_MAX_POOL_OPTS];
    int base_action_opt_count;
    int base_action_refresh_min_ms;
    int base_action_refresh_max_ms;
    divoom_buddy_micro_def_t micro_actions[DIVOOM_BUDDY_MAX_MICRO];
    int micro_action_count;
} divoom_buddy_state_cfg_t;

/* Active widgets share parsed entries under the LVGL lock. */
void divoom_buddy_config_retain(void);
void divoom_buddy_config_release(void);
/* Clear parsed caches when a new package takes ownership of ato/. */
void divoom_buddy_config_invalidate(void);
void divoom_buddy_config_set_dir(const char *cfg_dir);
const char *divoom_buddy_config_get_dir(void);
int divoom_buddy_config_load_layers(divoom_buddy_layers_cfg_t *out);
int divoom_buddy_config_get_clip(const char *clip_id, divoom_buddy_clip_cfg_t *out);
int divoom_buddy_config_get_state(const char *state_id, divoom_buddy_state_cfg_t *out);
int divoom_buddy_config_find_layer_index(const divoom_buddy_layers_cfg_t *layers, const char *layer_id);

#endif
