#include "divoom.h"

#define DIVOOM_PLAY_LOCAL_GIF_PLAY_TIME 3000
#define DIVOOM_PLAY_LOCAL_GIF_MAX_PIC_NUM     60    //最大缓冲图片张数



typedef struct _divoom_play_local_gif_image_ctrl 
{
    uint8 run_flag;
    uint8 send_flag;
    uint8 read_flag;
    uint8 exit_flag;
    uint8 wait_flag;
    int   read_file_id;
    int   wait_pic_id;
    uint8 time_frame_flag;  //是否增加时间显示
    int   pre_disp_time_info;   //前一次显示时间的时间
    void  *task_handle;
    void  *read_handle;
    uint32 insert_time;        //播放多久将插入一个时间
    uint32 next_disp_time;
    void *image_file_queue;
    void *image_cache_queue;
    void *image_nouse_queue;
    void *read_sem;
    void *read_lock_handle;
    void *gif_read_handle;
    struct tm pre_time_disp_date;
}divoom_play_local_gif_image_ctrl, *Pdivoom_play_local_gif_image_ctrl;


static Pdivoom_play_local_gif_image_ctrl pdivoom_play_local_gif_handle = NULL;

void divoom_play_local_gif_add_time_frame(void);

void divoom_play_local_gif_play_over_callback(void *para, uint8 area_id)
{
    //顺序播放，应该不会调用这个
        
}

/*********************************************************************
  Function:        divoom_play_gif_com_ctrl_time_frame
  Description:     是否增加时间显示//只有动画才使用这个功能，默认ＩＤ为3;
  Input:           on_off   开关选项
  Return:          返回句柄
**********************************************************************/
#define DIVOOM_PLAY_LOCAL_GIF_TIME_FRAME_ID 3
void divoom_play_local_gif_ctrl_time_frame(int on_off)
{
    divoom_image_shade_info image_shade;
    
    if(pdivoom_play_local_gif_handle)
    {
        if(on_off != pdivoom_play_local_gif_handle->time_frame_flag)
        {
            pdivoom_play_local_gif_handle->pre_time_disp_date.tm_hour = 100;
            pdivoom_play_local_gif_handle->pre_time_disp_date.tm_min = 100;
            if(on_off)
            {
            }
            else
            {
                //将关闭
                divoom_image_with_text_disp_set_shade_image(0, NULL);
                pdivoom_play_local_gif_handle->time_frame_flag = 2;
            }
        }
    }
}

void divoom_play_local_gif_add_time_frame(void)
{
    char time_info[10];
    time_t cur_time;
	struct tm cur_date;   
    Pdivoom_text_info ptext_info;
    
    if(pdivoom_play_local_gif_handle->time_frame_flag == 0)
    {
    }
    else if(pdivoom_play_local_gif_handle->time_frame_flag == 2)
    {
        pdivoom_play_local_gif_handle->time_frame_flag = 0;
        divoom_image_with_text_disp_del_text_frame(DIVOOM_PLAY_LOCAL_GIF_TIME_FRAME_ID);
        divoom_image_with_text_disp_set_shade_image(0, NULL);
    }
    else
    {
        cur_time = time(0);
        if(cur_time != pdivoom_play_local_gif_handle->pre_disp_time_info)
        {
	        localtime_r(&cur_time, &cur_date);
            if(cur_date.tm_hour != pdivoom_play_local_gif_handle->pre_time_disp_date.tm_hour ||  cur_date.tm_min != pdivoom_play_local_gif_handle->pre_time_disp_date.tm_min)
            {
                ptext_info = divoom_system_malloc(sizeof(divoom_text_info));
                if(ptext_info)
                {
                }
            }
            pdivoom_play_local_gif_handle->pre_disp_time_info = cur_time;
        }
    }
}

/*********************************************************
  Function:		divoom_play_local_gif_task_run
  Description:	主循环
  Input:			none
  Return:			none
************************************************************/

void *divoom_play_local_gif_task_run(void *arg) //请求设备的命令
{
    Pdivoom_image_info pimage_handle;
    
    while(pdivoom_play_local_gif_handle->run_flag)
    {
        divoom_play_local_gif_add_time_frame();
        if(divoom_image_with_text_disp_get_back_queue() == 0)
        {
            divoom_system_delay_ms(100);
            continue;
        }
        
        if(divoom_queue_items(pdivoom_play_local_gif_handle->image_cache_queue) == 0)
        {
            divoom_system_delay_ms(10);
            continue;
        }
        pimage_handle = divoom_queue_pop(pdivoom_play_local_gif_handle->image_cache_queue);
        if(pimage_handle == NULL)
        {
            divoom_system_delay_ms(10);
            continue;
        }
        divoom_image_with_text_disp_write_image(pimage_handle);
        divoom_queue_push(pdivoom_play_local_gif_handle->image_nouse_queue, pimage_handle);
        divoom_system_put_sem(pdivoom_play_local_gif_handle->read_sem);
    }
    pdivoom_play_local_gif_handle->exit_flag |= 2;
    divoom_print("exit run!\n");
    return NULL;
    
}

void *divoom_play_local_gif_task_read(void *arg) //请求设备的命令
{
    int read_num, first_flag = 0;
    char *file_name;
    Pdivoom_image_info pimage_handle;
    
    while(pdivoom_play_local_gif_handle->run_flag)
    {
        if(divoom_queue_items(pdivoom_play_local_gif_handle->image_file_queue) == 0 || 
            divoom_queue_items(pdivoom_play_local_gif_handle->image_nouse_queue) <= DIVOOM_BACK_QUEUE_MAX + 1)
        {
            divoom_system_get_sem(pdivoom_play_local_gif_handle->read_sem);
        }
        if(pdivoom_play_local_gif_handle->run_flag == 0)
        {
            break;
        }
        divoom_system_get_lock(pdivoom_play_local_gif_handle->read_lock_handle);
        pimage_handle = divoom_queue_pop(pdivoom_play_local_gif_handle->image_nouse_queue);

        if(pdivoom_play_local_gif_handle->wait_flag)
        {
            if(pimage_handle)
            {
                int image_list[] = {
                    DIVOOM_DISP_LOADING_00,
                    DIVOOM_DISP_LOADING_01,
                    DIVOOM_DISP_LOADING_02,
                    DIVOOM_DISP_LOADING_03,
                    DIVOOM_DISP_LOADING_04,
                    DIVOOM_DISP_LOADING_05,
                    DIVOOM_DISP_LOADING_06,
                    DIVOOM_DISP_LOADING_07,
                };
                divoom_disp_picture_get_64rgb_data(image_list[pdivoom_play_local_gif_handle->wait_pic_id%8], pimage_handle->rgb_data);
            	pimage_handle->gif_id = 0;	//0 表示背景图 局部图从1开始
            	pimage_handle->frame_attr = DIVOOM_IMAGE_PIC_TYPE; //数据包类型【起始帧、中间图片帧、结束帧】
            	pimage_handle->speed = 100;	//动画速度
            	pimage_handle->pic_high = 64;	//动画速度
            	pimage_handle->pic_width = 64;	//动画速度
            	pimage_handle->layer = 0; //该图存在哪个层级
            	pdivoom_play_local_gif_handle->wait_pic_id ++;
                divoom_queue_push(pdivoom_play_local_gif_handle->image_cache_queue, pimage_handle);
            }
            divoom_system_free_lock(pdivoom_play_local_gif_handle->read_lock_handle);
            divoom_system_delay_ms(100);
            continue;
        }
        read_num = 0;
        first_flag = 0;
        if(pdivoom_play_local_gif_handle->gif_read_handle != NULL)
        {
            read_num = divoom_gif_lib_get_pic_data(pdivoom_play_local_gif_handle->gif_read_handle, pimage_handle->rgb_data, &pimage_handle->pic_width, &pimage_handle->speed, 1, 0);
            if(read_num == 0)
            {
                //当前文件已经完成
                divoom_gif_lib_exit(pdivoom_play_local_gif_handle->gif_read_handle);
                pdivoom_play_local_gif_handle->gif_read_handle = NULL;
            }
        }
        if(pdivoom_play_local_gif_handle->gif_read_handle == NULL)
        {
            if(pdivoom_play_local_gif_handle->read_file_id == -1)
            {
                first_flag = 1;
            }
            pdivoom_play_local_gif_handle->read_file_id ++;
            pdivoom_play_local_gif_handle->read_file_id = pdivoom_play_local_gif_handle->read_file_id%divoom_queue_items(pdivoom_play_local_gif_handle->image_file_queue);
            file_name = divoom_queue_get_index_item(pdivoom_play_local_gif_handle->image_file_queue, pdivoom_play_local_gif_handle->read_file_id);
            if(file_name == NULL)
            {
                divoom_print("it has no gif file!!\n");
                divoom_system_free_lock(pdivoom_play_local_gif_handle->read_lock_handle);
                continue;
            }
            divoom_print("we will read new gif file: %s\n", file_name);
            pdivoom_play_local_gif_handle->gif_read_handle = divoom_gif_lib_init(file_name,NULL,0);
            if(pdivoom_play_local_gif_handle->gif_read_handle == NULL)
            {
                file_name = divoom_queue_pop_item(pdivoom_play_local_gif_handle->image_file_queue, file_name);
                divoom_print("we will remove the gif file which can't play!%s: %d\n",file_name, divoom_queue_items(pdivoom_play_local_gif_handle->image_file_queue));
                divoom_system_free(file_name);
                read_num = 0;
            }
            else
            {
                read_num = divoom_gif_lib_get_pic_data(pdivoom_play_local_gif_handle->gif_read_handle, pimage_handle->rgb_data, &pimage_handle->pic_width, &pimage_handle->speed, 1, 0);
            }
        }
        pimage_handle->pic_high = pimage_handle->pic_width;
        if(read_num == 0)
        {
            divoom_queue_push(pdivoom_play_local_gif_handle->image_nouse_queue, pimage_handle);
            divoom_print("it fails to read gif frame and check it!\n");
            
        }
        else
        {
            pimage_handle->pic_high = pimage_handle->pic_width;
            pimage_handle->frame_attr = first_flag?DIVOOM_IMAGE_START_TYPE:DIVOOM_IMAGE_PIC_TYPE;
            divoom_queue_push(pdivoom_play_local_gif_handle->image_cache_queue, pimage_handle);
        }
        
        divoom_system_free_lock(pdivoom_play_local_gif_handle->read_lock_handle);
    }
    pdivoom_play_local_gif_handle->exit_flag |= 1;  
    divoom_print("exit read!\n");
    return NULL;
}



/*********************************************************************
  Function:        divoom_play_local_gif_init
  Description:     模块初始化
  Input:           insert_time   插入显示时间的间隔,为0将不显示时间
                   gif_time      动画显示最少时间
  Return:          返回句柄
**********************************************************************/

void divoom_play_local_gif_init(char *file_name)
{
    int i;
    Pdivoom_image_info pimage_item;
    
    if(pdivoom_play_local_gif_handle)
    {
        return;
    }

    pdivoom_play_local_gif_handle = divoom_system_malloc(sizeof(divoom_play_local_gif_image_ctrl));
    if(pdivoom_play_local_gif_handle == NULL)
    {
        return;
    }
    pdivoom_play_local_gif_handle->run_flag = 1;
    pdivoom_play_local_gif_handle->read_file_id = -1;
    pdivoom_play_local_gif_handle->image_file_queue = divoom_queue_init(1000, __func__, __LINE__);
    pdivoom_play_local_gif_handle->image_cache_queue = divoom_queue_init(1000, __func__, __LINE__);
    pdivoom_play_local_gif_handle->image_nouse_queue = divoom_queue_init(1000, __func__, __LINE__);
    pdivoom_play_local_gif_handle->read_sem = divoom_system_create_sem(1, 0);
    pdivoom_play_local_gif_handle->read_lock_handle = divoom_system_create_lock();
    for(i = 0; i < DIVOOM_BACK_QUEUE_MAX * 2; i ++)
    {
        pimage_item = divoom_system_malloc(sizeof(divoom_image_info) +SCREEN_BUF_SIZE_64); //图片裸数据
        pimage_item->rgb_data = (uint8 *)&pimage_item[1];
    	pimage_item->gif_id = 0;	//0 表示背景图 局部图从1开始
    	pimage_item->frame_attr = DIVOOM_IMAGE_PIC_TYPE; //数据包类型【起始帧、中间图片帧、结束帧】
    	pimage_item->speed = 100;	//动画速度
    	pimage_item->pos_x = 0;	//坐标x起始位置
    	pimage_item->pos_y = 0;  //坐标y起始位置
    	pimage_item->layer = 0; //该图存在哪个层级
        divoom_queue_push(pdivoom_play_local_gif_handle->image_nouse_queue, pimage_item);
    }
    divoom_image_with_text_disp_init(NULL);
    divoom_print("start play!\n");
    pdivoom_play_local_gif_handle->read_flag = 0;
    pdivoom_play_local_gif_handle->task_handle = divoom_system_create_task(divoom_play_local_gif_task_run, "local_gif", NULL, DIVOOM_TASK_STACK_LOCAL_GIF_RUN);
    pdivoom_play_local_gif_handle->read_handle = divoom_system_create_task(divoom_play_local_gif_task_read, "local_gif_rd", NULL, DIVOOM_TASK_STACK_LOCAL_GIF_READ);
    if(file_name)
    {
        divoom_disp_picture_display(DIVOOM_DISP_HOME_PAGE_HOT);
        divoom_play_local_gif_add_file(file_name, 0);
    }
}

/*********************************************************************
  Function:        divoom_play_local_gif_wait_notice
  Description:     收到命令，正在等待下载
  Input:           
  Return:          none
**********************************************************************/

void divoom_play_local_gif_wait_notice(void)
{
    if(pdivoom_play_local_gif_handle)
    {
        
        pdivoom_play_local_gif_handle->wait_pic_id = 0;
        pdivoom_play_local_gif_handle->wait_flag = 1;
        divoom_system_get_lock(pdivoom_play_local_gif_handle->read_lock_handle);
        while(divoom_queue_items(pdivoom_play_local_gif_handle->image_cache_queue))
        {
            void *handle = divoom_queue_pop(pdivoom_play_local_gif_handle->image_cache_queue);
            divoom_queue_push(pdivoom_play_local_gif_handle->image_nouse_queue, handle);
        }
        divoom_system_free_lock(pdivoom_play_local_gif_handle->read_lock_handle);
    }
}

/*********************************************************************
  Function:        divoom_play_local_gif_exit_image
  Description:     模块退出，释放所有资源
  Input:           
  Return:          none
**********************************************************************/

void divoom_play_local_gif_add_file(char *file_name, int new_flag)
{
    char *new_file;

    if(pdivoom_play_local_gif_handle && file_name)
    {
        divoom_print("we will play file: %s, flag: %d\n", file_name, new_flag);
        divoom_system_get_lock(pdivoom_play_local_gif_handle->read_lock_handle);
        if(new_flag)
        {
            divoom_queue_pop_all(pdivoom_play_local_gif_handle->image_file_queue, NULL);
            while(divoom_queue_items(pdivoom_play_local_gif_handle->image_cache_queue))
            {
                void *handle = divoom_queue_pop(pdivoom_play_local_gif_handle->image_cache_queue);
                divoom_queue_push(pdivoom_play_local_gif_handle->image_nouse_queue, handle);
            }
            
            pdivoom_play_local_gif_handle->pre_disp_time_info = 0;
            if(pdivoom_play_local_gif_handle->time_frame_flag)
            {
                pdivoom_play_local_gif_handle->time_frame_flag = 0;
                divoom_play_local_gif_ctrl_time_frame(1);
            }
            pdivoom_play_local_gif_handle->read_file_id = -1;
            if(pdivoom_play_local_gif_handle->gif_read_handle)
            {
                divoom_gif_lib_exit(pdivoom_play_local_gif_handle->gif_read_handle);
                pdivoom_play_local_gif_handle->gif_read_handle = NULL;
            }
            if(pdivoom_play_local_gif_handle->wait_flag)
            {
                pdivoom_play_local_gif_handle->wait_flag = 0;
                divoom_image_with_text_disp_del_image(0);
            }
        }
        new_file= divoom_app_com_copy_string(file_name, 0);
        if(new_file)
        {
            divoom_queue_push(pdivoom_play_local_gif_handle->image_file_queue, new_file);
        }
        pdivoom_play_local_gif_handle->wait_flag = 0;
        divoom_system_free_lock(pdivoom_play_local_gif_handle->read_lock_handle);
        divoom_system_put_sem(pdivoom_play_local_gif_handle->read_sem);
    }
}


/*********************************************************************
  Function:        divoom_play_local_gif_exit_image
  Description:     模块退出，释放所有资源
  Input:           
  Return:          none
**********************************************************************/

void divoom_play_local_gif_exit_image(void)
{
    
    if(pdivoom_play_local_gif_handle == NULL)
    {
        return ;
    }
    pdivoom_play_local_gif_handle->exit_flag = 0;
    pdivoom_play_local_gif_handle->run_flag = 0;
    divoom_system_put_sem(pdivoom_play_local_gif_handle->read_sem);
    divoom_system_delay_ms(1);
    while(pdivoom_play_local_gif_handle->exit_flag != 3)
    {
        divoom_system_delay_ms(10);
    }
    divoom_print("star exit!\n");
    divoom_image_with_text_disp_exit();
    divoom_print("end play!\n");
    
    divoom_system_destroy_sem(pdivoom_play_local_gif_handle->read_sem);
    divoom_system_destroy_lock(pdivoom_play_local_gif_handle->read_lock_handle);
    divoom_queue_destroy(pdivoom_play_local_gif_handle->image_file_queue, NULL);
    divoom_queue_destroy(pdivoom_play_local_gif_handle->image_cache_queue, NULL);
    divoom_queue_destroy(pdivoom_play_local_gif_handle->image_nouse_queue, NULL);
    divoom_system_free(pdivoom_play_local_gif_handle);
    pdivoom_play_local_gif_handle = NULL;
    divoom_print("end play!\n");
}


