#ifndef _DIVOOM_BITMAP_H_
#define _DIVOOM_BITMAP_H_
//默认使用char数组,禁止变更
//init函数,初始化bitmap,传入bitmap数组,大小,进行置位,如果传入的为数组为空,那么返回在这里申请的bitmap首地址,后面的函数都用这个地址

//默认是char数组,所以一般来说,char占一位,size大小也是数组个数,也是数组大小
char *divoom_bitmap_init(char bitmap_in[], uint16 size);

//置位函数,传入比bitmap数组,大小,和需要职位的bit
//return 1 ok, 0 fail
int divoom_bitmap_set_bit(char bitmap_in[], uint16 size, uint16 set_bit);

//获取bitmap的位,传入bitmap数组,大小和要获取的位
//0xFF fail, 成功为1或者0
uint8 divoom_bitmap_get_bit(char bitmap_in[], uint16 size, uint16 get_bit);
#endif


