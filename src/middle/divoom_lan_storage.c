/* LAN-owned files are outside the evictable download cache. */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "divoom.h"
#include "divoom_lan_storage.h"
#include "divoom_clock_manage.h"
#include "divoom_app_com.h"
#include "divoom_disp_clock.h"
typedef struct lan_undo {
    char path[256];
    int existed, committed;
    struct lan_undo *next;
} lan_undo;
typedef struct lan_temp {
    char path[256];
    char dest[256];
    struct lan_temp *next;
} lan_temp;
static void *lan_lock;
static void *lan_clock_write_lock;
static lan_undo *undo;
static lan_temp *consumed;
static int failed;
void divoom_lan_storage_init(void) {
    if(!lan_lock) lan_lock = divoom_system_create_lock();
    if(!lan_clock_write_lock) lan_clock_write_lock = divoom_system_create_lock();
    /* local:// files are upload staging, never durable assets. */
    divoom_port_remove_tree(DIVOOM_LAN_ROOT "/uploads");
    divoom_port_mkdir_p(DIVOOM_LAN_ROOT "/uploads");
}
int divoom_lan_clock_write_begin(void) {
    if(!lan_clock_write_lock) return 0;
    divoom_system_get_lock(lan_clock_write_lock);
    if(divoom_lan_begin()) return 1;
    divoom_system_free_lock(lan_clock_write_lock);
    return 0;
}
void divoom_lan_clock_write_end(void) {
    if(lan_clock_write_lock) divoom_system_free_lock(lan_clock_write_lock);
}
int divoom_lan_begin(void) {
    if(!lan_lock) return 0;
    divoom_system_get_lock(lan_lock);
    undo = NULL;
    consumed = NULL;
    failed = 0;
    return 1;
}
static int copy_checked(const char *src, const char *dst) {
    FILE *in = divoom_port_file_open(src, "rb"), *out = NULL;
    unsigned char buf[2048];
    size_t n;
    int ok = 0;
    if(!in) return 0;
    out = divoom_port_file_open(dst, "wb");
    if(!out) { fclose(in); return 0; }
    while((n = fread(buf, 1, sizeof buf, in)) != 0)
        if(fwrite(buf, 1, n, out) != n) goto done;
    ok = !ferror(in) && !ferror(out);
done:
    if(fclose(out) != 0) ok = 0;
    fclose(in);
    if(!ok) unlink(dst);
    return ok;
}
static int remember(const char *path) {
    lan_undo *p;
    char backup[272];
    if(failed || strlen(path) >= sizeof p->path) return 0;
    for(p = undo; p; p = p->next) if(!strcmp(p->path, path)) return 1;
    p = divoom_system_malloc(sizeof *p);
    if(!p) { failed = 1; return 0; }
    p->committed = 0;
    strcpy(p->path, path);
    p->existed = divoom_port_file_exists(path);
    snprintf(backup, sizeof backup, "%s.lanbak", path);
    divoom_app_com_mk_dir_for_file_path(path);
    if(p->existed && !copy_checked(path, backup)) {
        divoom_system_free(p); failed = 1; return 0;
    }
    p->next = undo; undo = p;
    return 1;
}
int divoom_lan_copy(const char *source, const char *dest) {
    char tmp[272];
    if(!remember(dest)) return 0;
    snprintf(tmp, sizeof tmp, "%s.lantmp", dest);
    if(!copy_checked(source, tmp)) {
        unlink(tmp); failed = 1; return 0;
    }
    return 1;
}
int divoom_lan_write(const char *dest, const void *data, unsigned size) {
    FILE *f;
    char tmp[272];
    int ok;
    if(!remember(dest)) return 0;
    snprintf(tmp, sizeof tmp, "%s.lantmp", dest);
    f = divoom_port_file_open(tmp, "wb");
    if(!f) { failed = 1; return 0; }
    ok = fwrite(data, 1, size, f) == size;
    if(fclose(f) != 0) ok = 0;
    if(!ok) {
        unlink(tmp); failed = 1; return 0;
    }
    return 1;
}
int divoom_lan_finish(int success) {
    lan_undo *p, *ordered = NULL;
    lan_temp *t, *moved;
    char backup[272], tmp[272];
    int ok = success && !failed;
    /* Files are staged first; publish config last, after all asset validation. */
    while((p = undo) != NULL) { undo = p->next; p->next = ordered; ordered = p; }
    if(ok) {
        for(p = ordered; p; p = p->next) {
            snprintf(tmp, sizeof tmp, "%s.lantmp", p->path);
            if(p->existed && unlink(p->path) != 0) {
                divoom_print("LAN publish could not remove old file: %s\n", p->path);
                ok = 0;
                break;
            }
            /* Mark after removing the old file so rollback also covers rename failure. */
            p->committed = 1;
            if(rename(tmp, p->path) != 0) {
                divoom_print("LAN publish rename failed: %s -> %s\n", tmp, p->path);
                ok = 0;
                break;
            }
        }
    }
    while((p = ordered) != NULL) {
        ordered = p->next;
        snprintf(backup, sizeof backup, "%s.lanbak", p->path);
        snprintf(tmp, sizeof tmp, "%s.lantmp", p->path);
        moved = NULL;
        for(t = consumed; t; t = t->next)
            if(!strcmp(t->dest, p->path)) { moved = t; break; }
        if(!ok && moved) {
            const char *asset = p->committed ? p->path : tmp;
            unlink(moved->path);
            if(rename(asset, moved->path) != 0)
                divoom_print("LAN rollback could not restore temporary upload: %s\n", moved->path);
        } else unlink(tmp);
        if(!ok && p->committed) {
            if(p->existed) {
                if(!moved) unlink(p->path);
                if(rename(backup, p->path) != 0)
                    divoom_print("LAN rollback failed; backup retained: %s\n", backup);
            } else unlink(p->path);
        } else if(p->existed) unlink(backup);
        divoom_system_free(p);
    }
    while((t = consumed) != NULL) {
        consumed = t->next;
        divoom_system_free(t);
    }
    divoom_system_free_lock(lan_lock);
    return ok;
}
int divoom_lan_file_size(char *path) {
    lan_undo *p;
    char tmp[272];
    for(p = undo; p; p = p->next) if(!strcmp(p->path, path)) {
        snprintf(tmp, sizeof tmp, "%s.lantmp", path);
        return divoom_app_com_get_file_size(tmp);
    }
    return divoom_app_com_get_file_size(path);
}
void divoom_lan_image_path(uint32 model, uint32 id, char *path) {
    sprintf(path, DIVOOM_LAN_ROOT "/images/%u-%u.bin", (unsigned)model, (unsigned)id);
}
int divoom_lan_image_exists(uint32 model, uint32 id) {
    char path[128];
    divoom_lan_image_path(model, id, path);
    return divoom_port_file_exists(path);
}
void divoom_lan_clock_path(int id, char *path) {
    sprintf(path, DIVOOM_LAN_ROOT "/clocks/clock%d.cfg", id);
}
int divoom_lan_clock_exists(int id) {
    char path[128];

    if(id >= DIVOOM_USER_START_CLOCK_ID && id < DIVOOM_CLOUD_START_CLOCK_ID)
    {
        divoom_lan_clock_path(id, path);
        return divoom_port_file_exists(path);
    }
    else
    {
        return 0;
    }
}
int divoom_lan_save_clock(Pdivoom_clock_manage_item pc) {
    char path[128];
    char *json = divoom_clock_manage_sys_clock_info_to_json(pc);
    int ok;
    if(!json) return 0;
    divoom_lan_clock_path(pc->clock_id, path);
    ok = divoom_lan_write(path, json, strlen(json));
    divoom_system_free(json);
    return ok;
}
int divoom_lan_resolve_asset(const char *ref, char *path, unsigned size) {
    const char *leaf;
    unsigned n;
    if(!ref || strncmp(ref, "local://", 8)) return 0;
    leaf = ref + 8;
    n = strlen(leaf);
    if(n == 0 || n > 80 || strstr(leaf, "..") || strspn(leaf,
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-." ) != n)
        return 0;
    if(snprintf(path, size, DIVOOM_LAN_ROOT "/uploads/%s", leaf) >= (int)size) return 0;
    return divoom_port_file_exists(path);
}

int divoom_lan_move_asset(const char *ref, const char *dest) {
    lan_temp *p;
    char path[256], tmp[272];
    if(!divoom_lan_resolve_asset(ref, path, sizeof path)) return 0;
    for(p = consumed; p; p = p->next) if(!strcmp(p->path, path)) return 0;
    if(!remember(dest)) return 0;
    p = divoom_system_malloc(sizeof *p);
    if(!p) { failed = 1; return 0; }
    snprintf(tmp, sizeof tmp, "%s.lantmp", dest);
    unlink(tmp);
    if(rename(path, tmp) != 0) {
        divoom_system_free(p); failed = 1; return 0;
    }
    strcpy(p->path, path);
    strcpy(p->dest, dest);
    p->next = consumed;
    consumed = p;
    return 1;
}

/* Resolve only existing files. No download helper is called by this validation. */
static int lan_pin_image(unsigned model, unsigned id, char *ref, int pin) {
    char source[256], dest[256];
    int temporary = 0, size = 0;
    if(!ref[0]) return 1;
    if(!strncmp(ref, "local://", 8)) {
        temporary = 1;
        if(!divoom_lan_resolve_asset(ref, source, sizeof source)) return 0;
    } else {
        divoom_lan_image_path(model, id, source);
        size = divoom_lan_file_size(source);
        if(size <= 0)
            divoom_manage_image_download_get_file_name(model, id, source);
    }
    if(size <= 0 && divoom_lan_file_size(source) <= 0) return 0;
    if(pin) {
        divoom_lan_image_path(model, id, dest);
        if(temporary) {
            if(!divoom_lan_move_asset(ref, dest)) return 0;
        } else if(strcmp(source, dest) && !divoom_lan_copy(source, dest)) return 0;
        strcpy(ref, DIVOOM_LAN_LOCAL_IMAGE);
    }
    return 1;
}
static int lan_check_resources(Pdivoom_clock_manage_item pc, int pin, int depth) {
    int i, id;
    if(!pc || depth > 8 || !lan_pin_image(DIVOOM_MANAGE_IMAGE_CLOCK_DIAL,
        DIVOOM_MANAGE_IMAGE_ID_BASE_TIME_DIAL_PIC + pc->clock_id, pc->image_addr, pin)) return 0;
    for(i = 0; i < pc->disp_item_num; i++) {
        if(pc->pdisp_item_list[i].type >= DIVOOM_CLOCK_DISP_SUPPORT_DAIL_COMPONENT_START_ID &&
           pc->pdisp_item_list[i].type < DIVOOM_CLOCK_DISP_SUPPORT_DAIL_COMPONENT_END_ID &&
           pc->app_num == 1 && pc->papp_list && pc->papp_list[0].Config) {
            int child_id = 0, ok;
            Pdivoom_clock_manage_item child;
            divoom_app_com_get_int_from_config(pc->papp_list[0].Config,
                pc->pdisp_item_list[i].item_id, &child_id);
            if(child_id > 0) {
                child = divoom_clock_manage_get_clock_info_no_download(child_id, 0, 0);
                if(!child) return 0;
                ok = lan_check_resources(child, pin, depth + 1);
                if(ok && pin) ok = divoom_lan_save_clock(child);
                divoom_clock_manage_free_clock(child);
                if(!ok) return 0;
            }
        }
        if(!pc->pdisp_item_list[i].img_addr[0]) continue;
        id = divoom_clock_manage_get_disp_item_image(pc->clock_id, pc->pdisp_item_list[i].type);
        if(id < 0 || !lan_pin_image(DIVOOM_MANAGE_IMAGE_USER_DEFINE_CLOCK_PIC, id,
            pc->pdisp_item_list[i].img_addr, pin)) return 0;
    }
    return 1;
}

int divoom_lan_check_resources(Pdivoom_clock_manage_item pc, int pin) {
    return lan_check_resources(pc, pin, 0);
}
