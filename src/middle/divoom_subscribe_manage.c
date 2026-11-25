#include "divoom.h"

#define DIVOOM_SUBSCRIBE_MANAGE_MORE_GIF_CLOCK_ID       250
#define DIVOOM_SUBSCRIBE_MANAGE_MORE_GIF_FILE_NUM       100
#define DIVOOM_SUBSCRIBE_MANAGE_MORE_GIF_PLAY_FILE_SIZE 256

#define DIVOOM_SUBSCRIBE_RELOAD_RESULT_IDLE     (-1)
#define DIVOOM_SUBSCRIBE_RELOAD_RESULT_WAITING    0
#define DIVOOM_SUBSCRIBE_RELOAD_RESULT_COMPLETE   1
#define DIVOOM_SUBSCRIBE_TRANSFER_REQUEST_MAX    64
#define DIVOOM_SUBSCRIBE_TRANSFER_REOPEN_WAIT_MS 5000

int divoom_hot_manage_request_hot_image(int image_size, int wait_flag);

typedef struct _divoom_subscribe_manage_item
{
    int use_time;
    int clock_id;
    int clock_pos;
    int parent_id;
    int expert_id;
    int refresh_disp;
    divoom_disp_subscribe_data subscribe_info;
}divoom_subscribe_manage_item, *Pdivoom_subscribe_manage_item;

typedef struct _divoom_subscribe_transfer_request
{
    uint16 command;
    uint32 packet_flag;
}divoom_subscribe_transfer_request;

typedef struct _divoom_subscribe_manage_ctrl
{
    int update_time;
    int load_update_time;
    int reload_command;
    int reload_expected_num;
    int reload_received_num;
    int32 reload_add_file_num;
    int reload_result_state;
    int transfer_request_num;
    divoom_subscribe_transfer_request
        transfer_request[DIVOOM_SUBSCRIBE_TRANSFER_REQUEST_MAX];
    void *lock_handle;
    void *info_queue;
}divoom_subscribe_manage_ctrl, *Pdivoom_subscribe_manage_ctrl;

Pdivoom_subscribe_manage_ctrl pdivoom_subscribe_manage_handle = NULL;
static volatile uint8 g_divoom_subscribe_update_active = 0;
static volatile uint8 g_divoom_subscribe_transfer_response_active = 0;
static volatile uint8 g_divoom_subscribe_transfer_dialog_notified = 0;
static volatile uint8 g_divoom_subscribe_transfer_response_deferred = 0;
static volatile uint8 g_divoom_subscribe_transfer_reopen_wait = 0;
static uint16 g_divoom_subscribe_transfer_response_command = 0;
static uint16 g_divoom_subscribe_transfer_reopen_command = 0;
static uint32 g_divoom_subscribe_transfer_reopen_tick = 0;

/*********************************************************************
  Function:        divoom_subscribe_manage_init
  Description:     初始化
  Input:           none
                    
  Return:          none
**********************************************************************/

void divoom_subscribe_manage_init(void)
{
	char *table_item = "id INTEGER PRIMARY KEY     AUTOINCREMENT," \
		"clock_id INTEGER DEFAULT 0," \
		"clock_pos INTEGER DEFAULT 0," \
		"parent_id INTEGER DEFAULT 0," \
		"update_time INTEGER DEFAULT 0," \
		"mode INTEGER DEFAULT 0," \
		"disp INTEGER DEFAULT 0," \
		"play_time INTEGER DEFAULT 0," \
		"show_time INTEGER DEFAULT 0," \
		"album_id INTEGER DEFAULT 0," \
		"play_id INTEGER DEFAULT 0," \
		"expert_num INTEGER DEFAULT 0," \
		"expert_list  TEXT ";

	divoom_database_manage_add_new_table(DIVOOM_SUBSCRIBE_IMAGE_TABLE_NAME, table_item);
    divoom_database_manage_check_column_in_table(DIVOOM_SUBSCRIBE_IMAGE_TABLE_NAME, "play_id", "INTEGER  DEFAULT 0");
    divoom_database_manage_check_column_in_table(DIVOOM_SUBSCRIBE_IMAGE_TABLE_NAME, "show_time", "INTEGER  DEFAULT 0");
    divoom_database_manage_add_table_index(DIVOOM_SUBSCRIBE_IMAGE_TABLE_NAME, "clock_id");
    divoom_database_manage_add_table_index(DIVOOM_SUBSCRIBE_IMAGE_TABLE_NAME, "clock_pos");
    divoom_database_manage_add_table_index(DIVOOM_SUBSCRIBE_IMAGE_TABLE_NAME, "parent_id");
    divoom_database_manage_add_table_index(DIVOOM_SUBSCRIBE_IMAGE_TABLE_NAME, "update_time");
    pdivoom_subscribe_manage_handle = divoom_system_malloc(sizeof(divoom_subscribe_manage_ctrl));
    pdivoom_subscribe_manage_handle->info_queue = divoom_queue_init(100, __func__, __LINE__);
    pdivoom_subscribe_manage_handle->lock_handle = divoom_system_create_lock();
    pdivoom_subscribe_manage_handle->load_update_time = -1;
    pdivoom_subscribe_manage_handle->reload_result_state =
        DIVOOM_SUBSCRIBE_RELOAD_RESULT_IDLE;
    pdivoom_subscribe_manage_handle->transfer_request_num = 0;
    pdivoom_subscribe_manage_handle->update_time = divoom_database_manage_get_subscribe_config_update_time(DIVOOM_SUBSCRIBE_IMAGE_TABLE_NAME);
}

/* Run before clock reclamation, without holding clock/list locks. */
int divoom_subscribe_manage_clear_unused_data_if_needed(uint32 *free_space)
{
    int mode, i, result, deleted_num, total = 0;
    uint32 active_modes = 0;
    uint32 deleted_ids[DIVOOM_SUBSCRIBE_CLEAN_BATCH_SIZE];
    Pdivoom_subscribe_manage_item item;

    if(free_space == NULL || *free_space >= DIVOOM_ROOM_DEL_CLOCK_DATA ||
        pdivoom_subscribe_manage_handle == NULL)
    {
        divoom_print("[storage_gc] subscribe_skip space_ok=%d ready=%d\n",
            free_space != NULL && *free_space >= DIVOOM_ROOM_DEL_CLOCK_DATA, pdivoom_subscribe_manage_handle != NULL);
        return 0;
    }
    divoom_system_get_lock(pdivoom_subscribe_manage_handle->lock_handle);
    for(i = 0; i < divoom_queue_items(pdivoom_subscribe_manage_handle->info_queue); i++)
    {
        item = divoom_queue_get_index_item(pdivoom_subscribe_manage_handle->info_queue, i);
        if(item == NULL || item->subscribe_info.mode > DIVOOM_DISP_SUBSCRIBE_MODE_PLAY_LIST)
        {
            divoom_system_free_lock(pdivoom_subscribe_manage_handle->lock_handle);
            return 0;
        }
        active_modes |= 1U << item->subscribe_info.mode;
    }
    divoom_print("[storage_gc] subscribe_live mask=0x%x\n", active_modes);
    for(mode = DIVOOM_DISP_SUBSCRIBE_MODE_HOT;
        mode <= DIVOOM_DISP_SUBSCRIBE_MODE_PLAY_LIST &&
            *free_space < DIVOOM_ROOM_DEL_CLOCK_DATA; mode++)
    {
        if(active_modes & (1U << mode))
        {
            divoom_print("[storage_gc] subscribe_skip mode=%d reason=live\n", mode);
            continue;
        }
        do
        {
            deleted_num = 0;
            if(mode == DIVOOM_DISP_SUBSCRIBE_MODE_HOT)
            {
                result = divoom_hot_manage_clear_unused_batch(&deleted_num);
            }
            else
            {
                result = divoom_database_manage_clear_unused_subscribe_batch(
                    mode, deleted_ids, &deleted_num);
            }
            divoom_print("[storage_gc] subscribe_batch mode=%d result=%d deleted=%d\n", mode, result, deleted_num);
            total += deleted_num;
            if(deleted_num > 0)
            {
                /* One expensive free-space scan per batch, including the last partial batch. */
                *free_space = (uint32)divoom_port_fs_free_space(DIVOOM_USER_ROOT);
                divoom_print("subscribe cleaned: mode=%d files=%d free=%uKB\n",
                    mode, deleted_num, *free_space);
            }
        } while(result == DIVOOM_SUBSCRIBE_CLEAN_BATCH_SIZE &&
            *free_space < DIVOOM_ROOM_DEL_CLOCK_DATA);
        /* On failure, leave this mode for retry and try other unused modes. */
    }
    divoom_system_free_lock(pdivoom_subscribe_manage_handle->lock_handle);
    return total;
}

static int divoom_subscribe_manage_is_transfer_command(int command)
{
    return command == DIVOOM_NET_COMM_GET_FAVORITE_LIST ||
        command == DIVOOM_NET_COMM_GET_EXPERT_LIST ||
        command == DIVOOM_NET_COMM_GET_ALBUM_LIST ||
        command == DIVOOM_NET_COMM_PLAY_LIST_GET_IMAGE_LIST;
}

static int divoom_subscribe_manage_transfer_reopen_is_active(void)
{
    uint32 now;

    if(!g_divoom_subscribe_transfer_reopen_wait)
    {
        return 0;
    }
    now = (uint32)divoom_system_get_tickms();
    if((uint32)(now - g_divoom_subscribe_transfer_reopen_tick) >=
        DIVOOM_SUBSCRIBE_TRANSFER_REOPEN_WAIT_MS)
    {
        g_divoom_subscribe_transfer_reopen_wait = 0;
        return 0;
    }
    return 1;
}

/*
 * divoom_subscribe_manage_update 持有本模块锁期间同步发包；这里记录精确的
 * PacketFlag，后续只允许对应回包启用文件下载进度统计。
 */
void divoom_subscribe_manage_transfer_request_add(
    int command, uint32 packet_flag)
{
    int pos;
    int notify_dialog = 0;
    int reopen_request;

    reopen_request = divoom_subscribe_manage_transfer_reopen_is_active() &&
        g_divoom_subscribe_transfer_reopen_command == (uint16)command;
    if(pdivoom_subscribe_manage_handle == NULL ||
        (!g_divoom_subscribe_update_active &&
            !g_divoom_subscribe_transfer_response_active &&
            !reopen_request) ||
        !divoom_subscribe_manage_is_transfer_command(command))
    {
        return;
    }
    pos = pdivoom_subscribe_manage_handle->transfer_request_num;
    if(pos >= DIVOOM_SUBSCRIBE_TRANSFER_REQUEST_MAX)
    {
        divoom_print("subscribe transfer request full, cmd=%d flag=%u\n",
            command, packet_flag);
        return;
    }
    pdivoom_subscribe_manage_handle->transfer_request[pos].command =
        (uint16)command;
    pdivoom_subscribe_manage_handle->transfer_request[pos].packet_flag =
        packet_flag;
    pdivoom_subscribe_manage_handle->transfer_request_num++;
    if(g_divoom_subscribe_update_active &&
        !g_divoom_subscribe_transfer_dialog_notified)
    {
        g_divoom_subscribe_transfer_dialog_notified = 1;
        notify_dialog = 1;
    }
    if(notify_dialog)
    {
        /* send_packet 会在本函数返回后发包，先让界面进入 0% 等待状态。 */
        divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP,
            DIVOOM_LIGHT_APP_EVENT_APP_SEND_PHOTO);
    }
}

int divoom_subscribe_manage_transfer_response_begin(
    int command, uint32 packet_flag)
{
    int i;
    int active = 0;

    g_divoom_subscribe_transfer_response_active = 0;
    g_divoom_subscribe_transfer_response_deferred = 0;
    if(pdivoom_subscribe_manage_handle == NULL)
    {
        return 0;
    }
    divoom_system_get_lock(pdivoom_subscribe_manage_handle->lock_handle);
    for(i = 0; i < pdivoom_subscribe_manage_handle->transfer_request_num; i++)
    {
        if(pdivoom_subscribe_manage_handle->transfer_request[i].command ==
                command &&
            pdivoom_subscribe_manage_handle->transfer_request[i].packet_flag ==
                packet_flag)
        {
            pdivoom_subscribe_manage_handle->transfer_request_num--;
            if(i < pdivoom_subscribe_manage_handle->transfer_request_num)
            {
                memmove(&pdivoom_subscribe_manage_handle->transfer_request[i],
                    &pdivoom_subscribe_manage_handle->transfer_request[i + 1],
                    (uint32)(pdivoom_subscribe_manage_handle->transfer_request_num - i) *
                        sizeof(divoom_subscribe_transfer_request));
            }
            active = 1;
            break;
        }
    }
    divoom_system_free_lock(pdivoom_subscribe_manage_handle->lock_handle);
    if(active)
    {
        g_divoom_subscribe_transfer_response_command = (uint16)command;
    }
    g_divoom_subscribe_transfer_response_active = (uint8)active;
    return active;
}

void divoom_subscribe_manage_transfer_response_defer(void)
{
    if(g_divoom_subscribe_transfer_response_active)
    {
        g_divoom_subscribe_transfer_response_deferred = 1;
    }
}

void divoom_subscribe_manage_transfer_response_end(void)
{
    if(g_divoom_subscribe_transfer_response_active)
    {
        if(g_divoom_subscribe_transfer_response_deferred)
        {
            /*
             * 切换订阅时旧表盘会先销毁管理句柄；允许新表盘重开后发出的
             * 同类列表请求继续属于本轮进度会话。
             */
            g_divoom_subscribe_transfer_reopen_command =
                g_divoom_subscribe_transfer_response_command;
            g_divoom_subscribe_transfer_reopen_tick =
                (uint32)divoom_system_get_tickms();
            g_divoom_subscribe_transfer_reopen_wait = 1;
        }
        else
        {
            g_divoom_subscribe_transfer_reopen_wait = 0;
        }
    }
    g_divoom_subscribe_transfer_response_deferred = 0;
    g_divoom_subscribe_transfer_response_active = 0;
}

int divoom_subscribe_manage_transfer_response_is_active(void)
{
    return g_divoom_subscribe_transfer_response_active != 0;
}

int divoom_subscribe_manage_transfer_request_is_pending(void)
{
    return pdivoom_subscribe_manage_handle != NULL &&
        (g_divoom_subscribe_update_active ||
            pdivoom_subscribe_manage_handle->transfer_request_num > 0 ||
            g_divoom_subscribe_transfer_response_active ||
            divoom_subscribe_manage_transfer_reopen_is_active());
}

/* 仅接纳本轮订阅更新对应的回包，避免同类后台请求污染提示页结论。 */
void divoom_subscribe_manage_reload_result_add(int command, int32 add_file_num)
{
    if(pdivoom_subscribe_manage_handle == NULL)
    {
        return;
    }
    divoom_system_get_lock(pdivoom_subscribe_manage_handle->lock_handle);
    if(pdivoom_subscribe_manage_handle->reload_result_state ==
            DIVOOM_SUBSCRIBE_RELOAD_RESULT_WAITING &&
        pdivoom_subscribe_manage_handle->reload_command == command)
    {
        pdivoom_subscribe_manage_handle->reload_received_num++;
        if(add_file_num > 0)
        {
            pdivoom_subscribe_manage_handle->reload_add_file_num += add_file_num;
        }
        if(pdivoom_subscribe_manage_handle->reload_received_num >=
            pdivoom_subscribe_manage_handle->reload_expected_num)
        {
            pdivoom_subscribe_manage_handle->reload_result_state =
                DIVOOM_SUBSCRIBE_RELOAD_RESULT_COMPLETE;
        }
    }
    divoom_system_free_lock(pdivoom_subscribe_manage_handle->lock_handle);
}

int divoom_subscribe_manage_reload_result_get(int32 *add_file_num)
{
    int result_state;

    if(add_file_num != NULL)
    {
        *add_file_num = 0;
    }
    if(pdivoom_subscribe_manage_handle == NULL)
    {
        return DIVOOM_SUBSCRIBE_RELOAD_RESULT_IDLE;
    }
    divoom_system_get_lock(pdivoom_subscribe_manage_handle->lock_handle);
    result_state = pdivoom_subscribe_manage_handle->reload_result_state;
    if(add_file_num != NULL &&
        result_state == DIVOOM_SUBSCRIBE_RELOAD_RESULT_COMPLETE)
    {
        *add_file_num = pdivoom_subscribe_manage_handle->reload_add_file_num;
    }
    divoom_system_free_lock(pdivoom_subscribe_manage_handle->lock_handle);
    return result_state;
}

/*********************************************************************
  Function:        divoom_subscribe_manage_reload_more_gif
  Description:     每天为clock_id=250的多动画表盘更新一次订阅资源
  Input:           mday 当前日期中的日
  Return:          1 已发起当天更新，0 未发起更新
**********************************************************************/

int divoom_subscribe_manage_reload_more_gif(int mday)
{
    int i;
    int j;
    int expert_num;
    int request_num;
    int reload_command;
    int reload_expected_num;
    int update_time;
    divoom_clock_id_info clock_info = {0};
    divoom_disp_subscribe_data subscribe_info = {0};
    divoom_devcice_expert_request_json expert_info;
    divoom_device_playlist_image_list_item playlist_info;

    if(pdivoom_subscribe_manage_handle == NULL)
    {
        return 0;
    }

    divoom_system_get_lock(pdivoom_subscribe_manage_handle->lock_handle);
    if(pdivoom_subscribe_manage_handle->load_update_time == mday)
    {
        divoom_system_free_lock(pdivoom_subscribe_manage_handle->lock_handle);
        return 0;
    }

    clock_info.clock_id = DIVOOM_SUBSCRIBE_MANAGE_MORE_GIF_CLOCK_ID;
    if(divoom_database_manage_get_subscribe_config_record(
        DIVOOM_SUBSCRIBE_IMAGE_TABLE_NAME, clock_info.clock_id,
        clock_info.clock_pos, clock_info.parent_id, &subscribe_info) == 0)
    {
        divoom_system_free_lock(pdivoom_subscribe_manage_handle->lock_handle);
        return 0;
    }
    if(subscribe_info.mode > DIVOOM_DISP_SUBSCRIBE_MODE_PLAY_LIST)
    {
        divoom_system_free_lock(pdivoom_subscribe_manage_handle->lock_handle);
        return 0;
    }

    reload_command = -1;
    reload_expected_num = 0;
    if(subscribe_info.mode == DIVOOM_DISP_SUBSCRIBE_MODE_HOT)
    {
        reload_command = DIVOOM_NET_COMM_GET_HOT_LIST;
        reload_expected_num = 1;
    }
    else if(subscribe_info.mode == DIVOOM_DISP_SUBSCRIBE_MODE_FAVORITE)
    {
        reload_command = DIVOOM_NET_COMM_GET_FAVORITE_LIST;
        reload_expected_num = 1;
    }
    else if(subscribe_info.mode == DIVOOM_DISP_SUBSCRIBE_MODE_EXPERT)
    {
        expert_num = subscribe_info.expert_num;
        if(expert_num > (int)(sizeof(subscribe_info.expert_list) /
            sizeof(subscribe_info.expert_list[0])))
        {
            expert_num = (int)(sizeof(subscribe_info.expert_list) /
                sizeof(subscribe_info.expert_list[0]));
        }
        if(expert_num > 0)
        {
            reload_command = DIVOOM_NET_COMM_GET_EXPERT_LIST;
            reload_expected_num = (expert_num +
                DIVOOM_DEVICE_SUSCRIBE_EXPERT_NUM - 1) /
                DIVOOM_DEVICE_SUSCRIBE_EXPERT_NUM;
        }
    }
    else if(subscribe_info.mode == DIVOOM_DISP_SUBSCRIBE_MODE_ALBUM &&
        subscribe_info.album_id > 0)
    {
        reload_command = DIVOOM_NET_COMM_GET_ALBUM_LIST;
        reload_expected_num = 1;
    }
    else if(subscribe_info.mode == DIVOOM_DISP_SUBSCRIBE_MODE_PLAY_LIST &&
        subscribe_info.play_id > 0)
    {
        reload_command = DIVOOM_NET_COMM_PLAY_LIST_GET_IMAGE_LIST;
        reload_expected_num = 1;
    }
    if(reload_expected_num <= 0)
    {
        divoom_system_free_lock(pdivoom_subscribe_manage_handle->lock_handle);
        return 0;
    }

    pdivoom_subscribe_manage_handle->load_update_time = mday;
    pdivoom_subscribe_manage_handle->reload_command = reload_command;
    pdivoom_subscribe_manage_handle->reload_expected_num = reload_expected_num;
    pdivoom_subscribe_manage_handle->reload_received_num = 0;
    pdivoom_subscribe_manage_handle->reload_add_file_num = 0;
    pdivoom_subscribe_manage_handle->reload_result_state =
        DIVOOM_SUBSCRIBE_RELOAD_RESULT_WAITING;
    divoom_system_free_lock(pdivoom_subscribe_manage_handle->lock_handle);

    if(subscribe_info.mode == DIVOOM_DISP_SUBSCRIBE_MODE_HOT)
    {
        divoom_hot_manage_request_hot_image(0, 0);
    }
    else if(subscribe_info.mode == DIVOOM_DISP_SUBSCRIBE_MODE_FAVORITE)
    {
        update_time = divoom_database_manage_get_subscribe_image_last_update_time(
            DIVOOM_FAVORITE_IMAGE_TABLE_NAME, 0);
        if(!divoom_favorite_manage_request_list(update_time))
        {
            divoom_subscribe_manage_reload_result_add(
                DIVOOM_NET_COMM_GET_FAVORITE_LIST, 0);
        }
    }
    else if(subscribe_info.mode == DIVOOM_DISP_SUBSCRIBE_MODE_EXPERT)
    {
        for(i = 0; i < expert_num; i += request_num)
        {
            request_num = expert_num - i;
            if(request_num > DIVOOM_DEVICE_SUSCRIBE_EXPERT_NUM)
            {
                request_num = DIVOOM_DEVICE_SUSCRIBE_EXPERT_NUM;
            }
            memset(&expert_info, 0, sizeof(expert_info));
            expert_info.expert_num = request_num;
            expert_info.file_num = DIVOOM_SUBSCRIBE_MANAGE_MORE_GIF_FILE_NUM;
            for(j = 0; j < request_num; j++)
            {
                expert_info.expert_list[j] = subscribe_info.expert_list[i + j];
                expert_info.upload_time[j] =
                    divoom_database_manage_get_subscribe_image_last_update_time(
                    DIVOOM_EXPERT_IMAGE_TABLE_NAME,
                    expert_info.expert_list[j]);
            }
            divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_EXPERT_LIST,
                DIVOOM_SERVER_TOPIC, &expert_info);
        }
    }
    else if(subscribe_info.mode == DIVOOM_DISP_SUBSCRIBE_MODE_ALBUM)
    {
        if(subscribe_info.album_id > 0)
        {
            divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_ALBUM_LIST,
                DIVOOM_SERVER_TOPIC, &subscribe_info.album_id);
        }
    }
    else if(subscribe_info.mode == DIVOOM_DISP_SUBSCRIBE_MODE_PLAY_LIST)
    {
        if(subscribe_info.play_id > 0)
        {
            memset(&playlist_info, 0, sizeof(playlist_info));
            playlist_info.file_size =
                DIVOOM_SUBSCRIBE_MANAGE_MORE_GIF_PLAY_FILE_SIZE;
            playlist_info.playlist_id = subscribe_info.play_id;
            playlist_info.update_time =
                divoom_database_manage_get_divoom_image_update_tiem(
                DIVOOM_USERIMAGELIST_CONFIGTABLE_NAME,
                subscribe_info.play_id);
            divoom_net_app_send_packet(
                DIVOOM_NET_COMM_PLAY_LIST_GET_IMAGE_LIST,
                DIVOOM_SERVER_TOPIC, &playlist_info);
        }
    }

    return 1;
}

int divoom_subscribe_manage_compare_clock(void *item1, void *item2)
{
    Pdivoom_clock_id_info pclock_info = (Pdivoom_clock_id_info)item1;
    Pdivoom_subscribe_manage_item pcur_item = (Pdivoom_subscribe_manage_item)item2;

    if(pclock_info->clock_id == pcur_item->clock_id && pclock_info->clock_pos == pcur_item->clock_pos && pclock_info->parent_id == pcur_item->parent_id)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}
void *divoom_subscribe_manage_read(Pdivoom_clock_id_info pclock_info, int *read_db_flag, int *flag);

/*********************************************************************
  Function:        divoom_subscribe_manage_update
  Description:     更新表盘配置
  Input:           pclock_info
                   psubscribe_info 
  Return:          none
**********************************************************************/

void divoom_subscribe_manage_update(Pdivoom_clock_id_info pclock_info, Pdivoom_channel_set_subscribe_info psubscribe_info)
{
    int i, j, insert_flag, read_db_flag, expert_list_flag = 0, flag = 0;
    divoom_disp_subscribe_data cur_subscribe_info = {0};
    Pdivoom_subscribe_manage_item pcur_item;

    divoom_system_get_lock(pdivoom_subscribe_manage_handle->lock_handle);
    pcur_item = divoom_subscribe_manage_read( pclock_info, &read_db_flag, &flag);
    if(pcur_item == NULL)
    {
        divoom_system_free_lock(pdivoom_subscribe_manage_handle->lock_handle);
        return;
    }

    cur_subscribe_info.disp_default = pcur_item->subscribe_info.disp_default; //０:不显示默认图片，１：显示默认图片
    cur_subscribe_info.mode = psubscribe_info->type;    //订阅类型0:推荐图库1：收藏2：订阅作者
    cur_subscribe_info.disp = psubscribe_info->mode;    //0：循环最新1：随机全部
    cur_subscribe_info.play_time = pcur_item->subscribe_info.play_time;
    /* 更新订阅来源时保留独立下发的显示时间配置。 */
    cur_subscribe_info.show_time = pcur_item->subscribe_info.show_time;
    cur_subscribe_info.play_id = psubscribe_info->play_id;
    cur_subscribe_info.album_id = psubscribe_info->album_id;
    cur_subscribe_info.expert_num = psubscribe_info->user_num;
    if(psubscribe_info->user_num)
    {
        memcpy(cur_subscribe_info.expert_list, psubscribe_info->user_id_list, psubscribe_info->user_num * sizeof(int));
    }

    g_divoom_subscribe_transfer_dialog_notified = 0;
    g_divoom_subscribe_transfer_response_deferred = 0;
    g_divoom_subscribe_transfer_reopen_wait = 0;
    g_divoom_subscribe_update_active = 1;
    insert_flag = divoom_database_manage_update_subscribe_config_record(DIVOOM_SUBSCRIBE_IMAGE_TABLE_NAME, pclock_info->clock_id, pclock_info->clock_pos, pclock_info->parent_id, &cur_subscribe_info);
    if(insert_flag == 0)
    {
        //更新
        if(pcur_item)
        {
            divoom_album_manage_change_album_id(pclock_info->clock_id, pclock_info->clock_pos, pclock_info->parent_id, pcur_item->subscribe_info.album_id, psubscribe_info->album_id);
            divoom_userimagelist_manage_change_album_id(pclock_info->clock_id, pclock_info->clock_pos, pclock_info->parent_id, pcur_item->subscribe_info.play_id, psubscribe_info->play_id);
            for(i= 0; i < pcur_item->subscribe_info.expert_num; i++)
            {
                for(j = 0; j < psubscribe_info->user_num; j++)
                {
                    if(pcur_item->subscribe_info.expert_list[i] == psubscribe_info->user_id_list[j])
                    {
                        break;
                    }
                }
                if(j >= psubscribe_info->user_num)
                {
                    //这个达人是已经被删除了
                    divoom_expert_manage_delete_expert_info(pclock_info->clock_id, pclock_info->clock_pos, pclock_info->parent_id, pcur_item->subscribe_info.expert_list[i]);
                    if(pcur_item->subscribe_info.mode == cur_subscribe_info.mode && pcur_item->subscribe_info.mode == DIVOOM_DISP_SUBSCRIBE_MODE_EXPERT  && pcur_item->use_time > 1)
                    {
                        divoom_expert_manage_close_expert_handle(pcur_item->subscribe_info.expert_list[i]);
                    }
                }
            }
            
            for(i= 0; i < psubscribe_info->user_num; i++)
            {
                for(j = 0; j < pcur_item->subscribe_info.expert_num; j++)
                {
                    if(pcur_item->subscribe_info.expert_list[j] == psubscribe_info->user_id_list[i])
                    {
                        break;
                    }
                }
                if(j >= pcur_item->subscribe_info.expert_num)
                {
                    //这个达人是新增加
                    divoom_expert_manage_add_expert_info(pclock_info->clock_id, pclock_info->clock_pos, pclock_info->parent_id, psubscribe_info->user_id_list[i]);
                    if(pcur_item->subscribe_info.mode == cur_subscribe_info.mode && pcur_item->subscribe_info.mode == DIVOOM_DISP_SUBSCRIBE_MODE_EXPERT  && pcur_item->use_time > 1)
                    {
                        divoom_expert_manage_open_expert_handle(psubscribe_info->user_id_list[i]);
                    }
                }
            }
            
        }
    }
    else
    {
        //新增加
        divoom_album_manage_change_album_id(pclock_info->clock_id, pclock_info->clock_pos, pclock_info->parent_id, 0, psubscribe_info->album_id);
        divoom_userimagelist_manage_change_album_id(pclock_info->clock_id, pclock_info->clock_pos, pclock_info->parent_id, 0, psubscribe_info->play_id);
        for(i= 0; i < psubscribe_info->user_num; i++)
        {
            divoom_expert_manage_add_expert_info(pclock_info->clock_id, pclock_info->clock_pos, pclock_info->parent_id, psubscribe_info->user_id_list[i]);
        }
        
    }

    pcur_item->use_time --;
    if(pcur_item->use_time == 0)
    {
        pcur_item = divoom_queue_pop_cmp(pdivoom_subscribe_manage_handle->info_queue, pclock_info, divoom_subscribe_manage_compare_clock);
        divoom_system_free(pcur_item);
    }
    else
    {
        if(pcur_item->subscribe_info.mode != cur_subscribe_info.mode)
        {
            //模式发生变化 ，将先关闭之前的，再打开新的
            if(pcur_item->subscribe_info.mode == DIVOOM_DISP_SUBSCRIBE_MODE_HOT)
            {
            }
            else if(pcur_item->subscribe_info.mode == DIVOOM_DISP_SUBSCRIBE_MODE_FAVORITE)
            {
                divoom_favorite_manage_close();
            }
            else if(pcur_item->subscribe_info.mode == DIVOOM_DISP_SUBSCRIBE_MODE_EXPERT)
            {
                for(int i = 0; i < pcur_item->subscribe_info.expert_num; i++)
                {
                    divoom_expert_manage_close_expert_handle(pcur_item->subscribe_info.expert_list[i]);
                }
            }
            else if(pcur_item->subscribe_info.mode == DIVOOM_DISP_SUBSCRIBE_MODE_ALBUM)
            {
                divoom_album_manage_close_album_id(pcur_item->subscribe_info.album_id);
            }
            else if(pcur_item->subscribe_info.mode == DIVOOM_DISP_SUBSCRIBE_MODE_PLAY_LIST)
            {
                divoom_userimagelist_manage_close_album_id(pcur_item->subscribe_info.play_id);
            }

            //打开新的
            if(cur_subscribe_info.mode == DIVOOM_DISP_SUBSCRIBE_MODE_HOT)
            {
            }
            else if(cur_subscribe_info.mode == DIVOOM_DISP_SUBSCRIBE_MODE_FAVORITE)
            {
                divoom_favorite_manage_open();
            }
            else if(cur_subscribe_info.mode == DIVOOM_DISP_SUBSCRIBE_MODE_EXPERT)
            {
                for(int i = 0; i < cur_subscribe_info.expert_num; i++)
                {
                    divoom_expert_manage_open_expert_handle(cur_subscribe_info.expert_list[i]);
                }
            }
            else if(cur_subscribe_info.mode == DIVOOM_DISP_SUBSCRIBE_MODE_ALBUM)
            {
                divoom_album_manage_open_album_id(cur_subscribe_info.album_id);
            }
            else if(cur_subscribe_info.mode == DIVOOM_DISP_SUBSCRIBE_MODE_PLAY_LIST)
            {
                divoom_userimagelist_manage_open_album_id(cur_subscribe_info.play_id);
            }
            pcur_item->refresh_disp = 1;
        }
        else if(pcur_item->subscribe_info.mode == DIVOOM_DISP_SUBSCRIBE_MODE_ALBUM)
        {
            if(pcur_item->subscribe_info.album_id != cur_subscribe_info.album_id)
            {
                divoom_album_manage_close_album_id(pcur_item->subscribe_info.album_id);
                divoom_album_manage_open_album_id(cur_subscribe_info.album_id);
                pcur_item->refresh_disp = 1;
            }
        }
        else if(pcur_item->subscribe_info.mode == DIVOOM_DISP_SUBSCRIBE_MODE_PLAY_LIST)
        {
            if(pcur_item->subscribe_info.play_id != cur_subscribe_info.play_id)
            {
                divoom_userimagelist_manage_close_album_id(pcur_item->subscribe_info.play_id);
                divoom_userimagelist_manage_open_album_id(cur_subscribe_info.play_id);
                pcur_item->refresh_disp = 1;
            }
        }
        memcpy(&pcur_item->subscribe_info, &cur_subscribe_info, sizeof(divoom_disp_subscribe_data));
    }
    g_divoom_subscribe_update_active = 0;
    divoom_system_free_lock(pdivoom_subscribe_manage_handle->lock_handle);
}

/*********************************************************************
  Function:        divoom_subscribe_manage_read
  Description:     读表盘配置
  Input:           pclock_info
                   read_db_flag
					flag 0缺省 1用户设置 
  Return:          handle
**********************************************************************/

void *divoom_subscribe_manage_read(Pdivoom_clock_id_info pclock_info, int *read_db_flag, int *flag)
{
    Pdivoom_subscribe_manage_item pcur_item;

    pcur_item = divoom_queue_get_cmp(pdivoom_subscribe_manage_handle->info_queue, pclock_info, divoom_subscribe_manage_compare_clock);
    if(pcur_item)
    {
        pcur_item->use_time ++;
        *read_db_flag = 0;
        return pcur_item;
    }
    pcur_item = divoom_system_malloc(sizeof(divoom_subscribe_manage_item));
    pcur_item->clock_id = pclock_info->clock_id;
    pcur_item->clock_pos = pclock_info->clock_pos;
    pcur_item->parent_id = pclock_info->parent_id;
    pcur_item->use_time = 1;
    *read_db_flag = 1;
    *flag = divoom_database_manage_get_subscribe_config_record(DIVOOM_SUBSCRIBE_IMAGE_TABLE_NAME, pclock_info->clock_id, pclock_info->clock_pos, pclock_info->parent_id, &pcur_item->subscribe_info);
	divoom_queue_push(pdivoom_subscribe_manage_handle->info_queue, pcur_item);
    return pcur_item;
}

/*********************************************************************
  Function:        divoom_subscribe_manage_open
  Description:     打开读表盘配置句柄
  Input:           pclock_info
  Return:          handle
**********************************************************************/

void *divoom_subscribe_manage_open(Pdivoom_clock_id_info pclock_info)
{  
    int read_db_flag, flag = 0;
    Pdivoom_subscribe_manage_item pcur_item;
    
    divoom_system_get_lock(pdivoom_subscribe_manage_handle->lock_handle);

    pcur_item = divoom_subscribe_manage_read(pclock_info, &read_db_flag, &flag);
    if(pcur_item == NULL)
    {
        divoom_system_free_lock(pdivoom_subscribe_manage_handle->lock_handle);
        return NULL;
    }

	if (flag == 0)
	{
	    Pdivoom_net_command ret_data = divoom_net_app_send_packet_wait(DIVOOM_NET_COMM_DEVICE_GET_SUSCRIBE_SER, DIVOOM_SERVER_TOPIC, pclock_info);
		if (ret_data)
		{
			Pdivoom_channel_update_subscribe_info pinfo = (Pdivoom_channel_update_subscribe_info)ret_data->cmd_data;
	        divoom_system_free_lock(pdivoom_subscribe_manage_handle->lock_handle);
			if (pinfo)
			{
				divoom_subscribe_manage_update(pclock_info, &pinfo->subscribe_info);
			}
	        divoom_net_app_free_packet(ret_data);
			return pcur_item;
	    }
	}

    if(pcur_item->subscribe_info.mode == DIVOOM_DISP_SUBSCRIBE_MODE_HOT)
    {
    }
    else if(pcur_item->subscribe_info.mode == DIVOOM_DISP_SUBSCRIBE_MODE_FAVORITE)
    {
        divoom_favorite_manage_open();
    }
    else if(pcur_item->subscribe_info.mode == DIVOOM_DISP_SUBSCRIBE_MODE_EXPERT)
    {
        for(int i = 0; i < pcur_item->subscribe_info.expert_num; i++)
        {
            divoom_expert_manage_open_expert_handle(pcur_item->subscribe_info.expert_list[i]);
        }
    }
    else if(pcur_item->subscribe_info.mode == DIVOOM_DISP_SUBSCRIBE_MODE_ALBUM)
    {
        divoom_album_manage_open_album_id(pcur_item->subscribe_info.album_id);
    }
    else if(pcur_item->subscribe_info.mode == DIVOOM_DISP_SUBSCRIBE_MODE_PLAY_LIST)
    {
        divoom_userimagelist_manage_check_album_id(pclock_info->clock_id, pclock_info->clock_pos, pclock_info->parent_id, pcur_item->subscribe_info.play_id);
        divoom_userimagelist_manage_open_album_id(pcur_item->subscribe_info.play_id);
    }
    divoom_system_free_lock(pdivoom_subscribe_manage_handle->lock_handle);
    divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_GET_SUSCRIBE_PLAY_TIME, DIVOOM_SERVER_TOPIC, pclock_info);
    
    return  pcur_item;
}

/*********************************************************************
  Function:        divoom_subscribe_manage_get_play_time
  Description:     获取播放时间
  Input:           pclock_info
  Return:          时间
**********************************************************************/


int divoom_subscribe_manage_get_play_time(Pdivoom_clock_id_info pclock_info)
{
    Pdivoom_subscribe_manage_item pcur_item;

    pcur_item = divoom_queue_get_cmp(pdivoom_subscribe_manage_handle->info_queue, pclock_info, divoom_subscribe_manage_compare_clock);
    if(pcur_item == NULL)
    {
        return 0;
    }
    return pcur_item->subscribe_info.play_time;
}

/*********************************************************************
  Function:        divoom_subscribe_manage_get_show_time_flag
  Description:     获取订阅表盘是否显示时间
  Input:           pclock_info
  Return:          1 显示，其他值不显示
**********************************************************************/

int divoom_subscribe_manage_get_show_time_flag(Pdivoom_clock_id_info pclock_info)
{
    int show_time = 0;
    Pdivoom_subscribe_manage_item pcur_item;

    if(pdivoom_subscribe_manage_handle == NULL || pclock_info == NULL)
    {
        return 0;
    }

    /* show_time 可能由网络线程中途更新，读取时与更新接口使用同一把锁。 */
    divoom_system_get_lock(pdivoom_subscribe_manage_handle->lock_handle);
    pcur_item = divoom_queue_get_cmp(pdivoom_subscribe_manage_handle->info_queue,
        pclock_info, divoom_subscribe_manage_compare_clock);
    if(pcur_item != NULL)
    {
        show_time = pcur_item->subscribe_info.show_time;
    }
    divoom_system_free_lock(pdivoom_subscribe_manage_handle->lock_handle);
    return show_time;
}

/*********************************************************************
  Function:        divoom_subscribe_manage_update_play_time
  Description:     更新播放时间
  Input:           pclock_info
                   play_time
                   show_time
  Return:          none
**********************************************************************/


void divoom_subscribe_manage_update_play_time(Pdivoom_clock_id_info pclock_info, int play_time, int show_time)
{
    Pdivoom_subscribe_manage_item pcur_item;

    if(pclock_info->clock_id == 0)
    {
        divoom_print("this is err!%d,%d,%d\n", pclock_info->clock_id, pclock_info->clock_pos, pclock_info->parent_id);
        return;
    }
    divoom_system_get_lock(pdivoom_subscribe_manage_handle->lock_handle);
    pcur_item = divoom_queue_get_cmp(pdivoom_subscribe_manage_handle->info_queue, pclock_info, divoom_subscribe_manage_compare_clock);
    
    if(pcur_item &&
        (pcur_item->subscribe_info.play_time != play_time ||
        pcur_item->subscribe_info.show_time != show_time))
    {
        pcur_item->subscribe_info.play_time = play_time;
        pcur_item->subscribe_info.show_time = show_time;
    }
    divoom_database_manage_update_subscribe_play_time_record(DIVOOM_SUBSCRIBE_IMAGE_TABLE_NAME, pclock_info->clock_id, pclock_info->clock_pos, pclock_info->parent_id, play_time, show_time);
    divoom_system_free_lock(pdivoom_subscribe_manage_handle->lock_handle);
    
}

/*********************************************************************
  Function:        divoom_subscribe_manage_get_change_flag
  Description:     获取播放文件是否发生变化
  Input:           pclock_info
                   image_info
  Return:          none
**********************************************************************/


int divoom_subscribe_manage_get_change_flag(Pdivoom_clock_id_info pclock_info)
{
    int change_flag = 0;
    Pdivoom_subscribe_manage_item pcur_item;
    
    divoom_system_get_lock(pdivoom_subscribe_manage_handle->lock_handle);

    pcur_item = divoom_queue_get_cmp(pdivoom_subscribe_manage_handle->info_queue, pclock_info, divoom_subscribe_manage_compare_clock);
    if(pcur_item == NULL)
    {
        divoom_system_free_lock(pdivoom_subscribe_manage_handle->lock_handle);
        return 0;
    }
    change_flag = pcur_item->refresh_disp;
    divoom_system_free_lock(pdivoom_subscribe_manage_handle->lock_handle);
    return change_flag;
}

/*********************************************************************
  Function:        divoom_subscribe_manage_get_image_id
  Description:     获取播放文件相关信息
  Input:           pclock_info
                   image_info
  Return:          none
**********************************************************************/


void divoom_subscribe_manage_get_image_id(Pdivoom_clock_id_info pclock_info, Pdivoom_play_gif_image_info image_info)
{
    int user_id = 0;
    Pdivoom_subscribe_manage_item pcur_item;
    
    divoom_system_get_lock(pdivoom_subscribe_manage_handle->lock_handle);

    pcur_item = divoom_queue_get_cmp(pdivoom_subscribe_manage_handle->info_queue, pclock_info, divoom_subscribe_manage_compare_clock);
    if(pcur_item == NULL)
    {
        divoom_system_free_lock(pdivoom_subscribe_manage_handle->lock_handle);
        return ;
    }
    if(pcur_item->refresh_disp == 1)
    {
        pcur_item->refresh_disp = 0;
        image_info->image_id = -1;
        divoom_system_free_lock(pdivoom_subscribe_manage_handle->lock_handle);
        return;
    }
    if(pcur_item->subscribe_info.mode == DIVOOM_DISP_SUBSCRIBE_MODE_HOT)
    {
        
        image_info->image_id = divoom_hot_manage_get_random_pic_id(&user_id, image_info->image_name,
            DIVOOM_HOT_MANAGE_DEFAULT_MAX_SIZE);
        if(image_info->image_id == 0)
        {
            image_info->model_id =  DIVOOM_MANAGE_IMAGE_LOCAL_PIC_MODE;
            image_info->image_id =  DIVOOM_MANAGE_IMAGE_ID_BASE_LOCAL_START_GIF + DIVOOM_DISP_HOME_PAGE_HOT;
        }
        else
        {
            image_info->model_id = DIVOOM_MANAGE_IMAGE_SUBSCRIBE_DEFAULT_16;
        }
    }
    else if(pcur_item->subscribe_info.mode == DIVOOM_DISP_SUBSCRIBE_MODE_FAVORITE)
    {
        image_info->image_id = divoom_favorite_manage_get_disp_image_id(&user_id, image_info->image_name);
        if(image_info->image_id == 0)
        {
            image_info->model_id =  DIVOOM_MANAGE_IMAGE_LOCAL_PIC_MODE;
            image_info->image_id =  DIVOOM_MANAGE_IMAGE_ID_BASE_LOCAL_START_GIF + DIVOOM_DISP_USER_DEFINE_TV_PIC;
        }
        else
        {
            image_info->model_id = DIVOOM_MANAGE_IMAGE_SUBSCRIBE_FAVORITE;
        }
    }
    else if(pcur_item->subscribe_info.mode == DIVOOM_DISP_SUBSCRIBE_MODE_EXPERT)
    {
        if(pcur_item->subscribe_info.expert_num == 0)
        {
            image_info->model_id =  DIVOOM_MANAGE_IMAGE_LOCAL_PIC_MODE;
            image_info->image_id =  DIVOOM_MANAGE_IMAGE_ID_BASE_LOCAL_START_GIF + DIVOOM_DISP_USER_DEFINE_TV_PIC;
        }
        else
        {
            for(int i= 0; i < pcur_item->subscribe_info.expert_num; i++)
            {
                if(pcur_item->expert_id >= pcur_item->subscribe_info.expert_num)
                {
                    pcur_item->expert_id = 0;
                }
                divoom_expert_manage_get_disp_image_id(pcur_item->subscribe_info.expert_list[pcur_item->expert_id], image_info, pcur_item->subscribe_info.disp, image_info->image_name);
                user_id = pcur_item->subscribe_info.expert_list[pcur_item->expert_id];
                pcur_item->expert_id ++;
                if(image_info->image_id)
                {
                    break;
                }
            }
            if(image_info->image_id == 0)
            {
                divoom_print("image empty: %d, %d\n", pcur_item->expert_id, user_id);
                image_info->model_id =  DIVOOM_MANAGE_IMAGE_LOCAL_PIC_MODE;
                image_info->image_id =  DIVOOM_MANAGE_IMAGE_ID_BASE_LOCAL_START_GIF + DIVOOM_DISP_USER_DEFINE_TV_PIC;
            }
        }
    }
    else if(pcur_item->subscribe_info.mode == DIVOOM_DISP_SUBSCRIBE_MODE_ALBUM)
    {
        image_info->image_id = divoom_album_manage_get_random_pic_id(pcur_item->subscribe_info.album_id,&user_id, image_info->image_name);
        if(image_info->image_id == 0)
        {
            image_info->model_id =  DIVOOM_MANAGE_IMAGE_LOCAL_PIC_MODE;
            image_info->image_id =  DIVOOM_MANAGE_IMAGE_ID_BASE_LOCAL_START_GIF + DIVOOM_DISP_USER_DEFINE_TV_PIC;
        }
        else
        {
            image_info->model_id = DIVOOM_MANAGE_IMAGE_ALBUM_ID_LIST_PIC;
        }
    }
    else if(pcur_item->subscribe_info.mode == DIVOOM_DISP_SUBSCRIBE_MODE_PLAY_LIST)
    {
        image_info->image_id = divoom_userimagelist_manage_get_random_pic_id(pcur_item->subscribe_info.play_id,&user_id, image_info->image_name);
        if(image_info->image_id == 0)
        {
            image_info->model_id =  DIVOOM_MANAGE_IMAGE_LOCAL_PIC_MODE;
            image_info->image_id =  DIVOOM_MANAGE_IMAGE_ID_BASE_LOCAL_START_GIF + DIVOOM_DISP_USER_DEFINE_TV_PIC;
        }
        else
        {
            image_info->model_id = DIVOOM_MANAGE_IMAGE_USER_IMAGE_LIST_PIC;
        }
    }
    if(user_id == 0)
    {
        user_id = divoom_main_get_host_id();
    }
    image_info->user_id = user_id;
    divoom_system_free_lock(pdivoom_subscribe_manage_handle->lock_handle);
}


/*********************************************************************
  Function:        divoom_subscribe_manage_close
  Description:     关闭相关句柄信息
  Input:           pclock_info
                   
  Return:          none
**********************************************************************/


void divoom_subscribe_manage_close(Pdivoom_clock_id_info pclock_info)
{
    Pdivoom_subscribe_manage_item pcur_item;
    
    divoom_system_get_lock(pdivoom_subscribe_manage_handle->lock_handle);

    pcur_item = divoom_queue_get_cmp(pdivoom_subscribe_manage_handle->info_queue, pclock_info, divoom_subscribe_manage_compare_clock);
    if(pcur_item == NULL)
    {
        divoom_system_free_lock(pdivoom_subscribe_manage_handle->lock_handle);
        return ;
    }
    if(pcur_item->subscribe_info.mode == DIVOOM_DISP_SUBSCRIBE_MODE_HOT)
    {
    }
    else if(pcur_item->subscribe_info.mode == DIVOOM_DISP_SUBSCRIBE_MODE_FAVORITE)
    {
        divoom_favorite_manage_close();
    }
    else if(pcur_item->subscribe_info.mode == DIVOOM_DISP_SUBSCRIBE_MODE_EXPERT)
    {
        for(int i = 0; i < pcur_item->subscribe_info.expert_num; i++)
        {
            divoom_expert_manage_close_expert_handle(pcur_item->subscribe_info.expert_list[i]);
        }
    }
    else if(pcur_item->subscribe_info.mode == DIVOOM_DISP_SUBSCRIBE_MODE_ALBUM)
    {
        divoom_album_manage_close_album_id(pcur_item->subscribe_info.album_id);
    }
    else if(pcur_item->subscribe_info.mode == DIVOOM_DISP_SUBSCRIBE_MODE_PLAY_LIST)
    {
        divoom_userimagelist_manage_close_album_id(pcur_item->subscribe_info.play_id);
    }
    pcur_item->use_time --;
    if(pcur_item->use_time == 0)
    {
        pcur_item = divoom_queue_pop_cmp(pdivoom_subscribe_manage_handle->info_queue, pclock_info, divoom_subscribe_manage_compare_clock);
        if(pcur_item)
        {
            divoom_system_free(pcur_item);
        }
    }
    divoom_system_free_lock(pdivoom_subscribe_manage_handle->lock_handle);
}
