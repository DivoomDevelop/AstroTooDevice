#ifndef _divoom_disp_download_file_h_
#define _divoom_disp_download_file_h_

enum{
    DIVOOM_DISP_DWONLOAD_FILE_SYSTEM_FILE_MODE,
    DIVOOM_DISP_DWONLOAD_FILE_USER_DEFINE_MODE,
    DIVOOM_DISP_DWONLOAD_FILE_DOWNLOAD_GIF_MODE,
};
void divoom_disp_download_file_init(uint8 *data);
void divoom_disp_download_file_update(uint8 *data);

void divoom_disp_download_file_close(void);

#endif


