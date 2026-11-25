#ifndef _divoom_drawing_json_h_
#define _divoom_drawing_json_h_

#define PIC_NUM_16 60
#define PIC_NUM_32 24
#define PIC_NUM_64 6
#define LOCAL_IMAGE_PACK_LEN (100*1024) 

typedef struct _divoom_scroll_set_json_image_info
{
    int mode;
	int speed;
}divoom_scroll_set_json_image_info, *Pdivoom_scroll_set_json_image_info;


typedef struct _divoom_drawing_json_image_info
{
	uint8 speed;
	int pic_num;
	uint8 *pic_data;
}divoom_drawing_json_image_info, *Pdivoom_drawing_json_image_info;

typedef struct _divoom_drawing_json_sync_image_info
{
    int UserId;
	uint8 color[3];
	int num;
	uint16 *dot_pos;
}divoom_drawing_json_sync_image_info, *Pdivoom_drawing_json_sync_image_info; 


typedef struct _divoom_draw_remote_json_image_info
{
    int flag;
    int author_id;
    char author_name[200];
    char author_file_id[64];
    char author_file_name[64];
    char image_name[200];
	char file_id[64];
	char voice_id[64];
}divoom_draw_remote_json_image_info, *Pdivoom_draw_remote_json_image_info;


typedef struct _divoom_draw_local_json_image_info
{
	uint32 device_id;
	uint32 user_id;
	int image_flag; //动画标识
	uint32 total_len; //该动画的总长度
	uint32 packet_id;
	uint32 offset; //当前分包的偏移
	uint32 pic_len; //当前分包的长度
	uint8 *pic_data; //分包数据
}divoom_draw_local_json_image_info, *Pdivoom_draw_local_json_image_info;

typedef struct _divoom_draw_remote_by_app_json_image_info
{
	uint32 device_id;
	uint32 user_id;
	uint16 file_id_len; //文件id的长度
	char file_id[256]; //用文件id来区分服务器上的动画
	uint32 total_len; //该动画的总长度
	uint32 packet_id;
	uint32 offset; //当前分包的偏移
	uint32 pic_len; //当前分包的长度
	uint8 *pic_data; //分包数据
}divoom_draw_remote_by_app_json_image_info, *Pdivoom_draw_remote_by_app_json_image_info;

typedef struct _divoom_draw_set_info_json_image_info
{
	uint16 gif_speed; //动画速度
	uint8 scorll_mode;
	uint16 unicode_str[128];
	uint8 unicode_len; //多少个文字
	uint16 text_speed; //文字速度
	uint8 font_id;
	uint8 text_font; //字体
	uint8 text_effect; //文字效果
	uint8 pic_effect; //图片效果
	uint8 text_start_x;
	uint8 text_start_y;
	uint8 text_frame_width;
	uint8 text_frame_hight;
	uint8 text_size;
	uint8 text_color[3];
}divoom_draw_set_info_json_image_info, *Pdivoom_draw_set_info_json_image_info;

typedef struct _divoom_draw_set_speed_json_mode_info
{
	uint16 speed;
	uint8 scroll_mode;
}divoom_draw_set_speed_json_mode_info, *Pdivoom_draw_set_speed_json_mode_info;

typedef struct _divoom_drawing_json_http_gif_info
{
    int pic_num;        //总共多少张
	int pic_off;        //图片偏移
	int pic_speed;        //图片偏移
	int image_flag;     //图片唯一标识
	int pic_width;      //图片宽度
	uint8 pic_buf[64*64*3];     //图片缓冲区
}divoom_drawing_json_http_gif_info, *Pdivoom_drawing_json_http_gif_info; 

typedef struct _divoom_drawing_json_text_gif_info
{
    int type;
    int id;
	int x; 
	int y; 
	int dir; 
	int font_id; 
	int width;
	int high;
	int speed; 
    int align;
    int request_time;
    int disp_time;
    uint8 color[3];
	char str[514];     
}divoom_drawing_json_http_text_info, *Pdivoom_drawing_json_http_text_info; 

typedef struct _divoom_drawing_json_http_item_list_info
{
    int num;
    Pdivoom_drawing_json_http_text_info pitem_list;
}divoom_drawing_json_http_item_list_info, *Pdivoom_drawing_json_http_item_list_info; 

void divoom_drawing_json_sync_send_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_drawing_json_sync_send_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_drawing_json_send_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_scroll_set_json_send_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_scroll_set_json_send_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_drawing_json_send_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_drawing_json_sync_exit_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_drawing_json_sync_exit_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);


int divoom_sand_paint_json_send_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_remote_json_send_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_local_json_send_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info); //本地动画
int divoom_local_json_pixel_local_http_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_local_json_eq_local_http_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_drawing_set_info_json_send_reques_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_drawing_set_speed_mode_json_send_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_drawing_json_need_send_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_drawing_json_need_send_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_sand_paint_json_send_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_drawing_json_need_local_data_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_drawing_json_send_http_gif_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_drawing_json_send_http_text_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_drawing_json_send_http_item_list_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_drawing_remote_binary_send_request_unpack(uint8 *data, int data_len, Pdivoom_net_command pcmd_info);
void divoom_drawing_get_by_app_json_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_drawing_json_http_command_url_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

#endif

