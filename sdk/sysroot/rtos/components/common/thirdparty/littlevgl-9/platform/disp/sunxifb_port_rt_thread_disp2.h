/**
 * @file disp2.h
 *
 */

#ifndef SUNXIFB_PORT_RT_THREAD_DISP2_H
#define SUNXIFB_PORT_RT_THREAD_DISP2_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#ifdef USE_SUNXIFB_PORT_RT_THREAD_DISP2

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

void lv_rt_thread_disp2_get_sizes(uint32_t *width, uint32_t *height);
void lv_rt_thread_disp2_mmap(uint32_t bpp, uint32_t *fbnum, char **mem_phy, char **mem_vir);
void lv_rt_thread_disp2_init(void);
void lv_rt_thread_disp2_exit(void);
int lv_rt_thread_disp2_pan_display(uint32_t yoffset);

/**********************
 *      MACROS
 **********************/

#endif  /*USE_DISP2*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*DISP2_H*/
