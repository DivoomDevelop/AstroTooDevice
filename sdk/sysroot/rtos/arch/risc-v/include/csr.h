#ifndef _ASM_RISCV_CSR_H
#define _ASM_RISCV_CSR_H

#include "asm.h"
#include "consts.h"

/* Status register flags */
#define SR_SIE      _AC(0x00000002, UL) /* Supervisor Interrupt Enable */
#define SR_SPIE     _AC(0x00000020, UL) /* Previous Supervisor IE */
#define SR_SPP      _AC(0x00000100, UL) /* Previously Supervisor */
#define SR_SUM      _AC(0x00040000, UL) /* Supervisor User Memory Access */

#define SR_MIE      _AC(0x00000008, UL) /* Supervisor Interrupt Enable */
#define SR_MPIE     _AC(0x00000080, UL) /* Previous Supervisor IE */
#define SR_MPP      _AC(0x00001800, UL) /* Previously Supervisor */

#define SR_FS       _AC(0x00006000, UL) /* Floating-point Status */
#define SR_FS_OFF   _AC(0x00000000, UL)
#define SR_FS_INITIAL   _AC(0x00002000, UL)
#define SR_FS_CLEAN _AC(0x00004000, UL)
#define SR_FS_DIRTY _AC(0x00006000, UL)

#define SR_XS       _AC(0x00018000, UL) /* Extension Status */
#define SR_XS_OFF   _AC(0x00000000, UL)
#define SR_XS_INITIAL   _AC(0x00008000, UL)
#define SR_XS_CLEAN _AC(0x00010000, UL)
#define SR_XS_DIRTY _AC(0x00018000, UL)

#ifdef CONFIG_ARCH_RISCV_C906
#define SR_VS       _AC(0x01800000, UL) /* Vector extension Status */
#define SR_VS_OFF   _AC(0x00000000, UL)
#define SR_VS_INITIAL   _AC(0x00800000, UL)
#define SR_VS_CLEAN _AC(0x01000000, UL)
#define SR_VS_DIRTY _AC(0x01800000, UL)
#else
#define SR_VS       _AC(0x00000600, UL) /* Vector extension Status */
#define SR_VS_OFF   _AC(0x00000000, UL)
#define SR_VS_INITIAL   _AC(0x00000200, UL)
#define SR_VS_CLEAN _AC(0x00000400, UL)
#define SR_VS_DIRTY _AC(0x00000600, UL)
#endif

#define MR_MPP      _AC(0x00001800, UL) /* Previously Machine */
#define MR_MPIE     _AC(0x00000080, UL) /* Previously Machine */
#define MR_MIE      _AC(0x00000008, UL) /* Previously Machine */

#ifndef CONFIG_64BIT
#define SR_SD       _AC(0x80000000, UL) /* FS/XS dirty */
#else
#define SR_SD       _AC(0x8000000000000000, UL) /* FS/XS dirty */
#endif

/* SATP flags */
#ifndef CONFIG_64BIT
#define SATP_PPN    _AC(0x003FFFFF, UL)
#define SATP_MODE_32    _AC(0x80000000, UL)
#define SATP_MODE   SATP_MODE_32
#else
#define SATP_PPN    _AC(0x00000FFFFFFFFFFF, UL)
#define SATP_MODE_39    _AC(0x8000000000000000, UL)
#define SATP_MODE   SATP_MODE_39
#define SATP_ASID_BITS  16
#define SATP_ASID_SHIFT 44
#define SATP_ASID_MASK  _AC(0xFFFF, UL)
#endif

/* SCAUSE */
#define SCAUSE_IRQ_FLAG     (_AC(1, UL) << (__riscv_xlen - 1))

#define MCAUSE_IRQ_MASK     (_AC(0xfff, UL))
#define IRQ_U_SOFT      0
#define IRQ_S_SOFT      1
#define IRQ_M_SOFT      3
#define IRQ_U_TIMER     4
#define IRQ_S_TIMER     5
#define IRQ_M_TIMER     7
#define IRQ_U_EXT       8
#define IRQ_S_EXT       9
#define IRQ_M_EXT       11
#define IRQ_S_PMU       17

#define EXC_INST_MISALIGNED     0
#define EXC_INST_ACCESS         1
#define EXC_INST_ILLEGAL        2
#define EXC_BREAKPOINT          3
#define EXC_LOAD_MISALIGN       4
#define EXC_LOAD_ACCESS         5
#define EXC_STORE_MISALIGN      6
#define EXC_STORE_ACCESS        7
#define EXC_SYSCALL_FRM_U       8
#define EXC_SYSCALL_FRM_S       9
#define EXC_SYSCALL_FRM_M       11
#define EXC_INST_PAGE_FAULT     12
#define EXC_LOAD_PAGE_FAULT     13
#define EXC_STORE_PAGE_FAULT    15

#define CSR_CYCLE       0xc00
#define CSR_TIME        0xc01
#define CSR_INSTRET     0xc02
#define CSR_CYCLEH      0xc80
#define CSR_TIMEH       0xc81
#define CSR_INSTRETH    0xc82

/* S-Mode System Registers */
/* SIE (Interrupt Enable) and SIP (Interrupt Pending) flags */
#define SIE_SSIE        (_AC(0x1, UL) << IRQ_S_SOFT)
#define SIE_STIE        (_AC(0x1, UL) << IRQ_S_TIMER)
#define SIE_SEIE        (_AC(0x1, UL) << IRQ_S_EXT)
#define SIE_SMIE        (_AC(0x1, UL) << IRQ_S_PMU)

#define CSR_SSTATUS     0x100
#define CSR_SIE         0x104
#define CSR_STVEC       0x105
#define CSR_SCOUNTEREN  0x106

#define CSR_SSCRATCH    0x140
#define CSR_SEPC        0x141
#define CSR_SCAUSE      0x142
#define CSR_STVAL       0x143
#define CSR_SIP         0x144
#define CSR_SATP        0x180

/* M-Mode System Registers */
#define MIE_MSIE        (_AC(0x1, UL) << IRQ_M_SOFT)
#define MIE_MTIE        (_AC(0x1, UL) << IRQ_M_TIMER)
#define MIE_MEIE        (_AC(0x1, UL) << IRQ_M_EXT)

#define CSR_MSTATUS     0x300
#define CSR_MISA        0x301
#define CSR_MEDELEG     0x302
#define CSR_MIDELEG     0x303
#define CSR_MIE         0x304
#define CSR_MTVEC       0x305
#define CSR_MCOUNTEREN  0x306

#define CSR_MSCRATCH    0x340
#define CSR_MEPC        0x341
#define CSR_MCAUSE      0x342
#define CSR_MTVAL       0x343
#define CSR_MIP         0x344

/* Physical Memory Protection */
#define CSR_PMPCFG0			0x3a0
#define CSR_PMPCFG1			0x3a1
#define CSR_PMPCFG2			0x3a2
#define CSR_PMPCFG3			0x3a3
#define CSR_PMPCFG4			0x3a4
#define CSR_PMPCFG5			0x3a5
#define CSR_PMPCFG6			0x3a6
#define CSR_PMPCFG7			0x3a7
#define CSR_PMPCFG8			0x3a8
#define CSR_PMPCFG9			0x3a9
#define CSR_PMPCFG10			0x3aa
#define CSR_PMPCFG11			0x3ab
#define CSR_PMPCFG12			0x3ac
#define CSR_PMPCFG13			0x3ad
#define CSR_PMPCFG14			0x3ae
#define CSR_PMPCFG15			0x3af
#define CSR_PMPADDR0			0x3b0
#define CSR_PMPADDR1			0x3b1
#define CSR_PMPADDR2			0x3b2
#define CSR_PMPADDR3			0x3b3
#define CSR_PMPADDR4			0x3b4
#define CSR_PMPADDR5			0x3b5
#define CSR_PMPADDR6			0x3b6
#define CSR_PMPADDR7			0x3b7
#define CSR_PMPADDR8			0x3b8
#define CSR_PMPADDR9			0x3b9
#define CSR_PMPADDR10			0x3ba
#define CSR_PMPADDR11			0x3bb
#define CSR_PMPADDR12			0x3bc
#define CSR_PMPADDR13			0x3bd
#define CSR_PMPADDR14			0x3be
#define CSR_PMPADDR15			0x3bf
#define CSR_PMPADDR16			0x3c0
#define CSR_PMPADDR17			0x3c1
#define CSR_PMPADDR18			0x3c2
#define CSR_PMPADDR19			0x3c3
#define CSR_PMPADDR20			0x3c4
#define CSR_PMPADDR21			0x3c5
#define CSR_PMPADDR22			0x3c6
#define CSR_PMPADDR23			0x3c7
#define CSR_PMPADDR24			0x3c8
#define CSR_PMPADDR25			0x3c9
#define CSR_PMPADDR26			0x3ca
#define CSR_PMPADDR27			0x3cb
#define CSR_PMPADDR28			0x3cc
#define CSR_PMPADDR29			0x3cd
#define CSR_PMPADDR30			0x3ce
#define CSR_PMPADDR31			0x3cf
#define CSR_PMPADDR32			0x3d0
#define CSR_PMPADDR33			0x3d1
#define CSR_PMPADDR34			0x3d2
#define CSR_PMPADDR35			0x3d3
#define CSR_PMPADDR36			0x3d4
#define CSR_PMPADDR37			0x3d5
#define CSR_PMPADDR38			0x3d6
#define CSR_PMPADDR39			0x3d7
#define CSR_PMPADDR40			0x3d8
#define CSR_PMPADDR41			0x3d9
#define CSR_PMPADDR42			0x3da
#define CSR_PMPADDR43			0x3db
#define CSR_PMPADDR44			0x3dc
#define CSR_PMPADDR45			0x3dd
#define CSR_PMPADDR46			0x3de
#define CSR_PMPADDR47			0x3df
#define CSR_PMPADDR48			0x3e0
#define CSR_PMPADDR49			0x3e1
#define CSR_PMPADDR50			0x3e2
#define CSR_PMPADDR51			0x3e3
#define CSR_PMPADDR52			0x3e4
#define CSR_PMPADDR53			0x3e5
#define CSR_PMPADDR54			0x3e6
#define CSR_PMPADDR55			0x3e7
#define CSR_PMPADDR56			0x3e8
#define CSR_PMPADDR57			0x3e9
#define CSR_PMPADDR58			0x3ea
#define CSR_PMPADDR59			0x3eb
#define CSR_PMPADDR60			0x3ec
#define CSR_PMPADDR61			0x3ed
#define CSR_PMPADDR62			0x3ee
#define CSR_PMPADDR63			0x3ef

#define CSR_MCOR         0x7c2
#define CSR_MHCR         0x7c1
#define CSR_MCCR2        0x7c3
#define CSR_MHINT        0x7c5
#define CSR_MXSTATUS     0x7c0
#define CSR_PLIC_BASE    0xfc1
#define CSR_MRMR         0x7c6
#define CSR_MRVBR        0x7c7

#define CSR_MCYCLE			0xb00
#define CSR_MINSTRET			0xb02
#define CSR_MHPMCOUNTER3		0xb03
#define CSR_MHPMCOUNTER4		0xb04
#define CSR_MHPMCOUNTER5		0xb05
#define CSR_MHPMCOUNTER6		0xb06
#define CSR_MHPMCOUNTER7		0xb07
#define CSR_MHPMCOUNTER8		0xb08
#define CSR_MHPMCOUNTER9		0xb09
#define CSR_MHPMCOUNTER10		0xb0a
#define CSR_MHPMCOUNTER11		0xb0b
#define CSR_MHPMCOUNTER12		0xb0c
#define CSR_MHPMCOUNTER13		0xb0d
#define CSR_MHPMCOUNTER14		0xb0e
#define CSR_MHPMCOUNTER15		0xb0f
#define CSR_MHPMCOUNTER16		0xb10
#define CSR_MHPMCOUNTER17		0xb11
#define CSR_MHPMCOUNTER18		0xb12
#define CSR_MHPMCOUNTER19		0xb13
#define CSR_MHPMCOUNTER20		0xb14
#define CSR_MHPMCOUNTER21		0xb15
#define CSR_MHPMCOUNTER22		0xb16
#define CSR_MHPMCOUNTER23		0xb17
#define CSR_MHPMCOUNTER24		0xb18
#define CSR_MHPMCOUNTER25		0xb19
#define CSR_MHPMCOUNTER26		0xb1a
#define CSR_MHPMCOUNTER27		0xb1b
#define CSR_MHPMCOUNTER28		0xb1c
#define CSR_MHPMCOUNTER29		0xb1d
#define CSR_MHPMCOUNTER30		0xb1e
#define CSR_MHPMCOUNTER31		0xb1f

/* Machine Counter Setup */
#define CSR_MCOUNTINHIBIT		0x320
#define CSR_MCYCLECFG			0x321
#define CSR_MINSTRETCFG			0x322
#define CSR_MHPMEVENT3			0x323
#define CSR_MHPMEVENT4			0x324
#define CSR_MHPMEVENT5			0x325
#define CSR_MHPMEVENT6			0x326
#define CSR_MHPMEVENT7			0x327
#define CSR_MHPMEVENT8			0x328
#define CSR_MHPMEVENT9			0x329
#define CSR_MHPMEVENT10			0x32a
#define CSR_MHPMEVENT11			0x32b
#define CSR_MHPMEVENT12			0x32c
#define CSR_MHPMEVENT13			0x32d
#define CSR_MHPMEVENT14			0x32e
#define CSR_MHPMEVENT15			0x32f
#define CSR_MHPMEVENT16			0x330
#define CSR_MHPMEVENT17			0x331
#define CSR_MHPMEVENT18			0x332
#define CSR_MHPMEVENT19			0x333
#define CSR_MHPMEVENT20			0x334
#define CSR_MHPMEVENT21			0x335
#define CSR_MHPMEVENT22			0x336
#define CSR_MHPMEVENT23			0x337
#define CSR_MHPMEVENT24			0x338
#define CSR_MHPMEVENT25			0x339
#define CSR_MHPMEVENT26			0x33a
#define CSR_MHPMEVENT27			0x33b
#define CSR_MHPMEVENT28			0x33c
#define CSR_MHPMEVENT29			0x33d
#define CSR_MHPMEVENT30			0x33e
#define CSR_MHPMEVENT31			0x33f

/* For RV32 */
#define CSR_MCYCLECFGH			0x721
#define CSR_MINSTRETCFGH		0x722
#define CSR_MHPMEVENT3H			0x723
#define CSR_MHPMEVENT4H			0x724
#define CSR_MHPMEVENT5H			0x725
#define CSR_MHPMEVENT6H			0x726
#define CSR_MHPMEVENT7H			0x727
#define CSR_MHPMEVENT8H			0x728
#define CSR_MHPMEVENT9H			0x729
#define CSR_MHPMEVENT10H		0x72a
#define CSR_MHPMEVENT11H		0x72b
#define CSR_MHPMEVENT12H		0x72c
#define CSR_MHPMEVENT13H		0x72d
#define CSR_MHPMEVENT14H		0x72e
#define CSR_MHPMEVENT15H		0x72f
#define CSR_MHPMEVENT16H		0x730
#define CSR_MHPMEVENT17H		0x731
#define CSR_MHPMEVENT18H		0x732
#define CSR_MHPMEVENT19H		0x733
#define CSR_MHPMEVENT20H		0x734
#define CSR_MHPMEVENT21H		0x735
#define CSR_MHPMEVENT22H		0x736
#define CSR_MHPMEVENT23H		0x737
#define CSR_MHPMEVENT24H		0x738
#define CSR_MHPMEVENT25H		0x739
#define CSR_MHPMEVENT26H		0x73a
#define CSR_MHPMEVENT27H		0x73b
#define CSR_MHPMEVENT28H		0x73c
#define CSR_MHPMEVENT29H		0x73d
#define CSR_MHPMEVENT30H		0x73e
#define CSR_MHPMEVENT31H		0x73f

// REF - RISC-V External Debug Support Version 0.13-DRAFT
#define CSR_TSELECT			0x7a0
#define CSR_TDATA1			0x7a1
#define CSR_MCONTROL			0x7a1
#define CSR_ICOUNT			0x7a1
#define CSR_ITRIGGER			0x7a1
#define CSR_ETRIGGER			0x7a1
#define CSR_TDATA2			0x7a2
#define CSR_TDATA3			0x7a3
#define CSR_TEXTRA32			0x7a3
#define CSR_TEXTRA64			0x7a3
#define CSR_TINFO			0x7a4
#define CSR_TCONCTRL			0x7a5
#define CSR_MCONTEXT			0x7a8
#define CSR_SCONTEXT			0x7aa

#define CSR_MVENDORID			0xf11
#define CSR_MARCHID			0xf12
#define CSR_MIMPID			0xf13
#define CSR_MHARTID			0xf14
#define CSR_CYCLEH			0xc80
#define CSR_TIMEH			0xc81
#define CSR_INSTRETH			0xc82
#define CSR_HPMCOUNTER3H		0xc83
#define CSR_HPMCOUNTER4H		0xc84
#define CSR_HPMCOUNTER5H		0xc85
#define CSR_HPMCOUNTER6H		0xc86
#define CSR_HPMCOUNTER7H		0xc87
#define CSR_HPMCOUNTER8H		0xc88
#define CSR_HPMCOUNTER9H		0xc89
#define CSR_HPMCOUNTER10H		0xc8a
#define CSR_HPMCOUNTER11H		0xc8b
#define CSR_HPMCOUNTER12H		0xc8c
#define CSR_HPMCOUNTER13H		0xc8d
#define CSR_HPMCOUNTER14H		0xc8e
#define CSR_HPMCOUNTER15H		0xc8f
#define CSR_HPMCOUNTER16H		0xc90
#define CSR_HPMCOUNTER17H		0xc91
#define CSR_HPMCOUNTER18H		0xc92
#define CSR_HPMCOUNTER19H		0xc93
#define CSR_HPMCOUNTER20H		0xc94
#define CSR_HPMCOUNTER21H		0xc95
#define CSR_HPMCOUNTER22H		0xc96
#define CSR_HPMCOUNTER23H		0xc97
#define CSR_HPMCOUNTER24H		0xc98
#define CSR_HPMCOUNTER25H		0xc99
#define CSR_HPMCOUNTER26H		0xc9a
#define CSR_HPMCOUNTER27H		0xc9b
#define CSR_HPMCOUNTER28H		0xc9c
#define CSR_HPMCOUNTER29H		0xc9d
#define CSR_HPMCOUNTER30H		0xc9e
#define CSR_HPMCOUNTER31H		0xc9f
#define CSR_MCYCLEH			0xb80
#define CSR_MINSTRETH			0xb82
#define CSR_MHPMCOUNTER3H		0xb83
#define CSR_MHPMCOUNTER4H		0xb84
#define CSR_MHPMCOUNTER5H		0xb85
#define CSR_MHPMCOUNTER6H		0xb86
#define CSR_MHPMCOUNTER7H		0xb87
#define CSR_MHPMCOUNTER8H		0xb88
#define CSR_MHPMCOUNTER9H		0xb89
#define CSR_MHPMCOUNTER10H		0xb8a
#define CSR_MHPMCOUNTER11H		0xb8b
#define CSR_MHPMCOUNTER12H		0xb8c
#define CSR_MHPMCOUNTER13H		0xb8d
#define CSR_MHPMCOUNTER14H		0xb8e
#define CSR_MHPMCOUNTER15H		0xb8f
#define CSR_MHPMCOUNTER16H		0xb90
#define CSR_MHPMCOUNTER17H		0xb91
#define CSR_MHPMCOUNTER18H		0xb92
#define CSR_MHPMCOUNTER19H		0xb93
#define CSR_MHPMCOUNTER20H		0xb94
#define CSR_MHPMCOUNTER21H		0xb95
#define CSR_MHPMCOUNTER22H		0xb96
#define CSR_MHPMCOUNTER23H		0xb97
#define CSR_MHPMCOUNTER24H		0xb98
#define CSR_MHPMCOUNTER25H		0xb99
#define CSR_MHPMCOUNTER26H		0xb9a
#define CSR_MHPMCOUNTER27H		0xb9b
#define CSR_MHPMCOUNTER28H		0xb9c
#define CSR_MHPMCOUNTER29H		0xb9d
#define CSR_MHPMCOUNTER30H		0xb9e
#define CSR_MHPMCOUNTER31H		0xb9f

/* RISC-V Vector Extension Register */
#define CSR_VSTART 0x008
#define CSR_VXSAT 0x009
#define CSR_VXRM 0x00a
#define CSR_VCSR 0x00f
#define CSR_VL 0xc20
#define CSR_VTYPE 0xc21
#define CSR_VLENB 0xc22

#ifndef __ASSEMBLY__

#define csr_swap(csr, val)                  \
    ({                              \
        unsigned long __v = (unsigned long)(val);       \
        __asm__ __volatile__ ("csrrw %0, " __ASM_STR(csr) ", %1"\
                              : "=r" (__v) : "rK" (__v)     \
                              : "memory");          \
        __v;                            \
    })

#define csr_read(csr)                       \
    ({                              \
        register unsigned long __v;             \
        __asm__ __volatile__ ("csrr %0, " __ASM_STR(csr)    \
                              : "=r" (__v) :            \
                              : "memory");          \
        __v;                            \
    })

#define csr_write(csr, val)                 \
    ({                              \
        unsigned long __v = (unsigned long)(val);       \
        __asm__ __volatile__ ("csrw " __ASM_STR(csr) ", %0" \
                              : : "rK" (__v)            \
                              : "memory");          \
    })

#define csr_read_set(csr, val)                  \
    ({                              \
        unsigned long __v = (unsigned long)(val);       \
        __asm__ __volatile__ ("csrrs %0, " __ASM_STR(csr) ", %1"\
                              : "=r" (__v) : "rK" (__v)     \
                              : "memory");          \
        __v;                            \
    })

#define csr_set(csr, val)                   \
    ({                              \
        unsigned long __v = (unsigned long)(val);       \
        __asm__ __volatile__ ("csrs " __ASM_STR(csr) ", %0" \
                              : : "rK" (__v)            \
                              : "memory");          \
    })

#define csr_read_clear(csr, val)                \
    ({                              \
        unsigned long __v = (unsigned long)(val);       \
        __asm__ __volatile__ ("csrrc %0, " __ASM_STR(csr) ", %1"\
                              : "=r" (__v) : "rK" (__v)     \
                              : "memory");          \
        __v;                            \
    })

#define csr_clear(csr, val)                 \
    ({                              \
        unsigned long __v = (unsigned long)(val);       \
        __asm__ __volatile__ ("csrc " __ASM_STR(csr) ", %0" \
                              : : "rK" (__v)            \
                              : "memory");          \
    })

unsigned long csr_read_num(unsigned int csr_num);
void csr_write_num(unsigned int csr_num, unsigned long val);

#endif /* __ASSEMBLY__ */

#endif /* _ASM_RISCV_CSR_H */
