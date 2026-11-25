/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _COMPILER_H_
#define _COMPILER_H_

#ifndef likely
#define likely(x) __builtin_expect((long)!!(x), 1L)
#endif

#ifndef unlikely
#define unlikely(x) __builtin_expect((long)!!(x), 0L)
#endif

#if defined(__CC_ARM)
/* ARM Compiler */

#define inline __inline
//#define __inline	__inline
#define __inline__ __inline

#ifndef __always_inline
#define __always_inline __forceinline
#endif

#ifndef __noinline
#define __noinline
#endif

#ifdef __CHECKER__
#define __iomem __attribute__((noderef, address_space(2)))
#else
#define __iomem
#endif

#if defined(__GNUC__)
/* ARM Compiler support GNU */
#define __packed __attribute__((__packed__))
#else
//#define __packed	__packed
#endif
//#define __asm		__asm
//#define __weak	__weak

#elif defined(__GNUC__)
/* GNU Compiler */

#include <sys/cdefs.h>

//#define inline	inline
#define __inline inline
#define __inline__ inline

#ifdef __always_inline
#undef __always_inline /* already defined in <sys/cdefs.h> */
#define __always_inline inline __attribute__((always_inline))
#endif

#ifndef __noinline
#define __noinline __attribute__((__noinline__))
#endif

#ifndef __packed
#define __packed __attribute__((__packed__))
#endif

#ifndef __asm
#define __asm asm
#endif

#ifndef __weak
#define __weak __attribute__((weak))
#endif

#ifndef __maybe_unused
#define __maybe_unused __attribute__((unused))
#endif

#define _IS_ENABLED3(ignore_this, val, ...) val
#define _IS_ENABLED2(one_or_two_args) _IS_ENABLED3(one_or_two_args 1, 0)
#define _XXXX1 _YYYY,
#define _IS_ENABLED1(macro) _IS_ENABLED2(_XXXX##macro)
#define IS_ENABLED(macro) _IS_ENABLED1(macro)

#ifdef CONFIG_SECTION_ATTRIBUTE_XIP
#define __xip_text \
	__attribute__((section(".xip_text"))) __attribute__((aligned(16)))
#define __xip_rodata \
	__attribute__((section(".xip_rodata"))) __attribute__((aligned(16)))
#else
#define __xip_text
#define __xip_rodata
#endif

#ifdef CONFIG_SECTION_ATTRIBUTE_NONXIP
#define __nonxip_text __attribute__((section(".nonxip_text")))
#define __nonxip_rodata __attribute__((section(".nonxip_rodata")))
#define __nonxip_data __attribute__((section(".nonxip_data")))
#define __nonxip_bss __attribute__((section(".nonxip_bss")))
#else
#define __nonxip_text
#define __nonxip_rodata
#define __nonxip_data
#define __nonxip_bss
#endif

#ifdef CONFIG_SECTION_ATTRIBUTE_SRAM
#define __sram_text __attribute__((section(".sram_text")))
#define __sram_rodata __attribute__((section(".sram_rodata")))
#define __sram_data __attribute__((section(".sram_data")))
#define __sram_bss __attribute__((section(".sram_bss")))
#else
#define __sram_text
#define __sram_rodata
#define __sram_data
#define __sram_bss
#endif

#ifdef CONFIG_SECTION_ATTRIBUTE_PSRAM
#define __psram_text \
	__attribute__((section(".psram_text"))) __attribute__((aligned(16)))
#define __psram_rodata \
	__attribute__((section(".psram_rodata"))) __attribute__((aligned(16)))
#define __psram_data \
	__attribute__((section(".psram_data"))) __attribute__((aligned(16)))
#define __psram_bss \
	__attribute__((section(".psram_bss"))) __attribute__((aligned(16)))
#else
#define __psram_text
#define __psram_rodata
#define __psram_data
#define __psram_bss
#endif

#ifdef CONFIG_SECTION_ATTRIBUTE_HPSRAM
#define __hpsram_text \
	__attribute__((section(".hpsram_text"))) __attribute__((aligned(16)))
#define __hpsram_rodata \
	__attribute__((section(".hpsram_rodata"))) __attribute__((aligned(16)))
#define __hpsram_data \
	__attribute__((section(".hpsram_data"))) __attribute__((aligned(16)))
#define __hpsram_bss \
	__attribute__((section(".hpsram_bss"))) __attribute__((aligned(16)))
#else
#define __hpsram_text
#define __hpsram_rodata
#define __hpsram_data
#define __hpsram_bss
#endif

#else
#error "Compiler not supported."
#endif

#endif /* _COMPILER_H_ */
