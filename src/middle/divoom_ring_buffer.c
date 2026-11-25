/**@brief 仿照linux kfifo写的ring buffer
* https://www.linuxidc.com/Linux/2016-12/137936.htm
 *@atuher Anker  date:2013-12-18
* ring_buffer.h
 * */

#include "divoom.h"



//判断x是否是2的次方
#define divoom_ring_buffer_is_power_of_2(x) ((x) != 0 && (((x) & ((x) - 1)) == 0))
//取a和b中最小值
#define divoom_ring_buffer_min(a, b) (((a) < (b)) ? (a) : (b))

typedef struct _ring_buffer__
{
    void		*buffer;     //缓冲区
    uint32	size;       //大小
    uint32	in;         //入口位置
    uint32	out;        //出口位置
//    pthread_mutex_t *f_lock;    //互斥锁
	void *f_lock;
}ring_buffer;

void divoom_ring_mutex_lock(const void *handle)
{
    ring_buffer *ring_buf = (ring_buffer *)handle;
	divoom_system_get_lock(ring_buf->f_lock);
}

void divoom_ring_mutex_unlock(const void *handle)
{
    ring_buffer *ring_buf = (ring_buffer *)handle;
	divoom_system_free_lock(ring_buf->f_lock);
}

//size缓冲区空间的大小，在初化时，将它向上圆整成2的幂
void* divoom_ring_buffer_init(uint32 size)//, pthread_mutex_t *f_lock)
{
    ring_buffer *ring_buf = NULL;
	void *buffer = NULL;
    if (!divoom_ring_buffer_is_power_of_2(size))
    {
        divoom_print("%s:size must be power of 2.\n",__func__);
        return ring_buf;
    }
    ring_buf = (ring_buffer *)divoom_system_malloc(sizeof(ring_buffer));
    if (!ring_buf)
    {
        divoom_print("%s:Failed to malloc memory!\n",__func__);
        return ring_buf;
    }
	buffer = divoom_system_malloc(size);
    if (!buffer)
    {
        divoom_print("%s:ring_buf create failed!\n", __func__);
        divoom_system_free(ring_buf);
        return NULL;
    }
    memset(ring_buf, 0, sizeof(ring_buffer));
    ring_buf->buffer = buffer;
    ring_buf->size = size;
    ring_buf->in = 0;
    ring_buf->out = 0;
	ring_buf->f_lock = divoom_system_create_lock();
	if (!ring_buf->f_lock)
	{
		divoom_system_free(buffer);
		divoom_system_free(ring_buf);
		return NULL;
	}
    return ring_buf;
}

void *divoom_ring_buffer_init_static(uint32 size, void *buffer)
{
	ring_buffer *ring_buf = NULL;

	if (!buffer || !divoom_ring_buffer_is_power_of_2(size))
	{
		divoom_print("%s: bad size/buffer\n", __func__);
		return NULL;
	}
	ring_buf = (ring_buffer *)divoom_system_malloc(sizeof(ring_buffer));
	if (!ring_buf)
	{
		divoom_print("%s:Failed to malloc control block!\n", __func__);
		return NULL;
	}
	memset(ring_buf, 0, sizeof(ring_buffer));
	ring_buf->buffer = buffer;
	ring_buf->size = size;
	ring_buf->in = 0;
	ring_buf->out = 0;
	ring_buf->f_lock = divoom_system_create_lock();
	if (!ring_buf->f_lock)
	{
		divoom_system_free(ring_buf);
		return NULL;
	}
	return ring_buf;
}

void divoom_ring_buffer_free_static(void *handle)
{
	ring_buffer *ring_buf = handle;

	if (!ring_buf)
	{
		return;
	}
	ring_buf->in = 0;
	ring_buf->out = 0;
	if (ring_buf->f_lock)
	{
		divoom_system_destroy_lock(ring_buf->f_lock);
		ring_buf->f_lock = NULL;
	}
	ring_buf->buffer = NULL;
	ring_buf->size = 0;
	divoom_system_free(ring_buf);
}

//释放缓冲区
void divoom_ring_buffer_free(void *handle)
{
	ring_buffer *ring_buf = handle;
    if (ring_buf)
    {
        if (ring_buf->buffer)
        {
            divoom_system_free(ring_buf->buffer);
            ring_buf->buffer = NULL;
        }
		if(ring_buf->f_lock)
		{
			divoom_system_destroy_lock(ring_buf->f_lock);
		}
        divoom_system_free(ring_buf);
        ring_buf = NULL;
    }
}

//缓冲区的长度
uint32 __ring_buffer_len(const void *handle)
{
	const ring_buffer *ring_buf = handle;
    return (ring_buf->in - ring_buf->out);
}

//从缓冲区中取数据
uint32 __ring_buffer_get(void *handle, void * buffer, uint32 size)
{
	ring_buffer *ring_buf = handle;
    uint32 len = 0;
    size  = divoom_ring_buffer_min(size, ring_buf->in - ring_buf->out);
    /* first get the data from fifo->out until the end of the buffer */
    len = divoom_ring_buffer_min(size, ring_buf->size - (ring_buf->out & (ring_buf->size - 1)));
    memcpy(buffer, (unsigned char*)ring_buf->buffer + (ring_buf->out & (ring_buf->size - 1)), len);
    /* then get the rest (if any) from the beginning of the buffer */
    memcpy((unsigned char*)buffer + len, ring_buf->buffer, size - len);
    ring_buf->out += size;
    return size;
}
//向缓冲区中存放数据
uint32 __ring_buffer_put(void *handle, void *buffer, uint32 size)
{
	ring_buffer *ring_buf = handle;
    uint32 len = 0;
    size = divoom_ring_buffer_min(size, ring_buf->size - ring_buf->in + ring_buf->out);
    /* first put the data starting from fifo->in to buffer end */
    len  = divoom_ring_buffer_min(size, ring_buf->size - (ring_buf->in & (ring_buf->size - 1)));
    memcpy((unsigned char*)ring_buf->buffer + (ring_buf->in & (ring_buf->size - 1)), buffer, len);
    /* then put the rest (if any) at the beginning of the buffer */
    memcpy(ring_buf->buffer, (unsigned char*)buffer + len, size - len);
    ring_buf->in += size;
    return size;
}


uint32 divoom_ring_buffer_getdatasize(const void *handle)
{
    uint32 len = 0;
	if(NULL == handle)
	{
		return 0;
	}
	divoom_ring_mutex_lock(handle);
    len = __ring_buffer_len(handle);
	divoom_ring_mutex_unlock(handle);
    return len;
}


uint32 divoom_ring_buffer_getblanksize(const void *handle)
{
	const ring_buffer *ring_buf = handle;
    uint32 len = 0;
	if(NULL == handle)
	{
		return 0;
	}
	divoom_ring_mutex_lock(handle);
	len =  ring_buf->size - __ring_buffer_len(handle);
	divoom_ring_mutex_unlock(handle);
	return len;
}


uint32 divoom_ring_buffer_get(void *handle, void *buffer, uint32 size)
{
    uint32 ret;
	if(NULL == handle)
	{
		return 0;
	}
    divoom_ring_mutex_lock(handle);
    ret = __ring_buffer_get(handle, buffer, size);
    //buffer中没有数据
//    if (ring_buf->in == ring_buf->out)
//        ring_buf->in = ring_buf->out = 0;
    divoom_ring_mutex_unlock(handle);
    return ret;
}

uint32 divoom_ring_buffer_put(void *handle, void *buffer, uint32 size)
{
	ring_buffer *ring_buf = handle;
    uint32 ret;
	if(NULL == handle)
	{
		return 0;
	}
    divoom_ring_mutex_lock(handle);
    ret = __ring_buffer_put(ring_buf, buffer, size);
    divoom_ring_mutex_unlock(handle);
    return ret;
}


void divoom_ring_buffer_clr(void *handle)
{
	ring_buffer *ring_buf = handle;
	if(NULL == handle)
	{
		return;
	}
    divoom_ring_mutex_lock(handle);
	ring_buf->in = ring_buf->out = 0;
    divoom_ring_mutex_unlock(handle);
}




