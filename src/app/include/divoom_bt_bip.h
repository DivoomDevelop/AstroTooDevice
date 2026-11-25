#ifndef _DIVOOM_BT_BIP_H_
#define _DIVOOM_BT_BIP_H_

#include "divoom_type.h"

/*
 * 杰理把手机专辑封面经 0x58 分片转发过来。命令线程拼文件，
 * 界面线程用 divoom_bt_bip_copy() 取走一份拷贝。没有完整封面时界面继续画黑胶。
 *
 * 分片状态与杰理 bip_rx_data_handle 一致：
 *   1 开始  2 中间  3 结束  4 失败  5 重复获取  6 这首没有图
 */

enum {
	DIVOOM_BT_BIP_UI_UNCHANGED = 0, /* 和 have_gen 是同一代，调用方不用改界面 */
	DIVOOM_BT_BIP_UI_VINYL = 1,     /* 回到默认黑胶 */
	DIVOOM_BT_BIP_UI_JPEG = 2       /* *out 是调用方持有的 JPG，用完 divoom_system_free */
};

/* 启动时调用一次，把锁建好，避免收包和断开同时第一次建锁 */
void divoom_bt_bip_init(void);

/* 命令线程。payload = status(1) + body_len(2, 小端) + jpg 分片 */
void divoom_bt_bip_on_packet(const uint8_t *data, uint16_t len);

/* 蓝牙断开。文件状态和发布区共用一把锁，可以不在命令线程调用 */
void divoom_bt_bip_on_bt_down(void);

/* 仅蓝牙播放界面存封面。关掉时删掉临时文件和已发布的图 */
void divoom_bt_bip_set_store(int enable);

/*
 * 界面线程。have_gen 是上次已经处理过的一代。
 * 返回 JPEG 时 *out 为新分配的缓冲区，调用方负责释放。
 * 锁只包住指针交换和拷贝，不包住解码。
 */
int divoom_bt_bip_copy(uint32_t have_gen, uint8_t **out, uint32_t *out_len, uint32_t *out_gen);

#endif
