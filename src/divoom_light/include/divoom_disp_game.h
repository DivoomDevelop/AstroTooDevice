#ifndef _divoom_disp_game_h_
#define _divoom_disp_game_h_

																				//定义16*16游戏
#define DIVOOM_GAME_SMALL_WIDTH		SCREEN_WIDTH_16
#define DIVOOM_GAME_SMALL_HIGH		SCREEN_HEIGHT_16
#define DIVOOM_GAME_SMALL_BUF_SIZE	(DIVOOM_GAME_SMALL_WIDTH*DIVOOM_GAME_SMALL_HIGH*3)

#define DIVOOM_GAME_StartPage_Time	15

typedef void (*DIVOOM_GAME_FUNC_HANDLE_LOOP_CALLBACK)(void);
typedef void (*DIVOOM_GAME_FUNC_KEY_CALLBACK)(uint8 key);
typedef void (*DIVOOM_GAME_FUNC_START_CALLBACK)(void);
typedef void (*DIVOOM_GAME_FUNC_CLOSE_CALLBACK)(void);
typedef uint8 (*DIVOOM_GAME_FUNC_GET_STATU_CALLBACK)(void);

typedef enum{
	DIVOOM_GAME_PIXELS_16X16 = 0,
	DIVOOM_GAME_PIXELS_32X32 = 1,
}DIVOOM_GAME_PIXELS; //像素点数

typedef struct _divoom_disp_game_ctrl
{
    uint32 task_id;
    uint8 game_id;
    uint16 reflesh_time;
    uint8 tmp_buf[SCREEN_BUF_SIZE_32];
    void *decode_handle;    //解压句柄
    uint32 run_times;
	DIVOOM_GAME_FUNC_HANDLE_LOOP_CALLBACK func_handle_loop_callback;
	DIVOOM_GAME_FUNC_KEY_CALLBACK func_handle_key_callback;
	DIVOOM_GAME_FUNC_START_CALLBACK func_handle_start_callback;
	DIVOOM_GAME_FUNC_CLOSE_CALLBACK func_handle_close_callback;
	DIVOOM_GAME_FUNC_GET_STATU_CALLBACK func_handle_get_statu_callback;
}divoom_disp_game_ctrl, *Pdivoom_disp_game_ctrl;


//字模
typedef struct __game_model_info
{
	uint16 *map;
	uint8 size_x;
	uint8 size_y;
}game_model_info,*Pgame_model_info;

//游戏桌面影射表
typedef struct __game_board_info
{
	uint8 *buf;
	uint8 w;
	uint8 h;
}game_board_info,*Pgame_board_info;


typedef int8 (*DIVOOM_GAME_MODEL_COMPARE_FUNC)(uint8 type);

enum
{
    DIVOOM_DISP_GAME_START,
    DIVOOM_DISP_GAME_RUN,
    DIVOOM_DISP_GAME_CMD,
    DIVOOM_DISP_GAME_PAUSE,
    DIVOOM_DISP_GAME_END,    //结束过程
    DIVOOM_DISP_GAME_RESULT,
    DIVOOM_DISP_GAME_EXIT,    //退出选择菜单
};

#if 1
enum
{
	DIVOOM_DISP_GAME_TETRIS = 0,            //俄罗斯方块 
	DIVOOM_DISP_GAME_TARGE = 1,             //老虎机             
	DIVOOM_DISP_GAME_DICE = 2,              //骰子
	DIVOOM_DISP_GAME_ANSWER = 3,            //黑8
	DIVOOM_DISP_GAME_BALL = 4,              //弹球
	DIVOOM_DISP_GAME_FLAPPY_BIRD = 5,       //飞行小鸟
	DIVOOM_DISP_GAME_SNAKE =6,				//贪吃蛇
	DIVOOM_DISP_GAME_CAR = 7,               //赛车
	DIVOOM_DISP_GAME_BLOCKHIT = 8,			//方块消除
	DIVOOM_DISP_GAME_FROGGER = 9,			//青蛙过河
	DIVOOM_DISP_GAME_GOBANG = 10,			//五字棋
	DIVOOM_DISP_GAME_MatchPic = 11,			//图形匹配
	DIVOOM_DISP_GAME_Supperzzle = 12,		//对对碰
	DIVOOM_DISP_GAME_PLANE = 13,            //微信打飞机
	DIVOOM_DISP_GAME_Galaxian = 14,			//小蜜蜂
	DIVOOM_DISP_GAME_TANK = 15,				//坦克大战

	DIVOOM_DISP_GAME_MAX_TYPE,
	DIVOOM_DISP_GAME_GUESS,             //
};
#else
enum
{
	DIVOOM_DISP_GAME_TANK,				//坦克大战
    DIVOOM_DISP_GAME_PLANE,             //飞机
	DIVOOM_DISP_GAME_Supperzzle,		//对对碰
	DIVOOM_DISP_GAME_MatchPic,			//图形匹配
	DIVOOM_DISP_GAME_FROGGER,			//青蛙过河
    DIVOOM_DISP_GAME_GOBANG,			//五字棋
	DIVOOM_DISP_GAME_Galaxian,			//小蜜蜂
    DIVOOM_DISP_GAME_TETRIS,            //俄罗斯方块 
    DIVOOM_DISP_GAME_FLAPPY_BIRD,       //飞行小鸟
    DIVOOM_DISP_GAME_BLOCKHIT,			//方块消除
    DIVOOM_DISP_GAME_SNAKE,             //贪吃蛇
    DIVOOM_DISP_GAME_CAR,               //赛车
    DIVOOM_DISP_GAME_ANSWER,            //黑8
    DIVOOM_DISP_GAME_DICE,              //骰子
    DIVOOM_DISP_GAME_BALL,              //弹球
    DIVOOM_DISP_GAME_TARGE,             //老虎机             

    DIVOOM_DISP_GAME_MAX_TYPE,
    DIVOOM_DISP_GAME_GUESS,             //
};

#endif

#define DIVOOM_GAME_VOICE_VOL (60)

enum{
    DIVOOM_DISP_GAME_APP_KEY_LEFT = 1,
    DIVOOM_DISP_GAME_APP_KEY_RIGHT, 
    DIVOOM_DISP_GAME_APP_KEY_UP,
    DIVOOM_DISP_GAME_APP_KEY_DOWN,     
    DIVOOM_DISP_GAME_APP_KEY_OK,
    DIVOOM_DISP_GAME_APP_KEY_START,
    DIVOOM_DISP_GAME_APP_KEY_CANCLE,    

    DIVOOM_DISP_GAME_REMOTE_KEY_TURN_LEFT = 0x20,
    DIVOOM_DISP_GAME_REMOTE_KEY_TURN_RIGHT, 
    DIVOOM_DISP_GAME_REMOTE_KEY_OK,
    
    DIVOOM_DISP_GAME_KEY_LEFT = 0xa0,
    DIVOOM_DISP_GAME_KEY_RIGHT, 
    DIVOOM_DISP_GAME_KEY_UP,
    DIVOOM_DISP_GAME_KEY_DOWN,     
    DIVOOM_DISP_GAME_KEY_OK,
    DIVOOM_DISP_GAME_KEY_START,    
    DIVOOM_DISP_GAME_KEY_NEXT,
    DIVOOM_DISP_GAME_KEY_EXIT,
    DIVOOM_DISP_GAME_KEY_PAUSE,

	DIVOOM_DISP_GAME_KEY_FASTDOWN,
    DIVOOM_DISP_GAME_BLOCKHIT_KEY_0,
    DIVOOM_DISP_GAME_BLOCKHIT_KEY_1,
    DIVOOM_DISP_GAME_BLOCKHIT_KEY_2,
    DIVOOM_DISP_GAME_BLOCKHIT_KEY_3,
    DIVOOM_DISP_GAME_SNAKE_KEY_TURN_LEFT,
    DIVOOM_DISP_GAME_SNAKE_KEY_TURN_RIGHT,
    DIVOOM_DISP_GMAE_KEY_NONE,    
};

void divoom_disp_game_init(void *user_data);
void divoom_disp_game_set_game(Pdivoom_disp_ctrl_game_info game_info);
void divoom_disp_game_close(void);
void divoom_disp_game_play(uint8 play);
void divoom_disp_game_start(void);


void divoom_disp_game_mode_init(uint8 mode);
void divoom_disp_game_mode_close(void);
void divoom_disp_game_key_function(uint8 key_id);
T_BOOL divoom_disp_game_is_true(void);
void divoom_disp_game_next_mode(void);
void divoom_disp_game_pre_mode(void);
uint8 divoom_disp_game_get_game_mode(void);


/*****************************************************************************
 Prototype    : divoom_disp_game_screen_rgb_data
 Description  : 游戏显示中间接口
 Input        : uint8 *data  
                uint8 flag   DIVOOM_GAME_PIXELS_32X32：32*32的显示数据直接显示， DIVOOM_GAME_PIXELS_16X16：16*16的数据，转一下
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2020/1/16
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_disp_game_screen_rgb_data(uint8 *data,uint8 flag);
void divoom_disp_game_small_screen_ctrl_fill(uint8 *disp_buf, const divoom_screen_color_info * color);
void divoom_disp_game_set_small_dot(int16 x,int16 y, Pdivoom_screen_color_info pcolor, uint8 *disp_buf);
void divoom_disp_game_draw_small_digit(uint8 start_x, uint8 start_y, uint8 user_data, Pdivoom_screen_color_info color, uint8 *disp_buf);
void divoom_disp_game_draw_small_score_digit(uint8 start_x, uint8 start_y, uint8 user_data, uint8 *disp_buf);
void divoom_disp_game_draw_small_score(uint16 score, uint8 *disp_buf);
void divoom_disp_game_show_score(uint16 score, uint32 index, uint8 *disp_buf);
void divoom_disp_game_shine_score(uint16 score, uint8 *disp_buf, uint8 flag);
void divoom_disp_game_exit(void);

void divoom_disp_game_start_page(uint8 *disp_buf,uint8 flag);
uint8 divoom_disp_game_get_game_status(void);

void divoom_disp_game_font_fill_area(int16 start_x, int16 start_y, uint8 size_x, uint8 size_y, uint8 *digit_info, uint8 *disp_buf, Pdivoom_screen_color_info color);
void divoom_disp_game_font_fill_16area(int16 start_x, int16 start_y, uint8 size_x, uint8 size_y, uint16 *digit_info, uint8 *disp_buf, Pdivoom_screen_color_info color);

void divoom_game_draw_map_model(int16 start_x, int16 start_y, game_model_info *pmodel, game_board_info *pboard, uint8 type);
int8 divoom_game_check_map_model_type(int16 start_x, int16 start_y, game_model_info *pmodel, game_board_info *pboard,int16 type);
int8 divoom_game_check_map_model_func(int16 start_x, int16 start_y, game_model_info *pmodel, game_board_info *pboard, DIVOOM_GAME_MODEL_COMPARE_FUNC callback);
void divoom_disp_game_add_32x32result(uint8 *disp_buf,uint8 flag);
//按下按键
void divoom_game_push_app_key(uint8 key);
//释放按键
void divoom_game_release_app_key(uint8 key);

#endif


