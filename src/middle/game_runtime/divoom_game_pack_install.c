#include "divoom.h"
#include "divoom_game_runtime.h"

int divoom_game_pack_install_from_zip(const char *zip_path)
{
    (void)zip_path;
    return -1;
}

int divoom_game_pack_install_request_url(const char *url, const char *dest_zip_path)
{
    (void)url;
    (void)dest_zip_path;
    return -1;
}

int divoom_game_pack_install_pending(void)
{
    return 0;
}

int divoom_game_pack_install_poll(void)
{
    return 0;
}

int divoom_game_pack_install_from_local_manifest(void)
{
    return -1;
}
