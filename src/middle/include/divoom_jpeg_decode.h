#ifndef _DIVOOM_JPEG_DECODE_
#define _DIVOOM_JPEG_DECODE_

//#pragma pack(push, 2)
typedef struct _divoom_bit_map_info_header
{
	uint32 bi_size;
	uint32 bi_width;
	uint32 bi_height;
	uint16 bi_planes;
	uint16 bi_bit_count;
	uint32 bi_compression;
	uint32 bi_size_image;
	uint32 bi_xpels_per_meter;
	uint32 bi_ypels_per_meter;
	uint32 bi_clr_used;
	uint32 bi_clr_important;
}divoom_bit_map_info_header, *Pdivoom_bit_map_info_header;

typedef struct _divoom_bit_map_header
{
	uint16 bt_type;
	uint32 bt_size;
	uint16 bt_reserved1;
	uint16 bt_reserved2;
	uint32 bt_off_bits;
}divoom_bit_map_header, *Pdivoom_bit_map_header;

#if 0
//#pragma pack(push, 2)
typedef struct {
	unsigned short	bfType;
	unsigned int	bfSize;
	unsigned short	bfReserved1;
	unsigned short	bfReserved2;
	unsigned int	bfOffBits;
} BITMAPFILEHEADER;

typedef struct {
	unsigned int	biSize;
	int				biWidth;
	int				biHeight;
	unsigned short	biPlanes;
	unsigned short	biBitCount;
	unsigned int	biCompression;
	unsigned int	biSizeImage;
	int				biXPelsPerMeter;
	int				biYPelsPerMeter;
	unsigned int	biClrUsed;
	unsigned int	biClrImportant;
} BITMAPINFOHEADER;
//#pragma pack(pop)
#endif

/**
 * *  NAME          divoom_jpeg_decompress
 * *  @brief       	对输入的数据进行解压
 * *  @param[in] 	in_buff 要解码的数据 一张jpeg格式的图
 					in_len 要解码的数据长度
 					out_buff 存放解码之后的数据
 					out_len 解码之后的数据长度
 * *  @author       wangft                                   
 * *  @return       1 成功 0 失败
 * */ 
int divoom_jpeg_decompress(uint8 *in_buff, uint32 in_len, uint8 *out_buff, uint32 *out_len);

int divoom_jpeg_decompress_init(void);
void divoom_jpeg_decompress_exit(void);

void divoom_jpeg_decompress_test(void);
/**
 * *  NAME          divoom_jpeg_decode
 * *  @brief       	对输入的数据进行解码
 * *  @param[in]
 					out_buff 存储解码之后的bmp格式的数据
 					out_len 保存解压的长度
 * *  @author       wangft
 * *  @return       1 成功 0 失败
 * */
int divoom_jpeg_decode(uint8 *out_buff, uint32 *out_len);

#endif
