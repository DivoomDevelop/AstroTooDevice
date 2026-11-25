#ifndef __RV_VECTOR_H__
#define __RV_VECTOR_H__

#ifdef CONFIG_ARCH_RISCV_VECTOR
#include "isa.h"

#ifndef __riscv_vector
#error The toolchain did not define __riscv_vector, please check compile option!
#endif

#ifdef CONFIG_ARCH_RISCV_INTERRUPT_NEST
/*
 Currently XuanTie E900 CPU support interrupt preemption, but not support RISC-V vector extension
 So we don't need to support interrupt preemption.
*/
//#define VECTOR_CTX_SUPPORT_INTERRUPT_PREEMPTION
#error Interrupt preemption is not supported!
#endif

#if ((CONFIG_RISCV_VECTOR_REG_SIZE % 4) != 0)
#error CONFIG_RISCV_VECTOR_REG_SIZE must be a multiple of 4
#endif

#define RV_VECTOR_REG_SIZE CONFIG_RISCV_VECTOR_REG_SIZE

#define RV_VECTOR_GENERAL_REG_CNT (32)
#define RV_VECTOR_CSR_REG_CNT (4)
#define VECTOR_CONTEXT_SIZE ((RV_VECTOR_GENERAL_REG_CNT * RV_VECTOR_REG_SIZE) + (RV_VECTOR_CSR_REG_CNT * CPU_WORD_LEN))

#define VECTOR_CTX_VX_OFFSET(n) ((n) * RV_VECTOR_REG_SIZE)

#define VECTOR_CTX_VSTART_OFFSET (RV_VECTOR_GENERAL_REG_CNT * RV_VECTOR_REG_SIZE)
#ifdef CONFIG_RISCV_VECTOR_V0_7_1
#define VECTOR_CTX_VL_OFFSET (VECTOR_CTX_VSTART_OFFSET + CPU_WORD_LEN)
#else
#define VECTOR_CTX_VCSR_OFFSET (VECTOR_CTX_VSTART_OFFSET + CPU_WORD_LEN)
#define VECTOR_CTX_VL_OFFSET (VECTOR_CTX_VCSR_OFFSET + CPU_WORD_LEN)
#endif
#define VECTOR_CTX_VTYPE_OFFSET (VECTOR_CTX_VL_OFFSET + CPU_WORD_LEN)

#if defined(CONFIG_RISCV_VECTOR_V0_7_1)
#define STORE_V_REG vse.v
#define LOAD_V_REG vle.v
#else
#define STORE_V_REG vse8.v
#define LOAD_V_REG vle8.v
#endif

#ifndef __ASSEMBLY__

typedef riscv_reg_t vector_cnt_t;

typedef struct
{
	uint32_t v_reg_part[RV_VECTOR_REG_SIZE / sizeof(uint32_t)];
} vector_reg_t;

typedef struct
{
	vector_reg_t v_reg[RV_VECTOR_GENERAL_REG_CNT];
	riscv_csr_t vstart;
#ifdef CONFIG_RISCV_VECTOR_V0_7_1
	/* vxsat and vxrm save by FPU releated code */
#else
	riscv_csr_t vcsr;
#endif
	riscv_csr_t vl;
	riscv_csr_t vtype;
} vector_context_t;

#ifdef CONFIG_RECORD_RISCV_VECTOR_CTX_STATISTICS
extern vector_cnt_t g_skip_vector_ctx_save_cnt;
extern vector_cnt_t g_skip_vector_off_ctx_restore_cnt, g_skip_vector_init_ctx_restore_cnt, g_skip_vector_clean_ctx_restore_cnt;
#endif

#endif /* __ASSEMBLY__ */

#endif /* CONFIG_ARCH_RISCV_VECTOR */

#endif /* __RV_VECTOR_H__ */
