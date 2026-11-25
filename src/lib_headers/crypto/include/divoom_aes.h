#ifndef _divoom_aes_h_
#define _divoom_aes_h_
#define DIVOOM_AES_IV_SIZE 16
void divoom_aes_init_iv(uint8 iv[DIVOOM_AES_IV_SIZE]);
int divoom_aes_do_with_iv(uint8 *src, int len, uint8 iv[DIVOOM_AES_IV_SIZE]);
int divoom_aes_do(uint8 *src, int len);

#endif


