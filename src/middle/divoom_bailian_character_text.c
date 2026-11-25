#include "divoom_bailian_character_text.h"

int divoom_bailian_character_text_init(const divoom_bailian_character_text_config *config)
{
    (void)config;
    return DIVOOM_BAILIAN_CHARACTER_TEXT_DISABLED;
}
int divoom_bailian_character_text_init_from_env(void)
{
    return DIVOOM_BAILIAN_CHARACTER_TEXT_DISABLED;
}
int divoom_bailian_character_text_request(int role_id, const char *input_text,
    uint16 max_chars, uint16 max_bytes, divoom_bailian_character_text_callback callback,
    void *user_data, uint32 *request_id)
{
    (void)role_id; (void)input_text; (void)max_chars; (void)max_bytes;
    (void)callback; (void)user_data;
    if (request_id) *request_id = 0;
    return DIVOOM_BAILIAN_CHARACTER_TEXT_DISABLED;
}
void divoom_bailian_character_text_shutdown(void) {}
const char *divoom_bailian_character_text_get_role_name(int role_id)
{ (void)role_id; return ""; }
const char *divoom_bailian_character_text_get_role_summary(int role_id)
{ (void)role_id; return ""; }
const char *divoom_bailian_character_text_get_system_language(void)
{ return ""; }
