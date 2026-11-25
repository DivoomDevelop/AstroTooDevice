#include "divoom.h"
#include "divoom_product_test_format.h"
#include "divoom_product_test_wifi_input.h"
#include "divoom_user_key.h"

enum { KEY_CHAR, KEY_BACKSPACE, KEY_SHIFT, KEY_SPACE };
typedef struct {
    const char *text;
    short x,y,w,h;
    unsigned char row,col,col_span,kind;
} recovery_key;
static const recovery_key keys[] =
{
    { "a", 6, 130, 38, 44, 0, 0, 1, KEY_CHAR },
    { "b", 48, 130, 38, 44, 0, 1, 1, KEY_CHAR },
    { "c", 90, 130, 38, 44, 0, 2, 1, KEY_CHAR },
    { "d", 132, 130, 38, 44, 0, 3, 1, KEY_CHAR },
    { "e", 174, 130, 38, 44, 0, 4, 1, KEY_CHAR },
    { "f", 216, 130, 38, 44, 0, 5, 1, KEY_CHAR },
    { "g", 258, 130, 38, 44, 0, 6, 1, KEY_CHAR },
    { NULL, 300, 130, 44, 44, 0, 7, 1, KEY_BACKSPACE },
    { "1", 348, 130, 38, 44, 0, 8, 1, KEY_CHAR },
    { "2", 392, 130, 38, 44, 0, 9, 1, KEY_CHAR },
    { "3", 436, 130, 38, 44, 0, 10, 1, KEY_CHAR },
    { "h", 6, 180, 38, 44, 1, 0, 1, KEY_CHAR },
    { "i", 48, 180, 38, 44, 1, 1, 1, KEY_CHAR },
    { "j", 90, 180, 38, 44, 1, 2, 1, KEY_CHAR },
    { "k", 132, 180, 38, 44, 1, 3, 1, KEY_CHAR },
    { "l", 174, 180, 38, 44, 1, 4, 1, KEY_CHAR },
    { "m", 216, 180, 38, 44, 1, 5, 1, KEY_CHAR },
    { "n", 258, 180, 38, 44, 1, 6, 1, KEY_CHAR },
    { "4", 348, 180, 38, 44, 1, 8, 1, KEY_CHAR },
    { "5", 392, 180, 38, 44, 1, 9, 1, KEY_CHAR },
    { "6", 436, 180, 38, 44, 1, 10, 1, KEY_CHAR },
    { "o", 6, 230, 38, 44, 2, 0, 1, KEY_CHAR },
    { "p", 48, 230, 38, 44, 2, 1, 1, KEY_CHAR },
    { "q", 90, 230, 38, 44, 2, 2, 1, KEY_CHAR },
    { "r", 132, 230, 38, 44, 2, 3, 1, KEY_CHAR },
    { "s", 174, 230, 38, 44, 2, 4, 1, KEY_CHAR },
    { "t", 216, 230, 38, 44, 2, 5, 1, KEY_CHAR },
    { "u", 258, 230, 38, 44, 2, 6, 1, KEY_CHAR },
    { "7", 348, 230, 38, 44, 2, 8, 1, KEY_CHAR },
    { "8", 392, 230, 38, 44, 2, 9, 1, KEY_CHAR },
    { "9", 436, 230, 38, 44, 2, 10, 1, KEY_CHAR },
    { "v", 6, 280, 38, 44, 3, 0, 1, KEY_CHAR },
    { "w", 48, 280, 38, 44, 3, 1, 1, KEY_CHAR },
    { "x", 90, 280, 38, 44, 3, 2, 1, KEY_CHAR },
    { "y", 132, 280, 38, 44, 3, 3, 1, KEY_CHAR },
    { "z", 174, 280, 38, 44, 3, 4, 1, KEY_CHAR },
    { NULL, 216, 280, 82, 44, 3, 5, 2, KEY_SHIFT },
    { "0", 348, 280, 82, 44, 3, 8, 2, KEY_CHAR },
    { ".", 436, 280, 38, 44, 3, 10, 1, KEY_CHAR },
    { "-", 6, 330, 38, 44, 4, 0, 1, KEY_CHAR },
    { "=", 49, 330, 38, 44, 4, 1, 1, KEY_CHAR },
    { "%", 92, 330, 38, 44, 4, 2, 1, KEY_CHAR },
    { "*", 135, 330, 38, 44, 4, 3, 1, KEY_CHAR },
    { "/", 178, 330, 38, 44, 4, 4, 1, KEY_CHAR },
    { "\\", 221, 330, 38, 44, 4, 5, 1, KEY_CHAR },
    { "@", 264, 330, 38, 44, 4, 6, 1, KEY_CHAR },
    { "#", 307, 330, 38, 44, 4, 7, 1, KEY_CHAR },
    { "$", 350, 330, 38, 44, 4, 8, 1, KEY_CHAR },
    { "&", 393, 330, 38, 44, 4, 9, 1, KEY_CHAR },
    { "~", 436, 330, 38, 44, 4, 10, 1, KEY_CHAR },
    { "(", 6, 380, 38, 44, 5, 0, 1, KEY_CHAR },
    { ")", 49, 380, 38, 44, 5, 1, 1, KEY_CHAR },
    { "?", 92, 380, 38, 44, 5, 2, 1, KEY_CHAR },
    { "[", 135, 380, 38, 44, 5, 3, 1, KEY_CHAR },
    { "]", 178, 380, 38, 44, 5, 4, 1, KEY_CHAR },
    { "{", 221, 380, 38, 44, 5, 5, 1, KEY_CHAR },
    { "}", 264, 380, 38, 44, 5, 6, 1, KEY_CHAR },
    { "|", 307, 380, 38, 44, 5, 7, 1, KEY_CHAR },
    { "!", 350, 380, 38, 44, 5, 8, 1, KEY_CHAR },
    { ",", 393, 380, 38, 44, 5, 9, 1, KEY_CHAR },
    { ";", 436, 380, 38, 44, 5, 10, 1, KEY_CHAR },
    { "<", 6, 430, 38, 44, 6, 0, 1, KEY_CHAR },
    { ">", 49, 430, 38, 44, 6, 1, 1, KEY_CHAR },
    { NULL, 92, 430, 166, 44, 6, 2, 4, KEY_SPACE },
    { "\"", 264, 430, 38, 44, 6, 6, 1, KEY_CHAR },
    { ":", 307, 430, 38, 44, 6, 7, 1, KEY_CHAR },
    { "'", 350, 430, 38, 44, 6, 8, 1, KEY_CHAR },
    { "+", 393, 430, 38, 44, 6, 9, 1, KEY_CHAR },
    { "-", 436, 430, 38, 44, 6, 10, 1, KEY_CHAR },
};
#define KEY_COUNT ((int)(sizeof(keys)/sizeof(keys[0])))
#define WIFI_VISIBLE_ROWS 6
static lv_obj_t *root, *keyboard, *heading, *field, *buttons[KEY_COUNT], *labels[KEY_COUNT], *tools[2];
static lv_obj_t *network_list, *list_message, *list_rows[WIFI_VISIBLE_ROWS];
static lv_obj_t *list_names[WIFI_VISIBLE_ROWS], *list_signals[WIFI_VISIBLE_ROWS], *rescan;
static divoom_product_test_wifi_scan scan;
static unsigned int seen_generation = ~0U;
static char ssid[33], password[65];
static int focus, toolbar, tool, upper, password_step, list_selected, submitted, input_error;

static lv_obj_t *input_label(lv_obj_t *parent, const char *text, int x, int y, int width)
{
    lv_obj_t *obj = lv_label_create(parent);
    lv_obj_set_style_text_font(obj, lv_obj_get_style_text_font(parent, 0), 0);
    lv_obj_set_style_text_color(obj, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_width(obj, width);
    lv_obj_set_pos(obj, x, y);
    lv_label_set_text(obj, text);
    lv_label_set_long_mode(obj, LV_LABEL_LONG_SCROLL_CIRCULAR);
    return obj;
}

/* Display only: preserve valid printable Unicode so LVGL draws one missing-
 * glyph box per unsupported character. Raw bytes still drive joining/saving. */
static void input_ssid_label(lv_obj_t *label, const char *raw, int index)
{
    const unsigned char *p = (const unsigned char *)raw;
    char text[33];
    size_t i = 0, out = 0;
    while (i < 32 && p[i]) {
        size_t start = i;
        uint32_t cp = p[i++], minimum = 0;
        int extra = 0, j;
        if (cp >= 0xC2 && cp <= 0xDF) { extra = 1; minimum = 0x80; cp &= 0x1F; }
        else if (cp >= 0xE0 && cp <= 0xEF) { extra = 2; minimum = 0x800; cp &= 0x0F; }
        else if (cp >= 0xF0 && cp <= 0xF4) { extra = 3; minimum = 0x10000; cp &= 7; }
        else if (cp >= 0x80) continue;
        for (j = 0; j < extra && i + j < 32 && p[i+j] >= 0x80 && p[i+j] <= 0xBF; ++j)
            cp = (cp << 6) | (p[i+j] & 0x3F);
        if (j != extra) continue;
        i += extra;
        if (cp < minimum || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF) ||
            cp < 0x20 || (cp >= 0x7F && cp <= 0x9F) ||
            (cp >= 0x200B && cp <= 0x200F) || cp == 0xFEFF) continue;
        memcpy(text + out, p + start, i - start); out += i - start;
    }
    while (out && text[out-1] == ' ') --out;
    text[out] = 0;
    if (!out) snprintf(text, sizeof(text), "Network %d", index+1);
    if (strcmp(lv_label_get_text(label), text)) lv_label_set_text(label, text);
}

static void input_refresh(void)
{
    int i, top;
    divoom_product_test_format_get_wifi_scan(&scan);
    if (scan.generation != seen_generation) {
        seen_generation = scan.generation;
        password_step = toolbar = focus = upper = list_selected = submitted = input_error = 0;
        ssid[0] = password[0] = 0;
    }
    if (scan.state != DIVOOM_PT_WIFI_READY) password_step = 0;
    if (submitted) { lv_obj_add_flag(root, LV_OBJ_FLAG_HIDDEN); return; }
    if (!password_step) {
        lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(network_list, LV_OBJ_FLAG_HIDDEN);
        if (list_selected > scan.count) list_selected = scan.count;
        lv_label_set_text(list_message, scan.state == DIVOOM_PT_WIFI_SCANNING ? "Scanning Wi-Fi..." :
            scan.state == DIVOOM_PT_WIFI_SCAN_FAILED ? "Scan failed. Try Rescan." :
            scan.state == DIVOOM_PT_WIFI_EMPTY ? "No Wi-Fi. Select Rescan." :
            scan.connect_failed ? "Connection failed. Retry." : "Select a network");
        top = list_selected < WIFI_VISIBLE_ROWS ? 0 : list_selected-WIFI_VISIBLE_ROWS+1;
        if (top > scan.count-WIFI_VISIBLE_ROWS) top = scan.count-WIFI_VISIBLE_ROWS;
        if (top < 0) top = 0;
        for (i = 0; i < WIFI_VISIBLE_ROWS; ++i) {
            int index = top+i;
            if (scan.state != DIVOOM_PT_WIFI_READY || index >= scan.count) {
                lv_obj_add_flag(list_rows[i], LV_OBJ_FLAG_HIDDEN); continue;
            }
            lv_obj_clear_flag(list_rows[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_border_color(list_rows[i], lv_color_hex(index == list_selected ? 0x35D5E5 : 0x101827), 0);
            lv_obj_set_style_bg_color(list_rows[i], lv_color_hex(index == list_selected ? 0x143342 : 0x101827), 0);
            input_ssid_label(list_names[i], scan.networks[index].ssid, index);
            lv_label_set_text_fmt(list_signals[i], "%d", scan.networks[index].rssi);
        }
        lv_obj_set_style_border_color(rescan, lv_color_hex(list_selected == scan.count ? 0x35D5E5 : 0x405571), 0);
        return;
    }
    lv_obj_add_flag(network_list, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    input_ssid_label(heading, ssid, list_selected);
    if (strcmp(lv_label_get_text(field), input_error ? "Min 8 characters" : password))
        lv_label_set_text(field, input_error ? "Min 8 characters" : password);
    lv_obj_set_style_text_color(field, lv_color_hex(input_error ? 0xFFB454 : 0x35D5E5), 0);
    for (i = 0; i < 2; ++i)
        lv_obj_set_style_border_color(tools[i], lv_color_hex(toolbar && tool == i ? 0x35D5E5 : 0x405571), 0);
    for (i = 0; i < KEY_COUNT; ++i) {
        char text[2] = {0};
        lv_obj_set_style_border_color(buttons[i], lv_color_hex(!toolbar && focus == i ? 0x35D5E5 : 0x405571), 0);
        lv_obj_set_style_bg_color(buttons[i], lv_color_hex(!toolbar && focus == i ? 0x35D5E5 : 0x182538), 0);
        lv_obj_set_style_text_color(labels[i], lv_color_hex(!toolbar && focus == i ? 0x101827 : 0xFFFFFF), 0);
        if (keys[i].kind == KEY_CHAR && keys[i].text && keys[i].text[0] >= 'a' && keys[i].text[0] <= 'z') {
            text[0] = keys[i].text[0] - (upper ? 'a'-'A' : 0);
            lv_label_set_text(labels[i], text);
        }
    }
}

void divoom_product_test_wifi_input_show(lv_obj_t *parent, int visible)
{
    int i;
    if (!root && !visible) return;
    if (!root) {
        root = lv_obj_create(parent);
        lv_obj_remove_style_all(root);
        lv_obj_set_style_text_font(root, lv_obj_get_style_text_font(parent, 0), 0);
        lv_obj_set_size(root, 480, 480);
        lv_obj_set_pos(root, 0, 0);
        lv_obj_set_style_bg_color(root, lv_color_hex(0x101827), 0);
        lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
        lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
        network_list = lv_obj_create(root);
        lv_obj_remove_style_all(network_list);
        lv_obj_set_size(network_list, 480, 480);
        lv_obj_clear_flag(network_list, LV_OBJ_FLAG_SCROLLABLE);
        input_label(network_list, "Choose Wi-Fi", 24, 0, 432);
        list_message = input_label(network_list, "Scanning Wi-Fi...", 24, 44, 432);
        lv_obj_set_style_text_color(list_message, lv_color_hex(0xA4B4CC), 0);
        input_label(network_list, "dBm", 392, 58, 64);
        for (i = 0; i < WIFI_VISIBLE_ROWS; ++i) {
            list_rows[i] = lv_obj_create(network_list);
            lv_obj_remove_style_all(list_rows[i]);
            lv_obj_clear_flag(list_rows[i], LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_size(list_rows[i], 432, 45);
            lv_obj_set_pos(list_rows[i], 24, 104+i*48);
            lv_obj_set_style_bg_opa(list_rows[i], LV_OPA_COVER, 0);
            lv_obj_set_style_radius(list_rows[i], 8, 0);
            lv_obj_set_style_border_width(list_rows[i], 2, 0);
            list_names[i] = input_label(list_rows[i], "", 12, 14, 316);
            lv_label_set_long_mode(list_names[i], LV_LABEL_LONG_DOT);
            lv_obj_set_height(list_names[i], lv_obj_get_style_text_font(list_names[i], 0)->line_height);
            lv_obj_set_y(list_names[i], (45-lv_obj_get_style_text_font(list_names[i], 0)->line_height)/2);
            list_signals[i] = input_label(list_rows[i], "", 352, 14, 64);
            lv_obj_set_style_text_align(list_signals[i], LV_TEXT_ALIGN_RIGHT, 0);
            lv_obj_set_y(list_signals[i], (45-lv_obj_get_style_text_font(list_signals[i], 0)->line_height)/2);
        }
        rescan = lv_obj_create(network_list);
        lv_obj_remove_style_all(rescan);
        lv_obj_clear_flag(rescan, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_pos(rescan, 24, 392);
        lv_obj_set_size(rescan, 432, 40);
        lv_obj_set_style_radius(rescan, 8, 0);
        lv_obj_set_style_border_width(rescan, 2, 0);
        lv_obj_t *caption = input_label(rescan, "Rescan", 8, 12, 416);
        lv_obj_set_style_text_align(caption, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(caption);
        caption = input_label(network_list, "UP/DOWN  ENTER", 24, 434, 432);
        lv_obj_set_style_text_color(caption, lv_color_hex(0xA4B4CC), 0);
        lv_obj_set_style_text_align(caption, LV_TEXT_ALIGN_CENTER, 0);
        keyboard = lv_obj_create(root);
        lv_obj_remove_style_all(keyboard);
        lv_obj_set_size(keyboard, 480, 480);
        lv_obj_clear_flag(keyboard, LV_OBJ_FLAG_SCROLLABLE);
        input_label(keyboard, "SSID:", 8, 0, 80);
        heading = input_label(keyboard, "", 104, 0, 368);
        lv_label_set_long_mode(heading, LV_LABEL_LONG_SCROLL_CIRCULAR);
        input_label(keyboard, "PASSWORD", 8, 44, 128);
        field = input_label(keyboard, "", 152, 42, 320);
        lv_obj_set_style_bg_color(field, lv_color_hex(0x182538), 0);
        lv_obj_set_style_bg_opa(field, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(field, 6, 0);
        lv_obj_set_style_pad_all(field, 2, 0);
        lv_label_set_long_mode(field, LV_LABEL_LONG_SCROLL_CIRCULAR);
        for (i = 0; i < 2; ++i) {
            tools[i] = lv_obj_create(keyboard);
            lv_obj_remove_style_all(tools[i]);
            lv_obj_set_size(tools[i], i ? 152 : 120, 38);
            lv_obj_set_pos(tools[i], i ? 320 : 184, 88);
            lv_obj_set_style_radius(tools[i], 6, 0);
            lv_obj_set_style_border_width(tools[i], 2, 0);
            lv_obj_t *text = input_label(tools[i], i ? "Confirm" : "Clear", 4, 11, i ? 144 : 112);
            lv_obj_set_style_text_align(text, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_center(text);
        }
        input_label(keyboard, "ESC: Back", 8, 88, 160);
        for (i = 0; i < KEY_COUNT; ++i) {
            const char *text = keys[i].kind == KEY_BACKSPACE ? "<" :
                keys[i].kind == KEY_SHIFT ? "Aa" : keys[i].kind == KEY_SPACE ? "Space" : keys[i].text;
            buttons[i] = lv_obj_create(keyboard);
            lv_obj_remove_style_all(buttons[i]);
            lv_obj_clear_flag(buttons[i], LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_pos(buttons[i], keys[i].x, keys[i].y);
            lv_obj_set_size(buttons[i], keys[i].w, keys[i].h);
            lv_obj_set_style_border_width(buttons[i], 2, 0);
            lv_obj_set_style_radius(buttons[i], 5, 0);
            lv_obj_set_style_bg_opa(buttons[i], LV_OPA_COVER, 0);
            labels[i] = input_label(buttons[i], text ? text : "", 0, 0, keys[i].w-4);
            lv_obj_set_style_text_align(labels[i], LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_center(labels[i]);
        }
    }
    if (visible) {
        lv_obj_clear_flag(root, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(root);
        input_refresh();
    } else lv_obj_add_flag(root, LV_OBJ_FLAG_HIDDEN);
}

int divoom_product_test_wifi_input_key(int key)
{
    int i;
    char *text = password;
    size_t length = strlen(text), limit = 64;
    if (!root) return 0;
    input_refresh();
    if (submitted) return 1;
    if (!password_step) {
        if (scan.state == DIVOOM_PT_WIFI_SCANNING) return 1;
        if (key == DIVOOM_KEY_ASTROTOO_KEY_UP)
            list_selected = (list_selected+scan.count)%(scan.count+1);
        else if (key == DIVOOM_KEY_ASTROTOO_KEY_DOWN)
            list_selected = (list_selected+1)%(scan.count+1);
        else if (key == DIVOOM_KEY_ASTROTOO_KEY_ENTER) {
            if (list_selected == scan.count) divoom_product_test_format_rescan_wifi();
            else {
                snprintf(ssid, sizeof(ssid), "%s", scan.networks[list_selected].ssid);
                password[0] = 0; input_error = 0;
                if (scan.networks[list_selected].open) {
                    submitted = divoom_product_test_format_submit_wifi(ssid, "");
                } else { password_step = 1; focus = toolbar = upper = 0; }
            }
        }
        input_refresh(); return 1;
    }
    if (key == DIVOOM_KEY_ASTROTOO_KEY_ESC) {
        password_step = 0; toolbar = 0;
    } else if (key == DIVOOM_KEY_ASTROTOO_KEY_UP || key == DIVOOM_KEY_ASTROTOO_KEY_DOWN) {
        int row = keys[focus].row + (key == DIVOOM_KEY_ASTROTOO_KEY_UP ? -1 : 1);
        int col = keys[focus].col;
        if (toolbar) { toolbar = 0; focus = 0; }
        else if (row < 0) { toolbar = 1; tool = 1; }
        else if (row <= 6) {
            int nearest = -1, distance = 100;
            for (i = 0; i < KEY_COUNT; ++i) if (keys[i].row == row) {
                int d = col < keys[i].col ? keys[i].col-col :
                    col >= keys[i].col+keys[i].col_span ? col-(keys[i].col+keys[i].col_span-1) : 0;
                if (d < distance) { nearest = i; distance = d; }
            }
            if (nearest >= 0) focus = nearest;
        }
    } else if (key == DIVOOM_KEY_ASTROTOO_KEY_LEFT || key == DIVOOM_KEY_ASTROTOO_KEY_RIGHT) {
        int direction = key == DIVOOM_KEY_ASTROTOO_KEY_LEFT ? -1 : 1;
        if (toolbar) tool = !tool;
        else {
            int next = focus;
            do { next = (next+direction+KEY_COUNT)%KEY_COUNT; } while (keys[next].row != keys[focus].row);
            focus = next;
        }
    } else if (key == DIVOOM_KEY_ASTROTOO_KEY_ENTER) {
        if (toolbar) {
            if (!tool) { text[0] = 0; input_error = 0; }
            else {
                if (length < 8) { input_error = 1; input_refresh(); return 1; }
                submitted = divoom_product_test_format_submit_wifi(ssid, password);
            }
        } else if (keys[focus].kind == KEY_BACKSPACE) {
            if (length) text[length-1] = 0;
            input_error = 0;
        } else if (keys[focus].kind == KEY_SHIFT) upper = !upper;
        else if (length < limit) {
            char ch = keys[focus].kind == KEY_SPACE ? ' ' : keys[focus].text[0];
            if (upper && ch >= 'a' && ch <= 'z') ch -= 'a'-'A';
            text[length] = ch; text[length+1] = 0; input_error = 0;
        }
    }
    input_refresh();
    return 1;
}

void divoom_product_test_wifi_input_destroy(void)
{
    if (root) lv_obj_delete(root);
    root = NULL;
    memset(ssid, 0, sizeof(ssid)); memset(password, 0, sizeof(password));
    focus = toolbar = tool = upper = password_step = list_selected = submitted = input_error = 0;
    seen_generation = ~0U;
    memset(&scan, 0, sizeof(scan));
}
