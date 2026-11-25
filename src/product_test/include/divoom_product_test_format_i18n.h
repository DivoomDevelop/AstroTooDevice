#ifndef DIVOOM_PRODUCT_TEST_FORMAT_I18N_H
#define DIVOOM_PRODUCT_TEST_FORMAT_I18N_H

const char *divoom_product_test_system_check_text(int lang);
/* Built-in maintenance wording; callers already own the normal menu font. */
const char *divoom_product_test_integrity_text(int lang, const char *key);
const char *divoom_product_test_integrity_stage_key(unsigned stage);

/* Factory-local fonts/text. No global menu/language initialization. */
void *divoom_product_test_format_i18n_create(void);
/* Settings/system-check entry uses the same single 28px size, without NEXT text. */
void *divoom_product_test_format_menu_fonts_create(void);
/* One merged 28px snapshot, no whole-font CRC. Both handles own a reference;
 * release both even when their pointer values are equal. LVGL thread only. */
void *divoom_product_test_factory_fonts_create(void **next_font);
/* RAM-only NEXT snapshot: font 28 must open and render required glyphs at 28px. */
void *divoom_product_test_next_font_create(void);
const lv_font_t *divoom_product_test_next_font(void *handle);
void divoom_product_test_format_i18n_destroy(void *handle);
const char *divoom_product_test_format_text(void *handle, const char *key,
    const char *english);
const lv_font_t *divoom_product_test_format_font(void *handle, int title,
    const lv_font_t *fallback);

#endif
