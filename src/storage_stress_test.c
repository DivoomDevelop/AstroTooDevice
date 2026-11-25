#include "storage_stress_test.h"

#if DIVOOM_STORAGE_STRESS_TEST
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <unistd.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "divoom_port_fs_check.h"

#define TEST_DIR "/userdata/TEST"
#define TEST1_DIR "/userdata/TEST1"
#define TEST_MIN_FREE (15ULL * 1024 * 1024)
#define TEST_MAX_FILE (5U * 1024 * 1024)
#define TEST_BUFFER_SIZE 1024U
#define TEST_CYCLE_LIMIT STORAGE_STRESS_CYCLES

static unsigned char test_buffer[2][TEST_BUFFER_SIZE];
static int test_stopped;
static int fill_complete;
static SemaphoreHandle_t writer_start;
static SemaphoreHandle_t writer_done;
static SemaphoreHandle_t writer_first[2];
static int test_tasks;
static uint32_t writer_cycle;
static uint32_t writer_count[2];
static uint32_t status_cycle, status_phase, status_done, status_total, status_free_kib;
static int status_error;

void storage_stress_get_status(storage_stress_status *status)
{
    if (!status) return;
    status->cycle = __atomic_load_n(&status_cycle, __ATOMIC_ACQUIRE);
    status->phase = __atomic_load_n(&status_phase, __ATOMIC_ACQUIRE);
    status->files[0] = __atomic_load_n(&writer_count[0], __ATOMIC_ACQUIRE);
    status->files[1] = __atomic_load_n(&writer_count[1], __ATOMIC_ACQUIRE);
    status->done = __atomic_load_n(&status_done, __ATOMIC_ACQUIRE);
    status->total = __atomic_load_n(&status_total, __ATOMIC_ACQUIRE);
    status->free_kib = __atomic_load_n(&status_free_kib, __ATOMIC_ACQUIRE);
    status->error = __atomic_load_n(&status_error, __ATOMIC_ACQUIRE);
}

static const char *test_dir(unsigned int writer)
{
    return writer ? TEST1_DIR : TEST_DIR;
}

/* Private deterministic PRNG: the reader can regenerate each file without a manifest. */
static uint32_t test_random(uint32_t *state)
{
    *state = *state * 1664525U + 1013904223U;
    return *state;
}

static int test_is_stopped(void)
{
    return __atomic_load_n(&test_stopped, __ATOMIC_ACQUIRE);
}

static void test_stop(void)
{
    __atomic_store_n(&status_phase, STORAGE_STRESS_ERROR, __ATOMIC_RELEASE);
    __atomic_store_n(&test_stopped, 1, __ATOMIC_RELEASE);
}

static void test_task_done(void)
{
    if (__atomic_sub_fetch(&test_tasks, 1, __ATOMIC_ACQ_REL) == 0) {
        vSemaphoreDelete(writer_start);
        vSemaphoreDelete(writer_done);
        vSemaphoreDelete(writer_first[0]);
        vSemaphoreDelete(writer_first[1]);
    }
}

/* Each file operation gets an independent 500-2000 ms interval. */
static int test_wait(uint32_t *state)
{
    TickType_t remaining = pdMS_TO_TICKS(500U + test_random(state) % 1501U);
    TickType_t step = pdMS_TO_TICKS(100);
    if (step == 0) step = 1;
    while (remaining && !test_is_stopped()) {
        TickType_t delay = remaining < step ? remaining : step;
        vTaskDelay(delay);
        remaining -= delay;
    }
    return !test_is_stopped();
}

static void test_wait_for_boot(void)
{
    TickType_t now = xTaskGetTickCount();
    const TickType_t start = pdMS_TO_TICKS(10000);
    if (now < start) vTaskDelay(start - now);
}

static uint32_t test_seed(uint32_t cycle, unsigned int writer, uint32_t index)
{
    return 0x8652a971U ^ (cycle * 2654435761U) ^
           (index * 2246822519U) ^ (writer * 3266489917U);
}

static uint32_t test_file_size(uint32_t cycle, unsigned int writer, uint32_t index)
{
    uint32_t state = test_seed(cycle, writer, index);
    return 12U + test_random(&state) % (TEST_MAX_FILE - 11U);
}

/* The first 12 bytes uniquely identify cycle, writer and file index. */
static unsigned char test_byte(uint32_t *state, uint32_t cycle, unsigned int writer,
                               uint32_t index, uint32_t offset)
{
    unsigned char random_byte = (unsigned char)(test_random(state) >> 24);
    if (offset < 4U) return (unsigned char)(cycle >> (offset * 8U));
    if (offset < 8U) return (unsigned char)((uint32_t)writer >> ((offset - 4U) * 8U));
    if (offset < 12U) return (unsigned char)(index >> ((offset - 8U) * 8U));
    return random_byte;
}

static int test_free_bytes(uint64_t *bytes)
{
    struct statfs fs;
    if (statfs("/userdata", &fs) != 0 || fs.f_bsize <= 0) {
        __atomic_store_n(&status_error, errno ? errno : -1, __ATOMIC_RELEASE);
        printf("[storage_test] statfs failed errno=%d\n", errno);
        return -1;
    }
    *bytes = (uint64_t)fs.f_bavail * (uint64_t)fs.f_bsize;
    __atomic_store_n(&status_free_kib, (uint32_t)(*bytes / 1024U), __ATOMIC_RELEASE);
    return 0;
}

static int test_prepare_dir(const char *path)
{
    DIR *dir;
    struct dirent *entry;
    int empty = 1, saved_error = 0;
    if (mkdir(path, 0777) == 0) return 0;
    if (errno != EEXIST) {
        __atomic_store_n(&status_error, errno, __ATOMIC_RELEASE);
        printf("[storage_test] mkdir %s failed errno=%d\n", path, errno);
        return -1;
    }
    dir = opendir(path);
    if (!dir) {
        __atomic_store_n(&status_error, errno, __ATOMIC_RELEASE);
        printf("[storage_test] opendir %s failed errno=%d\n", path, errno);
        return -1;
    }
    for (;;) {
        errno = 0;
        entry = readdir(dir);
        if (!entry) { saved_error = errno; break; }
        if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
            empty = 0;
            break;
        }
    }
    if (closedir(dir) != 0 || saved_error || !empty) {
        __atomic_store_n(&status_error, !empty ? -1004 : (saved_error ? saved_error : errno), __ATOMIC_RELEASE);
        printf("[storage_test] %s not empty or directory read failed errno=%d; retained\n",
               path, saved_error ? saved_error : errno);
        return -1;
    }
    printf("[storage_test] using existing empty directory %s\n", path);
    return 0;
}

static int test_check_volume(void)
{
    ff_check_result result;
    int rc = FF_CHECK_BUSY, attempt;
    printf("[storage_test] CHECK /userdata FAT and directories begin\n");
    for (attempt = 1; attempt <= 3; ++attempt) {
        memset(&result, 0, sizeof(result));
        rc = divoom_port_fs_check_volume("/dev/UDISK", "/userdata", &result, NULL, 0, NULL, NULL);
        printf("[storage_test] CHECK full attempt=%d result=%d complete=%lu flags=0x%08lx "
               "unsafe=%lu affected=%lu orphans=%lu fs_error=%lu\n",
               attempt, rc, (unsigned long)result.complete, (unsigned long)result.damage_flags,
               (unsigned long)result.unsafe, (unsigned long)result.affected_files,
               (unsigned long)result.orphan_clusters, (unsigned long)result.fs_error);
        if (rc != FF_CHECK_BUSY || test_is_stopped()) break;
        if (attempt < 3) vTaskDelay(pdMS_TO_TICKS(1000));
    }
    if (test_is_stopped() || rc != FF_CHECK_CLEAN || !result.complete || result.damage_flags || result.unsafe) {
        __atomic_store_n(&status_error, rc ? rc : -1002, __ATOMIC_RELEASE);
        test_stop();
        return -1;
    }
    return 0;
}

static int test_file(uint32_t cycle, unsigned int writer, uint32_t index, int verify)
{
    char path[80];
    unsigned char *buffer = test_buffer[writer];
    uint32_t state = test_seed(cycle, writer, index);
    uint32_t size = test_file_size(cycle, writer, index), offset = 0;
    int fd, result = -1, error_code = 0;
    struct stat st;

    snprintf(path, sizeof(path), "%s/%08lu.bin", test_dir(writer), (unsigned long)index);
    printf("[storage_test] %s %s size=%lu\n", verify ? "READ/VERIFY" : "WRITE",
           path, (unsigned long)size);
    fd = open(path, verify ? O_RDONLY : (O_WRONLY | O_CREAT | O_EXCL), 0666);
    if (fd < 0) {
        __atomic_store_n(&status_error, errno ? errno : -1000, __ATOMIC_RELEASE);
        printf("[storage_test] open %s failed errno=%d\n", path, errno);
        return -1;
    }
    if (verify && (fstat(fd, &st) != 0 || st.st_size != size)) {
        error_code = -1001;
        printf("[storage_test] size mismatch/stat failed %s expected=%lu errno=%d\n",
               path, (unsigned long)size, errno);
        goto out;
    }
    while (offset < size) {
        size_t count = size - offset;
        ssize_t done;
        size_t i;
        if (test_is_stopped()) goto out;
        if (count > TEST_BUFFER_SIZE) count = TEST_BUFFER_SIZE;
        if (verify) {
            done = read(fd, buffer, count);
            if (done != (ssize_t)count) {
                error_code = done < 0 && errno ? errno : -1000;
                printf("[storage_test] read %s offset=%lu result=%ld errno=%d\n",
                       path, (unsigned long)offset, (long)done, errno);
                goto out;
            }
            for (i = 0; i < count; ++i) {
                unsigned char expected = test_byte(&state, cycle, writer, index, offset + i);
                if (buffer[i] != expected) {
                    error_code = -1001;
                    printf("[storage_test] mismatch %s offset=%lu expected=%02x got=%02x\n",
                           path, (unsigned long)(offset + i), expected, buffer[i]);
                    goto out;
                }
            }
        } else {
            for (i = 0; i < count; ++i)
                buffer[i] = test_byte(&state, cycle, writer, index, offset + i);
            done = write(fd, buffer, count);
            if (done != (ssize_t)count) {
                error_code = done < 0 && errno ? errno : -1000;
                printf("[storage_test] write %s offset=%lu result=%ld errno=%d\n",
                       path, (unsigned long)offset, (long)done, errno);
                goto out;
            }
        }
        offset += (uint32_t)count;
    }
    if (!verify && fsync(fd) != 0) {
        error_code = errno ? errno : -1000;
        printf("[storage_test] sync %s failed errno=%d\n", path, errno);
        goto out;
    }
    result = 0;
out:
    if (close(fd) != 0) {
        if (!error_code) error_code = errno ? errno : -1000;
        printf("[storage_test] close %s failed errno=%d\n", path, errno);
        result = -1;
    }
    if (error_code) __atomic_store_n(&status_error, error_code, __ATOMIC_RELEASE);
    return result;
}

/* Both writers complete a first file before either can start another. */
static int test_wait_semaphore(SemaphoreHandle_t semaphore)
{
    TickType_t wait = pdMS_TO_TICKS(100);
    if (!wait) wait = 1;
    while (!test_is_stopped()) {
        if (xSemaphoreTake(semaphore, wait) == pdTRUE) return 0;
    }
    return -1;
}

static int test_write_files(unsigned int writer, uint32_t cycle, uint32_t *delay_state)
{
    uint64_t free_bytes;
    uint32_t *count = &writer_count[writer];
    uint32_t current = 0;
    for (;;) {
        if (test_is_stopped()) return -1;
        if (current && __atomic_load_n(&fill_complete, __ATOMIC_ACQUIRE)) return 0;
        if (!test_wait(delay_state)) return -1;
        if (current && __atomic_load_n(&fill_complete, __ATOMIC_ACQUIRE)) return 0;
        if (current == UINT32_MAX || test_file(cycle, writer, current, 0) != 0) return -1;
        ++current;
        __atomic_store_n(count, current, __ATOMIC_RELEASE);
        if (current == 1U) {
            xSemaphoreGive(writer_first[writer]);
            if (test_wait_semaphore(writer_first[1U - writer]) != 0) return -1;
        }
        if (test_free_bytes(&free_bytes) != 0) return -1;
        if (free_bytes < TEST_MIN_FREE) {
            __atomic_store_n(&fill_complete, 1, __ATOMIC_RELEASE);
            return 0;
        }
    }
}

static void storage_writer_task(void *arg)
{
    uint32_t delay_state = 0x45b721e9U;
    (void)arg;
    while (test_wait_semaphore(writer_start) == 0) {
        if (test_write_files(1, writer_cycle, &delay_state) != 0) test_stop();
        xSemaphoreGive(writer_done);
    }
    printf("[storage_test] TEST1 writer stopped\n");
    test_task_done();
    vTaskDelete(NULL);
}

static void storage_stress_task(void *arg)
{
    uint32_t cycle = 0;
    uint32_t delay_state = 0x81c59f13U;
    (void)arg;
    test_wait_for_boot();
    if (test_is_stopped()) goto stopped;
    printf("[storage_test] starting concurrent writers on /userdata\n");
    for (;;) {
        unsigned int writer;
        uint32_t i;
        uint64_t free_bytes;
        char path[80];
        printf("[storage_test] cycle %lu/%u begin\n",
               (unsigned long)(cycle + 1U), TEST_CYCLE_LIMIT);
        __atomic_store_n(&status_cycle, cycle + 1U, __ATOMIC_RELEASE);
        if (test_prepare_dir(TEST_DIR) != 0 || test_prepare_dir(TEST1_DIR) != 0)
            break;
        if (test_free_bytes(&free_bytes) != 0) break;
        /* Reserve space for both first files before entering the fill loop. */
        if (free_bytes < TEST_MIN_FREE + 2ULL * TEST_MAX_FILE) {
            __atomic_store_n(&status_error, -1003, __ATOMIC_RELEASE);
            printf("[storage_test] initial free space insufficient for both writers\n");
            break;
        }
        __atomic_store_n(&writer_count[0], 0, __ATOMIC_RELEASE);
        __atomic_store_n(&writer_count[1], 0, __ATOMIC_RELEASE);
        writer_cycle = cycle;
        __atomic_store_n(&fill_complete, 0, __ATOMIC_RELEASE);
        __atomic_store_n(&status_phase, STORAGE_STRESS_WRITE, __ATOMIC_RELEASE);
        xSemaphoreGive(writer_start);
        if (test_write_files(0, cycle, &delay_state) != 0) {
            test_stop();
            goto stopped;
        }
        /* No file is verified or deleted until both writers have closed their files. */
        if (test_wait_semaphore(writer_done) != 0 || test_is_stopped()) goto stopped;
        if (test_free_bytes(&free_bytes) != 0) goto stopped;
        __atomic_store_n(&status_total, writer_count[0] + writer_count[1], __ATOMIC_RELEASE);
        __atomic_store_n(&status_done, 0, __ATOMIC_RELEASE);
        __atomic_store_n(&status_phase, STORAGE_STRESS_VERIFY, __ATOMIC_RELEASE);
        printf("[storage_test] cycle %lu/%u wrote TEST=%lu TEST1=%lu free=%llu; verifying\n",
               (unsigned long)(cycle + 1U), TEST_CYCLE_LIMIT,
               (unsigned long)writer_count[0], (unsigned long)writer_count[1],
               (unsigned long long)free_bytes);
        for (writer = 0; writer < 2; ++writer) {
            for (i = 0; i < writer_count[writer]; ++i) {
                if (!test_wait(&delay_state) || test_file(cycle, writer, i, 1) != 0)
                    goto stopped;
                __atomic_add_fetch(&status_done, 1, __ATOMIC_RELEASE);
            }
        }
        __atomic_store_n(&status_done, 0, __ATOMIC_RELEASE);
        __atomic_store_n(&status_phase, STORAGE_STRESS_DELETE, __ATOMIC_RELEASE);
        for (writer = 0; writer < 2; ++writer) {
            for (i = 0; i < writer_count[writer]; ++i) {
                if (!test_wait(&delay_state)) goto stopped;
                snprintf(path, sizeof(path), "%s/%08lu.bin", test_dir(writer), (unsigned long)i);
                printf("[storage_test] DELETE %s\n", path);
                if (unlink(path) != 0) {
                    __atomic_store_n(&status_error, errno, __ATOMIC_RELEASE);
                    printf("[storage_test] unlink %s failed errno=%d\n", path, errno);
                    goto stopped;
                }
                __atomic_add_fetch(&status_done, 1, __ATOMIC_RELEASE);
                if (test_free_bytes(&free_bytes) != 0) goto stopped;
            }
            if (rmdir(test_dir(writer)) != 0) {
                __atomic_store_n(&status_error, errno, __ATOMIC_RELEASE);
                printf("[storage_test] rmdir %s failed errno=%d\n", test_dir(writer), errno);
                goto stopped;
            }
        }
        if (test_is_stopped()) goto stopped;
        __atomic_store_n(&status_phase, STORAGE_STRESS_CHECK, __ATOMIC_RELEASE);
        if (test_check_volume() != 0) goto stopped;
        ++cycle;
        printf("[storage_test] cycle %lu/%u passed (full check)\n", (unsigned long)cycle, TEST_CYCLE_LIMIT);
        if (cycle == TEST_CYCLE_LIMIT) {
            __atomic_store_n(&test_stopped, 1, __ATOMIC_RELEASE);
            __atomic_store_n(&status_phase, STORAGE_STRESS_DONE, __ATOMIC_RELEASE);
            printf("[storage_test] completed all %u cycles\n", TEST_CYCLE_LIMIT);
            goto finished;
        }
    }
stopped:
    test_stop();
    printf("[storage_test] stopped after %lu/%u completed cycles; remaining files retained for inspection\n",
           (unsigned long)cycle, TEST_CYCLE_LIMIT);
finished:
    test_task_done();
    vTaskDelete(NULL);
}

void storage_stress_test_start(void)
{
    writer_start = xSemaphoreCreateBinary();
    writer_done = xSemaphoreCreateBinary();
    writer_first[0] = xSemaphoreCreateBinary();
    writer_first[1] = xSemaphoreCreateBinary();
    if (!writer_start || !writer_done || !writer_first[0] || !writer_first[1]) {
        printf("[storage_test] semaphore creation failed\n");
        __atomic_store_n(&status_error, -1005, __ATOMIC_RELEASE);
        test_stop();
        if (writer_start) vSemaphoreDelete(writer_start);
        if (writer_done) vSemaphoreDelete(writer_done);
        if (writer_first[0]) vSemaphoreDelete(writer_first[0]);
        if (writer_first[1]) vSemaphoreDelete(writer_first[1]);
        return;
    }
    __atomic_store_n(&test_tasks, 2, __ATOMIC_RELEASE);
    if (xTaskCreate(storage_stress_task, "storage_test", 2048, NULL,
                    tskIDLE_PRIORITY + 1, NULL) != pdPASS) {
        printf("[storage_test] task creation failed\n");
        test_stop();
        vSemaphoreDelete(writer_start);
        vSemaphoreDelete(writer_done);
        vSemaphoreDelete(writer_first[0]);
        vSemaphoreDelete(writer_first[1]);
        return;
    }
    if (xTaskCreate(storage_writer_task, "storage_writer", 2048, NULL,
                    tskIDLE_PRIORITY + 1, NULL) != pdPASS) {
        printf("[storage_test] second writer task creation failed\n");
        test_stop();
        test_task_done();
    }
}
#endif
