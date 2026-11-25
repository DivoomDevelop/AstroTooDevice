#ifndef _divoom_store_clock_prefetch_h_
#define _divoom_store_clock_prefetch_h_

#include "divoom_device_json.h"

/*
 * 表盘商店（Channel/StoreClockGetClassify · StoreClockGetList）后台预取与 UI 触发入口。
 *
 * 流程概要：
 *   1) divoom_main 首次连上 MQTT 后调用 on_server_connected → 拉分类；
 *   2) Net 线程收到应答后在 divoom_net_app_callback 里调 on_classify_response / on_list_response；
 *   3) 预览图走 divoom_manage_image（DIVOOM_MANAGE_IMAGE_CLOCK_DIAL_REVIEW_PIC），不写业务数据库；
 *   4) checkpoint + SD 上 MSKV1（样例）与 MFL1（各分类完整列表），命中签名与 TTL 时起机可少发 GetList；详见 .c；
 */

/*
 * 与 divoom_store_clock_market_data（见 divoom_store_clock_market_data.h / divoom_store_clock_market_data.c）分工：
 *
 *   · divoom_store_clock_prefetch（本模块 .c）——「网侧 + 磁盘」：发 GetClassify / GetList，
 *     解析列表后把每条表盘的预览任务交给 divoom_manage_image 下载；维护预取阶段（quick/full）、
 *     pending 插队、in-flight、以及 DIVOOM_STORE_CLOCK_PREFETCH_SAVE_PATH 的去重 checkpoint。
 *     不负责给 UI 存展示结构体，但会在适当时机 **调用** market_data 的接口把网络结果「灌」进去。
 *
 *   · divoom_store_clock_market_data ——「UI 用分类 + 样例 + full 列表」：
 *     set_from_classify / merge / apply_list_chunk；样例与完整列表可本地持久化（MSKV1 / MFL1），
 *     详见 divoom_store_clock_market_data.h。无网络时的 clear 由 on_net_lost 触发。
 *
 * 若只关心「分类摘要格子画什么」：读 market_data 头文件与表盘市场菜单；若只关心「何时发列表、何时入队预览」：读本模块 .c。
 */

/* 已连上 Divoom 服务后触发一次：请求 StoreClockGetClassify（若上层已在 divoom_main 里调用则无需重复调） */
void divoom_store_clock_prefetch_on_server_connected(void);

/* MQTT/HTTP 解包得到分类列表后由 net_app 调用：拷贝 classify_id，按策略启动首轮抽样或跳过自动列表拉取 */
void divoom_store_clock_prefetch_on_classify_response(Pdivoom_device_store_clock_classify_list pclassify);

/* StoreClockGetList 解包成功后由 net_app 调用：按当前阶段处理 ClockList，并入队预览下载 */
void divoom_store_clock_prefetch_on_list_response(Pdivoom_device_store_clock_list plist);

/* 断网或 MQTT 断开时清空内存状态（不删 checkpoint 文件） */
void divoom_store_clock_prefetch_on_net_lost(void);

/*
 * 表盘市场 UI：用户点到某一分类时调用。
 * 若仍在「每分类先拉前几条」的首轮阶段，会优先在该分类上做抽样；
 * full 阶段从第 1 页拉表；若有本地持久化列表且首页 id 前缀一致则从列表末尾后继续分页抓取更新。
 */
void divoom_store_clock_prefetch_prioritize_classify(int classify_id);

/*
 * 删除本地 checkpoint，下次联网会重新跑完整预取（下拉刷新、清缓存等）。
 * 路径见 divoom.h 中 DIVOOM_STORE_CLOCK_PREFETCH_SAVE_PATH。
 */
void divoom_store_clock_prefetch_clear_checkpoint(void);

#endif
