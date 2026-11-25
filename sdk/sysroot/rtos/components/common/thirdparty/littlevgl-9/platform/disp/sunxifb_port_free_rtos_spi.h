/**
 * @file spilcd.h
 *
 */

#ifndef SPILCD_H
#define SPILCD_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/


#ifdef USE_SPILCD


/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void lv_free_rtos_spi_init(void);
int lv_free_rtos_spi_pan_display(uint32_t yoffset);
void lv_free_rtos_spi_get_sizes(uint32_t *width, uint32_t *height);
void lv_free_rtos_spi_mmap(uint32_t bpp, uint32_t *fbnum, char **mem_phy, char **mem_vir);
void lv_free_rtos_spi_exit(void);

/**********************
 *      MACROS
 **********************/

#endif  /*USE_SPILCD*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  /*SPILCD_H*/
