/* 从用户 ato.clip_sequences.v2.1 确定性生成；由 Buddy 真正完成事件衔接。 */
static const struct { const char *clip; const char *next; } divoom_buddy_ato_links[] = {
    {"expression_confused_p01", "expression_confused_p02"},
    {"expression_confused_p02", "expression_confused_p01"},
    {"expression_speaking_p01", "expression_speaking_p02"},
    {"expression_speaking_p02", "expression_speaking_p01"},
    {"action_thinking_p01", "action_thinking_p02"},
    {"action_thinking_p02", "action_thinking_p03"},
    {"action_thinking_p03", "action_thinking_p01"},
    {"action_music_p01", "action_music_p02"},
    {"action_music_p02", "action_music_p01"},
    {"decoration_confused_p01", "decoration_confused_p02"},
    {"decoration_confused_p02", "decoration_confused_p01"},
};
