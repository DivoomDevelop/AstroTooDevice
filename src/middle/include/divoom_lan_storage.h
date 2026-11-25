#ifndef DIVOOM_LAN_STORAGE_H
#define DIVOOM_LAN_STORAGE_H
#include "divoom_type.h"
#define DIVOOM_LAN_ROOT "/userdata/lan"
#define DIVOOM_LAN_SOURCE 254
#define DIVOOM_LAN_LOCAL_IMAGE "UserDefine"
char *divoom_clock_manage_sys_clock_info_to_json(Pdivoom_clock_manage_item pc);
int divoom_clock_manage_load_clock_sys_info(int id, Pdivoom_clock_manage_item pc);
int divoom_lan_check_resources(Pdivoom_clock_manage_item pc, int pin);
void divoom_clock_manage_invalidate_lan(int clock_id);
void divoom_lan_storage_init(void);
int divoom_lan_begin(void);
int divoom_lan_clock_write_begin(void);
void divoom_lan_clock_write_end(void);
int divoom_lan_validate_image(const char *path, int size, char *err, int errlen);
int divoom_lan_finish(int success);
int divoom_lan_copy(const char *source, const char *dest);
int divoom_lan_file_size(char *path);
int divoom_lan_write(const char *dest, const void *data, unsigned size);
void divoom_lan_image_path(uint32 model, uint32 id, char *path);
int divoom_lan_image_exists(uint32 model, uint32 id);
void divoom_lan_clock_path(int id, char *path);
int divoom_lan_clock_exists(int id);
int divoom_lan_save_clock(Pdivoom_clock_manage_item pc);
int divoom_lan_resolve_asset(const char *ref, char *path, unsigned size);
int divoom_lan_move_asset(const char *ref, const char *dest);
char *divoom_lan_dispatch(const char *json, int *handled);
#endif
