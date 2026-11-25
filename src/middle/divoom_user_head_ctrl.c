#include "divoom.h"

#define DIVOOM_USER_HEAD_NUM 3
typedef struct _divoom_user_head_cache_item
{
    int clock_id;
    char url[64];
}divoom_user_head_cache_item, *Pdivoom_user_head_cache_item;
static char divoom_user_head_cache_url[DIVOOM_USER_HEAD_NUM][64];
static int divoom_user_head_img_clock_list[DIVOOM_USER_HEAD_NUM] = {383, 527, 216};


typedef struct _divoom_user_head_ctrl_ctrl
{
    void *head_queue;
}divoom_user_head_ctrl_ctrl, *Pdivoom_user_head_ctrl_ctrl;

static Pdivoom_user_head_ctrl_ctrl pdivoom_user_head_ctrl_handle = NULL;

void divoom_user_head_ctrl_load(void)
{
	divoom_cJSON *root = NULL, *list,*item;
    Pdivoom_user_head_cache_item pnew_item;
    char *file_buf = divoom_app_com_read_file(DIVOOM_DEIVCE_CLOCK_USER_HEAD_SAVE_PATH,  NULL);

    if(file_buf == NULL)
    {
        return;
    }	
    root = divoom_cJSON_Parse(file_buf);
    divoom_system_free(file_buf);

    if(root == NULL)
    {
        return;
    }
    
    list = divoom_cJSON_GetObjectItem(root, "List");
    for(int i = 0; i < divoom_cJSON_GetArraySize(list); i++)
    {
        item = divoom_cJSON_GetArrayItem(list, i);
        if(item == NULL)
        {
            break;
        }
        pnew_item = divoom_system_malloc(sizeof(divoom_user_head_cache_item));
        divoom_cJSON_GetObjectString(item, "url", pnew_item->url);
        pnew_item->clock_id = divoom_cJSON_GetObjectInt(item, "clock_id");
        divoom_queue_push(pdivoom_user_head_ctrl_handle->head_queue, pnew_item);
    }

    
    divoom_cJSON_Delete(root);
}
void divoom_user_head_ctrl_init(void)
{
    pdivoom_user_head_ctrl_handle = divoom_system_malloc(sizeof(divoom_user_head_ctrl_ctrl));
    pdivoom_user_head_ctrl_handle->head_queue = divoom_queue_init(100, __func__, __LINE__);
}

void divoom_user_head_ctrl_save_info(void)
{
    char *json_string;;
    divoom_cJSON *root, *item, *list;
    Pdivoom_user_head_cache_item pcur_item;
    
    
    root = divoom_cJSON_CreateObject();
    if(root == NULL)
    {
        return;
    }
	divoom_cJSON_AddItemToObject(root, "List", list = divoom_cJSON_CreateObject());
    for(int i = 0; i < divoom_queue_items(pdivoom_user_head_ctrl_handle->head_queue); i++)
    {
        pcur_item = divoom_queue_get_index_item(pdivoom_user_head_ctrl_handle->head_queue, i);
        if(pcur_item == NULL)
        {
            break;
        }
		divoom_cJSON_AddItemToArray(list, item = divoom_cJSON_CreateObject());      
        divoom_cJSON_AddNumberToObject(item , "clock_id", pcur_item->clock_id);
        divoom_cJSON_AddStringToObject(item , "url", pcur_item->url);
    }

    
    json_string = divoom_cJSON_Print(root);
    divoom_cJSON_Delete(root);
    divoom_app_com_write_file(DIVOOM_DEIVCE_CLOCK_USER_HEAD_SAVE_PATH,json_string, strlen(json_string));
    divoom_system_free(json_string);
}
int divoom_user_head_ctrl_compare(void *item1, void *item2)
{
    int *clock_id = (int *)item1;
    Pdivoom_user_head_cache_item pcur_item = (Pdivoom_user_head_cache_item)item2;

    return *clock_id == pcur_item->clock_id?0:1;
}

int divoom_user_head_ctrl_update(int clock_id, char *url)
{
    uint32 image_id, i;
    divoom_manage_image_item_info pimage_info;
    Pdivoom_user_head_cache_item pcur_item;
    
    image_id = divoom_clock_manage_get_disp_item_image(clock_id, DIVOOM_CLOCK_DISP_SUPPORT_DIVOOM_USER_HEAD_IMG);

    pcur_item = divoom_queue_pop_cmp(pdivoom_user_head_ctrl_handle->head_queue, &clock_id, divoom_user_head_ctrl_compare);
    if(pcur_item == NULL)
    {
        pcur_item = divoom_system_malloc(sizeof(divoom_user_head_cache_item));
        pcur_item->clock_id = clock_id;
        divoom_queue_push(pdivoom_user_head_ctrl_handle->head_queue, pcur_item);
    }

    if(strcmp(url ,pcur_item->url) == 0)
    {
        divoom_print("aj test!\n");
        strcpy(pimage_info.file_addr, url);
        pimage_info.image_id = image_id;
        pimage_info.pcallback = divoom_app_com_finishcallback;
        divoom_manage_image_add(DIVOOM_MANAGE_IMAGE_USER_DEFINE_CLOCK_PIC, &pimage_info, 1);
        //divoom_disp_clock_reload_user_head_image(NULL, 0);    
        return 0;
    }

    strcpy(pcur_item->url, url);
    divoom_user_head_ctrl_save_info();
    divoom_print("aj test!\n");
    divoom_manage_image_del_direct(DIVOOM_MANAGE_IMAGE_USER_DEFINE_CLOCK_PIC, &image_id, 1);
    strcpy(pimage_info.file_addr, url);
    pimage_info.image_id = image_id;
    pimage_info.pcallback = divoom_app_com_finishcallback;
    divoom_print("aj test!\n");
    divoom_manage_image_add(DIVOOM_MANAGE_IMAGE_USER_DEFINE_CLOCK_PIC, &pimage_info, 1);
    divoom_print("aj test!\n");
    return 1;
}

