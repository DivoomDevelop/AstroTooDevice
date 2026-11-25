
#ifndef __VIDEO_OPS_H__
#define __VIDEO_OPS_H__

#include <plat_type.h>
#include <mm_comm_vi.h>
#include <linux/videodev2.h>
#include <sunxi_camera_v2.h>

struct csi_sync_ctrl;
struct video_selection_rect;
struct osd_fmt;
struct orl_fmt;

typedef struct video_ops_t
{
    ERRORTYPE (*open)(int vipp_id);
    ERRORTYPE (*close)(int vipp_id);
    ERRORTYPE (*set_attr)(int vipp_id, VI_ATTR_S *pstAttr);
    ERRORTYPE (*get_attr)(int vipp_id, VI_ATTR_S *pstAttr);
    ERRORTYPE (*enable_chn)(int vipp_id);
    ERRORTYPE (*disable_chn)(int vipp_id);
    ERRORTYPE (*stream_on)(int vipp_id);
    ERRORTYPE (*stream_off)(int vipp_id);
    int (*get_fd)(int vipp_id);
    ERRORTYPE (*query_cap)(int vipp_id, struct v4l2_capability *cap);
    ERRORTYPE (*query_ctrl)(int vipp_id, struct v4l2_queryctrl *ctrl);
    ERRORTYPE (*get_ctrl)(int vipp_id, int id, int *value);
    ERRORTYPE (*set_ctrl)(int vipp_id, int id, int value);
    ERRORTYPE (*req_bufs)(int vipp_id, struct v4l2_requestbuffers *req);
    ERRORTYPE (*query_buf)(int vipp_id, struct v4l2_buffer *buf);
    ERRORTYPE (*q_buf)(int vipp_id, struct v4l2_buffer *buf);
    ERRORTYPE (*dq_buf)(int vipp_id, struct v4l2_buffer *buf);
    ERRORTYPE (*get_frame)(int vipp_id, VIDEO_FRAME_INFO_S *pFrameInfo);
    ERRORTYPE (*release_frame)(int vipp_id, VIDEO_FRAME_INFO_S *pFrameInfo);
    ERRORTYPE (*set_fps)(int vipp_id, int fps);
    ERRORTYPE (*get_sensor_info)(int vipp_id, int *fps);
    ERRORTYPE (*set_clk)(int vipp_id, unsigned long clk);
    ERRORTYPE (*set_ptn)(int vipp_id, int ptn);
    ERRORTYPE (*set_next_ptn)(int vipp_id, void *ptn_addr);
    ERRORTYPE (*set_freq)(int vipp_id, int freq);
    int (*get_isp_id)(int vipp_id);
    ERRORTYPE (*set_crop)(int vipp_id, struct video_selection_rect rect);
    ERRORTYPE (*set_sync_ctrl)(int vipp_id, struct csi_sync_ctrl *sync);
    ERRORTYPE (*video_set_input_bit_width_start)(int vipp_id, int bitwidth);
    ERRORTYPE (*video_set_input_bit_width_stop)(int vipp_id, int bitwidth);
    ERRORTYPE (*video_set_tdm_drop_frame_num)(int vipp_id, unsigned int drop_num);
    ERRORTYPE (*overlay_set_fmt)(int vipp_id, struct osd_fmt *ofmt);
    ERRORTYPE (*overlay_update)(int vipp_id, int on_off);
    ERRORTYPE (*orl_set_fmt)(int vipp_id, struct orl_fmt *pOrlFmt);
    ERRORTYPE (*set_mirror)(int vipp_id, int value);
    ERRORTYPE (*get_mirror)(int vipp_id, int *value);
    ERRORTYPE (*set_flip)(int vipp_id, int value);
    ERRORTYPE (*get_flip)(int vipp_id, int *value);
} video_ops_t;

#endif
