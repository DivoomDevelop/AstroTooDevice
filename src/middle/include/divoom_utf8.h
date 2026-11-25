#ifndef _DIVOOM_UTF8_
#define _DIVOOM_UTF8_

int divoom_utf8_getc(const unsigned char *str, int len, unsigned long *val);

int divoom_utf8_putc(unsigned char *str, int len, unsigned long value);
#endif

