#ifndef _divoom_random_h_
#define _divoom_random_h_
void *divoom_random_create(int max);
/* 每轮各项只选一次；数量大于 1 时跨轮也不会连续返回同一项。 */
int divoom_random_get(void *handle);
void divoom_random_destroy(void *handle);
int divoom_random_get_no_use(void *handle);
/* set_max 超出容量时扩容，句柄地址不变；新增范围标记为未选。
 * 返回 1 成功、0 参数无效或分配失败；失败时保留原状态。
 * set_max 保留前缀状态；insert/remove 同步移动下标对应的已选状态。
 * insert 仍要求有预留容量，超出容量时返回 0。
 * 调用方负责串行化；有效数量为 0 时 get 返回 -1。 */
int divoom_random_set_max(void *handle, int max);
int divoom_random_insert(void *handle, int index);
int divoom_random_remove(void *handle, int index);

#endif


