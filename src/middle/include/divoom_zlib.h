#ifndef _DIVOOM_ZLIB_H_
#define _DIVOOM_ZLIB_H_

//#define DIVOOM_ZLIB_TEST

void divoom_zlib_init(void);

#if 0
//uint8 *divoom_zlib_decompress(uint8 *zlib_data, uint32 data_len, uint8 pic_num, uint8 hight, uint8 wight);
//uint8 *divoom_zlib_decompress(uint8 *zlib_data, uint32 data_len, uint8 pic_num, uint8 hight, uint8 wight, uint8 device_num);
uint8 *divoom_zlib_decompress_pic(uint8 *zlib_data, uint32 data_len, uint8 pic_num, uint8 hight, uint8 wight, uint8 device_num, uint32 *return_data_len);

#ifdef DIVOOM_ZLIB_TEST
void divoom_zlib_compress(uint8 *data, uint32 data_len, char *file_name);

void divoom_zlib_test_compress_and_decompress(char *origin_name);
#endif
#else

int divoom_zlib_decompress(uint8 *dest, DIVOOM_UINT_ID *dest_len, uint8 *source, DIVOOM_UINT_ID source_len);

#endif

#endif






