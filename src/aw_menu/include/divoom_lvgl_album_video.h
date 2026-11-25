#ifndef DIVOOM_LVGL_ALBUM_VIDEO_H
#define DIVOOM_LVGL_ALBUM_VIDEO_H

typedef struct {
    void *data[2];
    unsigned int capacity[2];
    /* LVGL lock held, before the worker can reuse the old front buffer. */
    void (*first_frame)(void *owner);
    void *owner;
} divoom_lvgl_album_video_buffers;

/* All calls are made under the LVGL lock. Only one album video may exist.
 * The worker never calls LVGL; stop joins it before freeing frame buffers.
 * play_time_ms is the minimum stay: finish only at EOF after that interval.
 * Zero plays once to EOF; loop repeats until stopped and ignores the interval.
 * A failed/busy start returns NULL and never falls back to the VO player. */
void *divoom_lvgl_album_video_start(void *parent, const char *path,
    int width, int height, unsigned int play_time_ms, int loop);
/* Borrows buffers until stop returns; never frees them or allocates extra RGB buffers. */
void *divoom_lvgl_album_video_start_with_buffers(void *parent, const char *path,
    int width, int height, unsigned int play_time_ms, int loop,
    const divoom_lvgl_album_video_buffers *buffers);
/* True only after the UI timer submits a decoded frame to LVGL. */
int divoom_lvgl_album_video_has_frame(void *handle);
int divoom_lvgl_album_video_finished(void *handle);
void divoom_lvgl_album_video_stop(void *handle);

#endif
