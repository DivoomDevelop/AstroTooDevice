#ifndef _divoom_com_h_
#define _divoom_com_h_


void  divoom_com_pack_uint32(uint8 *data, uint32 value);
uint32  divoom_com_unpack_uint32(uint8 *data);
void  divoom_com_pack_uint16(uint8 *data, uint16 value);
uint16  divoom_com_unpack_uint16(uint8 *data);
int divoom_com_utf8_to_unicode(char* ppOutput,char* pInput, int max);
void divoom_com_print_buf(uint8 *data, uint16 len);
void divoom_com_print_buf_rgb(uint8 *data, uint16 len);
void divoom_com_str_to_low(char *data);

#endif


