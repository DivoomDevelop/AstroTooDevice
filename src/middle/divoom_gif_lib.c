#include "divoom.h"
#include "gif_lib.h"
#include "divoom_pixel_format.h"

typedef struct _divoom_gif_lib_ctrl
{
    uint8 file_type;
    uint8 all_cache_flag;
    uint32 file_off;
    int ImageNum;
    GifFileType *gif_handle;
    FILE *file_handle;
    uint8 *bgra_buf;
    uint8 *ScreenBuffer;
    /* RGB565 + Alpha 平面跨帧保留真实颜色，不能保留局部调色板色号。 */
    uint16 *rgb565_canvas;
    /* 仅 disposal=3 时申请，用于恢复绘制本帧之前的画布。 */
    uint8 *restore_canvas;
    GifPixelType *transparent_line;
    uint16 *rgb565_map;
    uint8 *rgb565_map_ready;
    ColorMapObject *rgb565_color_map;
    uint32 rgb565_color_hash;
    int rgb565_color_count;
    int screen_width;
    int screen_height;
    char file_name[256];
}divoom_gif_lib_ctrl, *Pdivoom_gif_lib_ctrl;

/**
 * NAME         gif_malloc
 * @BRIEF	    申请内存
                
 * @PARAM	    size : 
 
 * @RETURN	    ptr
 * @RETVAL	
 */

void *gif_malloc(int size)
{
    return divoom_system_malloc(size);
}

/**
 * NAME         gif_free
 * @BRIEF	    释放内存
                
 * @PARAM	    ptr
 
 * @RETURN	    
 * @RETVAL	
 */

void gif_free(void *ptr)
{
    divoom_system_free(ptr);
}

/**
 * NAME         divoom_gif_lib_init
 * @BRIEF	    打开一个ＧＩＦ文件句柄
                
 * @PARAM	    file_name : name
                file_handle
                ext_buf_flag   1:使用外部的BUF;0：内部申请BUF
 
 * @RETURN	    gif handle
 * @RETVAL	
 */

void *divoom_gif_lib_init(char *file_name, void * file_handle, uint8 ext_buf_flag)
{
    Pdivoom_gif_lib_ctrl pdivoom_gif_handle;
	int	i, Size,file_type = 0, file_off;
	GifFileType *GifFile;
	int Error;
    if(file_handle == NULL)
    {
    	if ((GifFile = DGifOpenFileName(file_name, &Error)) == NULL) {
            divoom_app_com_del_file(file_name);
            divoom_print("it fails to open gif file: %s, err:%d\n", file_name, Error);
            return NULL;
    	}
        file_off = 0;
        file_type = 1;
    }
    else
    {
        file_off = divoom_port_file_tell(file_handle);
    	if ((GifFile = DGifOpenFileHandle(file_handle, &Error)) == NULL) {
            divoom_print("it fails to open gif file: %d, %d\n", Error, file_off);
            return NULL;
    	}
    }
    if(GifFile->SWidth > 480 || GifFile->SHeight > 480)
    {
        divoom_app_com_del_file(file_name);
        DGifCloseFile(GifFile, &Error, file_type);
        return NULL;        
    }

    pdivoom_gif_handle = divoom_system_malloc(sizeof(divoom_gif_lib_ctrl));
    if(pdivoom_gif_handle == NULL)
    {
        DGifCloseFile(GifFile, &Error, file_type);
        return NULL;        
    }
    memset(pdivoom_gif_handle, 0, sizeof(divoom_gif_lib_ctrl));
    pdivoom_gif_handle->file_type = file_type;
    if(file_type == 1)
    {
        strcpy(pdivoom_gif_handle->file_name, file_name);
    }
    else
    {
        pdivoom_gif_handle->file_handle = file_handle;
    }
    pdivoom_gif_handle->ScreenBuffer = divoom_system_malloc_out(GifFile->SHeight * GifFile->SWidth);
    pdivoom_gif_handle->rgb565_canvas = divoom_system_malloc_out(
        GifFile->SHeight * GifFile->SWidth * 3U);
    pdivoom_gif_handle->transparent_line = divoom_system_malloc_out(GifFile->SWidth * sizeof(GifPixelType));
    pdivoom_gif_handle->rgb565_map = divoom_system_malloc_out(256 * sizeof(uint16));
    pdivoom_gif_handle->rgb565_map_ready = divoom_system_malloc_out(256);
    if(pdivoom_gif_handle->ScreenBuffer == NULL ||
        pdivoom_gif_handle->rgb565_canvas == NULL ||
        pdivoom_gif_handle->transparent_line == NULL ||
        pdivoom_gif_handle->rgb565_map == NULL ||
        pdivoom_gif_handle->rgb565_map_ready == NULL)
    {
        DGifCloseFile(GifFile, &Error, file_type);
        if(pdivoom_gif_handle->ScreenBuffer)
        {
            divoom_system_free(pdivoom_gif_handle->ScreenBuffer);
        }
        if(pdivoom_gif_handle->rgb565_canvas)
        {
            divoom_system_free(pdivoom_gif_handle->rgb565_canvas);
        }
        if(pdivoom_gif_handle->transparent_line)
        {
            divoom_system_free(pdivoom_gif_handle->transparent_line);
        }
        if(pdivoom_gif_handle->rgb565_map)
        {
            divoom_system_free(pdivoom_gif_handle->rgb565_map);
        }
        if(pdivoom_gif_handle->rgb565_map_ready)
        {
            divoom_system_free(pdivoom_gif_handle->rgb565_map_ready);
        }
        divoom_system_free(pdivoom_gif_handle);
        return NULL;
    }
    pdivoom_gif_handle->gif_handle = GifFile;
    pdivoom_gif_handle->file_off = file_off;
    pdivoom_gif_handle->screen_width = GifFile->SWidth;
    pdivoom_gif_handle->screen_height = GifFile->SHeight;
	/*
	* Allocate the screen as vector of column of rows. Note this
	* screen is device independent - it's the screen defined by the
	* GIF file parameters.
	*/
	Size = GifFile->SWidth * sizeof(GifPixelType);/* Size in bytes one row.*/

	for (i = 0; i < GifFile->SWidth; i++)  /* Set its color to BackGround. */
		pdivoom_gif_handle->ScreenBuffer[i] = GifFile->SBackGroundColor;
	for (i = 1; i < GifFile->SHeight; i++) {

		memcpy(&pdivoom_gif_handle->ScreenBuffer[i * GifFile->SWidth], pdivoom_gif_handle->ScreenBuffer, Size);
	}

    return pdivoom_gif_handle;
}

/**
 * NAME         divoom_gif_lib_convert_to_rgb_buf
 * @BRIEF	    获取当前帧的ＲＧＢ缓冲区
                
 * @PARAM	    GifFile : 
                ScreenBuffer
                rgb_buf
 
 * @RETURN	    none
 * @RETVAL	
 */

static uint32 divoom_gif_lib_get_color_map_hash(ColorMapObject *ColorMap)
{
    uint32 hash = 2166136261U;
    int i;

    if(ColorMap == NULL)
    {
        return 0;
    }
    hash ^= (uint32)ColorMap->ColorCount;
    hash *= 16777619U;
    for(i = 0; i < ColorMap->ColorCount && i < 256; i++)
    {
        hash ^= ColorMap->Colors[i].Red;
        hash *= 16777619U;
        hash ^= ColorMap->Colors[i].Green;
        hash *= 16777619U;
        hash ^= ColorMap->Colors[i].Blue;
        hash *= 16777619U;
    }
    return hash;
}

static void divoom_gif_lib_prepare_rgb565_map(Pdivoom_gif_lib_ctrl pdivoom_gif_handle,
    ColorMapObject *ColorMap)
{
    uint32 color_hash;

    if(pdivoom_gif_handle == NULL || ColorMap == NULL)
    {
        return;
    }
    color_hash = divoom_gif_lib_get_color_map_hash(ColorMap);
    if(pdivoom_gif_handle->rgb565_color_map == ColorMap &&
        pdivoom_gif_handle->rgb565_color_count == ColorMap->ColorCount &&
        pdivoom_gif_handle->rgb565_color_hash == color_hash)
    {
        return;
    }

    /*
     * GIF 常见情况是全局调色板不变。调色板变化时只清 ready 标记，
     * 后续遇到具体颜色 index 再生成 RGB565，避免每帧全量重算 256 色。
     */
    memset(pdivoom_gif_handle->rgb565_map_ready, 0, 256);
    pdivoom_gif_handle->rgb565_color_map = ColorMap;
    pdivoom_gif_handle->rgb565_color_count = ColorMap->ColorCount;
    pdivoom_gif_handle->rgb565_color_hash = color_hash;
}

static uint16 divoom_gif_lib_get_rgb565_color(Pdivoom_gif_lib_ctrl pdivoom_gif_handle,
    ColorMapObject *ColorMap, int index)
{
    if(index < 0 || index >= ColorMap->ColorCount || index >= 256)
    {
        index = 0;
    }
    if(pdivoom_gif_handle->rgb565_map_ready[index] == 0)
    {
        pdivoom_gif_handle->rgb565_map[index] = divoom_rgb888_to_rgb565(
            ColorMap->Colors[index].Red,
            ColorMap->Colors[index].Green,
            ColorMap->Colors[index].Blue);
        pdivoom_gif_handle->rgb565_map_ready[index] = 1;
    }
    return pdivoom_gif_handle->rgb565_map[index];
}

void divoom_gif_lib_convert_to_rgb_buf(Pdivoom_gif_lib_ctrl pdivoom_gif_handle, uint8 *rgb_buf, int trans_color, int bgar_flag)
{

    uint16 *canvas = pdivoom_gif_handle->rgb565_canvas;
    int32 i, j, ScreenHeight, ScreenWidth, index, offset;
	ColorMapObject *ColorMap;
    GifFileType *GifFile = pdivoom_gif_handle->gif_handle;
    uint32 pixel_count = (uint32)GifFile->SWidth * (uint32)GifFile->SHeight;
    uint8 *alpha = (uint8 *)canvas + pixel_count * 2U;
    uint16 background = 0;
    uint8 background_alpha = trans_color < 0 ? 0xFF : 0;
    ColorMapObject *global_map = GifFile->SColorMap;

    ColorMap = (GifFile->Image.ColorMap
		? GifFile->Image.ColorMap
		: GifFile->SColorMap);

    if(ColorMap == NULL)
    {
        divoom_print("Gif Image does not have a colormap\n");
        return;
    }
    divoom_gif_lib_prepare_rgb565_map(pdivoom_gif_handle, ColorMap);
    ScreenWidth = GifFile->SWidth;
    ScreenHeight = GifFile->SHeight;
    if(global_map != NULL && GifFile->SBackGroundColor >= 0 &&
        GifFile->SBackGroundColor < global_map->ColorCount)
    {
        GifColorType *color = &global_map->Colors[GifFile->SBackGroundColor];
        background = divoom_rgb888_to_rgb565(color->Red, color->Green, color->Blue);
    }
    /* 仅首帧/循环重开初始化整张画布；文件和嵌入式流遵守相同合成语义。 */
    if(pdivoom_gif_handle->ImageNum == 1)
    {
        for(i = 0; i < ScreenWidth * ScreenHeight; i++)
        {
            canvas[i] = background;
        }
        memset(alpha, background_alpha, pixel_count);
    }
    if(GifFile->DisposalMode == DISPOSE_PREVIOUS)
    {
        memcpy(pdivoom_gif_handle->restore_canvas, canvas, pixel_count * 3U);
    }
    /* 只合成当前帧矩形内的非透明像素，透明处沿用旧颜色而非旧色号。 */
    for(i = GifFile->Image.Top; i < GifFile->Image.Top + GifFile->Image.Height && i < ScreenHeight; i++)
    {
        for(j = GifFile->Image.Left; j < GifFile->Image.Left + GifFile->Image.Width && j < ScreenWidth; j++)
        {
            offset = i * ScreenWidth + j;
            index = pdivoom_gif_handle->ScreenBuffer[offset];
            if(index == trans_color)
            {
                continue;
            }
            if(index >= ColorMap->ColorCount)
            {
                index = 0;
            }
            canvas[offset] = divoom_gif_lib_get_rgb565_color(pdivoom_gif_handle, ColorMap, index);
            alpha[offset] = 0xFF;
        }
    }
    memcpy(rgb_buf, canvas, pixel_count * (bgar_flag ? 3U : 2U));

    /* disposal 描述显示本帧之后的处理，不能在绘制前清空整屏。 */
    if(GifFile->DisposalMode == DISPOSE_BACKGROUND)
    {
        for(i = GifFile->Image.Top; i < GifFile->Image.Top + GifFile->Image.Height && i < ScreenHeight; i++)
        {
            for(j = GifFile->Image.Left; j < GifFile->Image.Left + GifFile->Image.Width && j < ScreenWidth; j++)
            {
                offset = i * ScreenWidth + j;
                canvas[offset] = background;
                alpha[offset] = background_alpha;
            }
        }
    }
    else if(GifFile->DisposalMode == DISPOSE_PREVIOUS)
    {
        memcpy(canvas, pdivoom_gif_handle->restore_canvas, pixel_count * 3U);
    }

}

static int divoom_gif_lib_get_line_to_screen(GifFileType *GifFile,
    GifPixelType *dst, int width, int trans_color, GifPixelType *line_buf)
{
    if(trans_color == -1)
    {
        return DGifGetLine(GifFile, dst, width);
    }
    if(DGifGetLine(GifFile, line_buf, width) == GIF_ERROR)
    {
        return GIF_ERROR;
    }
    /*
     * 索引缓冲只保存本帧原始色号（包括透明色）；跨帧合成在颜色画布中完成，
     * 避免局部调色板切换后将上一帧保留的色号解释成另一种颜色。
     */
    memcpy(dst, line_buf, width * sizeof(GifPixelType));
    return GIF_OK;
}

/**
 * NAME         divoom_gif_lib_get_pic_width
 * @BRIEF	    获取图像的宽度
                
 * @PARAM	    pdivoom_gif_handle : 
 
 * @RETURN	    图像的宽度
 * @RETVAL	
 */

int divoom_gif_lib_get_pic_width(void *handle)
{
    Pdivoom_gif_lib_ctrl pdivoom_gif_handle;
	GifFileType *GifFile;

    pdivoom_gif_handle = (Pdivoom_gif_lib_ctrl)handle;
    if(pdivoom_gif_handle == NULL)
    {
        return 0;
    }

    GifFile = pdivoom_gif_handle->gif_handle;
    return GifFile->SWidth;
}

/**
 * NAME         divoom_gif_lib_get_pic_info
 * @BRIEF	    获取图像的信息
                
 * @PARAM	    pdivoom_gif_handle : 
 
 * @RETURN	    图像的数目
 * @RETVAL	
 */

int divoom_gif_lib_get_pic_info(void *handle, int *width, int *high, int *speed)
{
    Pdivoom_gif_lib_ctrl pdivoom_gif_handle;
	GifFileType *GifFile;
    
    pdivoom_gif_handle = (Pdivoom_gif_lib_ctrl)handle;
    if(pdivoom_gif_handle == NULL || pdivoom_gif_handle->gif_handle == NULL)
    {
        return 0;
    }
    
    GifFile = pdivoom_gif_handle->gif_handle;
    *width = GifFile->SWidth;
    *high = GifFile->SHeight;
    if(speed)
    {
        *speed = GifFile->SSpeed;
    }
    return GifFile->ImageCount;
}

/**
 * NAME         divoom_gif_lib_get_pic_number
 * @BRIEF	    获取图像的张数
                
 * @PARAM	    pdivoom_gif_handle : 
 
 * @RETURN	    图像的张数
 * @RETVAL	
 */

int divoom_gif_lib_get_pic_number(void *handle)
{
    int pic_num = 0;
    Pdivoom_gif_lib_ctrl pdivoom_gif_handle = (Pdivoom_gif_lib_ctrl)handle;
	GifFileType *GifFile = pdivoom_gif_handle->gif_handle;

    if(pdivoom_gif_handle == NULL || pdivoom_gif_handle->gif_handle == NULL)
    {
        return 0;
    }

    if(pdivoom_gif_handle->bgra_buf == NULL)
    {
        pdivoom_gif_handle->bgra_buf = divoom_system_malloc(GifFile->SWidth * GifFile->SHeight *3);
    }

    while(1)
    {
        if(0 == divoom_gif_lib_get_pic_data(handle,  pdivoom_gif_handle->bgra_buf, NULL, NULL, 1, 1))
        {
            divoom_gif_lib_reset(handle, 0);
            break;
        }
        pic_num ++;
    }
    
    return pic_num;
}

/**
 * NAME         divoom_gif_lib_reset
 * @BRIEF	    复位缓冲区
                
 * @PARAM	    handle : 
 
 * @RETURN	    none
 * @RETVAL	
 */

int divoom_gif_lib_reset(void *handle, int offset)
{
    Pdivoom_gif_lib_ctrl pdivoom_gif_handle;
    int Error = 0, Size, i;
    int seek_offset;

    pdivoom_gif_handle = (Pdivoom_gif_lib_ctrl)handle;
    if(pdivoom_gif_handle == NULL)
    {
        divoom_print("gif reset failed: control handle is null\n");
        return 0;
    }
    if(pdivoom_gif_handle->gif_handle != NULL)
    {
        DGifCloseFile(pdivoom_gif_handle->gif_handle, &Error,
            pdivoom_gif_handle->file_type);
        pdivoom_gif_handle->gif_handle = NULL;
    }
    if(pdivoom_gif_handle->file_type == 1)
    {
        pdivoom_gif_handle->gif_handle = DGifOpenFileName(pdivoom_gif_handle->file_name, &Error);
    }
    else
    {
        if(pdivoom_gif_handle->file_handle == NULL)
        {
            divoom_print("gif reset failed: embedded file handle is null, off=%u\n",
                pdivoom_gif_handle->file_off);
            return 0;
        }
        seek_offset = offset < 0 ? (int)pdivoom_gif_handle->file_off : offset;
        if(divoom_port_file_seek(pdivoom_gif_handle->file_handle,
            seek_offset, SEEK_SET) != 0)
        {
            divoom_print("gif reset failed: seek=%d, off=%u\n",
                seek_offset, pdivoom_gif_handle->file_off);
            return 0;
        }
    	pdivoom_gif_handle->gif_handle = DGifOpenFileHandle(pdivoom_gif_handle->file_handle, &Error);
    }
    if(pdivoom_gif_handle->gif_handle == NULL)
    {
        divoom_print("gif reset failed: reopen error=%d, type=%d, off=%u, file=%s\n",
            Error, pdivoom_gif_handle->file_type, pdivoom_gif_handle->file_off,
            pdivoom_gif_handle->file_name[0] ? pdivoom_gif_handle->file_name : "<embedded>");
        return 0;
    }
    if(pdivoom_gif_handle->gif_handle->SWidth != pdivoom_gif_handle->screen_width ||
        pdivoom_gif_handle->gif_handle->SHeight != pdivoom_gif_handle->screen_height)
    {
        divoom_print("gif reset failed: size changed %dx%d -> %dx%d\n",
            pdivoom_gif_handle->screen_width, pdivoom_gif_handle->screen_height,
            pdivoom_gif_handle->gif_handle->SWidth,
            pdivoom_gif_handle->gif_handle->SHeight);
        DGifCloseFile(pdivoom_gif_handle->gif_handle, &Error,
            pdivoom_gif_handle->file_type);
        pdivoom_gif_handle->gif_handle = NULL;
        return 0;
    }

	pdivoom_gif_handle->ImageNum = 0;
	Size = pdivoom_gif_handle->gif_handle->SWidth * sizeof(GifPixelType);/* Size in bytes one row.*/

	for (i = 0; i < pdivoom_gif_handle->gif_handle->SWidth; i++)  /* Set its color to BackGround. */
		pdivoom_gif_handle->ScreenBuffer[i] = pdivoom_gif_handle->gif_handle->SBackGroundColor;
	for (i = 1; i < pdivoom_gif_handle->gif_handle->SHeight; i++) {
		memcpy(&pdivoom_gif_handle->ScreenBuffer[i * pdivoom_gif_handle->gif_handle->SWidth], pdivoom_gif_handle->ScreenBuffer, Size);
	}
    return 1;
}

/**
 * NAME         divoom_gif_lib_get_pic_data
 * @BRIEF	    从句柄中得到多个图像
                
 * @PARAM	    handle : 
                pic_num
                rgb_buf
 
 * @RETURN	    none
 * @RETVAL	
 */

int divoom_gif_lib_get_pic_data(void *handle, uint8 *rgb_buf, uint8 *pic_width,uint16 *speed,int pic_num, int bgar_flag)
{

    Pdivoom_gif_lib_ctrl pdivoom_gif_handle;
	int	i, j, Row, Col, Width, Height, ExtCode, bytes_pre_dot, offset;
	GifRecordType RecordType;
	GifByteType *Extension;
	GifFileType *GifFile;
	int
		InterlacedOffset[] = { 0, 4, 2, 1 }, /* The way Interlaced image should. */
		InterlacedJumps[] = { 8, 8, 4, 2 };    /* be read - offsets and jumps... */
	int ImageNum = 0;

    pdivoom_gif_handle = (Pdivoom_gif_lib_ctrl)handle;
    if(pdivoom_gif_handle == NULL || pdivoom_gif_handle->gif_handle == NULL ||
        pdivoom_gif_handle->ScreenBuffer == NULL ||
        pdivoom_gif_handle->transparent_line == NULL || rgb_buf == NULL ||
        pic_num <= 0)
    {
        divoom_print("gif read rejected: ctrl=%p, gif=%p, screen=%p, line=%p, rgb=%p, num=%d\n",
            pdivoom_gif_handle,
            pdivoom_gif_handle ? pdivoom_gif_handle->gif_handle : NULL,
            pdivoom_gif_handle ? pdivoom_gif_handle->ScreenBuffer : NULL,
            pdivoom_gif_handle ? pdivoom_gif_handle->transparent_line : NULL,
            rgb_buf, pic_num);
        return 0;
    }

    GifFile = pdivoom_gif_handle->gif_handle;
    if(pic_width)
    {
        *pic_width = GifFile->SWidth;
    }
    if(speed)
    {
        *speed = GifFile->SSpeed;
    }
    bytes_pre_dot = bgar_flag ? 3 : 2;
	/* Scan the content of the GIF file and load the image(s) in: */
	do {
		if (DGifGetRecordType(GifFile, &RecordType) == GIF_ERROR) {
			//PrintGifError(GifFile->Error);
			divoom_print("gif record read failed: err=%d, decoded=%d, total=%d, off=%u\n",
                GifFile->Error, ImageNum, pdivoom_gif_handle->ImageNum,
                pdivoom_gif_handle->file_off);
			return 0;
		}
		switch (RecordType) {
		case IMAGE_DESC_RECORD_TYPE:
			if (DGifGetImageDesc(GifFile) == GIF_ERROR) {
				//PrintGifError(GifFile->Error);
                divoom_print("aj test!\n");
                return 0;
			}
            if(speed)
            {
                *speed = GifFile->SSpeed;
            }
            if(GifFile->DisposalMode == DISPOSE_PREVIOUS &&
                pdivoom_gif_handle->restore_canvas == NULL)
            {
                pdivoom_gif_handle->restore_canvas = divoom_system_malloc_out(
                    (uint32)GifFile->SWidth * (uint32)GifFile->SHeight * 3U);
                if(pdivoom_gif_handle->restore_canvas == NULL)
                {
                    return 0;
                }
            }
            ++ImageNum;
            pdivoom_gif_handle->ImageNum ++;
			//printf("\n%s:", pdivoom_gif_handle->ImageNum, Col, Row, Width, Height, GifFile->Image.Interlace);
			if (GifFile->Image.Left + GifFile->Image.Width > GifFile->SWidth ||
				GifFile->Image.Top + GifFile->Image.Height > GifFile->SHeight) {
    			Row = GifFile->SHeight - GifFile->Image.Height; /* Image Position relative to Screen. */
    			Col = GifFile->SWidth - GifFile->Image.Width;
    			Width = GifFile->Image.Width;
    			Height = GifFile->Image.Height;
			}
            else
            {
    			Row = GifFile->Image.Top; /* Image Position relative to Screen. */
    			Col = GifFile->Image.Left;
    			Width = GifFile->Image.Width;
    			Height = GifFile->Image.Height;
            }
			if (GifFile->Image.Interlace) {
				/* Need to perform 4 passes on the images: */
				for ( i = 0; i < 4; i++)
				for (j = Row + InterlacedOffset[i], offset = (Row + InterlacedOffset[i])*GifFile->SWidth + Col; j < Row + Height;
                j += InterlacedJumps[i], offset = InterlacedJumps[i] *GifFile->SWidth) {
					if (divoom_gif_lib_get_line_to_screen(GifFile,
                        &pdivoom_gif_handle->ScreenBuffer[j*GifFile->SWidth + Col],
						Width, GifFile->trans_color,
                        pdivoom_gif_handle->transparent_line) == GIF_ERROR) {
						divoom_print("gif line read failed: err=%d, image=%d, row=%d, col=%d, size=%dx%d, interlace=1\n",
                            GifFile->Error, ImageNum, j, Col, Width, Height);
                        return ImageNum - 1;
					}
				}
			}
			else {
				for (i = 0, offset = Row*GifFile->SWidth + Col; i < Height; i++, offset += GifFile->SWidth) {
					if (divoom_gif_lib_get_line_to_screen(GifFile,
                        &pdivoom_gif_handle->ScreenBuffer[offset],
						Width, GifFile->trans_color,
                        pdivoom_gif_handle->transparent_line) == GIF_ERROR) {
						divoom_print("gif line read failed: err=%d, image=%d, row=%d, col=%d, size=%dx%d, interlace=0\n",
                            GifFile->Error, ImageNum, Row, Col, Width, Height);
                        return ImageNum - 1;
					}
                    Row++;
				}
			}
            divoom_gif_lib_convert_to_rgb_buf(pdivoom_gif_handle, &rgb_buf[(ImageNum -1)*GifFile->SHeight*GifFile->SWidth*bytes_pre_dot], GifFile->trans_color, bgar_flag);
            //处理相关数据
            if(ImageNum >= pic_num)
            {
                return ImageNum;
            }
			break;
		case EXTENSION_RECORD_TYPE:
			/* Skip any extension blocks in file: */
			if (DGifGetExtension(GifFile, &ExtCode, &Extension) == GIF_ERROR) {
                divoom_print("aj test!\n");
                return 0;
			}
			while (Extension != NULL) {
				if (DGifGetExtensionNext(GifFile, &Extension) == GIF_ERROR) {
                    divoom_print("aj test!\n");
                    return 0;
				}
			}
			break;
		case TERMINATE_RECORD_TYPE:
			break;
		default:		    /* Should be trapped by DGifGetRecordType. */
			break;
		}
	} while (RecordType != TERMINATE_RECORD_TYPE);
   
    return ImageNum;
}

/**
 * NAME         divoom_gif_lib_exit
 * @BRIEF	    释放相关资源
                
 * @PARAM	    handle : 
                
 * @RETURN	    none
 * @RETVAL	
 */

void divoom_gif_lib_exit(void *handle)
{
    int Error = 0;
    int gif_close_ret = GIF_OK;
    int file_close_ret = 0;
    Pdivoom_gif_lib_ctrl pdivoom_gif_handle;
    
    pdivoom_gif_handle = (Pdivoom_gif_lib_ctrl)handle;
    if(pdivoom_gif_handle == NULL)
    {
        return ;
    }
    if(pdivoom_gif_handle->gif_handle)
    {
        gif_close_ret = DGifCloseFile(pdivoom_gif_handle->gif_handle, &Error,
            pdivoom_gif_handle->file_type);
        if(gif_close_ret == GIF_ERROR)
        {
            divoom_print("gif decoder close failed: ctrl=%p, err=%d, close_file=%u\n",
                pdivoom_gif_handle, Error, pdivoom_gif_handle->file_type);
        }
        pdivoom_gif_handle->gif_handle = NULL;
    }
    if(pdivoom_gif_handle->file_type == 0 && pdivoom_gif_handle->file_handle)
    {
        /* 嵌入式 GIF 的 DGifCloseFile 不关闭外部 FILE*，由本控制句柄统一收口。 */
        file_close_ret = divoom_port_file_close(pdivoom_gif_handle->file_handle);
        if(file_close_ret != 0)
        {
            divoom_print("gif embedded file close failed: handle=%p, ret=%d\n",
                pdivoom_gif_handle->file_handle, file_close_ret);
        }
        pdivoom_gif_handle->file_handle = NULL;
    }
    if(pdivoom_gif_handle->bgra_buf)
    {
        divoom_system_free(pdivoom_gif_handle->bgra_buf);
    }
    if(pdivoom_gif_handle->transparent_line)
    {
        divoom_system_free(pdivoom_gif_handle->transparent_line);
    }
    if(pdivoom_gif_handle->rgb565_map)
    {
        divoom_system_free(pdivoom_gif_handle->rgb565_map);
    }
    if(pdivoom_gif_handle->rgb565_map_ready)
    {
        divoom_system_free(pdivoom_gif_handle->rgb565_map_ready);
    }
    divoom_system_free(pdivoom_gif_handle->ScreenBuffer);
    divoom_system_free(pdivoom_gif_handle->rgb565_canvas);
    if(pdivoom_gif_handle->restore_canvas != NULL)
    {
        divoom_system_free(pdivoom_gif_handle->restore_canvas);
    }
    divoom_system_free(pdivoom_gif_handle);
}

/**
 * NAME         divoom_gif_lib_bgra_pass_frame_num
 * @BRIEF	    跳过多少帧图像
                
 * @PARAM	    handle :
                pic_num
 
 * @RETURN	    none
 * @RETVAL	
 */

void divoom_gif_lib_bgra_pass_frame_num(void *handle, int pic_num)
{
    int i;
    Pdivoom_gif_lib_ctrl pdivoom_gif_handle = (Pdivoom_gif_lib_ctrl)handle;
	GifFileType *GifFile = pdivoom_gif_handle->gif_handle;
    
    if(pdivoom_gif_handle == NULL || pdivoom_gif_handle->gif_handle == NULL)
    {
        return ;
    }

    if(pdivoom_gif_handle->bgra_buf == NULL)
    {
        pdivoom_gif_handle->bgra_buf = divoom_system_malloc(GifFile->SWidth * GifFile->SHeight *3);
    }

    for(i = 0; i < pic_num; i++)
    {
        if(0 == divoom_gif_lib_get_pic_data(handle,  pdivoom_gif_handle->bgra_buf, NULL, NULL, 1, 1))
        {
            divoom_gif_lib_reset(handle, 0);
        }
    }
    
}



/**
 * NAME         divoom_gif_lib_bgra_get_bgra_data
 * @BRIEF	    从句柄中得到多个bgra图像,这个返回值不需要释放，是循环使用
                
 * @PARAM	    handle :
                speed
                end_flag
 
 * @RETURN	    返回bgra缓冲区
 * @RETVAL	
 */

uint8 * divoom_gif_lib_bgra_get_bgra_data(void *handle, int *speed,int *end_flag)
{
    uint16 gif_speed;
    Pdivoom_gif_lib_ctrl pdivoom_gif_handle = (Pdivoom_gif_lib_ctrl)handle;
	GifFileType *GifFile = pdivoom_gif_handle->gif_handle;
    
    if(pdivoom_gif_handle == NULL || pdivoom_gif_handle->gif_handle == NULL)
    {
        return 0;
    }

    if(pdivoom_gif_handle->bgra_buf == NULL)
    {
        pdivoom_gif_handle->bgra_buf = divoom_system_malloc(GifFile->SWidth * GifFile->SHeight *3);
    }

    if(0 == divoom_gif_lib_get_pic_data(pdivoom_gif_handle,  pdivoom_gif_handle->bgra_buf, NULL, &gif_speed, 1, 1))
    {
        divoom_gif_lib_reset(pdivoom_gif_handle, 0);
        if(end_flag)
        {
            *end_flag = 1;
        }
        GifFile = pdivoom_gif_handle->gif_handle;
        divoom_gif_lib_get_pic_data(pdivoom_gif_handle,  pdivoom_gif_handle->bgra_buf, NULL, &gif_speed, 1, 1);
    }
    else
    {        
        if(end_flag)
        {
            *end_flag = 0;
        }
    }
    if(speed)
    {
        *speed = gif_speed;
    }
    return pdivoom_gif_handle->bgra_buf;
}


/**
 * NAME         divoom_gif_lib_bgra_get_bgra_data
 * @BRIEF	    从句柄中得到多个bgra图像,这个返回值需要释放
                
 * @PARAM	    handle :
                speed
                end_flag
 
 * @RETURN	    返回bgra缓冲区
 * @RETVAL	
 */

uint8 * divoom_gif_lib_bgra_get_malloc_bgra_data(void *handle, int *speed,int *end_flag)
{
    uint8 *bgra_buf;
    uint16 gif_speed;
    Pdivoom_gif_lib_ctrl pdivoom_gif_handle = (Pdivoom_gif_lib_ctrl)handle;
	GifFileType *GifFile = pdivoom_gif_handle->gif_handle;

    if(pdivoom_gif_handle == NULL || pdivoom_gif_handle->gif_handle == NULL)
    {
        return 0;
    }

    bgra_buf = divoom_system_malloc(GifFile->SWidth * GifFile->SHeight *3);

    if(0 == divoom_gif_lib_get_pic_data(pdivoom_gif_handle,  bgra_buf, NULL, &gif_speed, 1, 1))
    {
        divoom_gif_lib_reset(pdivoom_gif_handle, 0);
        if(end_flag)
        {
            *end_flag = 1;
        }
        GifFile = pdivoom_gif_handle->gif_handle;
        divoom_gif_lib_get_pic_data(pdivoom_gif_handle,  bgra_buf, NULL, &gif_speed, 1, 1);
    }
    else
    {        
        if(end_flag)
        {
            *end_flag = 0;
        }
    }
    if(speed)
    {
        *speed = gif_speed;
    }
    return bgra_buf;
}
