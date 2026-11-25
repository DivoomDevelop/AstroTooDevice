#ifndef _LINUX_KASAN_H
#define _LINUX_KASAN_H

#ifdef CONFIG_KASAN

#ifndef CONFIG_KASAN_PLATFORM_CHECK
#define PLATFORM_KASAN_CHECK_ADDR_FILTER(addr)
#endif
#if defined(CONFIG_ARCH_RISCV_E906) || defined(CONFIG_ARCH_RISCV_E907) || defined(CONFIG_THEAD_XUANTIE_C900)
/**
 * The virtual address area is not fully divided.
 * Shadow area is placed at the end of the area [CONFIG_ARCH_START_ADDRESS, CONFIG_ARCH_START_ADDRESS + CONFIG_ARCH_MEM_LENGTH).
 */
#define KASAN_SHADOW_SCALE_SHIFT 	(3)
#define KASAN_SHADOW_SIZE   		(CONFIG_ARCH_MEM_LENGTH >> KASAN_SHADOW_SCALE_SHIFT)
#define KASAN_SHADOW_START      	((unsigned long)CONFIG_ARCH_START_ADDRESS + (unsigned long)CONFIG_ARCH_MEM_LENGTH - (unsigned long)KASAN_SHADOW_SIZE)
#define KASAN_SHADOW_END        	(KASAN_SHADOW_START + KASAN_SHADOW_SIZE)
#define KASAN_SHADOW_OFFSET		CONFIG_KASAN_SHADOW_OFFSET

/* CONFIG_KASAN_SHADOW_OFFSET = (KASAN_SHADOW_START - (unsigned long)(CONFIG_ARCH_START_ADDRESS >> KASAN_SHADOW_SCALE_SHIFT)) */
#if (CONFIG_KASAN_SHADOW_OFFSET == 0x0)
# error "CONFIG_KASAN_SHADOW_OFFSET is invalid, please set CONFIG_KASAN_SHADOW_OFFSET!"
#endif

#undef likely
#undef unlikely
#define  likely(x)        __builtin_expect(!!(x), 1)
#define  unlikely(x)      __builtin_expect(!!(x), 0)

#ifdef CONFIG_KASAN_PLATFORM_CHECK
#define PLATFORM_KASAN_CHECK_ADDR_FILTER(addr) \
		       unsigned long t = (unsigned long)addr; \
		       if ((t < CONFIG_ARCH_START_ADDRESS) || \
				               (t > KASAN_SHADOW_END)) \
		       return

static inline int platform_check_memory_region_invalid_inline(unsigned long addr)
{
	if ((addr < CONFIG_ARCH_START_ADDRESS) || (addr >= KASAN_SHADOW_END))
		return 1;

	return 0;
}
#endif /* CONFIG_KASAN_PLATFORM_CHECK */
#else /* CONFIG_ARCH_RISCV_E906 || CONFIG_ARCH_RISCV_E907 */
#define VA_BITS 32
#define KASAN_SHADOW_SIZE   (1 << (VA_BITS - 3))

#define KASAN_SHADOW_SCALE_SHIFT (3)

#define VA_START (0x20000000L)

#define KASAN_SHADOW_START      (VA_START)
#define KASAN_SHADOW_END        ((unsigned long)KASAN_SHADOW_START + (unsigned long)KASAN_SHADOW_SIZE)

#define  likely(x)        __builtin_expect(!!(x), 1)
#define  unlikely(x)      __builtin_expect(!!(x), 0)

/*
 * This value is used to map an address to the corresponding shadow
 * address by the following formula:
 *     shadow_addr = (address >> 3) + KASAN_SHADOW_OFFSET;
 *
 * (1 << 61) shadow addresses - [KASAN_SHADOW_OFFSET,KASAN_SHADOW_END]
 * cover all 64-bits of virtual addresses. So KASAN_SHADOW_OFFSET
 * should satisfy the following equation:
 *      KASAN_SHADOW_OFFSET = KASAN_SHADOW_END - (1ULL << 61)
 */
#define KASAN_SHADOW_OFFSET     CONFIG_KASAN_SHADOW_OFFSET
#endif /* CONFIG_ARCH_RISCV_E906 || CONFIG_ARCH_RISCV_E907 */

void kasan_init(void);
void kasan_early_init(void);
void kasan_init_report(void);

void rt_page_alloc_func_hook(void *ptr, uint32_t pages);
void rt_page_free_func_hook(void *ptr, uint32_t pages);
void rt_malloc_large_func_hook(void *ptr, uint32_t size);
void rt_free_large_func_hook(void *ptr, uint32_t size);
void rt_malloc_small_func_hook(void *ptr, uint32_t size);
void rt_free_small_func_hook(void *ptr, uint32_t size);
void rt_realloc_small_func_hook(void *ptr, uint32_t size);
void rt_free_func_hook(void *ptr);

void kasan_populate_zero_shadow(const void *shadow_start,
                                const void *shadow_end);

static inline void *kasan_mem_to_shadow(const void *addr)
{
    return (void *)((unsigned long)addr >> KASAN_SHADOW_SCALE_SHIFT)
           + KASAN_SHADOW_OFFSET;
}

#else /* CONFIG_KASAN */


#endif /* CONFIG_KASAN */

#endif /* LINUX_KASAN_H */
