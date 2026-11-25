#ifndef _divoom_audio_session_h_
#define _divoom_audio_session_h_

typedef enum {
	DIVOOM_AUDIO_SESSION_OWNER_NONE = 0,
	DIVOOM_AUDIO_SESSION_OWNER_TONE,
	DIVOOM_AUDIO_SESSION_OWNER_SLEEP,
	DIVOOM_AUDIO_SESSION_OWNER_HANDOFF,
} divoom_audio_session_owner_t;

/* Phase0/P2 稳态（HANDOFF 为迁移瞬态） */
typedef enum {
	DIVOOM_AUDIO_SESSION_STATE_IDLE = 0,
	DIVOOM_AUDIO_SESSION_STATE_SLEEP,
	DIVOOM_AUDIO_SESSION_STATE_TONE,
	DIVOOM_AUDIO_SESSION_STATE_OVERLAY,
	DIVOOM_AUDIO_SESSION_STATE_HANDOFF,
} divoom_audio_session_state_t;

typedef enum {
	DIVOOM_AUDIO_POLICY_ALLOW = 0,
	DIVOOM_AUDIO_POLICY_DENY,
	DIVOOM_AUDIO_POLICY_DUCK_OVERLAY,
	DIVOOM_AUDIO_POLICY_DUCK_HOLD,
	DIVOOM_AUDIO_POLICY_PREEMPT,
} divoom_audio_policy_action_t;

typedef struct {
	int tone_cut_pending;
} divoom_audio_policy_tone_ctx_t;

divoom_audio_session_owner_t divoom_audio_session_get_owner(void);
divoom_audio_session_state_t divoom_audio_session_get_state(void);

int divoom_audio_session_prepare_sleep_from_tone(unsigned int timeout_ms);
/** Step5b：有界等待提示音停播（入队 STOP + poll），供助眠抢占 TONE bus */
int divoom_audio_session_wait_tone_idle(unsigned int timeout_ms);
void divoom_audio_session_release_tone_bus(const char *tag);
void divoom_audio_session_release_sleep_bus(const char *tag);
int divoom_audio_session_preempt_sleep_for_tone(void);

void divoom_audio_session_on_tone_play_begin(void);
void divoom_audio_session_on_tone_play_end(void);

void divoom_audio_session_on_sleep_pcm_start(void);
void divoom_audio_session_on_sleep_pcm_stop(void);

divoom_audio_policy_action_t divoom_audio_session_on_request_tone(void);
int divoom_audio_session_is_overlay(void);

void divoom_audio_session_sleep_set_volume(int vol_percent);
void divoom_audio_session_sleep_stop_async(void);
int divoom_audio_session_sleep_stop_wait(unsigned int timeout_ms);

/**
 * 由助眠 ctrl 在 init 时注册：把 async stop 投递到 sleep_aid 串行 job，
 * 避免 audio_session 再挂一条独立 stop 线程与 job 并发停 PCM。
 * 未注册时 async 退化为同步 stop。
 */
typedef void (*divoom_audio_session_sleep_stop_async_fn)(void);
typedef int (*divoom_audio_session_sleep_stop_wait_fn)(unsigned int timeout_ms);
void divoom_audio_session_set_sleep_stop_handlers(
	divoom_audio_session_sleep_stop_async_fn async_fn,
	divoom_audio_session_sleep_stop_wait_fn wait_fn);

void divoom_audio_session_diag_print(void);

divoom_audio_policy_action_t divoom_audio_policy_evaluate_tone(
	divoom_audio_session_state_t state, const divoom_audio_policy_tone_ctx_t *ctx);

#endif
