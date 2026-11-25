/*
 * 蓝牙 0x04/0x57 歌词解析：纯函数，无 OS 依赖。
 * type1 暂存，type2 到齐后按突发成对分类。now_ms 由调用方传入。
 * DIVOOM_BT_LYRIC_COMBINED_AS_TITLE=1：组合 type2 整段当歌名，type1 当词。
 *    鸿蒙酷狗裸 `-` 在已是组合时也不拆。
 * =0：组合串仍拆开并锚定左右；鸿蒙酷狗裸 `-` 按左歌手右歌名拆。
 */
#include "divoom_bt_lyric_parse.h"

#include <string.h>

/*
 * feed / settle 工作区（.bss，不占 btcore_work 栈）。
 * 仅允许一次 feed 串行使用，勿并行、勿嵌套调用 parse_feed。
 * 0–2 整次 feed 存活；3–7 仅 settle_pair。
 */
enum {
	LYRIC_WORK_OLD_TITLE = 0,
	LYRIC_WORK_OLD_ARTIST,
	LYRIC_WORK_OLD_LYRIC,
	LYRIC_WORK_T2_TITLE,
	LYRIC_WORK_T2_ARTIST,
	LYRIC_WORK_SLASH_ARTIST,
	LYRIC_WORK_SLASH_TITLE,
	LYRIC_WORK_SETTLE_OLD_TITLE,
	LYRIC_WORK_SLOTS
};
static char s_lyric_work[LYRIC_WORK_SLOTS][DIVOOM_BT_LYRIC_STR_LEN] = { { 0 } };

static void lyric_copy(char *dst, size_t dst_sz, const char *src)
{
	if (!dst || dst_sz == 0)
		return;
	if (!src) {
		dst[0] = '\0';
		return;
	}
	strncpy(dst, src, dst_sz - 1);
	dst[dst_sz - 1] = '\0';
}

static void lyric_trim_inplace(char *s)
{
	char *start;
	char *end;

	if (!s || !s[0])
		return;
	start = s;
	while (*start == ' ' || *start == '\t')
		start++;
	if (start != s)
		memmove(s, start, strlen(start) + 1);
	end = s + strlen(s);
	while (end > s && (end[-1] == ' ' || end[-1] == '\t'))
		end--;
	*end = '\0';
}

static void lyric_fold(const char *src, char *dst, size_t dst_sz)
{
	size_t i = 0;
	const unsigned char *p = (const unsigned char *)src;

	if (!dst || dst_sz == 0)
		return;
	dst[0] = '\0';
	if (!src)
		return;
	while (*p && i + 1 < dst_sz) {
		unsigned char c = *p++;

		if (c >= 'A' && c <= 'Z')
			c = (unsigned char)(c + 32);
		if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c >= 0x80)
			dst[i++] = (char)c;
	}
	dst[i] = '\0';
}

static int divoom_bt_lyric_line_is_credit(const char *text)
{
	static const char *zh[] = {
		"作词", "作曲", "编曲", "制作", "填词", "封面", "母带",
		"词：", "词:", "曲：", "曲:"
	};
	char t[DIVOOM_BT_LYRIC_STR_LEN];
	const char *p = text;
	int i;

	if (!p || !p[0])
		return 0;
	while (*p == ' ' || *p == '\t')
		p++;
	for (i = 0; i < (int)(sizeof(zh) / sizeof(zh[0])); i++) {
		size_t n = strlen(zh[i]);

		if (strncmp(p, zh[i], n) == 0)
			return 1;
	}
	lyric_fold(text, t, sizeof(t));
	if (strncmp(t, "writtenby", 9) == 0 ||
		strncmp(t, "lyricsby", 8) == 0 ||
		strncmp(t, "composedby", 10) == 0 ||
		strncmp(t, "producedby", 10) == 0)
		return 1;
	return 0;
}

static int fold_concat_eq(const char *a, const char *b, const char *t)
{
	char buf[DIVOOM_BT_LYRIC_STR_LEN * 2];
	size_t na;
	size_t nb;

	if (!a || !b || !t)
		return 0;
	na = strlen(a);
	nb = strlen(b);
	if (na + nb >= sizeof(buf))
		return 0;
	memcpy(buf, a, na);
	memcpy(buf + na, b, nb + 1);
	return strcmp(buf, t) == 0;
}

static int divoom_bt_lyric_line_is_header(const char *text, const char *artist, const char *track)
{
	char t[DIVOOM_BT_LYRIC_STR_LEN];
	char a[DIVOOM_BT_LYRIC_STR_LEN];
	char tr[DIVOOM_BT_LYRIC_STR_LEN];

	if (!text || !text[0])
		return 1;
	if (divoom_bt_lyric_line_is_credit(text))
		return 1;
	lyric_fold(text, t, sizeof(t));
	lyric_fold(artist, a, sizeof(a));
	lyric_fold(track, tr, sizeof(tr));
	if (!t[0])
		return 1;
	if (tr[0] && strcmp(t, tr) == 0)
		return 1;
	if (a[0] && strcmp(t, a) == 0)
		return 1;
	if (a[0] && tr[0] &&
		(fold_concat_eq(a, tr, t) || fold_concat_eq(tr, a, t)))
		return 1;
	return 0;
}

#if !DIVOOM_BT_LYRIC_COMBINED_AS_TITLE
static void split_title(const char *title, char *song_name, size_t song_sz,
	char *singer, size_t singer_sz)
{
	const char *seps[] = {
		" · ", "\xC2\xB7", " • ", "\xE2\x80\xA2",
		"--", " — ", " - ", " – ", " / ", " | ", " ? "
	};
	char tmp[DIVOOM_BT_LYRIC_STR_LEN];
	char *pos;
	int i;

	if (!title || !song_name || !singer || song_sz == 0 || singer_sz == 0)
		return;
	lyric_copy(song_name, song_sz, title);
	singer[0] = '\0';
	lyric_trim_inplace(song_name);
	if (!song_name[0])
		return;
	lyric_copy(tmp, sizeof(tmp), song_name);
	for (i = 0; i < (int)(sizeof(seps) / sizeof(seps[0])); i++) {
		pos = strstr(tmp, seps[i]);
		if (!pos)
			continue;
		*pos = '\0';
		pos += strlen(seps[i]);
		lyric_trim_inplace(tmp);
		lyric_trim_inplace(pos);
		if (tmp[0] && pos[0]) {
			/* 仅酷我 "--" 固定左歌手右歌名；" — " 方向由 settle 锚点决定 */
			if (strcmp(seps[i], "--") == 0) {
				lyric_copy(song_name, song_sz, pos);
				lyric_copy(singer, singer_sz, tmp);
			} else {
				lyric_copy(song_name, song_sz, tmp);
				lyric_copy(singer, singer_sz, pos);
			}
		}
		return;
	}
}

static int split_migu_type2(const char *text, char *title_out, size_t title_sz,
	char *artist_out, size_t artist_sz)
{
	const char *hyphen;
	char left[DIVOOM_BT_LYRIC_STR_LEN];
	char right[DIVOOM_BT_LYRIC_STR_LEN];

	if (!text || !title_out || !artist_out || title_sz == 0 || artist_sz == 0)
		return 0;
	if (strstr(text, " - ") != NULL || strstr(text, "--") != NULL ||
		strchr(text, '/') != NULL)
		return 0;
	hyphen = strchr(text, '-');
	if (!hyphen || hyphen == text || !hyphen[1])
		return 0;
	strncpy(left, text, (size_t)(hyphen - text));
	left[(size_t)(hyphen - text)] = '\0';
	lyric_copy(right, sizeof(right), hyphen + 1);
	lyric_trim_inplace(left);
	lyric_trim_inplace(right);
	if (!left[0] || !right[0])
		return 0;
	lyric_copy(title_out, title_sz, left);
	lyric_copy(artist_out, artist_sz, right);
	return 1;
}

static int migu_should_apply(const char *left, const char *right,
	const char *song_name, const char *singer, const char *type1)
{
	size_t left_len;

	if (!left || !left[0] || !right || !right[0])
		return 0;
	/* 鸿蒙酷狗 type1 带空格 ` - `，不要按咪咕左歌名拆 */
	if (type1 && strstr(type1, " - ") != NULL)
		return 0;
	if (song_name && song_name[0] && strcmp(left, song_name) == 0)
		return 1;
	if (singer && singer[0] && strcmp(right, singer) == 0)
		return 1;
	if (type1 && type1[0]) {
		left_len = strlen(left);
		if (left_len > 0 && strncmp(type1, left, left_len) == 0)
			return 1;
		if (divoom_bt_lyric_line_is_credit(type1) &&
			(!song_name || strcmp(left, song_name) != 0)) {
			if (singer && singer[0] &&
				strncmp(singer, left, left_len) == 0)
				return 0;
			return 1;
		}
	}
	return 0;
}
#endif /* !DIVOOM_BT_LYRIC_COMBINED_AS_TITLE */

static int split_artist_slash_title(const char *text, const char *known_title,
	char *artist, size_t artist_sz, char *title_out, size_t title_sz)
{
	const char *slash;
	char left[DIVOOM_BT_LYRIC_STR_LEN];
	char right[DIVOOM_BT_LYRIC_STR_LEN];
	size_t left_len;

	if (!text || !known_title || !known_title[0] || !strchr(text, '/'))
		return 0;
	if (!artist || !title_out || artist_sz == 0 || title_sz == 0)
		return 0;
	slash = text;
	while ((slash = strchr(slash, '/')) != NULL) {
		if (slash == text) {
			slash++;
			continue;
		}
		left_len = (size_t)(slash - text);
		if (left_len >= sizeof(left))
			left_len = sizeof(left) - 1;
		memcpy(left, text, left_len);
		left[left_len] = '\0';
		lyric_copy(right, sizeof(right), slash + 1);
		lyric_trim_inplace(left);
		lyric_trim_inplace(right);
		if (left[0] && right[0] && strcmp(right, known_title) == 0) {
			lyric_copy(artist, artist_sz, left);
			lyric_copy(title_out, title_sz, right);
			return 1;
		}
		slash++;
	}
	return 0;
}

#if !DIVOOM_BT_LYRIC_COMBINED_AS_TITLE
static int split_spaced_dash_once(const char *text, char *left, size_t left_sz,
	char *right, size_t right_sz)
{
	const char *sep;
	size_t left_len;

	if (!text || !left || !right || left_sz == 0 || right_sz == 0)
		return 0;
	sep = strstr(text, " - ");
	if (!sep || sep == text || !sep[3])
		return 0;
	left_len = (size_t)(sep - text);
	if (left_len >= left_sz)
		left_len = left_sz - 1;
	memcpy(left, text, left_len);
	left[left_len] = '\0';
	lyric_copy(right, right_sz, sep + 3);
	lyric_trim_inplace(left);
	lyric_trim_inplace(right);
	return (left[0] && right[0]) ? 1 : 0;
}

static int kugou_metadata_dash_pair(const char *type1, const char *type2,
	char *title_out, size_t title_sz, char *artist_out, size_t artist_sz)
{
	char t_left[DIVOOM_BT_LYRIC_STR_LEN];
	char t_right[DIVOOM_BT_LYRIC_STR_LEN];
	char a_left[DIVOOM_BT_LYRIC_STR_LEN];
	char a_right[DIVOOM_BT_LYRIC_STR_LEN];

	if (!type1 || !type2 || !title_out || !artist_out)
		return 0;
	if (!split_spaced_dash_once(type1, t_left, sizeof(t_left),
			t_right, sizeof(t_right)))
		return 0;
	if (!split_spaced_dash_once(type2, a_left, sizeof(a_left),
			a_right, sizeof(a_right)))
		return 0;
	if (strcmp(t_left, a_right) != 0 || strcmp(t_right, a_left) != 0)
		return 0;
	lyric_copy(title_out, title_sz, t_left);
	lyric_copy(artist_out, artist_sz, t_right);
	return 1;
}

static int split_artist_dash_title(const char *text, const char *known_title,
	char *artist, size_t artist_sz, char *title_out, size_t title_sz)
{
	char left[DIVOOM_BT_LYRIC_STR_LEN];
	char right[DIVOOM_BT_LYRIC_STR_LEN];

	if (!text || !known_title || !known_title[0] || !strstr(text, " - "))
		return 0;
	if (!artist || !title_out || artist_sz == 0 || title_sz == 0)
		return 0;
	if (!split_spaced_dash_once(text, left, sizeof(left), right, sizeof(right)))
		return 0;
	if (strcmp(right, known_title) != 0)
		return 0;
	lyric_copy(artist, artist_sz, left);
	lyric_copy(title_out, title_sz, right);
	return 1;
}
#endif /* !DIVOOM_BT_LYRIC_COMBINED_AS_TITLE */

static int split_last_slash(const char *text, char *artist, size_t artist_sz,
	char *title_out, size_t title_sz)
{
	const char *slash;
	const char *last = NULL;
	size_t left_len;
	char left[DIVOOM_BT_LYRIC_STR_LEN];
	char right[DIVOOM_BT_LYRIC_STR_LEN];

	if (!text || !artist || !title_out || artist_sz == 0 || title_sz == 0)
		return 0;
	slash = text;
	while ((slash = strchr(slash, '/')) != NULL) {
		last = slash;
		slash++;
	}
	if (!last || last == text || !last[1])
		return 0;
	left_len = (size_t)(last - text);
	if (left_len >= sizeof(left))
		left_len = sizeof(left) - 1;
	memcpy(left, text, left_len);
	left[left_len] = '\0';
	lyric_copy(right, sizeof(right), last + 1);
	lyric_trim_inplace(left);
	lyric_trim_inplace(right);
	if (!left[0] || !right[0])
		return 0;
	lyric_copy(artist, artist_sz, left);
	lyric_copy(title_out, title_sz, right);
	return 1;
}

static int text_has_bare_slash(const char *text)
{
	const char *p;

	if (!text || !strchr(text, '/'))
		return 0;
	if (strstr(text, " / ") != NULL)
		return 0;
	for (p = text; *p; p++) {
		if (*p != '/')
			continue;
		if (p > text && p[-1] == ' ')
			return 0;
		if (p[1] == ' ')
			return 0;
	}
	return 1;
}

static int text_has_bare_dash(const char *text)
{
	const char *hyphen;

	if (!text || strchr(text, '/') != NULL)
		return 0;
	if (strstr(text, " - ") != NULL || strstr(text, "--") != NULL)
		return 0;
	hyphen = strchr(text, '-');
	return (hyphen && hyphen != text && hyphen[1]) ? 1 : 0;
}

/* 裸 `-` 左右各一段（排除 `--`、空格 ` - `、含 /） */
static int split_bare_dash_once(const char *text, char *left, size_t left_sz,
	char *right, size_t right_sz)
{
	const char *hyphen;
	size_t left_len;

	if (!text || !left || !right || left_sz == 0 || right_sz == 0)
		return 0;
	if (!text_has_bare_dash(text))
		return 0;
	hyphen = strchr(text, '-');
	if (!hyphen || hyphen == text || !hyphen[1])
		return 0;
	left_len = (size_t)(hyphen - text);
	if (left_len >= left_sz)
		left_len = left_sz - 1;
	memcpy(left, text, left_len);
	left[left_len] = '\0';
	lyric_copy(right, right_sz, hyphen + 1);
	lyric_trim_inplace(left);
	lyric_trim_inplace(right);
	return (left[0] && right[0]) ? 1 : 0;
}

#if !DIVOOM_BT_LYRIC_COMBINED_AS_TITLE
static int text_starts_with_token(const char *text, const char *token)
{
	size_t n;

	if (!text || !token || !token[0])
		return 0;
	n = strlen(token);
	if (strncmp(text, token, n) != 0)
		return 0;
	return (text[n] == '\0' || text[n] == ' ' || text[n] == '(') ? 1 : 0;
}

static int sides_title_match(const char *long_side, const char *short_side)
{
	if (!long_side || !short_side || !short_side[0])
		return 0;
	if (strcmp(long_side, short_side) == 0)
		return 1;
	return text_starts_with_token(long_side, short_side);
}

/* 鸿蒙酷狗：type1 空格 ` - ` + type2 裸 `-`，type2 固定左歌手右歌名 */
static int harmony_kugou_from_pair(const char *type1, const char *type2,
	char *title_out, size_t title_sz, char *artist_out, size_t artist_sz)
{
	char t_left[DIVOOM_BT_LYRIC_STR_LEN];
	char t_right[DIVOOM_BT_LYRIC_STR_LEN];
	char a_left[DIVOOM_BT_LYRIC_STR_LEN];
	char a_right[DIVOOM_BT_LYRIC_STR_LEN];

	if (!type1 || !type2 || !title_out || !artist_out)
		return 0;
	if (!split_bare_dash_once(type2, a_left, sizeof(a_left),
			a_right, sizeof(a_right)))
		return 0;
	if (!split_spaced_dash_once(type1, t_left, sizeof(t_left),
			t_right, sizeof(t_right)))
		return 0;
	/* 对调：歌名 - 歌手 + 歌手-歌名 */
	if (strcmp(t_right, a_left) == 0 && sides_title_match(t_left, a_right)) {
		lyric_copy(title_out, title_sz, a_right);
		lyric_copy(artist_out, artist_sz, a_left);
		return 1;
	}
	/* 同序：歌手 - 歌名 + 歌手-歌名 */
	if (strcmp(t_left, a_left) == 0 && sides_title_match(t_right, a_right)) {
		lyric_copy(title_out, title_sz, a_right);
		lyric_copy(artist_out, artist_sz, a_left);
		return 1;
	}
	return 0;
}

static int harmony_kugou_lyric_phase(const divoom_bt_lyric_ctx *ctx, const char *type2)
{
	char left[DIVOOM_BT_LYRIC_STR_LEN];
	char right[DIVOOM_BT_LYRIC_STR_LEN];

	if (!ctx || !type2 || !ctx->title[0] || !ctx->artist[0])
		return 0;
	if (!split_bare_dash_once(type2, left, sizeof(left), right, sizeof(right)))
		return 0;
	if (strcmp(left, ctx->artist) != 0)
		return 0;
	return (strcmp(right, ctx->title) == 0 ||
		sides_title_match(ctx->title, right)) ? 1 : 0;
}
#endif /* !DIVOOM_BT_LYRIC_COMBINED_AS_TITLE */

static int sep_is_dot_family(const char *text)
{
	return (strstr(text, " · ") != NULL || strstr(text, "\xC2\xB7") != NULL ||
		strstr(text, " • ") != NULL || strstr(text, "\xE2\x80\xA2") != NULL);
}

static int sep_has_emdash(const char *text)
{
	return (text && strstr(text, " — ") != NULL) ? 1 : 0;
}

#if DIVOOM_BT_LYRIC_COMBINED_AS_TITLE
/* type2 是否已是「歌名+歌手」组合串（与只歌手、裸 / 区分） */
static int sep_is_combined_title(const char *text)
{
	if (!text || !text[0])
		return 0;
	if (sep_is_dot_family(text) || sep_has_emdash(text))
		return 1;
	if (strstr(text, "--") != NULL || strstr(text, " - ") != NULL)
		return 1;
	return 0;
}

/* 含汉字/假名/韩文（排除全角标点 EF），用来区分 顺子-歌名 与 350Ton-Xll */
static int text_has_east_asian(const char *s)
{
	const unsigned char *p = (const unsigned char *)s;

	if (!p)
		return 0;
	while (*p) {
		if (*p >= 0xE0 && *p <= 0xED)
			return 1;
		p++;
	}
	return 0;
}

/* 鸿蒙酷狗裸 `-`：已是组合才整段当标题，避免 350Ton-Xll！当歌名 */
static int harmony_bare_dash_as_title(const divoom_bt_lyric_ctx *ctx,
	const char *t1, const char *t2)
{
	char left[DIVOOM_BT_LYRIC_STR_LEN];
	char right[DIVOOM_BT_LYRIC_STR_LEN];

	if (!ctx || !t2 || !split_bare_dash_once(t2, left, sizeof(left),
			right, sizeof(right)))
		return 0;
	if (ctx->artist[0] && strcmp(t2, ctx->artist) == 0)
		return 0;
	/* 中途连上：type1 已是歌词，type2 稳定为 歌手-歌名 */
	if (text_has_east_asian(left) || text_has_east_asian(right))
		return 1;
	if (t1 && strstr(t1, " - ") != NULL)
		return 1;
	if (t1 && t1[0] &&
		(divoom_bt_lyric_line_is_credit(t1) ||
			divoom_bt_lyric_line_is_header(t1, ctx->artist, ctx->title))) {
		if (ctx->artist[0] && strcmp(t2, ctx->artist) == 0)
			return 0;
		return 1;
	}
	if (ctx->title[0] && strcmp(t2, ctx->title) == 0)
		return 1;
	if (ctx->title[0] && (strcmp(right, ctx->title) == 0 ||
			strcmp(left, ctx->title) == 0))
		return 1;
	if (ctx->artist[0] && (strcmp(left, ctx->artist) == 0 ||
			strcmp(right, ctx->artist) == 0))
		return 1;
	return 0;
}

static int split_combined_sides(const char *text, char *left, char *right)
{
	static const char *seps[] = {
		" · ", "\xC2\xB7", " • ", "\xE2\x80\xA2",
		" — ", "--", " - "
	};
	const char *pos;
	size_t left_len;
	int i;

	if (!text || !left || !right)
		return 0;
	left[0] = '\0';
	right[0] = '\0';
	for (i = 0; i < (int)(sizeof(seps) / sizeof(seps[0])); i++) {
		pos = strstr(text, seps[i]);
		if (!pos || pos == text || !pos[strlen(seps[i])])
			continue;
		left_len = (size_t)(pos - text);
		if (left_len >= DIVOOM_BT_LYRIC_STR_LEN)
			left_len = DIVOOM_BT_LYRIC_STR_LEN - 1;
		memcpy(left, text, left_len);
		left[left_len] = '\0';
		lyric_copy(right, DIVOOM_BT_LYRIC_STR_LEN, pos + strlen(seps[i]));
		lyric_trim_inplace(left);
		lyric_trim_inplace(right);
		return (left[0] && right[0]) ? 1 : 0;
	}
	return 0;
}

static int combined_same_pair(const char *a, const char *b)
{
	char a_left[DIVOOM_BT_LYRIC_STR_LEN];
	char a_right[DIVOOM_BT_LYRIC_STR_LEN];
	char b_left[DIVOOM_BT_LYRIC_STR_LEN];
	char b_right[DIVOOM_BT_LYRIC_STR_LEN];

	a_left[0] = a_right[0] = b_left[0] = b_right[0] = '\0';
	if (!split_combined_sides(a, a_left, a_right) ||
		!split_combined_sides(b, b_left, b_right))
		return 0;
	if (strcmp(a_left, b_left) == 0 && strcmp(a_right, b_right) == 0)
		return 1;
	if (strcmp(a_left, b_right) == 0 && strcmp(a_right, b_left) == 0)
		return 1;
	return 0;
}

static int combined_same_song(const divoom_bt_lyric_ctx *ctx, const char *t2)
{
	if (!ctx || !t2 || !t2[0])
		return 0;
	if (ctx->title[0] && strstr(t2, ctx->title) != NULL)
		return 1;
	if (ctx->artist[0] && strstr(t2, ctx->artist) != NULL)
		return 1;
	if (ctx->title[0] && combined_same_pair(ctx->title, t2))
		return 1;
	return 0;
}

/* 同曲只换标题形态（干净歌名→组合、汽水左右对调）不当切歌 */
static int apply_combined_as_title(divoom_bt_lyric_ctx *ctx, const char *t2)
{
	int had_title;
	int same;
	int song_changed = 0;

	if (!ctx || !t2 || !t2[0])
		return 0;
	had_title = ctx->title[0] ? 1 : 0;
	same = (!had_title || combined_same_song(ctx, t2)) ? 1 : 0;
	if (strcmp(ctx->title, t2) != 0) {
		if (had_title && !same)
			song_changed = 1;
		lyric_copy(ctx->title, sizeof(ctx->title), t2);
	}
	if (song_changed)
		ctx->artist[0] = '\0';
	ctx->combined = 1;
	ctx->lyric_phase = 1;
	ctx->amb_kind = 0;
	ctx->amb_first_type1[0] = '\0';
	if (song_changed) {
		ctx->lyric[0] = '\0';
		ctx->uart_lyric_valid = 0;
		ctx->hold_active = 0;
		ctx->hold_lyric[0] = '\0';
	}
	return song_changed;
}
#endif /* DIVOOM_BT_LYRIC_COMBINED_AS_TITLE */

#if !DIVOOM_BT_LYRIC_COMBINED_AS_TITLE
/* 汽水 · / 波点与安卓汽水 — ：右侧等于同突发 type1 或当前歌名则对调 */
static void maybe_swap_combined_by_anchor(divoom_bt_lyric_ctx *ctx,
	const char *t1, char *t2_title, char *t2_artist)
{
	const char *anchor = NULL;

	if (!ctx || !t2_title || !t2_artist || !t2_title[0] || !t2_artist[0])
		return;
	if (t1[0] && !divoom_bt_lyric_line_is_credit(t1))
		anchor = t1;
	if ((!anchor || strcmp(t2_artist, anchor) != 0) &&
		ctx->title[0] && !divoom_bt_lyric_line_is_credit(ctx->title))
		anchor = ctx->title;
	if (anchor && anchor[0] &&
		strcmp(t2_artist, anchor) == 0 &&
		strcmp(t2_title, anchor) != 0) {
		char tmp[DIVOOM_BT_LYRIC_STR_LEN];

		lyric_copy(tmp, sizeof(tmp), t2_title);
		lyric_copy(t2_title, DIVOOM_BT_LYRIC_STR_LEN, t2_artist);
		lyric_copy(t2_artist, DIVOOM_BT_LYRIC_STR_LEN, tmp);
	}
}
#endif /* !DIVOOM_BT_LYRIC_COMBINED_AS_TITLE */

static void fill_out(const divoom_bt_lyric_ctx *ctx, divoom_bt_lyric_out *out,
	const char *old_title, const char *old_artist, const char *old_lyric)
{
	if (!out || !ctx)
		return;
	out->title = ctx->title;
	out->artist = ctx->artist;
	out->lyric = ctx->lyric;
	out->title_changed = (strcmp(ctx->title, old_title) != 0) ? 1 : 0;
	out->artist_changed = (strcmp(ctx->artist, old_artist) != 0) ? 1 : 0;
	out->lyric_changed = (strcmp(ctx->lyric, old_lyric) != 0) ? 1 : 0;
	out->song_changed = 0;
	if (out->title_changed) {
		if (!(old_title[0] && old_artist[0] &&
			strcmp(old_title, ctx->artist) == 0 &&
			strcmp(old_artist, ctx->title) == 0))
			out->song_changed = 1;
	}
	out->uart_lyric_valid = ctx->uart_lyric_valid;
	if (!out->trace)
		out->trace = "-";
}

static void apply_lyric_now(divoom_bt_lyric_ctx *ctx, const char *text)
{
	if (!ctx || !text)
		return;
	lyric_copy(ctx->lyric, sizeof(ctx->lyric), text);
	ctx->uart_lyric_valid = 1;
	ctx->hold_active = 0;
	ctx->hold_lyric[0] = '\0';
}

static void apply_lyric_or_hold(divoom_bt_lyric_ctx *ctx, const char *text,
	int song_changed, uint32_t now_ms)
{
	if (!ctx || !text || !text[0])
		return;
	if (song_changed) {
		ctx->lyric[0] = '\0';
		ctx->uart_lyric_valid = 0;
		lyric_copy(ctx->hold_lyric, sizeof(ctx->hold_lyric), text);
		ctx->hold_active = 1;
		ctx->hold_ms = now_ms;
	} else {
		apply_lyric_now(ctx, text);
	}
}

static void flush_hold(divoom_bt_lyric_ctx *ctx, uint32_t now_ms)
{
	if (!ctx || !ctx->hold_active)
		return;
	if (now_ms >= ctx->hold_ms &&
		(now_ms - ctx->hold_ms) >= DIVOOM_BT_LYRIC_HOLD_MS &&
		ctx->hold_lyric[0]) {
		apply_lyric_now(ctx, ctx->hold_lyric);
	}
}

#if !DIVOOM_BT_LYRIC_COMBINED_AS_TITLE
static int pair_same(const char *old_t, const char *old_a,
	const char *new_t, const char *new_a)
{
	if (!old_t || !old_t[0])
		return 0;
	if (strcmp(old_t, new_t) == 0 && strcmp(old_a, new_a) == 0)
		return 1;
	if (strcmp(old_t, new_a) == 0 && strcmp(old_a, new_t) == 0)
		return 1;
	return 0;
}
#endif /* !DIVOOM_BT_LYRIC_COMBINED_AS_TITLE */

static int set_meta(divoom_bt_lyric_ctx *ctx, const char *title, const char *artist,
	int combined, int lyric_phase, uint32_t now_ms)
{
	int song_changed = 0;

	if (!ctx)
		return 0;
	if (title && title[0] && strcmp(ctx->title, title) != 0) {
		if (!(ctx->title[0] && ctx->artist[0] &&
			strcmp(ctx->title, artist) == 0 &&
			strcmp(ctx->artist, title) == 0))
			song_changed = 1;
		lyric_copy(ctx->title, sizeof(ctx->title), title);
	}
	if (artist)
		lyric_copy(ctx->artist, sizeof(ctx->artist), artist);
	ctx->combined = combined ? 1 : 0;
	ctx->lyric_phase = lyric_phase ? 1 : 0;
	ctx->amb_kind = 0;
	ctx->amb_first_type1[0] = '\0';
	if (song_changed) {
		ctx->lyric[0] = '\0';
		ctx->uart_lyric_valid = 0;
		ctx->hold_active = 0;
		ctx->hold_lyric[0] = '\0';
		(void)now_ms;
	}
	return song_changed;
}

static void watch_ambiguous(divoom_bt_lyric_ctx *ctx, const char *type1,
	const char *type2, int kind)
{
	if (!ctx || !type2)
		return;
	if (strcmp(ctx->last_type2, type2) != 0) {
		lyric_copy(ctx->last_type2, sizeof(ctx->last_type2), type2);
		ctx->amb_first_type1[0] = '\0';
	}
	ctx->amb_kind = (uint8_t)kind;
	if (!ctx->amb_first_type1[0] && type1 && type1[0])
		lyric_copy(ctx->amb_first_type1, sizeof(ctx->amb_first_type1), type1);
}

static void settle_pair(divoom_bt_lyric_ctx *ctx, const char *type1,
	const char *type2, uint32_t now_ms, divoom_bt_lyric_out *out)
{
	char *slash_artist = s_lyric_work[LYRIC_WORK_SLASH_ARTIST];
	char *slash_title = s_lyric_work[LYRIC_WORK_SLASH_TITLE];
#if !DIVOOM_BT_LYRIC_COMBINED_AS_TITLE
	char *t2_title = s_lyric_work[LYRIC_WORK_T2_TITLE];
	char *t2_artist = s_lyric_work[LYRIC_WORK_T2_ARTIST];
	char *old_title = s_lyric_work[LYRIC_WORK_SETTLE_OLD_TITLE];
	int migu = 0;
	int combined = 0;
	int same;
#endif
	int song_changed;
	const char *t1 = type1 ? type1 : "";
	const char *t2 = type2 ? type2 : "";

	slash_artist[0] = '\0';
	slash_title[0] = '\0';
#if !DIVOOM_BT_LYRIC_COMBINED_AS_TITLE
	t2_title[0] = '\0';
	t2_artist[0] = '\0';
	lyric_copy(old_title, DIVOOM_BT_LYRIC_STR_LEN, ctx->title);
#endif

#if DIVOOM_BT_LYRIC_COMBINED_AS_TITLE
	if (sep_is_combined_title(t2) || harmony_bare_dash_as_title(ctx, t1, t2)) {
		song_changed = apply_combined_as_title(ctx, t2);
		out->trace = "combined as title";
		if (t1[0]) {
			if (song_changed)
				apply_lyric_or_hold(ctx, t1, 1, now_ms);
			else
				apply_lyric_now(ctx, t1);
			if (song_changed && ctx->hold_active)
				out->trace = "hold burst lyric";
		}
		return;
	}
#else
	if (t1[0] && harmony_kugou_from_pair(t1, t2, t2_title, DIVOOM_BT_LYRIC_STR_LEN,
			t2_artist, DIVOOM_BT_LYRIC_STR_LEN)) {
		song_changed = set_meta(ctx, t2_title, t2_artist, 0, 1, now_ms);
		out->trace = "harmony kugou pair";
		if (t1[0] && strcmp(t1, ctx->title) != 0)
			apply_lyric_or_hold(ctx, t1, song_changed, now_ms);
		return;
	}

	if (harmony_kugou_lyric_phase(ctx, t2)) {
		ctx->lyric_phase = 1;
		out->trace = "harmony kugou lyric-phase";
		if (t1[0] && strcmp(t1, ctx->title) != 0)
			apply_lyric_now(ctx, t1);
		return;
	}

	if (t1[0] && kugou_metadata_dash_pair(t1, t2, t2_title, DIVOOM_BT_LYRIC_STR_LEN,
			t2_artist, DIVOOM_BT_LYRIC_STR_LEN)) {
		set_meta(ctx, t2_title, t2_artist, 0, 0, now_ms);
		out->trace = "kugou dash pair metadata";
		return;
	}

	if (split_artist_dash_title(t2, ctx->title, slash_artist, DIVOOM_BT_LYRIC_STR_LEN,
			slash_title, DIVOOM_BT_LYRIC_STR_LEN) ||
		(t1[0] && !divoom_bt_lyric_line_is_credit(t1) &&
			split_artist_dash_title(t2, t1, slash_artist, DIVOOM_BT_LYRIC_STR_LEN,
				slash_title, DIVOOM_BT_LYRIC_STR_LEN))) {
		song_changed = set_meta(ctx, slash_title, slash_artist, 0, 1, now_ms);
		out->trace = (t1[0] && strcmp(t1, slash_title) != 0) ?
			"kugou lyric before dash title" : "kugou lyric-phase artist/title";
		if (t1[0] && strcmp(t1, ctx->title) != 0)
			apply_lyric_or_hold(ctx, t1, song_changed, now_ms);
		return;
	}

	split_title(t2, t2_title, DIVOOM_BT_LYRIC_STR_LEN, t2_artist, DIVOOM_BT_LYRIC_STR_LEN);
	if (!t2_artist[0]) {
		char m_title[DIVOOM_BT_LYRIC_STR_LEN];
		char m_artist[DIVOOM_BT_LYRIC_STR_LEN];

		m_title[0] = '\0';
		m_artist[0] = '\0';
		if (split_migu_type2(t2, m_title, sizeof(m_title),
				m_artist, sizeof(m_artist)) &&
			migu_should_apply(m_title, m_artist, ctx->title, ctx->artist, t1)) {
			lyric_copy(t2_title, DIVOOM_BT_LYRIC_STR_LEN, m_title);
			lyric_copy(t2_artist, DIVOOM_BT_LYRIC_STR_LEN, m_artist);
			migu = 1;
		}
	}

	if (t2_artist[0]) {
		if (!migu && (sep_is_dot_family(t2) || sep_has_emdash(t2))) {
			maybe_swap_combined_by_anchor(ctx, t1, t2_title, t2_artist);
			/* 波点常连上即歌词相：片头 + 歌手 — 歌名，尚无锚则按左歌手 */
			if (sep_has_emdash(t2) && !ctx->title[0] &&
				t1[0] && divoom_bt_lyric_line_is_credit(t1)) {
				char tmp[DIVOOM_BT_LYRIC_STR_LEN];

				lyric_copy(tmp, sizeof(tmp), t2_title);
				lyric_copy(t2_title, DIVOOM_BT_LYRIC_STR_LEN, t2_artist);
				lyric_copy(t2_artist, DIVOOM_BT_LYRIC_STR_LEN, tmp);
			}
		}
		same = pair_same(ctx->title, ctx->artist, t2_title, t2_artist);
		song_changed = set_meta(ctx, t2_title, t2_artist, 1, 1, now_ms);
		combined = 1;
		out->trace = migu ? "migu combined" : "combined";
		if (t1[0] && strcmp(t1, ctx->title) != 0) {
			if (same)
				apply_lyric_now(ctx, t1);
			else
				apply_lyric_or_hold(ctx, t1, song_changed, now_ms);
			if (song_changed && ctx->hold_active)
				out->trace = "hold burst lyric";
		} else if (t1[0] && same && strcmp(t1, ctx->title) == 0 && old_title[0]) {
			/* 第一句歌词就是歌名，且已是同一对 */
			if (!song_changed)
				apply_lyric_now(ctx, t1);
		}
		(void)combined;
		return;
	}
#endif /* !DIVOOM_BT_LYRIC_COMBINED_AS_TITLE */

	if (split_artist_slash_title(t2, ctx->title, slash_artist, DIVOOM_BT_LYRIC_STR_LEN,
			slash_title, DIVOOM_BT_LYRIC_STR_LEN) ||
		(t1[0] && !divoom_bt_lyric_line_is_credit(t1) &&
			split_artist_slash_title(t2, t1, slash_artist, DIVOOM_BT_LYRIC_STR_LEN,
				slash_title, DIVOOM_BT_LYRIC_STR_LEN))) {
		song_changed = set_meta(ctx, slash_title, slash_artist, 0, 1, now_ms);
		out->trace = (t1[0] && strcmp(t1, slash_title) != 0) ?
			"lyric before slash title" : "lyric-phase artist/title";
		if (t1[0] && strcmp(t1, ctx->title) != 0)
			apply_lyric_or_hold(ctx, t1, song_changed, now_ms);
		return;
	}

	if (ctx->amb_kind && ctx->last_type2[0] &&
		strcmp(ctx->last_type2, t2) == 0 &&
		t1[0] && ctx->amb_first_type1[0] &&
		strcmp(t1, ctx->amb_first_type1) != 0) {
		if (ctx->amb_kind == 1 &&
			split_last_slash(t2, slash_artist, DIVOOM_BT_LYRIC_STR_LEN,
				slash_title, DIVOOM_BT_LYRIC_STR_LEN)) {
			set_meta(ctx, slash_title, slash_artist, 0, 1, now_ms);
			apply_lyric_now(ctx, t1);
			out->trace = "bare-slash confirm lyric";
			return;
		}
#if !DIVOOM_BT_LYRIC_COMBINED_AS_TITLE
		if (ctx->amb_kind == 2 &&
			split_migu_type2(t2, t2_title, DIVOOM_BT_LYRIC_STR_LEN,
				t2_artist, DIVOOM_BT_LYRIC_STR_LEN)) {
			set_meta(ctx, t2_title, t2_artist, 1, 1, now_ms);
			/* 第二次突发确认：纠正首包误把词当歌名，立即上词，不当切歌 hold */
			apply_lyric_now(ctx, t1);
			out->trace = "migu confirm";
			return;
		}
#endif
	}

	if (t2[0] && strcmp(t2, ctx->title) != 0) {
		if (ctx->artist[0] && strcmp(ctx->artist, t2) == 0 &&
			t1[0] && strcmp(t1, ctx->title) != 0 &&
			!text_has_bare_dash(t2) &&
			!divoom_bt_lyric_line_is_credit(t1) &&
			!divoom_bt_lyric_line_is_header(t1, t2, ctx->title)) {
			set_meta(ctx, t1, t2, 0, 0, now_ms);
			out->trace = "same-artist new title";
			return;
		}

		if (t1[0] && !divoom_bt_lyric_line_is_credit(t1) &&
			!divoom_bt_lyric_line_is_header(t1, t2, ctx->title)) {
			set_meta(ctx, t1, t2, 0, 0, now_ms);
			out->trace = "artist_only";
		} else {
			lyric_copy(ctx->artist, sizeof(ctx->artist), t2);
			ctx->combined = 0;
			ctx->lyric_phase = 0;
			out->trace = "artist_only";
#if DIVOOM_BT_LYRIC_COMBINED_AS_TITLE
			if (t1[0] && ctx->title[0] && strcmp(t1, ctx->title) != 0)
				apply_lyric_now(ctx, t1);
#endif
		}

		if (text_has_bare_slash(t2))
			watch_ambiguous(ctx, t1, t2, 1);
		else if (text_has_bare_dash(t2))
			watch_ambiguous(ctx, t1, t2, 2);
		else {
			ctx->amb_kind = 0;
			ctx->amb_first_type1[0] = '\0';
			lyric_copy(ctx->last_type2, sizeof(ctx->last_type2), t2);
		}
		return;
	}

	out->trace = "type2 ignored";
}

static void settle_isolated_type1(divoom_bt_lyric_ctx *ctx, uint32_t now_ms,
	divoom_bt_lyric_out *out)
{
	const char *t1;

	if (!ctx || !ctx->burst_type1_valid)
		return;
	if (now_ms < ctx->burst_type1_ms ||
		(now_ms - ctx->burst_type1_ms) < DIVOOM_BT_LYRIC_BURST_MS)
		return;
	t1 = ctx->burst_type1;
	if (ctx->combined) {
		apply_lyric_or_hold(ctx, t1, 0, now_ms);
		out->trace = "isolated type1 lyric (combined)";
	} else if (ctx->lyric_phase) {
		apply_lyric_now(ctx, t1);
		out->trace = "isolated type1 lyric (phase)";
	} else if (divoom_bt_lyric_line_is_credit(t1)) {
		apply_lyric_now(ctx, t1);
		out->trace = "isolated type1 credit";
	} else if (divoom_bt_lyric_line_is_header(t1, ctx->artist, ctx->title)) {
		out->trace = "isolated type1 header";
	} else {
		int song_changed = set_meta(ctx, t1, ctx->artist, 0, 0, now_ms);

		(void)song_changed;
		out->trace = "isolated type1 title";
	}
	ctx->burst_type1_valid = 0;
	ctx->burst_type1[0] = '\0';
}

void divoom_bt_lyric_parse_reset(divoom_bt_lyric_ctx *ctx)
{
	if (!ctx)
		return;
	memset(ctx, 0, sizeof(*ctx));
}

void divoom_bt_lyric_parse_feed(divoom_bt_lyric_ctx *ctx, divoom_bt_lyric_ev ev,
	const char *text, uint32_t num, uint32_t now_ms, divoom_bt_lyric_out *out)
{
	char *old_title = s_lyric_work[LYRIC_WORK_OLD_TITLE];
	char *old_artist = s_lyric_work[LYRIC_WORK_OLD_ARTIST];
	char *old_lyric = s_lyric_work[LYRIC_WORK_OLD_LYRIC];
	divoom_bt_lyric_out local;
	divoom_bt_lyric_out *o = out ? out : &local;

	if (!ctx)
		return;
	memset(o, 0, sizeof(*o));
	o->trace = "-";
	lyric_copy(old_title, DIVOOM_BT_LYRIC_STR_LEN, ctx->title);
	lyric_copy(old_artist, DIVOOM_BT_LYRIC_STR_LEN, ctx->artist);
	lyric_copy(old_lyric, DIVOOM_BT_LYRIC_STR_LEN, ctx->lyric);

	flush_hold(ctx, now_ms);

	if (ev == DIVOOM_BT_LYRIC_EV_TYPE1) {
		if (!text || !text[0]) {
			fill_out(ctx, o, old_title, old_artist, old_lyric);
			return;
		}
		if (ctx->burst_type1_valid)
			settle_isolated_type1(ctx, now_ms, o);
		lyric_copy(ctx->burst_type1, sizeof(ctx->burst_type1), text);
		ctx->burst_type1_ms = now_ms;
		ctx->burst_type1_valid = 1;
		if (!o->trace || o->trace[0] == '-')
			o->trace = "type1 buffered";
	} else if (ev == DIVOOM_BT_LYRIC_EV_TYPE2) {
		if (ctx->burst_type1_valid &&
			now_ms >= ctx->burst_type1_ms &&
			(now_ms - ctx->burst_type1_ms) > DIVOOM_BT_LYRIC_BURST_MS)
			settle_isolated_type1(ctx, now_ms, o);
		settle_pair(ctx, ctx->burst_type1_valid ? ctx->burst_type1 : "",
			text ? text : "", now_ms, o);
		ctx->burst_type1_valid = 0;
		ctx->burst_type1[0] = '\0';
		lyric_copy(ctx->last_type2, sizeof(ctx->last_type2), text ? text : "");
	} else if (ev == DIVOOM_BT_LYRIC_EV_TOTAL) {
		ctx->song_len = num;
		settle_isolated_type1(ctx, now_ms, o);
		if (!o->trace || o->trace[0] == '-')
			o->trace = "total";
	} else if (ev == DIVOOM_BT_LYRIC_EV_POS) {
		ctx->play_pos = num;
		settle_isolated_type1(ctx, now_ms, o);
		if (!o->trace || o->trace[0] == '-')
			o->trace = "pos";
	} else {
		settle_isolated_type1(ctx, now_ms, o);
		if (!o->trace || o->trace[0] == '-')
			o->trace = "tick";
	}

	fill_out(ctx, o, old_title, old_artist, old_lyric);
}
