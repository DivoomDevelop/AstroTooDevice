#include "lvgl.h"
#include "divoom.h"
#include "divoom_alarm.h"
#include "divoom_menu_control.h"


#define DIVOOM_LVGL_STREAM_CONT_CACHE_NUM   	5
#define DIVOOM_LVGL_CONT_BG_COLOR 				0x000000
#define DIVOOM_LVGL_CONT_FONT_COLOR 			0xFFFFFF
#define DIVOOM_CONT_SELETED_OPA 				LV_OPA_COVER
#define DIVOOM_CONT_UNSELETED_OPA 				LV_OPA_60



typedef struct _divoom_lvgl_stream_cont_item
{
	void* stream_cont;
    int cont_total;
	int cont_font_w;
	int cont_font_h;

	int start_flag;			//0:从0开始 1:从1开始
	int num_min_width;			//0~9的数字宽度 1:1位 2:2位
	int seleted_font_size;
	int unseleted_font_size;
	int pre_sum;
	int cur_sum;
	int speed_level;
	int speed;
	int timer_tick;
	int idle_tick;
	int had_run_step;		//快速移动过程已经走过的步数
	int move_steps;			//总步数
	int opa_move_steps;		//改变的步数用于改变透明度
	int base_pos;
    int dest_pos;
	int last_move_steps;
	int press_y;
	int press_time;
	lv_timer_t* move_timer;
	lv_timer_t* sign_timer;
	int seleted_value;		//选中值
    int pos_y[DIVOOM_LVGL_STREAM_CONT_CACHE_NUM];  
    int   cont_pos[DIVOOM_LVGL_STREAM_CONT_CACHE_NUM];
    void *cont_canvas_obj[DIVOOM_LVGL_STREAM_CONT_CACHE_NUM];//容器对象
    void *cont_obj[DIVOOM_LVGL_STREAM_CONT_CACHE_NUM];//label对象
	
}divoom_lvgl_stream_cont_item, *Pdivoom_lvgl_stream_cont_item;


//Pdivoom_lvgl_stream_cont_item Pdivoom_lvgl_stream_cont_item_handle = NULL;


int divoom_lvgl_stream_cont_get_pre_cont_id(Pdivoom_lvgl_stream_cont_item phandle)
{
    int i, min_pos, find;

    min_pos = phandle->pos_y[0];
    find = 0;
    for(i  = 1; i < DIVOOM_LVGL_STREAM_CONT_CACHE_NUM; i++)
    {
        if(min_pos > phandle->pos_y[i])
        {
            min_pos = phandle->pos_y[i];
            find = i;
        }
    }
    if(phandle->cont_pos[find])
    {
        return phandle->cont_pos[find] - 1;
    }
    else
    {
        return phandle->cont_total - 1;
    }
}

int divoom_lvgl_stream_cont_get_next_cont_id(Pdivoom_lvgl_stream_cont_item phandle)
{
    int i, max_pos, find;

    max_pos = phandle->pos_y[0];
    find = 0;
    for(i  = 1; i < DIVOOM_LVGL_STREAM_CONT_CACHE_NUM; i++)
    {
        if(max_pos < phandle->pos_y[i])
        {
            max_pos = phandle->pos_y[i];
            find = i;
        }
    }
    return (phandle->cont_pos[find]+1)%phandle->cont_total;
}

int divoom_lvgl_stream_cont_find_seleted_cont_pos(Pdivoom_lvgl_stream_cont_item phandle,int seleted_value)
{
	int i,find;

	for(i  = 0; i < DIVOOM_LVGL_STREAM_CONT_CACHE_NUM; i++)
    {
        if(seleted_value == phandle->cont_pos[i])
        {
            return i;
        }
    }
	return phandle->cont_total - 1;
}

void divoom_stream_cont_move(Pdivoom_lvgl_stream_cont_item phandle,int step)  
{
	int y, h, max_high, i, abs_move_steps, other_y, unseleted_value, opa, opa_no, num_font_h;
	int seleted_pos;
	int font_size_no, font_size;
	max_high = phandle->cont_font_h * DIVOOM_LVGL_STREAM_CONT_CACHE_NUM;

	phandle->opa_move_steps += step;
	abs_move_steps = (phandle->opa_move_steps > 0) ? phandle->opa_move_steps :(-phandle->opa_move_steps);
	num_font_h = abs_move_steps / phandle->cont_font_h;
	other_y = abs_move_steps % phandle->cont_font_h;
	//divoom_print("num_font_h = %d,step = %d,abs_opa_move_steps= %d,seleted_value = %d\n", num_font_h,step,abs_move_steps,phandle->seleted_value);

	if(abs_move_steps < phandle->cont_font_h)
	{
		if(phandle->opa_move_steps > 0)
		{
			//不选中后选中的透明度增加
			unseleted_value = (phandle->seleted_value > 0) ? (phandle->seleted_value - 1) : (phandle->cont_total - 1);
			seleted_pos = divoom_lvgl_stream_cont_find_seleted_cont_pos(phandle, unseleted_value);
			opa_no = lv_map(other_y, 0, phandle->cont_font_h, DIVOOM_CONT_UNSELETED_OPA, DIVOOM_CONT_SELETED_OPA);
			lv_obj_set_style_text_opa(phandle->cont_obj[seleted_pos], opa_no, 0);
			if(phandle->seleted_font_size > phandle->unseleted_font_size)
			{
				font_size_no = lv_map(other_y, 0, phandle->cont_font_h, phandle->unseleted_font_size, phandle->seleted_font_size);
				lv_obj_set_style_text_font(phandle->cont_obj[seleted_pos], divoom_menu_control_get_font_info(font_size_no, DIVOOM_MENU_CONTROL_ID_PHOTO_SLIDE_EFFECT), 0);
				lv_obj_center(phandle->cont_obj[seleted_pos]);
			}
			//选中后不选中的透明度减少
			opa = DIVOOM_CONT_SELETED_OPA - lv_map(other_y, 0, phandle->cont_font_h, LV_OPA_0, DIVOOM_CONT_SELETED_OPA - DIVOOM_CONT_UNSELETED_OPA);
			seleted_pos = divoom_lvgl_stream_cont_find_seleted_cont_pos(phandle, phandle->seleted_value);
			lv_obj_set_style_text_opa(phandle->cont_obj[seleted_pos], opa, 0);
			if(phandle->seleted_font_size > phandle->unseleted_font_size)
			{
				font_size = lv_map(other_y, 0, phandle->cont_font_h, 0, phandle->seleted_font_size - phandle->unseleted_font_size);
				lv_obj_set_style_text_font(phandle->cont_obj[seleted_pos], divoom_menu_control_get_font_info(phandle->seleted_font_size - font_size, DIVOOM_MENU_CONTROL_ID_PHOTO_SLIDE_EFFECT), 0);
				lv_obj_center(phandle->cont_obj[seleted_pos]);
			}
		}
		else
		{
			//向上运动
			//不选中后选中的透明度增加
			unseleted_value = ((phandle->seleted_value + 1) < phandle->cont_total) ? (phandle->seleted_value + 1) : 0;
			seleted_pos = divoom_lvgl_stream_cont_find_seleted_cont_pos(phandle, unseleted_value);
			opa_no = lv_map(other_y, 0, phandle->cont_font_h, DIVOOM_CONT_UNSELETED_OPA, DIVOOM_CONT_SELETED_OPA);
			lv_obj_set_style_text_opa(phandle->cont_obj[seleted_pos], opa_no, 0);
			if(phandle->seleted_font_size > phandle->unseleted_font_size)
			{
				font_size_no = lv_map(other_y, 0, phandle->cont_font_h, phandle->unseleted_font_size, phandle->seleted_font_size);
				lv_obj_set_style_text_font(phandle->cont_obj[seleted_pos], divoom_menu_control_get_font_info(font_size_no, DIVOOM_MENU_CONTROL_ID_PHOTO_SLIDE_EFFECT), 0);
				lv_obj_center(phandle->cont_obj[seleted_pos]);
			}
			
			//选中后不选中的透明度减少
			seleted_pos = divoom_lvgl_stream_cont_find_seleted_cont_pos(phandle, phandle->seleted_value);
			opa = DIVOOM_CONT_SELETED_OPA - lv_map(other_y, 0, phandle->cont_font_h, LV_OPA_0, DIVOOM_CONT_SELETED_OPA - DIVOOM_CONT_UNSELETED_OPA);
			lv_obj_set_style_text_opa(phandle->cont_obj[seleted_pos], opa, 0);
			if(phandle->seleted_font_size > phandle->unseleted_font_size)
			{
				font_size = lv_map(other_y, 0, phandle->cont_font_h, 0, phandle->seleted_font_size - phandle->unseleted_font_size);
				lv_obj_set_style_text_font(phandle->cont_obj[seleted_pos], divoom_menu_control_get_font_info(phandle->seleted_font_size - font_size, DIVOOM_MENU_CONTROL_ID_PHOTO_SLIDE_EFFECT), 0);
				lv_obj_center(phandle->cont_obj[seleted_pos]);
			}
		}
	}
	else //if((abs_move_steps >= phandle->cont_font_h) && (num_font_h < 2))
	{
		//选中走到未选中位置
		unseleted_value = phandle->seleted_value;
		seleted_pos = divoom_lvgl_stream_cont_find_seleted_cont_pos(phandle, unseleted_value);
		lv_obj_set_style_text_opa(phandle->cont_obj[seleted_pos], DIVOOM_CONT_UNSELETED_OPA, 0);
		if(phandle->seleted_font_size > phandle->unseleted_font_size)
		{
			lv_obj_set_style_text_font(phandle->cont_obj[seleted_pos], divoom_menu_control_get_font_info(phandle->unseleted_font_size, DIVOOM_MENU_CONTROL_ID_PHOTO_SLIDE_EFFECT), 0);
			lv_obj_center(phandle->cont_obj[seleted_pos]);
		}
		
		if(phandle->opa_move_steps > 0)//更改选中值
		{
			phandle->seleted_value = (phandle->seleted_value > 0) ? (phandle->seleted_value - 1) : (phandle->cont_total - 1);
			unseleted_value = (phandle->seleted_value > 0) ? (phandle->seleted_value - 1) : (phandle->cont_total - 1);
		}
		else
		{			
			phandle->seleted_value = (phandle->seleted_value + 1) % phandle->cont_total;
			unseleted_value = (phandle->seleted_value + 1) % phandle->cont_total;
		}
		phandle->opa_move_steps =  (phandle->opa_move_steps > 0) ? (phandle->opa_move_steps - phandle->cont_font_h) : (phandle->opa_move_steps + phandle->cont_font_h);

		//未选中后选中的透明度增加
		seleted_pos = divoom_lvgl_stream_cont_find_seleted_cont_pos(phandle, unseleted_value);
		opa_no = lv_map(other_y, 0, phandle->cont_font_h, DIVOOM_CONT_UNSELETED_OPA, DIVOOM_CONT_SELETED_OPA);
		lv_obj_set_style_text_opa(phandle->cont_obj[seleted_pos], opa_no, 0);
		if(phandle->seleted_font_size > phandle->unseleted_font_size)
		{
			font_size_no = lv_map(other_y, 0, phandle->cont_font_h, phandle->unseleted_font_size, phandle->seleted_font_size);
			lv_obj_set_style_text_font(phandle->cont_obj[seleted_pos], divoom_menu_control_get_font_info(font_size_no, DIVOOM_MENU_CONTROL_ID_PHOTO_SLIDE_EFFECT), 0);
			lv_obj_center(phandle->cont_obj[seleted_pos]);
		}
		
		//选中后不选中的透明度减少
		seleted_pos = divoom_lvgl_stream_cont_find_seleted_cont_pos(phandle, phandle->seleted_value);
		opa = DIVOOM_CONT_SELETED_OPA - lv_map(other_y, 0, phandle->cont_font_h, LV_OPA_0, DIVOOM_CONT_SELETED_OPA - DIVOOM_CONT_UNSELETED_OPA);
		lv_obj_set_style_text_opa(phandle->cont_obj[seleted_pos], opa, 0);
		if(phandle->seleted_font_size > phandle->unseleted_font_size)
		{
			font_size = lv_map(other_y, 0, phandle->cont_font_h, 0, phandle->seleted_font_size - phandle->unseleted_font_size);
			lv_obj_set_style_text_font(phandle->cont_obj[seleted_pos], divoom_menu_control_get_font_info(phandle->seleted_font_size - font_size, DIVOOM_MENU_CONTROL_ID_PHOTO_SLIDE_EFFECT), 0);
			lv_obj_center(phandle->cont_obj[seleted_pos]);
		}
	}
	
	for(i = 0;i < DIVOOM_LVGL_STREAM_CONT_CACHE_NUM;i++)
	{
		if(phandle->cont_canvas_obj[i] == NULL)
		{
			break;
		}
		h = phandle->pos_y[i];
        y = h + step;
		

		if(y >= (max_high - phandle->cont_font_h))
        {
        	//向下运动
			y -= max_high;
			phandle->cont_pos[i] = divoom_lvgl_stream_cont_get_pre_cont_id(phandle);
			if(phandle->start_flag)
			{
				if(phandle->num_min_width == 2)
				{
					lv_label_set_text_fmt(phandle->cont_obj[i], "%02d", phandle->cont_pos[i] + 1);
				}
				else if(phandle->num_min_width == 1)
				{
					lv_label_set_text_fmt(phandle->cont_obj[i], "%d", phandle->cont_pos[i] + 1);
				}
			}
			else
			{
				if(phandle->num_min_width == 2)
				{
					lv_label_set_text_fmt(phandle->cont_obj[i], "%02d", phandle->cont_pos[i]);
				}
				else if(phandle->num_min_width == 1) 
				{
					lv_label_set_text_fmt(phandle->cont_obj[i], "%d", phandle->cont_pos[i]);
				}
			}
			
		}
		else if(y <= (-phandle->cont_font_h))
		{
			//向上运动
			y += max_high;
			phandle->cont_pos[i] = divoom_lvgl_stream_cont_get_next_cont_id(phandle);
			if(phandle->start_flag)
			{
				if(phandle->num_min_width == 2)
				{
					lv_label_set_text_fmt(phandle->cont_obj[i], "%02d", phandle->cont_pos[i] + 1);
				}
				else if(phandle->num_min_width == 1)
				{
					lv_label_set_text_fmt(phandle->cont_obj[i], "%d", phandle->cont_pos[i] + 1);
				}
			}
			else
			{
				if(phandle->num_min_width == 2)
				{
					lv_label_set_text_fmt(phandle->cont_obj[i], "%02d", phandle->cont_pos[i]);
				}
				else if(phandle->num_min_width == 1)
				{
					lv_label_set_text_fmt(phandle->cont_obj[i], "%d", phandle->cont_pos[i]);
				}
			}
		}
		
		lv_obj_set_y(phandle->cont_canvas_obj[i], y);
		phandle->pos_y[i] = y;
	}
		
	
}

void *divoom_stream_cont_get_cont_obj(void *cont)
{
	Pdivoom_lvgl_stream_cont_item phandle = (Pdivoom_lvgl_stream_cont_item)cont;

	return phandle->stream_cont;
}

int divoom_stream_cont_get_current_selected(void *cont)
{
	Pdivoom_lvgl_stream_cont_item phandle = (Pdivoom_lvgl_stream_cont_item)cont;

	if(phandle->start_flag)
	{
		return (phandle->seleted_value + 1) % (phandle->cont_total + 1); 
	}
	else
	{
		return phandle->seleted_value % phandle->cont_total; 
	}
	
}

int divoom_stream_cont_set_current_selected(void *cont,int index)
{
	int seleted_pos, i, temp, temp_index;
	Pdivoom_lvgl_stream_cont_item phandle = (Pdivoom_lvgl_stream_cont_item)cont;

	//需要定时器暂停才能用
	if(phandle->sign_timer != NULL || phandle->move_timer != NULL)
	{
		return 0;
	}
	
	if(phandle->start_flag)
	{
		if(index > phandle->cont_total || index < 1)
		{
			return 0;
		}
		temp_index = index - 1;
	}
	else
	{
		if(index >= phandle->cont_total || index < 0)
		{
			return 0;
		}
		temp_index = index;
	}
	
	phandle->seleted_value = temp_index;

	for(i = 0;i < DIVOOM_LVGL_STREAM_CONT_CACHE_NUM;i++)
	{
		if(phandle->start_flag)
		{
			temp = index - 3 + i;
		}
		else
		{
			temp = index - 2 + i;
		}
		if(temp >= phandle->cont_total || temp < 0)
		{
			temp = (temp >= phandle->cont_total) ? (temp - phandle->cont_total) : (temp + phandle->cont_total);
		}
		phandle->cont_pos[i] = temp;

		lv_obj_set_pos(phandle->cont_canvas_obj[i], 0, i*phandle->cont_font_h - phandle->cont_font_h);
		phandle->pos_y[i] = i*phandle->cont_font_h - phandle->cont_font_h;
		
		if(phandle->start_flag)
		{
			if(phandle->num_min_width == 2)
			{
				lv_label_set_text_fmt(phandle->cont_obj[i], "%02d", phandle->cont_pos[i] + 1);
			}
			else if(phandle->num_min_width == 1)
			{
				lv_label_set_text_fmt(phandle->cont_obj[i], "%d", phandle->cont_pos[i] + 1);
			}
		}
		else
		{
			if(phandle->num_min_width == 2)
			{
				lv_label_set_text_fmt(phandle->cont_obj[i], "%02d", phandle->cont_pos[i]);
			}
			else if(phandle->num_min_width == 1)
			{
				lv_label_set_text_fmt(phandle->cont_obj[i], "%d", phandle->cont_pos[i]);
			}
		}
		
		if(i == 2)
		{
			lv_obj_set_style_text_font(phandle->cont_obj[i], divoom_menu_control_get_font_info(phandle->seleted_font_size, DIVOOM_MENU_CONTROL_ID_PHOTO_SLIDE_EFFECT), 0);
			lv_obj_set_style_text_opa(phandle->cont_obj[i], DIVOOM_CONT_SELETED_OPA, 0);	
		}
		else
		{
			lv_obj_set_style_text_font(phandle->cont_obj[i], divoom_menu_control_get_font_info(phandle->unseleted_font_size, DIVOOM_MENU_CONTROL_ID_PHOTO_SLIDE_EFFECT), 0);
			lv_obj_set_style_text_opa(phandle->cont_obj[i], DIVOOM_CONT_UNSELETED_OPA, 0);	
		}
	}
	return 1;
}

void divoom_stream_cont_pause_cont(void *cont)
{
	Pdivoom_lvgl_stream_cont_item phandle = (Pdivoom_lvgl_stream_cont_item)cont;
	
	if(phandle->move_timer != NULL)
	{
		lv_timer_pause(phandle->move_timer);
		lv_timer_del(phandle->move_timer);
		phandle->move_timer = NULL;
	}
	if(phandle->sign_timer != NULL)
	{
		lv_timer_pause(phandle->sign_timer);
		lv_timer_del(phandle->sign_timer);
		phandle->sign_timer = NULL;
	}

	phandle->opa_move_steps = 0;
	phandle->had_run_step = 0;
		
	if(phandle->start_flag)
	{
		divoom_stream_cont_set_current_selected(cont, phandle->seleted_value+1);
	}
	else
	{
		divoom_stream_cont_set_current_selected(cont, phandle->seleted_value);
	}
}


static void divoom_lvgl_stream_cont_time_move_task(lv_timer_t *timer)
{
	int move_steps, run_steps,seleted_pos;
    Pdivoom_lvgl_stream_cont_item phandle = (Pdivoom_lvgl_stream_cont_item)lv_timer_get_user_data(timer);

	if(phandle->base_pos == phandle->dest_pos)
    {
        phandle->had_run_step = 0;
        lv_timer_del(phandle->move_timer);
        phandle->move_timer = NULL;
        phandle->timer_tick = 0;
        return;
    }
	
	phandle->idle_tick = 0;
	if(phandle->timer_tick == 0)
    {
        phandle->pre_sum = 0;
        phandle->cur_sum = 0;
    }

	phandle->timer_tick ++;
	phandle->cur_sum += 200*200 - phandle->timer_tick*phandle->timer_tick;

	if(((phandle->cur_sum - phandle->pre_sum) * phandle->speed)<5353300)
    {
    	if(phandle->timer_tick > 190)
    	{
			run_steps = 2;	
		}
		else
		{
        	//不需要移动
        	return;
		}
    }
	else
	{
		run_steps = ((phandle->cur_sum - phandle->pre_sum) * phandle->speed)/5353300;
	}
	//divoom_print("timer_tick = %d,run_steps = %d\n",phandle->timer_tick,run_steps);
	
	phandle->pre_sum += 5353300 * run_steps/(phandle->speed);
    move_steps = phandle->base_pos - phandle->dest_pos;
	
	if(move_steps > 0)//向下运动
    {
        if(move_steps > run_steps)
        {
            move_steps = 0 - run_steps;
        }
        else 
        {
            move_steps = 0 - move_steps;
        }
		if(-move_steps > phandle->cont_font_h)
		{
			move_steps = 0 - phandle->cont_font_h;
		}
        divoom_stream_cont_move(phandle, move_steps);
    }
    else//向上运动
    {
        move_steps = 0 - move_steps;
        if(move_steps > run_steps)
        {
            move_steps = run_steps;
        }
		if(move_steps > phandle->cont_font_h)
		{
			move_steps = phandle->cont_font_h;
		}
        divoom_stream_cont_move(phandle, move_steps);
    }
    phandle->base_pos += move_steps;
	phandle->had_run_step += move_steps;
}


static void divoom_lvgl_stream_cont_time_sign_task(lv_timer_t *timer)
{
	int move_steps, run_steps,seleted_pos;
    Pdivoom_lvgl_stream_cont_item phandle = (Pdivoom_lvgl_stream_cont_item)lv_timer_get_user_data(timer);

	if(phandle->base_pos == phandle->dest_pos)
    {
        lv_timer_del(phandle->sign_timer);
        phandle->sign_timer = NULL;
        return;
    }
	
	phandle->idle_tick =0;
	run_steps = phandle->speed;
    move_steps = phandle->base_pos - phandle->dest_pos;
	
	if(move_steps > 0)//向下运动
    {
        if(move_steps > run_steps)
        {
            move_steps = 0 - run_steps;
        }
        else 
        {
            move_steps = 0 - move_steps;
        }
		if(-move_steps > phandle->cont_font_h)
		{
			move_steps = 0 - phandle->cont_font_h;
		}
        divoom_stream_cont_move(phandle, move_steps);
    }
    else//向上运动
    {
        move_steps = 0 - move_steps;
        if(move_steps > run_steps)
        {
            move_steps = run_steps;
        }
		if(move_steps > phandle->cont_font_h)
		{
			move_steps = phandle->cont_font_h;
		}
        divoom_stream_cont_move(phandle, move_steps);
    }
    phandle->base_pos += move_steps;
}


void divoom_stream_cont_move_speedup(Pdivoom_lvgl_stream_cont_item phandle,int speed,int sum_steps) 
{	
	if(phandle->move_timer == NULL)
	{
		phandle->move_timer = lv_timer_create(divoom_lvgl_stream_cont_time_move_task, LV_DISP_DEF_REFR_PERIOD, phandle);
	}
}

void divoom_stream_cont_key_roll_down(void *cont)
{
	//往下移动一个单位,速度不变
	Pdivoom_lvgl_stream_cont_item phandle = (Pdivoom_lvgl_stream_cont_item)cont;
	
	phandle->dest_pos += phandle->cont_font_h;
	phandle->speed = phandle->cont_font_h/8;
	//phandle->seleted_value = (phandle->seleted_value > 0) ? (phandle->seleted_value - 1) : (phandle->cont_total - 1);
	
	if(phandle->sign_timer == NULL)
	{
		phandle->sign_timer = lv_timer_create(divoom_lvgl_stream_cont_time_sign_task, 16, phandle);
	}
	lv_timer_resume(phandle->sign_timer);
	
}

void divoom_stream_cont_key_roll_up(void *cont)
{
	//往上移动一个单位,速度不变
	Pdivoom_lvgl_stream_cont_item phandle = (Pdivoom_lvgl_stream_cont_item)cont;
	
	phandle->dest_pos -= phandle->cont_font_h;
	phandle->speed = phandle->cont_font_h/8;
	//phandle->seleted_value = (phandle->seleted_value + 1) % phandle->cont_total;
	
	if(phandle->sign_timer == NULL)
	{
		phandle->sign_timer = lv_timer_create(divoom_lvgl_stream_cont_time_sign_task, 16, phandle);
	}
	lv_timer_resume(phandle->sign_timer);
}


static void divoom_lvgl_stream_cont_event_cb(lv_event_t * e)
{
	lv_point_t p1;
	lv_event_code_t code = lv_event_get_code(e);
	Pdivoom_lvgl_stream_cont_item phandle = (Pdivoom_lvgl_stream_cont_item)lv_event_get_user_data(e);
   
	switch(code)
	{
        case LV_EVENT_KEY:
            divoom_print("event: %d\n", code);		
            break;
            
		case LV_EVENT_PRESSED:
	        lv_indev_get_point(lv_indev_get_act(), &p1);
            phandle->move_steps = 0;
			phandle->last_move_steps = 0;
            phandle->press_y = p1.y;
            phandle->press_time = divoom_system_get_tickms();
            if(phandle->move_timer != NULL)
            {
                lv_timer_pause(phandle->move_timer);
            }
			break;
		case LV_EVENT_PRESSING:
			lv_indev_get_point(lv_indev_get_act(), &p1);
			if((p1.y - phandle->press_y > phandle->cont_font_h) || (p1.y - phandle->press_y < -phandle->cont_font_h))
			{
				return;
			}
			if((phandle->last_move_steps >= 0 && p1.y - phandle->press_y < -phandle->cont_font_h/2) || (phandle->last_move_steps <= 0 &&  p1.y - phandle->press_y > phandle->cont_font_h/2))
			{
				return;
			}
			phandle->last_move_steps = p1.y - phandle->press_y;
			divoom_stream_cont_move(phandle, p1.y - phandle->press_y);
            phandle->move_steps += (p1.y - phandle->press_y);
            phandle->press_y = p1.y;
			break;
		case LV_EVENT_RELEASED:
		{
			int abs_move_steps,abs_had_move_step,seleted_pos,move_step,other_step,press_time = divoom_system_get_tickms() - phandle->press_time;

			abs_move_steps = phandle->move_steps>0?phandle->move_steps:(0-phandle->move_steps);
			abs_had_move_step = phandle->had_run_step>0?phandle->had_run_step:(0-phandle->had_run_step);
			
			//divoom_print("abs_move_steps = %d , press_time = %d, abs_move_steps * 500 / press_time = %d\n", abs_move_steps,press_time,abs_move_steps * 500 / press_time);
			if(abs_move_steps * 400 / press_time > phandle->cont_font_h)
			{
				phandle->dest_pos = 0;
                phandle->timer_tick = 0;    
				if(phandle->move_timer == NULL)
                {
                    phandle->speed_level = 1;
                    phandle->speed = 7 * phandle->cont_font_h + 4 * abs_move_steps/3;
                }
                else if(phandle->speed_level == 1)
                {
                    phandle->speed_level = 2;
                    phandle->speed = 14 * phandle->cont_font_h + 6 * abs_move_steps/5;
                }
				else
				{
					phandle->speed_level = 3;
					phandle->speed = 21 * phandle->cont_font_h + 8 * abs_move_steps/7;
				}
				//divoom_print("1:move_step = %d, had_run_step= %d,move_step = %d \n",phandle->move_steps,phandle->had_run_step,move_step);

				if(phandle->move_steps > 0 && phandle->had_run_step < 0)//方向相反
				{
					phandle->had_run_step = phandle->cont_font_h - (abs_had_move_step % phandle->cont_font_h);
				}
				else if(phandle->move_steps < 0 && phandle->had_run_step > 0)//方向相反
				{
					phandle->had_run_step = abs_had_move_step % phandle->cont_font_h - phandle->cont_font_h;
				}
				abs_had_move_step = phandle->had_run_step>0?phandle->had_run_step:(0-phandle->had_run_step);
				
				other_step = (abs_had_move_step + abs_move_steps + phandle->speed) % phandle->cont_font_h;
				phandle->had_run_step += phandle->move_steps;
				move_step = phandle->speed - other_step;
				
				//divoom_print("2:move_step = %d, had_run_step= %d,move_step = %d \n",phandle->move_steps,phandle->had_run_step,move_step);
				if(phandle->move_steps > 0)
				{
					phandle->dest_pos = phandle->base_pos + move_step;
					divoom_stream_cont_move_speedup(phandle, phandle->speed, move_step);
				}
				else
				{
					phandle->dest_pos = phandle->base_pos - move_step;
					divoom_stream_cont_move_speedup(phandle, phandle->speed, -move_step);
				}
				lv_timer_resume(phandle->move_timer);
			}
            else if(phandle->move_timer == NULL || phandle->move_timer->paused == true)
            {
            	if(phandle->move_timer != NULL)
            	{
            		phandle->had_run_step = 0;
					lv_timer_del(phandle->move_timer);
					phandle->move_timer = NULL;
				}
				seleted_pos = divoom_lvgl_stream_cont_find_seleted_cont_pos(phandle,phandle->seleted_value);

				phandle->dest_pos = phandle->base_pos + phandle->cont_font_h - phandle->pos_y[seleted_pos];//移动到选中对齐
				phandle->speed = phandle->cont_font_h/6;
				if(phandle->sign_timer == NULL)
				{
					phandle->sign_timer = lv_timer_create(divoom_lvgl_stream_cont_time_sign_task, 16, phandle);
				}
				lv_timer_resume(phandle->sign_timer);
            }
			else
            {	
            	phandle->had_run_step = 0;
    			lv_timer_del(phandle->move_timer);
                phandle->move_timer = NULL;
            }

			break;
		}
        default:
            break;
	}
}


void* divoom_stream_cont_init(void* main_obj,int x,int y,int cont_w,int cont_h,int cont_font_w,int cont_font_h, int unseleted_font_size, int seleted_font_size, int font_color_hex, int num_font_cont,int start_flag,int num_min_width)
{
	int i;
	char disp_str[32];
	Pdivoom_lvgl_stream_cont_item phandle;

	phandle = (Pdivoom_lvgl_stream_cont_item)divoom_system_malloc(sizeof(divoom_lvgl_stream_cont_item));
	memset(phandle, 0, sizeof(divoom_lvgl_stream_cont_item));
	
	phandle->cont_total = num_font_cont;
	//phandle->stream_cont = lv_obj_create(main_obj);
	phandle->stream_cont = lv_btn_create(main_obj);
	lv_obj_remove_style_all(phandle->stream_cont);
	lv_obj_set_size(phandle->stream_cont, cont_w, cont_h);
	lv_obj_set_pos(phandle->stream_cont, x, y);
	lv_obj_set_style_bg_opa(phandle->stream_cont, LV_OPA_TRANSP, 0);
	lv_obj_clear_flag(phandle->stream_cont, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_clear_flag(phandle->stream_cont, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_style_pad_all(phandle->stream_cont, 0, LV_PART_MAIN);
	lv_obj_set_style_border_width(phandle->stream_cont, 0, 0);
	lv_obj_set_style_radius(phandle->stream_cont, 0, 0);

	phandle->cont_font_h = cont_font_h;
	phandle->cont_font_w = cont_font_w;
	phandle->start_flag = start_flag;
	phandle->seleted_font_size = seleted_font_size;
	phandle->unseleted_font_size = unseleted_font_size;
	phandle->num_min_width = num_min_width;
		
	for(i = 0;i < DIVOOM_LVGL_STREAM_CONT_CACHE_NUM;i++)
	{ 
		phandle->cont_canvas_obj[i] = lv_obj_create(phandle->stream_cont);

		lv_obj_set_style_radius(phandle->cont_canvas_obj[i], 0, 0);
		lv_obj_set_size(phandle->cont_canvas_obj[i], cont_font_w, cont_font_h);
		lv_obj_set_pos(phandle->cont_canvas_obj[i], 0, i*cont_font_h - cont_font_h);
		lv_obj_set_x(phandle->cont_canvas_obj[i], 0);
		lv_obj_set_width(phandle->cont_canvas_obj[i], cont_font_w);
		lv_obj_clear_flag(phandle->cont_canvas_obj[i], LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_set_style_pad_all(phandle->cont_canvas_obj[i], 0, LV_PART_MAIN);
		lv_obj_set_style_border_width(phandle->cont_canvas_obj[i], 0, 0);
		lv_obj_set_style_bg_opa(phandle->cont_canvas_obj[i], LV_OPA_TRANSP, 0);

		phandle->cont_obj[i] = lv_label_create(phandle->cont_canvas_obj[i]);
		phandle->pos_y[i] = i*cont_font_h - cont_font_h;
		phandle->cont_pos[i] = i%num_font_cont;
		lv_obj_set_size(phandle->cont_obj[i], cont_font_w, cont_font_h);
		

		if(start_flag)
		{
			if(num_min_width == 2)
			{
				lv_label_set_text_fmt(phandle->cont_obj[i], "%02d", phandle->cont_pos[i] + 1);
			}
			else if(num_min_width == 1)
			{
				lv_label_set_text_fmt(phandle->cont_obj[i], "%d", phandle->cont_pos[i] + 1);
			}
		}
		else
		{
			if(num_min_width == 2)
			{
				lv_label_set_text_fmt(phandle->cont_obj[i], "%02d", phandle->cont_pos[i]);
			}
			else if(num_min_width == 1)
			{
				lv_label_set_text_fmt(phandle->cont_obj[i], "%d", phandle->cont_pos[i]);
			}
		}
		
		lv_obj_set_style_text_color(phandle->cont_obj[i], lv_color_hex(font_color_hex), 0);
		lv_obj_set_style_text_align(phandle->cont_obj[i], LV_TEXT_ALIGN_CENTER, 0);
		lv_obj_center(phandle->cont_obj[i]);

		if(i == 2)
		{	
			lv_obj_set_style_text_font(phandle->cont_obj[i], divoom_menu_control_get_font_info(seleted_font_size, DIVOOM_MENU_CONTROL_ID_PHOTO_SLIDE_EFFECT), 0);
			lv_obj_set_style_text_opa(phandle->cont_obj[i], DIVOOM_CONT_SELETED_OPA, 0);	
			phandle->seleted_value = i;
		}
		else
		{
			lv_obj_set_style_text_font(phandle->cont_obj[i], divoom_menu_control_get_font_info(unseleted_font_size, DIVOOM_MENU_CONTROL_ID_PHOTO_SLIDE_EFFECT), 0);
			lv_obj_set_style_text_opa(phandle->cont_obj[i], DIVOOM_CONT_UNSELETED_OPA, 0);	
		}
		//lv_obj_add_event_cb(phandle->cont_canvas_obj[i], divoom_lvgl_stream_cont_event_cb, LV_EVENT_ALL, phandle);	
	}
   	
	return phandle;

}


void divoom_stream_cont_del(void* cont)
{
	Pdivoom_lvgl_stream_cont_item phandle = (Pdivoom_lvgl_stream_cont_item)cont;

	if(phandle->move_timer)
	{
		lv_timer_pause(phandle->move_timer);
		lv_timer_del(phandle->move_timer);
	}
	if(phandle->sign_timer)
	{
		lv_timer_pause(phandle->sign_timer);
		lv_timer_del(phandle->sign_timer);
	}
	lv_obj_del(phandle->stream_cont);
	divoom_system_free(phandle);

}


