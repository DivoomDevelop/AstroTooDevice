#include <stdio.h>
#include "memoryAdapter.h"
#include "vdecoder.h"

#define Vdec_GetMemAdapterOpsS() \
    MemAdapterGetOpsS()

#define Vdec_MemPalloc(memops, nSize)         \
    CdcMemPalloc(memops, nSize, NULL, NULL)

#define Vdec_MemPfree(memops, pMem)          \
    CdcMemPfree(memops, pMem, NULL, NULL)

#define Vdec_MemPallocSecure(memops, nSize) \
    CdcMemPallocSecure(memops, nSize, NULL, NULL)

#define Vdec_MemOpen(memops)           \
    CdcMemOpen(memops)

#define Vdec_MemClose(memops)          \
    CdcMemClose(memops)

#define Vdec_MemFlushCache(memops, pMem, nSize)  \
    CdcMemFlushCache(memops, pMem, nSize)

#define Vdec_MemTotalSize(memops)   \
    CdcMemTotalSize(memops)

#define Vdec_MemCopy(memops, pMemDst, pMemSrc, nSize) \
    CdcMemCopy(memops, pMemDst, pMemSrc, nSize)

#define Vdec_MemRead(memops, pMemDst, pMemSrc, nSize) \
    CdcMemRead(memops, pMemDst, pMemSrc, nSize)

#define Vdec_MemWrite(memops, pMemDst, pMemSrc, nSize) \
    CdcMemWrite(memops, pMemDst, pMemSrc, nSize)

#define Vdec_MemGetPhysicAddress(memops, virt) \
    CdcMemGetPhysicAddressCpu(memops, virt)

#define Vdec_MemGetVirtualAddress(memops, phy) \
    CdcMemGetVirtualAddressCpu(memops, phy)

#define Vdec_MemSetup(memops) \
    CdcMemSetup(memops)

#define Vdec_MemShutdown(memops) \
    CdcMemShutdown(memops)

#define Vdec_MemGetVirtualAddrByHandle(memops, int) \
    CdcMemGetVirByFd(memops, int, 0)



