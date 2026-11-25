#ifndef __divoom_disp_fill_ctrl_h
#define __divoom_disp_fill_ctrl_h

typedef struct __divoom_pixel_pos
{
	struct __divoom_pixel_pos *link;
	int16 x;
	int16 y;
}divoom_pixel_pos,*Pdivoom_pixel_pos;


typedef struct __divoom_board_info
{
	int16 w;
	int16 h;
	uint8 *buf;
}divoom_board_info,*Pdivoom_board_info;


/*
 *清空整个链表
 */
void divoom_disp_fill_node_destorylist(Pdivoom_pixel_pos *rootp);
/*
 *取出头
 */
int divoom_disp_fill_node_pop_head(Pdivoom_pixel_pos *rootp, int16 *x, int16 *y);

/*
 *取出头,但不释放
 */
int divoom_disp_fill_node_pop_head_without_free(Pdivoom_pixel_pos *rootp, int16 *x, int16 *y);

Pdivoom_pixel_pos divoom_ScanLineSeedpoint(int16 x, int16 y,Pdivoom_board_info pinfo,int16 *pcount);


#if 0
/*****************************************************************************
 Prototype    : ScanLineSeedFill
 Description  : 通过对图片某点同一颜色区域的识别，在另一张图片进行该区域的填充
 Input        : int16 x                              
                int16 y                              
                Pdivoom_screen_color_info new_color  要填充的颜色，为NULL时使
													 用原图的颜色
                uint8 *raw_buf		原始图片            
                uint8 *draw_buf     要进行填充的图片
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2019/4/25
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void ScanLineSeedFill(int16 x, int16 y, Pdivoom_screen_color_info new_color, uint8 *raw_buf, uint8 *draw_buf);

/*****************************************************************************
 Prototype    : divoom_ScanLineSeedFill
 Description  : 
 Input        : int16 x                               
                int16 y                               
                Pdivoom_screen_color_info draw_color  原图（X,Y）的颜色
                uint8 *raw_buf        				  原始图片                
                uint8 *draw_buf     		          正在进行绘画的图片                  
 Output       : None
 Return Value : 									  返回查找到的点的链表
 
  History        :
  1.Date         : 2019/5/10
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
Pdivoom_pixel_pos divoom_ScanLineSeedFill(int16 x, int16 y, Pdivoom_screen_color_info draw_color, uint8 *raw_buf, uint8 *draw_buf);
#endif

#endif

