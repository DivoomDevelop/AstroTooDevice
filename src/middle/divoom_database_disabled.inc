/* SQLite-disabled API: empty results, no persistence or callbacks.
 * Included only by divoom_database_manage_rtos_stub.c when SQLite is off.
 * Counts and output buffers are initialized; cleanup fails closed.
 */

int divoom_database_manage_clear_unused_subscribe_batch(int mode, uint32 *deleted_ids, int *deleted_num)
{
    if (deleted_num) *deleted_num = 0;
    return -1;
}

int divoom_database_manage_init(char *db_name)
{
    return 0;
}

int divoom_database_manage_fresh_db(void)
{
    return 0;
}

void divoom_database_manage_add_table_index(char *table_name, char *item_name)
{
    /* Disabled: intentionally no side effects. */
}

void divoom_database_manage_run_sql_command(char *cmd_name)
{
    /* Disabled: intentionally no side effects. */
}

int divoom_database_manage_add_new_table(char *table_name, char *table_item)
{
    return 0;
}

int divoom_database_manage_insert_new_item(char *table_name, char *item_data)
{
    return 0;
}

void divoom_database_manage_update_record(char *table_name, char *item_data)
{
    /* Disabled: intentionally no side effects. */
}

void divoom_database_manage_remove_record(char *table_name, int id)
{
    /* Disabled: intentionally no side effects. */
}

void divoom_database_manage_remove_table(char *table_name)
{
    /* Disabled: intentionally no side effects. */
}

void divoom_database_manage_close(char *db_name)
{
    /* Disabled: intentionally no side effects. */
}

int divoom_database_manage_check_count_err(char *cmd_info)
{
    return 0;
}

int divoom_database_manage_get_record(char *table_name, char *item_data,int id, DIVOOM_DATABASE_MANAGE_GET_DATA_CALLBACK *pcallback, void *para)
{
    return 0;
}

int divoom_database_manage_reset(char *db_name)
{
    return 0;
}

int divoom_database_manage_check_column_in_table(char *table_name, char *column_name, char *new_column_definition)
{
    return 0;
}

void divoom_database_manage_insert_album_item(int id, int type, char* album_name)
{
    /* Disabled: intentionally no side effects. */
}

void divoom_database_manage_update_album_name_record(int id, char *album_name)
{
    /* Disabled: intentionally no side effects. */
}

void divoom_database_manage_delete_album_table(int id)
{
    /* Disabled: intentionally no side effects. */
}

void divoom_database_manage_remove_album_record(int album_id, int pic_id)
{
    /* Disabled: intentionally no side effects. */
}

int divoom_database_manage_add_album_record(int album_id, int pic_id)
{
    return 0;
}

int divoom_database_manage_add_album_records_batch(int album_id,
    const uint32 *pic_list, int pic_num)
{
    return 0;
}

uint32 * divoom_database_manage_get_all_pic_from_album_table(int album_id, int *pic_num, int order_flag, int desc_flag, int like_flag, int hide_flag)
{
    if (pic_num) *pic_num = 0;
    return NULL;
}

int divoom_database_manage_get_pic_num_from_album_table(int album_id, int hide_flag)
{
    return 0;
}

Pdivoom_pic_album_manage_head_item divoom_database_manage_get_all_album_info(int *album_num)
{
    if (album_num) *album_num = 0;
    return NULL;
}

int divoom_database_manage_get_pic_info(int pic_id, void * pic_info)
{
    return 0;
}

int divoom_database_manage_get_pic_path_name(int pic_id, char *path_name)
{
    if (path_name) path_name[0] = '\0';
    return 0;
}

int divoom_database_manage_get_pic_id_from_path_name(char *path_name)
{
    return 0;
}

int divoom_database_manage_add_pic_record(double save_time, int photo_time, char *path_name, int photo_x, int photo_y, int user_id,char *pic_path, int video_flag, int width_flag)
{
    return 0;
}

int divoom_database_manage_get_pic_title(int pic_id, char *pic_title)
{
    if (pic_title) pic_title[0] = '\0';
    return 0;
}

void divoom_database_manage_update_pic_title(int pic_id, char *pic_title)
{
    /* Disabled: intentionally no side effects. */
}

void divoom_database_manage_update_pic_attr(Pdivoom_pic_phone_manage_item_offset pic_info, int attr_flag)
{
    /* Disabled: intentionally no side effects. */
}

void divoom_database_manage_delete_pic(int pic_id)
{
    /* Disabled: intentionally no side effects. */
}

void divoom_database_manage_reset_pic_info(void)
{
    /* Disabled: intentionally no side effects. */
}

uint32 * divoom_database_manage_get_album_pic_list(int album_id, int start,int *pic_num)
{
    if (pic_num) *pic_num = 0;
    return NULL;
}

int divoom_database_manage_insert_music_item(char *item_data, const char *root_name)
{
    return 0;
}

void divoom_database_manage_delete_music_item(int id)
{
    /* Disabled: intentionally no side effects. */
}

void divoom_database_manage_get_music_list(divoom_database_manage_get_music_list_callback *pcallback, void *para)
{
    /* Disabled: intentionally no side effects. */
}

int divoom_database_manage_clear_local_music_reset_id(void)
{
    return 0;
}

int divoom_database_manage_get_local_music_count(void)
{
    return 0;
}

void divoom_database_manage_txn_begin(void)
{
    /* Disabled: intentionally no side effects. */
}

void divoom_database_manage_txn_end(void)
{
    /* Disabled: intentionally no side effects. */
}

void divoom_database_manage_local_music_batch_begin(void)
{
    /* Disabled: intentionally no side effects. */
}

void divoom_database_manage_local_music_batch_end(void)
{
    /* Disabled: intentionally no side effects. */
}

void divoom_database_manage_update_music_like(int id, int like_flag)
{
    /* Disabled: intentionally no side effects. */
}

int divoom_database_manage_get_user_define_last_update_time(void)
{
    return 0;
}

Pdivoom_image_hot_item divoom_database_manage_get_user_define_pic_list(int clock_id, int clock_pos, int parent_id, int *pic_num, char **pvoice_list)
{
    if (pic_num) *pic_num = 0;
    if (pvoice_list) *pvoice_list = NULL;
    return NULL;
}

int divoom_database_manage_get_user_define_pic_addr(int id, char *file_addr)
{
    if (file_addr) file_addr[0] = '\0';
    return 0;
}

void divoom_database_manage_update_user_define_pic_addr(int clock_id, int clock_pos, int parent_id, int id, char *file_addr, int update_time, int author_id, char *image_name, char *voice_id)
{
    /* Disabled: intentionally no side effects. */
}

void divoom_database_manage_delete_user_define_item(int clock_id, int clock_pos, int parent_id, int id, int update_time)
{
    /* Disabled: intentionally no side effects. */
}

void divoom_database_manage_insert_user_define_pic_item(int clock_id, int clock_pos, int parent_id, int id, char *file_addr, int update_time, int author_id, char *image_name, char *voice_id)
{
    /* Disabled: intentionally no side effects. */
}

void divoom_database_manage_empty_user_define_item(int clock_id, int clock_pos, int parent_id, int update_time)
{
    /* Disabled: intentionally no side effects. */
}

void divoom_database_manage_update_user_define_play_time(int clock_id, int clock_pos, int parent_id, int play_time, int show_time)
{
    /* Disabled: intentionally no side effects. */
}

int divoom_database_manage_get_user_define_play_time(int clock_id, int clock_pos, int parent_id, int *show_time)
{
    if (show_time) *show_time = 0;
    return 0;
}

void divoom_database_manage_update_user_define_update_time(int clock_id, int clock_pos, int parent_id, int update_time)
{
    /* Disabled: intentionally no side effects. */
}

int divoom_database_manage_get_subscribe_image_last_update_time(char *table_name, int image_type)
{
    return 0;
}

Pdivoom_image_hot_item divoom_database_manage_get_subscribe_image_pic_list(char *table_name, int image_type, int *pic_num)
{
    if (pic_num) *pic_num = 0;
    return NULL;
}

Pdivoom_image_hot_item divoom_database_manage_get_subscribe_image_pic_list_by_updatetime(char *table_name, int image_type, int *pic_num)
{
    if (pic_num) *pic_num = 0;
    return NULL;
}

void divoom_database_manage_delete_subscribe_image_item(char *table_name, int image_type, int image_id)
{
    /* Disabled: intentionally no side effects. */
}

void divoom_database_manage_delete_subscribe_image_all(char *table_name, int image_type)
{
    /* Disabled: intentionally no side effects. */
}

void divoom_database_manage_add_subscribe_image_item(char *table_name, int image_type, int image_id, int update_time, char *file_addr, int user_id, char *image_name)
{
    /* Disabled: intentionally no side effects. */
}

void divoom_database_manage_add_subscribe_image_item_with_size(char *table_name, int image_type, int image_size, int image_id, int update_time, char *file_addr, int user_id, char *image_name)
{
    /* Disabled: intentionally no side effects. */
}

void divoom_database_manage_update_subscribe_image_name_by_user_id(char *table_name, int user_id, char *image_name)
{
    /* Disabled: intentionally no side effects. */
}

int divoom_database_manage_get_subscribe_image_pic_addr(char *table_name, int image_id, char *file_addr)
{
    if (file_addr) file_addr[0] = '\0';
    return 0;
}

int * divoom_database_manage_get_divoom_image_image_type_list(char *table_name, int *list_num)
{
    if (list_num) *list_num = 0;
    return NULL;
}

int divoom_database_manage_check_divoom_image_config_exist(char *table_name, int clock_id, int clock_pos, int parent_id, int image_type)
{
    return 0;
}

int divoom_database_manage_check_divoom_image_type_exist(char *table_name, int image_type)
{
    return 0;
}

int divoom_database_manage_add_divoom_image_config_item(char *table_name, int clock_id, int clock_pos, int parent_id, int image_type)
{
    return 0;
}

int divoom_database_manage_update_divoom_image_config_item(char *table_name, int clock_id, int clock_pos, int parent_id, int image_type)
{
    return 0;
}

void divoom_database_manage_delete_divoom_image_config_item(char *table_name, int clock_id, int clock_pos, int parent_id, int image_type)
{
    /* Disabled: intentionally no side effects. */
}

void divoom_database_manage_clear_all_divoom_image(char *table_name)
{
    /* Disabled: intentionally no side effects. */
}

int divoom_database_manage_update_subscribe_config_record(char *table_name, int clock_id, int clock_pos, int parent_id, Pdivoom_disp_subscribe_data pinfo)
{
    return 0;
}

int divoom_database_manage_update_subscribe_play_time_record(char *table_name, int clock_id, int clock_pos, int parent_id, int play_time, int show_time)
{
    return 0;
}

int divoom_database_manage_get_subscribe_config_record(char *table_name, int clock_id, int clock_pos, int parent_id, Pdivoom_disp_subscribe_data pinfo)
{
    if (pinfo) memset(pinfo, 0, sizeof(*pinfo));
    return 0;
}

int divoom_database_manage_get_subscribe_config_update_time(char *table_name)
{
    return 0;
}

int divoom_database_manage_get_item_update_time(char *table_name, int id)
{
    return 0;
}

void * divoom_database_manage_get_eq_image_list(char *table_name,int clock_id, int clock_pos, int parent_id, int *list_num)
{
    if (list_num) *list_num = 0;
    return NULL;
}

int divoom_database_manage_add_eq_image_item(char *table_name,int clock_id, int clock_pos, int parent_id, int update_time, char *file_id, int user_id, char *image_name)
{
    return 0;
}

void divoom_database_manage_get_eq_image_addr(char *table_name,int image_id, char *file_addr)
{
    if (file_addr) file_addr[0] = '\0';
}

void divoom_database_manage_delete_eq_image_item(char *table_name,int image_id)
{
    /* Disabled: intentionally no side effects. */
}

int divoom_database_manage_get_eq_mode_index(char *table_name,int clock_id, int clock_pos, int parent_id)
{
    return 0;
}

void divoom_database_manage_update_eq_mode_index(char *table_name,int clock_id, int clock_pos, int parent_id, int mode)
{
    /* Disabled: intentionally no side effects. */
}

void divoom_database_manage_set_subscribe_config_update_time(char *table_name, int id, int update_time)
{
    /* Disabled: intentionally no side effects. */
}

uint32 * divoom_database_manage_get_user_head_list(int *user_num)
{
    if (user_num) *user_num = 0;
    return NULL;
}

void divoom_database_manage_insert_user_head_item(int user_id, char *user_name, char *file_id)
{
    /* Disabled: intentionally no side effects. */
}

void divoom_database_manage_update_user_head_item(int user_id, char *user_name, char *file_id)
{
    /* Disabled: intentionally no side effects. */
}

void divoom_database_manage_update_user_head_time(int user_id)
{
    /* Disabled: intentionally no side effects. */
}

int divoom_database_manage_get_user_head_item(int user_id, char *user_name, char *file_id)
{
    if (user_name) user_name[0] = '\0';
    if (file_id) file_id[0] = '\0';
    return 0;
}

int divoom_database_manage_check_album_include_pic(int album_id,int pic_id)
{
    return 0;
}

Pdivoom_pic_id_name_info divoom_database_manage_get_all_pic_id_and_name(int album_id, int *pic_num, int num, int order_flag, int desc_flag, double start_time, int start_id)
{
    if (pic_num) *pic_num = 0;
    return NULL;
}

int divoom_database_manage_get_first_pic_id(int album_id)
{
    return 0;
}

int divoom_database_manage_get_disp_order_pic_id(int album_id, int order_flag, int reverse_flag, int movie_flag, int pos)
{
    return 0;
}

int divoom_database_manage_get_disp_order_pic_pos(int album_id, int order_flag, int reverse_flag, int movie_flag, int pic_id)
{
    return -1;
}

int divoom_database_manage_get_total_pic_num(int album_id, int order_flag, int reverse_flag, double save_time, int pic_flag, int movie_flag)
{
    return 0;
}

uint32 * divoom_database_manage_get_all_pic_id(int album_id, int *pic_num, int order_flag, int desc_flag, double start_time, int pic_flag, int movie_flag)
{
    if (pic_num) *pic_num = 0;
    return NULL;
}

int divoom_database_manage_get_max_pic_id(void)
{
    return 0;
}

uint32 * divoom_database_manage_get_all_pic_id_reverse(int album_id, int *pic_num, int order_flag, int desc_flag, double start_time, int movie_flag)
{
    if (pic_num) *pic_num = 0;
    return NULL;
}

Pdivoom_pic_id_width_info divoom_database_manage_get_all_pic_id_width(int album_id, int *pic_num, int order_flag, int desc_flag, int pic_flag)
{
    if (pic_num) *pic_num = 0;
    return NULL;
}

int divoom_database_manage_insert_custom_photo_item(Pdivoom_clock_id_info clock_info, double save_time, int photo_time, char *file_name)
{
    return 0;
}

void divoom_database_manage_delete_custom_photo_item(int id)
{
    /* Disabled: intentionally no side effects. */
}

void divoom_database_manage_delete_custom_photo_all(Pdivoom_clock_id_info clock_info)
{
    /* Disabled: intentionally no side effects. */
}

uint32 * divoom_database_manage_get_custom_photo_list(Pdivoom_clock_id_info clock_info, int *pic_num, int order_flag, int order_reverse, double start_time)
{
    if (pic_num) *pic_num = 0;
    return NULL;
}

uint32 * divoom_database_manage_get_custom_photo_list_reverse(Pdivoom_clock_id_info clock_info, int *pic_num, int order_flag, int order_reverse, double start_time)
{
    if (pic_num) *pic_num = 0;
    return NULL;
}

Pdivoom_pic_id_name_info divoom_database_manage_get_custom_local_photo_list(Pdivoom_clock_id_info clock_info, int *pic_num,double start_time, int order_flag, int order_reverse)
{
    if (pic_num) *pic_num = 0;
    return NULL;
}

int divoom_database_manage_get_total_custom_pic_num(Pdivoom_clock_id_info clock_info, int order_flag, int reverse_flag, double save_time)
{
    return 0;
}

int divoom_database_manage_get_custom_photo_name(int id, char *file_name)
{
    if (file_name) file_name[0] = '\0';
    return 0;
}

int divoom_database_manage_get_custom_photo_info(int id, divoom_custom_photo_info *pic_info)
{
    if (pic_info) memset(pic_info, 0, sizeof(*pic_info));
    return 0;
}

void divoom_database_manage_reset_calendar_info(void)
{
    /* Disabled: intentionally no side effects. */
}

void divoom_database_manage_load_calendar_list_info(void *event_queue)
{
    /* Disabled: intentionally no side effects. */
}

void divoom_database_manage_update_calendar_item(void *data)
{
    /* Disabled: intentionally no side effects. */
}

int divoom_database_manage_insert_calendar_item(void *data)
{
    return 0;
}

void divoom_database_manage_remove_calendar_item(int calendar_id)
{
    /* Disabled: intentionally no side effects. */
}

void divoom_database_manage_update_event_item(int calendar_id, void *data)
{
    /* Disabled: intentionally no side effects. */
}

int divoom_database_manage_insert_event_item(int calendar_id, void *data)
{
    return 0;
}

void divoom_database_manage_remove_event_item(int calendar_id, char *id)
{
    /* Disabled: intentionally no side effects. */
}

void divoom_database_manage_remove_time_out_event(int start_utc)
{
    /* Disabled: intentionally no side effects. */
}

void divoom_database_manage_load_calendar_event_info(int calendar_id, void *event_queue, int start_utc, int end_utc)
{
    /* Disabled: intentionally no side effects. */
}

void divoom_database_manage_load_calendar_all_event_info(void **event_queue, int start_utc, int end_utc)
{
    /* Disabled: intentionally no side effects. */
}

int divoom_database_manage_insert_vision_photo_item(char *table_name, int vision_id, char *file_name)
{
    return 0;
}

int divoom_database_manage_insert_vision_photo_item_with_time(char *table_name,int vision_id, char *file_name, int update_time)
{
    return 0;
}

void divoom_database_manage_delete_vision_photo_item(char *table_name, int id)
{
    /* Disabled: intentionally no side effects. */
}

void divoom_database_manage_update_vision_photo_item_download(char *table_name,int id)
{
    /* Disabled: intentionally no side effects. */
}

Pdivoom_vision_item divoom_database_manage_get_vision_photo_list(char *table_name, int *pic_num)
{
    if (pic_num) *pic_num = 0;
    return NULL;
}

int divoom_database_manage_get_vision_photo_name(char *table_name,int vision,char *oss_name)
{
    if (oss_name) oss_name[0] = '\0';
    return 0;
}

int divoom_database_manage_get_vision_last_update_time(char *table_name)
{
    return 0;
}

int divoom_database_manage_update_divoom_image_config_update_time(char *table_name, int image_type,int update_time)
{
    return 0;
}

int divoom_database_manage_get_divoom_image_update_tiem(char *table_name, int image_type)
{
    return 0;
}

int divoom_database_manage_get_divoom_image_type_id(char *table_name, int clock_id, int clock_pos, int parent_id)
{
    return 0;
}

int divoom_database_tf_is_rebuilding(void)
{
    return 0;
}

int divoom_database_tf_dir_total(int folder_id)
{
    return 0;
}

int divoom_database_tf_dir_page(int folder_id, int offset, int limit, divoom_tf_entry *out, int out_cap)
{
    return 0;
}

int divoom_database_tf_subtree_count(int folder_id)
{
    return 0;
}

int divoom_database_tf_subtree_page(int folder_id, int offset, int limit, divoom_tf_entry *out, int out_cap)
{
    return 0;
}

int divoom_database_tf_folder_info(int folder_id, divoom_tf_folder_info *out)
{
    if (out) memset(out, 0, sizeof(*out));
    return 0;
}

int divoom_database_tf_folder_parent(int folder_id)
{
    return -1;
}

int divoom_database_tf_track_path(int track_id, char *buf, int buf_size)
{
    if (buf && buf_size > 0) buf[0] = '\0';
    return 0;
}

int divoom_database_tf_track_name(int track_id, char *buf, int buf_size)
{
    if (buf && buf_size > 0) buf[0] = '\0';
    return 0;
}

int divoom_database_tf_track_row(int folder_id, int track_id)
{
    return -1;
}

int divoom_database_tf_next_id(int cur_id)
{
    return 0;
}

int divoom_database_tf_prev_id(int cur_id)
{
    return 0;
}

int divoom_database_tf_random_id(int seed_id)
{
    return 0;
}

int divoom_database_tf_min_id(void)
{
    return 0;
}

int divoom_database_tf_max_id(void)
{
    return 0;
}

int divoom_database_tf_find_id_by_path(const char *music_path)
{
    return 0;
}

int divoom_database_tf_set_like(int track_id, int like_flag)
{
    return 0;
}

int divoom_database_tf_total(void)
{
    return 0;
}

int divoom_database_tf_like_count(void)
{
    return 0;
}

int divoom_database_tf_meta_get_u32(const char *key, uint32 *out)
{
    if (out) *out = 0;
    return 0;
}

int divoom_database_tf_meta_set_u32(const char *key, uint32 value)
{
    return 0;
}

int divoom_database_tf_rebuild_begin(void)
{
    return 0;
}

int divoom_database_tf_ensure_folder(int parent_id, const char *name, const char *path, int depth, int *out_id)
{
    if (out_id) *out_id = 0;
    return 0;
}

int divoom_database_tf_add_track(int track_id, int folder_id, const char *name)
{
    return 0;
}

int divoom_database_tf_rebuild_end(void)
{
    return 0;
}
