#include "divoom.h"

#define DOT_NUM 5
#define DOT_NUM2 7
#define COLOR_NUM 7
#define START_NUM 5
#define TABLE_NUM 25
#define GRAPH_SIZE 3
#define GRAPH_BIG_NUM 12
#define DIVOOM_LIGHT_START_BLINK_GRAD_GET_RAND(x)     (rand()%(x))
#define DIVOOM_LIGHT_INDEX_END			7

enum{
	DIVOOM_LIGHT_START_INIT,
	DIVOOM_LIGHT_START_ADD_LIGHT,
	DIVOOM_LIGHT_START_REDUCE_LIGHT,
	DIVOOM_LIGHT_START_END,
};

typedef struct _divoom_light_start_info
{		
	T_U32 time_limite;
	T_U8 flag;	
	T_S32 light;
	T_U8 index;		
	T_U8 color_index;
}divoom_light_start_info, *Pdivoom_light_start_info;

typedef struct _divoom_light_start_blink
{
	divoom_light_start_info start_info[START_NUM];
	T_U8 disp_buf[SCREEN_BUF_SIZE];
	bool flag;//ak_true-->Ê¹ÓÃ½ö°×É«µÄ£¬ak_false-->Ê¹ÓÃ²ÊÉ«µÄ
}divoom_light_start_blink, *Pdivoom_light_start_blink;


static Pdivoom_light_start_blink pdivoom_light_start_blink_handle = AK_NULL; 

static const T_U16 data_table[DOT_NUM2*TABLE_NUM] = {	
	3*SCREEN_WIDTH+3,
	2*SCREEN_WIDTH+3, 3*SCREEN_WIDTH+4, 4*SCREEN_WIDTH+3, 3*SCREEN_WIDTH+2, 
	1*SCREEN_WIDTH+3, 2*SCREEN_WIDTH+4, 3*SCREEN_WIDTH+5, 4*SCREEN_WIDTH+4, 5*SCREEN_WIDTH+3, 4*SCREEN_WIDTH+2, 3*SCREEN_WIDTH+1, 2*SCREEN_WIDTH+2, 
	0*SCREEN_WIDTH+3, 1*SCREEN_WIDTH+4, 2*SCREEN_WIDTH+5, 3*SCREEN_WIDTH+6, 4*SCREEN_WIDTH+5, 5*SCREEN_WIDTH+4, 6*SCREEN_WIDTH+3, 5*SCREEN_WIDTH+2, 4*SCREEN_WIDTH+1, 3*SCREEN_WIDTH+0,2*SCREEN_WIDTH+1, 1*SCREEN_WIDTH+2,

	3*SCREEN_WIDTH+3+9,
	2*SCREEN_WIDTH+3+9, 3*SCREEN_WIDTH+4+9, 4*SCREEN_WIDTH+3+9, 3*SCREEN_WIDTH+2+9, 
	1*SCREEN_WIDTH+3+9, 2*SCREEN_WIDTH+4+9, 3*SCREEN_WIDTH+5+9, 4*SCREEN_WIDTH+4+9, 5*SCREEN_WIDTH+3+9, 4*SCREEN_WIDTH+2+9, 3*SCREEN_WIDTH+1+9, 2*SCREEN_WIDTH+2+9, 
	0*SCREEN_WIDTH+3+9, 1*SCREEN_WIDTH+4+9, 2*SCREEN_WIDTH+5+9, 3*SCREEN_WIDTH+6+9, 4*SCREEN_WIDTH+5+9, 5*SCREEN_WIDTH+4+9, 6*SCREEN_WIDTH+3+9, 5*SCREEN_WIDTH+2+9, 4*SCREEN_WIDTH+1+9, 3*SCREEN_WIDTH+0+9,2*SCREEN_WIDTH+1+9, 1*SCREEN_WIDTH+2+9,

	12*SCREEN_WIDTH+3,
	11*SCREEN_WIDTH+3, 12*SCREEN_WIDTH+4, 13*SCREEN_WIDTH+3, 12*SCREEN_WIDTH+2, 
	10*SCREEN_WIDTH+3, 11*SCREEN_WIDTH+4, 12*SCREEN_WIDTH+5, 13*SCREEN_WIDTH+4, 14*SCREEN_WIDTH+3, 13*SCREEN_WIDTH+2, 12*SCREEN_WIDTH+1, 11*SCREEN_WIDTH+2, 
	9*SCREEN_WIDTH+3, 10*SCREEN_WIDTH+4, 11*SCREEN_WIDTH+5, 12*SCREEN_WIDTH+6, 13*SCREEN_WIDTH+5, 14*SCREEN_WIDTH+4, 15*SCREEN_WIDTH+3, 14*SCREEN_WIDTH+2, 13*SCREEN_WIDTH+1, 12*SCREEN_WIDTH+0,11*SCREEN_WIDTH+1, 10*SCREEN_WIDTH+2,

	12*SCREEN_WIDTH+3+9,
	11*SCREEN_WIDTH+3+9, 12*SCREEN_WIDTH+4+9, 13*SCREEN_WIDTH+3+9, 12*SCREEN_WIDTH+2+9, 
	10*SCREEN_WIDTH+3+9, 11*SCREEN_WIDTH+4+9, 12*SCREEN_WIDTH+5+9, 13*SCREEN_WIDTH+4+9, 14*SCREEN_WIDTH+3+9, 13*SCREEN_WIDTH+2+9, 12*SCREEN_WIDTH+1+9, 11*SCREEN_WIDTH+2+9, 
	9*SCREEN_WIDTH+3+9, 10*SCREEN_WIDTH+4+9, 11*SCREEN_WIDTH+5+9, 12*SCREEN_WIDTH+6+9, 13*SCREEN_WIDTH+5+9, 14*SCREEN_WIDTH+4+9, 15*SCREEN_WIDTH+3+9, 14*SCREEN_WIDTH+2+9, 13*SCREEN_WIDTH+1+9, 12*SCREEN_WIDTH+0+9, 11*SCREEN_WIDTH+1+9, 10*SCREEN_WIDTH+2+9,

	7*SCREEN_WIDTH+7,
	6*SCREEN_WIDTH+7, 7*SCREEN_WIDTH+8, 8*SCREEN_WIDTH+7, 7*SCREEN_WIDTH+6, 
	5*SCREEN_WIDTH+7, 6*SCREEN_WIDTH+8, 7*SCREEN_WIDTH+9, 8*SCREEN_WIDTH+8, 9*SCREEN_WIDTH+7, 8*SCREEN_WIDTH+6, 7*SCREEN_WIDTH+5, 6*SCREEN_WIDTH+6, 
	4*SCREEN_WIDTH+7, 5*SCREEN_WIDTH+8, 6*SCREEN_WIDTH+9, 7*SCREEN_WIDTH+10, 8*SCREEN_WIDTH+9, 9*SCREEN_WIDTH+8, 10*SCREEN_WIDTH+7, 9*SCREEN_WIDTH+6, 8*SCREEN_WIDTH+5, 7*SCREEN_WIDTH+4,6*SCREEN_WIDTH+5, 5*SCREEN_WIDTH+6,
	
	3*SCREEN_WIDTH+3+5,
	2*SCREEN_WIDTH+3+5, 3*SCREEN_WIDTH+4+5, 4*SCREEN_WIDTH+3+5, 3*SCREEN_WIDTH+2+5, 
	1*SCREEN_WIDTH+3+5, 2*SCREEN_WIDTH+4+5, 3*SCREEN_WIDTH+5+5, 4*SCREEN_WIDTH+4+5, 5*SCREEN_WIDTH+3+5, 4*SCREEN_WIDTH+2+5, 3*SCREEN_WIDTH+1+5, 2*SCREEN_WIDTH+2+5, 
	0*SCREEN_WIDTH+3+5, 1*SCREEN_WIDTH+4+5, 2*SCREEN_WIDTH+5+5, 3*SCREEN_WIDTH+6+5, 4*SCREEN_WIDTH+5+5, 5*SCREEN_WIDTH+4+5, 6*SCREEN_WIDTH+3+5, 5*SCREEN_WIDTH+2+5, 4*SCREEN_WIDTH+1+5, 3*SCREEN_WIDTH+0+5,2*SCREEN_WIDTH+1+5, 1*SCREEN_WIDTH+2+5,
	
	12*SCREEN_WIDTH+3+5,
	11*SCREEN_WIDTH+3+5, 12*SCREEN_WIDTH+4+5, 13*SCREEN_WIDTH+3+5, 12*SCREEN_WIDTH+2+5, 
	10*SCREEN_WIDTH+3+5, 11*SCREEN_WIDTH+4+5, 12*SCREEN_WIDTH+5+5, 13*SCREEN_WIDTH+4+5, 14*SCREEN_WIDTH+3+5, 13*SCREEN_WIDTH+2+5, 12*SCREEN_WIDTH+1+5, 11*SCREEN_WIDTH+2+5, 
	9*SCREEN_WIDTH+3+5, 10*SCREEN_WIDTH+4+5, 11*SCREEN_WIDTH+5+5, 12*SCREEN_WIDTH+6+5, 13*SCREEN_WIDTH+5+5, 14*SCREEN_WIDTH+4+5, 15*SCREEN_WIDTH+3+5, 14*SCREEN_WIDTH+2+5, 13*SCREEN_WIDTH+1+5, 12*SCREEN_WIDTH+0+5,11*SCREEN_WIDTH+1+5, 10*SCREEN_WIDTH+2+5,
};	

static const divoom_screen_color_info color_table_white[7][5] = {
	{
		{0, 0, 0},							//°×
		{255-(25*6), 255-(25*6), 255-(25*6)}, 	//°µ
		{255-(25*4), 255-(25*4), 255-(25*4)},
		{255-(25*2), 255-(25*2), 255-(25*2)},
		{255, 255, 255}
	},
	{
		{0, 0, 0},							//°×
		{255-(25*6), 255-(25*6), 255-(25*6)}, 	//°µ
		{255-(25*4), 255-(25*4), 255-(25*4)},
		{255-(25*2), 255-(25*2), 255-(25*2)},
		{255, 255, 255}
	},
	{
		{0, 0, 0},							//°×
		{255-(25*6), 255-(25*6), 255-(25*6)}, 	//°µ
		{255-(25*4), 255-(25*4), 255-(25*4)},
		{255-(25*2), 255-(25*2), 255-(25*2)},
		{255, 255, 255}
	},
	{
		{0, 0, 0},							//°×
		{255-(25*6), 255-(25*6), 255-(25*6)}, 	//°µ
		{255-(25*4), 255-(25*4), 255-(25*4)},
		{255-(25*2), 255-(25*2), 255-(25*2)},
		{255, 255, 255}
	},
	{
		{0, 0, 0},							//°×
		{255-(25*6), 255-(25*6), 255-(25*6)}, 	//°µ
		{255-(25*4), 255-(25*4), 255-(25*4)},
		{255-(25*2), 255-(25*2), 255-(25*2)},
		{255, 255, 255}
	},
	{
		{0, 0, 0},							//°×
		{255-(25*6), 255-(25*6), 255-(25*6)}, 	//°µ
		{255-(25*4), 255-(25*4), 255-(25*4)},
		{255-(25*2), 255-(25*2), 255-(25*2)},
		{255, 255, 255}
	},
	{
		{0, 0, 0},							//°×
		{255-(25*6), 255-(25*6), 255-(25*6)}, 	//°µ
		{255-(25*4), 255-(25*4), 255-(25*4)},
		{255-(25*2), 255-(25*2), 255-(25*2)},
		{255, 255, 255}
	}
};

static const divoom_screen_color_info color_table[7][5] = {
	{
		{0, 0, 0},							//°×			
		{255-(25*6), 255-(25*6), 255-(25*6)}, 	//°µ
		{255-(25*4), 255-(25*4), 255-(25*4)},
		{255-(25*2), 255-(25*2), 255-(25*2)},		
		{255, 255, 255}			
	},
	{
		{0, 0, 0},							//ÂÌ
		{0, 255-(25*6), 0},
		{0, 255-(25*4), 0},
		{0, 255-(25*2), 0},
		{0, 255, 0}
	},
	{
		{0, 0, 0},							//À¶
		{0, 0, 255-(25*6)},
		{0, 0, 255-(25*4)},
		{0, 0, 255-(25*2)},
		{0, 0, 255}
	},
	{
		{0, 0, 0},							//³È
		{255-(25*6), 144-(14*6), 0},		
		{255-(25*4), 144-(14*4), 0},
		{255-(25*2), 144-(14*2), 0},
		{255,144,0}
	},
	{
		{0, 0, 0},							//»Æ
		{255-(25*6), 255-(25*6), 0},
		{255-(25*4), 255-(25*4), 0},
		{255-(25*2), 255-(25*2), 0},		
		{255,255,0},	 
	},
	{
		{0, 0, 0},							 //Çà
		{0, 255-(25*6), 255-(25*6)},
		{0, 255-(25*4), 255-(25*4)},		
		{0, 255-(25*2), 255-(25*2)},
		{0,255,255},
	},
	{
		{0, 0, 0},							 //ºì
		{255-(25*6), 0, 0},
		{255-(25*4), 0, 0},		
		{255-(25*2), 0, 0},
		{255, 0, 0},
	}
};

//uint32 divoom_light_start_blink_init()
uint32 divoom_light_start_blink_init(bool flag)
{	
	T_U8 i = 0, j = 0, k = 0, lcd_id = 2;
	
	if(pdivoom_light_start_blink_handle)
	{
		return 0;
	}
	pdivoom_light_start_blink_handle = divoom_system_malloc(sizeof(divoom_light_start_blink));
	if(pdivoom_light_start_blink_handle == AK_NULL)
	{
		divoom_print("%s:%d: it fails to malloc!\n",__FILE__, __LINE__);
		return 0; 
	}
	memset(pdivoom_light_start_blink_handle, 0, sizeof(divoom_light_start_blink));

	pdivoom_light_start_blink_handle->flag = flag;

	for (i = 0; i < START_NUM; i++)
	{
		if (i == 0)
		{						
			do{
				pdivoom_light_start_blink_handle->start_info[i].index = DIVOOM_LIGHT_START_BLINK_GRAD_GET_RAND(DOT_NUM);	
			}while (pdivoom_light_start_blink_handle->start_info[i].index == pdivoom_light_start_blink_handle->start_info[1].index || 
			pdivoom_light_start_blink_handle->start_info[i].index == pdivoom_light_start_blink_handle->start_info[2].index || 
			pdivoom_light_start_blink_handle->start_info[i].index == pdivoom_light_start_blink_handle->start_info[3].index || 
			pdivoom_light_start_blink_handle->start_info[i].index == pdivoom_light_start_blink_handle->start_info[4].index);					
		}
		else if (i == 1)
		{
			do{
				pdivoom_light_start_blink_handle->start_info[i].index = DIVOOM_LIGHT_START_BLINK_GRAD_GET_RAND(DOT_NUM);	
			}while (pdivoom_light_start_blink_handle->start_info[i].index == pdivoom_light_start_blink_handle->start_info[0].index || 
			pdivoom_light_start_blink_handle->start_info[i].index == pdivoom_light_start_blink_handle->start_info[2].index || 
			pdivoom_light_start_blink_handle->start_info[i].index == pdivoom_light_start_blink_handle->start_info[3].index || 
			pdivoom_light_start_blink_handle->start_info[i].index == pdivoom_light_start_blink_handle->start_info[4].index);					
		}
		else if (i == 2)
		{
			do{
				pdivoom_light_start_blink_handle->start_info[i].index = DIVOOM_LIGHT_START_BLINK_GRAD_GET_RAND(DOT_NUM);	
			}while (pdivoom_light_start_blink_handle->start_info[i].index == pdivoom_light_start_blink_handle->start_info[0].index || 
			pdivoom_light_start_blink_handle->start_info[i].index == pdivoom_light_start_blink_handle->start_info[1].index || 
			pdivoom_light_start_blink_handle->start_info[i].index == pdivoom_light_start_blink_handle->start_info[3].index || 
			pdivoom_light_start_blink_handle->start_info[i].index == pdivoom_light_start_blink_handle->start_info[4].index);					
		}
		else if (i == 3)
		{
			do{
				pdivoom_light_start_blink_handle->start_info[i].index = DIVOOM_LIGHT_START_BLINK_GRAD_GET_RAND(DOT_NUM);	
			}while (pdivoom_light_start_blink_handle->start_info[i].index == pdivoom_light_start_blink_handle->start_info[0].index || 
			pdivoom_light_start_blink_handle->start_info[i].index == pdivoom_light_start_blink_handle->start_info[1].index || 
			pdivoom_light_start_blink_handle->start_info[i].index == pdivoom_light_start_blink_handle->start_info[2].index || 
			pdivoom_light_start_blink_handle->start_info[i].index == pdivoom_light_start_blink_handle->start_info[4].index);					
		}
		else
		{
			do{
				pdivoom_light_start_blink_handle->start_info[i].index = DIVOOM_LIGHT_START_BLINK_GRAD_GET_RAND(DOT_NUM);	
			}while (pdivoom_light_start_blink_handle->start_info[i].index == pdivoom_light_start_blink_handle->start_info[0].index || 
			pdivoom_light_start_blink_handle->start_info[i].index == pdivoom_light_start_blink_handle->start_info[1].index || 
			pdivoom_light_start_blink_handle->start_info[i].index == pdivoom_light_start_blink_handle->start_info[3].index || 
			pdivoom_light_start_blink_handle->start_info[i].index == pdivoom_light_start_blink_handle->start_info[2].index);					
		}
		
		pdivoom_light_start_blink_handle->start_info[i].color_index = DIVOOM_LIGHT_START_BLINK_GRAD_GET_RAND(COLOR_NUM);					
	}		

	for (i = 0; i < START_NUM; i++)
	{
		pdivoom_light_start_blink_handle->start_info[i].light = GRAPH_SIZE + 1;
		for (j = 0; j < pdivoom_light_start_blink_handle->start_info[i].light; j++)
		{
			for (k = 0; k < GRAPH_BIG_NUM - (GRAPH_SIZE- j) * 4; k++)
			{			
				if(pdivoom_light_start_blink_handle->flag == AK_TRUE)
				{
					divoom_disp_ctrl_set_data(data_table[pdivoom_light_start_blink_handle->start_info[i].index * TABLE_NUM + 1 + 4 * ((j - 1) * j)/2 + k], 
							&color_table_white[pdivoom_light_start_blink_handle->start_info[i].color_index][pdivoom_light_start_blink_handle->start_info[i].light - j], pdivoom_light_start_blink_handle->disp_buf); 
				}
				else
				{
					divoom_disp_ctrl_set_data(data_table[pdivoom_light_start_blink_handle->start_info[i].index * TABLE_NUM + 1 + 4 * ((j - 1) * j)/2 + k], &color_table[pdivoom_light_start_blink_handle->start_info[i].color_index][pdivoom_light_start_blink_handle->start_info[i].light - j], pdivoom_light_start_blink_handle->disp_buf); 
				}
			}
		}					
		if(pdivoom_light_start_blink_handle->flag == AK_TRUE)
		{
			divoom_disp_ctrl_set_data(data_table[pdivoom_light_start_blink_handle->start_info[i].index * TABLE_NUM], 
					&color_table_white[pdivoom_light_start_blink_handle->start_info[i].color_index][pdivoom_light_start_blink_handle->start_info[i].light], pdivoom_light_start_blink_handle->disp_buf);				
		}
		else
		{
			divoom_disp_ctrl_set_data(data_table[pdivoom_light_start_blink_handle->start_info[i].index * TABLE_NUM], &color_table[pdivoom_light_start_blink_handle->start_info[i].color_index][pdivoom_light_start_blink_handle->start_info[i].light], pdivoom_light_start_blink_handle->disp_buf);				
		}
		pdivoom_light_start_blink_handle->start_info[i].light++;
		if (pdivoom_light_start_blink_handle->start_info[i].light >= GRAPH_SIZE + 2)
		{
			pdivoom_light_start_blink_handle->start_info[i].light = GRAPH_SIZE + 1;
			pdivoom_light_start_blink_handle->start_info[i].flag = DIVOOM_LIGHT_START_INIT;			
		}
	}	
	divoom_screen_disp_rgb_data(pdivoom_light_start_blink_handle->disp_buf);
	return 0;
}

void divoom_light_start_blink_set_light()
{
	T_U8  i = 0, k = 0;
	T_U16 j = 0;
	
	for (i = 0; i < START_NUM; i++)
	{	
		if (pdivoom_light_start_blink_handle->start_info[i].flag == DIVOOM_LIGHT_START_ADD_LIGHT)
		{
			pdivoom_light_start_blink_handle->start_info[i].time_limite++;
			if ((pdivoom_light_start_blink_handle->start_info[i].time_limite) % (1) == 0)
			{
				if (pdivoom_light_start_blink_handle->start_info[i].light < 0)
					pdivoom_light_start_blink_handle->start_info[i].light = 0;
				for (j = 0; j < pdivoom_light_start_blink_handle->start_info[i].light; j++)
				{
					for (k = 0; k < GRAPH_BIG_NUM - (GRAPH_SIZE- j) * 4; k++)
					{			
						if(pdivoom_light_start_blink_handle->flag == AK_TRUE)
						{
							divoom_disp_ctrl_set_data(data_table[pdivoom_light_start_blink_handle->start_info[i].index * TABLE_NUM + 1 + 4 * ((j - 1) * j)/2 + k], 
									&color_table_white[pdivoom_light_start_blink_handle->start_info[i].color_index][pdivoom_light_start_blink_handle->start_info[i].light - j], pdivoom_light_start_blink_handle->disp_buf); 
						}
						else
						{
							divoom_disp_ctrl_set_data(data_table[pdivoom_light_start_blink_handle->start_info[i].index * TABLE_NUM + 1 + 4 * ((j - 1) * j)/2 + k], &color_table[pdivoom_light_start_blink_handle->start_info[i].color_index][pdivoom_light_start_blink_handle->start_info[i].light - j], pdivoom_light_start_blink_handle->disp_buf); 
						}
					}
				}					
				if(pdivoom_light_start_blink_handle->flag == AK_TRUE)
				{
					divoom_disp_ctrl_set_data(data_table[pdivoom_light_start_blink_handle->start_info[i].index * TABLE_NUM], 
							&color_table_white[pdivoom_light_start_blink_handle->start_info[i].color_index][pdivoom_light_start_blink_handle->start_info[i].light], pdivoom_light_start_blink_handle->disp_buf);				
				}
				else
				{
					divoom_disp_ctrl_set_data(data_table[pdivoom_light_start_blink_handle->start_info[i].index * TABLE_NUM], &color_table[pdivoom_light_start_blink_handle->start_info[i].color_index][pdivoom_light_start_blink_handle->start_info[i].light], pdivoom_light_start_blink_handle->disp_buf);				
				}
				pdivoom_light_start_blink_handle->start_info[i].light++;
				if (pdivoom_light_start_blink_handle->start_info[i].light >= GRAPH_SIZE + 2)
				{
					pdivoom_light_start_blink_handle->start_info[i].light = GRAPH_SIZE + 1;
					pdivoom_light_start_blink_handle->start_info[i].flag = DIVOOM_LIGHT_START_REDUCE_LIGHT;			
				}
			}
		}
		else if (pdivoom_light_start_blink_handle->start_info[i].flag == DIVOOM_LIGHT_START_REDUCE_LIGHT)
		{			
			if (pdivoom_light_start_blink_handle->start_info[i].time_limite > 0)			
				pdivoom_light_start_blink_handle->start_info[i].time_limite--;	
			if ((pdivoom_light_start_blink_handle->start_info[i].time_limite) %  (1)	== 0)
			{
				pdivoom_light_start_blink_handle->start_info[i].light--;
				if (pdivoom_light_start_blink_handle->start_info[i].light < 0)
				{
					pdivoom_light_start_blink_handle->start_info[i].index = DIVOOM_LIGHT_INDEX_END;
					pdivoom_light_start_blink_handle->start_info[i].flag = DIVOOM_LIGHT_START_END;
					pdivoom_light_start_blink_handle->start_info[i].time_limite = 0;
				}			
				else
				{
					for (j = 0; j <= pdivoom_light_start_blink_handle->start_info[i].light; j++)
					{
						for (k = 0; k < GRAPH_BIG_NUM - (GRAPH_SIZE - j) * 4; k++)
						{			
							if(pdivoom_light_start_blink_handle->flag == AK_TRUE)
							{
								divoom_disp_ctrl_set_data(data_table[pdivoom_light_start_blink_handle->start_info[i].index * TABLE_NUM + 1 + 4 * ((j - 1) * j)/2 + k], 
										&color_table_white[pdivoom_light_start_blink_handle->start_info[i].color_index][pdivoom_light_start_blink_handle->start_info[i].light - j], pdivoom_light_start_blink_handle->disp_buf); 
							}
							else
							{
								divoom_disp_ctrl_set_data(data_table[pdivoom_light_start_blink_handle->start_info[i].index * TABLE_NUM + 1 + 4 * ((j - 1) * j)/2 + k], &color_table[pdivoom_light_start_blink_handle->start_info[i].color_index][pdivoom_light_start_blink_handle->start_info[i].light - j], pdivoom_light_start_blink_handle->disp_buf); 
							}
						}
					}				
					if(pdivoom_light_start_blink_handle->flag == AK_TRUE)
					{
						divoom_disp_ctrl_set_data(data_table[pdivoom_light_start_blink_handle->start_info[i].index * TABLE_NUM], 
								&color_table_white[pdivoom_light_start_blink_handle->start_info[i].color_index][pdivoom_light_start_blink_handle->start_info[i].light], pdivoom_light_start_blink_handle->disp_buf);
					}
					else
					{
						divoom_disp_ctrl_set_data(data_table[pdivoom_light_start_blink_handle->start_info[i].index * TABLE_NUM], &color_table[pdivoom_light_start_blink_handle->start_info[i].color_index][pdivoom_light_start_blink_handle->start_info[i].light], pdivoom_light_start_blink_handle->disp_buf);
					}
				}
			}
		}	
		else if (pdivoom_light_start_blink_handle->start_info[i].flag == DIVOOM_LIGHT_START_INIT)
		{
			pdivoom_light_start_blink_handle->start_info[i].flag = DIVOOM_LIGHT_START_REDUCE_LIGHT;
		}
		else
		{	
			pdivoom_light_start_blink_handle->start_info[i].time_limite++;
			if ((pdivoom_light_start_blink_handle->start_info[i].time_limite) % (2 + i*1) == 0)
			{
				if (i == 0)
				{						
					do{
						pdivoom_light_start_blink_handle->start_info[i].index = DIVOOM_LIGHT_START_BLINK_GRAD_GET_RAND(DOT_NUM2);	
					}while (pdivoom_light_start_blink_handle->start_info[i].index == pdivoom_light_start_blink_handle->start_info[1].index || 
					pdivoom_light_start_blink_handle->start_info[i].index == pdivoom_light_start_blink_handle->start_info[2].index || 
					pdivoom_light_start_blink_handle->start_info[i].index == pdivoom_light_start_blink_handle->start_info[3].index || 
					pdivoom_light_start_blink_handle->start_info[i].index == pdivoom_light_start_blink_handle->start_info[4].index);					
				}
				else if (i == 1)
				{
					do{
						pdivoom_light_start_blink_handle->start_info[i].index = DIVOOM_LIGHT_START_BLINK_GRAD_GET_RAND(DOT_NUM2);	
					}while (pdivoom_light_start_blink_handle->start_info[i].index == pdivoom_light_start_blink_handle->start_info[0].index || 
					pdivoom_light_start_blink_handle->start_info[i].index == pdivoom_light_start_blink_handle->start_info[2].index || 
					pdivoom_light_start_blink_handle->start_info[i].index == pdivoom_light_start_blink_handle->start_info[3].index || 
					pdivoom_light_start_blink_handle->start_info[i].index == pdivoom_light_start_blink_handle->start_info[4].index);					
				}
				else if (i == 2)
				{
					do{
						pdivoom_light_start_blink_handle->start_info[i].index = DIVOOM_LIGHT_START_BLINK_GRAD_GET_RAND(DOT_NUM2);	
					}while (pdivoom_light_start_blink_handle->start_info[i].index == pdivoom_light_start_blink_handle->start_info[0].index || 
					pdivoom_light_start_blink_handle->start_info[i].index == pdivoom_light_start_blink_handle->start_info[1].index || 
					pdivoom_light_start_blink_handle->start_info[i].index == pdivoom_light_start_blink_handle->start_info[3].index || 
					pdivoom_light_start_blink_handle->start_info[i].index == pdivoom_light_start_blink_handle->start_info[4].index);					
				}
				else if (i == 3)
				{
					do{
						pdivoom_light_start_blink_handle->start_info[i].index = DIVOOM_LIGHT_START_BLINK_GRAD_GET_RAND(DOT_NUM2);	
					}while (pdivoom_light_start_blink_handle->start_info[i].index == pdivoom_light_start_blink_handle->start_info[0].index || 
					pdivoom_light_start_blink_handle->start_info[i].index == pdivoom_light_start_blink_handle->start_info[1].index || 
					pdivoom_light_start_blink_handle->start_info[i].index == pdivoom_light_start_blink_handle->start_info[2].index || 
					pdivoom_light_start_blink_handle->start_info[i].index == pdivoom_light_start_blink_handle->start_info[4].index);					
				}
				else
				{
					do{
						pdivoom_light_start_blink_handle->start_info[i].index = DIVOOM_LIGHT_START_BLINK_GRAD_GET_RAND(DOT_NUM2);	
					}while (pdivoom_light_start_blink_handle->start_info[i].index == pdivoom_light_start_blink_handle->start_info[0].index || 
					pdivoom_light_start_blink_handle->start_info[i].index == pdivoom_light_start_blink_handle->start_info[1].index || 
					pdivoom_light_start_blink_handle->start_info[i].index == pdivoom_light_start_blink_handle->start_info[3].index || 
					pdivoom_light_start_blink_handle->start_info[i].index == pdivoom_light_start_blink_handle->start_info[2].index);					
				}
				pdivoom_light_start_blink_handle->start_info[i].color_index = DIVOOM_LIGHT_START_BLINK_GRAD_GET_RAND(COLOR_NUM);					
				pdivoom_light_start_blink_handle->start_info[i].flag = DIVOOM_LIGHT_START_ADD_LIGHT;
			}
		}
	}
}

void divoom_light_start_blink_set_data(uint8 *disp_buf)
{
	memcpy(disp_buf, pdivoom_light_start_blink_handle->disp_buf, SCREEN_BUF_SIZE);
}

void divoom_light_start_blink_main(uint8 *disp_buf, uint8 music_power, uint32 task_id)
{				
	uint8 time = 0;
	
	time = SCREEN_HEIGHT - music_power;
	if(pdivoom_light_start_blink_handle->flag == AK_FALSE)
	{
		divoom_com_update_task(task_id, time);
	}
	divoom_light_start_blink_set_light();
	divoom_light_start_blink_set_data(disp_buf);
}

void divoom_light_start_blink_close()
{
	if (pdivoom_light_start_blink_handle)
	{
		divoom_system_free(pdivoom_light_start_blink_handle);
		pdivoom_light_start_blink_handle = AK_NULL;
	}
}
