/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/** @file */
#ifndef __AS_COMMON_H__
#define __AS_COMMON_H__

#include <errno.h>
#include <string.h>
#include <AudioTrack.h>
#include <AudioRecord.h>

typedef struct PCM_CONFIG_S
{
    tAudioRecord *ar;
    tAudioTrack *at;
    char cardName[128]; ///< pcm handle identifier. e.g., PlaybackRateDmix:16000,1,960

    unsigned int chnCnt; //underlying channel number of alsaLib, including refChn if aec is enable.
    unsigned int sampleRate;

    unsigned long bufferSize;   //unit: sample number
    unsigned long chunkSize;    //unit: sample number, we take it as period_size. 0:use default value.

    unsigned int bitsPerSample;
    unsigned int significantBitsPerSample;
    unsigned int bitsPerFrame;
    unsigned int chunkBytes;    //unit: bytes
    int aec_delay_ms;

    int snd_card_id;
    int pcm_flag;
    int read_pcm_aec;             // used to indicate aec condition or normal one when read pcm. 1:aec, 0:normal
} PCM_CONFIG_S;

typedef struct AIO_MIXER_S {
    void *handle;
    int snd_card_id;
} AIO_MIXER_S;

int alsaSetPcmParams(PCM_CONFIG_S *pcmCfg);
int alsaOpenPcm(PCM_CONFIG_S *pcmCfg, const char *card, int pcmFlag);
void alsaClosePcm(PCM_CONFIG_S *pcmCfg, int pcmFlag);
void alsaPreparePcm(PCM_CONFIG_S *pcmCfg);

ssize_t alsaReadPcm(PCM_CONFIG_S *pcmCfg, void *data, size_t rcount);
ssize_t alsaWritePcm(PCM_CONFIG_S *pcmCfg, void *data, size_t wcount);
int alsaDrainPcm(PCM_CONFIG_S *pcmCfg);
int alsaGetDelayMs(PCM_CONFIG_S *pcmCfg);
int alsaOpenMixer(AIO_MIXER_S *mixer, const char *card);
void alsaCloseMixer(AIO_MIXER_S *mixer);
int alsaMixerSetCapPlaySyncMode(AIO_MIXER_S *mixer,  int value);
int alsaMixerSetAudioCodecHubMode(AIO_MIXER_S *mixer,  int value);
int alsaMixerSetDAudio0HubMode(AIO_MIXER_S *mixer,  int value);
int alsaMixerSetDAudio0LoopBackEn(AIO_MIXER_S *mixer,  int value);

int alsaMixerSetAudioCodecDacDrc(AIO_MIXER_S *mixer,  int value);
int alsaMixerSetAudioCodecAdcDrc(AIO_MIXER_S *mixer,  int value);
int alsaMixerSetAudioCodecDacHpf(AIO_MIXER_S *mixer,  int value);
int alsaMixerSetAudioCodecAdcHpf(AIO_MIXER_S *mixer,  int value);

int alsaMixerSetVolume(AIO_MIXER_S *mixer, int playFlag, long value);
int alsaMixerGetVolume(AIO_MIXER_S *mixer, int playFlag, long *value);
int alsaMixerSetSoftVolume(AIO_MIXER_S *mixer, int playFlag, long value);
int alsaMixerGetSoftVolume(AIO_MIXER_S *mixer, int playFlag, long *value);
int alsaMixerSetMute(AIO_MIXER_S *mixer, int playFlag, int bEnable);
int alsaMixerGetMute(AIO_MIXER_S *mixer, int playFlag, int *pVolVal);
void updateDebugfsByChnCnt(int pcmFlag, int cnt);

int alsaGetDelay(PCM_CONFIG_S *pcmCfg);
int alsaMixerSetPlayBackPA(AIO_MIXER_S *mixer, int bHighLevel);
int alsaMixerGetPlayBackPA(AIO_MIXER_S *mixer, int *pbHighLevel);

int alsaMixerSetMicXEnable(AIO_MIXER_S *mixer, int nMicId, int value);
int alsaMixerSetLineInEnable(AIO_MIXER_S *mixer,  int value);
#endif /* __AS_COMMON_H__ */
