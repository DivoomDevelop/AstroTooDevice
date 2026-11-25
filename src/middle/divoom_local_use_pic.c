#include "divoom.h"

typedef struct _divoom_local_use_pic_save_item
{
    int disp_id;
    int version;
}divoom_local_use_pic_save_item, *Pdivoom_local_use_pic_save_item;

typedef struct _divoom_local_use_pic_disp_item
{
    int disp_id;
    int version;
    char file_id[52];
}divoom_local_use_pic_disp_item, *Pdivoom_local_use_pic_disp_item;

const divoom_local_use_pic_disp_item divoom_local_use_pic_notice_pic_table[] = 
{
	{DIVOOM_lOCAL_USE_PIC_EQ_POS_PIC_2, 2, "group1/M00/DA/7D/eEwpPWDUTzaEbXb3AAAAAPshqy05559149"},
	{DIVOOM_lOCAL_USE_PIC_EQ_POS_PIC_2_EX, 2, "group1/M00/DA/7F/eEwpPWDUVVyEZQE6AAAAAEDoUDY6301821"},
	{DIVOOM_lOCAL_USE_PIC_EQ_POS_PIC_4, 1, "group1/M00/DA/7A/eEwpPWDUQzCEQYnPAAAAAH1N9Qk6956766"},
	{DIVOOM_lOCAL_USE_PIC_EQ_POS_PIC_6, 1, "group1/M00/DA/7C/eEwpPWDUSDGEKdPnAAAAAKDImGg3543172"},
	{DIVOOM_lOCAL_USE_PIC_EQ_POS_PIC_7, 2, "group1/M00/D9/AA/L1ghbmDUgZ6ECQUuAAAAAHJNkHQ4730522"},	 
	{DIVOOM_lOCAL_USE_PIC_EQ_POS_PIC_8, 2, "group1/M00/DA/89/eEwpPWDUgcmEXprDAAAAAJBMUFw6601956"},
	{DIVOOM_lOCAL_USE_PIC_EQ_POS_PIC_10, 1, "group1/M00/D9/A5/L1ghbmDUbxuEH9T7AAAAALvXI845093885"},
	{DIVOOM_lOCAL_USE_PIC_EQ_POS_PIC_11, 1, "group1/M00/D9/AA/L1ghbmDUgUiEYBPtAAAAAAntwYU7609988"},
	{DIVOOM_lOCAL_USE_PIC_EQ_POS_PIC_11_EX, 1, "group1/M00/D9/AA/L1ghbmDUghiEcqZwAAAAAFu8R1g8747653"},
	{DIVOOM_lOCAL_USE_PIC_EQ_POS_PIC_12, 1, "group1/M00/DA/89/eEwpPWDUglKEYpIZAAAAAAkVe3Y2412848"},
	{DIVOOM_lOCAL_USE_GIF_LIKE_ANIMATION_MSG, 1, "group1/M00/07/01/Ci0s8WiUFROELKcrAAAAAO1yB8o55.webp"},
};


void divoom_local_use_pic_save(void)
{
    int i, file_num;
    FILE *file_handle;
    Pdivoom_local_use_pic_save_item pall_items = NULL;
    uint32 check_sum = 0;

    file_handle = divoom_port_file_open(DIVOOM_DEVICE_NOTICE_SAVE_PATH, "wb");
    if(file_handle == NULL)
    {
        return;
    }

    file_num = sizeof(divoom_local_use_pic_notice_pic_table)/ sizeof(divoom_local_use_pic_notice_pic_table[0]);
    pall_items = divoom_system_malloc(file_num * sizeof(divoom_local_use_pic_save_item));
    for(i = 0; i < file_num; i ++)
    {
        pall_items[i].disp_id = divoom_local_use_pic_notice_pic_table[i].disp_id;
        pall_items[i].version = divoom_local_use_pic_notice_pic_table[i].version;
    }
    
    check_sum = divoom_app_com_get_check_sum((uint8 *)&file_num, sizeof(file_num));
    divoom_port_file_write(&file_num,sizeof(file_num),  1, file_handle);
    
    check_sum += divoom_app_com_get_check_sum((uint8 *)pall_items, file_num * sizeof(divoom_local_use_pic_save_item));
    divoom_port_file_write(pall_items,file_num * sizeof(divoom_local_use_pic_save_item),  1, file_handle);
    
    divoom_port_file_write(&check_sum,sizeof(check_sum),  1, file_handle);

    divoom_port_file_close(file_handle);
    divoom_system_free(pall_items);
}

void divoom_local_use_pic_load(void)
{
    static uint8 load_flag = 0;
    int i, j, file_num = 0;
    uint32 image_id;
    FILE *file_handle;
    Pdivoom_local_use_pic_save_item pall_items = NULL;
    divoom_manage_image_item_info image_item;
    uint32 check_sum = 0,raw_check_sum;

    if(load_flag == 1)
    {
        return;
    }
    load_flag = 1;
    file_handle = divoom_port_file_open(DIVOOM_DEVICE_NOTICE_SAVE_PATH, "rb");
    if(file_handle == NULL)
    {
        divoom_local_use_pic_save();
        return;
    }

    if(0 == divoom_port_file_read(&file_num,sizeof(file_num),  1, file_handle))
    {
        divoom_port_file_close(file_handle);
        goto  divoom_local_use_pic_load_check_download_file_table;
    }
    check_sum = divoom_app_com_get_check_sum((uint8 *)&file_num, sizeof(file_num));
    if(file_num > 0 && file_num < 10000)
    {
        pall_items = divoom_system_malloc(file_num * sizeof(divoom_local_use_pic_save_item));
        if(0 == divoom_port_file_read(pall_items, file_num * sizeof(divoom_local_use_pic_save_item),  1, file_handle))
        {
            divoom_port_file_close(file_handle);
            goto  divoom_local_use_pic_load_check_download_file_table;
        }
        check_sum += divoom_app_com_get_check_sum((uint8 *)pall_items, file_num * sizeof(divoom_local_use_pic_save_item));
        if(0 == divoom_port_file_read(&raw_check_sum,sizeof(raw_check_sum),  1, file_handle))
        {
            divoom_port_file_close(file_handle);
            goto  divoom_local_use_pic_load_check_download_file_table;
        }
        if(raw_check_sum != check_sum)
        {
            divoom_print("check sum err: %d, %d!\n", raw_check_sum , check_sum);
            file_num = 0;
        }
    }
    divoom_port_file_close(file_handle);
    

    divoom_local_use_pic_load_check_download_file_table:
    if(file_num == 0)
    {
        divoom_print("delete all local images!\n");
        for(i = 0; i < sizeof(divoom_local_use_pic_notice_pic_table)/ sizeof(divoom_local_use_pic_notice_pic_table[0]); i ++)
        {
            image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_NOTICE_NET_PIC + divoom_local_use_pic_notice_pic_table[i].disp_id ;
            divoom_manage_image_del_direct(DIVOOM_MANAGE_IMAGE_NOTICE_NET_GIF, &image_id, 1);
        }
    }
    else
    {
        //先删除不一样的文件，
        for(j = 0; j < file_num; j ++)
        {
            image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_NOTICE_NET_PIC + pall_items[j].disp_id;
            for(i = 0; i < sizeof(divoom_local_use_pic_notice_pic_table)/ sizeof(divoom_local_use_pic_notice_pic_table[0]); i ++)
            {
                if(divoom_local_use_pic_notice_pic_table[i].disp_id == pall_items[j].disp_id)
                {
                    if(divoom_local_use_pic_notice_pic_table[i].version != pall_items[j].version)
                    {
                        divoom_print("delete local image id: %d!\n", pall_items[j].disp_id);
                        divoom_manage_image_del_direct(DIVOOM_MANAGE_IMAGE_NOTICE_NET_GIF, &image_id, 1);
                    }
                    break;
                }
            }
            if(i >= sizeof(divoom_local_use_pic_notice_pic_table)/ sizeof(divoom_local_use_pic_notice_pic_table[0]))
            {
                divoom_manage_image_del_direct(DIVOOM_MANAGE_IMAGE_NOTICE_NET_GIF, &image_id, 1);
            }
        }
    }

    //再添加所有文件
    for(i = 0; i < sizeof(divoom_local_use_pic_notice_pic_table)/ sizeof(divoom_local_use_pic_notice_pic_table[0]); i ++)
    {
        image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_NOTICE_NET_PIC + divoom_local_use_pic_notice_pic_table[i].disp_id;
        image_item.image_id = image_id;
        image_item.utc_time = 0;
        strcpy(image_item.file_addr, divoom_local_use_pic_notice_pic_table[i].file_id);
        image_item.pcallback = divoom_app_com_finishcallback;
        //divoom_print("we will download: %d\n",image_id);
        if(image_id < DIVOOM_MANAGE_IMAGE_ID_BASE_NOTICE_NET_PIC + DIVOOM_lOCAL_USE_END_LOVAL_MSG)
        {
            divoom_manage_image_add(DIVOOM_MANAGE_IMAGE_NOTICE_NET_GIF, &image_item, 1);
        }
        else
        {
            divoom_manage_image_add_font(DIVOOM_MANAGE_IMAGE_NOTICE_NET_GIF, &image_item, 1);
        }
    }
    if(pall_items)
    {
        divoom_system_free(pall_items);
    }
    divoom_local_use_pic_save();
    
}


/*********************************************************************
  Function:        divoom_local_use_pic_get_url_addr
  Description:     获取指定文件的网络地址
  Input:           model_id
                   image_id
                   url_addr
                    
  Return:          image_flag
**********************************************************************/

int divoom_local_use_pic_get_url_addr(int model_id, int image_id, char *url_addr)
{

    int i;

    image_id -= DIVOOM_MANAGE_IMAGE_ID_BASE_NOTICE_NET_PIC;
    for(i = 0; i < sizeof(divoom_local_use_pic_notice_pic_table)/ sizeof(divoom_local_use_pic_notice_pic_table[0]); i++)
    {
        if(divoom_local_use_pic_notice_pic_table[i].disp_id == image_id)
        {
            strcpy(url_addr, divoom_local_use_pic_notice_pic_table[i].file_id);
            break;
        }
    }
    return 1;
}

