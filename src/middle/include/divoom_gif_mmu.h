#ifndef _DIVOOM_GIF_MMU_
#define _DIVOOM_GIF_MMU_

typedef struct _divoom_gif_mmu_handle
{
	void *gif_queue_first;
	void *gif_queue_second;
	int flag;					//1 正在使用队列1 2 正在使用队列2
	int recv_flag;		//1 表示当前有app发送数据过来 0 表示app没有发送数据
	int packge_id;		//当前包的id
	int packge_recv_state1[10];    //最多是10个包
	int packge_recv_state2[10];    //最多是10个包
}divoom_gif_mmu_handle, *Pdivoom_gif_mmu_handle;

typedef struct _divoom_gif_data
{
	uint8 *encode_data;
	int encode_len;
	uint16 time;
	int index;  		//第几张图片
}divoom_gif_data, *Pdivoom_gif_data;

/**
 * NAME              divoom_gif_mmu_set_flag
 * *  @brief       	对当前正在使用的队列进行交换
 * *  @param[in] 		
 * *	 @author         wangft               
 * *  @return       0 成功 -1 失败
 * */

void divoom_gif_mmu_set_flag(void);

/**
 * NAME              divoom_gif_mmu_destroy
 * *  @brief       	清空队列
 * *  @param[in] 		queue_handle 队列句柄
 * *	 @author         wangft               
 * *  @return       
 * */

void divoom_gif_mmu_destroy(void *queue_handle);

/**
 * NAME              divoom_master_slave_udp_manage_deal_rgb_data
 * *  @brief       	图片rgb数据处理
 * *  @param[in] 		
 * *	 @author         wangft               
 * *  @return       0 成功 -1 失败
 * */

Pdivoom_gif_data divoom_gif_mmu_get_pic(void *queue_handle, int queue_index);

/**
 * NAME              divoom_gif_mmu_get_items
 * *  @brief       	获取链表总条目数
 * *  @param[in] 		queue_handle 链表句柄
 * *	 @author         wangft               
 * *  @return       总数
 * */

uint16 divoom_gif_mmu_get_items(void *queue_handle);

/**
 * NAME              divoom_gif_mmu_sort
 * *  @brief       	由小到大排序
 * *  @param[in] 		
 * *	 @author         wangft               
 * *  @return       返回排完之后的队列
 * */

void *divoom_gif_mmu_sort(void);

/**
 * NAME              divoom_gif_mmu_add_pic
 * *  @brief       	增加图片到队列
 * *  @param[in] 		encode_buf 编码之后的图片 packet_len 编码之后图片长度 packet_index 第几张图片
 * *	 @author         wangft               
 * *  @return       0 成功 -1 失败
 * */

int divoom_gif_mmu_add_pic(uint8 *encode_buf, int packet_len, int packet_index, int time);

/**
 * NAME             divoom_gif_mmu_set_packge_recv_state
 * *  @brief       	查找判断当前数据包是否已经收到
 * *  @param[in] 		packet_index 第几个数据包
 * *	 @author         wangft               
 * *  @return       0 成功 -1 失败
 * */

int divoom_gif_mmu_set_packge_recv_state(int packge_id, int packet_index);

/**
 * NAME             divoom_gif_mmu_get_packge_recv_state
 * *  @brief       	查看当前数据包是否全部收到
 * *  @param[in] 		total_packge 总包数
 * *	 @author         wangft               
 * *  @return       0 成功 -1 失败
 * */

int divoom_gif_mmu_get_packge_recv_state(int total_packge);

void divoom_gif_mmu_init(void);
#endif
