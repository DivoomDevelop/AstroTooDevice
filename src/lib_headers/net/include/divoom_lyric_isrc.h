#ifndef DIVOOM_LYRIC_ISRC_H
#define DIVOOM_LYRIC_ISRC_H
/* Internal data only: callbacks are never serialized. All sends use wait_ret=1,
 * so the worker owns these snapshots until the network task has completed. */
typedef struct {
    char title[256], artist[256], isrc[13];
    int duration, found;
    int (*allow)(void *, int);
    void *user;
} divoom_lyric_isrc_info;
void divoom_lyric_isrc_request_pack(divoom_cJSON *, Pdivoom_net_command);
int divoom_lyric_isrc_response_unpack(divoom_cJSON *, Pdivoom_net_command);
#endif
