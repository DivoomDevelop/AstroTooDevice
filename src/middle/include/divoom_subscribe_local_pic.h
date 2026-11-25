#ifndef _divoom_subscribe_local_pic_h_
#define _divoom_subscribe_local_pic_h_

#include "divoom_type.h"

/*
 * Load local subscribe pics from divoom model_id bin files when DB is empty.
 * default_user_id is written to user_id when non-zero (expert subscribe).
 */
int divoom_subscribe_local_pic_load(
    int model_id,
    divoom_image_base_info *list,
    int *num,
    int max_num,
    uint32 default_user_id);

/* 深拷贝作者名；返回缓冲区由调用方使用 divoom_system_free 释放。 */
char *divoom_subscribe_local_pic_copy_image_name(const char *image_name);

/* 按需扩容公共图片列表；已有 image_name 指针的所有权保持不变。 */
int32 divoom_subscribe_local_pic_reserve_image_list(
    Pdivoom_image_hot_item *pimage_list,
    int32 image_num,
    int32 *pimage_capacity,
    int32 required_num);

/* 向公共图片列表追加四字段元数据，image_name 会被深拷贝。 */
int32 divoom_subscribe_local_pic_append_image(
    Pdivoom_image_hot_item *pimage_list,
    int32 *pimage_num,
    int32 *pimage_capacity,
    uint32 image_size,
    uint32 image_id,
    const char *image_name,
    uint32 user_id);

/* 替换单项作者名；申请成功后才释放旧名称。 */
int32 divoom_subscribe_local_pic_replace_image_name(
    Pdivoom_image_hot_item pimage_item, const char *image_name);

/* 深度释放公共图片列表及其中逐项动态申请的作者名。 */
void divoom_subscribe_local_pic_free_image_list(
    Pdivoom_image_hot_item image_list, int32 image_num);

/* 释放并移除列表头部若干项；数组容量保持不变，供后续继续追加。 */
void divoom_subscribe_local_pic_remove_first_image(
    Pdivoom_image_hot_item image_list, int32 *pimage_num,
    int32 remove_num);

/* 数据库为空时按业务上限加载本地兜底图片，内存仍按实际数量申请。 */
Pdivoom_image_hot_item divoom_subscribe_local_pic_load_dynamic(
    int model_id, int32 *pimage_num, int32 max_num,
    uint32 default_user_id);

#endif
