#ifndef _DIVOOM_HUFFMAN_DECODE_
#define _DIVOOM_HUFFMAN_DECODE_

/*
协议：
字符表长度（两个字节）+ 字符表 + 二进制串长度（四个字节）+ 压缩长度 （两个字节）+ 压缩数据
*/

/**
 * *  NAME          divoom_huffman_compress
 * *  @brief       	对字符数据进行哈夫曼编码压缩，当get_len不为空则是先获取压缩的长度
 * *  @param[in] 	
 					buff_in 要编码的数据	
  					in_len 要编码的数据的长度
  					buff_out 压缩之后的数据, 内部开辟，外部用完需要释放
  					out_len 压缩之后的数据长度
  					get_len 仅仅的得到压缩的长度, 不为空就是获取压缩之后的长度，同时buff_out、out_len可以传空
 * *  @author       wangft                                   
 * *  @return       1成功 0失败
 * */
int divoom_huffman_compress(uint8 *buff_in, uint16 in_len, uint8 **buff_out, uint16 *out_len, int *get_len);

/*
协议：
字符表长度（两个字节）+ 字符表 + 二进制串长度（四个字节）+ 压缩长度 （两个字节）+ 压缩数据
*/

/**
 * *  NAME          divoom_huffman_uncompress
 * *  @brief       	对压缩后的数据进行解压
 * *  @param[in] 	
 					buff_in 要解压的数据	
  					in_len 要解压的数据长度
  					buff_out 解压之后的数据, 内部开辟，外部用完需要释放
  					out_len 解压之后的数据长度
 * *  @author       wangft                                   
 * *  @return       1成功 0失败
 * */
int divoom_huffman_uncompress(uint8 *buff_in, uint16 in_len, uint8 **buff_out, uint16 *out_len);

#endif
