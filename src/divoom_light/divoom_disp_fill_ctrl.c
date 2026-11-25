#include "divoom_disp_com.h"
#include "divoom_disp_fill_ctrl.h"



//Pdivoom_pixel_pos pdivoom_pixel_pos_handle = NULL;

/*
 *插入到一个有序的链表,头部插入
 */
bool divoom_disp_fill_node_insert(Pdivoom_pixel_pos *rootp, int16 x, int16 y)
{
    Pdivoom_pixel_pos cur = NULL,new_node = NULL;
    cur = *rootp;
    new_node = (Pdivoom_pixel_pos)divoom_com_malloc(sizeof(divoom_pixel_pos), __func__, __LINE__);
    if(NULL == new_node)
    {
        divoom_print("%s:malloc_fail\n",__func__);
        return false;
    }
    new_node->x = x;
    new_node->y = y;
    new_node->link = cur;
    *rootp = new_node;
    return true;
}


/*
 *插入到一个有序的链表,尾部插入
 */
bool divoom_disp_fill_node_append_tail(Pdivoom_pixel_pos *rootp, int16 x, int16 y)
{
    Pdivoom_pixel_pos new_node = NULL,tail = *rootp;
    new_node = (Pdivoom_pixel_pos)divoom_com_malloc(sizeof(divoom_pixel_pos), __func__, __LINE__);
    if(NULL == new_node)
    {
        divoom_print("%s:malloc_fail\n",__func__);
        return false;
    }
    new_node->x = x;
    new_node->y = y;
    new_node->link = NULL;


	//当头节点为空时的处理
	if (NULL == *rootp)
	{
		*rootp = new_node;
		return true;
	}

	//遍历链表,直到最尾部为止
	while (tail->link != NULL)
		tail = tail->link;
    tail->link = new_node;
    return true;
}



/*
 *清空整个链表
 */
void divoom_disp_fill_node_destorylist(Pdivoom_pixel_pos *rootp)
{
	Pdivoom_pixel_pos cur = *rootp;
	while(NULL != cur)
	{
		Pdivoom_pixel_pos tmp = cur;
		cur = cur->link;
		divoom_com_free(tmp, __func__, __LINE__);
	}
	*rootp = NULL;
}
/*
 *取出头
 */

int divoom_disp_fill_node_pop_head(Pdivoom_pixel_pos *rootp, int16 *x, int16 *y)
{
    Pdivoom_pixel_pos cur = *rootp;
	if(NULL != cur)
	{
		*x = cur->x;
		*y = cur->y;
		*rootp = cur->link;
		divoom_com_free(cur, __func__, __LINE__);
		return 1;
	}else
	{
		return 0;
	}
}

/*
 *取出头,但不释放
 */
int divoom_disp_fill_node_pop_head_without_free(Pdivoom_pixel_pos *rootp, int16 *x, int16 *y)
{
    Pdivoom_pixel_pos cur = *rootp;
	if(NULL != cur)
	{
		*x = cur->x;
		*y = cur->y;
		*rootp = cur->link;
//		Fwl_Free(cur);
		return 1;
	}else
	{
		return 0;
	}
}




int IsPixelValid_Point(int16 x, int16 y, uint8 value, Pdivoom_board_info pinfo, uint8 *disp_buf) 
{
	uint16 off = y*pinfo->w+x;
	if(x < 0 || y < 0 || x >= pinfo->w || y >= pinfo->h)
		return 0;
	if(0 != disp_buf[off])	//这个点被画了
		return 0;
	if(pinfo->buf[off] == value)
		return 1;
	else
		return 0;
}



int16 divoom_FillLineRight_point(int16 x, int16 y, uint8 value, Pdivoom_pixel_pos *rootp, Pdivoom_board_info pinfo,uint8 *disp_buf)
{
	int ret;
	int16 count = 0,off = y*pinfo->w+x;
	while(1)
	{
		ret = IsPixelValid_Point(x,y,value,pinfo,disp_buf);
		if(0 == ret)
		{
			break;
		}
		disp_buf[off] = 1;	//标记该点要填充	
		divoom_disp_fill_node_append_tail(rootp,x,y);
		count++;
		off++;
		x++;
	}
	return count;
}

int16 divoom_FillLineLeft_point(int16 x, int16 y, uint8 value, Pdivoom_pixel_pos *rootp,  Pdivoom_board_info pinfo,uint8 *disp_buf)
{
	int ret;
	int16 count = 0,off = y*pinfo->w+x;
	while(1)
	{
		ret = IsPixelValid_Point(x,y,value,pinfo,disp_buf);
		if(0 == ret)
		{
			break;
		}
		disp_buf[off] = 1;	//标记该点要填充	
		divoom_disp_fill_node_append_tail(rootp,x,y);
		count++;
		off--;
		x--;
	}
	return count;
}


void divoom_SearchLineNewSeed_point(Pdivoom_pixel_pos *rootp, int16 xLeft, int16 xRight,
                        int16 y, uint8 value,
						Pdivoom_board_info pinfo, uint8 *disp_buf)
{
 	int16 xt = xLeft + 1, e = xRight + 1;
    for(; xt<e; xt++)
	{
		if(0 == IsPixelValid_Point(xt, y, value, pinfo, disp_buf))
		{
			if(IsPixelValid_Point(xt-1, y, value, pinfo, disp_buf))
				divoom_disp_fill_node_append_tail(rootp,xt-1,y);
		}
	}
	if(IsPixelValid_Point(xt-1, y, value, pinfo, disp_buf))
		divoom_disp_fill_node_append_tail(rootp,xt-1,y);
}


Pdivoom_pixel_pos divoom_ScanLineSeedpoint(int16 x, int16 y,Pdivoom_board_info pinfo,int16 *pcount)
{
	int16 count,xRight,xLeft,total = 0;
	Pdivoom_pixel_pos pix_pos_handle = NULL,track_handle = NULL;
	uint8 *cmp_buf = NULL;	//记录被画过的点
	uint8 value = pinfo->buf[pinfo->w*y+x]; //检测点的值
	if(value == 0)
		return NULL;

	cmp_buf = divoom_com_malloc(pinfo->w*pinfo->h, __func__, __LINE__);
	if(NULL == cmp_buf)
	{
		divoom_print("%s,malloc_fail!\n",__func__);
		return NULL;
	}
	memset(cmp_buf, 0, pinfo->w*pinfo->h);
	divoom_disp_fill_node_insert(&pix_pos_handle,x,y);//第1步，种子点入栈
    while(NULL != pix_pos_handle)
    {
		divoom_disp_fill_node_pop_head(&pix_pos_handle,&x,&y); //第2步，取当前种子点
		//第3步，向左右填充
		count = divoom_FillLineRight_point(x, y,value, &track_handle,pinfo,cmp_buf);//向右填满
		total+=count;

		xRight = x + count - 1;
		count = divoom_FillLineLeft_point(x - 1, y,value, &track_handle,pinfo,cmp_buf);//向左填满
		xLeft = x - count;
		total+=count;
//		divoom_print("L:%d,R:%d\n",xLeft,xRight);
		//第4步，处理相邻两条扫描线
		divoom_SearchLineNewSeed_point(&pix_pos_handle, xLeft, xRight, y - 1, value,pinfo,cmp_buf);
		divoom_SearchLineNewSeed_point(&pix_pos_handle, xLeft, xRight, y + 1, value,pinfo,cmp_buf);
    }
	if(pcount)
	{
		*pcount = total;
	}
//	divoom_print("%s,total:%d\n",__func__,total);
//释放资源
	divoom_com_free(cmp_buf, __func__, __LINE__);
	divoom_disp_fill_node_destorylist(&pix_pos_handle);
	return track_handle;
}





#if 0
/* 扫描线种子生成算法 */
int SetRP(int x,int y,COLORREF color,COLORREF mColor,CDrawDC *pDC){
    while(pDC->GetPixel(CPoint(x,y))== mColor){
        pDC->SetPixel(ROUND(x),ROUND(y),color);
        x++;
    }
    return x-1;
}
int SetLP(int x,int y,COLORREF color,COLORREF mColor,CDrawDC *pDC){
    while(pDC->GetPixel(CPoint(x-1,y))==mColor){
        pDC->SetPixel(ROUND(--x),ROUND(y),color);
    }
    return x;
}
void NewLineSeed(std::stack<CPoint> *stk,int lx,int rx,int y,COLORREF color,COLORREF mColor,CDrawDC *pDC){
    for(int x=lx+1,e=rx+1;x<e;x++){
        if(pDC->GetPixel(CPoint(x,y))!=mColor){
            if(pDC->GetPixel(CPoint(x-1,y))==mColor)
                stk->push(CPoint(x-1,y));
        }
    }
    if(pDC->GetPixel(CPoint(x-1,y))==mColor)
        stk->push(CPoint(x-1,y));
}
void CDrawDC::ScanLineSeedFill(int x, int y, COLORREF color)
{
    int pRight,pLeft;
    std::stack<CPoint> stk;
    mColor = GetPixel(CPoint(x,y));

    stk.push(CPoint(x,y));
    while(!stk.empty()){    
        CPoint p=stk.top();
        stk.pop();

        pRight = SetRP(p.x,p.y,color,mColor,this);
        pLeft = SetLP(p.x,p.y,color,mColor,this);

        NewLineSeed(&stk,pLeft,pRight,p.y+1,color,mColor,this);
        NewLineSeed(&stk,pLeft,pRight,p.y-1,color,mColor,this);
    }
}

#endif

#if 0
int IsPixelValid(int16 x, int16 y, Pdivoom_screen_color_info pcolor, uint8 *raw_buf, uint8 *disp_buf) 
{
	uint16 off;
	if(x < 0 || y < 0 || x >= SCREEN_WIDTH_32 || y >= SCREEN_HEIGHT_32)
		return 0;
	off = (y * SCREEN_WIDTH_32 + x);
	if(0 != disp_buf[off])	//这个点被画了
		return 0;
	off = (off << 1) + off;
	if(0 == memcmp((Pdivoom_screen_color_info)(raw_buf+off),pcolor,sizeof(divoom_screen_color_info)))
		return 1;
	else
		return 0;
}

int16 FillLineRight(int16 x,int16 y, Pdivoom_screen_color_info pbasecolor, Pdivoom_screen_color_info pcolor, uint8 *raw_buf, uint8 *disp_buf)
{
	int ret;
	int16 count = 0;
	while(1)
	{
		ret = IsPixelValid(x,y,pbasecolor,raw_buf,disp_buf);
		if(0 == ret)
		{
			break;
		}
//		divoom_disp_ctrl_set_data_xy(x, y, pcolor, disp_buf);
		disp_buf[y * SCREEN_WIDTH_32 + x] = 1;			//标记该点要填充
		count++;
		x++;
	}
	return count;
}

int16 FillLineLeft(int16 x,int16 y, Pdivoom_screen_color_info pbasecolor, Pdivoom_screen_color_info pcolor, uint8 *raw_buf, uint8 *disp_buf)
{
	int ret;
	int16 count = 0;
	while(1)
	{
		ret = IsPixelValid(x,y,pbasecolor,raw_buf,disp_buf);
		if(0 == ret)
		{
			break;
		}
		disp_buf[y * SCREEN_WIDTH_32 + x] = 1;			//标记该点要填充
		count++;
		x--;
	}
	return count;
}

void SearchLineNewSeed(Pdivoom_pixel_pos *rootp, int16 xLeft, int16 xRight,
                        int16 y, Pdivoom_screen_color_info pcolor,
						uint8 *raw_buf, uint8 *disp_buf)
{
 	int16 xt = xLeft + 1, e = xRight + 1;
    for(; xt<e; xt++)
	{
		if(0 == IsPixelValid(xt, y, pcolor, raw_buf, disp_buf))
		{
			if(IsPixelValid(xt-1, y, pcolor, raw_buf, disp_buf))
				divoom_disp_fill_node_insert(rootp,xt-1,y);
		}
	}
	if(IsPixelValid(xt-1, y, pcolor, raw_buf, disp_buf))
		divoom_disp_fill_node_insert(rootp,xt-1,y);
}

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
void ScanLineSeedFill(int16 x, int16 y, Pdivoom_screen_color_info new_color, uint8 *raw_buf, uint8 *draw_buf)
{
	int16 count,xRight,xLeft;
	divoom_screen_color_info color;
	Pdivoom_pixel_pos pix_pos_handle = NULL;
	uint8 *cmp_buf = NULL;	//记录被画过的点

	divoom_disp_ctrl_get_dot(x,y,&color,raw_buf);	//获取该点颜色
	if(NULL == new_color)
		new_color = &color;

	if(0 == color.r_c && 0 == color.g_c && 0 == color.b_c)	//黑色的就不处理了
		return;
	cmp_buf = divoom_com_malloc(SCREEN_DOT_NUM, __func__, __LINE__);
	if(NULL == cmp_buf)
	{
		divoom_print("%s,malloc_fail!\n",__func__);
		return;
	}
	memset(cmp_buf, 0, SCREEN_DOT_NUM);
	divoom_disp_fill_node_insert(&pix_pos_handle,x,y);//第1步，种子点入站
    while(NULL != pix_pos_handle)
    {
		divoom_disp_fill_node_pop_head(&pix_pos_handle,&x,&y); //第2步，取当前种子点
		//第3步，向左右填充
		count = FillLineRight(x, y, &color, new_color,raw_buf,cmp_buf);//向右填满
		xRight = x + count - 1;
		count = FillLineLeft(x - 1, y, &color, new_color,raw_buf,cmp_buf);//向左填满
		xLeft = x - count;
//		divoom_print("L:%d,R:%d\n",xLeft,xRight);
		//第4步，处理相邻两条扫描线
		SearchLineNewSeed(&pix_pos_handle, xLeft, xRight, y - 1, &color,raw_buf,cmp_buf);
		SearchLineNewSeed(&pix_pos_handle, xLeft, xRight, y + 1, &color,raw_buf,cmp_buf);
    }

//对图片进行颜色的填充
	for(count = 0; count < SCREEN_DOT_NUM; count++)
	{
		if(cmp_buf[count])
		{
			divoom_disp_ctrl_set_data(count,&color,draw_buf);
		}
	}
//释放资源
	divoom_com_free(cmp_buf, __func__, __LINE__);
	divoom_disp_fill_node_destorylist(&pix_pos_handle);
}




int16 divoom_FillLineRight(int16 x,int16 y, Pdivoom_screen_color_info *pbasecolor, Pdivoom_pixel_pos *rootp, uint8 *raw_buf, uint8 *disp_buf)
{
	int ret;
	int16 count = 0;
	while(1)
	{
		ret = IsPixelValid(x,y,pbasecolor,raw_buf,disp_buf);
		if(0 == ret)
		{
			break;
		}
		disp_buf[y * SCREEN_WIDTH_32 + x] = 1;			//标记该点要填充
		divoom_disp_fill_node_append_tail(rootp,x,y);
		count++;
		x++;
	}
	return count;
}

int16 divoom_FillLineLeft(int16 x,int16 y, Pdivoom_screen_color_info *pbasecolor, Pdivoom_pixel_pos *rootp, uint8 *raw_buf, uint8 *disp_buf)
{
	int ret;
	int16 count = 0;
	while(1)
	{
		ret = IsPixelValid(x,y,pbasecolor,raw_buf,disp_buf);
		if(0 == ret)
		{
			break;
		}
		disp_buf[y * SCREEN_WIDTH_32 + x] = 1;			//标记该点要填充
		divoom_disp_fill_node_append_tail(rootp,x,y);
		count++;
		x--;
	}
	return count;
}

void divoom_SearchLineNewSeed(Pdivoom_pixel_pos *rootp, int16 xLeft, int16 xRight,
                        int16 y, Pdivoom_screen_color_info pcolor,
						uint8 *raw_buf, uint8 *disp_buf)
{
 	int16 xt = xLeft + 1, e = xRight + 1;
    for(; xt<e; xt++)
	{
		if(0 == IsPixelValid(xt, y, pcolor, raw_buf, disp_buf))
		{
			if(IsPixelValid(xt-1, y, pcolor, raw_buf, disp_buf))
				divoom_disp_fill_node_append_tail(rootp,xt-1,y);
		}
	}
	if(IsPixelValid(xt-1, y, pcolor, raw_buf, disp_buf))
		divoom_disp_fill_node_append_tail(rootp,xt-1,y);
}


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
Pdivoom_pixel_pos divoom_ScanLineSeedFill(int16 x, int16 y, Pdivoom_screen_color_info draw_color, uint8 *raw_buf, uint8 *draw_buf)
{
	int16 count,xRight,xLeft;
	divoom_screen_color_info color,color1;
	Pdivoom_pixel_pos pix_pos_handle = NULL,track_handle = NULL;
	uint8 *cmp_buf = NULL;	//记录被画过的点

	divoom_disp_ctrl_get_dot(x,y,&color,raw_buf);	//获取原图该点颜色
	memcpy(draw_color,&color,sizeof(divoom_screen_color_info));
//	if(0 == color.r_c && 0 == color.g_c && 0 == color.b_c)	//黑色的就不处理了
//		return NULL;


	//对坐标点所在画布与原图的颜色对比，如果相同直接退出
	divoom_disp_ctrl_get_dot(x,y,&color1,draw_buf);	//获取画布上该点颜色
	if(0 == memcmp(&color,&color1,sizeof(divoom_screen_color_info)))
	{
		return NULL;
	}

	cmp_buf = divoom_com_malloc(SCREEN_DOT_NUM, __func__, __LINE__);
	if(NULL == cmp_buf)
	{
		divoom_print("%s,malloc_fail!\n",__func__);
		return NULL;
	}
	memset(cmp_buf, 0, SCREEN_DOT_NUM);
	divoom_disp_fill_node_insert(&pix_pos_handle,x,y);//第1步，种子点入站
    while(NULL != pix_pos_handle)
    {
		divoom_disp_fill_node_pop_head(&pix_pos_handle,&x,&y); //第2步，取当前种子点
		//第3步，向左右填充
		count = divoom_FillLineRight(x, y, &color, &track_handle,raw_buf,cmp_buf);//向右填满
		xRight = x + count - 1;
		count = divoom_FillLineLeft(x - 1, y, &color, &track_handle,raw_buf,cmp_buf);//向左填满
		xLeft = x - count;
//		divoom_print("L:%d,R:%d\n",xLeft,xRight);
		//第4步，处理相邻两条扫描线
		divoom_SearchLineNewSeed(&pix_pos_handle, xLeft, xRight, y - 1, &color,raw_buf,cmp_buf);
		divoom_SearchLineNewSeed(&pix_pos_handle, xLeft, xRight, y + 1, &color,raw_buf,cmp_buf);
    }

//释放资源
	divoom_com_free(cmp_buf, __func__, __LINE__);
	divoom_disp_fill_node_destorylist(&pix_pos_handle);
	return track_handle;
}
#endif




