#ifndef DIVOOM_CRASH_LOG_H
#define DIVOOM_CRASH_LOG_H
#include <stdint.h>
#include <stddef.h>
#define DIVOOM_CRASH_UPLOAD_PATH "/userdata/crash/upload.bin"

#ifdef CONFIG_DIVOOM_CRASH_LOG
struct divoom_crash_task_info {
    uintptr_t handle, stack_low, stack_high, saved_sp;
    uint32_t priority, number;
    char name[32];
};
/* Lock-free kernel snapshot, callable with scheduling/interrupts disabled. */
int divoom_crash_task_snapshot(void *task, struct divoom_crash_task_info *info);
void divoom_crash_log_overflow_task(void *task);
int divoom_crash_log_init(void);
int divoom_crash_log_export(void);
int divoom_crash_log_available(void);
int divoom_crash_log_record_info(const void *record, size_t size, uint32_t *version);
/* Task context, serialized by caller: 1=ready, 0=none, -1=retained for retry. */
int divoom_crash_upload_prepare(uint32_t *version);
int divoom_crash_upload_finish(void);
/* Panic-only: interrupts must remain disabled until reboot. */
int divoom_crash_log_begin(void);
/* Software fatal path; caller has disabled interrupts and armed the watchdog. */
int divoom_crash_log_capture_fatal(const char *reason, const char *file,
                                   unsigned int line, unsigned long pc,
                                   unsigned long sp);
void divoom_crash_log_registers(const void *regs, unsigned long cause,
                                unsigned long pc, unsigned long value);
int divoom_crash_log_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void divoom_crash_log_stack(unsigned long sp);
void divoom_crash_log_task(unsigned long sp);
void divoom_crash_log_backtrace(unsigned long status, unsigned long sp,
                                unsigned long pc, unsigned long ra);
int divoom_crash_log_flush(void);
void divoom_crash_log_print(void);
#else
static inline int divoom_crash_log_init(void) { return 0; }
static inline int divoom_crash_log_export(void) { return 0; }
#endif

#endif
