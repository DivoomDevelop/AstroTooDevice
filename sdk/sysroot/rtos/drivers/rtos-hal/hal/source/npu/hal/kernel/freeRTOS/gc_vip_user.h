/******************************************************************************\
|* Copyright (c) 2017-2023 by Vivante Corporation.  All Rights Reserved.      *|
|*                                                                            *|
|* The material in this file is confidential and contains trade secrets of    *|
|* of Vivante Corporation.  This is proprietary information owned by Vivante  *|
|* Corporation.  No part of this work may be disclosed, reproduced, copied,   *|
|* transmitted, or used in any way for any purpose, without the express       *|
|* written permission of Vivante Corporation.                                 *|
|*                                                                            *|
\******************************************************************************/


#ifndef _GC_VIP_USER_H
#define _GC_VIP_USER_H

#include "../../../include/gc_vip_common.h"


/*
@Brief Allocate a system memory.
@param size The size of memory.
@param memory Allocate memory object.
*/
vip_status_e gcvip_user_allocate_memory(
    IN vip_uint32_t size,
    OUT void **memory
    );

/*
@Brief Free system memory.
@param memory The memory object should be free.
*/
vip_status_e gcvip_user_free_memory(
    IN void *Memory
    );


#endif
