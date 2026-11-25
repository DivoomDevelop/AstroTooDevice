#include "divoom.h"

#define DIVOOM_PIC_RAW_SIZE 768

#define OUT




T_U16  divoom_format_unpack_uint16(T_U8 *data)
{
    T_U16 value;

    value = data[1] + ((T_U32)data[0] << 8);
    return value;
}


T_U32  divoom_format_unpack_uint32(T_U8 *data)
{
    T_U32 value;

    value = data[3] + ((T_U32)data[2] << 8) + ((T_U32)data[1] << 16) + ((T_U32)data[0] << 24);
    return value;
}

uint8 *divoom_format_mul_pic(uint8 *data, DIVOOM_UINT_ID len, uint8 pic_num, uint8 device_num, OUT uint32 *dest_len)
{
	uint8 *pic_aes_data = NULL;
	//固定16*16的大小
	DIVOOM_UINT_ID return_dest_len = 0;
	uint32 all_pic_data_len = SCREEN_WIDTH*SCREEN_HEIGHT*BYTES_PRE_DOT*device_num*pic_num;
	pic_aes_data = divoom_system_malloc(sizeof(uint8)*all_pic_data_len);

	if(divoom_zlib_decompress(pic_aes_data, &return_dest_len, data, len) < 0 || return_dest_len != all_pic_data_len)
	{
		divoom_free(pic_aes_data);
		return NULL;
	}
	*dest_len = return_dest_len;
	return pic_aes_data;
}

/*
speed(2bytes) + width(2bytes) + height(2bytes) + raw_pic(768) + steps(4bytes) + [dots(2bytes) +rgb(3bytes) + dot_off(dots)]
*/

uint8 *divoom_format_sand_paint_to_buf(int *out_len, uint8 *data, int len)
{
    int review_pic, read_off, pic_len, encode_size = 0, i, tol_step,dots, base_off;
    uint16 speed_time;
    uint8 *sand_paint_buf;


    review_pic = data[0];
    read_off = review_pic * DIVOOM_PIC_RAW_SIZE + 1;
    sand_paint_buf = divoom_system_malloc(len - read_off + DIVOOM_PIC_RAW_SIZE + 4);
    divoom_print("divoom_format_sand_paint_to_buf: test :%d\n",len - read_off);
    speed_time = divoom_format_unpack_uint16(data + read_off);
    divoom_com_pack_uint16(sand_paint_buf, speed_time); 
    divoom_com_pack_uint16(sand_paint_buf + 2, SCREEN_WIDTH); 
    divoom_com_pack_uint16(sand_paint_buf + 4, SCREEN_HEIGHT); 
    read_off += 2;
    encode_size = 6;
    if(data[read_off])
    {
        read_off ++;    
        pic_len = DIVOOM_PIC_RAW_SIZE;
        memcpy(sand_paint_buf + encode_size, &data[read_off], pic_len);
        encode_size += pic_len;
        read_off += DIVOOM_PIC_RAW_SIZE;
    }
    else
    {
        memset(sand_paint_buf + encode_size, 0, DIVOOM_PIC_RAW_SIZE);
        pic_len = DIVOOM_PIC_RAW_SIZE;
        encode_size += pic_len;
        read_off ++;
    }
    tol_step = divoom_format_unpack_uint32(&data[read_off]);
    read_off += 4;
    base_off = read_off;
    for(i = 0, pic_len = 0; i < tol_step; i ++)
    {
        dots = divoom_format_unpack_uint16(&data[read_off]);
        read_off += 2 + dots + 3;
        pic_len += 2 + dots + 3;
    }
    memcpy(sand_paint_buf + encode_size, &data[base_off], pic_len);
    encode_size += pic_len;
    *out_len = encode_size;
    return sand_paint_buf;
    
}

int divoom_format_sand_paint(FILE *save_file_handle, uint8 *data, int len)
{
    int review_pic, read_off, pic_len, encode_size = 0, i, tol_step,dots, base_off;
    uint16 speed_time, tmp;
    uint8 *head_buf;

    head_buf = divoom_system_malloc(DIVOOM_PIC_RAW_SIZE);
    memset(head_buf, 0, DIVOOM_PAGE_SIZE);
    divoom_port_file_write(head_buf, DIVOOM_PAGE_SIZE, 1, save_file_handle); 
    review_pic = data[0];
    read_off = review_pic * DIVOOM_PIC_RAW_SIZE + 1;
    speed_time = divoom_format_unpack_uint16(data + read_off);
    divoom_port_file_write(&speed_time, 2, 1, save_file_handle); 
    tmp = 16;
    divoom_port_file_write(&tmp, 2, 1, save_file_handle); 
    divoom_port_file_write(&tmp, 2, 1, save_file_handle); 
    read_off += 2;
    encode_size = 6;
    if(data[read_off])
    {
        read_off ++;    
        pic_len = DIVOOM_PIC_RAW_SIZE;
        divoom_port_file_write(&data[read_off], pic_len, 1, save_file_handle); 
        encode_size += pic_len;
        read_off += DIVOOM_PIC_RAW_SIZE;
    }
    else
    {
        memset(head_buf, 0, DIVOOM_PIC_RAW_SIZE);
        pic_len = DIVOOM_PIC_RAW_SIZE;
        divoom_port_file_write(head_buf, pic_len, 1, save_file_handle); 
        encode_size += pic_len;
        read_off ++;
    }
    tol_step = divoom_format_unpack_uint32(&data[read_off]);
    read_off += 4;
    base_off = read_off;
    for(i = 0, pic_len = 0; i < tol_step; i ++)
    {
        dots = divoom_format_unpack_uint16(&data[read_off]);
        read_off += 2 + dots + 3;
        pic_len += 2 + dots + 3;
    }
    for(i = 0; i < pic_len; i += 1024)
    {
        if(pic_len > i + 1024)
        {
            divoom_port_file_write(&data[base_off + i], 1024, 1, save_file_handle); 
        }
        else
        {
            divoom_port_file_write(&data[base_off + i], pic_len - i, 1, save_file_handle); 
        }
    }
    encode_size += pic_len;
    divoom_port_file_seek(save_file_handle, 0, SEEK_SET);
    divoom_com_pack_uint32(head_buf, encode_size);
    head_buf[4] = DIVOOM_IMAGE_LOCAL_TYPE_SAND_PAINT;
    divoom_port_file_write(head_buf, DIVOOM_PAGE_SIZE, 1, save_file_handle); 
    divoom_system_free(head_buf);
    return encode_size;
    
}

/*
speed(2bytes) + width(2bytes) + height(2bytes)+ rgb_data(3*width*height) + steps(4bytes) + [dots(2bytes) +rgb(3bytes) + dot_off(id+off,2bytes)]
*/

uint8 *divoom_format_new_mul_sand_paint_to_buf(int *out_len, uint8 *data, int len)
{
    int review_pic, read_off, pic_len, encode_size = 0, i, tol_step,dots, base_off, width, height;
    uint16 speed_time;
    uint8 *sand_paint_buf;


    width = data[0];
    height = data[1];
    read_off = 2;
    review_pic = data[read_off];
    read_off = review_pic * width * height * DIVOOM_PIC_RAW_SIZE + 3;
    sand_paint_buf = divoom_system_malloc(len - read_off + width * height *DIVOOM_PIC_RAW_SIZE + 10);
    speed_time = divoom_format_unpack_uint16(data + read_off);
    divoom_com_pack_uint16(sand_paint_buf, speed_time); 
    divoom_com_pack_uint16(sand_paint_buf + 2, width * SCREEN_WIDTH);
    divoom_com_pack_uint16(sand_paint_buf + 4, height * SCREEN_HEIGHT); 
    read_off += 2;
    encode_size = 6;
    if(data[read_off])
    {
        read_off ++;    
        pic_len = width * height * DIVOOM_PIC_RAW_SIZE;
        memcpy(sand_paint_buf + encode_size, &data[read_off], pic_len);
        encode_size += pic_len;
        read_off += width * height * DIVOOM_PIC_RAW_SIZE;
    }
    else
    {
        memset(sand_paint_buf + encode_size, 0, width * height * DIVOOM_PIC_RAW_SIZE);
        pic_len = width * height * DIVOOM_PIC_RAW_SIZE;
        encode_size += pic_len;
        read_off ++;
    }
    tol_step = divoom_format_unpack_uint32(&data[read_off]);
    read_off += 4;
    base_off = read_off;
    for(i = 0, pic_len = 0; i < tol_step; i ++)
    {
        dots = divoom_format_unpack_uint16(&data[read_off]);
        read_off += 2 + dots*2 + 3;
        pic_len += 2 + dots*2 + 3;
    }
    memcpy(sand_paint_buf + encode_size, &data[base_off], pic_len);
    encode_size += pic_len;
    *out_len = encode_size;
    return sand_paint_buf;
    
}

int divoom_format_new_mul_sand_paint(FILE *save_file_handle, uint8 *data, int len)
{
    int review_pic, read_off, pic_len, encode_size = 0, i, tol_step,dots, base_off;
    uint16 speed_time, heigth, width, tmp;
    uint8 *head_buf;

    width = data[0];
    heigth = data[1];
    read_off = 2;
    head_buf = divoom_system_malloc(DIVOOM_PIC_RAW_SIZE);
    memset(head_buf, 0, DIVOOM_PAGE_SIZE);
    divoom_port_file_write(head_buf, DIVOOM_PAGE_SIZE, 1, save_file_handle); 
    review_pic = data[read_off];
    read_off = review_pic * width * heigth * DIVOOM_PIC_RAW_SIZE + 3;
    speed_time = divoom_format_unpack_uint16(data + read_off);
    divoom_port_file_write(&speed_time, 2, 1, save_file_handle); 
    tmp = width * SCREEN_WIDTH;
    divoom_port_file_write(&tmp, 2, 1, save_file_handle); 
    tmp = heigth * SCREEN_HEIGHT;
    divoom_port_file_write(&tmp, 2, 1, save_file_handle); 
    read_off += 2;
    encode_size = 6;
    if(data[read_off])
    {
        read_off ++;    
        pic_len = width * heigth * DIVOOM_PIC_RAW_SIZE;
        divoom_port_file_write(&data[read_off], pic_len, 1, save_file_handle); 
        encode_size += pic_len;
        read_off += width * heigth * DIVOOM_PIC_RAW_SIZE;
    }
    else
    {
        memset(head_buf, 0, width * heigth * DIVOOM_PIC_RAW_SIZE);
        pic_len = width * heigth * DIVOOM_PIC_RAW_SIZE;
        divoom_port_file_write(head_buf, pic_len, 1, save_file_handle); 
        encode_size += pic_len;
        read_off ++;
    }
    tol_step = divoom_format_unpack_uint32(&data[read_off]);
    read_off += 4;
    base_off = read_off;
    for(i = 0, pic_len = 0; i < tol_step; i ++)
    {
        dots = divoom_format_unpack_uint16(&data[read_off]);
        read_off += 2 + dots*2 + 3;
        pic_len += 2 + dots*2 + 3;
    }
    for(i = 0; i < pic_len; i += 1024)
    {
        if(pic_len > i + 1024)
        {
            divoom_port_file_write(&data[base_off + i], 1024, 1, save_file_handle); 
        }
        else
        {
            divoom_port_file_write(&data[base_off + i], pic_len - i, 1, save_file_handle); 
        }
    }
    encode_size += pic_len;
    divoom_port_file_seek(save_file_handle, 0, SEEK_SET);
    divoom_com_pack_uint32(head_buf, encode_size);
    head_buf[4] = DIVOOM_IMAGE_LOCAL_TYPE_SAND_PAINT;
    divoom_port_file_write(head_buf, DIVOOM_PAGE_SIZE, 1, save_file_handle); 
    divoom_system_free(head_buf);
    return encode_size;
    
}

void divoom_format_pic_set_dot(int dest_off,  uint8 *dest_buf, int src_off, uint8 *src_buf)
{    
    dest_off = (dest_off << 1) + dest_off;
    src_off = (src_off << 1) + src_off;
    dest_buf[dest_off] = src_buf[src_off];
    dest_buf[dest_off+1] = src_buf[src_off+1];
    dest_buf[dest_off+1] = src_buf[src_off+1];
}

int divoom_format_pic_convert_position(int small_x, int small_y,int pos_w, int pos_h,int width, int height)
{
    int big_off, big_x, big_y;

    big_x = small_x + pos_w * SCREEN_WIDTH;
    big_y = small_y + pos_h * SCREEN_HEIGHT;
    big_off = big_x + big_y * (width * SCREEN_WIDTH);
    return big_off;
}

void divoom_format_pic_get_big_pic(uint8 *big_buf, uint8 *small_buf, int pic_num, int width, int height)
{
    int i, j , k, x, y, big_off;
    uint8 *cur_big_buf = big_buf;
    uint8 *cur_small_buf = small_buf;

    for(i = 0; i < pic_num; i++)
    {
        for(j = 0; j < height; j ++)
        {
            for(k = 0; k < width; k ++)
            {
                for(y = 0; y < SCREEN_HEIGHT; y++)
                {
                    for(x = 0; x < SCREEN_WIDTH; x++)
                    {
                        big_off = divoom_format_pic_convert_position(x, y, k, j, width, height);
                        divoom_format_pic_set_dot(big_off, cur_big_buf, y *SCREEN_WIDTH + x, cur_small_buf);
                    }
                }
                cur_small_buf += SCREEN_WIDTH * SCREEN_HEIGHT * 3;
            }
        }
        cur_big_buf += (width*SCREEN_WIDTH) * (height * SCREEN_HEIGHT) * 3;
    }
}


int divoom_format_gif_set(FILE *save_file_handle, uint8 *data, int len)
{
#if 0
    int i, set_num, pic_num, off = 0, speed, encode_size, tol_size = 0, j, width, height;
    uint8 *pic_data, *head_buf, *rgb_data;

    head_buf = divoom_system_malloc(DIVOOM_PAGE_SIZE);
    memset(head_buf, 0, DIVOOM_PAGE_SIZE);
    divoom_port_file_write(head_buf, DIVOOM_PAGE_SIZE, 1, save_file_handle); 

    set_num = data[off];
    off ++;

    for(i = 0; i < set_num; i ++)
    {
        width = data[off];
        off ++;
        height = data[off];
        off ++;
        pic_num = data[off];
        off ++;
        speed = divoom_format_unpack_uint16(&data[off]);
        off += 2;
        rgb_data = divoom_system_malloc(pic_num * width * height * DIVOOM_PIC_RAW_SIZE);
        divoom_format_pic_get_big_pic(rgb_data, &data[off], pic_num, width, height);
        pic_data = divoom_image_encode_encode_mul_pic(rgb_data, pic_num, &encode_size, speed);
        off += pic_num * width * height * DIVOOM_PIC_RAW_SIZE;
        divoom_system_free(rgb_data);
        tol_size += encode_size;
        for(j = 0; j < encode_size; j += 1024)
        {
            if(j + 1024 > encode_size)
            {
                divoom_port_file_write(pic_data + j, encode_size - j, 1, save_file_handle); 
            }
            else
            {
                divoom_port_file_write(pic_data + j, 1024, 1, save_file_handle); 
                divoom_system_delay_ms(10);
            }
        }
        divoom_system_free(pic_data);
    }

    memset(head_buf, 0, DIVOOM_PAGE_SIZE);
    divoom_com_pack_uint32(head_buf , tol_size);
    head_buf[4] = DIVOOM_IMAGE_LOCAL_TYPE_NORMAL;
    divoom_port_file_seek(save_file_handle, 0, SEEK_SET);
    divoom_port_file_write(head_buf, DIVOOM_PAGE_SIZE, 1, save_file_handle); 
    divoom_system_free(head_buf);
    return tol_size;
#endif
    return 0;
}

//这个不知道用不用
//不知道需要什么样的数据格式
uint8 *divoom_format_gif_set_buf(uint8 *data, int len)
{
	uint8 *pic_data = NULL;
	pic_data = divoom_system_malloc(len);
	memcpy(pic_data, data, len);
	return pic_data;
}

/*
如果为滚动画时：
Data[1]:  模式；一字节
Data[2]:速度，二字节，data[2] = speed & 0xFF; data[3] >> 8
Data[4]:长度，二字节，data[4] = len & 0xff, data[5]= len >> 8


*/
int divoom_format_pic_scan(FILE *save_file_handle, uint8 *data, int len, int mode, int speed)
{
    uint8 head_buf[DIVOOM_PAGE_SIZE];
    int pic_len = DIVOOM_PIC_RAW_SIZE * 4;

    memset(head_buf, 0, DIVOOM_PAGE_SIZE);
    divoom_com_pack_uint32(head_buf , pic_len  + 3);
    head_buf[4] = DIVOOM_IMAGE_LOCAL_TYPE_PIC_SCAN;
    divoom_port_file_write(head_buf, DIVOOM_PAGE_SIZE, 1, save_file_handle); 

    head_buf[0] = mode;
    divoom_com_pack_uint16(&head_buf[1], speed);
    divoom_port_file_write(head_buf, 3, 1, save_file_handle); 
    divoom_port_file_write(data , DIVOOM_PIC_RAW_SIZE * 1, 1, save_file_handle); 
    divoom_port_file_write(data + DIVOOM_PIC_RAW_SIZE, DIVOOM_PIC_RAW_SIZE * 1, 1, save_file_handle); 
    divoom_port_file_write(data + DIVOOM_PIC_RAW_SIZE * 2, DIVOOM_PIC_RAW_SIZE * 1, 1, save_file_handle); 
    divoom_port_file_write(data + DIVOOM_PIC_RAW_SIZE * 3, DIVOOM_PIC_RAW_SIZE * 1, 1, save_file_handle); 

    return pic_len + 3;
}

uint8 *divoom_format_pic_scan_rgb(uint8 *file_buf, int *pic_len, int pic_size, int speed, int mode)
{
	uint8 *pic_data, *rgb_data = NULL;
	uint32 minilzo_len;
	
	minilzo_len = DIVOOM_PIC_RAW_SIZE * 4;
	rgb_data = divoom_system_malloc(minilzo_len);
	
	if (rgb_data == NULL)
	{
		return NULL;
	}
	//divoom_minilzo_decompress(file_buf, pic_size, rgb_data, &minilzo_len);
	*pic_len = DIVOOM_PIC_RAW_SIZE * 4 + 3;

	pic_data = divoom_system_malloc(*pic_len);
	pic_data[0] = mode&0xff;
	divoom_com_pack_uint16(&pic_data[1], speed);
	memcpy(&pic_data[3], rgb_data, DIVOOM_PIC_RAW_SIZE * 4);
	divoom_free(rgb_data);
	return pic_data;
}

uint8 *divoom_format_pic_scan_buf(uint8 *file_buf, int *pic_len, int tol_size)
{
    uint8 *pic_data;
    uint16 speed;
    
    *pic_len = DIVOOM_PIC_RAW_SIZE * 4 + 3;
    
    divoom_aes_do(&file_buf[3],tol_size - 3);
    pic_data = divoom_system_malloc(*pic_len);
    speed = divoom_format_unpack_uint16(&file_buf[1]);
    memcpy(pic_data, file_buf, *pic_len );
    divoom_com_pack_uint16(&pic_data[1], speed);
    return pic_data;
}

uint8* divoom_format_pic_single(uint8 *file_buf, int *pic_len, int tol_size, int flag)
{
    uint8 *pic_data = NULL;
/*
    if(flag)
    {
        divoom_aes_do(&file_buf[1], tol_size - 1);
    }

    pic_data = divoom_image_encode_encode_mul_pic(&file_buf[1], 1, pic_len, 100);
*/
    return pic_data;
}

uint8* divoom_format_pic_gif(uint8 *file_buf, int *pic_len, int tol_size, int flag, uint8 iframe_flag)
{
/*    
    uint8 *pic_data = NULL;
    int pic_num, pic_time;
    pic_num = file_buf[1];
    pic_time = divoom_format_unpack_uint16(&file_buf[2]);
	divoom_print("[%s:%d] pic_num %d", __func__, __LINE__, pic_num);
    if(flag)
    {
        divoom_aes_do(&file_buf[4], tol_size - 4);
    }

    if(iframe_flag)
    {
        pic_data = divoom_image_encode_encode_mul_iframe_pic(&file_buf[4], pic_num, pic_len, pic_time);        
    }
    else
    {
        pic_data = divoom_image_encode_encode_mul_pic(&file_buf[4], pic_num, pic_len, pic_time);
    }
*/
    return NULL;
}

uint8 *divoom_format_new_mul_pic_single_handle(uint8 *file_buf, int *pic_len, int form)
{
    #if 0
	uint8 *pic_data;

	pic_data = divoom_image_encode_encode_packet(form, form, 100, file_buf, pic_len); 
	return pic_data;
    #endif
    return NULL;
}

uint8* divoom_format_new_mul_pic_single(uint8 *file_buf, int *pic_len)
{
    #if 0
    uint8 *pic_data;
    uint8 width;
    uint8 height;

    width = file_buf[0];
    height = file_buf[1];
    *pic_len = divoom_format_unpack_uint32(&file_buf[2]);

    pic_data = divoom_image_encode_encode_packet(width * SCREEN_WIDTH , height *SCREEN_WIDTH, 100, &file_buf[6], pic_len);
    return pic_data;
    #endif
    return NULL;
}

#define IN_LEN      (SCREEN_BUF_SIZE)
#define OUT_LEN     (IN_LEN + IN_LEN / 16 + 64 + 3)

#define IN_LEN_32  (SCREEN_BUF_SIZE_32)
#define OUT_LEN_32 (IN_LEN_32 + IN_LEN_32 / 16 + 64 + 3)

#define IN_LEN_64  (SCREEN_BUF_SIZE_64)
#define OUT_LEN_64 (IN_LEN_64 + IN_LEN_64 / 16 + 64 + 3)

#define BUF_LEN (SCREEN_BUF_SIZE_64 + 256 + DIVOOM_IMAGE_ENCODE_DATA_OFF)
#define BUF_LEN_32 (SCREEN_BUF_SIZE_32 + 256 + DIVOOM_IMAGE_ENCODE_DATA_OFF)


uint8 *divoom_format_new_mul_pic_gif(uint8 *file_buf, int *pic_len, int tol_size)
{
    #if 0
	uint8 *pic_data;
	int i, j, pic_num, pic_time, buf_off = 0, raw_off, raw_size, n, m, off1, off2;
	uint8 width;
	uint8 height;

	pic_num = file_buf[0];
	pic_time = divoom_format_unpack_uint16(&file_buf[1]);
	width = file_buf[3];
	height = file_buf[4];
	uint32 minilzo_in_len = 0;
	uint8 *minilzo_in = NULL, *pic_off = NULL;
	uint8 *rgb_data_32 = NULL;
	uint8 *rgb_data_64 = NULL;
	
	uint32 minilzo_out_len = 0;
	uint8 *minilzo_out = NULL; 
	
	if (width == 2)
		minilzo_in = divoom_system_malloc(IN_LEN_32);
	else if (width == 4)
		minilzo_in = divoom_system_malloc(IN_LEN_64);
	if (minilzo_in == NULL)
	{
		divoom_print("[%s:%d] malloc error", __func__, __LINE__);
		return NULL;
	}
	pic_data = divoom_system_malloc(tol_size + 1024 + DIVOOM_IMAGE_ENCODE_DATA_OFF * pic_num);
	if(pic_data == NULL)
	{
		divoom_print("[%s:%d] malloc error", __func__, __LINE__);
		divoom_free(minilzo_in);
		return NULL;
	}
	raw_off = 5;
	if (width == 2)
	{
		rgb_data_32 = divoom_system_malloc(SCREEN_BUF_SIZE_32);
		if (rgb_data_32 == NULL)
		{
			divoom_print("[%s:%d] malloc error", __func__, __LINE__);
			divoom_free(minilzo_in);
			divoom_free(pic_data);
			return NULL;
		}
		minilzo_out = divoom_system_malloc(OUT_LEN_32);
		if (minilzo_out == NULL)
		{
			divoom_print("[%s:%d] malloc error", __func__, __LINE__);
			divoom_free(rgb_data_32);
			divoom_free(minilzo_in);
			divoom_free(pic_data);
			return NULL;
		}
		for(i = 0; i < pic_num; i ++)
		{
			raw_size = divoom_format_unpack_uint32(&file_buf[raw_off]);
			raw_off += 4;
			//minilzo解压成rgb
			divoom_minilzo_decompress(file_buf+raw_off, raw_size, minilzo_in, &minilzo_in_len);
			//divoom_print("[%s:%d] minilzo_in_len %d, pic_num = %d, raw_size = %d", __func__, __LINE__, minilzo_in_len, pic_num, raw_size);
		   	raw_off += raw_size;         
			for (j = 0; j < width*height; j++)
			{
				pic_off = &minilzo_in[j*SCREEN_BUF_SIZE];
				for (n = 0; n < SCREEN_HEIGHT; n++) //拼接
				{
					for (m = 0; m < SCREEN_WIDTH; m++)
					{
						off1 = n*SCREEN_WIDTH + m; //小图的绝对坐标
						off2 = (n + (j/width)*SCREEN_WIDTH)*SCREEN_WIDTH_32 + (j%width)*SCREEN_WIDTH + m; //大图绝对坐标
						off1 = (off1 << 1) + off1;
						off2 = (off2 << 1) + off2;
						rgb_data_32[off2] = pic_off[off1];
						rgb_data_32[off2 + 1] = pic_off[off1 + 1];
						rgb_data_32[off2 + 2] = pic_off[off1 + 2];
					}
				}
			}
			//minilzo压缩
			divoom_minilzo_compress(rgb_data_32, SCREEN_BUF_SIZE_32, minilzo_out, &minilzo_out_len);
			buf_off += divoom_image_encode_encode_packet_to_buf(width*SCREEN_WIDTH, height*SCREEN_WIDTH, pic_time, minilzo_out, minilzo_out_len, pic_data + buf_off);
		}
		divoom_free(rgb_data_32);
		divoom_free(minilzo_out);
	}
	else if (width == 4)
	{
		minilzo_out = (uint8 *)divoom_system_malloc(OUT_LEN_64);
		if (minilzo_out == NULL)
		{
			divoom_print("[%s:%d] malloc error", __func__, __LINE__);
			divoom_free(minilzo_in);
			divoom_free(pic_data);
			return NULL;
		}
		
		rgb_data_64 = (uint8 *)divoom_system_malloc(SCREEN_BUF_SIZE_64);
		if (rgb_data_64 == NULL)
		{
			divoom_print("[%s:%d] malloc error", __func__, __LINE__);
			divoom_free(minilzo_in);
			divoom_free(pic_data);
			divoom_free(rgb_data_64);
			return NULL;
		}
		buf_off = 0;
		for(j = 0; j < pic_num; j++)
		{
			raw_size = divoom_format_unpack_uint32(&file_buf[raw_off]);
	        raw_off += 4;
			//minilzo解压成rgb
			divoom_minilzo_decompress(file_buf+raw_off, raw_size, minilzo_in, &minilzo_in_len);
		   	raw_off += raw_size;         
			for(i = 0; i < width*height; i++) //拼接
		    {
				pic_off = &minilzo_in[i*SCREEN_BUF_SIZE];
				for (n = 0; n < SCREEN_HEIGHT; n++) //拼接
				{
					for (m = 0; m < SCREEN_WIDTH; m++)
					{
						off1 = n*SCREEN_WIDTH + m; //小图的绝对坐标
						off2 = (n + (i/width)*SCREEN_WIDTH)*SCREEN_WIDTH_64 + (i%width)*SCREEN_WIDTH + m; //大图绝对坐标
						off1 = (off1 << 1) + off1;
						off2 = (off2 << 1) + off2;
						rgb_data_64[off2] = pic_off[off1];
						rgb_data_64[off2 + 1] = pic_off[off1 + 1];
						rgb_data_64[off2 + 2] = pic_off[off1 + 2];
					}
				}
		    }
			//minilzo压缩
			divoom_minilzo_compress(rgb_data_64, SCREEN_BUF_SIZE_64, minilzo_out, &minilzo_out_len);
			buf_off += divoom_image_encode_encode_packet_to_buf(width*SCREEN_WIDTH, height*SCREEN_WIDTH, pic_time, minilzo_out, minilzo_out_len, pic_data + buf_off);
		}
		divoom_free(rgb_data_64);
		divoom_free(minilzo_out);
	}
	*pic_len = buf_off;
	divoom_free(minilzo_in);
	return pic_data;
    #else
    return NULL;
    #endif
}

/**********************************************
文件储存格式
前面256字节定义为: 4字节文件大小，+1字节文件类型，+其它数据
**************************************************/
int divoom_format_srv_hot_file(char *net_addr, char *local_file, uint8 *sha1_sum)
{
#if 0
    int real_size, encode_size = 0, out_len, ret = 0, i = 0;
    uint8 *file_buf, head_buf[DIVOOM_PAGE_SIZE], *rgb_data, *encode_data;
	uint8 sha1_buf[64];
	uint8 sha1_exchange[64];
    uint16 pic_time;
    FILE *save_file_handle;
    void *divoom_old_handle;

    save_file_handle = divoom_port_file_open(local_file, "wb");
    if(save_file_handle == NULL)
    {
        return 0;
    }
    rgb_data = divoom_system_malloc(SCREEN_BUF_SIZE);
    if(rgb_data == NULL)
    {
        divoom_port_file_close(save_file_handle);
        return 0;
    }
    file_buf = (uint8 *)divoom_http_client_get_file(net_addr, &real_size);
    if(file_buf == NULL)
    {
        divoom_port_file_close(save_file_handle);
        divoom_system_free(rgb_data);
        return 0;
    }
	/*************sha1测试***************/
	if(sha1_sum != NULL)
	{
		memset(sha1_buf, 0, sizeof(sha1_buf));
		memset(sha1_exchange, 0, sizeof(sha1_exchange));
		//获取到的是char?man ascii查看char那一栏，要转化成16进制的
		//sha1长度为20个字符或者40个16进制字符，64足够了
		esp_sha(SHA1, file_buf, real_size, sha1_buf);
		//Output is 20 bytes for sha_type SHA1, 32 bytes for sha_type SHA2_256, 48 bytes for sha_type SHA2_384, 64 bytes for sha_type SHA2_512.
		ret = 20;
		for(i=0;i<ret;i++)
		{
			//ret = sha1_buf[i];
			//sha1_exchange[i*2] = sha1_buf[i]/16;
			//sha1_exchange[i*2+1] = sha1_buf[i]%16;
			sprintf((char *)(sha1_exchange+i*2), "%x%x", sha1_buf[i]/16, sha1_buf[i]%16);
			//divoom_print("sha1_buf[%d] = %d---0x%02x", i, sha1_buf[i], sha1_buf[i]);
			//divoom_print("sha1_exchange[%d] = %x, sha1_exchange[%d] = %x", i*2, sha1_exchange[i*2], i*2+1, sha1_exchange[i*2+1]);
		}
		//divoom_print("sha1_exchange = %s", sha1_exchange);
		if(strcmp((char *)sha1_sum, (char *)sha1_exchange) != 0)
		{
			ESP_LOGE("SHA1_CHECK", "%s:%d-->sha1 give:%s, sha1_get:%s, old sha:%s", __func__, __LINE__, sha1_sum, sha1_exchange, sha1_buf);
			divoom_system_free(file_buf);
			divoom_port_file_close(save_file_handle);
			return 0;
		}
	}
	/*************sha1测试***************/
    memset(head_buf, 0, sizeof(head_buf));
    divoom_com_pack_uint32(head_buf,real_size);
    head_buf[4] = DIVOOM_IMAGE_LOCAL_TYPE_NORMAL;
    divoom_port_file_write(head_buf, DIVOOM_PAGE_SIZE, 1, save_file_handle); 
    divoom_old_handle = divoom_image_decode_creat_handle();
    divoom_image_decode_add_data(divoom_old_handle, file_buf, real_size, 0);
    while(1)
    {
        if(0 == divoom_image_decode_decode_pic(divoom_old_handle, rgb_data, &pic_time))
        {
            break;
        }
        encode_data = divoom_image_encode_encode_mul_pic(rgb_data, 1, &out_len, pic_time);
        divoom_port_file_write(encode_data, out_len, 1, save_file_handle); 
        encode_size += out_len;
        divoom_system_free(encode_data);
        divoom_system_delay_ms(20);
    }
    divoom_port_file_seek(save_file_handle, 0, SEEK_SET);
    divoom_com_pack_uint32(head_buf,encode_size);
    divoom_port_file_write(head_buf, DIVOOM_PAGE_SIZE, 1, save_file_handle); 
    divoom_system_free(file_buf);
    divoom_system_free(rgb_data);
    divoom_port_file_close(save_file_handle);
#endif
    return 1;
}

/**********************************************
文件储存格式
前面256字节定义为: 4字节文件大小，+1字节文件类型，+其它数据
**************************************************/
int divoom_format_srv_file_do(char *net_addr, char *local_file, uint8 iframe_flag, uint8 *sha1_sum)
{
#if 0
    int ret = 0, i = 0;
    int file_size, real_size;
	uint8 sha1_buf[64];
	uint8 sha1_exchange[64];
    uint8 *file_buf, head_buf[DIVOOM_PAGE_SIZE];
    uint8 *pic_data;
    FILE *save_file_handle;

    save_file_handle = divoom_port_file_open(local_file, "wb");
    if(save_file_handle == NULL)
    {
        divoom_print("%s: err wirte: %d, %s\n", __func__, __LINE__, local_file);
        return 0;
    }
    
    file_buf = (uint8 *)divoom_http_client_get_file(net_addr, &file_size);
    if(file_buf == NULL)
    {
        divoom_port_file_close(save_file_handle);
        divoom_print("%s: err download file: %d\n", __func__, __LINE__);
        return 0;
    }
	/*************sha1测试***************/
	if(sha1_sum != NULL)
	{
		memset(sha1_buf, 0, sizeof(sha1_buf));
		memset(sha1_exchange, 0, sizeof(sha1_exchange));
		//获取到的是char?man ascii查看char那一栏，要转化成16进制的
		//sha1长度为20个字符或者40个16进制字符，64足够了
		esp_sha(SHA1, file_buf, file_size, sha1_buf);
		//Output is 20 bytes for sha_type SHA1, 32 bytes for sha_type SHA2_256, 48 bytes for sha_type SHA2_384, 64 bytes for sha_type SHA2_512.
		ret = 20;
		for(i=0;i<ret;i++)
		{
			//ret = sha1_buf[i];
			//sha1_exchange[i*2] = sha1_buf[i]/16;
			//sha1_exchange[i*2+1] = sha1_buf[i]%16;
			sprintf((char *)(sha1_exchange+i*2), "%x%x", sha1_buf[i]/16, sha1_buf[i]%16);
			//divoom_print("sha1_buf[%d] = %d---0x%02x", i, sha1_buf[i], sha1_buf[i]);
			//divoom_print("sha1_exchange[%d] = %x, sha1_exchange[%d] = %x", i*2, sha1_exchange[i*2], i*2+1, sha1_exchange[i*2+1]);
		}
		//divoom_print("sha1_exchange = %s", sha1_exchange);
		if(strcmp((char *)sha1_sum, (char *)sha1_exchange) != 0)
		{
			ESP_LOGE("SHA1_CHECK", "%s:%d-->sha1 give:%s, sha1_get:%s, old sha:%s", __func__, __LINE__, sha1_sum, sha1_exchange, sha1_buf);
			divoom_system_free(file_buf);
			divoom_port_file_close(save_file_handle);
			return 0;
		}
	}
	/*************sha1测试***************/
    switch(file_buf[0])
    {       
           
        case DIVOOM_NET_FILE_TYPE_SINGLE_16:
        case DIVOOM_NET_FILE_TYPE_SINGLE_AES:
            pic_data = divoom_format_pic_single(file_buf, &real_size, file_size, file_buf[0] == DIVOOM_NET_FILE_TYPE_SINGLE_AES);
            divoom_port_file_seek(save_file_handle, 0, SEEK_SET);
            memset(head_buf, 0, sizeof(head_buf));
            divoom_com_pack_uint32(head_buf,real_size);
            divoom_print("DIVOOM_NET_FILE_TYPE_SINGLE_AES: %d\n", real_size);
            head_buf[4] = DIVOOM_IMAGE_LOCAL_TYPE_NORMAL;
        	divoom_port_file_write(head_buf, DIVOOM_PAGE_SIZE, 1, save_file_handle); 
        	divoom_port_file_write(pic_data, real_size, 1, save_file_handle); 
            divoom_system_free(pic_data);
            ret = real_size;
            break;
             
        case DIVOOM_NET_FILE_TYPE_GIF_16:
        case DIVOOM_NET_FILE_TYPE_GIF_AES:
            memset(head_buf, 0, sizeof(head_buf));
            pic_data = divoom_format_pic_gif(file_buf, &real_size, file_size,  file_buf[0] == DIVOOM_NET_FILE_TYPE_GIF_AES, iframe_flag);
            divoom_port_file_seek(save_file_handle, 0, SEEK_SET);
            divoom_com_pack_uint32(head_buf,real_size);
            head_buf[4] = DIVOOM_IMAGE_LOCAL_TYPE_NORMAL;
    	    divoom_port_file_write(head_buf, DIVOOM_PAGE_SIZE, 1, save_file_handle); 
            divoom_print("DIVOOM_NET_FILE_TYPE_GIF_AES: %d\n", real_size);
            for(file_size = 0; file_size < real_size; file_size += 1024)
            {
                if(file_size + 1024 < real_size)
                {
    	            divoom_port_file_write(pic_data + file_size, 1024, 1, save_file_handle); 
                }
                else
                {
    	            divoom_port_file_write(pic_data + file_size, real_size - file_size, 1, save_file_handle); 
                }
                divoom_system_delay_ms(10);
            }
            divoom_system_free(pic_data);
            ret = real_size;
            break;
        
        case DIVOOM_NET_FILE_TYPE_PIC_SCAN:
        {
            int mode, speed;

            mode = file_buf[1];
            speed = divoom_format_unpack_uint16(&file_buf[2]);
            divoom_aes_do(&file_buf[4], file_size - 4);
            ret = divoom_format_pic_scan(save_file_handle, &file_buf[4], file_size - 4, mode, speed);
            break;
        }
        
        case DIVOOM_NET_FILE_TYPE_SAND_PAINT_REVIEW:
            divoom_aes_do(&file_buf[1], file_size - 1);
            ret = divoom_format_sand_paint(save_file_handle, &file_buf[1], file_size -1);
            break;
            
        case DIVOOM_NET_FILE_TYPE_LED_DEVICE:
            memset(head_buf, 0, sizeof(head_buf));
            real_size = file_size - 769;
            divoom_com_pack_uint32(head_buf,real_size);
            head_buf[4] = DIVOOM_IMAGE_LOCAL_TYPE_NORMAL;
    	    divoom_port_file_write(head_buf, DIVOOM_PAGE_SIZE, 1, save_file_handle); 
            for(file_size = 0; file_size < real_size; file_size += 1024)
            {
                if(file_size + 1024 < real_size)
                {
    	            divoom_port_file_write(file_buf + 769 + file_size, 1024, 1, save_file_handle); 
                }
                else
                {
    	            divoom_port_file_write(file_buf + 769 + file_size, real_size - file_size, 1, save_file_handle); 
                }
                divoom_system_delay_ms(10);
            }
            ret = 1;
            break;

        case DIVOOM_NET_FILE_TYPE_GIF_SET:
            divoom_aes_do(&file_buf[1], file_size - 1);
            ret = divoom_format_gif_set(save_file_handle, &file_buf[1], file_size -1);
            break;

        case DIVOOM_NET_FILE_TYPE_NEW_MUL_SAND_PAINT:
            ret = divoom_format_sand_paint(save_file_handle, &file_buf[1], file_size -1);
            break;

		case DIVOOM_NET_FILE_TYPE_NEW_MUL_SINGLE:
            divoom_aes_do(&file_buf[7], file_size - 7);
            pic_data = divoom_format_new_mul_pic_single(&file_buf[1], &real_size);
            memset(head_buf, 0, sizeof(head_buf));
            divoom_com_pack_uint32(head_buf,real_size);
            head_buf[4] = DIVOOM_IMAGE_LOCAL_TYPE_NORMAL;
    	    divoom_port_file_write(head_buf, DIVOOM_PAGE_SIZE, 1, save_file_handle); 
			divoom_port_file_write(pic_data, 1, real_size, save_file_handle);
			divoom_free(pic_data);
			break;
		case DIVOOM_NET_FILE_TYPE_NEW_MUL_GIF:
            divoom_aes_do(&file_buf[6], file_size - 6);
            pic_data = divoom_format_new_mul_pic_gif(&file_buf[1], &real_size, file_size -1);
            memset(head_buf, 0, sizeof(head_buf));
            divoom_com_pack_uint32(head_buf,real_size);
            head_buf[4] = DIVOOM_IMAGE_LOCAL_TYPE_NORMAL;
    	    divoom_port_file_write(head_buf, DIVOOM_PAGE_SIZE, 1, save_file_handle); 
			divoom_port_file_write(pic_data, 1, real_size, save_file_handle);
			divoom_free(pic_data);
			break;

        default:
            divoom_print("divoom_format_srv_file_do: we don't do with the type: %d, %s\n", file_buf[0], net_addr);
            
    }
    divoom_system_free(file_buf);
    divoom_port_file_close(save_file_handle);
#endif
    return 0;
    
}

#if 0
/**********************************************
加载动画
**************************************************/
uint8 *divoom_format_srv_gif_file_to_buf(char *net_addr, int *real_size)
{
    uint8 *file_buf;
    int file_size;
    uint8 *pic_data = NULL;
    
    file_buf = (uint8 *)divoom_http_client_get_file(net_addr, &file_size);
    if(file_buf == NULL)
    {
        divoom_print("%s:fails to download: %s\n", __func__, net_addr);
        return NULL;
    }
    
    switch(file_buf[0])
    {       
        case DIVOOM_NET_FILE_TYPE_SINGLE_16:
        case DIVOOM_NET_FILE_TYPE_SINGLE_AES:
            pic_data = divoom_format_pic_single(file_buf, real_size, file_size, file_buf[0] == DIVOOM_NET_FILE_TYPE_SINGLE_AES);
            break;
            
        case DIVOOM_NET_FILE_TYPE_GIF_16:
        case DIVOOM_NET_FILE_TYPE_GIF_AES:
            pic_data = divoom_format_pic_gif(file_buf, real_size, file_size, file_buf[0] == DIVOOM_NET_FILE_TYPE_GIF_AES, 0);
            break;
        

        case DIVOOM_NET_FILE_TYPE_LED_DEVICE:
            *real_size = file_size - 769;
            pic_data = divoom_system_malloc(*real_size);
            memcpy(pic_data, file_buf + 769, *real_size);
            break;
           
        case DIVOOM_NET_FILE_TYPE_SAND_PAINT_REVIEW:
            divoom_aes_do(&file_buf[1], file_size - 1);
            pic_data = divoom_format_sand_paint_to_buf(real_size, &file_buf[1], file_size -1);
            break;
             
        case DIVOOM_NET_FILE_TYPE_NEW_MUL_SINGLE:
            divoom_aes_do(&file_buf[7], file_size - 7);
            pic_data = divoom_format_new_mul_pic_single(&file_buf[1], real_size);
            break;
            
        case DIVOOM_NET_FILE_TYPE_NEW_MUL_GIF:
            divoom_aes_do(&file_buf[6], file_size - 6);
            pic_data = divoom_format_new_mul_pic_gif(&file_buf[1], real_size, file_size -1);
            break;
            
        case DIVOOM_NET_FILE_TYPE_NEW_MUL_SAND_PAINT:
            pic_data = divoom_format_new_mul_sand_paint_to_buf(real_size, &file_buf[1], file_size -1);
            break;

		case DIVOOM_NET_FILE_TYPE_PIC_SCAN:
			//感觉应该给1字节的模式,2字节的速度,后面是数据
			pic_data = divoom_format_pic_scan_buf(file_buf+1, real_size, file_size-1);
			break;

		case DIVOOM_NET_FILE_TYPE_GIF_SET:
			//先把数据打包
            divoom_aes_do(file_buf+1, file_size - 1);
			pic_data = divoom_format_gif_set_buf(file_buf+1, file_size-1);
			break;
            
        default:
            *real_size = 0;
            divoom_print("%s: we don't do with the type: %d, file: %s\n", __func__, file_buf[0], net_addr);
            break;
            
    }
    divoom_system_free(file_buf);
    return pic_data;
    
}


uint8 *divoom_format_srv_draw_pic_handle(uint8 *pic_buf, int *pic_len, int pic_size, int form, int pic_time)
{
	switch (form)
	{
		case SCREEN_WIDTH_16:
			return divoom_format_new_mul_pic_single_handle(pic_buf, pic_len, SCREEN_WIDTH_16);
			
		case SCREEN_WIDTH_32:
			return divoom_format_new_mul_pic_single_handle(pic_buf, pic_len, SCREEN_WIDTH_32);
			
		case SCREEN_WIDTH_64:
			return divoom_format_new_mul_pic_single_handle(pic_buf, pic_len, SCREEN_WIDTH_64);
	}
	return NULL;
}

//把服务器的格式转化为本地格式
//本地格式前面256字节定义为: 4字节文件大小，+1字节文件类型，+其它数据
//raw_buf:服务器存储的原始全部图片数据
//pic_len:返回的本地格式的数据长度.不包括256字节了
//raw_size:原始图片数据的总长度
uint8 *divoom_format_srv_buf_to_local(uint8 *raw_buf, int *pic_len, int raw_size)
{
	if(raw_size < 7)
	{
		divoom_print("%s---Here Data Maybe Wrong!", __func__);
		return NULL;
	}
    switch(raw_buf[0])
    {       
        case DIVOOM_NET_FILE_TYPE_SINGLE_16:
        case DIVOOM_NET_FILE_TYPE_SINGLE_AES:
            return divoom_format_pic_single(raw_buf, pic_len, raw_size, raw_buf[0] == DIVOOM_NET_FILE_TYPE_SINGLE_AES);
            break;
            
        case DIVOOM_NET_FILE_TYPE_GIF_16:
        case DIVOOM_NET_FILE_TYPE_GIF_AES:
            return divoom_format_pic_gif(raw_buf, pic_len, raw_size, raw_buf[0] == DIVOOM_NET_FILE_TYPE_GIF_AES, 0);
            break;

        case DIVOOM_NET_FILE_TYPE_PIC_SCAN:
            return divoom_format_pic_scan_buf(&raw_buf[1], pic_len, raw_size);
            break;
		
        case DIVOOM_NET_FILE_TYPE_NEW_MUL_SINGLE:
            divoom_aes_do(&raw_buf[7], raw_size - 7);
            return divoom_format_new_mul_pic_single(&raw_buf[1], pic_len);
            break;
            
        case DIVOOM_NET_FILE_TYPE_NEW_MUL_GIF:
            divoom_aes_do(&raw_buf[6], raw_size - 6);
            return divoom_format_new_mul_pic_gif(&raw_buf[1], pic_len, raw_size -1);
            break;

        case DIVOOM_NET_FILE_TYPE_SAND_PAINT_REVIEW:
            divoom_aes_do(raw_buf+1, raw_size - 1);
            return divoom_format_sand_paint_to_buf(pic_len, raw_buf + 1, raw_size -1);
            break;

        case DIVOOM_NET_FILE_TYPE_LED_DEVICE:
			{
				uint8 *pic_data = NULL;
				*pic_len = raw_size- 769;
				pic_data = divoom_system_malloc(*pic_len);
				memcpy(pic_data, raw_buf + 769, *pic_len);
				return pic_data;
			}
            break;

        case DIVOOM_NET_FILE_TYPE_NEW_MUL_SAND_PAINT:
            return divoom_format_new_mul_sand_paint_to_buf(pic_len, raw_buf + 1, raw_size -1);
            break;

		case DIVOOM_NET_FILE_TYPE_GIF_SET:
			//先把数据打包
            divoom_aes_do(raw_buf+1, raw_size-1);
			return divoom_format_gif_set_buf(raw_buf+1, raw_size-1);
			break;
    }
    divoom_print("%s: it err at type: %d\n",__FUNCTION__, raw_buf[0]);
    *pic_len = 0;
    return NULL;
    
}

#endif






