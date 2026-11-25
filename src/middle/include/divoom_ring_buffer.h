#ifndef __divoom_ring_buffer_
#define __divoom_ring_buffer_

//size缓冲区空间的大小，在初化时，将它向上圆整成2的幂
void* divoom_ring_buffer_init(uint32 size);

/* 数据区由调用方提供（静态池）；仅分配 ring 控制块与锁，热路径不 malloc 数据区 */
void *divoom_ring_buffer_init_static(uint32 size, void *buffer);

/* 释放 init_static 的控制块与锁；不释放 buffer */
void divoom_ring_buffer_free_static(void *handle);

void divoom_ring_buffer_free(void *handle);

uint32 divoom_ring_buffer_getdatasize(const void *handle);

uint32 divoom_ring_buffer_getblanksize(const void *handle);

uint32 divoom_ring_buffer_get(void *handle, void *buffer, uint32 size);

uint32 divoom_ring_buffer_put(void *handle, void *buffer, uint32 size);

void divoom_ring_buffer_clr(void *handle);


#endif


