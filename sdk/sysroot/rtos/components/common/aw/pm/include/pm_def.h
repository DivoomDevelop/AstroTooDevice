#ifndef _PM_DEF_H_
#define _PM_DEF_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#define     __IO    volatile
#define HAL_REG_32BIT(reg_addr)  (*((__IO uint32_t *)(reg_addr)))
#define HAL_REG_16BIT(reg_addr)  (*((__IO uint16_t *)(reg_addr)))
#define HAL_REG_8BIT(reg_addr)   (*((__IO uint8_t  *)(reg_addr)))
#define HAL_GET_BIT_VAL(reg, shift, vmask)  (((reg) >> (shift)) & (vmask))
#define HAL_SET_BIT_VAL(reg, shift, vmask, val) \
	((reg) = ((reg) & (~((vmask) << (shift)))) | (((val) & (vmask)) << (shift)))
#define HAL_MODIFY_REG(reg, clr_mask, set_mask) \
	((reg) = (((reg) & (~(clr_mask))) | (set_mask)))

#define HAL_GET_BITS(reg_addr, mask)   (HAL_REG_32BIT(reg_addr) & (mask))
#define HAL_CLR_BITS(reg_addr, clr)    (HAL_REG_32BIT(reg_addr) &= ~(clr))
#define HAL_SET_BITS(reg_addr, set)    (HAL_REG_32BIT(reg_addr) |= (set))
#define HAL_CLR_SET_BITS(reg_addr, clr, set) \
do {\
	HAL_REG_32BIT(reg_addr) &= ~(clr);\
	HAL_REG_32BIT(reg_addr) |= (set);\
} while (0)

#ifdef __cplusplus
}
#endif
#endif /* _PM_DEF_H_ */