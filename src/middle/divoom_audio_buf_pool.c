#include "divoom_build_config.h"

#include "divoom.h"
#include "divoom_audio_buf_pool.h"

#include <string.h>

#if DIVOOM_BUILD_RTOS_DEVICE
#include <pthread.h>

typedef struct {
	uint8_t leased;
	char owner[16];
	void *ring;
} divoom_audio_buf_pool_slot_t;

static divoom_audio_buf_pool_slot_t s_slots[DIVOOM_AUDIO_BUS_COUNT];
static unsigned int s_lease_fail_cnt;
static int s_inited;
/* Static initialization prevents competing first-time users creating
 * different locks. No allocation failure can silently disable exclusion. */
static pthread_mutex_t s_pool_lock = PTHREAD_MUTEX_INITIALIZER;
static void divoom_audio_buf_pool_lock(void) { pthread_mutex_lock(&s_pool_lock); }
static void divoom_audio_buf_pool_unlock(void) { pthread_mutex_unlock(&s_pool_lock); }

static int divoom_audio_buf_pool_is_dynamic(divoom_audio_bus_t bus)
{
    return bus == DIVOOM_AUDIO_BUS_SLEEP
#ifdef CONFIG_DIVOOM_MPP_NET_AUDIO
        || bus == DIVOOM_AUDIO_BUS_NETWORK
#endif
        ;
}

int divoom_audio_buf_pool_init(void)
{
	unsigned int i;

	divoom_audio_buf_pool_lock();
	if (s_inited)
	{
		divoom_audio_buf_pool_unlock();
		return 0;
	}
	for (i = 0; i < DIVOOM_AUDIO_BUS_COUNT; i++)
	{
		uint32 bytes = DIVOOM_AUDIO_BUF_POOL_RING_BYTES;

#if DIVOOM_SHOW_TF_NAND
		if (i == DIVOOM_AUDIO_BUS_MUSIC)
		{
			bytes = DIVOOM_AUDIO_BUF_POOL_MUSIC_RING_BYTES;
		}
#endif
		if (divoom_audio_buf_pool_is_dynamic((divoom_audio_bus_t)i))
		{
			continue;
		}
		s_slots[i].ring = divoom_ring_buffer_init(bytes);
		if (!s_slots[i].ring)
		{
			divoom_print("audio_buf_pool: init fail bus=%u\n", i);
			while (i > 0) {
				--i;
				if (s_slots[i].ring)
				{
					divoom_ring_buffer_free(s_slots[i].ring);
					s_slots[i].ring = NULL;
				}
			}
			divoom_audio_buf_pool_unlock();
			return -1;
		}
	}
	s_inited = 1;
	divoom_audio_buf_pool_unlock();
	divoom_print("audio_buf_pool: init ok slots=%u bytes=%u\n",
		     (unsigned)DIVOOM_AUDIO_BUS_COUNT,
		     (unsigned)DIVOOM_AUDIO_BUF_POOL_RING_BYTES);
	return 0;
}

int divoom_audio_buf_pool_acquire(divoom_audio_bus_t bus, const char *owner_tag)
{
	divoom_audio_buf_pool_slot_t *slot;

	if (bus >= DIVOOM_AUDIO_BUS_COUNT || divoom_audio_buf_pool_init() != 0)
	{
		__atomic_add_fetch(&s_lease_fail_cnt, 1, __ATOMIC_RELAXED);
		return -1;
	}

	divoom_audio_buf_pool_lock();
	slot = &s_slots[bus];
	if (slot->leased)
	{
		divoom_print("audio_buf_pool: acquire fail bus=%u busy owner=%s req=%s\n",
			     (unsigned)bus, slot->owner,
			     owner_tag ? owner_tag : "?");
		__atomic_add_fetch(&s_lease_fail_cnt, 1, __ATOMIC_RELAXED);
		divoom_audio_buf_pool_unlock();
		return -1;
	}
	if (!slot->ring)
	{
		uint32 bytes = DIVOOM_AUDIO_BUF_POOL_RING_BYTES;
#ifdef CONFIG_DIVOOM_MPP_NET_AUDIO
		if (bus == DIVOOM_AUDIO_BUS_NETWORK)
		{
			bytes = DIVOOM_AUDIO_BUF_POOL_NETWORK_RING_BYTES;
		}
#endif
		slot->ring = divoom_ring_buffer_init(bytes);
		if (!slot->ring)
		{
			divoom_print("audio_buf_pool: alloc fail bus=%u bytes=%u\n", (unsigned)bus, (unsigned)bytes);
			__atomic_add_fetch(&s_lease_fail_cnt, 1, __ATOMIC_RELAXED);
			divoom_audio_buf_pool_unlock();
			return -1;
		}
	}
	slot->leased = 1;
	if (owner_tag)
	{
		strncpy(slot->owner, owner_tag, sizeof(slot->owner) - 1u);
		slot->owner[sizeof(slot->owner) - 1u] = '\0';
	}
	else
	{
		slot->owner[0] = '\0';
	}
	if (slot->ring)
	{
		divoom_ring_buffer_clr(slot->ring);
	}
	divoom_print("audio_buf_pool: acquire bus=%u owner=%s\n",
		     (unsigned)bus, slot->owner);
	divoom_audio_buf_pool_unlock();
	return 0;
}

static void divoom_audio_buf_pool_release_locked(divoom_audio_bus_t bus)
{
	divoom_audio_buf_pool_slot_t *slot;

	if (bus >= DIVOOM_AUDIO_BUS_COUNT)
	{
		return;
	}
	slot = &s_slots[bus];
	if (!slot->leased)
	{
		return;
	}
	if (divoom_audio_buf_pool_is_dynamic(bus))
	{
		divoom_ring_buffer_free(slot->ring);
		slot->ring = NULL;
	}
	else
	{
		divoom_ring_buffer_clr(slot->ring);
	}
	slot->leased = 0;
	slot->owner[0] = '\0';
}

void divoom_audio_buf_pool_release(divoom_audio_bus_t bus, const char *owner_tag)
{
	(void)owner_tag;
	if (bus >= DIVOOM_AUDIO_BUS_COUNT)
	{
		return;
	}
	divoom_audio_buf_pool_lock();
	if (!s_slots[bus].leased)
	{
		divoom_audio_buf_pool_unlock();
		return;
	}
	divoom_print("audio_buf_pool: release bus=%u owner=%s\n",
		     (unsigned)bus, s_slots[bus].owner);
	divoom_audio_buf_pool_release_locked(bus);
	divoom_audio_buf_pool_unlock();
}

void divoom_audio_buf_pool_force_release(divoom_audio_bus_t bus, const char *owner_tag)
{
	(void)owner_tag;
	divoom_audio_buf_pool_release(bus, owner_tag);
}

int divoom_audio_buf_pool_is_leased(divoom_audio_bus_t bus)
{
	int leased;

	if (bus >= DIVOOM_AUDIO_BUS_COUNT)
	{
		return 0;
	}
	divoom_audio_buf_pool_lock();
	leased = s_slots[bus].leased ? 1 : 0;
	divoom_audio_buf_pool_unlock();
	return leased;
}

int divoom_audio_buf_pool_is_owner(divoom_audio_bus_t bus, const char *owner_tag)
{
	int owned;
	if (bus >= DIVOOM_AUDIO_BUS_COUNT || !owner_tag) return 0;
	divoom_audio_buf_pool_lock();
	owned = s_slots[bus].leased && !strcmp(s_slots[bus].owner, owner_tag);
	divoom_audio_buf_pool_unlock();
	return owned;
}

void *divoom_audio_buf_pool_ring(divoom_audio_bus_t bus)
{
	void *ring;

	if (bus >= DIVOOM_AUDIO_BUS_COUNT)
	{
		return NULL;
	}
	divoom_audio_buf_pool_lock();
	ring = s_slots[bus].leased ? s_slots[bus].ring : NULL;
	divoom_audio_buf_pool_unlock();
	return ring;
}

unsigned int divoom_audio_buf_pool_lease_fail_count(void)
{
	return __atomic_load_n(&s_lease_fail_cnt, __ATOMIC_RELAXED);
}

#else

int divoom_audio_buf_pool_init(void) { return 0; }
int divoom_audio_buf_pool_is_owner(divoom_audio_bus_t bus, const char *tag)
{ (void)bus; (void)tag; return 1; }
int divoom_audio_buf_pool_acquire(divoom_audio_bus_t bus, const char *owner_tag)
{
	(void)bus; (void)owner_tag;
	return 0;
}
void divoom_audio_buf_pool_release(divoom_audio_bus_t bus, const char *owner_tag)
{
	(void)bus; (void)owner_tag;
}
void divoom_audio_buf_pool_force_release(divoom_audio_bus_t bus, const char *owner_tag)
{
	(void)bus; (void)owner_tag;
}
int divoom_audio_buf_pool_is_leased(divoom_audio_bus_t bus) { (void)bus; return 0; }
void *divoom_audio_buf_pool_ring(divoom_audio_bus_t bus) { (void)bus; return NULL; }
unsigned int divoom_audio_buf_pool_lease_fail_count(void) { return 0u; }

#endif
