/** @file

  File Name     : g2d_scale.h
  Version       : Initial Draft
  Author        : eric_wang
  Created       : 2023/02/17
  Last Modified :
  Description   : use g2d to scale/convert picture size and format
  Function List :
  History       :
*/

#ifndef _G2D_SCALE_H_
#define _G2D_SCALE_H_

#include <mm_common.h>
#include <mm_comm_video.h>
#ifdef CONFIG_KERNEL_FREERTOS
#include <g2d_driver.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct ScalePictureParam
{
    PIXEL_FORMAT_E eSrcPixFormat;
    unsigned int eSrcColorSpace;  // colorspace identifier (Linux: v4l2_colorspace, FreeRTOS: G2D_BT709=1)
    unsigned int mSrcPhyAddrs[3];
    SIZE_S mSrcPicSize;
    RECT_S mSrcValidRect;

    PIXEL_FORMAT_E eDstPixFormat;
    unsigned int eDstColorSpace;  // colorspace identifier (Linux: v4l2_colorspace, FreeRTOS: G2D_BT709=1)
    unsigned int mDstPhyAddrs[3];
    SIZE_S mDstPicSize;
    RECT_S mDstValidRect;

    int nRotate; //0, 90, 180, 270. clock-wise. can't scale!
}ScalePictureParam;

int ScalePictureByG2d(ScalePictureParam *pScaleInfo, int nG2dFd);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* _G2D_SCALE_H_ */

