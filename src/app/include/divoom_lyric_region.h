#ifndef DIVOOM_LYRIC_REGION_H
#define DIVOOM_LYRIC_REGION_H

#include <string.h>

/* Mainland-only test override: 1 enables lrc.cx/LRCLIB; restore to 0 after testing. */
#ifndef DIVOOM_LYRIC_CN_TEST
#define DIVOOM_LYRIC_CN_TEST 1
#endif
typedef enum {
    DIVOOM_LYRIC_SOURCE_OFF = 0,
    DIVOOM_LYRIC_SOURCE_MUSIXMATCH = 1,
    DIVOOM_LYRIC_SOURCE_LEGACY = 2
} divoom_lyric_source;

/* IP country only: do not infer licensing from UI language or server selection. */
static inline int divoom_lyric_source_for_country(const char *code)
{
    static const char *const mxm[] = {
        "JP", "US", "CA", "GB", "IE", "FR", "DE", "IT", "ES", "PT", "NL",
        "BE", "LU", "AT", "CH", "LI", "DK", "SE", "NO", "FI", "IS",
        "EE", "LV", "LT", "PL", "CZ", "SK", "HU", "SI", "HR", "BA",
        "RS", "ME", "MK", "AL", "GR", "BG", "RO", "MD", "UA", "BY",
        "RU", "MT", "CY", "AD", "MC", "SM", "VA", "XK", "TR"
    };
    unsigned int i;
    if (!code || strlen(code) != 2 || code[0] < 'A' || code[0] > 'Z' ||
        code[1] < 'A' || code[1] > 'Z' ||
        !strcmp(code, "XX") || !strcmp(code, "ZZ"))
        return DIVOOM_LYRIC_SOURCE_OFF;
    if (!strcmp(code, "CN"))
        return DIVOOM_LYRIC_CN_TEST ? DIVOOM_LYRIC_SOURCE_LEGACY : DIVOOM_LYRIC_SOURCE_OFF;
    for (i = 0; i < sizeof(mxm) / sizeof(mxm[0]); ++i)
        if (!strcmp(code, mxm[i])) return DIVOOM_LYRIC_SOURCE_MUSIXMATCH;
    return DIVOOM_LYRIC_SOURCE_LEGACY;
}
int divoom_main_net_lyric_source(void);
void divoom_bt_music_lyric_region_changed(void);
#endif
