/**
 * @file disp2.h
 *
 */

#ifndef SUNXIFB_PORT_LINUX_FBDEV_H
#define SUNXIFB_PORT_LINUX_FBDEV_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef USE_SUNXIFB_PORT_LINUX_FBDEV
/*********************
 *      INCLUDES
 *********************/

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

void lv_linux_fbdev_get_sizes(uint32_t *width, uint32_t *height);
void lv_linux_fbdev_mmap(uint32_t bpp, uint32_t *fbnum, char **mem_phy, char **mem_vir);
void lv_linux_fbdev_init(void);
void lv_linux_fbdev_exit(void);
int lv_linux_fbdev_pan_display(uint32_t yoffset);

/**********************
 *      MACROS
 **********************/

#endif  /*USE_SUNXIFB_PORT_LINUX_FBDEV*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*SUNXIFB_PORT_LINUX_FBDEV_H*/
