#include "divoom.h"

#define INFLOW_TEXT_FRAME_NUM	 4
#define INFLOW_TEXT_SLIP_DOT    6
#define INFLOW_TEXT_SPACE_DOT   4
#define INFLOW_TASK_TIME	5
#define INFLOW_SMALL_IMAGE_QUEUE_NUM 20 //暂时定20个
#define INFLOW_SMALL_IMAGE_NUM 4
#define INFLOW_GIF_DIS_TOTAL_TIME 10000  //10s
#define INFLOW_TEXT_X_POS	18
#define INFLOW_TEXT_FRAME_WIDTH	64 -18
#define INFLOW_TEXT_FRAME_HEIGHT 16
#define INFLOW_IMAGE_WIDTH_64 64

typedef struct _divoom_inflow_font_info
{
	T_U8 font_size;    //16x 24x 32x
	T_U16 font_speed;  //文字速度
	T_U16 font_index;  //控制文字滚动速度
	T_U16 font_move_step; //一次移动几步，设备这边微任务最快一次也需要50ms，当app设置的时间小于50ms时，让文字一次移动多格来处理
	T_U8 font_effect;  //上下左右滚动
	divoom_screen_color_info font_disp_color; //文字显示的颜色
}divoom_inflow_font_info, *Pdivoom_inflow_font_info;

typedef struct _divoom_inflow_frame_info
{
	T_U8 frame_width;   //框的宽度
	T_U8 frame_high;    //框的高度
	T_U8 frame_x; 		//x轴
	T_U8 frame_y; 		//y轴
	T_U8 frame_queue_index; //记录当前显示的是第几个文本框
	T_U32 frame_total_time; //该文本框显示总时间的大小
}divoom_inflow_frame_info, *Pdivoom_inflow_frame_info;

typedef struct _divoom_inflow_gif_info
{
	T_U16 gif_speed; //动画播放速度
	T_U32 gif_first_tick_time; //记录动画最开始的时间
	T_U32 gif_speed_time; //利用时间来控制动画显示时间
	T_U8 pic_index; //记录显示该动画的第几张图
	T_U32 gif_total_time; //该动画显示总时间的大小
	T_U8 gif_queue_index; //记录当前显示的是哪个队列里面的图
}divoom_inflow_gif_info, *Pdivoom_inflow_gif_info;

typedef struct _divoom_inflow_disp_word_info
{
	T_U8 word_num;      //总共要显示多少个字
	T_U8 word_id;       //正在显示的字的下标
	T_U8 col_id;        //显示的字的第几行
	T_U8 frist_word_col_pos; //记录第一个字的第一行所在的坐标位置
	T_U8 cur_disp_word_length; //统计当前显示的字占多宽
	T_U8 roll_flag;		//滚动标志 1不动 0 滚动
	T_U16 *unicode_str; //动态分配内存，需要多少开辟多少内存
	T_U8 space_step;
	T_U8 end_tick;
	T_U8 font_num; //存放uft8_str 的字体数量
	T_U32 cur_screen_info[SCREEN_WIDTH_64]; //从字符串数组里面取数据显示，然后把这个buf覆盖在disp_pic_buf上
}divoom_inflow_disp_word_info, *Pdivoom_inflow_disp_word_info;

typedef struct _divoom_inflow_text_frame_info
{
	T_U8 dis_flag; //1表示该框有文本显示 0表示该框没有显示
	divoom_inflow_frame_info frame_info;
	divoom_inflow_font_info font_info;
	divoom_inflow_disp_word_info disp_word_info;
	//DIVOOM_IMAGE_ENCODE_WORD_FONT word_info[DIVOOM_WORD_MAX_NUM];   //先暂时放在Ram里面
}divoom_inflow_text_frame_info, *Pdivoom_inflow_text_frame_info;

typedef struct _divoom_inflow_pic_info
{
	T_U16 pic_step_index; //记录步骤
	T_U16 pic_disp_index; //显示到哪一步
	T_U8 pic_disp_over; //标记图片显示完毕 0 表示图片显示完毕，可显示下张图片 1表示当前有图片正在显示
}divoom_inflow_pic_info, *Pdivoom_inflow_pic_info;

typedef struct _divoom_inflow_display_info
{
	divoom_inflow_text_frame_info text_array_info[INFLOW_TEXT_FRAME_NUM]; //文本框控制
	divoom_inflow_gif_info gif_info[INFLOW_SMALL_IMAGE_NUM];//普通动画显示控制
	divoom_inflow_pic_info pic_info;//单张图片显示控制
	T_VOID *all_small_image_queue; //所有的局部图数据临时队列
	T_VOID *small_image_queue[INFLOW_SMALL_IMAGE_QUEUE_NUM]; //局部图队列
	T_VOID *text_info_queue; //文本框显示
	T_U8 disp_buf[SCREEN_BUF_SIZE_64];//显示图片,这个缓冲区利用公共缓冲区
	T_U8 disp_pic_buf[SCREEN_BUF_SIZE_64]; //保存图片
	T_U8 gif_queue_dis_index; //记录当前gif显示到哪个位置
	T_U8 text_queue_dis_index;//记录当前text显示到哪个位置
	T_U8 disp_pic_buf_flag;
	PDIVOOM_IMAGE_FREE_CALLBACK pfree_callback;
	T_U32 task_id;
}divoom_inflow_display_info, *Pdivoom_inflow_display_info;

static Pdivoom_inflow_display_info pdivoom_inflow_display_handle = NULL; 

/*
  Function:       divoom_inflow_display_del_small_image_queue
  Description:	  清空局部图队列
  Input:          data  NULL表示清空所有队列否则清空某一个队列
  Return:         NONE
  Author:         wangft
  Data:           2020-12-21
*/
T_VOID divoom_inflow_display_del_small_image_queue(T_VOID *data)
{
	T_U8 i = 0;
	Pdivoom_inflow_image_info image_data = NULL, image_info = (Pdivoom_inflow_image_info)data;

	divoom_com_pause_task(pdivoom_inflow_display_handle->task_id);
	if (data == NULL)
	{
		for (i = 0; i < INFLOW_SMALL_IMAGE_QUEUE_NUM; i++)
		{
			divoom_queue_pop_all(pdivoom_inflow_display_handle->small_image_queue[i], NULL);
		}
	}
	else
	{
		for (i = 0; i < INFLOW_SMALL_IMAGE_QUEUE_NUM; i++)
		{
			if (divoom_queue_not_empty(pdivoom_inflow_display_handle->small_image_queue[i])) //先找不是空队列，再取里面的数据进行比较是否一样,一样则清空该队列
			{
				image_data = (Pdivoom_inflow_image_info)divoom_queue_get_index_item(pdivoom_inflow_display_handle->small_image_queue[i], 0);
				if (image_data->index == image_info->index)
				{
					//存在则清空该队列
					divoom_queue_pop_all(pdivoom_inflow_display_handle->small_image_queue[i], NULL);
					break;
				}
			}
		}
	}
	divoom_queue_pop_all(pdivoom_inflow_display_handle->all_small_image_queue, NULL); //把暂存队列也清空
	divoom_com_resume_task(pdivoom_inflow_display_handle->task_id);
}

/*
  Function:       divoom_inflow_display_write_small_image_queue
  Description:	  往局部图队列里面写数据
  Input:          要写的数据
  Return:         1 写成功 0 写失败
  Author:         wangft
  Data:           2020-12-21
*/
T_U8 divoom_inflow_display_write_small_image_queue(T_VOID *data)
{
	Pdivoom_inflow_image_info image_info = (Pdivoom_inflow_image_info)data;
	T_U8 ret = 0;

	if (pdivoom_inflow_display_handle == NULL)
		return 0;

	divoom_com_pause_task( pdivoom_inflow_display_handle->task_id);
	ret = divoom_queue_push(pdivoom_inflow_display_handle->all_small_image_queue, image_info);
	divoom_com_resume_task( pdivoom_inflow_display_handle->task_id);
	return ret;
}

/*
  Function:       divoom_inflow_display_count_small_image_queue
  Description:	  统计当前有多少个队列
  Input:          NONE
  Return:         队列数
  Author:         wangft
  Data:           2020-12-18
*/
T_U8 divoom_inflow_display_count_small_image_queue(T_VOID)
{
	T_U8 i = 0, num = 0;

	for (i = 0; i < INFLOW_SMALL_IMAGE_QUEUE_NUM; i++)
	{
		if (divoom_queue_not_empty(pdivoom_inflow_display_handle->small_image_queue[i]))
			num++;
		else
			break;
	}
	return num;
}

/*
  Function:       divoom_inflow_add_small_image
  Description:	  把small_image添加到队列组
  Input:          image_info 图片信息
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
T_VOID divoom_inflow_display_add_small_image(Pdivoom_inflow_image_info image_info)
{
	T_U8 i = 0, index = 0;

	if (image_info == NULL)
		return;
	index = image_info->index; //用index对动画列表进行编号
	for (i = 0; i < INFLOW_SMALL_IMAGE_QUEUE_NUM; i++)
	{
		if (i == index)
		{
			divoom_queue_push(pdivoom_inflow_display_handle->small_image_queue[i], image_info);
			break;
		}
	}
}

/*
  Function:       divoom_inflow_allot_small_image_queue
  Description:	  把all_small_image_queue里面的数据全部读取出来，再把同一类的数据分到同一个队列
  				  只要all_small_image_queue里面有数据就全部读出来
  Input:          NONE
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
T_VOID divoom_inflow_display_allot_small_image_queue(T_VOID)
{
	Pdivoom_inflow_image_info image_info = NULL;
	if (pdivoom_inflow_display_handle == NULL)
		return ;

	while (divoom_queue_not_empty(pdivoom_inflow_display_handle->all_small_image_queue))
	{
		image_info = (Pdivoom_inflow_image_info)divoom_queue_pop(pdivoom_inflow_display_handle->all_small_image_queue);
		if (image_info != NULL)
		{
			divoom_inflow_display_add_small_image(image_info); //需要排序
		}
	}
}

/*
  Function:       divoom_inflow_display_count_text_info_queue
  Description:	  统计文本宽队列当前有多少个文本框
  Input:          NONE
  Return:         数量
  Author:         wangft
  Data:           2020-12-18
*/
T_U8 divoom_inflow_display_count_text_info_queue(T_VOID)
{
	return divoom_queue_items(pdivoom_inflow_display_handle->text_info_queue);
}

/*
  Function:       divoom_inflow_display_compare_func
  Description:	  文本框比较函数
  Input:          item1、 item2要比较的数据
  Return:         1 相等 0 不相等
  Author:         wangft
  Data:           2020-12-21
*/
T_S32 divoom_inflow_display_compare_func(T_VOID *item1, T_VOID *item2)
{
	Pdivoom_inflow_text_info text_data1 = (Pdivoom_inflow_text_info)item1;
	Pdivoom_inflow_text_info text_data2 = (Pdivoom_inflow_text_info)item2;

	if (text_data1->index == text_data2->index)
		return 1;
	else 
		return 0;
}

/*
  Function:       divoom_inflow_display_update_text_info_queue
  Description:	  往文本队列里面写文本数据
  Input:          要写的数据
  Return:         1 写成功 0 写失败
  Author:         wangft
  Data:           2020-12-21
*/
T_U8 divoom_inflow_display_update_text_info_queue(T_VOID *data)
{
	//先找到该文本框，再更改里面的内容
	Pdivoom_inflow_text_info text_data = NULL, text_info = (Pdivoom_inflow_text_info)data;

	if (text_info == NULL)
		return 0;

	text_data = divoom_queue_pop_cmp(pdivoom_inflow_display_handle->text_info_queue, text_info, divoom_inflow_display_compare_func); //先删除，再写入

	if (text_data != NULL)
		divoom_free(text_data);

	divoom_queue_push(pdivoom_inflow_display_handle->text_info_queue, text_info);
	return 1;
}

/*
  Function:       divoom_inflow_display_write_text_info_queue
  Description:	  往文本队列里面写文本数据
  Input:          要写的数据
  Return:         1 写成功 0 写失败
  Author:         wangft
  Data:           2020-12-21
*/
T_U8 divoom_inflow_display_write_text_info_queue(T_VOID *data)
{
	Pdivoom_inflow_text_info text_info = (Pdivoom_inflow_text_info)data;
	T_U8 ret = 0;

	if (pdivoom_inflow_display_handle == NULL)
		return 0;

	//文本框队列，不能边用边释放
	divoom_com_pause_task( pdivoom_inflow_display_handle->task_id);
	ret = divoom_queue_push(pdivoom_inflow_display_handle->text_info_queue, text_info);
	divoom_com_resume_task( pdivoom_inflow_display_handle->task_id);
	return ret;
}

/*
  Function:       divoom_inflow_display_read_text_info_queue
  Description:	  往文本队列里面读文本数据
  Input:          index 指定读哪个文本
  Return:         读出来的数据
  Author:         wangft
  Data:           2020-12-21
*/
T_VOID *divoom_inflow_display_read_text_info_queue(T_U8 index)
{
	T_U8 item_index = 0;
	Pdivoom_inflow_text_info text_info = NULL;

	while ((text_info = divoom_queue_get_index_item(pdivoom_inflow_display_handle->text_info_queue, item_index)))
	{
		if (text_info->index == index)
			return text_info;
		item_index++;
	}
	return NULL;
}

/*
  Function:       divoom_inflow_display_move_gif_frame
  Description:	   移动动画框
  Input:          NONE
  Return:         NONE
  Author:         wangft
  Data:           2020-12-21
*/
T_VOID divoom_inflow_display_move_gif_frame(T_VOID)
{
	//调整动画框
	//把下面的框往上移
	//从动画队列组里面取队列填补空动画框
	T_U8 i = 0, gif_queue_num = 0;

	for (i = 0; i < INFLOW_SMALL_IMAGE_NUM -1; i++)
	{
		//对动画框的参数进行初始化
		pdivoom_inflow_display_handle->gif_info[i].gif_first_tick_time = pdivoom_inflow_display_handle->gif_info[i + 1].gif_first_tick_time;
		pdivoom_inflow_display_handle->gif_info[i].gif_queue_index = pdivoom_inflow_display_handle->gif_info[i + 1].gif_queue_index;
		pdivoom_inflow_display_handle->gif_info[i].pic_index = pdivoom_inflow_display_handle->gif_info[i + 1].pic_index;
		if (i == 0)
			pdivoom_inflow_display_handle->gif_info[i].gif_total_time = divoom_system_get_tickcount() + divoom_system_set_interva_ms(INFLOW_GIF_DIS_TOTAL_TIME);
	}
	gif_queue_num = divoom_inflow_display_count_small_image_queue();
	
	//对动画框的参数进行初始化,填补最后一个动画框
	pdivoom_inflow_display_handle->gif_info[i].gif_first_tick_time = divoom_system_get_tickcount();
	pdivoom_inflow_display_handle->gif_info[i].gif_queue_index = pdivoom_inflow_display_handle->gif_queue_dis_index; //确定当前动画框指向的队列
	pdivoom_inflow_display_handle->gif_queue_dis_index++; //当前的被取了，则要指向下一个动画
	if (pdivoom_inflow_display_handle->gif_queue_dis_index > gif_queue_num - 1) //回头
		pdivoom_inflow_display_handle->gif_queue_dis_index = 0;
	pdivoom_inflow_display_handle->gif_info[i].pic_index = 0; //从动画的第一张开始取
	pdivoom_inflow_display_handle->gif_info[i].gif_total_time = divoom_system_get_tickcount() + divoom_system_set_interva_ms(INFLOW_GIF_DIS_TOTAL_TIME);
}

/*
  Function:       divoom_inflow_display_init_gif_frame
  Description:	  初始化动画框
  Input:          frame_num 需要初始化的动画框数量
  Return:         NONE
  Author:         wangft
  Data:           2020-12-21
*/
T_VOID divoom_inflow_display_init_gif_frame(T_U8 frame_num)
{
	T_U8 i = 0, gif_queue_num = 0;

	gif_queue_num = divoom_inflow_display_count_small_image_queue();
	for (i = 0; i < frame_num; i++)
	{
		//对动画框的参数进行初始化
		pdivoom_inflow_display_handle->gif_info[i].gif_first_tick_time = divoom_system_get_tickcount();
		pdivoom_inflow_display_handle->gif_info[i].gif_queue_index = pdivoom_inflow_display_handle->gif_queue_dis_index; //确定当前动画框指向的队列
		pdivoom_inflow_display_handle->gif_queue_dis_index++; //当前的被取了，则要指向下一个动画
		if (pdivoom_inflow_display_handle->gif_queue_dis_index > gif_queue_num - 1) //回头
			pdivoom_inflow_display_handle->gif_queue_dis_index = 0;
		pdivoom_inflow_display_handle->gif_info[i].pic_index = 0; //从动画的第一张开始取
		if (i == 0)
		{
			pdivoom_inflow_display_handle->gif_info[i].gif_total_time = divoom_system_get_tickcount() + divoom_system_set_interva_ms(INFLOW_GIF_DIS_TOTAL_TIME); //设置动画显示的总时长
		}
	}
}

/*
  Function:       divoom_inflow_display_handle_gif_frame
  Description:	   处理动画框
  Input:          NONE
  Return:         NONE
  Author:         wangft
  Data:           2020-12-21
*/
T_VOID divoom_inflow_display_handle_gif_frame(T_VOID)
{
	T_U8 j = 0, queue_index = 0;
	T_U32 time = 0;
	T_U8 queue_num = 0, frame_num = 0;

	time = divoom_system_get_tickcount();
	queue_index = pdivoom_inflow_display_handle->gif_info[0].gif_queue_index; //以第一个动画框为基准点
	if (queue_index != 0xff) //有数据在显示
	{
		if (pdivoom_inflow_display_handle->gif_info[0].gif_total_time > 0 && time > pdivoom_inflow_display_handle->gif_info[0].gif_total_time) //该动画显示时间到了
		{
			for (j = 1; j < INFLOW_SMALL_IMAGE_NUM; j++) //判断后面的框是否都被填满
			{
				queue_index = pdivoom_inflow_display_handle->gif_info[j].gif_queue_index;
				if (queue_index == 0xff)
					break;
			}
			if (j == INFLOW_SMALL_IMAGE_NUM && (divoom_inflow_display_count_small_image_queue() > INFLOW_SMALL_IMAGE_NUM)) //4个动画框都有动画并且动画队列组大于4个
			{
				//把下面的框移到上一个框
				divoom_inflow_display_move_gif_frame();
				//当前动画需要往上移动
				memset(&pdivoom_inflow_display_handle->pic_info, 0, sizeof(divoom_inflow_pic_info));
				pdivoom_inflow_display_handle->pic_info.pic_disp_over = 1; //标记这张图正在显示
				memcpy(pdivoom_inflow_display_handle->disp_pic_buf, pdivoom_inflow_display_handle->disp_buf, SCREEN_BUF_SIZE_64);//先把图拷贝出来
			}
			else //当前的动画框足够显示，不要滚动显示
			{
				//把第一个动画框的显示时间重新刷新
				pdivoom_inflow_display_handle->gif_info[0].gif_total_time = divoom_system_get_tickcount() + divoom_system_set_interva_ms(INFLOW_GIF_DIS_TOTAL_TIME); //设置动画显示的总时长
			}
		}
	}
	else //当前第一个动画框还没数据在显示
	{
		queue_num = divoom_inflow_display_count_small_image_queue();
		frame_num = (queue_num < INFLOW_SMALL_IMAGE_NUM) ? queue_num : INFLOW_SMALL_IMAGE_NUM;
		if (frame_num > 0) //动画队列组有动画
		{
			//初始化各个动画框的显示参数
			divoom_inflow_display_init_gif_frame(frame_num);
		}
		else //动画队列组还没有动画
		{
			return;
		}
	}
}

/*
  Function:       divoom_inflow_display_set_small_pic
  Description:	   设置小图,目前只针对16*16的图
  Input:          index 哪个图框  image_info 图片数据
  Return:         NONE
  Author:         wangft
  Data:           2020-12-21
*/
T_VOID divoom_inflow_display_set_small_pic(T_U8 index, Pdivoom_inflow_image_info image_info)
{
	//根据index 来确定图的坐标位置
	T_U8 i = 0, j = 0, x = 0, y = 0;
	T_U16 off1 = 0, off2 = 0;
	divoom_screen_color_info color_info = {0};

	if (image_info == NULL)
		return;

	x = 0;
	y = 16*index;
	for (i = x; i < x + 16; i++)
	{
		for (j = y; j < y + 16; j++)
		{
			off1 = (j - y) * 16 + (i - x);
			divoom_disp_ctrl_get_data(off1, &color_info, image_info->rgb_data);
			off2 = j * INFLOW_IMAGE_WIDTH_64 + i;
			divoom_disp_ctrl_set_data(off2, &color_info, pdivoom_inflow_display_handle->disp_buf);
		}
	}
}

/*
  Function:       divoom_inflow_display_set_back_ground_pic
  Description:	   设置背景图
  Input:          NONEL
  Return:         NONE
  Author:         wangft
  Data:           2020-12-21
*/
T_VOID divoom_inflow_display_set_back_ground_pic(T_VOID)
{
	//1、获取背景图
	//2、把背景图拷贝到disp_buf
}

/*
  Function:       divoom_inflow_display_dis_gif_frame
  Description:	   显示各个动画框
  Input:          NONEL
  Return:         NONE
  Author:         wangft
  Data:           2020-12-21
*/
T_VOID divoom_inflow_display_dis_gif_frame(T_VOID)
{
	T_U8 i = 0, j = 0, queue_index = 0, skip_step = 0, pic_index = 0;
	T_U32 time_now = 0, time = divoom_system_get_tickcount();
	Pdivoom_inflow_image_info image_info = NULL;

	memset(pdivoom_inflow_display_handle->disp_buf, 0, SCREEN_BUF_SIZE_64); //先清空一把
	divoom_inflow_display_set_back_ground_pic();
	//1、从队列中取图
	for (i = 0; i < INFLOW_SMALL_IMAGE_NUM; i++)
	{
		queue_index = pdivoom_inflow_display_handle->gif_info[i].gif_queue_index;
		if (queue_index != 0xff)
		{
			time_now = time - pdivoom_inflow_display_handle->gif_info[i].gif_first_tick_time; //间隔时间
			if (time_now >= pdivoom_inflow_display_handle->gif_info[i].gif_speed_time)
			{
				time_now -= pdivoom_inflow_display_handle->gif_info[i].gif_speed_time;
				if (pdivoom_inflow_display_handle->gif_info[i].gif_speed > 0)
					skip_step = time_now/pdivoom_inflow_display_handle->gif_info[i].gif_speed;
				else
					skip_step = 0;
				
				for (j = 0; j <= skip_step; j++)
				{
					pdivoom_inflow_display_handle->gif_info[i].gif_speed_time += pdivoom_inflow_display_handle->gif_info[i].gif_speed;
					pic_index = pdivoom_inflow_display_handle->gif_info[i].pic_index; //读取下一张图片
					image_info = (Pdivoom_inflow_image_info)divoom_queue_get_index_item(pdivoom_inflow_display_handle->small_image_queue[i], pic_index);
					if (image_info != NULL)
					{
						pdivoom_inflow_display_handle->gif_info[i].pic_index++;
					}
					else //如果读不到数据
					{
						pdivoom_inflow_display_handle->gif_info[i].pic_index = 0;
						image_info = (Pdivoom_inflow_image_info)divoom_queue_get_index_item(pdivoom_inflow_display_handle->small_image_queue[i], 0); //回头
						pdivoom_inflow_display_handle->gif_info[i].pic_index++;
					}
				}
				pdivoom_inflow_display_handle->gif_info[i].pic_index -= 1; //修正一下下标
			}
			pic_index = pdivoom_inflow_display_handle->gif_info[i].pic_index; //读取下一张图片
			image_info = (Pdivoom_inflow_image_info)divoom_queue_get_index_item(pdivoom_inflow_display_handle->small_image_queue[i], pic_index);

			//2、把图打进去显示
			divoom_inflow_display_set_small_pic(i, image_info);
			pdivoom_inflow_display_handle->disp_pic_buf_flag = 1;
		}
	}
}

/*
  Function:       divoom_inflow_display_image
  Description:	   逻辑处理
  Input:          NONE
  Return:         NONE
  Author:         wangft
  Data:           2020-12-21
*/
T_VOID divoom_inflow_display_image(T_VOID)
{
	//图片显示的逻辑流程：
	/*
		列表管理和显示管理两块：

		列表管理：
		1、提供一个写接口，用一个临时队列保存上层写下来的所有图片数据；
		2、提供一个分配接口，内部使用，对临时队列进行读取分配，分配到各个指定的队列，方便读取显示；
		3、提供一个删除接口，可以指定是删除所有队列，还是删除某一个队列；因为用户有可能完全替换，也有可能替换某一个频道；
		#4、提供一个更新接口，用户可以更新某个队列，但是需要先删除指定的队列，再不断的写数据，即不断的调用该接口，但是删除只能调用一次，所以提供一个更新接口，不太好判断这个删除，即这个接口提供起来是个麻烦，不如不要；
		5、更新操作，同4，先删除某个队列，再不断的写数据；
		
		显示管理：
		1、		
	*/
	//1、先分配图像数据到各个队列
	divoom_inflow_display_allot_small_image_queue();
	//2、处理动画框
	divoom_inflow_display_handle_gif_frame();
	//3、对各个动画框取图进行显示
	if (pdivoom_inflow_display_handle->pic_info.pic_disp_over == 0) //当这个参数为1表示正在切换动画框
		divoom_inflow_display_dis_gif_frame();
}

/*
  Function:       divoom_inflow_display_set_font_speed
  Description:	  转换文字显示速度
  Input:          speed 要设置的速度大小
  				  id 指定文本框
  Return:         NONE
  Author:         wangft
  Data:           2019-11-21
*/
T_VOID divoom_inflow_display_set_font_speed(T_U16 speed, T_U8 id)
{
	T_U8 speed_int = 0, speed_mod = 0;

	speed_int = speed/(10*INFLOW_TASK_TIME); //整数部分
	speed_mod = speed%(10*INFLOW_TASK_TIME); //余数部分
	if (speed_mod < 5*INFLOW_TASK_TIME)
		speed_mod = 0;
	else
		speed_mod = 1;

	if (speed < 35 && speed >= 20)
		pdivoom_inflow_display_handle->text_array_info[id].font_info.font_move_step = 2;
	else if (speed < 20 && speed > 10 )
		pdivoom_inflow_display_handle->text_array_info[id].font_info.font_move_step = 3;
	else if (speed <= 10)
		pdivoom_inflow_display_handle->text_array_info[id].font_info.font_move_step = 4;
	else
		pdivoom_inflow_display_handle->text_array_info[id].font_info.font_move_step = 1;
	pdivoom_inflow_display_handle->text_array_info[id].font_info.font_speed = speed_int + speed_mod;
}

/*
  Function:       divoom_inflow_display_set_str_screen
  Description:	  设置文字显示情况
  Input:          pword_info 文本参数信息
  				  index 下标
  Return:         NONE
  Author:         wangft
  Data:           2019-11-21
*/
T_VOID divoom_inflow_display_set_str_screen(Pdivoom_inflow_text_info pword_info, T_U8 index)
{
	T_U8 i = 0, word_reset = 0, frame_width = 0, frame_high = 0, id = index;

	frame_width = (pword_info->width <= SCREEN_WIDTH_64)?pword_info->width:SCREEN_WIDTH_64;
	frame_high = (pword_info->high <= SCREEN_HEIGHT_64)?pword_info->high:SCREEN_HEIGHT_64;
	
	if (pdivoom_inflow_display_handle->text_array_info[id].font_info.font_effect != pword_info->font_effect || pdivoom_inflow_display_handle->text_array_info[id].frame_info.frame_width != frame_width
		|| pdivoom_inflow_display_handle->text_array_info[id].frame_info.frame_high != frame_high) //文字显示效果变了
	{
		word_reset = 1;
		pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.word_id = 0; //从第一个字开始显示
		pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.col_id = 0; //从该字符第一行开始显示
	}
	else
	{
		if (pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.word_num > 0 && pword_info->unicode_len > 0)
		{
			if (pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.word_num <= pword_info->unicode_len) //增加文字
			{
				//当前显示的是第几个字，比较显示前面几个字，如果都相同，则不动，如果有不同直接回到第一个字显示
				for (i = 0; i <= pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.word_id; i++)
				{
					//如果当前文字比较不同
					if (pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.unicode_str[i] != pword_info->unicode_str[i])
						break;					
				}
				if (i <= pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.word_id)//表示有不同，复位
				{
					word_reset = 1;
					pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.word_id = 0; //从第一个字开始显示
					pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.col_id = 0; //从该字符第一行开始显示
				}
			}
			else if (pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.word_num > pword_info->unicode_len) //当前显示的文字数比新显示的文字数多，表示有文字删除
			{
				if (pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.word_id >= pword_info->unicode_len)
				{
					word_reset = 1;
					pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.word_id = 0; //从第一个字开始显示
					pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.col_id = 0; //从该字符第一行开始显示
				}
				else 
				{
					for (i = 0; i <= pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.word_id; i++)
					{
						//如果当前文字比较不同
						if (pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.unicode_str[i] != pword_info->unicode_str[i])
							break;					
					}
					if (i <= pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.word_id)//表示有不同，复位
					{
						word_reset = 1;
						pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.word_id = 0; //从第一个字开始显示
						pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.col_id = 0; //从该字符第一行开始显示
					}
				}
			}
		}
		else
		{
			word_reset = 1;
			pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.word_id = 0; //从第一个字开始显示
			pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.col_id = 0; //从该字符第一行开始显示
		}
	}
	pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.word_num = pword_info->unicode_len;//要显示的文字数
	pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.font_num = pword_info->word_num; //有多少个点阵字

	if (pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.unicode_str != NULL)
		divoom_free(pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.unicode_str);
	pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.unicode_str = (T_U16 *)divoom_system_malloc(pword_info->unicode_len*sizeof(T_U16));
	memcpy(pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.unicode_str, pword_info->unicode_str, pword_info->unicode_len*sizeof(T_U16));
	if (pdivoom_inflow_display_handle->text_array_info[id].font_info.font_size != pword_info->size || word_reset)
		memset(pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.cur_screen_info, 0, sizeof(pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.cur_screen_info));
	pdivoom_inflow_display_handle->text_array_info[id].font_info.font_size = pword_info->size;
	pdivoom_inflow_display_handle->text_array_info[id].font_info.font_effect = pword_info->font_effect;

	pdivoom_inflow_display_handle->text_array_info[id].frame_info.frame_width = frame_width;
	pdivoom_inflow_display_handle->text_array_info[id].frame_info.frame_high = frame_high;
	//memcpy(pdivoom_inflow_display_handle->text_array_info[id].word_info, pword_info->word_info, pword_info->word_num*sizeof(DIVOOM_IMAGE_ENCODE_WORD_FONT));
	divoom_print("[%s:%d] %d %d %d %d %d %d %d %d %d %d\n", __func__, __LINE__, pword_info->size, pword_info->speed, pword_info->width, 
		pword_info->high, pword_info->unicode_len, pword_info->word_num, pword_info->color[0], pword_info->color[1], pword_info->color[2], pword_info->font_effect);
}

/*
  Function:       divoom_inflow_display_set_word_param
  Description:	  设置文本信息参数
  Input:          pword_info 文本参数信息
  				  index 设置哪个文本框
  Return:         NONE
  Author:         wangft
  Data:           2019-11-21
*/
T_VOID divoom_inflow_display_set_word_param(Pdivoom_inflow_text_info pword_info, T_U8 index)
{
	pdivoom_inflow_display_handle->text_array_info[index].frame_info.frame_x = INFLOW_TEXT_X_POS; //坐标根据index来设置
	pdivoom_inflow_display_handle->text_array_info[index].frame_info.frame_y = index * INFLOW_TEXT_FRAME_HEIGHT;
	pword_info->width = INFLOW_TEXT_FRAME_WIDTH;
	pword_info->high = INFLOW_TEXT_FRAME_HEIGHT;
	pdivoom_inflow_display_handle->text_array_info[index].dis_flag = 1; //标记该文字框有文字要显示
	memcpy(&pdivoom_inflow_display_handle->text_array_info[index].font_info.font_disp_color, pword_info->color, 3);
	divoom_inflow_display_set_font_speed(pword_info->speed, index);
	divoom_inflow_display_set_str_screen(pword_info, index);
	//divoom_font_cache_update(1, pword_info);//把字体信息保存到flash
}

/*
  Function:       divoom_inflow_display_set_text_frame
  Description:	  设置指定的文本框
  Input:          index 指定哪个文本框
  Return:         NONE
  Author:         wangft
  Data:           2020-12-21
*/
T_VOID divoom_inflow_display_set_text_frame(T_U8 index)
{
	Pdivoom_inflow_text_info ptext_info = NULL;
	T_U8 text_queue_num = 0;

	text_queue_num = divoom_inflow_display_count_text_info_queue();
	//对动画框的参数进行初始化
	pdivoom_inflow_display_handle->text_array_info[index].font_info.font_index = 0; //控制文字滚动速度
	pdivoom_inflow_display_handle->text_array_info[index].frame_info.frame_queue_index = pdivoom_inflow_display_handle->text_queue_dis_index; //确定当前文本框指向第几个文本
	pdivoom_inflow_display_handle->text_queue_dis_index++; //当前的被取了，则要指向下一个
	if (pdivoom_inflow_display_handle->text_queue_dis_index > text_queue_num - 1) //回头
		pdivoom_inflow_display_handle->text_queue_dis_index = 0;
	if (index == 0)
		pdivoom_inflow_display_handle->text_array_info[0].frame_info.frame_total_time = pdivoom_inflow_display_handle->gif_info[0].gif_total_time; //要保持与动画的时间同步
	//读取文本数据
	ptext_info = divoom_inflow_display_read_text_info_queue(pdivoom_inflow_display_handle->text_array_info[index].frame_info.frame_queue_index);
	//设置文本参数
	if (ptext_info != NULL)
		divoom_inflow_display_set_word_param(ptext_info, index);
}

/*
  Function:       divoom_inflow_display_move_text_frame
  Description:	  移动文本框
  Input:          NONE
  Return:         NONE
  Author:         wangft
  Data:           2020-12-21
*/
T_VOID divoom_inflow_display_move_text_frame(T_VOID)
{
	//调整动画框
	//把下面的框往上移
	//从动画队列组里面取队列填补空动画框
	T_U8 i = 0;

	for (i = 0; i < INFLOW_SMALL_IMAGE_NUM -1; i++)
	{
		//把下一个文本框的内容替换上来
		memcpy(&pdivoom_inflow_display_handle->text_array_info[i].frame_info, &pdivoom_inflow_display_handle->text_array_info[i + 1].frame_info, sizeof(divoom_inflow_frame_info));
		if (i == 0)
			pdivoom_inflow_display_handle->text_array_info[i].frame_info.frame_total_time = pdivoom_inflow_display_handle->gif_info[0].gif_total_time;//要保持与动画的时间同步
		memcpy(&pdivoom_inflow_display_handle->text_array_info[i].font_info, &pdivoom_inflow_display_handle->text_array_info[i + 1].font_info, sizeof(divoom_inflow_font_info));
		memcpy(&pdivoom_inflow_display_handle->text_array_info[i].disp_word_info, &pdivoom_inflow_display_handle->text_array_info[i + 1].disp_word_info, sizeof(divoom_inflow_disp_word_info));
	}
	divoom_inflow_display_set_text_frame(i);
}

/*
  Function:       divoom_inflow_display_init_text_frame
  Description:	  初始化文本框
  Input:          frame_num 需要初始化的文本框数量
  Return:         NONE
  Author:         wangft
  Data:           2020-12-21
*/
T_VOID divoom_inflow_display_init_text_frame(T_U8 frame_num)
{
	T_U8 i = 0;

	for (i = 0; i < frame_num; i++)
	{
		//对动画框的参数进行初始化
		divoom_inflow_display_set_text_frame(i);
	}
}

/*
  Function:       divoom_inflow_display_handle_text_frame
  Description:	   处理文本框
  Input:          NONE
  Return:         NONE
  Author:         wangft
  Data:           2020-12-21
*/
T_VOID divoom_inflow_display_handle_text_frame(T_VOID)
{
	T_U8 j = 0, queue_index = 0;
	T_U32 time = 0;
	T_U8 queue_num = 0, frame_num = 0;

	time = divoom_system_get_tickcount();
	queue_index = pdivoom_inflow_display_handle->text_array_info[0].frame_info.frame_queue_index; //先判断第一个文本框
	if (queue_index != 0xff) //有数据在显示
	{
		if (pdivoom_inflow_display_handle->text_array_info[0].frame_info.frame_total_time > 0 && time > pdivoom_inflow_display_handle->text_array_info[0].frame_info.frame_total_time) //该动画显示时间到了
		{
			for (j = 1; j < INFLOW_SMALL_IMAGE_NUM; j++) //判断后面的框是否都被填满
			{
				queue_index = pdivoom_inflow_display_handle->text_array_info[j].frame_info.frame_queue_index;
				if (queue_index == 0xff)
					break;
			}
			if (j == INFLOW_SMALL_IMAGE_NUM && (divoom_inflow_display_count_text_info_queue() > INFLOW_SMALL_IMAGE_NUM)) //4个动画框都有动画并且动画队列组大于4个
			{
				//把下面的框移到上一个框
				divoom_inflow_display_move_text_frame();
			}
			else //当前的动画框足够显示，不要滚动显示
			{
				//把第一个动画框的显示时间重新刷新
				pdivoom_inflow_display_handle->text_array_info[0].frame_info.frame_total_time = pdivoom_inflow_display_handle->gif_info[0].gif_total_time;//要保持与动画的时间同步
			}
		}
	}
	else //当前第一个动画框还没数据在显示
	{
		queue_num = divoom_inflow_display_count_text_info_queue();
		frame_num = (queue_num < INFLOW_SMALL_IMAGE_NUM) ? queue_num : INFLOW_SMALL_IMAGE_NUM;
		if (frame_num > 0) //动画队列组有动画
		{
			//初始化各个动画框的显示参数
			divoom_inflow_display_init_text_frame(frame_num);
		}
		else //动画队列组还没有动画
		{
			return;
		}
	}
}

/*
  Function:       divoom_inflow_display_get_new_line_info
  Description:	  把16*16的字符转换一下，再获取相应的行和列
  Input:          word_index 字符下标
  				  line_index 取第几行或列
  				  frame_id 属于哪个框
  				  line_flag 行显或者列显
  Return:         返回相应的行或者列	
  Author:         wangft
  Data:           2019-11-21
*/
T_U32 divoom_inflow_display_get_new_line_info(T_U8 word_index, T_U8 line_index, T_U8 frame_id, T_U8 line_flag)
{
	return 0;
}

/*
  Function:       divoom_inflow_display_fill_font
  Description:	  填充字符显示区
  Input:          id 哪个文本框
  Return:         none
  Author:         wangft
  Data:           2019-11-21
*/
T_VOID divoom_inflow_display_fill_font(T_U8 id)
{
    T_U8 i, frame_length = 0, index = 0;
    T_U32 new_col = 0;

	if (pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.word_num == 0)
		return ;

	if (pdivoom_inflow_display_handle->text_array_info[id].font_info.font_effect == DIVOOM_TEXT_EFFECT_UP || pdivoom_inflow_display_handle->text_array_info[id].font_info.font_effect == DIVOOM_TEXT_EFFECT_DOWN) //行显
	{
		frame_length = pdivoom_inflow_display_handle->text_array_info[id].frame_info.frame_high;
	}
	else if (pdivoom_inflow_display_handle->text_array_info[id].font_info.font_effect == DIVOOM_TEXT_EFFECT_RIGHT || pdivoom_inflow_display_handle->text_array_info[id].font_info.font_effect == DIVOOM_TEXT_EFFECT_LEFT)
	{
		frame_length = pdivoom_inflow_display_handle->text_array_info[id].frame_info.frame_width;
	}

    if (pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.word_id >= pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.word_num) //显示完最后一个字需要显示6列空行，显示完空行之后显示从第一个字开始显示
    {
		if (pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.cur_disp_word_length <= frame_length)
		{
			if (pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.frist_word_col_pos == 0)//判断当前pos是否与x轴重合
			{
				pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.roll_flag = 1; //字固定不动
			}
			else
			{
				pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.roll_flag = 0; //继续滚动
				new_col = 0;
			}
		}
		else //当前文字框宽度无法显示全部文字
		{
			pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.roll_flag = 0;
	        pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.end_tick++;
	        if (pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.end_tick >= INFLOW_TEXT_SLIP_DOT)
	        {
	            pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.word_id = 0;
	            pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.col_id = 0;
	        }
	        new_col = 0;
		}
    }
    else
    {
		pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.roll_flag = 0;
        if (pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.space_step)
        {
            new_col = 0;
            pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.space_step--;
        }
        else
        {
            for (i = pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.col_id; i < pdivoom_inflow_display_handle->text_array_info[id].font_info.font_size; i++) //取每个字相应的列，行，这里的字体大小分16x 24x 32x ,当字是左右滚动是选列，当字是上下滚动是选行
            {
				if (pdivoom_inflow_display_handle->text_array_info[id].font_info.font_effect == DIVOOM_TEXT_EFFECT_UP) //行显
				{
					new_col = divoom_inflow_display_get_new_line_info(pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.word_id, i, id, DIVOOM_TEXT_DISP_ROW); //获取某一行, 行显
				}
				else if (pdivoom_inflow_display_handle->text_array_info[id].font_info.font_effect == DIVOOM_TEXT_EFFECT_DOWN)
				{
					index = pdivoom_inflow_display_handle->text_array_info[id].font_info.font_size - 1 - i;
					new_col = divoom_inflow_display_get_new_line_info(pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.word_id, index, id, DIVOOM_TEXT_DISP_ROW); //获取某一行, 行显
				}
				else if (pdivoom_inflow_display_handle->text_array_info[id].font_info.font_effect == DIVOOM_TEXT_EFFECT_LEFT)
				{
	                new_col = divoom_inflow_display_get_new_line_info(pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.word_id, i, id, DIVOOM_TEXT_DISP_COL); //获取某一列, 列显
					
				}
				else if (pdivoom_inflow_display_handle->text_array_info[id].font_info.font_effect == DIVOOM_TEXT_EFFECT_RIGHT )
				{
					index = pdivoom_inflow_display_handle->text_array_info[id].font_info.font_size - 1 - i;
					new_col = divoom_inflow_display_get_new_line_info(pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.word_id, index, id, DIVOOM_TEXT_DISP_COL); //获取某一列, 列显
				}
                if (new_col != 0)
                {
                	if (pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.word_id == 0 && 
						pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.col_id == 0)
                	{
                		pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.cur_disp_word_length = 0;
						pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.frist_word_col_pos = frame_length;
					}
                    break;
                }
            }
            if (pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.col_id == 0 && i == pdivoom_inflow_display_handle->text_array_info[id].font_info.font_size)
            {
                pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.space_step = INFLOW_TEXT_SPACE_DOT;
            }
            pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.col_id = i + 1;
            if (i == pdivoom_inflow_display_handle->text_array_info[id].font_info.font_size) //显示完一个，接着显示空列，接着显示下一个字
            {
                pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.word_id++;
                pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.col_id = 0;
				pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.space_step = 0;
            }
        }
    }
	if (pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.roll_flag == 0)
	{
		memmove(&pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.cur_screen_info[0], &pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.cur_screen_info[1], (frame_length - 1) * sizeof(T_U32));
		pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.cur_screen_info[frame_length - 1] = new_col; //在这里标注一下颜色，每一行的颜色
		if (pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.frist_word_col_pos > 0)
			pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.frist_word_col_pos--;
		pdivoom_inflow_display_handle->text_array_info[id].disp_word_info.cur_disp_word_length++;
	}
}

/*
  Function:       divoom_inflow_display_set_word_buf
  Description:	   设置文本显示区
  Input:          NONE
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
T_VOID divoom_inflow_display_set_word_buf(T_VOID)
{
	T_U8 k = 0, i = 0;

	for (k = 0; k < INFLOW_TEXT_FRAME_NUM; k++)
	{
		if (pdivoom_inflow_display_handle->text_array_info[k].dis_flag == 1) //该文本框有数据要显示
		{
			pdivoom_inflow_display_handle->text_array_info[k].font_info.font_index++;
			if (pdivoom_inflow_display_handle->text_array_info[k].font_info.font_index >= pdivoom_inflow_display_handle->text_array_info[k].font_info.font_speed) //文字的显示时间到了，需要做滚动处理
			{
				pdivoom_inflow_display_handle->text_array_info[k].font_info.font_index = 0;
				for (i = 0; i < pdivoom_inflow_display_handle->text_array_info[k].font_info.font_move_step; i++)
				{
					divoom_inflow_display_fill_font(k);
				}
			}
		}
	}
}

/*
  Function:       divoom_inflow_display_disp_font
  Description:	  把字符送去显示
  Input:          NONE
  Return:         NONE
  Author:         wangft
  Data:           2019-11-21
*/
T_VOID divoom_inflow_display_disp_font(T_VOID)
{
    T_U8 i, j, k, x, y, frame_length = 0;
	divoom_screen_color_info disp_color;

	for (k = 0; k < INFLOW_TEXT_FRAME_NUM; k++)
	{
		if (pdivoom_inflow_display_handle->text_array_info[k].disp_word_info.word_num == 0)
			continue;

		pdivoom_inflow_display_handle->disp_pic_buf_flag = 1;
		x = pdivoom_inflow_display_handle->text_array_info[k].frame_info.frame_x;
		y = pdivoom_inflow_display_handle->text_array_info[k].frame_info.frame_y;
		
		if (pdivoom_inflow_display_handle->text_array_info[k].font_info.font_effect == DIVOOM_TEXT_EFFECT_UP || pdivoom_inflow_display_handle->text_array_info[k].font_info.font_effect == DIVOOM_TEXT_EFFECT_DOWN) //行显,上下滚动
		{
			frame_length = pdivoom_inflow_display_handle->text_array_info[k].frame_info.frame_high;

			for (i = 0; i < frame_length; i++) //这个框有大小问题
			{
				memcpy(&disp_color, &pdivoom_inflow_display_handle->text_array_info[k].font_info.font_disp_color, sizeof(divoom_screen_color_info));
				for (j = 0; j < pdivoom_inflow_display_handle->text_array_info[k].font_info.font_size; j++)
				{
					if (pdivoom_inflow_display_handle->text_array_info[k].disp_word_info.cur_screen_info[i] & (1 << j))
					{
						if (pdivoom_inflow_display_handle->text_array_info[k].font_info.font_effect == DIVOOM_TEXT_EFFECT_UP)
						{
							divoom_disp_ctrl_set_dot(j + x, i + y, &disp_color, pdivoom_inflow_display_handle->disp_buf);//Y轴不动，X轴动
						}
						else if (pdivoom_inflow_display_handle->text_array_info[k].font_info.font_effect == DIVOOM_TEXT_EFFECT_DOWN)
						{
							divoom_disp_ctrl_set_dot(j + x, frame_length -1 - i + y, &disp_color, pdivoom_inflow_display_handle->disp_buf);//Y轴不动，X轴动
						}
					}
				}
			}
		}
		else if (pdivoom_inflow_display_handle->text_array_info[k].font_info.font_effect == DIVOOM_TEXT_EFFECT_RIGHT || pdivoom_inflow_display_handle->text_array_info[k].font_info.font_effect == DIVOOM_TEXT_EFFECT_LEFT) //列显，左右滚动
		{
			frame_length = pdivoom_inflow_display_handle->text_array_info[k].frame_info.frame_width;

			//确定坐标，显示的位置
			for (i = 0; i < frame_length; i++) //这个框有大小问题
			{
				memcpy(&disp_color, &pdivoom_inflow_display_handle->text_array_info[k].font_info.font_disp_color, sizeof(divoom_screen_color_info));
				for (j = 0; j < pdivoom_inflow_display_handle->text_array_info[k].font_info.font_size; j++)
				{
					if (pdivoom_inflow_display_handle->text_array_info[k].disp_word_info.cur_screen_info[i] & ( 1 << j))
					{
						if (pdivoom_inflow_display_handle->text_array_info[k].font_info.font_effect == DIVOOM_TEXT_EFFECT_LEFT)
						{
							divoom_disp_ctrl_set_dot(i + x, j + y, &disp_color, pdivoom_inflow_display_handle->disp_buf);//X轴不动，Y轴动
						}
						else if (pdivoom_inflow_display_handle->text_array_info[k].font_info.font_effect == DIVOOM_TEXT_EFFECT_RIGHT)
						{
							divoom_disp_ctrl_set_dot(frame_length -1 -i + x, j + y, &disp_color, pdivoom_inflow_display_handle->disp_buf);//X轴不动，Y轴动
						}
					}
				}
			}
		}
	}
}

/*
  Function:       divoom_inflow_display_dis_text_frame
  Description:	   显示各个动画框
  Input:          NONEL
  Return:         NONE
  Author:         wangft
  Data:           2020-12-21
*/
T_VOID divoom_inflow_display_dis_text_frame(T_VOID)
{
	//显示文本
	divoom_inflow_display_set_word_buf();
	divoom_inflow_display_disp_font();
}

/*
  Function:       divoom_inflow_display_text
  Description:	   逻辑处理
  Input:          NONE
  Return:         NONE
  Author:         wangft
  Data:           2020-12-21
*/
T_VOID divoom_inflow_display_text(T_VOID)
{
	//文本显示的逻辑流程：
	/*
		列表管理和显示管理两块：

		列表管理：
		1、提供一个写接口，这里不能用一个临时的队列保存，这个队列不能边用边释放；
		###2、提供一个删除接口，可以指定是删除所有文本，还是删除某一个文本；假如用户想替换某个文本，需要先删除该文本，再进行写入；
		3、提供一个更新接口，用户可以更新某个文本的内容，这样就要先删除该文本，再进行写入，因为写文本，调用一次就够了，在这里是可以提供一个更新接口的；

		显示管理：
		1、
	*/
	divoom_inflow_display_handle_text_frame();
	if (pdivoom_inflow_display_handle->pic_info.pic_disp_over == 0)
		divoom_inflow_display_dis_text_frame();
}

/*
  Function:       divoom_inflow_display_disp_pic
  Description:	  显示图片
  Input:          NONE
  Return:         NONE
  Author:         wangft
  Data:           2019-11-21
*/
T_VOID divoom_inflow_display_disp_pic(T_VOID)
{
	T_U8 x = 0, y = 0;
	T_S8 i = 0;
	T_U16 disp_index = 0;
	divoom_screen_color_info color = {0};

	//图片的显示也是分效果的
	pdivoom_inflow_display_handle->pic_info.pic_step_index++;
	if (pdivoom_inflow_display_handle->pic_info.pic_step_index%1 == 0)
		pdivoom_inflow_display_handle->pic_info.pic_disp_index++;

	disp_index = pdivoom_inflow_display_handle->pic_info.pic_disp_index;
	if ((disp_index > 0) && disp_index <= SCREEN_HEIGHT_64/4) //把这张图移出去
	{
		memset(pdivoom_inflow_display_handle->disp_buf, 0, SCREEN_BUF_SIZE_64);
		for (i = disp_index; i < SCREEN_HEIGHT_64; i++) //取disp_index~SCREEN_HEIGHT_64 行的数据
		{
			//取disp_index~SCREEN_HEIGHT_64 行的数据到 disp_buf 的 0~（SCREEN_HEIGHT_64 - disp_index）行
			for (x = 0; x < SCREEN_WIDTH_64; x++)
			{
				y = i;
				divoom_disp_ctrl_get_dot(x, y, &color, pdivoom_inflow_display_handle->disp_pic_buf);
				y = i - disp_index;
				divoom_disp_ctrl_set_dot(x, y, &color, pdivoom_inflow_display_handle->disp_buf);
			}
		}
	}
	if (pdivoom_inflow_display_handle->pic_info.pic_disp_index == SCREEN_HEIGHT_64/4)
		pdivoom_inflow_display_handle->pic_info.pic_disp_over = 0; //本张图片显示完毕
	pdivoom_inflow_display_handle->disp_pic_buf_flag = 1;
}

/*
  Function:       divoom_inflow_display_pic_buf
  Description:	   做最终的显示
  Input:          NONEL
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
T_VOID divoom_inflow_display_pic_buf(T_VOID)
{
	if (pdivoom_inflow_display_handle->pic_info.pic_disp_over == 1)
		divoom_inflow_display_disp_pic();
	if (pdivoom_inflow_display_handle->disp_pic_buf_flag == 1)
	{
		//divoom_screen_disp_rgb_data(pdivoom_inflow_display_handle->disp_buf);
	}
}

/*
  Function:       divoom_inflow_display_main
  Description:	   逻辑处理
  Input:          arg 参数 一般为NULL
  Return:         NONE
  Author:         wangft
  Data:           2020-12-21
*/
T_VOID divoom_inflow_display_main(T_VOID *arg)
{
	divoom_inflow_display_image();
	divoom_inflow_display_text();
	divoom_inflow_display_pic_buf();
}

/*
  Function:       divoom_inflow_display_init
  Description:    模块初始化
  Input:          NONE
  Return:         NONE
  Author:         wangft
  Data:           2020-12-21
*/
T_VOID divoom_inflow_display_init(T_VOID)
{
	T_U8 i = 0;

	if (pdivoom_inflow_display_handle)
		return;

	pdivoom_inflow_display_handle = divoom_system_malloc(sizeof(divoom_inflow_display_info));
	if (pdivoom_inflow_display_handle == NULL)
		return;

	pdivoom_inflow_display_handle->text_info_queue = divoom_queue_init(INFLOW_SMALL_IMAGE_QUEUE_NUM, __func__, __LINE__); //保存信息的文本
	pdivoom_inflow_display_handle->all_small_image_queue = divoom_queue_init(180, __func__, __LINE__); //保存信息流的动画

	for (i = 0; i < INFLOW_SMALL_IMAGE_QUEUE_NUM; i++)
	{
		pdivoom_inflow_display_handle->small_image_queue[i] = divoom_queue_init(60, __func__, __LINE__); //对所有的动画做一个分配
	}

	for (i = 0; i < INFLOW_SMALL_IMAGE_NUM; i++)
	{
		pdivoom_inflow_display_handle->gif_info[i].gif_queue_index = 0xff;
		pdivoom_inflow_display_handle->text_array_info[i].frame_info.frame_queue_index = 0xff;
	}
	
	pdivoom_inflow_display_handle->task_id = divoom_com_create_task(divoom_inflow_display_main, INFLOW_TASK_TIME);
	divoom_com_resume_task(pdivoom_inflow_display_handle->task_id);
}

/*
  Function:       divoom_inflow_display_exit
  Description:    模块退出
  Input:          NONE
  Return:         NONE
  Author:         wangft
  Data:           2020-12-21
*/
T_VOID divoom_inflow_display_exit(T_VOID)
{
	T_U8 i = 0;

	if (pdivoom_inflow_display_handle)
	{
		divoom_queue_destroy(pdivoom_inflow_display_handle->text_info_queue, NULL);
		divoom_queue_destroy(pdivoom_inflow_display_handle->all_small_image_queue, NULL);
		for (i = 0; i < INFLOW_SMALL_IMAGE_QUEUE_NUM; i++)
		{
			divoom_queue_destroy(pdivoom_inflow_display_handle->small_image_queue[i], NULL);
		}
		divoom_free(pdivoom_inflow_display_handle);
	}
}

