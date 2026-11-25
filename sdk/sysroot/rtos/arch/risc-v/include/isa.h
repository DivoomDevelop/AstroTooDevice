#ifndef __ISA_H__
#define __ISA_H__

#if !defined(__riscv_xlen)
	#error The toolchain did not define __riscv_xlen!
#elif __riscv_xlen == 64
	#define CPU_REG_SIZE 8
	#define CPU_WORD_LEN CPU_REG_SIZE

	#define STORE_REG sd
	#define LOAD_REG ld
	#define FMV_FROM_REG fmv.d.x
#elif __riscv_xlen == 32
	#define CPU_REG_SIZE 4
	#define CPU_WORD_LEN CPU_REG_SIZE

	#define STORE_REG sw
	#define LOAD_REG lw
	#define FMV_FROM_REG fmv.w.x
#else
	#error Unsupported __riscv_xlen value!
#endif

#ifdef CONFIG_ARCH_RISCV_FPU
#if !defined(__riscv_flen)
	#error The toolchain did not define __riscv_flen, please check compile option!
#elif __riscv_flen == 64
	#define FPU_REG_SIZE 8

	#define FSTORE fsd
	#define FLOAD fld
#elif __riscv_flen == 32
	#define FPU_REG_SIZE 4

	#define FSTORE fsw
	#define FLOAD flw
#else
	#error Unsupported __riscv_flen value!
#endif
#endif /* CONFIG_ARCH_RISCV_FPU */

#ifndef __ASSEMBLY__

#include <stdint.h>

#if CPU_REG_SIZE == 8
typedef uint64_t riscv_reg_t;
typedef riscv_reg_t riscv_csr_t;
#elif CPU_REG_SIZE == 4
typedef uint32_t riscv_reg_t;
typedef riscv_reg_t riscv_csr_t;
#else
#error Unsupported CPU register size or not define __riscv_xlen!
#endif

#endif /* __ASSEMBLY__ */

#endif /* __ISA_H__ */
