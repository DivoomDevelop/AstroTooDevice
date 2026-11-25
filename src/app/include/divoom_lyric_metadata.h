#ifndef DIVOOM_LYRIC_METADATA_H
#define DIVOOM_LYRIC_METADATA_H
#include <stddef.h>
#include <string.h>

/* Filter device playback status only in the artist field, never lyric text.
 * Match whole status phrases; an artist merely mentioning iPhone remains valid. */
static inline int divoom_lyric_artist_is_status(const char *artist)
{
    char text[256];
    size_t n = 0, i, w, len;
    static const char *const endings[] = {
        "上收听", "上收聽", "上聆听", "上聆聽", "上播放"
    };
    if (!artist) return 0;
    for (; *artist; ++artist)
    {
        unsigned char ch = (unsigned char)*artist;
        if (ch == ' ' || (ch >= '\t' && ch <= '\r'))
        {
            if (n && text[n - 1] != ' ')
            {
                if (n + 1 >= sizeof(text)) return 0;
                text[n++] = ' ';
            }
            continue;
        }
        if (n + 1 >= sizeof(text)) return 0;
        text[n++] = ch >= 'A' && ch <= 'Z' ? ch - 'A' + 'a' : ch;
    }
    if (n && text[n - 1] == ' ') --n;
    text[n] = '\0';
    if (!strcmp(text, "not playing") ||
        (!strncmp(text, "listening on ", 13) && n > 13) ||
        (!strncmp(text, "playing on ", 11) && n > 11))
        return 1;
    /* Chinese device labels may arrive with or without spaces. */
    for (i = w = 0; i < n; ++i)
        if (text[i] != ' ') text[w++] = text[i];
    text[w] = '\0';
    if (!strcmp(text, "正在收听") || !strcmp(text, "正在收聽") ||
        !strcmp(text, "正在聆听") || !strcmp(text, "正在聆聽") ||
        !strcmp(text, "正在播放") || !strcmp(text, "未在播放"))
        return 1;
    if (strncmp(text, "正在", strlen("正在"))) return 0;
    for (i = 0; i < sizeof(endings) / sizeof(endings[0]); ++i)
    {
        len = strlen(endings[i]);
        if (w > strlen("正在") + len && !strcmp(text + w - len, endings[i]))
            return 1;
    }
    return 0;
}
#endif
