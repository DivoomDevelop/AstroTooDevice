#ifndef _divoom_minilzo_
#define _divoom_minilzo_

/**
 * NAME            divoom_minilzo_init
 * *  @brief       	minilzo 初始化
 * *  @param[in] 		
 * *	 @author         wangft               
 * *  @return       0 成功 
 * */

int divoom_minilzo_init(void);

/**
 * NAME            divoom_minilzo_compress
 * *  @brief       	minilzo 压缩
 * *  @param[in] 	in 要压缩的数据缓冲区 in_len 压缩数据的长度 out 压缩之后的数据缓冲区 out_len 压缩之后的数据长度 	
 * *	 @author         wangft               
 * *  @return       0 成功 
 * */

int divoom_minilzo_compress(uint8 *in, uint32 in_len, uint8 *out, uint32 *out_len);

/**
 * NAME            divoom_minilzo_decompress
 * *  @brief       	minilzo 解压
 * *  @param[in] 	out 要解压的数据缓冲区 out_len 要解压的数据长度 in 解压之后的数据缓冲区 new_len 解压之后的数据长度 		
 * *	 @author         wangft               
 * *  @return       0 成功
 * */

int divoom_minilzo_decompress(uint8 *out, uint32 out_len, uint8 *in, uint32 *new_len);

/**
 * NAME            divoom_minilzo_decompress
 * *  @brief       	minilzo 解压
 * *  @param[in] 	out 要解压的数据缓冲区 out_len 要解压的数据长度 in 解压之后的数据缓冲区 new_len 解压之后的数据长度 	lzo1x_decompress_safe函数会对要解压缩数据的有效性进行验证
 * *	 @author         wangft               
 * *  @return       0 成功
 * */

int divoom_minilzo_decompress_safe(uint8 *out, uint32 out_len, uint8 *in, uint32 *new_len);


#endif

