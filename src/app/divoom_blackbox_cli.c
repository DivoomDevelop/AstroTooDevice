/* Read-only observations for host-driven black-box tests; never changes state. */
#include "divoom.h"
#ifdef DIVOOM_BLACKBOX_TEST
#include <console.h>
#include <stdio.h>
#include <string.h>
#include "divoom_light_app.h"
#include "divoom_menu_control.h"
#include "divoom_port_fs.h"

volatile int divoom_blackbox_displayed_clock_id;

static int bbtest_write_ids(char *out, const char *name, const int *ids, int count)
{
    int i, used = sprintf(out, "\"%s\":[", name);
    for(i = 0; i < count; i++)
        used += sprintf(out + used, "%s%d", i ? "," : "", ids[i]);
    used += sprintf(out + used, "]");
    return used;
}

static int bbtest_main(int argc, char **argv)
{
    if(argc == 2 && strcmp(argv[1], "state") == 0)
    {
        uint32 finished = 0, total = 0;
        uint8 known = 0;
        int ready = divoom_light_app_copy_my_clock_list_num() > 0;
        int downloading = divoom_light_app_get_clock_download_progress(&finished, &total, &known);
        printf("[BBTEST] STATE {\"ready\":%d,\"clock\":%d,\"displayed_clock\":%d,\"menu\":%d,\"downloading\":%d,"
               "\"finished\":%lu,\"total\":%lu,\"total_known\":%u,"
               "\"free_kb\":%ld,\"largest_kb\":%ld,\"storage_kb\":%lu}\n",
               ready, ready ? divoom_light_app_get_cur_clock_id() : 0,
               divoom_blackbox_displayed_clock_id, divoom_menu_control_get_active_menu_id(),
               downloading, (unsigned long)finished, (unsigned long)total, (unsigned)known,
               divoom_system_get_ram_free_size(), divoom_system_get_ram_largest_free_block_size(),
               (unsigned long)divoom_port_fs_free_space(DIVOOM_USER_ROOT));
        return 0;
    }
    if(argc == 2 && strcmp(argv[1], "clocks") == 0)
    {
        int *ids = divoom_system_malloc(3 * DIVOOM_MAX_CLOCK_NUM * sizeof(int));
        /* 12 bytes cover each signed int and its comma; lists are bounded. */
        char *json = divoom_system_malloc(3 * DIVOOM_MAX_CLOCK_NUM * 12 + 256);
        int n, recent, favorite, actual, used;
        if(ids == NULL || json == NULL)
        {
            if(ids) divoom_system_free(ids);
            if(json) divoom_system_free(json);
            printf("[BBTEST] ERROR allocation failed\n");
            return -1;
        }
        /* copy_*_limited do not lock: take one consistent list snapshot. */
        divoom_light_app_clock_list_lock();
        actual = divoom_light_app_copy_my_clock_list_num();
        n = divoom_light_app_copy_my_clock_list_limited(ids, DIVOOM_MAX_CLOCK_NUM);
        recent = divoom_light_app_copy_recent_clock_list_limited(ids + DIVOOM_MAX_CLOCK_NUM, DIVOOM_MAX_CLOCK_NUM);
        favorite = divoom_light_app_copy_fav_clock_list_limited(ids + 2 * DIVOOM_MAX_CLOCK_NUM, DIVOOM_MAX_CLOCK_NUM);
        divoom_light_app_clock_list_unlock();
        /* Printing may block on UART; no business lock is held. */
        used = sprintf(json, "{\"count\":%d,\"truncated\":%d,", actual, actual > n);
        used += bbtest_write_ids(json + used, "my", ids, n);
        used += sprintf(json + used, ",");
        used += bbtest_write_ids(json + used, "recent", ids + DIVOOM_MAX_CLOCK_NUM, recent);
        used += sprintf(json + used, ",");
        used += bbtest_write_ids(json + used, "favorite", ids + 2 * DIVOOM_MAX_CLOCK_NUM, favorite);
        sprintf(json + used, "}");
        printf("[BBTEST] CLOCKS %s\n", json);
        divoom_system_free(json);
        divoom_system_free(ids);
        return 0;
    }
    printf("bbtest state | clocks (read-only)\n");
    return -1;
}

FINSH_FUNCTION_EXPORT_CMD(bbtest_main, bbtest, read-only black-box observations);
#endif
