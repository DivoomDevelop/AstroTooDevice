#ifndef __CLEAR_FW_H__
#define __CLEAR_FW_H__
#include <fcntl.h>
#include "image/image.h"

typedef struct __fw_update_info
{
	__u8 *Data;
	__u64 Already_UpdateSize;
	__u64 Current_UpdateItemSize;
	__u64 TotalSize;         // unit by 512
	__u32 Burn_prog;
	__s32 Burn_status;     //是否校验OK的标志
	__u8 MainType[MAINTYPE_LEN+2];
	__u8 subType[SUBTYPE_LEN+2];
} __Update_Info_t;

#ifdef CONFIG_SUNXI_UPDATE_FIRMWARE_BUFF
__s32 FW_Img_Open(HIMAGE h_img);
#else
__s32 FW_Img_Open(const char *fw_name);
#endif
__s32 FW_ImgItem_Open(__Update_Info_t *info);
__u64 FW_GetItemSize(__Update_Info_t *info);
__s32 FW_GetItemData(__Update_Info_t *info, __u64 len);
__s32 FW_ImgItem_Close(void);
__s32 FW_Img_Close(void);
#endif
