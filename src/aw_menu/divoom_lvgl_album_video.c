#include "divoom.h"
#include "lvgl.h"
#include "divoom_pixel_format.h"
#include "divoom_lvgl_album_video.h"
#include "divoom_local_movie_play.h"

#if (!defined(DIVOOM_SIMULATOR) || !(DIVOOM_SIMULATOR)) && \
    defined(CONFIG_COMPONENTS_AW_EYESEE_MPP_VDEC) && defined(CONFIG_DRIVER_G2D_200)
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <mpi_sys.h>
#include <mpi_demux.h>
#include <mpi_vdec.h>
#include <mpi_clock.h>
#ifdef G2D_FORMAT_ARGB8888
#undef G2D_FORMAT_ARGB8888
#endif
#ifdef G2D_FORMAT_ABGR8888
#undef G2D_FORMAT_ABGR8888
#endif
#include "g2d_200/g2d_driver.h"

extern void AwStreamInit(void);
extern void AwParserInit(void);
extern int sunxi_g2d_open(void);
extern int sunxi_g2d_close(void);
extern int sunxi_g2d_control(int cmd, void *arg);
extern void *sunxifb_mem_alloc(size_t size, char *label);
extern void sunxifb_mem_free(void **data, char *label);
extern void *sunxifb_mem_get_phyaddr(void *data);
extern void sunxifb_mem_flush_cache(void *data, size_t size);
extern void sunxifb_mem_invalidate_cache(void *data, size_t size);

typedef struct
{
    pthread_mutex_t lock;
    void *worker;
    lv_obj_t *image;
    lv_timer_t *timer;
    lv_img_dsc_t frames[2];
    void *buffers[2];
    int borrowed_buffers;
    void (*first_frame)(void *owner);
    void *buffer_owner;
    int front;
    int pending;
    int stop;
    int finished;
    int eof;
    int loop;
    unsigned int decoded_frames;
    unsigned int converted_frames;
    unsigned int presented_frames;
    unsigned int play_time_ms;
    int width;
    int height;
    unsigned int frame_interval_ms;
    char uri[272];
    /* 5216 bytes on RV32. Keep this in the heap-owned player, not on the
     * worker stack alongside the synchronous demux/parser call chain. */
    DEMUX_MEDIA_INFO_S media;
    DEMUX_CHN demux;
    VDEC_CHN vdec;
    CLOCK_CHN clock;
    int demux_bound;
    int clock_bound;
    int g2d_opened;
} divoom_album_video;

/* UI-owned singleton. Hardware resources remain claimed until stop joins. */
static divoom_album_video *s_album_video;

static int album_video_stopping(divoom_album_video *video)
{
    int stop;
    pthread_mutex_lock(&video->lock);
    stop = video->stop;
    pthread_mutex_unlock(&video->lock);
    return stop || !divoom_local_movie_play_is_idle();
}

static int album_video_delay(divoom_album_video *video, unsigned int ms)
{
    while (ms && !album_video_stopping(video))
    {
        unsigned int step = ms > 10 ? 10 : ms;
        divoom_system_delay_ms(step);
        ms -= step;
    }
    return album_video_stopping(video);
}

static ERRORTYPE album_video_callback(void *cookie, MPP_CHN_S *channel,
    MPP_EVENT_TYPE event, void *event_data)
{
    divoom_album_video *video = cookie;
    if (event == MPP_EVENT_NOTIFY_EOF)
    {
        if (channel->mModId == MOD_ID_DEMUX && video->vdec >= 0)
            AW_MPI_VDEC_SetStreamEof(video->vdec, TRUE);
        else if (channel->mModId == MOD_ID_VDEC)
        {
            pthread_mutex_lock(&video->lock);
            video->eof = 1;
            pthread_mutex_unlock(&video->lock);
        }
    }
    (void)event_data;
    return SUCCESS;
}

static void album_video_destroy_pipeline(divoom_album_video *video)
{
    MPP_CHN_S demux = {MOD_ID_DEMUX, 0, video->demux};
    MPP_CHN_S vdec = {MOD_ID_VDEC, 0, video->vdec};
    MPP_CHN_S clock = {MOD_ID_CLOCK, 0, video->clock};
    if (video->vdec >= 0) AW_MPI_VDEC_StopRecvStream(video->vdec);
    if (video->demux >= 0) AW_MPI_DEMUX_Stop(video->demux);
    if (video->clock >= 0) AW_MPI_CLOCK_Stop(video->clock);
    if (video->clock_bound) AW_MPI_SYS_UnBind(&clock, &demux);
    if (video->demux_bound) AW_MPI_SYS_UnBind(&demux, &vdec);
    if (video->demux >= 0) AW_MPI_DEMUX_DestroyChn(video->demux);
    if (video->vdec >= 0) AW_MPI_VDEC_DestroyChn(video->vdec);
    if (video->clock >= 0) AW_MPI_CLOCK_DestroyChn(video->clock);
    video->demux = video->vdec = video->clock = MM_INVALID_CHN;
    video->demux_bound = video->clock_bound = 0;
    if (video->g2d_opened)
    {
        sunxi_g2d_close();
        video->g2d_opened = 0;
    }
    /* MPP is shared with the existing player. Never call AW_MPI_SYS_Exit. */
}

static int album_video_prepare(divoom_album_video *video)
{
    MPP_SYS_CONF_S config;
    DEMUX_CHN_ATTR_S demux_attr;
    DEMUX_MEDIA_INFO_S *media = &video->media;
    VDEC_CHN_ATTR_S vdec_attr;
    CLOCK_CHN_ATTR_S clock_attr;
    MPPCallbackInfo callback = {video, album_video_callback};
    MPP_CHN_S demux = {MOD_ID_DEMUX, 0, 0};
    MPP_CHN_S vdec = {MOD_ID_VDEC, 0, 0};
    MPP_CHN_S clock = {MOD_ID_CLOCK, 0, 0};
    DEMUX_VIDEO_STREAM_INFO_S *stream;
    ERRORTYPE ret;

    AwStreamInit();
    AwParserInit();
    memset(&config, 0, sizeof(config));
    if (AW_MPI_SYS_GetConf(&config) != SUCCESS || config.nAlignWidth == 0)
    {
        config.nAlignWidth = 32;
        ret = AW_MPI_SYS_SetConf(&config);
        if (ret != SUCCESS) return -1;
    }
    if (AW_MPI_SYS_Init() != SUCCESS || sunxi_g2d_open() < 0) return -1;
    video->g2d_opened = 1;

    memset(&demux_attr, 0, sizeof(demux_attr));
    demux_attr.mSourceType = SOURCETYPE_URL;
    demux_attr.mSourceUrl = video->uri;
    demux_attr.mFd = -1;
    demux_attr.mDemuxDisableTrack = DEMUX_DISABLE_AUDIO_TRACK | DEMUX_DISABLE_SUBTITLE_TRACK;
    /* Do not search for another channel: an occupied channel is busy. */
    if (AW_MPI_DEMUX_CreateChn(0, &demux_attr) != SUCCESS) return -1;
    video->demux = 0;
    if (AW_MPI_DEMUX_RegisterCallback(0, &callback) != SUCCESS ||
        AW_MPI_DEMUX_GetMediaInfo(0, media) != SUCCESS ||
        media->mVideoNum <= 0 || media->mVideoIndex < 0 || media->mVideoIndex >= media->mVideoNum)
        return -1;
    stream = &media->mVideoStreamInfo[media->mVideoIndex];
    if (stream->mWidth <= 0 || stream->mHeight <= 0) return -1;
    video->frame_interval_ms = stream->mFrameRate > 0 ? 1000000U / stream->mFrameRate : 40;
    if (video->frame_interval_ms == 0) video->frame_interval_ms = 1;

    memset(&vdec_attr, 0, sizeof(vdec_attr));
    /* Original-size decode. G2D alone scales the output to the widget. */
    vdec_attr.mPicWidth = stream->mWidth;
    vdec_attr.mPicHeight = stream->mHeight;
    vdec_attr.mType = stream->mCodecType;
    vdec_attr.mOutputPixelFormat = MM_PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    /* The RTOS Cedarc build actually uses a 512KB bitstream buffer. */
    vdec_attr.mBufSize = 512 * 1024;
    vdec_attr.mVdecVideoAttr.mMode = VIDEO_MODE_FRAME;
    vdec_attr.mVdecVideoAttr.mSupportBFrame = 0;
    /* Opt in only this channel to the SDK's existing small-H264 budget.
     * G2D holds one decoded frame until conversion completes; LVGL keeps
     * separate RGB buffers. Decoder reference/smooth frames are unchanged. */
    if (stream->mCodecType == PT_H264 && stream->mWidth <= 480 && stream->mHeight <= 480)
    {
        vdec_attr.bEnableExtraFrameNum = TRUE;
        vdec_attr.mExtraFrameNum = 2;
    }
    if (AW_MPI_VDEC_CreateChn(0, &vdec_attr) != SUCCESS) return -1;
    video->vdec = 0;
    if (AW_MPI_VDEC_RegisterCallback(0, &callback) != SUCCESS ||
        AW_MPI_VDEC_ForceFramePackage(0, FALSE) != SUCCESS ||
        AW_MPI_SYS_Bind(&demux, &vdec) != SUCCESS) return -1;
    video->demux_bound = 1;

    memset(&clock_attr, 0, sizeof(clock_attr));
    if (AW_MPI_CLOCK_CreateChn(0, &clock_attr) != SUCCESS) return -1;
    video->clock = 0;
    if (AW_MPI_SYS_Bind(&clock, &demux) != SUCCESS) return -1;
    video->clock_bound = 1;
    if (AW_MPI_CLOCK_Start(0) != SUCCESS ||
        AW_MPI_VDEC_StartRecvStream(0) != SUCCESS || AW_MPI_DEMUX_Start(0) != SUCCESS)
        return -1;
    divoom_print("album video: source:%dx%d output:%dx%d duration_ms:%u slide_ms:%u loop:%d extra_frames:%d vbv_kb:%u, LVGL only\n",
        stream->mWidth, stream->mHeight, video->width, video->height,
        media->mDuration, video->play_time_ms, video->loop,
        vdec_attr.bEnableExtraFrameNum ? vdec_attr.mExtraFrameNum : -1,
        (unsigned int)vdec_attr.mBufSize / 1024);
    return 0;
}

static int album_video_rewind(divoom_album_video *video)
{
    if (AW_MPI_VDEC_Pause(video->vdec) != SUCCESS ||
        AW_MPI_DEMUX_Stop(video->demux) != SUCCESS ||
        AW_MPI_CLOCK_Stop(video->clock) != SUCCESS ||
        AW_MPI_DEMUX_Seek(video->demux, 0) != SUCCESS ||
        AW_MPI_VDEC_SetStreamEof(video->vdec, FALSE) != SUCCESS ||
        AW_MPI_VDEC_Seek(video->vdec) != SUCCESS ||
        AW_MPI_CLOCK_Seek(video->clock) != SUCCESS) return -1;
    pthread_mutex_lock(&video->lock);
    video->eof = 0;
    pthread_mutex_unlock(&video->lock);
    if (AW_MPI_CLOCK_Start(video->clock) != SUCCESS ||
        AW_MPI_VDEC_StartRecvStream(video->vdec) != SUCCESS ||
        AW_MPI_DEMUX_Start(video->demux) != SUCCESS) return -1;
    return 0;
}

static int album_video_convert(divoom_album_video *video, VIDEO_FRAME_INFO_S *frame, int back)
{
    VIDEO_FRAME_S *src = &frame->VFrame;
    g2d_blt_h blit;
    int crop_w = src->mOffsetRight - src->mOffsetLeft;
    int crop_h = src->mOffsetBottom - src->mOffsetTop;
    int stride = src->mStride[0] ? src->mStride[0] : src->mWidth;
    int dst_w, dst_h;
    void *dst_phy = sunxifb_mem_get_phyaddr(video->buffers[back]);
    if (!dst_phy || !src->mPhyAddr[0] || !src->mPhyAddr[1] || stride < (int)src->mWidth)
        return -1;
    memset(&blit, 0, sizeof(blit));
    if (src->mPixelFormat == MM_PIXEL_FORMAT_YUV_SEMIPLANAR_420)
        blit.src_image_h.format = G2D_FORMAT_YUV420UVC_V1U1V0U0;
    else if (src->mPixelFormat == MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420)
        blit.src_image_h.format = G2D_FORMAT_YUV420UVC_U1V1U0V0;
    else return -1;
    if (crop_w <= 0 || crop_h <= 0)
    {
        crop_w = src->mWidth;
        crop_h = src->mHeight;
        blit.src_image_h.clip_rect.x = blit.src_image_h.clip_rect.y = 0;
    }
    else
    {
        blit.src_image_h.clip_rect.x = src->mOffsetLeft;
        blit.src_image_h.clip_rect.y = src->mOffsetTop;
    }
    if (blit.src_image_h.clip_rect.x < 0 || blit.src_image_h.clip_rect.y < 0 ||
        blit.src_image_h.clip_rect.x + crop_w > stride ||
        blit.src_image_h.clip_rect.y + crop_h > (int)src->mHeight) return -1;

    /* Fit without distortion; black bars are part of the small RGB buffer. */
    dst_w = video->width;
    dst_h = (int)((int64_t)crop_h * dst_w / crop_w);
    if (dst_h > video->height)
    {
        dst_h = video->height;
        dst_w = (int)((int64_t)crop_w * dst_h / crop_h);
    }
    if (dst_w <= 0 || dst_h <= 0) return -1;
    memset(video->buffers[back], 0, video->frames[back].data_size);
    sunxifb_mem_flush_cache(video->buffers[back], video->frames[back].data_size);
    blit.flag_h = G2D_BLT_NONE_H;
    blit.src_image_h.width = stride;
    blit.src_image_h.height = src->mHeight;
    blit.src_image_h.clip_rect.w = crop_w;
    blit.src_image_h.clip_rect.h = crop_h;
    blit.src_image_h.laddr[0] = src->mPhyAddr[0];
    blit.src_image_h.laddr[1] = src->mPhyAddr[1];
    blit.src_image_h.use_phy_addr = 1;
    /* G2D200 treats bbuff==0 as solid-color fill, ignoring the YUV addresses. */
    blit.src_image_h.bbuff = 1;
    /* YUV has no alpha. In this driver's bitblt path, mode!=0 also reads
     * the RGB destination into the YUV blender before output CSC. Keep
     * this an opaque, source-only conversion; LVGL handles later blending. */
    blit.src_image_h.mode = G2D_PIXEL_ALPHA;
    blit.src_image_h.alpha = 255;
    blit.src_image_h.gamut = src->mHeight >= 720 ? G2D_BT709 : G2D_BT601;
    blit.dst_image_h.format = G2D_FORMAT_RGB565;
    blit.dst_image_h.width = video->width;
    blit.dst_image_h.height = video->height;
    blit.dst_image_h.clip_rect.x = (video->width - dst_w) / 2;
    blit.dst_image_h.clip_rect.y = (video->height - dst_h) / 2;
    blit.dst_image_h.clip_rect.w = dst_w;
    blit.dst_image_h.clip_rect.h = dst_h;
    blit.dst_image_h.laddr[0] = (uintptr_t)dst_phy;
    blit.dst_image_h.use_phy_addr = 1;
    blit.dst_image_h.bbuff = 1;
    blit.dst_image_h.mode = G2D_PIXEL_ALPHA;
    blit.dst_image_h.alpha = 255;
    blit.dst_image_h.gamut = blit.src_image_h.gamut;
    {
        int ret = sunxi_g2d_control(G2D_CMD_BITBLT_H, &blit);
        if (ret < 0)
        {
            divoom_print("album video G2D failed: ret:%d src:%dx%d dst:%dx%d\n",
                ret, crop_w, crop_h, dst_w, dst_h);
            return -1;
        }
    }
    sunxifb_mem_invalidate_cache(video->buffers[back], video->frames[back].data_size);
    if (video->converted_frames < 3)
    {
        const uint16_t *pixels = (const uint16_t *)video->buffers[back];
        unsigned int hash = 2166136261U;
        unsigned int min = 0xffff, max = 0;
        int x, y;
        for (y = 0; y < 8; y++)
            for (x = 0; x < 8; x++)
            {
                unsigned int pixel = pixels[(blit.dst_image_h.clip_rect.y +
                    y * dst_h / 8) * video->width + blit.dst_image_h.clip_rect.x + x * dst_w / 8];
                if (pixel < min) min = pixel;
                if (pixel > max) max = pixel;
                hash = (hash ^ pixel) * 16777619U;
            }
        divoom_print("album video RGB sample: frame:%u min:%04x max:%04x hash:%08x\n",
            video->converted_frames + 1, min, max, hash);
    }
    return 0;
}

static void *album_video_worker(void *arg)
{
    divoom_album_video *video = arg;
    unsigned int start = 0, epoch = 0, tail_due = 0;
    unsigned int last_frame = divoom_system_get_tickms();
    uint64_t first_pts = 0;
    unsigned int frame_num = 0;
    int have_frame = 0, result = -1;
    if (album_video_stopping(video) || album_video_prepare(video) < 0) goto done;
    while (!album_video_stopping(video))
    {
        VIDEO_FRAME_INFO_S frame;
        unsigned int now = divoom_system_get_tickms();
        int eof, back;
        if (AW_MPI_VDEC_GetImage(video->vdec, &frame, 50) != SUCCESS)
        {
            pthread_mutex_lock(&video->lock);
            eof = video->eof;
            pthread_mutex_unlock(&video->lock);
            if (eof)
            {
                if (!have_frame) break;
                /* EOF may arrive immediately after the last frame. Keep its
                 * display interval before looping, especially for short clips. */
                now = divoom_system_get_tickms();
                if ((int32_t)(tail_due - now) > 0 &&
                    album_video_delay(video, tail_due - now)) break;
                /* Match the VO player's EOF policy: the slide interval is
                 * a minimum stay, never a deadline that cuts a video short. */
                if (!video->loop && (!video->play_time_ms ||
                    (unsigned int)(divoom_system_get_tickms() - start) >= video->play_time_ms))
                {
                    result = 0;
                    break;
                }
                if (album_video_rewind(video) < 0) break;
                frame_num = 0;
                epoch = 0;
                last_frame = divoom_system_get_tickms();
            }
            else if ((unsigned int)(divoom_system_get_tickms() - last_frame) > 5000)
                break;
            else album_video_delay(video, 5);
            continue;
        }
        now = divoom_system_get_tickms();
        last_frame = now;
        if (video->decoded_frames++ == 0)
        {
            divoom_print("album video first decoded: fmt:%d buffer:%ux%u stride:%u crop:%d,%d,%d,%d\n",
                frame.VFrame.mPixelFormat, frame.VFrame.mWidth, frame.VFrame.mHeight,
                frame.VFrame.mStride[0], frame.VFrame.mOffsetLeft, frame.VFrame.mOffsetTop,
                frame.VFrame.mOffsetRight, frame.VFrame.mOffsetBottom);
            divoom_print("album video RAM after decode: free_kb:%ld largest_kb:%ld\n",
                divoom_system_get_ram_free_size(), divoom_system_get_ram_largest_free_block_size());
        }
        if (frame_num == 0)
        {
            first_pts = frame.VFrame.mpts;
            epoch = now;
            if (!have_frame) start = now;
        }
        {
            int64_t delta = ((int64_t)frame.VFrame.mpts - (int64_t)first_pts) / 1000;
            unsigned int elapsed = now - epoch;
            /* Invalid/missing timestamps fall back to the stream frame rate. */
            if (delta < 0 || delta > (int64_t)elapsed + 60000 ||
                (frame_num && frame.VFrame.mpts == first_pts))
                delta = (int64_t)frame_num * video->frame_interval_ms;
            if (delta > elapsed) album_video_delay(video, (unsigned int)(delta - elapsed));
            tail_due = epoch + (unsigned int)delta + video->frame_interval_ms;
        }
        frame_num++;
        pthread_mutex_lock(&video->lock);
        back = video->pending < 0 ? 1 - video->front : -1;
        pthread_mutex_unlock(&video->lock);
        if (album_video_stopping(video))
        {
            AW_MPI_VDEC_ReleaseImage(video->vdec, &frame);
            break;
        }
        if (back >= 0)
        {
            int converted = album_video_convert(video, &frame, back);
            AW_MPI_VDEC_ReleaseImage(video->vdec, &frame);
            if (converted < 0) break;
            pthread_mutex_lock(&video->lock);
            video->converted_frames++;
            video->pending = back;
            pthread_mutex_unlock(&video->lock);
            have_frame = 1;
        }
        else AW_MPI_VDEC_ReleaseImage(video->vdec, &frame);
    }
done:
    album_video_destroy_pipeline(video);
    pthread_mutex_lock(&video->lock);
    video->finished = 1;
    divoom_print("album video exit: result:%d decoded:%u G2D:%u LVGL:%u elapsed_ms:%u\n",
        result, video->decoded_frames, video->converted_frames, video->presented_frames,
        have_frame ? (unsigned int)(divoom_system_get_tickms() - start) : 0);
    pthread_mutex_unlock(&video->lock);
    return NULL;
}

/* Runs in LVGL, never in the decoder worker. The worker only writes the back
 * buffer and cannot reuse the displayed buffer until this swap completes. */
static void album_video_refresh(lv_timer_t *timer)
{
    divoom_album_video *video = lv_timer_get_user_data(timer);
    pthread_mutex_lock(&video->lock);
    if (video->pending >= 0)
    {
        int front = video->pending;
        lv_img_cache_invalidate_src(&video->frames[front]);
        lv_img_set_src(video->image, &video->frames[front]);
        lv_obj_clear_flag(video->image, LV_OBJ_FLAG_HIDDEN);
        lv_obj_invalidate(video->image);
        if (video->presented_frames++ == 0)
        {
            if(video->first_frame) video->first_frame(video->buffer_owner);
            const uint16_t *pixels = (const uint16_t *)video->frames[front].data;
            divoom_print("album video first LVGL frame: %dx%d center_rgb565:%04x\n",
                video->width, video->height,
                pixels[(video->height / 2) * video->width + video->width / 2]);
        }
        video->front = front;
        video->pending = -1;
    }
    pthread_mutex_unlock(&video->lock);
}

void *divoom_lvgl_album_video_start(void *parent, const char *path,
    int width, int height, unsigned int play_time_ms, int loop)
{
    return divoom_lvgl_album_video_start_with_buffers(parent, path, width, height,
        play_time_ms, loop, NULL);
}

void *divoom_lvgl_album_video_start_with_buffers(void *parent, const char *path,
    int width, int height, unsigned int play_time_ms, int loop,
    const divoom_lvgl_album_video_buffers *buffers)
{
    divoom_album_video *video;
    int i;
    if (!parent || !path || !path[0] || width <= 0 || height <= 0 ||
        width > 480 || height > 480 || strlen(path) > 255 || s_album_video ||
        !divoom_local_movie_play_is_idle()) return NULL;
    video = divoom_system_malloc(sizeof(*video));
    if (!video) return NULL;
    memset(video, 0, sizeof(*video));
    if (pthread_mutex_init(&video->lock, NULL) != 0)
    {
        divoom_system_free(video);
        return NULL;
    }
    video->demux = video->vdec = video->clock = MM_INVALID_CHN;
    video->pending = -1;
    video->width = width;
    video->height = height;
    video->play_time_ms = play_time_ms;
    video->loop = loop;
    video->borrowed_buffers = buffers != NULL;
    if(buffers)
    {
        video->first_frame = buffers->first_frame;
        video->buffer_owner = buffers->owner;
    }
    snprintf(video->uri, sizeof(video->uri), "%s%s",
        strncmp(path, "file://", 7) == 0 ? "" : "file://", path);
    s_album_video = video;
    for (i = 0; i < 2; i++)
    {
        divoom_lv_img_dsc_init_fmt(&video->frames[i], width, height,
            LV_COLOR_FORMAT_RGB565, NULL);
        if(buffers)
        {
            uintptr_t base = (uintptr_t)buffers->data[i];
            uintptr_t aligned;
            unsigned int padding;
            if(!base || base > (uintptr_t)-1 - 63U) goto fail;
            aligned = (base + 63U) & ~(uintptr_t)63U;
            padding = (unsigned int)(aligned - base);
            if(padding > buffers->capacity[i] ||
                video->frames[i].data_size > buffers->capacity[i] - padding ||
                aligned > (uintptr_t)-1 - video->frames[i].data_size) goto fail;
            video->buffers[i] = (void *)aligned;
        }
        else video->buffers[i] = sunxifb_mem_alloc(video->frames[i].data_size, "album_video");
        if (!video->buffers[i]) goto fail;
        /* The borrowed front buffer may still hold the entry placeholder. */
        if(!buffers) memset(video->buffers[i], 0, video->frames[i].data_size);
        video->frames[i].data = video->buffers[i];
    }
    if((uintptr_t)video->buffers[0] < (uintptr_t)video->buffers[1] + video->frames[1].data_size &&
        (uintptr_t)video->buffers[1] < (uintptr_t)video->buffers[0] + video->frames[0].data_size)
        goto fail;
    video->image = lv_img_create(parent);
    if (!video->image) goto fail;
    lv_obj_set_pos(video->image, 0, 0);
    lv_obj_set_size(video->image, width, height);
    lv_obj_add_flag(video->image, LV_OBJ_FLAG_HIDDEN);
    video->timer = lv_timer_create(album_video_refresh, 16, video);
    if (!video->timer) goto fail;
    video->worker = divoom_system_create_joinable_task(album_video_worker,
        "album_video", video, DIVOOM_TASK_STACK_ALBUM_VIDEO);
    if (!video->worker) goto fail;
    return video;
fail:
    divoom_lvgl_album_video_stop(video);
    return NULL;
}

int divoom_lvgl_album_video_has_frame(void *handle)
{
    divoom_album_video *video = handle;
    int ready;
    if (!video) return 0;
    pthread_mutex_lock(&video->lock);
    ready = video->presented_frames > 0;
    pthread_mutex_unlock(&video->lock);
    return ready;
}

int divoom_lvgl_album_video_finished(void *handle)
{
    divoom_album_video *video = handle;
    int finished;
    if (!video) return 1;
    pthread_mutex_lock(&video->lock);
    finished = video->finished;
    pthread_mutex_unlock(&video->lock);
    return finished;
}

void divoom_lvgl_album_video_stop(void *handle)
{
    divoom_album_video *video = handle;
    int i;
    if (!video) return;
    if (video->timer) lv_timer_del(video->timer);
    if (video->image) lv_obj_del(video->image);
    pthread_mutex_lock(&video->lock);
    video->stop = 1;
    pthread_mutex_unlock(&video->lock);
    if (video->worker) divoom_system_join_task(video->worker);
    for (i = 0; i < 2; i++)
    {
        lv_img_cache_invalidate_src(&video->frames[i]);
        if (video->buffers[i] && !video->borrowed_buffers)
            sunxifb_mem_free(&video->buffers[i], "album_video");
    }
    if (s_album_video == video) s_album_video = NULL;
    pthread_mutex_destroy(&video->lock);
    divoom_system_free(video);
}

#else
void *divoom_lvgl_album_video_start_with_buffers(void *parent, const char *path,
    int width, int height, unsigned int play_time_ms, int loop,
    const divoom_lvgl_album_video_buffers *buffers)
{
    (void)buffers;
    return divoom_lvgl_album_video_start(parent, path, width, height, play_time_ms, loop);
}

void *divoom_lvgl_album_video_start(void *parent, const char *path,
    int width, int height, unsigned int play_time_ms, int loop)
{
    (void)parent; (void)path; (void)width; (void)height;
    (void)play_time_ms; (void)loop;
    return NULL;
}
int divoom_lvgl_album_video_has_frame(void *handle) { (void)handle; return 0; }
int divoom_lvgl_album_video_finished(void *handle) { (void)handle; return 1; }
void divoom_lvgl_album_video_stop(void *handle) { (void)handle; }
#endif
