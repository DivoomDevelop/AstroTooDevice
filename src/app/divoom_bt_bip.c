#include "divoom.h"
#include "divoom_bt_bip.h"
#include "divoom_system.h"

#include <stdio.h>
#include <string.h>

/* 网易云实测一张封面约 22KB。上限只用来拒掉异常大包，不会按这个长度预分配 */
#define DIVOOM_BT_BIP_MAX_BYTES  (128u * 1024u)
/* 设备没有 /tmp，临时文件放 /userdata/tmp，走 divoom_port_file_* */
#define DIVOOM_BT_BIP_PART_PATH  DIVOOM_TMP_DIR "/bt_bip_cover.part"
#define DIVOOM_BT_BIP_READY_PATH DIVOOM_TMP_DIR "/bt_bip_cover_%u.jpg"

#define DIVOOM_BT_BIP_START    0x01u
#define DIVOOM_BT_BIP_CONTINUE 0x02u
#define DIVOOM_BT_BIP_STOP     0x03u
#define DIVOOM_BT_BIP_ERR      0x04u
#define DIVOOM_BT_BIP_ERR_GET  0x05u
#define DIVOOM_BT_BIP_GET_NULL 0x06u

/*
 * 文件状态和发布区都由 s_lock 保护。断开回调可能不在命令线程。
 * 界面线程只在 copy 里短暂持锁，解码在锁外。
 * 新的开始包把 s_gen 加一，后到的旧分片因为 s_open 已关而丢掉。
 * 已经显示的封面先留着。同一张专辑杰理不会再发图，切歌不能清掉。
 * 下一张收齐后才换上。
 */
static void *s_fp;
static uint32_t s_got;
static uint32_t s_gen;
static int s_open;
/* 只有蓝牙播放界面为 1。其他界面不写文件，也不留已发布的图 */
static int s_store;

static void *s_lock;
static uint8_t *s_jpeg;
static uint32_t s_jpeg_len;
static uint32_t s_jpeg_gen;
static int s_show;
/* 界面看到的状态序号。开始收图和收齐各加一次，不能跟 s_gen 共用 */
static uint32_t s_view_seq;

void divoom_bt_bip_init(void)
{
	if (!s_lock)
		s_lock = divoom_system_create_lock();
}

static int bip_lock(void)
{
	if (!s_lock)
		divoom_bt_bip_init();
	if (!s_lock)
		return 0;
	divoom_system_get_lock(s_lock);
	return 1;
}

static void bip_unlock(void)
{
	divoom_system_free_lock(s_lock);
}

static void bip_close_part(void)
{
	if (s_fp)
	{
		divoom_port_file_close(s_fp);
		s_fp = NULL;
	}
}

static void bip_discard_part(void)
{
	bip_close_part();
	divoom_port_remove_file(DIVOOM_BT_BIP_PART_PATH);
	s_got = 0;
	s_open = 0;
}

/* 调用方已持锁。清发布区，旧 JPG 交给调用方在锁外释放 */
static uint8_t *bip_take_published_locked(void)
{
	uint8_t *old;

	old = s_jpeg;
	s_jpeg = NULL;
	s_jpeg_len = 0;
	s_show = 0;
	s_jpeg_gen = s_gen;
	s_view_seq++;
	if (s_view_seq == 0u)
		s_view_seq = 1u;
	return old;
}

static int bip_append_locked(const uint8_t *body, uint16_t body_len)
{
	if (!s_open || !s_fp)
		return 0;
	if (body_len == 0)
		return 1;
	if (s_got + body_len > DIVOOM_BT_BIP_MAX_BYTES)
		return 0;
	if (divoom_port_file_write(body, 1, body_len, s_fp) != body_len)
		return 0;
	s_got += body_len;
	return 1;
}

static void bip_fail(void)
{
	uint8_t *old;

	if (!bip_lock())
		return;
	bip_discard_part();
	old = bip_take_published_locked();
	bip_unlock();
	if (old)
		divoom_system_free(old);
}

/* 只清这一代。读文件期间若已开始下一张，不能把新封面清掉 */
static void bip_fail_if_gen(uint32_t gen)
{
	uint8_t *old;

	if (!bip_lock())
		return;
	if (s_gen != gen)
	{
		bip_unlock();
		return;
	}
	bip_discard_part();
	old = bip_take_published_locked();
	bip_unlock();
	if (old)
		divoom_system_free(old);
}

/*
 * 结束包把临时文件换成带代数的文件名，锁外再读入。
 * 读的时候如果又来了开始包，代数对不上，这份缓冲丢掉。
 */
static void bip_load_ready(uint32_t gen)
{
	char path[96];
	void *fp;
	long sz;
	uint32_t n;
	uint8_t *buf;
	uint8_t *old;

	snprintf(path, sizeof(path), DIVOOM_BT_BIP_READY_PATH, (unsigned)gen);
	fp = divoom_port_file_open(path, "rb");
	if (!fp)
	{
		bip_fail_if_gen(gen);
		return;
	}
	if (divoom_port_file_seek(fp, 0, DIVOOM_PORT_SEEK_END) != 0)
	{
		divoom_port_file_close(fp);
		divoom_port_remove_file(path);
		bip_fail_if_gen(gen);
		return;
	}
	sz = divoom_port_file_tell(fp);
	if (sz <= 0 || (uint32_t)sz > DIVOOM_BT_BIP_MAX_BYTES
		|| divoom_port_file_seek(fp, 0, DIVOOM_PORT_SEEK_SET) != 0)
	{
		divoom_port_file_close(fp);
		divoom_port_remove_file(path);
		bip_fail_if_gen(gen);
		return;
	}
	n = (uint32_t)sz;
	buf = (uint8_t *)divoom_system_malloc(n);
	if (!buf || divoom_port_file_read(buf, 1, n, fp) != n)
	{
		divoom_port_file_close(fp);
		divoom_port_remove_file(path);
		if (buf)
			divoom_system_free(buf);
		bip_fail_if_gen(gen);
		return;
	}
	divoom_port_file_close(fp);
	divoom_port_remove_file(path);

	if (!bip_lock())
	{
		divoom_system_free(buf);
		return;
	}
	/* 读文件期间又开始了下一张，这张不能再盖上去 */
	if (s_gen != gen)
	{
		bip_unlock();
		divoom_system_free(buf);
		return;
	}
	old = s_jpeg;
	s_jpeg = buf;
	s_jpeg_len = n;
	s_jpeg_gen = gen;
	s_show = 1;
	s_view_seq++;
	if (s_view_seq == 0u)
		s_view_seq = 1u;
	bip_unlock();
	if (old)
		divoom_system_free(old);
}

void divoom_bt_bip_on_bt_down(void)
{
	bip_fail();
}

void divoom_bt_bip_set_store(int enable)
{
	uint8_t *old;

	if (!bip_lock())
		return;
	s_store = enable ? 1 : 0;
	if (s_store)
	{
		bip_unlock();
		return;
	}
	bip_discard_part();
	old = bip_take_published_locked();
	bip_unlock();
	if (old)
		divoom_system_free(old);
}

void divoom_bt_bip_on_packet(const uint8_t *data, uint16_t len)
{
	uint8_t status;
	uint16_t body_len;
	uint32_t gen;
	const uint8_t *body;
	uint8_t *old;
	char path[64];

	if (!data || len < 3u)
		return;
	/* 不在蓝牙播放界面：丢掉这一包，不写临时文件 */
	if (!s_store)
		return;

	status = data[0];
	body_len = (uint16_t)data[1] | ((uint16_t)data[2] << 8);
	if ((uint32_t)body_len + 3u > len)
	{
		bip_fail();
		return;
	}
	body = data + 3;

	/* 重复获取：手机还在发上一张，封面保持 */
	if (status == DIVOOM_BT_BIP_ERR_GET)
		return;

	if (status == DIVOOM_BT_BIP_ERR || status == DIVOOM_BT_BIP_GET_NULL)
	{
		bip_fail();
		return;
	}

	if (!bip_lock())
		return;
	if (!s_store)
	{
		bip_unlock();
		return;
	}

	if (status == DIVOOM_BT_BIP_START)
	{
		s_gen++;
		if (s_gen == 0u)
			s_gen = 1u;
		/* 只丢掉还没拼完的临时文件。已经在显示的封面留到这一张收齐 */
		bip_discard_part();
		old = NULL;
		divoom_port_mkdir_p(DIVOOM_TMP_DIR);
		s_fp = divoom_port_file_open(DIVOOM_BT_BIP_PART_PATH, "wb");
		if (!s_fp)
		{
			bip_unlock();
			if (old)
				divoom_system_free(old);
			return;
		}
		s_open = 1;
		if (!bip_append_locked(body, body_len))
		{
			bip_discard_part();
			bip_unlock();
			if (old)
				divoom_system_free(old);
			return;
		}
		bip_unlock();
		if (old)
			divoom_system_free(old);
		return;
	}

	if (status == DIVOOM_BT_BIP_CONTINUE)
	{
		if (!s_open)
		{
			bip_unlock();
			return;
		}
		if (!bip_append_locked(body, body_len))
		{
			bip_discard_part();
			old = bip_take_published_locked();
			bip_unlock();
			if (old)
				divoom_system_free(old);
			return;
		}
		bip_unlock();
		return;
	}

	if (status != DIVOOM_BT_BIP_STOP || !s_open)
	{
		bip_unlock();
		return;
	}
	if (!bip_append_locked(body, body_len))
	{
		bip_discard_part();
		old = bip_take_published_locked();
		bip_unlock();
		if (old)
			divoom_system_free(old);
		return;
	}
	gen = s_gen;
	bip_close_part();
	s_open = 0;
	snprintf(path, sizeof(path), DIVOOM_BT_BIP_READY_PATH, (unsigned)gen);
	if (divoom_port_rename_file(DIVOOM_BT_BIP_PART_PATH, path) != 0)
	{
		bip_discard_part();
		old = bip_take_published_locked();
		bip_unlock();
		if (old)
			divoom_system_free(old);
		return;
	}
	bip_unlock();
	bip_load_ready(gen);
}

int divoom_bt_bip_copy(uint32_t have_gen, uint8_t **out, uint32_t *out_len, uint32_t *out_gen)
{
	uint8_t *copy;
	uint8_t *src;
	uint32_t n;
	uint32_t gen;
	int show;

	if (out)
		*out = NULL;
	if (out_len)
		*out_len = 0;
	if (!out || !out_len || !out_gen)
		return DIVOOM_BT_BIP_UI_UNCHANGED;

	/* 还没有任何封面包。页面初始就是黑胶 */
	if (!s_lock)
	{
		*out_gen = 0;
		return (have_gen == 0u) ? DIVOOM_BT_BIP_UI_UNCHANGED : DIVOOM_BT_BIP_UI_VINYL;
	}

	if (!bip_lock())
		return DIVOOM_BT_BIP_UI_UNCHANGED;

	show = s_show;
	gen = s_view_seq;
	n = s_jpeg_len;
	src = s_jpeg;
	bip_unlock();

	if (!show)
	{
		*out_gen = gen;
		return (have_gen != gen) ? DIVOOM_BT_BIP_UI_VINYL : DIVOOM_BT_BIP_UI_UNCHANGED;
	}
	if (have_gen == gen || !src || n == 0u)
	{
		*out_gen = gen;
		return DIVOOM_BT_BIP_UI_UNCHANGED;
	}

	/* 先分配再加锁拷贝，避免持锁时进堆分配器和命令线程互等 */
	copy = (uint8_t *)divoom_system_malloc(n);
	if (!copy)
		return DIVOOM_BT_BIP_UI_UNCHANGED;

	if (!bip_lock())
	{
		divoom_system_free(copy);
		return DIVOOM_BT_BIP_UI_UNCHANGED;
	}
	if (!s_show || s_jpeg != src || s_view_seq != gen || s_jpeg_len != n)
	{
		bip_unlock();
		divoom_system_free(copy);
		return DIVOOM_BT_BIP_UI_UNCHANGED;
	}
	memcpy(copy, src, n);
	bip_unlock();

	*out = copy;
	*out_len = n;
	*out_gen = gen;
	return DIVOOM_BT_BIP_UI_JPEG;
}
