#include "divoom.h"
#include "divoom_main.h"
#include "divoom_onoff.h"
#include "divoom_music_ctrl.h"
#include "divoom_local_movie_play.h"
#include "divoom_white_noise_player.h"
#include "divoom_product_test.h"
#include "divoom_idle_poweroff.h"

const int divoom_idle_poweroff_secs[DIVOOM_IDLE_POWEROFF_OPTION_CNT] = {
    DIVOOM_IDLE_POWEROFF_30MIN_SEC, 3600, 10800, 21600, 43200, 0
};

static uint32 s_idle_sec;

void divoom_idle_poweroff_reset(void)
{
    s_idle_sec = 0;
}

static int divoom_idle_poweroff_is_active(void)
{
    if (divoom_music_ctrl_is_playing())
    {
        return 1;
    }
    if (divoom_local_movie_play_is_playing())
    {
        return 1;
    }
    if (divoom_white_noise_player_is_playing())
    {
        return 1;
    }
    return 0;
}

void divoom_idle_poweroff_tick(void)
{
    Pdivoom_sys_setting_info cfg;
    int limit;

    if (divoom_product_test_is_active())
    {
        return;
    }
    if (divoom_onoff_is_poweroff_active())
    {
        return;
    }

    cfg = divoom_main_get_system_config();
    if (!cfg)
    {
        return;
    }

    limit = cfg->idle_poweroff_time;
    if (limit <= 0)
    {
        s_idle_sec = 0;
        return;
    }

    if (divoom_idle_poweroff_is_active())
    {
        s_idle_sec = 0;
        return;
    }

    s_idle_sec++;
    divoom_print("idle_poweroff: %u / %d\n", (unsigned)s_idle_sec, limit);
    if (s_idle_sec >= (uint32)limit)
    {
        divoom_print("idle_poweroff: timeout, power off\n");
        s_idle_sec = 0;
        divoom_onoff_poweroff_no_tone();
    }
}
