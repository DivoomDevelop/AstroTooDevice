#include <divoom.h>
#include <speex/speex_preprocess.h>

// --- 配置参数 ---
// 每次处理的音频帧大小（样本数）
// 需要根据你的应用和 speex 的要求选择，通常与音频编解码器的帧大小一致
// 例如，对于 8kHz 采样率、20ms 帧，帧大小为 160
// 对于 16kHz 采样率、20ms 帧，帧大小为 320
#define FRAME_SIZE 160
// 音频采样率 (Hz)
#define SAMPLE_RATE 16000
// ----------------

void *divoom_speex_record_init(int sample_rate,int frame_size)
{
#if 0
    SpeexPreprocessState *preprocess_state;
    spx_int16_t *frame_buffer; // 用于存放一帧音频数据的缓冲区
    int denoise_enabled = 1;   // 启用降噪
    int noise_suppress_level = -25; // 噪声抑制级别 (dB)，负值表示抑制程度，可以调整

    // 1. 初始化预处理状态
    preprocess_state = speex_preprocess_state_init(frame_size, sample_rate);
    if (!preprocess_state) {
        fprintf(stderr, "Error initializing Speex preprocessor state.\n");
        return 1;
    }

    // 2. 配置预处理器参数 (按需配置)
    // 启用/禁用降噪
    speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_DENOISE, &denoise_enabled);

    // 设置噪声抑制级别 (根据需要调整，值越小抑制越强)
    speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &noise_suppress_level);

    /* 可选配置:
    // 启用/禁用 VAD (语音活动检测)
    int vad_enabled = 1;
    speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_VAD, &vad_enabled);

    // 启用/禁用 AGC (自动增益控制)
    int agc_enabled = 1;
    speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_AGC, &agc_enabled);
    // 设置 AGC 级别 (根据需要调整)
    // float agc_level = 8000.0;
    // speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_AGC_LEVEL, &agc_level);

    // 启用/禁用去混响
    int dereverb_enabled = 0; // 通常降噪和去混响只启用一个
    speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_DEREVERB, &dereverb_enabled);
    */
    return preprocess_state;
    #else
    return NULL;
    #endif
}


void divoom_speex_record_do(void *handle,uint8 *pcm_buf)
{
    //speex_preprocess_run(handle, pcm_buf);
}

void divoom_speex_record_destory(void *handle)
{
    //speex_preprocess_state_destroy(handle);
}

