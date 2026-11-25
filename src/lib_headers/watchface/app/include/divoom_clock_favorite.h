#ifndef _divoom_clock_favorite_h_
#define _divoom_clock_favorite_h_

/** 表盘收藏（与「我的表盘」列表独立）：持久化于 SD system 目录 JSON */

void divoom_clock_favorite_init(void);

/** @return 1 已收藏 */
int divoom_clock_favorite_is(int clock_id);

/** 加入或移除收藏。@return 0 成功，1 失败（参数非法或已满） */
int divoom_clock_favorite_toggle(int clock_id);

int divoom_clock_favorite_get_count(void);

/** 收藏列表第 index 项 clock_id；越界返回 0 */
int divoom_clock_favorite_get_at(int index);

/** 若 clock_id 在收藏列表中则移除并落盘；不在则无操作 */
void divoom_clock_favorite_remove_if_present(int clock_id);

#endif
