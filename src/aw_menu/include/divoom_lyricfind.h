#ifndef _divoom_lyricfind_h_
#define _divoom_lyricfind_h_

#include <stdint.h>

/* territory 必填；设备暂无 ISO 国家缓存，测评默认 US，可用 -DDIVOOM_LYRICFIND_TERRITORY=xx 覆盖。 */
#ifndef DIVOOM_LYRICFIND_TERRITORY
#define DIVOOM_LYRICFIND_TERRITORY  "US"
#endif

#define DIVOOM_LYRICFIND_LINE_MAX   512
#define DIVOOM_LYRICFIND_LINE_SIZE  256
#define DIVOOM_LYRICFIND_META_SIZE  256

typedef struct {
    uint32_t time_ms;
    char text[DIVOOM_LYRICFIND_LINE_SIZE];
} divoom_lyricfind_line_t;

typedef struct {
    int code;          /* PDF Lyric 返回码：111 LRC，101 全文，其它失败 */
    int synced;        /* 1=111 有时间戳，0=101 按行均分 */
    int line_count;
    divoom_lyricfind_line_t *lines;
    char description[64]; /* response.description，用来区分 FILTERED / NO LRC / NOT AVAILABLE */
    char copyright[DIVOOM_LYRICFIND_META_SIZE];
    char writer[DIVOOM_LYRICFIND_META_SIZE];
} divoom_lyricfind_result_t;

/* 打 lyric.do（format=lrc, output=json）。成功返回 0（即使 202 无词，line_count=0）；HTTP/解析失败返回 -1。 */
int divoom_lyricfind_fetch(const char *artist, const char *track, const char *album, int duration_sec, const char *territory, divoom_lyricfind_result_t *out);
void divoom_lyricfind_result_free(divoom_lyricfind_result_t *out);

#endif
