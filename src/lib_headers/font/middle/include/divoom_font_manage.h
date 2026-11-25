#ifndef _divoom_font_manage_h_
#define _divoom_font_manage_h_

enum{
	DIOVOM_FONT_MANAGE_TYPE_MATRIX = 0,
	DIVOOM_FONT_MANAGE_TYPE_IMAGE = 1,
	DIVOOM_FONT_MANAGE_TYPE_PIC = 3,
};

#define DIVOOM_FONT_MANAGE_CHARSET_NUM	100

typedef struct _divoom_font_manage_font_type_info
{
    uint8 w;          //字体宽度
    uint8 h;          //字体高度
    uint8 font_type;  //字体类型1:图像字体;0:为点阵字体
    int font_attr;    //字体种类  　id
	char charset[DIVOOM_FONT_MANAGE_CHARSET_NUM];	//图像字体字符集
    int font_id;      //字体框  　id
	divoom_screen_color_info font_color; //字体颜色
	divoom_screen_color_info shade_color; //阴影颜色
}divoom_font_manage_font_type_info, *Pdivoom_font_manage_font_type_info;

/*********************************************************************
  Function:        divoom_font_manage_init
  Description:     将所有使用的字体列表传来进行初始化，如果是图像字体，将字体所有的信息全部缓存，如果是点阵字体，先缓冲数字与字母,
                   并打开ＳＤ文件，不关闭，到最后退出时才关闭
                   图像字体的顺序:0~9,'-','/',':','%','c'(摄氏度),'f'(华氏度),'k','m',A~Z
  Input:            pfont_list  所有字体的列表
                    num         字体数目
                    font_path   字体路径，点阵字体的ＳＤ文件路径为font_path_font_attr;("%s_%s",font_path,font_attr)
  Return:           NONE
**********************************************************************/

void divoom_font_manage_init(Pdivoom_font_manage_font_type_info pfont_list, int num, char *font_path);
/*********************************************************************
  Function:        divoom_font_manage_get_font_data
  Description:     得到字体相关信息，如果是图像字体的话，将使用divoom_manage_image_read(DIVOOM_MANAGE_IMAGE_ID_BASE_TIME_DIAL_FONT + font_attr)得到图像数据
                   当访问到图像数据库的对应区域内的点颜色为白色(0xFFFFFF)填充１到缓冲区，为红色  填充２到缓冲区，否则为0,
                   查找文字对应的字体将使用文字框id相等，pword_info->font_id＝＝pfont_list->font_id；
                   如果要访问的字不在缓冲中，将增加到缓冲区中，此缓冲区只增加不减少
  Input:           pword_info
  Return:           NONE
**********************************************************************/

void divoom_font_manage_get_font_data(Pdivoom_image_word_font_info pword_info);

/*********************************************************************
  Function:        divoom_font_manage_update_font_code
  Description:     通知将使用哪些字体,此模块，将先缓存下面所使用的字体
  Input:           font_id          文字框ＩＤ
                   unicode_list     字的ＵＮＩＣＯＤＥ列表
                   num              个数
  Return:           NONE
**********************************************************************/

void divoom_font_manage_update_font_code(int font_id,uint16 *unicode_list, uint16 num);



/*********************************************************************
  Function:        divoom_font_manage_exit
  Description:     退出模块，释放所有使用的内存
  Input:           none
  Return:           NONE
**********************************************************************/

void divoom_font_manage_exit(void);


/*****************************************************************************
 Prototype    : divoom_font_manage_clear_font_id_list
 Description  : 清空字框队列
 Input        : void  
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2021/3/13
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_font_manage_clear_font_id_list(void);
/*****************************************************************************
 Prototype    : divoom_font_manage_update_font_id_list
 Description  : 增加字框字体列表
 Input        : Pdivoom_font_manage_font_type_info pfont_list  
                int num                                        
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2021/3/4
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_font_manage_update_font_id_list(Pdivoom_font_manage_font_type_info pfont_list, int num);

/*****************************************************************************
 Prototype    : divoom_font_manage_update_font_attr_list
 Description  : 更新字体列表
 Input        : Pdivoom_font_manage_font_type_info pfont_list  
                int num                                        
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2021/3/13
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_font_manage_update_font_attr_list(Pdivoom_font_manage_font_type_info pfont_list, int num);


/*****************************************************************************
 Prototype    : divoom_font_image_check_local_font_image
 Description  : 检查字体文件是否本地
 Input        : font_id                            
 Output       : None
 Return Value : 
 
*****************************************************************************/
int divoom_font_image_check_local_font_image(int font_id);

/*****************************************************************************
 Prototype    : divoom_font_image_get_font_image_id
 Description  : 得到字体文件的ＩＤ
 Input        : font_id                            
 Output       : None
 Return Value : 
 
*****************************************************************************/
int divoom_font_image_get_font_image_id(int font_id);
/*****************************************************************************
 Prototype    : divoom_font_manage_remove_font_id
 Description  : 删除变更的字体列表
 Input        : font_id pfont_list  
                                                    
 Output       : None
 Return Value : 
 
*****************************************************************************/
void divoom_font_manage_remove_font_id(int font_id);

/*****************************************************************************
 Prototype    : divoom_font_manage_clear_font_cache
 Description  : 清空字框队列
 Input        : void  
 Output       : None
 Return Value : 
 
*****************************************************************************/
void divoom_font_manage_clear_font_cache(void);

#endif

