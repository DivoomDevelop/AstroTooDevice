#ifndef DIVOOM_PRODUCT_TEST_REPAIR_H
#define DIVOOM_PRODUCT_TEST_REPAIR_H
#include "divoom_update_system_file.h"
#include "divoom_product_test_format.h"
#define DIVOOM_PRODUCT_TEST_REPAIR_TRANSIENT 3
#define DIVOOM_PRODUCT_TEST_REPAIR_INCOMPLETE 4 /* Deleted targets unavailable on server. */
/* 1 complete, 2 requires format, 3 temporary resource pressure (retry without
 * formatting), 4 complete with unavailable targets, 0 failed (intent retained). */
int divoom_product_test_repair_run(int (*network)(void), int (*retry)(int),
    divoom_system_file_restore_progress progress);
/* Read-only CRC preview. 0 means retry the preview. */
int divoom_product_test_repair_preview(divoom_product_test_repair_plan *plan,
    divoom_system_file_restore_progress progress);
int divoom_product_test_repair_backup_ledger(void);
int divoom_product_test_repair_restore_ledger(void);
/* Count the persisted repair list; -1 if unavailable or invalid. */
int divoom_product_test_repair_error_count(void);
int divoom_product_test_repair_cleanup(void);
/* Last explicitly located persistent local I/O failure: -1 none, 0 userdata, 1 usr. */
int divoom_product_test_repair_io_error_target(int clear);
void divoom_product_test_repair_record_io(const char *path, const char *operation, int error);
#endif
