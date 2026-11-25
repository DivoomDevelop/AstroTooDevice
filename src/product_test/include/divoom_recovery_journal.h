#ifndef DIVOOM_RECOVERY_JOURNAL_H
#define DIVOOM_RECOVERY_JOURNAL_H
#include <stdint.h>
#include <string.h>
#include "ffcheck.h"
#define DIVOOM_RECOVERY_PLAN_MAX 240U
#define DIVOOM_RECOVERY_WIFI_MAX 4096U
#define DIVOOM_RECOVERY_SYSTEM_CONFIG_MAX 8192U
#define DIVOOM_RECOVERY_FORMAT_APPROVAL(usr) (0x1000U << (usr))
#define DIVOOM_RECOVERY_MENU_CHECK 0x4000U
#define DIVOOM_RECOVERY_SYSTEM_CONFIG 0x8000U
enum { DIVOOM_RECOVERY_SCAN=1, DIVOOM_RECOVERY_DETACH=2, DIVOOM_RECOVERY_FREE=3,
 DIVOOM_RECOVERY_VERIFY=4, DIVOOM_RECOVERY_FORMAT=5, DIVOOM_RECOVERY_RESTORE=6,
 DIVOOM_RECOVERY_COMPLETE=7, DIVOOM_RECOVERY_HARD_FAULT=8,
 DIVOOM_RECOVERY_ORPHANS=9 };
typedef struct {
 uint32_t pending_mask, target_usr, phase, format_mask, plan_count, wifi_size, flags, reserved;
 ff_check_geometry geometry;
 union {
  ff_check_entry plan[DIVOOM_RECOVERY_PLAN_MAX];
  /* The deletion plan is retired before a user-volume format is committed. */
  struct {
   uint32_t size;
   char json[DIVOOM_RECOVERY_SYSTEM_CONFIG_MAX];
  } system_config;
 };
 char wifi[DIVOOM_RECOVERY_WIFI_MAX];
} divoom_recovery_record;
static inline void divoom_recovery_record_clear_system_config(divoom_recovery_record *record)
{
 if (record->flags & DIVOOM_RECOVERY_SYSTEM_CONFIG) {
  if (!record->plan_count) memset(&record->system_config, 0, sizeof(record->system_config));
  record->flags &= ~DIVOOM_RECOVERY_SYSTEM_CONFIG;
 }
}
/* Called before mounting FAT or starting any business writer. */
int divoom_recovery_journal_boot(void);
int divoom_recovery_journal_pending(void);
/* Retry a failed journal read without publishing a new transaction. */
int divoom_recovery_journal_retry_read(void);
const divoom_recovery_record *divoom_recovery_journal_get(void);
int divoom_recovery_journal_save(const divoom_recovery_record *record);
int divoom_recovery_journal_clear(void);
/* Platform callback addresses only the independent 32KiB recovery region. */
int divoom_recovery_storage_io(int write_data, uint32_t offset, void *data, uint32_t bytes);
#endif
