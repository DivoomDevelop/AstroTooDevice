#ifndef _divoom_light_pic_scan_h_
#define _divoom_light_pic_scan_h_
typedef struct _divoom_light_pic_scan_para
{
    uint8 flag;  //是否更新动画数据
    uint8 mode;
    uint8 type;
    uint16 speed;
    uint32 file_index;
}divoom_light_pic_scan_para, *Pdivoom_light_pic_scan_para;

enum
{
    DIVOOM_LIGHT_PIC_SCAN_MODE_1_4 = 1,
    DIVOOM_LIGHT_PIC_SCAN_MODE_4_1,
    DIVOOM_LIGHT_PIC_SCAN_MODE_2_2,
};
void divoom_light_pic_scan_set_callback(DIVOOM_DISP_CTRL_OVER_CALL_END *pcallback);

void divoom_light_pic_scan_update(Pdivoom_light_pic_scan_para para);
void divoom_light_pic_scan_init(Pdivoom_light_pic_scan_para para);
void divoom_light_pic_scan_close(void);
void divoom_light_pic_scan_play(uint8 play);
void divoom_light_pic_scan_convert(uint8 type,uint32 file_index, Pdivoom_light_pic_scan_para para_info);

#endif


