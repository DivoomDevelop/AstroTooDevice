#ifndef _divoom_disp_font_h_
#define _divoom_disp_font_h_
void divoom_disp_font_get_word_info(uint16 *unicode_info, uint16 num, uint8 *dis_info, uint8 flag);
uint16 divoom_disp_font_get_unicode_offset(uint16 word);
void divoom_disp_font_update_font_info(uint8 *font_ctrl_info, uint8 flag);

/**
* NAME         divoom_disp_font_get_row_info
* @BRIEF	通过１６*１６字体信息得到放大后的字体
* @PARAM	font_info?字体信息16*16的字体信息
                    row             字体的行
                    row_info      字体的行的相关信息这个缓存器大小为font_size/8
                    font_size     字体的大小
* @RETURN	NONE
* @RETVAL
*/


void divoom_disp_font_get_row_info(uint16 *font_info, uint16 row, uint8 *row_info, uint8 font_size);

/**
* NAME         divoom_disp_font_get_column_info
* @BRIEF	通过１６*１６字体信息得到放大后的字体
* @PARAM	font_info?字体信息 16*16的字体信息
                    column             字体的列
                    column_info      字体的列的相关信息这个缓存器大小为font_size/8
                    font_size     字体的大小
* @RETURN	NONE
* @RETVAL
*/

void divoom_disp_font_get_column_info(uint16 *font_info, uint16 column, uint8 *column_info, uint8 font_size);

#endif


