#ifndef _divoom_light_start_flow_h_
#define _divoom_light_start_flow_h_

#define DOT_FLOW_NUM 12
#define START_FLOW_NUM 3
#define FLOW_COLOR_NUM 7
#define FLOW_NUMS 24
#define DIVOOM_LIGHT_START_FLOW_GRAD_GET_RAND(x)     divoom_com_get_random(x)

typedef struct _divoom_light_start_flow_info
{		
	uint32 time_limite;		//定时
	uint8 flag;
	uint8 move_flag;			//移动标志位，左右上下
	uint8 move_size;			//移动的单位距离
	uint8 light_size;
	uint8 figure_index;		//哪张图形
	uint8 color_index;		//颜色值	
	uint8 disp_buf[SCREEN_BUF_SIZE_32];
	uint16 pos_info[FLOW_NUMS];
}divoom_light_start_flow_info, *Pdivoom_light_start_flow_info;

typedef struct _divoom_light_start_flow
{
	divoom_light_start_flow_info flow_info[START_FLOW_NUM];
	uint8 disp_buf[SCREEN_BUF_SIZE_32];
}divoom_light_start_flow, *Pdivoom_light_start_flow;

typedef struct _divoom_light_start_flow_pos
{
	T_S8 y;
	T_S8 x;
}divoom_light_start_flow_pos, *Pdivoom_light_start_flow_pos;


/**
 * NAME               divoom_light_start_flow_init
 * *  @brief       	流星初始化
 * *  @param[in] 		void
 * *	 @author         wangft               
 * *  @return        0
 * */

uint32 divoom_light_start_flow_init(void);

/*
 * NAME               divoom_light_start_flow_move
 * *  @brief       	流星移动操作
 * *  @table_index 	图形
 * *  @move_flag 		移动方式
 * *  @size		 	移动的单位距离
 * *  @color_index 	颜色下标
 * *	  @start_flow_num  那个点
 * *	  @author         wangft               
 * *   @return        void
 * */

void divoom_light_start_flow_move(uint8 table_index, uint8 move_flag, uint8 size, uint8 color_index, uint8 start_flow_num, uint8 light_size);
void divoom_light_start_flow_set_light(void);
void divoom_light_start_flow_set_data(uint8 *disp_buf);
void divoom_light_start_flow_main(uint8 *disp_buf, uint8 music_power, uint32 task_id);
void divoom_light_start_flow_close(void);
#endif

