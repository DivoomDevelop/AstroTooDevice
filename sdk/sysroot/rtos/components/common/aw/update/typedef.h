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
#ifndef _TYPEDEF_H_
#define _TYPEDEF_H_

#include <pthread.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef  UPDATE_DEBUG
#define  UPDATE_DEBUG
#endif

#define __s8		int8_t
#define __s16		int16_t
#define __s32		int32_t
#define __s64		int64_t

#define __u8		uint8_t
#define __u16		uint16_t
#define __u32		uint32_t
#define __u64		uint64_t

#define uint		unsigned int
#define sint		signed int

#define eLIBs_printf 		printf
#define esMEMS_Balloc 		malloc
#define esMEMS_free 		free

#define esMEMS_Malloc(x, size) 	malloc(size)
#define esMEMS_Bfree(x, size) 	free(x)

#define eLIBs_malloc 	malloc
#define eLIBs_free 		free
#define eLIBs_memset 	memset
#define eLIBs_sprintf 	sprintf
#define eLIBs_snprintf 	snprintf
#define eLIBs_strcpy 	strcpy
#define eLIBs_strncpy 	strncpy
#define eLIBs_strlcpy 	strlcpy
#define eLIBs_strcmp 	strcmp
#define eLIBs_strncmp 	strncmp
#define eLIBs_strnicmp 	strncasecmp
#define eLIBs_memcpy 	memcpy
#define eLIBs_strlen 	strlen
#define eLIBs_strstr 	strstr
#define eLIBs_strchr 	strchr

#define eLIBs_fopen 	fopen
#define eLIBs_fread 	fread
#define eLIBs_fseek 	fseek
#define eLIBs_ftell 	ftell
#define eLIBs_fclose 	fclose

// 定义颜色宏
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_RESET   "\033[0m"

// 修改宏定义
#define __err(fmt, ...)  printf(COLOR_RED "[ERR] " fmt " " COLOR_RESET, ##__VA_ARGS__)
#define __msg(fmt, ...)  printf(COLOR_GREEN "[MSG] " fmt " " COLOR_RESET, ##__VA_ARGS__)
#define __log(fmt, ...)  printf(COLOR_BLUE "[LOG] " fmt " " COLOR_RESET, ##__VA_ARGS__)

#define EPDK_OK               0
#define EPDK_FAIL            -1
#define EPDK_TRUE             1
#define EPDK_FALSE            0

#define SPINOR_ONLINE_LOGICAL_OFFSET     	((CONFIG_PARTITIONS_LOGICAL_OFFSET-16)*1024)
#define SPINOR_GPT_TABLE_SIZE    			GPT_TABLE_SIZE
#define SPINOR_GPT_START_MAPPING 			GPT_START_MAPPING
#ifdef CONFIG_SUNXI_ONLINE_UPDATE
#define SPINOR_GPT_ADDRESS       			SPINOR_ONLINE_LOGICAL_OFFSET
#else
#define SPINOR_GPT_ADDRESS       			(SPINOR_GPT_START_MAPPING - SPINOR_GPT_TABLE_SIZE)
#endif

#define SPINAND_GPT_TABLE_SIZE   (64*1024)
#define SPINAND_GPT_ADDRESS      0

#define UPDATE_THREAD_LOW_PRIORITY    17
#define UPDATE_THREAD_NORMAL_PRIORITY 18
#define UPDATE_THREAD_HIGH_PRIORITY   19

#define UPDATE_THREAD_STACK_SIZE (0x4000)
typedef void *(*update_thread_func_t)(void *arg);

static inline int update_thread_create(pthread_t *tid, update_thread_func_t start, const char *name,
                                    void *arg, int priority, int joinable)
{
    int ret;
    pthread_attr_t attr;
    struct sched_param sched;
    (void)name; /* Each worker names itself before it can exit. */

    memset(&sched, 0, sizeof(sched));
    sched.sched_priority = priority;
    pthread_attr_init(&attr);
    pthread_attr_setschedparam(&attr, &sched);
    pthread_attr_setdetachstate(&attr, joinable);
    pthread_attr_setstacksize(&attr, UPDATE_THREAD_STACK_SIZE);
    ret = pthread_create(tid, &attr, start, arg);
    if (ret != 0){
		printf("pthread_create error : %d\n", ret);
		return ret;
	}
    return ret;
}

#endif /* _TYPEDEF_H_ */
