#ifndef MBEDTLS_FREERTOS_LIST_H
#define MBEDTLS_FREERTOS_LIST_H

#include <stddef.h>

struct list_head {
  struct list_head *next;
  struct list_head *prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }
#define LIST_HEAD_DEF(name) struct list_head name = LIST_HEAD_INIT(name)

static inline void INIT_LIST_HEAD(struct list_head *head)
{
  head->next = head;
  head->prev = head;
}

static inline void list_add(struct list_head *entry, struct list_head *head)
{
  entry->next = head->next;
  entry->prev = head;
  head->next->prev = entry;
  head->next = entry;
}

static inline void list_del(struct list_head *entry)
{
  entry->next->prev = entry->prev;
  entry->prev->next = entry->next;
  entry->next = entry;
  entry->prev = entry;
}

#define container_of(ptr, type, member) \
  ((type *)((char *)(ptr) - offsetof(type, member)))
#define list_entry(ptr, type, member) container_of(ptr, type, member)
#define list_first_entry(ptr, type, member) list_entry((ptr)->next, type, member)
#define list_next_entry(pos, member) \
  list_entry((pos)->member.next, __typeof__(*(pos)), member)
#define list_for_each_entry(pos, head, member) \
  for (pos = list_first_entry(head, __typeof__(*(pos)), member); \
       &pos->member != (head); \
       pos = list_next_entry(pos, member))
#define list_for_each_entry_safe(pos, next, head, member) \
  for (pos = list_first_entry(head, __typeof__(*(pos)), member), \
       next = list_next_entry(pos, member); \
       &pos->member != (head); \
       pos = next, next = list_next_entry(next, member))

#endif
