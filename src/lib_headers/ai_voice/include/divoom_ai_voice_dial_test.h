#ifndef DIVOOM_AI_VOICE_DIAL_TEST_H
#define DIVOOM_AI_VOICE_DIAL_TEST_H
/* 0：按住录音、松开后整段判空再发送；1：长按松开才录音，由VAD判起止。
 * 默认关闭静音自动结束，但保留发送前的全静音检查。仅影响ato 3499。 */
#define DIVOOM_AI_VOICE_ATO_HOLD_TO_SEND 0
#define DIVOOM_AI_VOICE_ATO_RELEASE_TO_VAD 1
#ifndef DIVOOM_AI_VOICE_ATO_INPUT_MODE
#define DIVOOM_AI_VOICE_ATO_INPUT_MODE DIVOOM_AI_VOICE_ATO_HOLD_TO_SEND
#endif
#if DIVOOM_AI_VOICE_ATO_INPUT_MODE != DIVOOM_AI_VOICE_ATO_HOLD_TO_SEND && \
    DIVOOM_AI_VOICE_ATO_INPUT_MODE != DIVOOM_AI_VOICE_ATO_RELEASE_TO_VAD
#error Invalid DIVOOM_AI_VOICE_ATO_INPUT_MODE
#endif
#ifdef __cplusplus
extern "C" {
#endif
/* LVGL lifecycle hook. Nonblocking; owner is an opaque identity, never dereferenced.
 * UI and dial-worker notifications are serialized internally. Repeated shows are idempotent.
 * A stale owner's hide cannot stop a newer visible owner's conversation. */
void divoom_ai_voice_dial_test_update(const void *owner, int clock_id, int visible);
/* 3499 的短按仅停止当前回答；空闲返回0，沿用原媒体按键。 */
int divoom_ai_voice_dial_test_play_key(void);
/* 原始物理边沿：450ms确认由控制线程处理，返回1表示本手势由ato消费。
 * 即使页面切换也应通知UP，防止新页面继承按住状态。 */
int divoom_ai_voice_dial_test_raw_key(int pressed);
/* 真机JL事件：0短按、1已确认长按、2持续按住、3松开；其它值不消费。 */
int divoom_ai_voice_dial_test_device_key(int action);
/* UI线程复制快照，hint_key必须经本地get_disp_str翻译；speech_text只含正文。 */
typedef struct
{
    unsigned revision;
    const char *hint_key;
    char visual_state[32];
    char speech_text[1024];
} divoom_ai_voice_dial_snapshot;
int divoom_ai_voice_dial_test_snapshot(divoom_ai_voice_dial_snapshot *out);
/* 完整正文副本，不受快照预览大小限制；调用方用 divoom_system_free 释放。 */
char *divoom_ai_voice_dial_test_copy_speech(unsigned *revision);
#ifdef __cplusplus
}
#endif
#endif
