#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <time.h>

extern int clock_gettime(int clock_id, struct timespec *tp);

#include "divoom.h"
#include "divoom_local_movie_play.h"
#include "divoom_menu_control.h"

#if !defined(CONFIG_COMPONENTS_AW_EYESEE_MPP) || \
    !defined(CONFIG_COMPONENTS_AW_EYESEE_MPP_DEMUXER) || \
    !defined(CONFIG_COMPONENTS_AW_EYESEE_MPP_VDEC) || \
    !defined(CONFIG_COMPONENTS_AW_EYESEE_MPP_VO)
#error "divoom_local_movie_play requires MPP, DEMUXER, VDEC and VO enabled"
#endif

#include <mpi_sys.h>
#include <mpi_demux.h>
#include <mpi_vdec.h>
#include <mpi_vo.h>
#include <mpi_clock.h>
#include <vdecoder.h>
#include <memoryAdapter.h>
#include <sc_interface.h>
#include "hwdisplay.h"

extern void AwStreamInit(void);
extern void AwParserInit(void);

enum
{
    DIVOOM_LOCAL_MOVIE_CMD_STOP = 0,
    DIVOOM_LOCAL_MOVIE_CMD_SEEK_START,
    DIVOOM_LOCAL_MOVIE_CMD_PLAY_ERR,
    DIVOOM_LOCAL_MOVIE_CMD_EOF,
};

enum
{
    DIVOOM_LOCAL_MOVIE_CLOCK_PORT_VIDEO = 1,
};

typedef struct _divoom_local_movie_cmd_item
{
    int cmd_type;
} divoom_local_movie_cmd_item, *Pdivoom_local_movie_cmd_item;

typedef struct _divoom_local_movie_play_item
{
    void *cmd_queue;
    void *exit_sem;
    void *notice_sem;
    int start_x;
    int start_y;
    int width;
    int high;
    int play_time;
    int play_frame_time;
    uint32 play_start_time_ms;
    uint32 play_start_time_sec;
    int frame_num;
    int play_over_flag;
    int play_start_flag;
    int exit_flag;
    int no_cb;
    int paused;
    int playing;
    int loop_enabled;
    int session_active;
    int src_fd;
    void *time_handle;
    char *movie_name;
    char src_path[256];
    divoom_local_movie_play_play_get_next *pcallback;
    void *para;

    MPP_SYS_CONF_S sys_conf;
    DEMUX_CHN dmx_chn;
    DEMUX_CHN_ATTR_S dmx_attr;
    DEMUX_MEDIA_INFO_S media_info;
    VDEC_CHN vdec_chn;
    VDEC_CHN_ATTR_S vdec_attr;
    VO_DEV vo_dev;
    VO_LAYER vo_layer;
    VO_CHN vo_chn;
    int ui_layer;
    VO_VIDEO_LAYER_ATTR_S vo_layer_attr;
    CLOCK_CHN clock_chn;
    CLOCK_CHN_ATTR_S clock_attr;
    int disp_x;
    int disp_y;
    int disp_w;
    int disp_h;
    int mpp_inited;
} divoom_local_movie_play_item, *Pdivoom_local_movie_play_item;

static void *pdivoom_local_movie_play_global_exit_sem = NULL;
static void *pdivoom_local_movie_play_global_lock_handle = NULL;
static Pdivoom_local_movie_play_item pdivoom_local_movie_play_handle = NULL;
static int divoom_local_movie_play_mpp_core_inited = 0;
static int s_local_movie_keep_jl_tone_volume = 0;

int divoom_local_movie_play_over_callback(void *para);
static void divoom_local_movie_play_destroy_pipeline(Pdivoom_local_movie_play_item data);

static uint32 divoom_local_movie_play_monotonic_ms(void)
{
    struct timespec now;
    uint64_t ms;

    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
    {
        return divoom_system_get_tickms();
    }

    ms = (uint64_t)now.tv_sec * 1000ULL + (uint64_t)now.tv_nsec / 1000000ULL;
    return (uint32)ms;
}

static int divoom_local_movie_play_mpp_core_init_once(Pdivoom_local_movie_play_item data)
{
    if (divoom_local_movie_play_mpp_core_inited)
    {
        return 0;
    }
    AwStreamInit();
    AwParserInit();
    memset(&data->sys_conf, 0, sizeof(data->sys_conf));
    data->sys_conf.nAlignWidth = 32;
    AW_MPI_SYS_SetConf(&data->sys_conf);
    if (AW_MPI_SYS_Init() != SUCCESS)
    {
        divoom_print("movie mpp sys init failed\n");
        return -1;
    }
    divoom_local_movie_play_mpp_core_inited = 1;
    return 0;
}

static int divoom_local_movie_play_time_reached(Pdivoom_local_movie_play_item data)
{
    uint32 elapsed_ms;
    uint64_t target_ms;

    if (data == NULL || data->play_time <= 0 || data->play_start_time_ms == 0)
    {
        return 0;
    }

    elapsed_ms = divoom_local_movie_play_monotonic_ms() - data->play_start_time_ms;
    target_ms = (uint64_t)(uint32)data->play_time * 1000ULL;
    return ((uint64_t)elapsed_ms >= target_ms);
}

static void divoom_local_movie_play_restore_ui_layer(Pdivoom_local_movie_play_item data)
{
    if (data == NULL || data->ui_layer < 0)
    {
        return;
    }

    divoom_print("restore ui layer by menu control: layer:%d\n", data->ui_layer);
    //divoom_menu_control_change_disp_mode(NULL, 0);
}

static void divoom_local_movie_play_hide_ui_layer(Pdivoom_local_movie_play_item data, const char *reason)
{
    if (data == NULL || data->ui_layer < 0)
    {
        return;
    }

    divoom_print("hide ui layer by menu control(%s): layer:%d\n",
                 reason ? reason : "",
                 data->ui_layer);
    //divoom_menu_control_change_disp_mode(NULL, 1);
}

static int divoom_local_movie_play_push_cmd(Pdivoom_local_movie_play_item play_handle, int cmd_type)
{
    Pdivoom_local_movie_cmd_item item;

    if (play_handle == NULL || play_handle->cmd_queue == NULL)
    {
        return 0;
    }

    item = divoom_system_malloc(sizeof(divoom_local_movie_cmd_item));
    if (item == NULL)
    {
        return 0;
    }
    item->cmd_type = cmd_type;
    if(!divoom_queue_push(play_handle->cmd_queue, item))
    {
        divoom_system_free(item);
        return 0;
    }
    if (play_handle->exit_sem)
    {
        divoom_system_put_sem(play_handle->exit_sem);
    }
    return 1;
}

static void divoom_local_movie_play_reset_channels(Pdivoom_local_movie_play_item data)
{
    if (data == NULL)
    {
        return;
    }

    data->dmx_chn = MM_INVALID_CHN;
    data->vdec_chn = MM_INVALID_CHN;
    data->vo_dev = MM_INVALID_DEV;
    data->vo_layer = MM_INVALID_DEV;
    data->vo_chn = MM_INVALID_CHN;
    data->clock_chn = MM_INVALID_CHN;
    data->ui_layer = MM_INVALID_CHN;
    data->src_fd = -1;
}

static void divoom_local_movie_play_calc_disp_rect(Pdivoom_local_movie_play_item data, int video_w, int video_h)
{
    if (data == NULL)
    {
        return;
    }
    data->disp_x = data->start_x;
    data->disp_y = data->start_y;
    data->disp_w = data->width;
    data->disp_h = data->high;
    if (data->disp_w <= 0)
    {
        data->disp_w = video_w;
    }
    if (data->disp_h <= 0)
    {
        data->disp_h = video_h;
    }
}

static ERRORTYPE divoom_local_movie_play_apply_disp_rect(Pdivoom_local_movie_play_item data)
{
    if (data == NULL || data->vo_layer < 0)
    {
        return DIVOOM_COMM_FAILURE;
    }
    data->vo_layer_attr.stDispRect.X = data->disp_x;
    data->vo_layer_attr.stDispRect.Y = data->disp_y;
    data->vo_layer_attr.stDispRect.Width = data->disp_w;
    data->vo_layer_attr.stDispRect.Height = data->disp_h;
    return AW_MPI_VO_SetVideoLayerAttr(data->vo_layer, &data->vo_layer_attr);
}

static ERRORTYPE divoom_local_movie_play_mpi_callback(void *cookie, MPP_CHN_S *p_chn, MPP_EVENT_TYPE event, void *p_event_data)
{
    Pdivoom_local_movie_play_item data = (Pdivoom_local_movie_play_item)cookie;

    if (data == NULL || p_chn == NULL)
    {
        return SUCCESS;
    }

    if (p_chn->mModId == MOD_ID_DEMUX)
    {
        if (event == MPP_EVENT_NOTIFY_EOF)
        {
            divoom_print("demux eof\n");
            if (data->vdec_chn >= 0)
            {
                AW_MPI_VDEC_SetStreamEof(data->vdec_chn, 1);
            }
        }
    }
    else if (p_chn->mModId == MOD_ID_VDEC)
    {
        if (event == MPP_EVENT_NOTIFY_EOF)
        {
            divoom_print("vdec eof\n");
            if (data->vo_layer >= 0 && data->vo_chn >= 0)
            {
                AW_MPI_VO_SetStreamEof(data->vo_layer, data->vo_chn, 1);
            }
        }
    }
    else if (p_chn->mModId == MOD_ID_VOU)
    {
        if (event == MPP_EVENT_RENDERING_START)
        {
            /* 首帧才完成 Cedar 的懒分配；总空闲和最大连续块必须同时观察。 */
            divoom_print("movie first frame file=%s free_kb=%ld largest_kb=%ld\n", data->src_path,
                divoom_system_get_ram_free_size(), divoom_system_get_ram_largest_free_block_size());
            data->play_start_flag = 1;
            data->play_frame_time = divoom_system_get_tickms();
            if (data->play_start_time_ms == 0)
            {
                data->play_start_time_ms = divoom_local_movie_play_monotonic_ms();
            }
            if (data->play_start_time_sec == 0)
            {
                data->play_start_time_sec = divoom_system_get_ticks();
            }
        }
        else if (event == MPP_EVENT_NOTIFY_EOF)
        {
            uint32 elapsed_ms = data->play_start_time_ms ?
                (divoom_local_movie_play_monotonic_ms() - data->play_start_time_ms) : 0;
            int time_reached = divoom_local_movie_play_time_reached(data);
            divoom_print("vo eof, elapsed_sec:%u, play_time:%d, reached:%d\n",
                         elapsed_ms / 1000,
                         data->play_time,
                         time_reached);
            if (data->exit_flag == 0 && data->loop_enabled && time_reached == 0)
            {
                divoom_local_movie_play_push_cmd(data, DIVOOM_LOCAL_MOVIE_CMD_SEEK_START);
            }
            else if (data->exit_flag == 0)
            {
                divoom_local_movie_play_push_cmd(data, DIVOOM_LOCAL_MOVIE_CMD_EOF);
            }
        }
    }

    (void)p_event_data;
    return SUCCESS;
}

static int divoom_local_movie_play_open_src(Pdivoom_local_movie_play_item data, const char *uri)
{
    const char *path = uri;
    int fd;

    if (data == NULL || path == NULL)
    {
        return -1;
    }
    if (strstr(path, "file://") == path)
    {
        path += 7;
    }

    strncpy(data->src_path, path, sizeof(data->src_path) - 1);
    data->src_path[sizeof(data->src_path) - 1] = '\0';

    fd = open(data->src_path, O_RDONLY);
    if (fd < 0)
    {
        divoom_print("open src fail: %s\n", data->src_path);
        return -1;
    }
    close(fd);
    data->src_fd = -1;
    return 0;
}

static ERRORTYPE divoom_local_movie_play_create_dmx(Pdivoom_local_movie_play_item data)
{
    ERRORTYPE ret;
    BOOL success = FALSE;
    MPPCallbackInfo cb_info;

    memset(&data->dmx_attr, 0, sizeof(data->dmx_attr));
    data->dmx_attr.mSourceType = SOURCETYPE_URL;
    data->dmx_attr.mSourceUrl = data->movie_name;
    data->dmx_attr.mFd = -1;
    data->dmx_attr.mDemuxDisableTrack = DEMUX_DISABLE_AUDIO_TRACK | DEMUX_DISABLE_SUBTITLE_TRACK;

    data->dmx_chn = 0;
    while (data->dmx_chn < DEMUX_MAX_CHN_NUM)
    {
        ret = AW_MPI_DEMUX_CreateChn(data->dmx_chn, &data->dmx_attr);
        if (ret == SUCCESS)
        {
            success = TRUE;
            break;
        }
        if (ret == ERR_DEMUX_EXIST)
        {
            data->dmx_chn++;
            continue;
        }
        break;
    }
    if (success == FALSE)
    {
        data->dmx_chn = MM_INVALID_CHN;
        return DIVOOM_COMM_FAILURE;
    }

    cb_info.cookie = (void *)data;
    cb_info.callback = (MPPCallbackFuncType)&divoom_local_movie_play_mpi_callback;
    AW_MPI_DEMUX_RegisterCallback(data->dmx_chn, &cb_info);
    return SUCCESS;
}

static ERRORTYPE divoom_local_movie_play_create_vdec(Pdivoom_local_movie_play_item data)
{
    ERRORTYPE ret;
    BOOL success = FALSE;
    MPPCallbackInfo cb_info;
    DEMUX_VIDEO_STREAM_INFO_S *stream_info;

    stream_info = &data->media_info.mVideoStreamInfo[data->media_info.mVideoIndex];
    memset(&data->vdec_attr, 0, sizeof(data->vdec_attr));
    data->vdec_attr.mPicWidth = stream_info->mWidth;
    data->vdec_attr.mPicHeight = stream_info->mHeight;
    data->vdec_attr.mOutputPixelFormat = MM_PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    data->vdec_attr.mBufSize = 3 * 512 * 1024;
    /* 480px H.264 使用 Cedar 支持的最小 1MB 码流缓冲，给歌词字形和变换层留出 512KB。 */
    if (stream_info->mCodecType == PT_H264 && stream_info->mWidth > 0 && stream_info->mWidth <= 480 &&
        stream_info->mHeight > 0 && stream_info->mHeight <= 480)
    {
        data->vdec_attr.mBufSize = 1024 * 1024;
        /* 显示保留从 3 帧降到 2 帧；VO 双缓冲、解码参考帧与平滑帧均保留。 */
        data->vdec_attr.bEnableExtraFrameNum = TRUE;
        data->vdec_attr.mExtraFrameNum = 2;
    }
    data->vdec_attr.mType = stream_info->mCodecType;
    data->vdec_attr.mVdecVideoAttr.mSupportBFrame = 0;
    data->vdec_attr.mVdecVideoAttr.mMode = VIDEO_MODE_FRAME;

    data->vdec_chn = 0;
    while (data->vdec_chn < VDEC_MAX_CHN_NUM)
    {
        ret = AW_MPI_VDEC_CreateChn(data->vdec_chn, &data->vdec_attr);
        if (ret == SUCCESS)
        {
            success = TRUE;
            break;
        }
        if (ret == ERR_VDEC_EXIST)
        {
            data->vdec_chn++;
            continue;
        }
        break;
    }
    if (success == FALSE)
    {
        data->vdec_chn = MM_INVALID_CHN;
        return DIVOOM_COMM_FAILURE;
    }

    cb_info.cookie = (void *)data;
    cb_info.callback = (MPPCallbackFuncType)&divoom_local_movie_play_mpi_callback;
    AW_MPI_VDEC_RegisterCallback(data->vdec_chn, &cb_info);
    AW_MPI_VDEC_ForceFramePackage(data->vdec_chn, FALSE);
    return SUCCESS;
}

static ERRORTYPE divoom_local_movie_play_create_vo(Pdivoom_local_movie_play_item data)
{
    ERRORTYPE ret;
    BOOL success = FALSE;
    MPPCallbackInfo cb_info;
    VO_PUB_ATTR_S pub_attr;
    int hlay0;

    data->vo_dev = 0;
    data->ui_layer = HLAY(2, 0);
    AW_MPI_VO_Enable(data->vo_dev);
    AW_MPI_VO_AddOutsideVideoLayer(data->ui_layer);
    divoom_local_movie_play_hide_ui_layer(data, "create_vo");

    AW_MPI_VO_GetPubAttr(data->vo_dev, &pub_attr);
    pub_attr.enIntfType = VO_INTF_LCD;
    pub_attr.enIntfSync = VO_OUTPUT_NTSC;
    AW_MPI_VO_SetPubAttr(data->vo_dev, &pub_attr);

    hlay0 = 0;
    while (hlay0 < VO_MAX_LAYER_NUM)
    {
        if (AW_MPI_VO_EnableVideoLayer(hlay0) == SUCCESS)
        {
            break;
        }
        hlay0++;
    }
    if (hlay0 >= VO_MAX_LAYER_NUM)
    {
        data->vo_layer = MM_INVALID_DEV;
        AW_MPI_VO_RemoveOutsideVideoLayer(data->ui_layer);
        AW_MPI_VO_Disable(data->vo_dev);
        return DIVOOM_COMM_FAILURE;
    }

    data->vo_layer = hlay0;
    AW_MPI_VO_GetVideoLayerAttr(data->vo_layer, &data->vo_layer_attr);
    data->vo_layer_attr.stDispRect.X = data->disp_x;
    data->vo_layer_attr.stDispRect.Y = data->disp_y;
    data->vo_layer_attr.stDispRect.Width = data->disp_w;
    data->vo_layer_attr.stDispRect.Height = data->disp_h;
    data->vo_layer_attr.enPixFormat = MM_PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    ret = AW_MPI_VO_SetVideoLayerAttr(data->vo_layer, &data->vo_layer_attr);
    if (ret != SUCCESS)
    {
        divoom_print("vo set layer attr failed: layer:%d ret:0x%x\n", data->vo_layer, ret);
        return ret;
    }

    ret = AW_MPI_VO_SetVideoLayerPriority(data->vo_layer, ZORDER_MAX);
    if (ret != SUCCESS)
    {
        divoom_print("vo set layer priority failed: layer:%d ret:0x%x\n", data->vo_layer, ret);
        return ret;
    }

    /* Keep the layer closed until VideoRender submits and shows its first frame. */
    ret = AW_MPI_VO_CloseVideoLayer(data->vo_layer);
    if (ret != SUCCESS)
    {
        divoom_print("vo close layer before first frame failed: layer:%d ret:0x%x\n",
                     data->vo_layer, ret);
        return ret;
    }
    divoom_print("vo layer prepared hidden: layer:%d rect:%d,%d %dx%d\n",
                 data->vo_layer,
                 data->disp_x, data->disp_y, data->disp_w, data->disp_h);

    data->vo_chn = 0;
    while (data->vo_chn < VO_MAX_CHN_NUM)
    {
        ret = AW_MPI_VO_CreateChn(data->vo_layer, data->vo_chn);
        if (ret == SUCCESS)
        {
            success = TRUE;
            break;
        }
        if (ret == ERR_VO_CHN_NOT_DISABLE)
        {
            data->vo_chn++;
            continue;
        }
        break;
    }
    if (success == FALSE)
    {
        data->vo_chn = MM_INVALID_CHN;
        return DIVOOM_COMM_FAILURE;
    }

    cb_info.cookie = (void *)data;
    cb_info.callback = (MPPCallbackFuncType)&divoom_local_movie_play_mpi_callback;
    AW_MPI_VO_RegisterCallback(data->vo_layer, data->vo_chn, &cb_info);
    AW_MPI_VO_SetChnDispBufNum(data->vo_layer, data->vo_chn, 2);
    return SUCCESS;
}

static ERRORTYPE divoom_local_movie_play_create_clock(Pdivoom_local_movie_play_item data)
{
    ERRORTYPE ret;
    BOOL success = FALSE;

    memset(&data->clock_attr, 0, sizeof(data->clock_attr));
    data->clock_attr.nWaitMask |= 1 << DIVOOM_LOCAL_MOVIE_CLOCK_PORT_VIDEO;
    data->clock_chn = 0;
    while (data->clock_chn < CLOCK_MAX_CHN_NUM)
    {
        ret = AW_MPI_CLOCK_CreateChn(data->clock_chn, &data->clock_attr);
        if (ret == SUCCESS)
        {
            success = TRUE;
            break;
        }
        if (ret == ERR_CLOCK_EXIST)
        {
            data->clock_chn++;
            continue;
        }
        break;
    }
    if (success == FALSE)
    {
        data->clock_chn = MM_INVALID_CHN;
        return DIVOOM_COMM_FAILURE;
    }
    return SUCCESS;
}

static ERRORTYPE divoom_local_movie_play_prepare(Pdivoom_local_movie_play_item data)
{
    MPP_CHN_S dmx_chn;
    MPP_CHN_S vdec_chn;
    MPP_CHN_S vo_chn;
    MPP_CHN_S clock_chn;

    if (divoom_local_movie_play_create_dmx(data) != SUCCESS)
    {
        return DIVOOM_COMM_FAILURE;
    }
    if (AW_MPI_DEMUX_GetMediaInfo(data->dmx_chn, &data->media_info) != SUCCESS)
    {
        divoom_local_movie_play_destroy_pipeline(data);
        return DIVOOM_COMM_FAILURE;
    }
    if (data->media_info.mVideoNum <= 0)
    {
        divoom_local_movie_play_destroy_pipeline(data);
        return DIVOOM_COMM_FAILURE;
    }

    divoom_local_movie_play_calc_disp_rect(data,
                                           data->media_info.mVideoStreamInfo[data->media_info.mVideoIndex].mWidth,
                                           data->media_info.mVideoStreamInfo[data->media_info.mVideoIndex].mHeight);

    if (divoom_local_movie_play_create_vdec(data) != SUCCESS)
    {
        divoom_local_movie_play_destroy_pipeline(data);
        return DIVOOM_COMM_FAILURE;
    }

    dmx_chn.mModId = MOD_ID_DEMUX;
    dmx_chn.mDevId = 0;
    dmx_chn.mChnId = data->dmx_chn;
    vdec_chn.mModId = MOD_ID_VDEC;
    vdec_chn.mDevId = 0;
    vdec_chn.mChnId = data->vdec_chn;
    if (AW_MPI_SYS_Bind(&dmx_chn, &vdec_chn) != SUCCESS)
    {
        divoom_local_movie_play_destroy_pipeline(data);
        return DIVOOM_COMM_FAILURE;
    }

    if (divoom_local_movie_play_create_vo(data) != SUCCESS)
    {
        divoom_local_movie_play_destroy_pipeline(data);
        return DIVOOM_COMM_FAILURE;
    }

    vo_chn.mModId = MOD_ID_VOU;
    vo_chn.mDevId = data->vo_layer;
    vo_chn.mChnId = data->vo_chn;
    if (AW_MPI_SYS_Bind(&vdec_chn, &vo_chn) != SUCCESS)
    {
        divoom_local_movie_play_destroy_pipeline(data);
        return DIVOOM_COMM_FAILURE;
    }

    if (divoom_local_movie_play_create_clock(data) != SUCCESS)
    {
        divoom_local_movie_play_destroy_pipeline(data);
        return DIVOOM_COMM_FAILURE;
    }

    clock_chn.mModId = MOD_ID_CLOCK;
    clock_chn.mDevId = 0;
    clock_chn.mChnId = data->clock_chn;
    if (AW_MPI_SYS_Bind(&clock_chn, &dmx_chn) != SUCCESS ||
        AW_MPI_SYS_Bind(&clock_chn, &vo_chn) != SUCCESS)
    {
        divoom_local_movie_play_destroy_pipeline(data);
        return DIVOOM_COMM_FAILURE;
    }

    divoom_local_movie_play_apply_disp_rect(data);
    return SUCCESS;
}

static ERRORTYPE divoom_local_movie_play_start_pipeline(Pdivoom_local_movie_play_item data)
{
    ERRORTYPE ret;

    ret = AW_MPI_CLOCK_Start(data->clock_chn);
    if (ret != SUCCESS)
    {
        divoom_print("clock start failed: 0x%x\n", ret);
        return ret;
    }

    ret = AW_MPI_VDEC_StartRecvStream(data->vdec_chn);
    if (ret != SUCCESS)
    {
        divoom_print("vdec start failed: 0x%x\n", ret);
        return ret;
    }

    ret = AW_MPI_VO_StartChn(data->vo_layer, data->vo_chn);
    if (ret != SUCCESS)
    {
        divoom_print("vo start failed: 0x%x\n", ret);
        return ret;
    }

    ret = AW_MPI_DEMUX_Start(data->dmx_chn);
    if (ret != SUCCESS)
    {
        divoom_print("demux start failed: 0x%x\n", ret);
    }
    return ret;
}

static ERRORTYPE divoom_local_movie_play_stop_channels(Pdivoom_local_movie_play_item data)
{
    ERRORTYPE ret;

    if (data == NULL)
    {
        return DIVOOM_COMM_FAILURE;
    }
    if (data->vo_layer >= 0 && data->vo_chn >= 0)
    {
        ret = AW_MPI_VO_StopChn(data->vo_layer, data->vo_chn);
        divoom_print("movie stop vo chn ret:0x%x\n", ret);
    }
    if (data->vdec_chn >= 0)
    {
        ret = AW_MPI_VDEC_StopRecvStream(data->vdec_chn);
        divoom_print("movie stop vdec ret:0x%x\n", ret);
    }
    if (data->dmx_chn >= 0)
    {
        ret = AW_MPI_DEMUX_Stop(data->dmx_chn);
        divoom_print("movie stop demux ret:0x%x\n", ret);
    }
    if (data->clock_chn >= 0)
    {
        ret = AW_MPI_CLOCK_Stop(data->clock_chn);
        divoom_print("movie stop clock ret:0x%x\n", ret);
    }
    return SUCCESS;
}

static void divoom_local_movie_play_destroy_pipeline(Pdivoom_local_movie_play_item data)
{
    if (data == NULL)
    {
        return;
    }
    divoom_print("movie destroy begin\n");

    divoom_local_movie_play_stop_channels(data);
    divoom_print("movie destroy stop channels done\n");
    divoom_print("movie destroy by mpp doc order\n");

    if (data->vo_layer >= 0)
    {
        if (data->vo_chn >= 0)
        {
            ERRORTYPE ret = AW_MPI_VO_DestroyChn(data->vo_layer, data->vo_chn);
            divoom_print("movie destroy vo chn ret:0x%x\n", ret);
            data->vo_chn = MM_INVALID_CHN;
        }
        {
            ERRORTYPE ret = AW_MPI_VO_DisableVideoLayer(data->vo_layer);
            divoom_print("movie disable vo layer ret:0x%x\n", ret);
        }
        divoom_system_delay_ms(50);
        data->vo_layer = MM_INVALID_CHN;
        {
            ERRORTYPE ret = AW_MPI_VO_RemoveOutsideVideoLayer(data->ui_layer);
            divoom_print("movie remove outside layer ret:0x%x\n", ret);
        }
        {
            ERRORTYPE ret = AW_MPI_VO_Disable(data->vo_dev);
            divoom_print("movie disable vo dev ret:0x%x\n", ret);
        }
        data->vo_dev = MM_INVALID_DEV;
    }
    divoom_print("movie destroy vo done\n");

    if (data->vdec_chn >= 0)
    {
        ERRORTYPE ret = AW_MPI_VDEC_DestroyChn(data->vdec_chn);
        divoom_print("movie destroy vdec ret:0x%x\n", ret);
        data->vdec_chn = MM_INVALID_CHN;
    }
    if (data->dmx_chn >= 0)
    {
        ERRORTYPE ret = AW_MPI_DEMUX_DestroyChn(data->dmx_chn);
        divoom_print("movie destroy demux ret:0x%x\n", ret);
        data->dmx_chn = MM_INVALID_CHN;
    }
    if (data->clock_chn >= 0)
    {
        ERRORTYPE ret = AW_MPI_CLOCK_DestroyChn(data->clock_chn);
        divoom_print("movie destroy clock ret:0x%x\n", ret);
        data->clock_chn = MM_INVALID_CHN;
    }
    divoom_local_movie_play_restore_ui_layer(data);
    if (data->src_fd >= 0)
    {
        close(data->src_fd);
        data->src_fd = -1;
    }
    divoom_print("movie destroy end free_kb=%ld largest_kb=%ld\n",
        divoom_system_get_ram_free_size(), divoom_system_get_ram_largest_free_block_size());

    data->session_active = 0;
    data->playing = 0;
    data->paused = 0;
    data->loop_enabled = 0;
    data->play_start_flag = 0;
    data->play_start_time_ms = 0;
    data->play_start_time_sec = 0;
}

static int divoom_local_movie_play_start_playing(Pdivoom_local_movie_play_item data, const char *uri, int volume, int play_time)
{
    (void)volume;

    if (data == NULL || uri == NULL || uri[0] == '\0')
    {
        return -1;
    }

    data->loop_enabled = (play_time > 0) ? 1 : 0;
    data->play_time = play_time;
    data->play_start_time_ms = 0;
    data->play_start_time_sec = 0;
    data->play_frame_time = 0;
    data->play_over_flag = 0;

    if (divoom_local_movie_play_open_src(data, uri) != 0)
    {
        return -1;
    }
    if (divoom_local_movie_play_mpp_core_init_once(data) != 0)
    {
        close(data->src_fd);
        data->src_fd = -1;
        return -1;
    }

    divoom_print("movie prepare file=%s free_kb=%ld largest_kb=%ld\n", data->src_path,
        divoom_system_get_ram_free_size(), divoom_system_get_ram_largest_free_block_size());
    if (divoom_local_movie_play_prepare(data) != SUCCESS)
    {
        divoom_local_movie_play_destroy_pipeline(data);
        return -1;
    }
    /* CreateChn 成功不代表首帧缓冲已备齐，不能将此处的空闲量当作稳态余量。 */
    divoom_print("movie decoder prepared codec=%d size=%dx%d vbv=%u free_kb=%ld largest_kb=%ld\n",
        data->vdec_attr.mType, data->vdec_attr.mPicWidth, data->vdec_attr.mPicHeight,
        data->vdec_attr.mBufSize, divoom_system_get_ram_free_size(),
        divoom_system_get_ram_largest_free_block_size());
    if (divoom_local_movie_play_start_pipeline(data) != SUCCESS)
    {
        divoom_local_movie_play_destroy_pipeline(data);
        return -1;
    }

    data->session_active = 1;
    data->playing = 1;
    data->paused = 0;
    divoom_print("movie play started: %s\n", data->src_path);
    return 0;
}

static void divoom_local_movie_play_pause_playing(Pdivoom_local_movie_play_item data)
{
    if (data == NULL || data->session_active == 0 || data->paused)
    {
        return;
    }
    if (data->dmx_chn >= 0)
    {
        AW_MPI_DEMUX_Pause(data->dmx_chn);
    }
    if (data->vdec_chn >= 0)
    {
        AW_MPI_VDEC_Pause(data->vdec_chn);
    }
    if (data->clock_chn >= 0)
    {
        AW_MPI_CLOCK_Pause(data->clock_chn);
    }
    if (data->vo_layer >= 0 && data->vo_chn >= 0)
    {
        AW_MPI_VO_PauseChn(data->vo_layer, data->vo_chn);
    }
    data->paused = 1;
}

static int divoom_local_movie_play_clamp_color(int value)
{
    return value < 0 ? 0 : (value > 255 ? 255 : value);
}

int divoom_local_movie_play_copy_last_frame(uint8 *buffer, int width, int height)
{
    Pdivoom_local_movie_play_item data;
    disp_layer_config config;
    struct ScMemOpsS *memops;
    uint8 *y_plane, *uv_plane;
    int crop_x, crop_y, crop_w, crop_h, stride, uv_stride;
    int x, y, vu, result = 0;

    if (buffer == NULL || width <= 0 || height <= 0 ||
        pdivoom_local_movie_play_global_lock_handle == NULL)
    {
        return 0;
    }
    divoom_system_get_lock(pdivoom_local_movie_play_global_lock_handle);
    data = pdivoom_local_movie_play_handle;
    if (data == NULL || data->session_active == 0 || data->vo_layer < 0 || !data->play_start_flag)
    {
        goto done;
    }
    divoom_local_movie_play_pause_playing(data);
    memset(&config, 0, sizeof(config));
    config.channel = HD2CHN(data->vo_layer);
    config.layer_id = HD2LYL(data->vo_layer);
    if (layer_get_para(&config) != RET_OK || !config.enable ||
        (config.info.fb.format != DISP_FORMAT_YUV420_SP_UVUV &&
         config.info.fb.format != DISP_FORMAT_YUV420_SP_VUVU))
    {
        goto done;
    }
    crop_x = config.info.fb.crop.x >> 32;
    crop_y = config.info.fb.crop.y >> 32;
    crop_w = config.info.fb.crop.width >> 32;
    crop_h = config.info.fb.crop.height >> 32;
    stride = config.info.fb.size[0].width;
    uv_stride = config.info.fb.size[1].width * 2;
    if (crop_x < 0 || crop_y < 0 || crop_w <= 0 || crop_h <= 0 ||
        crop_x + crop_w > stride || (unsigned int)(crop_y + crop_h) > config.info.fb.size[0].height ||
        ((crop_x + crop_w + 1) & ~1) > uv_stride ||
        (unsigned int)(crop_y + crop_h + 1) / 2 > config.info.fb.size[1].height)
    {
        goto done;
    }
    memops = MemAdapterGetOpsS();
    if (memops == NULL || !config.info.fb.addr[0] || !config.info.fb.addr[1])
    {
        goto done;
    }
    y_plane = CdcMemGetVirtualAddressCpu(memops, (void *)(uintptr_t)config.info.fb.addr[0]);
    uv_plane = CdcMemGetVirtualAddressCpu(memops, (void *)(uintptr_t)config.info.fb.addr[1]);
    if (y_plane == NULL || uv_plane == NULL)
    {
        goto done;
    }
    CdcMemFlushCache(memops, y_plane, stride * config.info.fb.size[0].height);
    CdcMemFlushCache(memops, uv_plane, uv_stride * config.info.fb.size[1].height);
    vu = config.info.fb.format == DISP_FORMAT_YUV420_SP_VUVU;
    for (y = 0; y < height; y++)
    {
        int sy = crop_y + (int)((int64_t)y * crop_h / height);
        for (x = 0; x < width; x++)
        {
            int sx = crop_x + (int)((int64_t)x * crop_w / width);
            int uv = (sy / 2) * uv_stride + (sx & ~1);
            int yy = y_plane[sy * stride + sx] - 16;
            int u = uv_plane[uv + vu] - 128;
            int v = uv_plane[uv + !vu] - 128;
            int r = divoom_local_movie_play_clamp_color((298 * yy + 409 * v + 128) >> 8);
            int g = divoom_local_movie_play_clamp_color((298 * yy - 100 * u - 208 * v + 128) >> 8);
            int b = divoom_local_movie_play_clamp_color((298 * yy + 516 * u + 128) >> 8);
            unsigned int pixel = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
            buffer[(y * width + x) * 2] = pixel;
            buffer[(y * width + x) * 2 + 1] = pixel >> 8;
        }
    }
    memset(buffer + width * height * 2, 255, width * height);
    result = 1;
done:
    divoom_system_free_lock(pdivoom_local_movie_play_global_lock_handle);
    return result;
}

int divoom_local_movie_play_is_rendering(void)
{
    Pdivoom_local_movie_play_item data;
    int rendering;

    if (pdivoom_local_movie_play_global_lock_handle == NULL)
    {
        return 0;
    }
    divoom_system_get_lock(pdivoom_local_movie_play_global_lock_handle);
    data = pdivoom_local_movie_play_handle;
    rendering = data && data->play_start_flag && !data->exit_flag && !data->play_over_flag;
    divoom_system_free_lock(pdivoom_local_movie_play_global_lock_handle);
    return rendering;
}

static void divoom_local_movie_play_resume_playing(Pdivoom_local_movie_play_item data)
{
    if (data == NULL || data->session_active == 0 || data->paused == 0)
    {
        return;
    }
    if (data->clock_chn >= 0)
    {
        AW_MPI_CLOCK_Start(data->clock_chn);
    }
    if (data->vdec_chn >= 0)
    {
        AW_MPI_VDEC_StartRecvStream(data->vdec_chn);
    }
    if (data->vo_layer >= 0 && data->vo_chn >= 0)
    {
        AW_MPI_VO_StartChn(data->vo_layer, data->vo_chn);
        AW_MPI_VO_ShowChn(data->vo_layer, data->vo_chn);
    }
    if (data->dmx_chn >= 0)
    {
        AW_MPI_DEMUX_Start(data->dmx_chn);
    }
    data->paused = 0;
}

static void divoom_local_movie_play_stop_playing(Pdivoom_local_movie_play_item data)
{
    if (data == NULL)
    {
        return;
    }
    divoom_local_movie_play_stop_channels(data);
    data->playing = 0;
}

int divoom_local_movie_play_is_playing(void)
{
    if (pdivoom_local_movie_play_handle && pdivoom_local_movie_play_handle->session_active)
    {
        return pdivoom_local_movie_play_handle->playing && (pdivoom_local_movie_play_handle->paused == 0);
    }
    return 0;
}

int divoom_local_movie_play_wait_rendering(unsigned int timeout_ms)
{
    uint32 start_ms = divoom_system_get_tickms();

    while (divoom_system_get_tickms() - start_ms < timeout_ms)
    {
        Pdivoom_local_movie_play_item data = pdivoom_local_movie_play_handle;

        if (data == NULL || data->exit_flag || data->play_over_flag)
        {
            return -1;
        }
        if (data->play_start_flag)
        {
            return 0;
        }
        divoom_system_delay_ms(10);
    }
    divoom_print("movie wait rendering timeout ms=%u\n", timeout_ms);
    return -1;
}

int divoom_local_movie_play_is_idle(void)
{
    int idle;
    if(pdivoom_local_movie_play_global_lock_handle == NULL)
    {
        return pdivoom_local_movie_play_handle == NULL;
    }
    /* The worker clears its handle BEFORE freeing the queue and control.
     * Taking its lock prevents a false idle result during that interval. */
    if(divoom_system_try_lock(pdivoom_local_movie_play_global_lock_handle) != 0)
    {
        return 0;
    }
    idle = pdivoom_local_movie_play_handle == NULL;
    divoom_system_free_lock_direct(pdivoom_local_movie_play_global_lock_handle);
    return idle;
}

int divoom_local_movie_play_wait_stopped(unsigned int timeout_ms)
{
    uint32 start_ms = divoom_system_get_tickms();

    while (!divoom_local_movie_play_is_idle())
    {
        if (divoom_system_get_tickms() - start_ms >= timeout_ms)
        {
            divoom_print("movie wait stopped timeout ms=%u\n", timeout_ms);
            return -1;
        }
        divoom_system_delay_ms(10);
    }
    return 0;
}

int divoom_local_movie_play_get_movie_status(void *handle)
{
    (void)handle;
    return divoom_local_movie_play_is_playing();
}

void *divoom_local_movie_play_play_main_loop(void *param)
{
    void *notice_sem;
    int loop_flag = 1;
    Pdivoom_local_movie_play_item play_handle = (Pdivoom_local_movie_play_item)param;
    Pdivoom_local_movie_cmd_item pcmd_info;

    while (loop_flag)
    {
        if (divoom_queue_items(play_handle->cmd_queue) == 0)
        {
            /* 当前 MPP 吞掉解码 Invalid 错误，不会发业务回调或 VO EOF。
             * 最多一秒检查一次；命令仍即时唤醒，所有通道操作仍由同一锁串行。 */
            divoom_system_get_sem_with_time(play_handle->exit_sem, 1);
        }

        /* Owner shutdown must remain possible when command allocation fails. */
        divoom_system_get_lock(pdivoom_local_movie_play_global_lock_handle);
        if(play_handle->exit_flag)
        {
            divoom_system_free_lock(pdivoom_local_movie_play_global_lock_handle);
            break;
        }
        divoom_system_free_lock(pdivoom_local_movie_play_global_lock_handle);
        pcmd_info = divoom_queue_pop(play_handle->cmd_queue);
        if (pcmd_info == NULL)
        {
            divoom_system_get_lock(pdivoom_local_movie_play_global_lock_handle);
            if (play_handle->session_active && !play_handle->exit_flag &&
                play_handle->vdec_chn >= 0)
            {
                VDEC_CHN_STAT_S decoder_stat;
                ERRORTYPE ret = AW_MPI_VDEC_Query(play_handle->vdec_chn, &decoder_stat);
                if (ret != SUCCESS)
                {
                    divoom_print("movie decoder failed file=%s ret=0x%x first_frame=%d free_kb=%ld largest_kb=%ld; release pipeline\n",
                        play_handle->src_path, ret, play_handle->play_start_flag,
                        divoom_system_get_ram_free_size(), divoom_system_get_ram_largest_free_block_size());
                    /* 不再入队申请内存；沿用本线程的统一停止、销毁和结束通知。 */
                    play_handle->exit_flag = 1;
                    loop_flag = 0;
                }
            }
            divoom_system_free_lock(pdivoom_local_movie_play_global_lock_handle);
            continue;
        }

        divoom_print("movie cmd: %d\n", pcmd_info->cmd_type);
        divoom_system_get_lock(pdivoom_local_movie_play_global_lock_handle);
        switch (pcmd_info->cmd_type)
        {
        case DIVOOM_LOCAL_MOVIE_CMD_PLAY_ERR:
            play_handle->exit_flag = 1;
            loop_flag = 0;
            if (play_handle->pcallback && play_handle->no_cb == 0)
            {
                play_handle->pcallback(play_handle->para, 0);
            }
            break;

        case DIVOOM_LOCAL_MOVIE_CMD_STOP:
            play_handle->exit_flag = 1;
            loop_flag = 0;
            break;

        case DIVOOM_LOCAL_MOVIE_CMD_EOF:
        {
            int next_action = 1;
            /* Ask on the player thread before destroying VO. A zero return
             * requests replay using the existing channels and display layer. */
            if (play_handle->session_active && !play_handle->exit_flag &&
                !play_handle->no_cb && play_handle->pcallback)
            {
                next_action = play_handle->pcallback(play_handle->para, 0);
            }
            if (next_action == DIVOOM_LOCAL_MOVIE_HOLD_LAST_FRAME)
            {
                play_handle->play_over_flag = 1;
                play_handle->loop_enabled = 0;
                play_handle->playing = 0;
                divoom_print("movie hold last frame for next item\n");
                break;
            }
            if (next_action != 0)
            {
                play_handle->exit_flag = 1;
                loop_flag = 0;
                break;
            }
            play_handle->play_over_flag = 0;
            play_handle->play_start_time_ms = 0;
            play_handle->play_start_time_sec = 0;
            play_handle->loop_enabled = (play_handle->play_time > 0) ? 1 : 0;
        }
            /* fall through */
        case DIVOOM_LOCAL_MOVIE_CMD_SEEK_START:
            if (play_handle->session_active && play_handle->exit_flag == 0)
            {
                ERRORTYPE ret;

                /* Quiesce VDEC/VO before stopping DEMUX. Keep VO paused:
                 * StopChn exits the renderer and hides the last frame. */
                ret = AW_MPI_VO_PauseChn(play_handle->vo_layer, play_handle->vo_chn);
                /* VideoRender switches to Idle after notifying EOF while
                 * keeping the renderer and last frame. Pause rejects Idle;
                 * the EOF seek path can already proceed from that state. */
                if (ret == ERR_VO_CHN_INCORRECT_STATE_TRANSITION)
                {
                    divoom_print("movie loop vo already idle at EOF, keep last frame\n");
                    ret = SUCCESS;
                }
                if (ret == SUCCESS)
                {
                    ret = AW_MPI_VDEC_Pause(play_handle->vdec_chn);
                }
                if (ret == SUCCESS)
                {
                    ret = AW_MPI_DEMUX_Stop(play_handle->dmx_chn);
                }
                if (ret == SUCCESS)
                {
                    ret = AW_MPI_CLOCK_Stop(play_handle->clock_chn);
                }
                if (ret != SUCCESS)
                {
                    divoom_print("movie loop pause channels failed: 0x%x\n", ret);
                    divoom_local_movie_play_push_cmd(play_handle, DIVOOM_LOCAL_MOVIE_CMD_STOP);
                    break;
                }
                ret = AW_MPI_DEMUX_Seek(play_handle->dmx_chn, 0);
                if (ret != SUCCESS)
                {
                    divoom_print("movie loop demux seek failed: 0x%x\n", ret);
                    divoom_local_movie_play_push_cmd(play_handle, DIVOOM_LOCAL_MOVIE_CMD_STOP);
                    break;
                }
                if (play_handle->vo_layer >= 0 && play_handle->vo_chn >= 0)
                {
                    AW_MPI_VO_SetStreamEof(play_handle->vo_layer, play_handle->vo_chn, 0);
                    AW_MPI_VO_Seek(play_handle->vo_layer, play_handle->vo_chn);
                }
                if (play_handle->vdec_chn >= 0)
                {
                    AW_MPI_VDEC_SetStreamEof(play_handle->vdec_chn, 0);
                    AW_MPI_VDEC_Seek(play_handle->vdec_chn);
                }
                if (play_handle->clock_chn >= 0)
                {
                    AW_MPI_CLOCK_Seek(play_handle->clock_chn);
                }
                ret = divoom_local_movie_play_start_pipeline(play_handle);
                if (ret != SUCCESS)
                {
                    divoom_print("movie loop restart pipeline failed: 0x%x\n", ret);
                    divoom_local_movie_play_push_cmd(play_handle, DIVOOM_LOCAL_MOVIE_CMD_STOP);
                    break;
                }
                AW_MPI_VO_SetVideoLayerPriority(play_handle->vo_layer, ZORDER_MAX);
                hwd_layer_top(play_handle->vo_layer);
                AW_MPI_VO_ShowChn(play_handle->vo_layer, play_handle->vo_chn);
                divoom_print("movie loop restarted\n");
            }
            else
            {
                
            }
            break;
        default:
            break;
        }
        divoom_system_free(pcmd_info);
        divoom_system_free_lock(pdivoom_local_movie_play_global_lock_handle);
    }

    
    divoom_system_get_lock(pdivoom_local_movie_play_global_lock_handle);
    
    divoom_local_movie_play_stop_playing(play_handle);
    
    divoom_local_movie_play_destroy_pipeline(play_handle);
    
    notice_sem = play_handle->notice_sem;
    
    pdivoom_local_movie_play_handle = NULL;

    if (play_handle->pcallback && play_handle->no_cb == 0)
    {
        
        play_handle->pcallback(play_handle->para, 1);
    }
    
    divoom_system_destroy_sem(play_handle->exit_sem);
    divoom_queue_destroy(play_handle->cmd_queue, NULL);
    if (play_handle->time_handle)
    {
        divoom_software_timer_delete_timer(play_handle->time_handle);
    }
    if (play_handle->movie_name)
    {
        divoom_system_free(play_handle->movie_name);
    }
    divoom_system_free(play_handle);
    if (notice_sem)
    {
        divoom_system_put_sem(notice_sem);
    }
    divoom_system_free_lock(pdivoom_local_movie_play_global_lock_handle);
    divoom_print("exit play\n");
    return NULL;
}

int divoom_local_movie_play_over_callback(void *para)
{
    Pdivoom_local_movie_play_item play_handle = (Pdivoom_local_movie_play_item)para;

    if (play_handle == NULL || pdivoom_local_movie_play_handle == NULL)
    {
        return 0;
    }
    divoom_print("movie play over!%d, %d, %d, %d, %d, %d\n", play_handle->frame_num,
                 divoom_local_movie_play_is_playing(), play_handle->play_start_flag, play_handle->play_time,
                 play_handle->play_frame_time, divoom_system_get_tickms());
    play_handle->play_over_flag = 1;
    play_handle->loop_enabled = 0;
    divoom_local_movie_play_push_cmd(play_handle, DIVOOM_LOCAL_MOVIE_CMD_STOP);
    return 0;
}

void divoom_local_movie_play_play_with_pos(char *music_name, int volume, int play_time,
                                           divoom_local_movie_play_play_get_next *pnext_callback, void *para,
                                           int start_x, int start_y, int width, int high)
{
    Pdivoom_local_movie_play_item play_handle;

    if (pdivoom_local_movie_play_global_exit_sem == NULL)
    {
        pdivoom_local_movie_play_global_exit_sem = divoom_system_create_sem(1, 0);
    }
    if (pdivoom_local_movie_play_global_lock_handle == NULL)
    {
        pdivoom_local_movie_play_global_lock_handle = divoom_system_create_lock();
    }

    malloc_trim(0);
    divoom_system_get_lock(pdivoom_local_movie_play_global_lock_handle);
    divoom_print("we will play file : %s, %d, %d, area : %d, %d, %d,%d\n", music_name, volume, play_time,
                 start_x, start_y, width, high);

    if (music_name == NULL || music_name[0] == '\0')
    {
        divoom_system_free_lock(pdivoom_local_movie_play_global_lock_handle);
        return;
    }

    if (pdivoom_local_movie_play_handle)
    {
        if (pdivoom_local_movie_play_handle->time_handle)
        {
            divoom_software_timer_delete_timer(pdivoom_local_movie_play_handle->time_handle);
            pdivoom_local_movie_play_handle->time_handle = NULL;
        }
        divoom_local_movie_play_destroy_pipeline(pdivoom_local_movie_play_handle);
    }
    else
    {
        pdivoom_local_movie_play_handle = divoom_system_malloc(sizeof(divoom_local_movie_play_item));
        if (pdivoom_local_movie_play_handle == NULL)
        {
            divoom_system_free_lock(pdivoom_local_movie_play_global_lock_handle);
            return;
        }
        memset(pdivoom_local_movie_play_handle, 0, sizeof(divoom_local_movie_play_item));
        pdivoom_local_movie_play_handle->exit_sem = divoom_system_create_sem(1, 0);
        pdivoom_local_movie_play_handle->cmd_queue = divoom_queue_init(100, __func__, __LINE__);
        if (pdivoom_local_movie_play_handle->exit_sem == NULL ||
            pdivoom_local_movie_play_handle->cmd_queue == NULL)
        {
            if (pdivoom_local_movie_play_handle->exit_sem)
            {
                divoom_system_destroy_sem(pdivoom_local_movie_play_handle->exit_sem);
            }
            if (pdivoom_local_movie_play_handle->cmd_queue)
            {
                divoom_queue_destroy(pdivoom_local_movie_play_handle->cmd_queue, NULL);
            }
            divoom_system_free(pdivoom_local_movie_play_handle);
            pdivoom_local_movie_play_handle = NULL;
            divoom_system_free_lock(pdivoom_local_movie_play_global_lock_handle);
            return;
        }
        divoom_local_movie_play_reset_channels(pdivoom_local_movie_play_handle);
        divoom_system_create_task(divoom_local_movie_play_play_main_loop, "movie_play", pdivoom_local_movie_play_handle, DIVOOM_TASK_STACK_LOCAL_MOVIE);
    }

    play_handle = pdivoom_local_movie_play_handle;
    play_handle->start_x = start_x;
    play_handle->start_y = start_y;
    play_handle->width = width;
    play_handle->high = high;
    play_handle->play_time = play_time;
    play_handle->notice_sem = NULL;
    play_handle->play_start_flag = 0;
    play_handle->play_start_time_sec = 0;
    play_handle->play_over_flag = 0;
    play_handle->exit_flag = 0;
    play_handle->no_cb = 0;
    play_handle->pcallback = pnext_callback;
    play_handle->para = para;
    play_handle->frame_num = 0;
    divoom_queue_pop_all(play_handle->cmd_queue, NULL);

    if (play_handle->movie_name)
    {
        divoom_system_free(play_handle->movie_name);
        play_handle->movie_name = NULL;
    }
    if (strstr(music_name, "file://"))
    {
        play_handle->movie_name = divoom_app_com_copy_string(music_name, 0);
    }
    else
    {
        play_handle->movie_name = divoom_system_malloc(strlen(music_name) + 10);
        if (play_handle->movie_name == NULL)
        {
            divoom_system_free_lock(pdivoom_local_movie_play_global_lock_handle);
            return;
        }
        strcpy(play_handle->movie_name, "file://");
        strcat(play_handle->movie_name, music_name);
    }

    if (divoom_local_movie_play_start_playing(play_handle, play_handle->movie_name, volume, play_time) == -1)
    {
        divoom_print("it fails to play movie!\n");
        divoom_system_free_lock(pdivoom_local_movie_play_global_lock_handle);
        divoom_local_movie_play_over_callback(play_handle);
        return;
    }

    divoom_system_free_lock(pdivoom_local_movie_play_global_lock_handle);
}

void divoom_local_movie_play_play(char *music_name, int volume, int play_time,
                                  divoom_local_movie_play_play_get_next *pnext_callback, void *para)
{
    
    divoom_local_movie_play_play_with_pos(music_name, volume, play_time, pnext_callback, para,
                                          0, 0, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
}

void divoom_local_movie_play_stop_movie(void)
{
    if (pdivoom_local_movie_play_handle == NULL ||
        pdivoom_local_movie_play_global_lock_handle == NULL)
        return;

    divoom_system_get_lock(pdivoom_local_movie_play_global_lock_handle);
    if (pdivoom_local_movie_play_handle)
    {
        pdivoom_local_movie_play_handle->no_cb = 0;
        pdivoom_local_movie_play_handle->loop_enabled = 0;
        divoom_local_movie_play_push_cmd(pdivoom_local_movie_play_handle, DIVOOM_LOCAL_MOVIE_CMD_STOP);
    }
    divoom_system_free_lock(pdivoom_local_movie_play_global_lock_handle);
}

void divoom_local_movie_play_stop_movie_no_cb(void)
{
    if (pdivoom_local_movie_play_handle == NULL ||
        pdivoom_local_movie_play_global_lock_handle == NULL)
        return;

    divoom_system_get_lock(pdivoom_local_movie_play_global_lock_handle);
    if (pdivoom_local_movie_play_handle)
    {
        pdivoom_local_movie_play_handle->no_cb = 1;
        pdivoom_local_movie_play_handle->loop_enabled = 0;
        divoom_local_movie_play_push_cmd(pdivoom_local_movie_play_handle, DIVOOM_LOCAL_MOVIE_CMD_STOP);
    }
    divoom_system_free_lock(pdivoom_local_movie_play_global_lock_handle);
}

/* The movie worker holds this lock through its final owner callback and free. */
int divoom_local_movie_play_stop_owner(void *owner, unsigned int timeout_ms)
{
    uint32 start_ms = divoom_system_get_tickms();
    if(owner == NULL) return 0;
    if(pdivoom_local_movie_play_global_lock_handle == NULL)
        return pdivoom_local_movie_play_handle == NULL ? 0 : -1;
    do
    {
        if(divoom_system_try_lock(pdivoom_local_movie_play_global_lock_handle) == 0)
        {
            Pdivoom_local_movie_play_item player = pdivoom_local_movie_play_handle;
            if(player == NULL || player->para != owner)
            {
                divoom_system_free_lock_direct(pdivoom_local_movie_play_global_lock_handle);
                return 0;
            }
            player->no_cb = 1;
            player->loop_enabled = 0;
            player->exit_flag = 1;
            if(player->exit_sem) divoom_system_put_sem(player->exit_sem);
            divoom_system_free_lock_direct(pdivoom_local_movie_play_global_lock_handle);
        }
        if(divoom_system_get_tickms() - start_ms >= timeout_ms) return -1;
        divoom_system_delay_ms(10);
    } while(1);
}

void divoom_local_movie_play_ctrl(uint8 play)
{
    if (pdivoom_local_movie_play_handle == NULL ||
        pdivoom_local_movie_play_global_lock_handle == NULL)
        return;

    divoom_system_get_lock(pdivoom_local_movie_play_global_lock_handle);
    if (pdivoom_local_movie_play_handle)
    {
        if (play)
        {
            divoom_local_movie_play_resume_playing(pdivoom_local_movie_play_handle);
        }
        else
        {
            divoom_local_movie_play_pause_playing(pdivoom_local_movie_play_handle);
        }
    }
    divoom_system_free_lock(pdivoom_local_movie_play_global_lock_handle);
}

int divoom_local_movie_play_have_handle(void)
{
    return pdivoom_local_movie_play_handle != NULL;
}

void divoom_local_movie_play_seek(int start_time)
{
    if (pdivoom_local_movie_play_handle == NULL ||
        pdivoom_local_movie_play_global_lock_handle == NULL)
        return;

    divoom_system_get_lock(pdivoom_local_movie_play_global_lock_handle);
    if (pdivoom_local_movie_play_handle && pdivoom_local_movie_play_handle->session_active)
    {
        AW_MPI_DEMUX_Seek(pdivoom_local_movie_play_handle->dmx_chn, start_time * 1000);
        if (pdivoom_local_movie_play_handle->vdec_chn >= 0)
        {
            AW_MPI_VDEC_SetStreamEof(pdivoom_local_movie_play_handle->vdec_chn, 0);
            AW_MPI_VDEC_Seek(pdivoom_local_movie_play_handle->vdec_chn);
        }
        if (pdivoom_local_movie_play_handle->vo_layer >= 0 && pdivoom_local_movie_play_handle->vo_chn >= 0)
        {
            AW_MPI_VO_SetStreamEof(pdivoom_local_movie_play_handle->vo_layer, pdivoom_local_movie_play_handle->vo_chn, 0);
            AW_MPI_VO_Seek(pdivoom_local_movie_play_handle->vo_layer, pdivoom_local_movie_play_handle->vo_chn);
        }
    }
    divoom_system_free_lock(pdivoom_local_movie_play_global_lock_handle);
}

void divoom_local_movie_play_set_movie_volume(uint8 volume)
{
    s_local_movie_keep_jl_tone_volume = (volume == 0);
    (void)volume;
}

int divoom_local_movie_play_is_album_slideshow(void)
{
    return divoom_local_movie_play_is_playing();
}

int divoom_local_movie_play_should_keep_jl_tone_volume(void)
{
    return s_local_movie_keep_jl_tone_volume && divoom_local_movie_play_is_playing();
}
