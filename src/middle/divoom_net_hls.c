/* HLS音频生产端：解析有界播放列表、PAT/PMT及PES，只向解码端交付压缩音频。
 * HTTP/TLS、压缩队列和解码由调用者管理；本模块不申请PCM，不持有UI或JSON。 */
#include "divoom.h"
#include "divoom_net_hls.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define DIVOOM_NET_HLS_LIST_BYTES 32768
#define DIVOOM_NET_HLS_READ_BYTES 4096
#define DIVOOM_NET_HLS_TS_BYTES 188
#define DIVOOM_NET_HLS_PSI_BYTES 1024
/* 媒体序号使用SDK的64位类型，防止直播运行时间增长后回绕。 */
typedef T_U64 divoom_net_hls_sequence;

typedef struct
{
    uint8 data[DIVOOM_NET_HLS_PSI_BYTES];
    uint32 used;
    uint32 needed;
    int32 cc;
} divoom_net_hls_psi;

typedef struct
{
    const divoom_net_hls_io *io;
    char playlist[DIVOOM_NET_HLS_LIST_BYTES + 1];
    uint32 playlist_bytes;
    char playlist_url[DIVOOM_NET_HLS_URL_BYTES];
    char segment_url[DIVOOM_NET_HLS_URL_BYTES];
    uint8 read_data[DIVOOM_NET_HLS_READ_BYTES];
    uint8 packet[DIVOOM_NET_HLS_TS_BYTES];
    uint32 packet_used;
    divoom_net_hls_psi pat;
    divoom_net_hls_psi pmt;
    uint16 pmt_pid;
    uint16 audio_pid;
    uint8 codec;
    int32 audio_cc;
    uint8 pes_header[264];
    uint32 pes_used;
    uint32 pes_needed;
    int32 pes_remaining;
    uint8 pes_active;
    divoom_net_hls_sequence next;
    uint8 have_next;
} divoom_net_hls;

int32 divoom_net_hls_resolve(const char *base, const char *relative, char *out, uint32 capacity)
{
    char joined[DIVOOM_NET_HLS_URL_BYTES];
    const char *scheme = strstr(base, "://");
    if (!scheme || (!strncmp(base, "http://", 7) ? 0 : strncmp(base, "https://", 8)))
    {
        return -1;
    }
    uint32 prefix;
    if (!strncmp(relative, "http://", 7) || !strncmp(relative, "https://", 8))
    {
        prefix = 0;
    }
    else if (strstr(relative, "://"))
    {
        return -1;
    }
    else if (!strncmp(relative, "//", 2))
    {
        prefix = (uint32)(scheme - base) + 1;
    }
    else
    {
        const char *authority = scheme + 3;
        const char *path = authority + strcspn(authority, "/?#");
        if (path == authority)
        {
            return -1;
        }
        const char *end = base + strcspn(base, "?#");
        prefix = (uint32)(path - base);
        if (relative[0] != '/')
        {
            if (relative[0] == '?')
            {
                prefix = (uint32)(end - base);
            }
            else
            {
                while (end > path && end[-1] != '/')
                {
                    end--;
                }
                prefix = (uint32)(end - base);
            }
        }
    }
    size_t suffix = strcspn(relative, "#");
    uint32 slash = prefix && relative[0] != '/' && relative[0] != '?' && base[prefix - 1] != '/';
    if (prefix + slash + suffix >= sizeof(joined))
    {
        return -1;
    }
    memcpy(joined, base, prefix);
    if (slash)
    {
        joined[prefix++] = '/';
    }
    memcpy(joined + prefix, relative, suffix);
    joined[prefix + suffix] = 0;
    char *authority = strstr(joined, "://");
    if (!authority)
    {
        return -1;
    }
    authority += 3;
    char *path = authority + strcspn(authority, "/?#");
    uint32 write = (uint32)(path - joined);
    uint32 position = write;
    /* 仅规整路径中的点段，查询串和转义字节保持原样。 */
    while (joined[position] && joined[position] != '?' && joined[position] != '#')
    {
        uint32 start = position;
        if (joined[position] == '/')
        {
            position++;
        }
        uint32 segment = position;
        while (joined[position] && joined[position] != '/' && joined[position] != '?' && joined[position] != '#')
        {
            position++;
        }
        uint32 length = position - segment;
        if (length == 1 && joined[segment] == '.')
        {
            continue;
        }
        if (length == 2 && joined[segment] == '.' && joined[segment + 1] == '.')
        {
            while (write > (uint32)(path - joined) && joined[write - 1] != '/')
            {
                write--;
            }
            if (write > (uint32)(path - joined))
            {
                write--;
            }
            continue;
        }
        memmove(joined + write, joined + start, position - start);
        write += position - start;
    }
    memmove(joined + write, joined + position, strlen(joined + position) + 1);
    if (strlen(joined) >= capacity)
    {
        return -1;
    }
    strcpy(out, joined);
    return 0;
}

static char *divoom_net_hls_line(char **cursor)
{
    if (!*cursor || !**cursor)
    {
        return NULL;
    }
    char *line = *cursor;
    char *end = strpbrk(line, "\r\n");
    if (end)
    {
        *end++ = 0;
        while (*end == '\r' || *end == '\n')
        {
            *end++ = 0;
        }
        *cursor = end;
    }
    else
    {
        *cursor = NULL;
    }
    return line;
}

/* 属性中的引号可包含逗号；只返回完整值，不接受截断URI。 */
static int32 divoom_net_hls_attribute(const char *line, const char *key, char *out, uint32 capacity)
{
    const char *p = strchr(line, ':');
    p = p ? p + 1 : line;
    while (*p)
    {
        while (*p == ',' || *p == ' ')
        {
            p++;
        }
        const char *name = p;
        const char *equal = strchr(p, '=');
        if (!equal)
        {
            return 0;
        }
        p = equal + 1;
        uint8 quoted = *p == '"';
        p += quoted;
        const char *value = p;
        while (*p && (quoted ? *p != '"' : *p != ','))
        {
            p++;
        }
        if ((uint32)(equal - name) == strlen(key) && !memcmp(name, key, strlen(key)))
        {
            if ((quoted && !*p) || (uint32)(p - value) >= capacity)
            {
                return -1;
            }
            memcpy(out, value, (size_t)(p - value));
            out[p - value] = 0;
            return 1;
        }
        if (quoted && *p)
        {
            p++;
        }
        while (*p && *p != ',')
        {
            p++;
        }
    }
    return 0;
}

static int32 divoom_net_hls_playlist(divoom_net_hls *hls)
{
    uint32 used = 0;
    int32 result = hls->io->open(hls->io->user, hls->playlist_url);
    if (result)
    {
        hls->io->close(hls->io->user);
        return result;
    }
    while (!hls->io->cancelled(hls->io->user) && used < DIVOOM_NET_HLS_LIST_BYTES)
    {
        uint32 request = DIVOOM_NET_HLS_LIST_BYTES - used;
        if (request > DIVOOM_NET_HLS_READ_BYTES)
        {
            request = DIVOOM_NET_HLS_READ_BYTES;
        }
        int32 bytes = hls->io->read(hls->io->user, (uint8 *)hls->playlist + used, request);
        if (bytes < 0)
        {
            result = -1;
            break;
        }
        if (!bytes)
        {
            break;
        }
        used += (uint32)bytes;
    }
    hls->io->close(hls->io->user);
    hls->playlist[used] = 0;
    hls->playlist_bytes = used;
    if (hls->io->cancelled(hls->io->user))
    {
        return -1;
    }
    if (used == DIVOOM_NET_HLS_LIST_BYTES || strncmp(hls->playlist, "#EXTM3U", 7))
    {
        return -2;
    }
    return result;
}

static void divoom_net_hls_ts_reset(divoom_net_hls *hls)
{
    hls->packet_used = 0;
    hls->pat.used = hls->pmt.used = 0;
    hls->pat.needed = hls->pmt.needed = 0;
    hls->pat.cc = hls->pmt.cc = hls->audio_cc = -1;
    hls->pmt_pid = hls->audio_pid = 0x1fff;
    hls->codec = 0;
    hls->pes_active = 0;
    hls->pes_used = hls->pes_needed = 0;
    hls->pes_remaining = -1;
}

static uint32 divoom_net_hls_crc(const uint8 *data, uint32 bytes)
{
    uint32 crc = 0xffffffff;
    for (uint32 i = 0; i < bytes; i++)
    {
        crc ^= (uint32)data[i] << 24;
        for (uint32 bit = 0; bit < 8; bit++)
        {
            crc = crc & 0x80000000 ? (crc << 1) ^ 0x04c11db7 : crc << 1;
        }
    }
    return crc;
}

static int32 divoom_net_hls_table(divoom_net_hls *hls, divoom_net_hls_psi *psi)
{
    const uint8 *p = psi->data;
    uint32 end = psi->needed;
    if (end < 12 || !(p[1] & 0x80) || !(p[5] & 1) || divoom_net_hls_crc(p, end))
    {
        return 0;
    }
    if (psi == &hls->pat && p[0] == 0)
    {
        for (uint32 pos = 8; pos + 4 <= end - 4; pos += 4)
        {
            if (p[pos] || p[pos + 1])
            {
                uint16 pid = (uint16)(((p[pos + 2] & 31) << 8) | p[pos + 3]);
                if (pid != hls->pmt_pid)
                {
                    hls->pmt_pid = pid;
                    hls->pmt.used = hls->pmt.needed = 0;
                    hls->pmt.cc = -1;
                }
                break;
            }
        }
    }
    else if (psi == &hls->pmt && p[0] == 2 && end >= 16)
    {
        uint32 pos = 12 + (((p[10] & 15) << 8) | p[11]);
        while (pos + 5 <= end - 4)
        {
            uint32 length = 5 + (((p[pos + 3] & 15) << 8) | p[pos + 4]);
            if (length > end - 4 - pos)
            {
                return -2;
            }
            uint8 codec = p[pos] == 15 ? DIVOOM_NET_HLS_AAC :
                ((p[pos] == 3 || p[pos] == 4) ? DIVOOM_NET_HLS_MP3 : 0);
            if (codec)
            {
                uint16 pid = (uint16)(((p[pos + 1] & 31) << 8) | p[pos + 2]);
                if (hls->codec && (codec != hls->codec || pid != hls->audio_pid))
                {
                    int32 emit_result = hls->io->emit(hls->io->user, DIVOOM_NET_HLS_RESET, NULL, 0);
                    if (emit_result)
                    {
                        return emit_result == -2 ? -2 : -1;
                    }
                }
                if (pid != hls->audio_pid)
                {
                    hls->audio_cc = -1;
                    hls->pes_active = 0;
                }
                hls->codec = codec;
                hls->audio_pid = pid;
                return 0;
            }
            pos += length;
        }
        return -2;
    }
    return 0;
}

static int32 divoom_net_hls_psi_feed(divoom_net_hls *hls, divoom_net_hls_psi *psi,
    const uint8 *data, uint32 bytes)
{
    while (bytes)
    {
        if (!psi->used && *data == 0xff)
        {
            break;
        }
        uint32 target = psi->used < 3 ? 3 : psi->needed;
        uint32 copy = target - psi->used;
        if (copy > bytes)
        {
            copy = bytes;
        }
        memcpy(psi->data + psi->used, data, copy);
        psi->used += copy;
        data += copy;
        bytes -= copy;
        if (psi->used == 3)
        {
            psi->needed = 3 + (((psi->data[1] & 15) << 8) | psi->data[2]);
            if (psi->needed < 3 || psi->needed > sizeof(psi->data))
            {
                return -2;
            }
        }
        if (psi->needed && psi->used == psi->needed)
        {
            int32 result = divoom_net_hls_table(hls, psi);
            psi->used = psi->needed = 0;
            if (result)
            {
                return result;
            }
        }
    }
    return 0;
}

static int32 divoom_net_hls_pes(divoom_net_hls *hls, const uint8 *data, uint32 bytes, uint8 start)
{
    if (start)
    {
        hls->pes_active = 1;
        hls->pes_used = 0;
        hls->pes_needed = 9;
        hls->pes_remaining = -1;
    }
    if (!hls->pes_active)
    {
        return 0;
    }
    while (bytes && hls->pes_used < hls->pes_needed)
    {
        hls->pes_header[hls->pes_used++] = *data++;
        bytes--;
        if (hls->pes_used == 9)
        {
            const uint8 *p = hls->pes_header;
            if (p[0] || p[1] || p[2] != 1 || (p[6] & 0xc0) != 0x80)
            {
                return -2;
            }
            hls->pes_needed = 9 + p[8];
            uint32 length = (p[4] << 8) | p[5];
            if (length)
            {
                if (length < (uint32)(3 + p[8]))
                {
                    return -2;
                }
                hls->pes_remaining = (int32)(length - 3 - p[8]);
            }
        }
    }
    if (hls->pes_remaining >= 0 && bytes > (uint32)hls->pes_remaining)
    {
        bytes = (uint32)hls->pes_remaining;
    }
    if (bytes)
    {
        int32 emit_result = hls->io->emit(hls->io->user, hls->codec, data, bytes);
        if (emit_result)
        {
            return emit_result == -2 ? -2 : -1;
        }
        if (hls->pes_remaining >= 0)
        {
            hls->pes_remaining -= (int32)bytes;
        }
    }
    return 0;
}

static int32 divoom_net_hls_packet(divoom_net_hls *hls)
{
    const uint8 *p = hls->packet;
    uint16 pid = (uint16)(((p[1] & 31) << 8) | p[2]);
    uint8 start = (p[1] & 0x40) != 0;
    int32 cc = p[3] & 15;
    uint32 pos = 4;
    if (p[0] != 0x47 || (p[1] & 0x80))
    {
        hls->pes_active = 0;
        return -1;
    }
    if (p[3] & 0xc0)
    {
        return -2;
    }
    if (p[3] & 0x20)
    {
        pos += 1 + p[4];
        if (pos > DIVOOM_NET_HLS_TS_BYTES)
        {
            return -2;
        }
        if (p[4] && (p[5] & 0x80))
        {
            if (pid == hls->audio_pid)
            {
                hls->audio_cc = -1;
                hls->pes_active = 0;
                int32 emit_result = hls->io->emit(hls->io->user, DIVOOM_NET_HLS_RESET, NULL, 0);
                if (emit_result)
                {
                    return emit_result == -2 ? -2 : -1;
                }
            }
            if (pid == 0)
            {
                hls->pat.cc = -1;
                hls->pat.used = hls->pat.needed = 0;
            }
            else if (pid == hls->pmt_pid)
            {
                hls->pmt.cc = -1;
                hls->pmt.used = hls->pmt.needed = 0;
            }
        }
    }
    if (!(p[3] & 0x10) || pos == DIVOOM_NET_HLS_TS_BYTES)
    {
        return 0;
    }
    divoom_net_hls_psi *psi = pid == 0 ? &hls->pat : (pid == hls->pmt_pid ? &hls->pmt : NULL);
    if (psi)
    {
        if (psi->cc == cc)
        {
            return 0;
        }
        if (psi->cc >= 0 && cc != ((psi->cc + 1) & 15))
        {
            psi->used = psi->needed = 0;
        }
        psi->cc = cc;
        if (start)
        {
            uint32 pointer = p[pos++];
            if (pointer > DIVOOM_NET_HLS_TS_BYTES - pos)
            {
                return -2;
            }
            if (psi->used && divoom_net_hls_psi_feed(hls, psi, p + pos, pointer))
            {
                return -2;
            }
            psi->used = psi->needed = 0;
            pos += pointer;
        }
        else if (!psi->used)
        {
            return 0;
        }
        return divoom_net_hls_psi_feed(hls, psi, p + pos, DIVOOM_NET_HLS_TS_BYTES - pos);
    }
    if (pid == hls->audio_pid)
    {
        if (hls->audio_cc == cc)
        {
            return 0;
        }
        if (hls->audio_cc >= 0 && cc != ((hls->audio_cc + 1) & 15))
        {
            hls->pes_active = 0;
            int32 emit_result = hls->io->emit(hls->io->user, DIVOOM_NET_HLS_RESET, NULL, 0);
            if (emit_result)
            {
                return emit_result == -2 ? -2 : -1;
            }
        }
        hls->audio_cc = cc;
        return divoom_net_hls_pes(hls, p + pos, DIVOOM_NET_HLS_TS_BYTES - pos, start);
    }
    return 0;
}

static int32 divoom_net_hls_segment(divoom_net_hls *hls)
{
    /* 同一不可变分片断线重连时跳过已接收字节，保留残TS包，不重播已交付音频。 */
    uint32 received = 0;
    for (uint32 attempt = 0; attempt < 3 && !hls->io->cancelled(hls->io->user); attempt++)
    {
        int32 result = hls->io->open(hls->io->user, hls->segment_url);
        uint32 skipped = 0;
        while (!result && !hls->io->cancelled(hls->io->user))
        {
            int32 bytes = hls->io->read(hls->io->user, hls->read_data, sizeof(hls->read_data));
            if (bytes < 0)
            {
                result = -1;
                break;
            }
            if (!bytes)
            {
                hls->io->close(hls->io->user);
                return skipped >= received && !hls->packet_used ? 0 : -1;
            }
            uint32 pos = 0;
            if (skipped < received)
            {
                pos = received - skipped;
                if (pos > (uint32)bytes)
                {
                    pos = (uint32)bytes;
                }
                skipped += pos;
            }
            while (pos < (uint32)bytes)
            {
                hls->packet[hls->packet_used++] = hls->read_data[pos++];
                received++;
                skipped++;
                if (hls->packet_used == DIVOOM_NET_HLS_TS_BYTES)
                {
                    hls->packet_used = 0;
                    result = divoom_net_hls_packet(hls);
                    if (result)
                    {
                        break;
                    }
                }
            }
        }
        hls->io->close(hls->io->user);
        if (result == -2 || hls->io->cancelled(hls->io->user))
        {
            return result == -2 ? -2 : -1;
        }
        if (hls->io->wait(hls->io->user, 250u << attempt))
        {
            return -1;
        }
    }
    return -1;
}

/* 声明了CODECS时按已支持音频白名单选择；不能把更低码率AC-3/Opus选到AAC之前。 */
static int32 divoom_net_hls_codecs_supported(const char *codecs)
{
    static const char *allowed[] = {"mp4a.40.2", "mp4a.40.02", "mp4a.40.34", "mp4a.69", "mp4a.6B", "mp4a.6b", "mp3"};
    uint8 found = 0;
    while (*codecs)
    {
        while (*codecs == ' ' || *codecs == ',')
        {
            codecs++;
        }
        const char *end = codecs + strcspn(codecs, ",");
        const char *trim = end;
        while (trim > codecs && trim[-1] == ' ')
        {
            trim--;
        }
        uint8 supported = 0;
        for (uint32 i = 0; i < sizeof(allowed) / sizeof(allowed[0]); i++)
        {
            if ((size_t)(trim - codecs) == strlen(allowed[i]) && !memcmp(codecs, allowed[i], strlen(allowed[i])))
            {
                supported = 1;
                break;
            }
        }
        if (!supported)
        {
            return 0;
        }
        found = 1;
        codecs = *end ? end + 1 : end;
    }
    return found;
}

static int32 divoom_net_hls_run_inner(divoom_net_hls *hls)
{
    uint32 redirects = 0;
    uint32 failures = 0;
    while (!hls->io->cancelled(hls->io->user))
    {
        int32 result = divoom_net_hls_playlist(hls);
        if (result)
        {
            if (result == -2 || ++failures >= 3 || hls->io->wait(hls->io->user, 1000))
            {
                return result;
            }
            continue;
        }
        failures = 0;
        uint8 ended = strstr(hls->playlist, "#EXT-X-ENDLIST") != NULL;
        uint32 count = 0, target_ms = 1000;
        divoom_net_hls_sequence sequence = 0;
        char *cursor = hls->playlist;
        char *line;
        char attribute[DIVOOM_NET_HLS_URL_BYTES];
        char variant[DIVOOM_NET_HLS_URL_BYTES] = {0};
        uint32 best_bandwidth = 0xffffffff, bandwidth = 0xffffffff;
        uint8 pending_variant = 0;
        uint8 best_rank = 0;
        uint8 pending_duration = 0;
        while ((line = divoom_net_hls_line(&cursor)) != NULL)
        {
            if (!strncmp(line, "#EXT-X-MEDIA-SEQUENCE:", 22))
            {
                sequence = strtoull(line + 22, NULL, 10);
            }
            else if (!strncmp(line, "#EXTINF:", 8))
            {
                pending_duration = 1;
            }
            else if (!strncmp(line, "#EXT-X-TARGETDURATION:", 22))
            {
                uint32 seconds = (uint32)strtoul(line + 22, NULL, 10);
                target_ms = seconds > 10 ? 5000 : (seconds ? seconds * 500 : 500);
            }
            else if (!strncmp(line, "#EXT-X-MAP:", 11) || !strncmp(line, "#EXT-X-BYTERANGE:", 17))
            {
                return -2;
            }
            else if (!strncmp(line, "#EXT-X-KEY:", 11))
            {
                if (divoom_net_hls_attribute(line, "METHOD", attribute, sizeof(attribute)) != 1 || strcmp(attribute, "NONE"))
                {
                    return -2;
                }
            }
            else if (!strncmp(line, "#EXT-X-MEDIA:", 13))
            {
                if (divoom_net_hls_attribute(line, "TYPE", attribute, sizeof(attribute)) == 1 && !strcmp(attribute, "AUDIO") &&
                    divoom_net_hls_attribute(line, "URI", attribute, sizeof(attribute)) == 1 && !variant[0])
                {
                    strcpy(variant, attribute);
                    best_bandwidth = 0;
                }
            }
            else if (!strncmp(line, "#EXT-X-STREAM-INF:", 18))
            {
                pending_variant = 1;
                bandwidth = 0xffffffff;
                if (divoom_net_hls_attribute(line, "BANDWIDTH", attribute, sizeof(attribute)) == 1)
                {
                    bandwidth = (uint32)strtoul(attribute, NULL, 10);
                }
                int32 declared = divoom_net_hls_attribute(line, "CODECS", attribute, sizeof(attribute));
                if (declared)
                {
                    pending_variant = declared > 0 && divoom_net_hls_codecs_supported(attribute) ? 2 : 3;
                }
            }
            else if (line[0] && line[0] != '#')
            {
                if (pending_variant)
                {
                    if (pending_variant != 3 &&
                        (pending_variant > best_rank || (pending_variant == best_rank && bandwidth < best_bandwidth)) &&
                        strlen(line) < sizeof(variant) && best_bandwidth != 0)
                    {
                        strcpy(variant, line);
                        best_bandwidth = bandwidth;
                        best_rank = pending_variant;
                    }
                    pending_variant = 0;
                }
                else if (pending_duration)
                {
                    count++;
                    pending_duration = 0;
                }
            }
        }
        if (variant[0])
        {
            if (++redirects > 5 || divoom_net_hls_resolve(hls->playlist_url, variant,
                hls->segment_url, sizeof(hls->segment_url)))
            {
                return -2;
            }
            strcpy(hls->playlist_url, hls->segment_url);
            continue;
        }
        if (!count || sequence > (divoom_net_hls_sequence)-1 - count)
        {
            return -2;
        }
        redirects = 0;
        if (!hls->have_next)
        {
            hls->next = sequence + (!ended && count > 3 ? count - 3 : 0);
            hls->have_next = 1;
        }
        if (hls->next < sequence)
        {
            hls->next = sequence;
            divoom_net_hls_ts_reset(hls);
            int32 emit_result = hls->io->emit(hls->io->user, DIVOOM_NET_HLS_RESET, NULL, 0);
            if (emit_result)
            {
                return emit_result == -2 ? -2 : -1;
            }
        }
        /* line()已把换行改为0，按原缓冲长度走第二遍，保持每个URI的序号。 */
        uint32 position = 0;
        uint8 advanced = 0;
        uint8 discontinuity = 0;
        uint8 have_duration = 0;
        while (position < hls->playlist_bytes)
        {
            if (!hls->playlist[position])
            {
                position++;
                continue;
            }
            line = hls->playlist + position;
            position += (uint32)strlen(line) + 1;
            if (!strncmp(line, "#EXT-X-DISCONTINUITY", 20) && line[20] != '-')
            {
                discontinuity = 1;
            }
            if (!strncmp(line, "#EXTINF:", 8))
            {
                have_duration = 1;
            }
            if (line[0] && line[0] != '#' && have_duration)
            {
                if (sequence >= hls->next)
                {
                    if (discontinuity)
                    {
                        divoom_net_hls_ts_reset(hls);
                        int32 emit_result = hls->io->emit(hls->io->user, DIVOOM_NET_HLS_RESET, NULL, 0);
                        if (emit_result)
                        {
                            return emit_result == -2 ? -2 : -1;
                        }
                    }
                    if (divoom_net_hls_resolve(hls->playlist_url, line, hls->segment_url, sizeof(hls->segment_url)))
                    {
                        return -2;
                    }
                    result = divoom_net_hls_segment(hls);
                    if (result)
                    {
                        return result;
                    }
                    int32 emit_result = hls->io->emit(hls->io->user, DIVOOM_NET_HLS_SEGMENT, NULL, 0);
                    if (emit_result)
                    {
                        return emit_result == -2 ? -2 : -1;
                    }
                    hls->next = sequence + 1;
                    advanced = 1;
                }
                sequence++;
                have_duration = discontinuity = 0;
            }
        }
        if (ended)
        {
            return 0;
        }
        /* 下载新分片已受播放背压节流，及时刷新列表；只在无新片时等待，避免额外休眠耗尽压缩缓冲。 */
        if (!advanced && hls->io->wait(hls->io->user, target_ms))
        {
            return -1;
        }
    }
    return -1;
}

int32 divoom_net_hls_run(const char *url, const divoom_net_hls_io *io)
{
    if (!url || strlen(url) >= DIVOOM_NET_HLS_URL_BYTES || !io || !io->open || !io->read ||
        !io->close || !io->wait || !io->cancelled || !io->emit)
    {
        return -2;
    }
    divoom_net_hls *hls = divoom_system_malloc(sizeof(*hls));
    if (!hls)
    {
        return -1;
    }
    hls->io = io;
    strcpy(hls->playlist_url, url);
    divoom_net_hls_ts_reset(hls);
    int32 result = divoom_net_hls_run_inner(hls);
    divoom_system_free(hls);
    return result;
}
