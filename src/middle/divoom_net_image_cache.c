#include "divoom.h"

#define DIVOOM_NET_IMAGE_CACHE_TEST

#define DIVOOM_NET_IMAGE_CACHE_MAX_RAM_SIZE   (3*1024*1024)     //下载的最大缓冲区

/*
已经下载的队列，每次有读取的话，都先将元素弹出再压，让最后使用的在队列尾，
当下载缓冲区达到限制时，停止等待下载的下载，如果有新文件请求，不在下载队列中，将从下载队列中弹出元素，每次弹出两个
将弹出的资源释放，再压入等待下载下载队列，再开始下载需要请求的元素,
当缓冲区满后，再移除元素后，空间释放，要重新启动下载线程
*/

typedef struct _divoom_net_image_cache_image_item
{
    int     model_id;       //资源所属于的模块
    int     flag;           //使用状态，０：未使用，１：正在使用
    int     pic_id;         //文件的ＩＤ,如果为-1就无效
    char    file_addr[52];  //文件的地址，与ＩＤ是互斥
    int     divoom_flag;    //唯一标识，用于后面初始化字体,以系统的ms为单位的一个标识
    Pdivoom_http_download_image_file_info pimage_handle;
}divoom_net_image_cache_image_item, *Pdivoom_net_image_cache_image_item;


typedef struct _divoom_net_image_cache_ctrl
{
    uint8 run_flag;         //运行标记
    uint8 net_status;       //网络状态
    int cache_size;         //缓冲区大小
    void *download_sem;     //下载信号量
    void *task_handle;      //下载任务句柄
    void *queue_cache;      //已经下载完图片的队列
    void *queue_no_cache;   //等待下载的图片的队列
    Pdivoom_net_image_cache_image_item pdownloading;    //正在下载
}divoom_net_image_cache_ctrl, *Pdivoom_net_image_cache_ctrl;


static Pdivoom_net_image_cache_ctrl pdivoom_net_image_cache_handle = NULL;


/*********************************************************************
  Function:        divoom_net_image_cache_free_image
  Description:     释放图像相关资源
  Input:            pimage_info
                    
  Return:           none
**********************************************************************/

void divoom_net_image_cache_free_image(Pdivoom_net_image_cache_image_item pimage_info)
{
    if(pimage_info)
    {
        if(pimage_info->pimage_handle)
        {
            divoom_http_download_image_free_image_handle(pimage_info->pimage_handle);
            pimage_info->pimage_handle = NULL;
        }
        divoom_system_free(pimage_info);
    }
}

/*********************************************************************
  Function:        divoom_net_image_cache_compare_no_user
  Description:     通过模块图像的未使用取得图像
  Input:            item1
                    item２
  Return:           相同返回0,否则返回1
**********************************************************************/

int divoom_net_image_cache_compare_no_user(void *item1, void *item2)
{
    int *mode_id = (int *)item1;
    Pdivoom_net_image_cache_image_item com_item;

    com_item = (Pdivoom_net_image_cache_image_item)item2;
    if(0 == com_item->flag && *mode_id == com_item->model_id)
    {
        return 0;
    }
    return 1;
}


/*********************************************************************
  Function:        divoom_net_image_cache_compare_model_id
  Description:      得到某模块的的数据
  Input:            item1
                    item２
  Return:           相同返回0,否则返回1
**********************************************************************/

int divoom_net_image_cache_compare_model_id(void *item1, void *item2)
{
    int *mode_id = (int *)item1;
    Pdivoom_net_image_cache_image_item com_item;

    com_item = (Pdivoom_net_image_cache_image_item)item2;
    if(*mode_id == com_item->model_id)
    {
        return 0;
    }
    return 1;
}


/*********************************************************************
  Function:        divoom_net_image_cache_compare_id
  Description:     通过图像的ＩＤ比较图像
  Input:            item1
                    item２
  Return:           相同返回0,否则返回1
**********************************************************************/

int divoom_net_image_cache_compare_id(void *item1, void *item2)
{
    int *id = (int *)item1;
    Pdivoom_net_image_cache_image_item com_item;

    com_item = (Pdivoom_net_image_cache_image_item)item2;
    if(*id == com_item->pic_id)
    {
        return 0;
    }
    return 1;
}


/*********************************************************************
  Function:        divoom_net_image_cache_compare_file_addr
  Description:     通过图像的地址比较图像
  Input:            item1
                    item２
  Return:           相同返回0,否则返回非0
**********************************************************************/


int divoom_net_image_cache_compare_file_addr(void *item1, void *item2)
{
    char *src_str = (char *)item1;
    Pdivoom_net_image_cache_image_item com_item;

    com_item = (Pdivoom_net_image_cache_image_item)item2;
    return strcmp(src_str, com_item->file_addr);
}


/*********************************************************************
  Function:        divoom_net_image_cache_get_image_handle
  Description:     获取图像的相关数据指针,此函数可能会造成一定的阻塞,外面只能使用这个结构，但是不能释放与写入
  Input:            id          服务器的图像id,为-1时使用file_addr
                    file_addr   服务器的图像网络地址
  Return:           返回图像缓冲区
**********************************************************************/


Pdivoom_http_download_image_file_info divoom_net_image_cache_get_image_handle(int id, char *file_addr)
{
    Pdivoom_net_image_cache_image_item pfind = NULL;
    
    if(id == -1)
    {
        pfind = divoom_queue_pop_cmp(pdivoom_net_image_cache_handle->queue_cache, file_addr, divoom_net_image_cache_compare_file_addr);
    }
    else
    {
        pfind = divoom_queue_pop_cmp(pdivoom_net_image_cache_handle->queue_cache, &id, divoom_net_image_cache_compare_id);
    }

    if(pfind == NULL)
    {
        pfind = divoom_queue_pop_cmp(pdivoom_net_image_cache_handle->queue_no_cache, &id, divoom_net_image_cache_compare_id);
        if(pfind == NULL)
        {
            //正在下载,等系统下载完
            if(pdivoom_net_image_cache_handle->pdownloading->pic_id == id ||
                strcmp(pdivoom_net_image_cache_handle->pdownloading->file_addr, file_addr)==0)
            {
                //不断查询已下载的列表
                uint32 time_out_tick = 400;
                
                while(pfind == NULL && time_out_tick)
                {
                    if(id == -1)
                    {
                        pfind = divoom_queue_pop_cmp(pdivoom_net_image_cache_handle->queue_cache, file_addr, divoom_net_image_cache_compare_file_addr);
                    }
                    else
                    {
                        pfind = divoom_queue_pop_cmp(pdivoom_net_image_cache_handle->queue_cache, &id, divoom_net_image_cache_compare_id);
                    }
                    divoom_system_delay_ms(10);
                    time_out_tick --;
                }
                if(pfind == NULL)
                {
                    divoom_print("%s:%d  we time out to get the id: %d, file: %s\n", __FUNCTION__, __LINE__, id, file_addr?file_addr:"");
                    return NULL;
                }
                divoom_queue_push(pdivoom_net_image_cache_handle->queue_cache, pfind);
            }
            else
            {
                //出错，
                divoom_print("%s:%d  we can't find the id: %d, file: %s\n", __FUNCTION__, __LINE__, id, file_addr?file_addr:"");
                return NULL;
            }
        }
        else
        {
            //我们将下载，再添加到下载队列
            pfind->pimage_handle = divoom_http_download_image_download_image(pfind->file_addr);
            if(pfind->pimage_handle)
            {
                pdivoom_net_image_cache_handle->cache_size += pfind->pimage_handle->cache_szie;
                divoom_queue_push(pdivoom_net_image_cache_handle->queue_cache, pfind);
            }
        }
    }
    else
    {
        divoom_queue_push(pdivoom_net_image_cache_handle->queue_cache, pfind);
    }

    return pfind->pimage_handle;
}



/*********************************************************************
  Function:        divoom_net_image_cache_add_addr_item
  Description:     向模块添加一个文件，启动下载
  Input:            mode_id          服务器的图像id,为-1时使用file_addr
                    id          方便查找
                    file_addr   服务器的图像网络地址
  Return:           NONE
**********************************************************************/


void divoom_net_image_cache_add_addr_item(int mode_id, int id, char *file_addr)
{
    Pdivoom_net_image_cache_image_item new_item;

    new_item = divoom_system_malloc(sizeof(divoom_net_image_cache_image_item));
    if(new_item == NULL)
    {
        return;
    }
    memset(new_item, 0, sizeof(divoom_net_image_cache_image_item));
    new_item->model_id = mode_id;
    new_item->pic_id = id;
    if(file_addr != NULL)
    {
        strcpy(new_item->file_addr, file_addr);
    }
    else
    {
        new_item->file_addr[0] = 0;
    }
    new_item->flag = 1;
    new_item->divoom_flag = divoom_system_get_tickcount();
    divoom_queue_push(pdivoom_net_image_cache_handle->queue_no_cache, new_item);
    divoom_system_put_sem(pdivoom_net_image_cache_handle->download_sem);
}



/*********************************************************************
  Function:        divoom_net_image_cache_clear_mode
  Description:     清除模块,只是设置清除标记
  Input:            mode_id          模块ＩＤ
  Return:           NONE
**********************************************************************/

void divoom_net_image_cache_clear_mode(int mode_id)
{
    uint32 i, tol_items;
    Pdivoom_net_image_cache_image_item pFindItem;

    //先修改未下载队列的
    tol_items = divoom_queue_get_reset(pdivoom_net_image_cache_handle->queue_no_cache);
    for(i = 0; i < tol_items; i++)
    {
        pFindItem = divoom_queue_get_index_item(pdivoom_net_image_cache_handle->queue_no_cache, i);
        if(pFindItem && pFindItem->model_id == mode_id)
        {
            pFindItem->flag = 0;
        }
    }

    //修改下载队列
    tol_items = divoom_queue_get_reset(pdivoom_net_image_cache_handle->queue_cache);
    for(i = 0; i < tol_items; i++)
    {
        pFindItem = divoom_queue_get_index_item(pdivoom_net_image_cache_handle->queue_cache, i);
        if(pFindItem && pFindItem->model_id == mode_id)
        {
            pFindItem->flag = 0;
        }
    }

}

/*********************************************************************
  Function:        divoom_net_image_cache_upate_mode
  Description:     设置使用标记
  Input:            mode_id          
                    id              服务器的图像id,为-1时使用file_addr
                    file_addr       文件地址
  Return:           NONE
**********************************************************************/

void divoom_net_image_cache_upate_mode(int mode_id, int id, char *file_addr)
{
    Pdivoom_net_image_cache_image_item pfind;
    
    if(id == -1)
    {
        pfind = divoom_queue_get_item(pdivoom_net_image_cache_handle->queue_cache, file_addr, divoom_net_image_cache_compare_file_addr);
    }
    else
    {
        pfind = divoom_queue_get_item(pdivoom_net_image_cache_handle->queue_cache, &id, divoom_net_image_cache_compare_id);
    }
    if(pfind == NULL)
    {
        if(id == -1)
        {
            pfind = divoom_queue_get_item(pdivoom_net_image_cache_handle->queue_no_cache, file_addr, divoom_net_image_cache_compare_file_addr);
        }
        else
        {
            pfind = divoom_queue_get_item(pdivoom_net_image_cache_handle->queue_no_cache, &id, divoom_net_image_cache_compare_id);
        }
    }
    if(pfind)
    {
        pfind->flag = 1;
    }
    else
    {
        divoom_print("%s:%d  we can't find the id: %d, file: %s\n", __FUNCTION__, __LINE__, id, file_addr?file_addr:"");
    }
}


/*********************************************************************
  Function:        divoom_net_image_cache_upate_end
  Description:     结束模块更新，将这个模块没有使用的数据全部清除
  Input:            mode_id          
                    
  Return:           NONE
**********************************************************************/

void divoom_net_image_cache_upate_end(int mode_id)
{
    Pdivoom_net_image_cache_image_item pfind;

    do{        
        pfind = divoom_queue_pop_cmp(pdivoom_net_image_cache_handle->queue_cache, &mode_id, divoom_net_image_cache_compare_no_user);
        if(pfind)
        {
            pdivoom_net_image_cache_handle->cache_size -= pfind->pimage_handle->cache_szie;
        }
        divoom_net_image_cache_free_image(pfind);
    }while(pfind);
    
    do{        
        pfind = divoom_queue_pop_cmp(pdivoom_net_image_cache_handle->queue_no_cache, &mode_id, divoom_net_image_cache_compare_no_user);
        divoom_net_image_cache_free_image(pfind);
    }while(pfind);
    
    divoom_system_put_sem(pdivoom_net_image_cache_handle->download_sem);
}


/*********************************************************************
  Function:        divoom_net_image_cache_remove_mode
  Description:     清除这个模块的所有的数据
  Input:            mode_id          
                    
  Return:           NONE
**********************************************************************/

void divoom_net_image_cache_remove_mode(int mode_id)
{
    Pdivoom_net_image_cache_image_item pfind;

    do{        
        pfind = divoom_queue_pop_cmp(pdivoom_net_image_cache_handle->queue_cache, &mode_id, divoom_net_image_cache_compare_model_id);
        if(pfind)
        {
            pdivoom_net_image_cache_handle->cache_size -= pfind->pimage_handle->cache_szie;
        }
        divoom_net_image_cache_free_image(pfind);
    }while(pfind);
    
    do{        
        pfind = divoom_queue_pop_cmp(pdivoom_net_image_cache_handle->queue_no_cache, &mode_id, divoom_net_image_cache_compare_model_id);
        divoom_net_image_cache_free_image(pfind);
    }while(pfind);
    
    divoom_system_put_sem(pdivoom_net_image_cache_handle->download_sem);
}


/*********************************************************************
  Function:        divoom_net_image_cache_remove_image
  Description:     清除某个图像的数据
  Input:            id          
                    file_addr
  Return:           NONE
**********************************************************************/

void divoom_net_image_cache_remove_image(int id, char *file_addr)
{

    Pdivoom_net_image_cache_image_item pfind;
    
    if(id == -1)
    {
        pfind = divoom_queue_pop_cmp(pdivoom_net_image_cache_handle->queue_cache, file_addr, divoom_net_image_cache_compare_file_addr);
    }
    else
    {
        pfind = divoom_queue_pop_cmp(pdivoom_net_image_cache_handle->queue_cache, &id, divoom_net_image_cache_compare_id);
    }

    if(pfind == NULL)
    {
        
        if(id == -1)
        {
            pfind = divoom_queue_pop_cmp(pdivoom_net_image_cache_handle->queue_no_cache, file_addr, divoom_net_image_cache_compare_file_addr);
        }
        else
        {
            pfind = divoom_queue_pop_cmp(pdivoom_net_image_cache_handle->queue_no_cache, &id, divoom_net_image_cache_compare_id);
        }
    }
    else
    {
        pdivoom_net_image_cache_handle->cache_size -= pfind->pimage_handle->cache_szie;
    }
    divoom_net_image_cache_free_image(pfind);
    
    divoom_system_put_sem(pdivoom_net_image_cache_handle->download_sem);
}

void *divoom_net_image_cache_task_run(void *param)
{
    while(pdivoom_net_image_cache_handle->run_flag)
    {
        divoom_system_get_sem(pdivoom_net_image_cache_handle->download_sem);
        while(pdivoom_net_image_cache_handle->cache_size < DIVOOM_NET_IMAGE_CACHE_MAX_RAM_SIZE && pdivoom_net_image_cache_handle->net_status)
       {
            
            pdivoom_net_image_cache_handle->pdownloading = divoom_queue_pop(pdivoom_net_image_cache_handle->queue_no_cache);
            if(pdivoom_net_image_cache_handle->pdownloading == NULL)
            {
                break;
            }
            pdivoom_net_image_cache_handle->pdownloading->pimage_handle = divoom_http_download_image_download_image(pdivoom_net_image_cache_handle->pdownloading->file_addr);
            if(pdivoom_net_image_cache_handle->pdownloading->pimage_handle)
            {
                divoom_print("image(id: %d) have been downloaded!\n", pdivoom_net_image_cache_handle->pdownloading->pic_id);
                pdivoom_net_image_cache_handle->cache_size += pdivoom_net_image_cache_handle->pdownloading->pimage_handle->cache_szie;
                divoom_queue_push(pdivoom_net_image_cache_handle->queue_cache, pdivoom_net_image_cache_handle->pdownloading);
                divoom_system_delay_ms(10);
            }
            else
            {
                divoom_queue_push(pdivoom_net_image_cache_handle->queue_no_cache, pdivoom_net_image_cache_handle->pdownloading);
                divoom_print("image(id: %d) fails to downloaded!\n", pdivoom_net_image_cache_handle->pdownloading->pic_id);
                divoom_system_delay_ms(100);
            }
            
        }
    }

    return NULL;
}


/*********************************************************************
  Function:        divoom_net_image_cache_update_net_status
  Description:     更新网络状况
  Input:            
  Return:           NONE
**********************************************************************/


void divoom_net_image_cache_update_net_status(uint8 net_ok)
{
    if(pdivoom_net_image_cache_handle == NULL)
    {
        return ;
    }
    pdivoom_net_image_cache_handle->net_status = net_ok;
    if(net_ok)
    {
        divoom_system_put_sem(pdivoom_net_image_cache_handle->download_sem);
    }
}

/*********************************************************************
  Function:        divoom_net_image_cache_init
  Description:     模块初始化操作
  Input:            
  Return:           NONE
**********************************************************************/
void divoom_net_image_cache_test(void);

void divoom_net_image_cache_init(void)
{
    if(pdivoom_net_image_cache_handle)
    {
        return ;
    }

    pdivoom_net_image_cache_handle = divoom_system_malloc(sizeof(divoom_net_image_cache_ctrl));
    pdivoom_net_image_cache_handle->run_flag = 1;
    pdivoom_net_image_cache_handle->net_status = 0;
    pdivoom_net_image_cache_handle->queue_cache = divoom_queue_init(1000, __func__, __LINE__);
    pdivoom_net_image_cache_handle->queue_no_cache = divoom_queue_init(1000, __func__, __LINE__);
    pdivoom_net_image_cache_handle->download_sem = divoom_system_create_sem(1, 0);
    pdivoom_net_image_cache_handle->task_handle = divoom_system_create_task(divoom_net_image_cache_task_run, "img_cache", NULL, DIVOOM_TASK_STACK_NET_IMG_CACHE);
   // divoom_net_image_cache_test();
}


#ifndef DIVOOM_NET_IMAGE_CACHE_TEST

typedef struct _divoom_net_image_cache_test_data
{
    int id;
    char file_name[52];
}divoom_net_image_cache_test_data, *Pdivoom_net_image_cache_test_data;

static const divoom_net_image_cache_test_data small_image_test_table[] = 
{
    
    {936509,    "group1/M00/AA/6D/L1ghbl_7luaEaq3EAAAAAPB-Zv44368631"},
    {936187,    "group1/M00/AB/36/eEwpPV_7ZgGEAHQTAAAAACZy-zs6072243"},
    {935487,    "group1/M00/AB/28/eEwpPV_7LoCEGb0SAAAAALzkqCY5368991"},
    {935375,    "group1/M00/AB/26/eEwpPV_7Jq6EeOusAAAAAOPJ_fE9204251"},
    {934835,    "group1/M00/AA/44/L1ghbl_6-jiER-SxAAAAAJslQ7E1807867"},
    {934821,    "group1/M00/AB/19/eEwpPV_6-S2EbIBpAAAAAMcY0e02220688"},
    {934815,    "group1/M00/AA/44/L1ghbl_69q-EHwteAAAAAC6zZFw7246319"},
    {934807,    "group1/M00/AA/43/L1ghbl_69dqEDsSSAAAAAO3xC8c1098235"},
    {934597,    "group1/M00/AA/3E/L1ghbl_64v2EY3SxAAAAAO_cAbY1147904"},
    {934463,    "group1/M00/AA/3A/L1ghbl_60wuELlGAAAAAAOoAX-M3456519"},
    {934171,    "group1/M00/AA/33/L1ghbl_6to-EbAMPAAAAAHJX2Gw1893076"},
    {934125,    "group1/M00/AA/31/L1ghbl_6ruqEJndvAAAAACNxG5M9100518"},
    {933969,    "group1/M00/AA/FE/eEwpPV_6is6EXsuMAAAAAFQ32nU1863794"},
    {933691,    "group1/M00/AA/1D/L1ghbl_6XeSET7zHAAAAAE2122U8341934"},
    {933635,    "group1/M00/AA/F0/eEwpPV_6U_aEZ77rAAAAAEDykRc6728313"},
    {933563,    "group1/M00/AA/19/L1ghbl_6SeqEPDs3AAAAAERpPmU2678439"},
    {933555,    "group1/M00/AA/18/L1ghbl_6SJWEdFtbAAAAACxvgVY9002975"},
    {933457,    "group1/M00/AA/15/L1ghbl_6OnOEEMFCAAAAAFoUbBs8905769"},
    {933417,    "group1/M00/AA/E9/eEwpPV_6NeCEYwoVAAAAAKxCTWQ7508770"},
    {933223,    "group1/M00/AA/10/L1ghbl_6JcmEAU33AAAAAAp8MVs8783214"},
    {933079,    "group1/M00/AA/E2/eEwpPV_6GPuETbKNAAAAAGNX1vg5645305"},
    {933053,    "group1/M00/AA/E0/eEwpPV_6FLeEBJe3AAAAAHNIZZ85963300"},
    {932679,    "group1/M00/AA/D9/eEwpPV_5-DeEUHAnAAAAALBNyWI5555674"},
    {932671,    "group1/M00/AA/03/L1ghbl_59keEPRDXAAAAAA55Kek3548539"},
    {932587,    "group1/M00/AA/D7/eEwpPV_57zaEcDJqAAAAAE3iynw6479870"},
    {932249,    "group1/M00/AA/CF/eEwpPV_50zeEHTGEAAAAAE9tZ4s4635928"},
    {931813,    "group1/M00/A9/EF/L1ghbl_5qeKEZqbKAAAAAOUDx902125123"},
    {931561,    "group1/M00/A9/E9/L1ghbl_5kpqEEm_gAAAAAGbXVGA5379229"},
    {931517,    "group1/M00/A9/E7/L1ghbl_5jIeEfMXLAAAAAAEm__03184046"},
    {931401,    "group1/M00/AA/BA/eEwpPV_5gS6ETKzuAAAAAC-_KNE7345554"},
    {931307,    "group1/M00/A9/E2/L1ghbl_5djyERTYVAAAAADF8xok1508502"},
    {931271,    "group1/M00/A9/E1/L1ghbl_5cveEYtsEAAAAAIMiM_M6259075"},
    {931141,    "group1/M00/AA/B2/eEwpPV_5YqmEBxz6AAAAACXM_IA6822136"},
    {930943,    "group1/M00/A9/D5/L1ghbl_5QvGETGdlAAAAAMbPyV04613157"},
    {930729,    "group1/M00/AA/A4/eEwpPV_5KBiEM9IlAAAAAN5A5mc4524690"},
    {930663,    "group1/M00/AA/A1/eEwpPV_5HROEMMsAAAAAAM9LGKg4807246"},
    {930583,    "group1/M00/AA/9E/eEwpPV_5EO6EJmUxAAAAADc1wK82142167"},
    {930425,    "group1/M00/A9/C4/L1ghbl_4_bSEXdOJAAAAAK0Yc7I9238606"},
    {930317,    "group1/M00/A9/BF/L1ghbl_468yENnqcAAAAADV9npg3507917"},
    {930127,    "group1/M00/A9/BA/L1ghbl_41bmEFdgwAAAAAHEumQs6953725"},
    {929925,    "group1/M00/AA/8A/eEwpPV_4wRGEMo4HAAAAAElZjvY2462294"},
    {929907,    "group1/M00/A9/B4/L1ghbl_4v6eEevHyAAAAAKzGDK85270002"},
    {928981,    "group1/M00/A9/A1/L1ghbl_4d2iEShR_AAAAAPW0D6Q0134185"},
    {928973,    "group1/M00/AA/76/eEwpPV_4dsKEQSZxAAAAAOti9f04221510"},
    {928821,    "group1/M00/AA/72/eEwpPV_4Zz-EPFm5AAAAABI-Y880924877"},
    {928735,    "group1/M00/A9/9B/L1ghbl_4YWOEZTg0AAAAAHm50CA3698342"},
    {928411,    "group1/M00/AA/69/eEwpPV_4R3WEJhOwAAAAAEG0PxE5511662"},
    {928252,    "group1/M00/AA/64/eEwpPV_4MsyED8KWAAAAAKO7_P02506994"},
    {928143,    "group1/M00/A9/8B/L1ghbl_4JSOEQrd5AAAAACN088I7957788"},
    {928119,    "group1/M00/AA/60/eEwpPV_4I-OEOKrgAAAAADqVPfc4672896"}
    
    
    
};


static const divoom_net_image_cache_test_data big_image_test_table[] = 
{    
    {936533,    "group1/M00/AA/6E/L1ghbl_7mw-ECloJAAAAAP8udyM7482698"},
    {936501,    "group1/M00/AB/42/eEwpPV_7ld2Ebv7OAAAAACoteqk3560822"},
    {936433,    "group1/M00/AA/6A/L1ghbl_7itCEWADbAAAAAIvs2WM4517596"},
    {936357,    "group1/M00/AA/68/L1ghbl_7gGiELbqNAAAAAJ6pRok0169289"},
    {936275,    "group1/M00/AA/64/L1ghbl_7cRyEbEeSAAAAAP3P0889594979"},
    {936207,    "group1/M00/AA/62/L1ghbl_7aAyEOOfgAAAAAFeR4Ck7764184"},
    {935977,    "group1/M00/AA/5D/L1ghbl_7Vd6EePf-AAAAAE3zOEU1342340"},
    {935657,    "group1/M00/AB/2B/eEwpPV_7OwCEclwrAAAAAGQwJiQ5068506"},
    {935587,    "group1/M00/AA/55/L1ghbl_7NvOEZMZXAAAAAMVNquc4084352"},
    {935257,    "group1/M00/AA/4D/L1ghbl_7HCCEYg9eAAAAAMLmOdk7118725"},
    {935255,    "group1/M00/AB/23/eEwpPV_7HACEUOvCAAAAAGX_RUU0650679"},
    {935213,    "group1/M00/AB/22/eEwpPV_7GWCEI2JWAAAAAN7cYzY5785319"},
    {935185,    "group1/M00/AB/22/eEwpPV_7F5SEHGh1AAAAAOhWR6U2588436"},
    {935173,    "group1/M00/AB/21/eEwpPV_7F0qEGEZtAAAAAPJyKcA6851649"},
    {935063,    "group1/M00/AB/1F/eEwpPV_7DUiENqeGAAAAAN_gCZM9928515"},
    {934949,    "group1/M00/AB/1C/eEwpPV_7A4OESAFOAAAAANm9cY44022521"},
    {934805,    "group1/M00/AB/19/eEwpPV_69ciEdTvJAAAAALFtRvw0503496"},
    {934783,    "group1/M00/AA/43/L1ghbl_68vOENajUAAAAAC3GzX82514278"},
    {934751,    "group1/M00/AA/42/L1ghbl_676qEQMUeAAAAAGxv_V42864512"},
    {934677,    "group1/M00/AA/40/L1ghbl_66qqEQyXIAAAAABRSU7Y3486047"},
    {934137,    "group1/M00/AB/07/eEwpPV_6sCqEM6uxAAAAAJyZaYw2698775"},
    {934063,    "group1/M00/AB/03/eEwpPV_6os2EIFg1AAAAAL5Scyc6291629"},
    {934053,    "group1/M00/AA/2D/L1ghbl_6oCSEQlhfAAAAAAKgR5w7348143"},
    {933963,    "group1/M00/AA/28/L1ghbl_6igyEfxoEAAAAAF16-K43197598"},
    {933757,    "group1/M00/AA/F7/eEwpPV_6bo-EfK8fAAAAAOyYqVw3156337"},
    {933657,    "group1/M00/AA/F1/eEwpPV_6V62EN8SRAAAAAGe9Kas9313515"},
    {933619,    "group1/M00/AA/F0/eEwpPV_6Ug-EGiRBAAAAAKH99DU7104004"},
    {933569,    "group1/M00/AA/19/L1ghbl_6SxmELeruAAAAAAaWPD44028201"},
    {933511,    "group1/M00/AA/17/L1ghbl_6QpKEKri3AAAAAN76-gs0546781"},
    {933499,    "group1/M00/AA/17/L1ghbl_6QeSEcGOvAAAAAObxR0E5398278"},
    {933493,    "group1/M00/AA/16/L1ghbl_6P_SEajCnAAAAANpBmck7722379"},
    {933359,    "group1/M00/AA/12/L1ghbl_6MGmERzjIAAAAADN8N1k9108830"},
    {933335,    "group1/M00/AA/E7/eEwpPV_6Lm-EKT1CAAAAAGRbe1Q6912284"},
    {933329,    "group1/M00/AA/E7/eEwpPV_6LkuEbl8EAAAAAKfv9rQ3255526"},
    {933325,    "group1/M00/AA/E7/eEwpPV_6LheEZh3zAAAAAMCEXEs5780897"},
    {933321,    "group1/M00/AA/E7/eEwpPV_6LdWEDIRnAAAAAIyOoFQ4709226"},
    {933305,    "group1/M00/AA/E7/eEwpPV_6LDuEYuyfAAAAAB4rA8c8305786"},
    {933187,    "group1/M00/AA/0F/L1ghbl_6I0aEFlD0AAAAAHpdPoU3646666"},
    {933139,    "group1/M00/AA/E3/eEwpPV_6IEeEHaVyAAAAAH3tBYU5312611"},
    {932847,    "group1/M00/AA/DC/eEwpPV_6BD6EHvPDAAAAAHmh-d83510248"},
    {932823,    "group1/M00/AA/06/L1ghbl_6Ah-ECRvAAAAAAMIMIqQ6822850"},
    {932803,    "group1/M00/AA/DB/eEwpPV_6AMqERX3sAAAAAOVpUYw7808280"},
    {932677,    "group1/M00/AA/D9/eEwpPV_5982Eey0WAAAAACTX6k81214076"},
    {932551,    "group1/M00/AA/01/L1ghbl_57P2EGBVmAAAAAKO9pO43943521"},
    {932401,    "group1/M00/AA/D2/eEwpPV_53lCEbjpzAAAAANwLxo88598755"},
    {932341,    "group1/M00/AA/D1/eEwpPV_52TiERG8DAAAAADQMe7k2418547"},
    {932253,    "group1/M00/A9/FA/L1ghbl_506iEIAGjAAAAAFIOX2o4604672"},
    {932077,    "group1/M00/AA/CB/eEwpPV_5xGiEeqtPAAAAANqOozA2940759"},
    {931973,    "group1/M00/AA/C9/eEwpPV_5ul6EaHn1AAAAANHDMm40125336"},
    {931935,    "group1/M00/AA/C8/eEwpPV_5ttyEZ6WJAAAAAFVT73c6419419"}
};

/*
测试相关的功能接口
*/

void divoom_net_image_cache_test(void)
{
    divoom_net_image_cache_add_addr_item(0, small_image_test_table[0].id, small_image_test_table[0].file_name);
    divoom_net_image_cache_add_addr_item(1, big_image_test_table[0].id, big_image_test_table[0].file_name);
}

#endif

