/* Included from divoom_database_manage.c — uses its lock, music_db_handle, sqlite wrappers. */

#define DIVOOM_TF_DB_USER_VERSION 1
#define DIVOOM_TF_DB_COLLATION_NATURAL "TFNAT"

enum {
	TFST_FOLDER_INFO = 0,
	TFST_FOLDER_PARENT,
	TFST_FOLDER_PAGE,
	TFST_TRACK_GORD,
	TFST_TRACK_BY_ID,
	TFST_NEXT_ID,
	TFST_PREV_ID,
	TFST_RANDOM_ID,
	TFST_MIN_ID,
	TFST_MAX_ID,
	TFST_FOLDER_BY_PARENT_NAME,
	TFST_FOLDER_BY_PATH,
	TFST_INSERT_FOLDER,
	TFST_INSERT_TRACK,
	TFST_UPDATE_FOLDER,
	TFST_SET_LIKE,
	TFST_META_GET,
	TFST_META_SET,
	TFST_TRACK_COUNT,
	TFST_LIKE_COUNT,
	TFST_TRACKS_IN_FOLDER,
	TFST_CHILD_FOLDER_NEXT,
	TFST_UPDATE_GORD,
	TFST_GORD_FIRST,
	TFST_GORD_LAST,
	TFST_MAX
};

static sqlite3_stmt *s_tf_stmt[TFST_MAX];
static int s_tf_rebuilding;
static int s_tf_next_folder_id;
static int s_tf_skip_cnt;

static const char *s_tf_sql[TFST_MAX] = {
	"SELECT id,parent_id,name,path,depth,ord,dir_cnt,trk_cnt,gord_lo,gord_hi FROM tf_folder WHERE id=?1",
	"SELECT parent_id FROM tf_folder WHERE id=?1",
	"SELECT id,name FROM tf_folder WHERE parent_id=?1 AND ord>=?2 ORDER BY ord LIMIT ?3",
	"SELECT id,name FROM tf_track WHERE gord>=?1 AND gord<?2 ORDER BY gord LIMIT ?3",
	"SELECT id,folder_id,name,gord,like_flag FROM tf_track WHERE id=?1",
	"SELECT id FROM tf_track WHERE gord>?1 ORDER BY gord LIMIT 1",
	"SELECT id FROM tf_track WHERE gord<?1 ORDER BY gord DESC LIMIT 1",
	"SELECT id FROM tf_track WHERE id>=?1 ORDER BY id LIMIT 1",
	"SELECT MIN(id) FROM tf_track",
	"SELECT MAX(id) FROM tf_track",
	"SELECT id FROM tf_folder WHERE parent_id=?1 AND name=?2",
	"SELECT id FROM tf_folder WHERE path=?1",
	"INSERT INTO tf_folder(id,parent_id,name,path,depth,ord,dir_cnt,trk_cnt,gord_lo,gord_hi) VALUES(?1,?2,?3,?4,?5,?6,0,0,0,0)",
	"INSERT INTO tf_track(id,folder_id,name,gord,like_flag) VALUES(?1,?2,?3,0,0)",
	"UPDATE tf_folder SET dir_cnt=?2,trk_cnt=?3,gord_lo=?4,gord_hi=?5 WHERE id=?1",
	"UPDATE tf_track SET like_flag=?2 WHERE id=?1",
	"SELECT v FROM tf_meta WHERE k=?1",
	"INSERT OR REPLACE INTO tf_meta(k,v) VALUES(?1,?2)",
	"SELECT COUNT(*) FROM tf_track",
	"SELECT COUNT(*) FROM tf_track WHERE like_flag=1",
	"SELECT id FROM tf_track WHERE folder_id=?1 ORDER BY name COLLATE TFNAT, id",
	"SELECT id,ord FROM tf_folder WHERE parent_id=?1 AND ord>=?2 ORDER BY ord LIMIT 1",
	"UPDATE tf_track SET gord=?2 WHERE id=?1",
	"SELECT id FROM tf_track ORDER BY gord LIMIT 1",
	"SELECT id FROM tf_track ORDER BY gord DESC LIMIT 1"
};

static void divoom_tf_db_copy_text(char *dst, int dst_size, sqlite3_stmt *st, int col)
{
	const unsigned char *raw;

	if (!dst || dst_size <= 0)
		return;
	dst[0] = '\0';
	raw = sqlite3_column_text(st, col);
	if (raw)
		strncpy(dst, (const char *)raw, (size_t)dst_size - 1u);
	dst[dst_size - 1] = '\0';
}

static void divoom_tf_db_finalize_all(void)
{
	int i;

	for (i = 0; i < TFST_MAX; i++) {
		if (s_tf_stmt[i]) {
			sqlite3_finalize(s_tf_stmt[i]);
			s_tf_stmt[i] = NULL;
		}
	}
}

static int divoom_tf_db_ascii_digit(unsigned char c)
{
	return (c >= '0' && c <= '9');
}

static unsigned char divoom_tf_db_ascii_fold(unsigned char c)
{
	if (c >= 'A' && c <= 'Z')
		return (unsigned char)(c + 32);
	return c;
}

/* 字母忽略大小写；连续数字按数值（1 < 2 < 10），避免 atoi 溢出 */
static int divoom_tf_db_nat_cmp_keys(const char *a, int alen, const char *b, int blen)
{
	int i = 0;
	int j = 0;

	if (!a) {
		a = "";
		alen = 0;
	}
	if (!b) {
		b = "";
		blen = 0;
	}
	if (alen < 0)
		alen = 0;
	if (blen < 0)
		blen = 0;

	while (i < alen && j < blen) {
		unsigned char ca = (unsigned char)a[i];
		unsigned char cb = (unsigned char)b[j];

		if (divoom_tf_db_ascii_digit(ca) && divoom_tf_db_ascii_digit(cb)) {
			int ia = i;
			int ib = j;
			int sa;
			int sb;
			int na;
			int nb;
			int k;
			int za;
			int zb;

			while (ia < alen && a[ia] == '0')
				ia++;
			while (ib < blen && b[ib] == '0')
				ib++;
			sa = ia;
			sb = ib;
			while (ia < alen && divoom_tf_db_ascii_digit((unsigned char)a[ia]))
				ia++;
			while (ib < blen && divoom_tf_db_ascii_digit((unsigned char)b[ib]))
				ib++;
			na = ia - sa;
			nb = ib - sb;
			if (na != nb)
				return (na < nb) ? -1 : 1;
			for (k = 0; k < na; k++) {
				if (a[sa + k] != b[sb + k])
					return ((unsigned char)a[sa + k] < (unsigned char)b[sb + k]) ? -1 : 1;
			}
			za = sa - i;
			zb = sb - j;
			if (za != zb)
				return (za > zb) ? -1 : 1;
			i = ia;
			j = ib;
			continue;
		}
		ca = divoom_tf_db_ascii_fold(ca);
		cb = divoom_tf_db_ascii_fold(cb);
		if (ca != cb)
			return (ca < cb) ? -1 : 1;
		i++;
		j++;
	}
	if (i >= alen && j >= blen)
		return 0;
	return (i >= alen) ? -1 : 1;
}

static int divoom_tf_db_collation_natural(void *ud, int n1, const void *k1, int n2, const void *k2)
{
	(void)ud;
	return divoom_tf_db_nat_cmp_keys((const char *)k1, n1, (const char *)k2, n2);
}

static int divoom_tf_db_register_collation(sqlite3 *db)
{
	int rc;

	if (!db)
		return 0;
	rc = sqlite3_create_collation(db, DIVOOM_TF_DB_COLLATION_NATURAL, SQLITE_UTF8,
		NULL, divoom_tf_db_collation_natural);
	if (rc != SQLITE_OK) {
		divoom_print("[TFDB] collation TFNAT fail: %s\n", sqlite3_errmsg(db));
		return 0;
	}
	return 1;
}

static int divoom_tf_db_prepare_all(sqlite3 *db)
{
	int i;
	int rc;

	if (!divoom_tf_db_register_collation(db))
		return 0;
	divoom_tf_db_finalize_all();
	for (i = 0; i < TFST_MAX; i++) {
		rc = sqlite3_prepare_v2(db, s_tf_sql[i], -1, &s_tf_stmt[i], NULL);
		if (rc != SQLITE_OK || s_tf_stmt[i] == NULL) {
			divoom_print("[TFDB] prepare %d failed: %s\n", i, sqlite3_errmsg(db));
			divoom_tf_db_finalize_all();
			return 0;
		}
	}
	return 1;
}

static int divoom_tf_db_exec(sqlite3 *db, const char *sql)
{
	int rc;

	rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
	if (rc != SQLITE_OK) {
		divoom_print("[TFDB] exec fail: %s; %s\n", sql, sqlite3_errmsg(db));
		return 0;
	}
	return 1;
}

static int divoom_tf_db_user_version(sqlite3 *db, int *version)
{
	sqlite3_stmt *st = NULL;
	int rc, finalize_rc;

	rc = sqlite3_prepare_v2(db, "PRAGMA user_version", -1, &st, NULL);
	if (rc == SQLITE_OK) {
		rc = sqlite3_step(st);
		if (rc == SQLITE_ROW) {
			*version = sqlite3_column_int(st, 0);
			rc = SQLITE_OK;
		}
	}
	finalize_rc = sqlite3_finalize(st);
	if (rc != SQLITE_OK || finalize_rc != SQLITE_OK) {
		divoom_print("[TFDB] read user_version failed: %s\n", sqlite3_errmsg(db));
		return 0;
	}
	return 1;
}

static int divoom_tf_db_create_tables(sqlite3 *db)
{
	if (!divoom_tf_db_exec(db,
		"CREATE TABLE tf_folder("
		"id INTEGER PRIMARY KEY, parent_id INTEGER NOT NULL,"
		"name TEXT NOT NULL, path TEXT NOT NULL, depth INTEGER NOT NULL,"
		"ord INTEGER NOT NULL, dir_cnt INTEGER NOT NULL DEFAULT 0,"
		"trk_cnt INTEGER NOT NULL DEFAULT 0,"
		"gord_lo INTEGER NOT NULL DEFAULT 0, gord_hi INTEGER NOT NULL DEFAULT 0)"))
		return 0;
	if (!divoom_tf_db_exec(db,
		"CREATE TABLE tf_track("
		"id INTEGER PRIMARY KEY, folder_id INTEGER NOT NULL,"
		"name TEXT NOT NULL, gord INTEGER NOT NULL,"
		"like_flag INTEGER NOT NULL DEFAULT 0)"))
		return 0;
	if (!divoom_tf_db_exec(db,
		"CREATE TABLE tf_meta(k TEXT PRIMARY KEY, v TEXT NOT NULL)"))
		return 0;
	if (!divoom_tf_db_exec(db,
		"INSERT OR REPLACE INTO tf_folder(id,parent_id,name,path,depth,ord,dir_cnt,trk_cnt,gord_lo,gord_hi)"
		" VALUES(0,-1,'','',0,0,0,0,0,0)"))
		return 0;
	if (!divoom_tf_db_exec(db, "PRAGMA user_version = 1"))
		return 0;
	return 1;
}

static int divoom_tf_db_indexes_drop(sqlite3 *db)
{
	divoom_tf_db_exec(db, "DROP INDEX IF EXISTS ix_folder_parent_ord");
	divoom_tf_db_exec(db, "DROP INDEX IF EXISTS ix_folder_parent_name");
	divoom_tf_db_exec(db, "DROP INDEX IF EXISTS ix_folder_path");
	divoom_tf_db_exec(db, "DROP INDEX IF EXISTS ix_track_gord");
	divoom_tf_db_exec(db, "DROP INDEX IF EXISTS ix_track_like");
	return 1;
}

static int divoom_tf_db_indexes_create(sqlite3 *db)
{
	if (!divoom_tf_db_exec(db, "CREATE UNIQUE INDEX ix_folder_parent_ord ON tf_folder(parent_id, ord)"))
		return 0;
	if (!divoom_tf_db_exec(db, "CREATE UNIQUE INDEX ix_folder_parent_name ON tf_folder(parent_id, name)"))
		return 0;
	if (!divoom_tf_db_exec(db, "CREATE UNIQUE INDEX ix_folder_path ON tf_folder(path)"))
		return 0;
	if (!divoom_tf_db_exec(db, "CREATE UNIQUE INDEX ix_track_gord ON tf_track(gord)"))
		return 0;
	if (!divoom_tf_db_exec(db, "CREATE INDEX ix_track_like ON tf_track(like_flag, id) WHERE like_flag = 1"))
		return 0;
	return 1;
}

static int divoom_tf_db_schema_ready(sqlite3 *db)
{
	int ver = 0;

	if (!divoom_tf_db_user_version(db, &ver))
		return 0;
	if (ver == DIVOOM_TF_DB_USER_VERSION) {
		sqlite3_stmt *st = NULL;
		int complete = 0, rc, finalize_rc;

		/* The shared DB check may have dropped an unreadable cache table. */
		rc = sqlite3_prepare_v2(db,
			"SELECT count(*) FROM sqlite_master WHERE type='table' "
			"AND name IN ('tf_folder','tf_track','tf_meta')", -1, &st, NULL);
		if (rc == SQLITE_OK) {
			rc = sqlite3_step(st);
			if (rc == SQLITE_ROW) {
				complete = sqlite3_column_int(st, 0) == 3;
				rc = SQLITE_OK;
			}
		}
		finalize_rc = sqlite3_finalize(st);
		if (rc != SQLITE_OK || finalize_rc != SQLITE_OK) {
			divoom_print("[TFDB] read cache schema failed: %s\n", sqlite3_errmsg(db));
			return 0;
		}
		if (complete)
			return 1;
	}

	/* Only rebuild the TF cache; an empty tf_meta invalidates scan fingerprints.
	 * Roll back every DDL/commit failure so other tables and old cache survive. */
	if (!divoom_tf_db_exec(db, "BEGIN IMMEDIATE"))
		return 0;
	if (!divoom_tf_db_exec(db, "DROP TABLE IF EXISTS tf_track") ||
	    !divoom_tf_db_exec(db, "DROP TABLE IF EXISTS tf_folder") ||
	    !divoom_tf_db_exec(db, "DROP TABLE IF EXISTS tf_meta") ||
	    !divoom_tf_db_create_tables(db) ||
	    !divoom_tf_db_indexes_create(db) ||
	    !divoom_tf_db_exec(db, "COMMIT")) {
		if (!sqlite3_get_autocommit(db))
			divoom_tf_db_exec(db, "ROLLBACK");
		return 0;
	}
	divoom_print("[TFDB] cache schema rebuilt; scan fingerprints cleared\n");
	return 1;
}

static sqlite3 *divoom_tf_db(void)
{
	return (pdivoom_database_manage_handle) ? pdivoom_database_manage_handle->music_db_handle : NULL;
}

static int divoom_tf_db_busy_empty(void)
{
	return s_tf_rebuilding;
}

static int divoom_tf_db_reset_bind(sqlite3_stmt *st)
{
	if (!st)
		return 0;
	sqlite3_reset(st);
	sqlite3_clear_bindings(st);
	return 1;
}

static int divoom_tf_db_step_one(sqlite3_stmt *st)
{
	int rc;

	rc = sqlite3_step(st);
	return (rc == SQLITE_ROW) ? 1 : 0;
}

static int divoom_tf_db_folder_info_locked(int folder_id, divoom_tf_folder_info *out)
{
	sqlite3_stmt *st = s_tf_stmt[TFST_FOLDER_INFO];

	if (!st || !out)
		return 0;
	memset(out, 0, sizeof(*out));
	divoom_tf_db_reset_bind(st);
	sqlite3_bind_int(st, 1, folder_id);
	if (!divoom_tf_db_step_one(st))
		return 0;
	out->id = sqlite3_column_int(st, 0);
	out->parent_id = sqlite3_column_int(st, 1);
	divoom_tf_db_copy_text(out->name, (int)sizeof(out->name), st, 2);
	divoom_tf_db_copy_text(out->path, (int)sizeof(out->path), st, 3);
	out->depth = sqlite3_column_int(st, 4);
	out->ord = sqlite3_column_int(st, 5);
	out->dir_cnt = sqlite3_column_int(st, 6);
	out->trk_cnt = sqlite3_column_int(st, 7);
	out->gord_lo = sqlite3_column_int(st, 8);
	out->gord_hi = sqlite3_column_int(st, 9);
	return 1;
}

int divoom_database_tf_is_rebuilding(void)
{
	return s_tf_rebuilding;
}

int divoom_database_tf_dir_total(int folder_id)
{
	divoom_tf_folder_info info;
	int n = 0;

	if (!divoom_music_db_is_ready() || divoom_tf_db_busy_empty())
		return 0;
	divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
	if (divoom_tf_db_folder_info_locked(folder_id, &info))
		n = info.dir_cnt + info.trk_cnt;
	divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
	return n;
}

int divoom_database_tf_dir_page(int folder_id, int offset, int limit, divoom_tf_entry *out, int out_cap)
{
	divoom_tf_folder_info info;
	int filled = 0;
	int take;
	int i;
	sqlite3_stmt *st;

	if (!out || limit <= 0 || out_cap <= 0)
		return 0;
	if (limit > out_cap)
		limit = out_cap;
	if (!divoom_music_db_is_ready() || divoom_tf_db_busy_empty() || offset < 0)
		return 0;

	divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
	if (!divoom_tf_db_folder_info_locked(folder_id, &info)) {
		divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
		return 0;
	}

	if (offset < info.dir_cnt) {
		take = info.dir_cnt - offset;
		if (take > limit)
			take = limit;
		st = s_tf_stmt[TFST_FOLDER_PAGE];
		divoom_tf_db_reset_bind(st);
		sqlite3_bind_int(st, 1, folder_id);
		sqlite3_bind_int(st, 2, offset);
		sqlite3_bind_int(st, 3, take);
		while (filled < take && divoom_tf_db_step_one(st)) {
			out[filled].is_dir = 1;
			out[filled].id = sqlite3_column_int(st, 0);
			divoom_tf_db_copy_text(out[filled].name, DIVOOM_TF_NAME_MAX, st, 1);
			filled++;
		}
	}

	if (filled < limit) {
		int trk_off = offset - info.dir_cnt;

		if (trk_off < 0)
			trk_off = 0;
		take = limit - filled;
		st = s_tf_stmt[TFST_TRACK_GORD];
		divoom_tf_db_reset_bind(st);
		sqlite3_bind_int(st, 1, info.gord_lo + trk_off);
		sqlite3_bind_int(st, 2, info.gord_lo + info.trk_cnt);
		sqlite3_bind_int(st, 3, take);
		while (filled < limit && divoom_tf_db_step_one(st)) {
			out[filled].is_dir = 0;
			out[filled].id = sqlite3_column_int(st, 0);
			divoom_tf_db_copy_text(out[filled].name, DIVOOM_TF_NAME_MAX, st, 1);
			filled++;
		}
	}
	for (i = filled; i < out_cap; i++) {
		out[i].is_dir = 0;
		out[i].id = 0;
		out[i].name[0] = '\0';
	}
	divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
	return filled;
}

int divoom_database_tf_subtree_count(int folder_id)
{
	divoom_tf_folder_info info;
	int n = 0;

	if (!divoom_music_db_is_ready() || divoom_tf_db_busy_empty())
		return 0;
	divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
	if (divoom_tf_db_folder_info_locked(folder_id, &info)) {
		n = info.gord_hi - info.gord_lo;
		if (n < 0)
			n = 0;
	}
	divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
	return n;
}

int divoom_database_tf_subtree_page(int folder_id, int offset, int limit, divoom_tf_entry *out, int out_cap)
{
	divoom_tf_folder_info info;
	int filled = 0;
	sqlite3_stmt *st;

	if (!out || limit <= 0 || out_cap <= 0)
		return 0;
	if (limit > out_cap)
		limit = out_cap;
	if (!divoom_music_db_is_ready() || divoom_tf_db_busy_empty() || offset < 0)
		return 0;

	divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
	if (!divoom_tf_db_folder_info_locked(folder_id, &info)) {
		divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
		return 0;
	}
	st = s_tf_stmt[TFST_TRACK_GORD];
	divoom_tf_db_reset_bind(st);
	sqlite3_bind_int(st, 1, info.gord_lo + offset);
	sqlite3_bind_int(st, 2, info.gord_hi);
	sqlite3_bind_int(st, 3, limit);
	while (filled < limit && divoom_tf_db_step_one(st)) {
		out[filled].is_dir = 0;
		out[filled].id = sqlite3_column_int(st, 0);
		divoom_tf_db_copy_text(out[filled].name, DIVOOM_TF_NAME_MAX, st, 1);
		filled++;
	}
	divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
	return filled;
}

int divoom_database_tf_folder_info(int folder_id, divoom_tf_folder_info *out)
{
	int ok;

	if (!out || !divoom_music_db_is_ready() || divoom_tf_db_busy_empty())
		return 0;
	divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
	ok = divoom_tf_db_folder_info_locked(folder_id, out);
	divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
	return ok;
}

int divoom_database_tf_folder_parent(int folder_id)
{
	sqlite3_stmt *st;
	int parent = -1;

	if (!divoom_music_db_is_ready() || divoom_tf_db_busy_empty())
		return -1;
	divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
	st = s_tf_stmt[TFST_FOLDER_PARENT];
	divoom_tf_db_reset_bind(st);
	sqlite3_bind_int(st, 1, folder_id);
	if (divoom_tf_db_step_one(st))
		parent = sqlite3_column_int(st, 0);
	divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
	return parent;
}

int divoom_database_tf_track_path(int track_id, char *buf, int buf_size)
{
	sqlite3_stmt *st;
	divoom_tf_folder_info folder;
	int folder_id;
	char name[DIVOOM_TF_NAME_MAX];
	int ok = 0;

	if (!buf || buf_size <= 0)
		return 0;
	buf[0] = '\0';
	if (!divoom_music_db_is_ready() || divoom_tf_db_busy_empty() || track_id <= 0)
		return 0;
	divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
	st = s_tf_stmt[TFST_TRACK_BY_ID];
	divoom_tf_db_reset_bind(st);
	sqlite3_bind_int(st, 1, track_id);
	if (divoom_tf_db_step_one(st)) {
		folder_id = sqlite3_column_int(st, 1);
		divoom_tf_db_copy_text(name, (int)sizeof(name), st, 2);
		if (divoom_tf_db_folder_info_locked(folder_id, &folder)) {
			if (folder.path[0])
				snprintf(buf, (size_t)buf_size, "%s%s", folder.path, name);
			else
				snprintf(buf, (size_t)buf_size, "%s", name);
			ok = 1;
		}
	}
	divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
	return ok;
}

int divoom_database_tf_track_name(int track_id, char *buf, int buf_size)
{
	sqlite3_stmt *st;
	int ok = 0;

	if (!buf || buf_size <= 0)
		return 0;
	buf[0] = '\0';
	if (!divoom_music_db_is_ready() || divoom_tf_db_busy_empty() || track_id <= 0)
		return 0;
	divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
	st = s_tf_stmt[TFST_TRACK_BY_ID];
	divoom_tf_db_reset_bind(st);
	sqlite3_bind_int(st, 1, track_id);
	if (divoom_tf_db_step_one(st)) {
		divoom_tf_db_copy_text(buf, buf_size, st, 2);
		ok = 1;
	}
	divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
	return ok;
}

int divoom_database_tf_track_row(int folder_id, int track_id)
{
	sqlite3_stmt *st;
	divoom_tf_folder_info info;
	int gord = -1;
	int row = -1;

	if (!divoom_music_db_is_ready() || divoom_tf_db_busy_empty() || track_id <= 0)
		return -1;
	divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
	st = s_tf_stmt[TFST_TRACK_BY_ID];
	divoom_tf_db_reset_bind(st);
	sqlite3_bind_int(st, 1, track_id);
	if (divoom_tf_db_step_one(st))
		gord = sqlite3_column_int(st, 3);
	if (gord >= 0 && divoom_tf_db_folder_info_locked(folder_id, &info))
		row = gord - info.gord_lo;
	divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
	return row;
}

static int divoom_tf_db_track_gord_locked(int track_id, int *gord)
{
	sqlite3_stmt *st = s_tf_stmt[TFST_TRACK_BY_ID];

	if (!st || !gord || track_id <= 0)
		return 0;
	divoom_tf_db_reset_bind(st);
	sqlite3_bind_int(st, 1, track_id);
	if (!divoom_tf_db_step_one(st))
		return 0;
	*gord = sqlite3_column_int(st, 3);
	return 1;
}

int divoom_database_tf_next_id(int cur_id)
{
	sqlite3_stmt *st;
	int id = 0;
	int gord = -1;

	if (!divoom_music_db_is_ready() || divoom_tf_db_busy_empty())
		return 0;
	divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
	if (divoom_tf_db_track_gord_locked(cur_id, &gord)) {
		st = s_tf_stmt[TFST_NEXT_ID];
		divoom_tf_db_reset_bind(st);
		sqlite3_bind_int(st, 1, gord);
		if (divoom_tf_db_step_one(st))
			id = sqlite3_column_int(st, 0);
	}
	if (id <= 0) {
		st = s_tf_stmt[TFST_GORD_FIRST];
		divoom_tf_db_reset_bind(st);
		if (divoom_tf_db_step_one(st))
			id = sqlite3_column_int(st, 0);
	}
	divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
	return id;
}

int divoom_database_tf_prev_id(int cur_id)
{
	sqlite3_stmt *st;
	int id = 0;
	int gord = -1;

	if (!divoom_music_db_is_ready() || divoom_tf_db_busy_empty())
		return 0;
	divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
	if (divoom_tf_db_track_gord_locked(cur_id, &gord)) {
		st = s_tf_stmt[TFST_PREV_ID];
		divoom_tf_db_reset_bind(st);
		sqlite3_bind_int(st, 1, gord);
		if (divoom_tf_db_step_one(st))
			id = sqlite3_column_int(st, 0);
	}
	if (id <= 0) {
		st = s_tf_stmt[TFST_GORD_LAST];
		divoom_tf_db_reset_bind(st);
		if (divoom_tf_db_step_one(st))
			id = sqlite3_column_int(st, 0);
	}
	divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
	return id;
}

int divoom_database_tf_random_id(int seed_id)
{
	sqlite3_stmt *st;
	int id = 0;

	if (!divoom_music_db_is_ready() || divoom_tf_db_busy_empty())
		return 0;
	divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
	st = s_tf_stmt[TFST_RANDOM_ID];
	divoom_tf_db_reset_bind(st);
	sqlite3_bind_int(st, 1, seed_id);
	if (divoom_tf_db_step_one(st))
		id = sqlite3_column_int(st, 0);
	if (id <= 0) {
		st = s_tf_stmt[TFST_MIN_ID];
		divoom_tf_db_reset_bind(st);
		if (divoom_tf_db_step_one(st))
			id = sqlite3_column_int(st, 0);
	}
	divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
	return id;
}

int divoom_database_tf_min_id(void)
{
	sqlite3_stmt *st;
	int id = 0;

	if (!divoom_music_db_is_ready() || divoom_tf_db_busy_empty())
		return 0;
	divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
	st = s_tf_stmt[TFST_MIN_ID];
	divoom_tf_db_reset_bind(st);
	if (divoom_tf_db_step_one(st))
		id = sqlite3_column_int(st, 0);
	divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
	return id;
}

int divoom_database_tf_max_id(void)
{
	sqlite3_stmt *st;
	int id = 0;

	if (!divoom_music_db_is_ready() || divoom_tf_db_busy_empty())
		return 0;
	divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
	st = s_tf_stmt[TFST_MAX_ID];
	divoom_tf_db_reset_bind(st);
	if (divoom_tf_db_step_one(st))
		id = sqlite3_column_int(st, 0);
	divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
	return id;
}

int divoom_database_tf_find_id_by_path(const char *music_path)
{
	char dir[DIVOOM_TF_PATH_MAX];
	char name[DIVOOM_TF_NAME_MAX];
	const char *slash;
	divoom_tf_folder_info info;
	sqlite3_stmt *st;
	int folder_id = 0;
	int found = 0;

	if (!music_path || !music_path[0] || !divoom_music_db_is_ready() || divoom_tf_db_busy_empty())
		return 0;
	slash = strrchr(music_path, '/');
	if (slash) {
		int n = (int)(slash - music_path) + 1;
		if (n >= (int)sizeof(dir))
			n = (int)sizeof(dir) - 1;
		memcpy(dir, music_path, (size_t)n);
		dir[n] = '\0';
		strncpy(name, slash + 1, sizeof(name) - 1u);
		name[sizeof(name) - 1] = '\0';
	} else {
		dir[0] = '\0';
		strncpy(name, music_path, sizeof(name) - 1u);
		name[sizeof(name) - 1] = '\0';
	}

	divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
	st = s_tf_stmt[TFST_FOLDER_BY_PATH];
	divoom_tf_db_reset_bind(st);
	sqlite3_bind_text(st, 1, dir, -1, SQLITE_TRANSIENT);
	if (divoom_tf_db_step_one(st))
		folder_id = sqlite3_column_int(st, 0);
	if (divoom_tf_db_folder_info_locked(folder_id, &info)) {
		st = s_tf_stmt[TFST_TRACK_GORD];
		divoom_tf_db_reset_bind(st);
		sqlite3_bind_int(st, 1, info.gord_lo);
		sqlite3_bind_int(st, 2, info.gord_lo + info.trk_cnt);
		sqlite3_bind_int(st, 3, info.trk_cnt > 0 ? info.trk_cnt : 1);
		while (divoom_tf_db_step_one(st)) {
			char cur[DIVOOM_TF_NAME_MAX];

			divoom_tf_db_copy_text(cur, (int)sizeof(cur), st, 1);
			if (strcmp(cur, name) == 0) {
				found = sqlite3_column_int(st, 0);
				break;
			}
		}
	}
	divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
	return found;
}

int divoom_database_tf_set_like(int track_id, int like_flag)
{
	sqlite3_stmt *st;
	int rc;

	if (!divoom_music_db_is_ready() || track_id <= 0)
		return 0;
	divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
	st = s_tf_stmt[TFST_SET_LIKE];
	divoom_tf_db_reset_bind(st);
	sqlite3_bind_int(st, 1, track_id);
	sqlite3_bind_int(st, 2, like_flag ? 1 : 0);
	rc = sqlite3_step(st);
	divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
	return rc == SQLITE_DONE;
}

int divoom_database_tf_total(void)
{
	sqlite3_stmt *st;
	int n = 0;

	if (!divoom_music_db_is_ready() || divoom_tf_db_busy_empty())
		return 0;
	divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
	st = s_tf_stmt[TFST_TRACK_COUNT];
	divoom_tf_db_reset_bind(st);
	if (divoom_tf_db_step_one(st))
		n = sqlite3_column_int(st, 0);
	divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
	return n;
}

int divoom_database_tf_like_count(void)
{
	sqlite3_stmt *st;
	int n = 0;

	if (!divoom_music_db_is_ready() || divoom_tf_db_busy_empty())
		return 0;
	divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
	st = s_tf_stmt[TFST_LIKE_COUNT];
	divoom_tf_db_reset_bind(st);
	if (divoom_tf_db_step_one(st))
		n = sqlite3_column_int(st, 0);
	divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
	return n;
}

int divoom_database_tf_meta_get_u32(const char *key, uint32 *out)
{
	sqlite3_stmt *st;
	int ok = 0;
	const unsigned char *raw;

	if (!key || !out || !divoom_music_db_is_ready())
		return 0;
	*out = 0;
	divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
	st = s_tf_stmt[TFST_META_GET];
	divoom_tf_db_reset_bind(st);
	sqlite3_bind_text(st, 1, key, -1, SQLITE_TRANSIENT);
	if (divoom_tf_db_step_one(st)) {
		raw = sqlite3_column_text(st, 0);
		if (raw) {
			*out = (uint32)strtoul((const char *)raw, NULL, 10);
			ok = 1;
		}
	}
	divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
	return ok;
}

int divoom_database_tf_meta_set_u32(const char *key, uint32 value)
{
	sqlite3_stmt *st;
	char buf[16];
	int rc;

	if (!key || !divoom_music_db_is_ready())
		return 0;
	snprintf(buf, sizeof(buf), "%u", (unsigned)value);
	divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
	st = s_tf_stmt[TFST_META_SET];
	divoom_tf_db_reset_bind(st);
	sqlite3_bind_text(st, 1, key, -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(st, 2, buf, -1, SQLITE_TRANSIENT);
	rc = sqlite3_step(st);
	divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
	return rc == SQLITE_DONE;
}

static void divoom_tf_db_rollback_savepoint(sqlite3 *db)
{
	if (!db)
		return;
	(void)sqlite3_exec(db, "ROLLBACK TO tf_ord", NULL, NULL, NULL);
	(void)sqlite3_exec(db, "RELEASE tf_ord", NULL, NULL, NULL);
}

/* 同级文件夹按自然序重排 ord；曲目 gord 在 assign 时按同一 collation */
static int divoom_tf_db_resort_folder_ord_on(sqlite3 *db)
{
	if (!db)
		return 0;
	if (!divoom_tf_db_exec(db, "SAVEPOINT tf_ord"))
		return 0;
	if (!divoom_tf_db_exec(db, "UPDATE tf_folder SET ord = -id WHERE id > 0")) {
		divoom_tf_db_rollback_savepoint(db);
		return 0;
	}
	if (!divoom_tf_db_exec(db,
		"UPDATE tf_folder SET ord = ("
		"SELECT COUNT(*) FROM tf_folder AS b "
		"WHERE b.parent_id = tf_folder.parent_id AND b.id > 0 AND "
		"(b.name COLLATE TFNAT < tf_folder.name COLLATE TFNAT OR "
		"(b.name COLLATE TFNAT = tf_folder.name COLLATE TFNAT AND b.id < tf_folder.id))"
		") WHERE id > 0")) {
		divoom_tf_db_rollback_savepoint(db);
		return 0;
	}
	return divoom_tf_db_exec(db, "RELEASE tf_ord");
}

static int divoom_tf_db_assign_gord_locked(int folder_id, int *gord)
{
	divoom_tf_folder_info info;
	sqlite3_stmt *st;
	sqlite3_stmt *ug;
	int lo;
	int child;
	int next_ord;

	if (!gord)
		return 0;
	lo = *gord;
	st = s_tf_stmt[TFST_TRACKS_IN_FOLDER];
	divoom_tf_db_reset_bind(st);
	sqlite3_bind_int(st, 1, folder_id);
	while (divoom_tf_db_step_one(st)) {
		int rc;

		ug = s_tf_stmt[TFST_UPDATE_GORD];
		divoom_tf_db_reset_bind(ug);
		sqlite3_bind_int(ug, 1, sqlite3_column_int(st, 0));
		sqlite3_bind_int(ug, 2, (*gord)++);
		rc = sqlite3_step(ug);
		if (rc != SQLITE_DONE) {
			divoom_print("[TFDB] update gord fail rc=%d %s\n", rc,
				divoom_tf_db() ? sqlite3_errmsg(divoom_tf_db()) : "");
			divoom_tf_db_reset_bind(st);
			return 0;
		}
	}

	next_ord = 0;
	{
		int *kids = NULL;
		int kn = 0;
		int kcap = 0;

		for (;;) {
			st = s_tf_stmt[TFST_CHILD_FOLDER_NEXT];
			divoom_tf_db_reset_bind(st);
			sqlite3_bind_int(st, 1, folder_id);
			sqlite3_bind_int(st, 2, next_ord);
			if (!divoom_tf_db_step_one(st))
				break;
			child = sqlite3_column_int(st, 0);
			next_ord = sqlite3_column_int(st, 1) + 1;
			if (kn >= kcap) {
				int ncap = kcap ? kcap * 2 : 8;
				int *nk = (int *)divoom_system_malloc((size_t)ncap * sizeof(int));

				if (!nk) {
					if (kids)
						divoom_system_free(kids);
					return 0;
				}
				if (kids) {
					memcpy(nk, kids, (size_t)kn * sizeof(int));
					divoom_system_free(kids);
				}
				kids = nk;
				kcap = ncap;
			}
			kids[kn++] = child;
		}
		for (next_ord = 0; next_ord < kn; next_ord++) {
			if (!divoom_tf_db_assign_gord_locked(kids[next_ord], gord)) {
				divoom_system_free(kids);
				return 0;
			}
		}
		if (kids)
			divoom_system_free(kids);
	}

	if (!divoom_tf_db_folder_info_locked(folder_id, &info))
		return 0;
	st = s_tf_stmt[TFST_UPDATE_FOLDER];
	divoom_tf_db_reset_bind(st);
	sqlite3_bind_int(st, 1, folder_id);
	sqlite3_bind_int(st, 2, info.dir_cnt);
	sqlite3_bind_int(st, 3, info.trk_cnt);
	sqlite3_bind_int(st, 4, lo);
	sqlite3_bind_int(st, 5, *gord);
	if (sqlite3_step(st) != SQLITE_DONE)
		return 0;
	return 1;
}

static void divoom_tf_db_rebuild_abort_locked(sqlite3 *db)
{
	if (db)
		(void)sqlite3_exec(db, "ROLLBACK", NULL, NULL, NULL);
	divoom_tf_db_finalize_all();
	if (db)
		(void)divoom_tf_db_indexes_create(db);
	if (db)
		(void)divoom_tf_db_prepare_all(db);
	s_tf_rebuilding = 0;
}

int divoom_database_tf_rebuild_begin(void)
{
	sqlite3 *db;

	if (!divoom_music_db_is_ready())
		return 0;
	divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
	db = divoom_tf_db();
	s_tf_rebuilding = 1;
	s_tf_skip_cnt = 0;
	divoom_tf_db_finalize_all();
	divoom_tf_db_indexes_drop(db);
	if (!divoom_tf_db_exec(db, "BEGIN")) {
		divoom_print("[TFDB] rebuild begin fail\n");
		(void)divoom_tf_db_indexes_create(db);
		(void)divoom_tf_db_prepare_all(db);
		s_tf_rebuilding = 0;
		divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
		return 0;
	}
	if (!divoom_tf_db_exec(db, "DELETE FROM tf_track")
	    || !divoom_tf_db_exec(db, "DELETE FROM tf_folder WHERE id!=0")
	    || !divoom_tf_db_exec(db,
		"UPDATE tf_folder SET dir_cnt=0,trk_cnt=0,gord_lo=0,gord_hi=0,ord=0 WHERE id=0")) {
		divoom_tf_db_rebuild_abort_locked(db);
		divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
		return 0;
	}
	s_tf_next_folder_id = 1;
	if (!divoom_tf_db_prepare_all(db)) {
		divoom_tf_db_rebuild_abort_locked(db);
		divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
		return 0;
	}
	divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
	return 1;
}

int divoom_database_tf_ensure_folder(int parent_id, const char *name, const char *path, int depth, int *out_id)
{
	sqlite3_stmt *st;
	int id = 0;
	int rc;

	if (!name || !path || !out_id || !divoom_music_db_is_ready())
		return 0;
	*out_id = 0;
	divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
	st = s_tf_stmt[TFST_FOLDER_BY_PARENT_NAME];
	divoom_tf_db_reset_bind(st);
	sqlite3_bind_int(st, 1, parent_id);
	sqlite3_bind_text(st, 2, name, -1, SQLITE_TRANSIENT);
	if (divoom_tf_db_step_one(st)) {
		id = sqlite3_column_int(st, 0);
		*out_id = id;
		divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
		return 1;
	}
	id = s_tf_next_folder_id++;
	st = s_tf_stmt[TFST_INSERT_FOLDER];
	divoom_tf_db_reset_bind(st);
	sqlite3_bind_int(st, 1, id);
	sqlite3_bind_int(st, 2, parent_id);
	sqlite3_bind_text(st, 3, name, -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(st, 4, path, -1, SQLITE_TRANSIENT);
	sqlite3_bind_int(st, 5, depth);
	{
		divoom_tf_folder_info pinfo;

		if (divoom_tf_db_folder_info_locked(parent_id, &pinfo))
			sqlite3_bind_int(st, 6, pinfo.dir_cnt);
		else
			sqlite3_bind_int(st, 6, 0);
	}
	rc = sqlite3_step(st);
	if (rc == SQLITE_DONE) {
		divoom_tf_folder_info pinfo;

		if (divoom_tf_db_folder_info_locked(parent_id, &pinfo)) {
			st = s_tf_stmt[TFST_UPDATE_FOLDER];
			divoom_tf_db_reset_bind(st);
			sqlite3_bind_int(st, 1, parent_id);
			sqlite3_bind_int(st, 2, pinfo.dir_cnt + 1);
			sqlite3_bind_int(st, 3, pinfo.trk_cnt);
			sqlite3_bind_int(st, 4, pinfo.gord_lo);
			sqlite3_bind_int(st, 5, pinfo.gord_hi);
			sqlite3_step(st);
		}
		*out_id = id;
		divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
		return 1;
	}
	s_tf_skip_cnt++;
	divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
	return 0;
}

int divoom_database_tf_add_track(int track_id, int folder_id, const char *name)
{
	sqlite3_stmt *st;
	int rc;

	if (!name || track_id <= 0 || !divoom_music_db_is_ready())
		return 0;
	divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
	st = s_tf_stmt[TFST_INSERT_TRACK];
	divoom_tf_db_reset_bind(st);
	sqlite3_bind_int(st, 1, track_id);
	sqlite3_bind_int(st, 2, folder_id);
	sqlite3_bind_text(st, 3, name, -1, SQLITE_TRANSIENT);
	rc = sqlite3_step(st);
	if (rc == SQLITE_DONE) {
		divoom_tf_folder_info pinfo;

		if (divoom_tf_db_folder_info_locked(folder_id, &pinfo)) {
			st = s_tf_stmt[TFST_UPDATE_FOLDER];
			divoom_tf_db_reset_bind(st);
			sqlite3_bind_int(st, 1, folder_id);
			sqlite3_bind_int(st, 2, pinfo.dir_cnt);
			sqlite3_bind_int(st, 3, pinfo.trk_cnt + 1);
			sqlite3_bind_int(st, 4, pinfo.gord_lo);
			sqlite3_bind_int(st, 5, pinfo.gord_hi);
			sqlite3_step(st);
		}
		divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
		return 1;
	}
	s_tf_skip_cnt++;
	divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
	return 0;
}

int divoom_database_tf_rebuild_end(void)
{
	sqlite3 *db;
	int gord = 0;
	uint32 gen = 0;

	if (!divoom_music_db_is_ready())
		return 0;
	divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
	db = divoom_tf_db();
	if (!divoom_tf_db_resort_folder_ord_on(db)
	    || !divoom_tf_db_assign_gord_locked(0, &gord)
	    || !divoom_tf_db_exec(db, "COMMIT")) {
		divoom_print("[TFDB] rebuild end abort gord=%d\n", gord);
		divoom_tf_db_rebuild_abort_locked(db);
		divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
		return 0;
	}
	divoom_tf_db_finalize_all();
	if (!divoom_tf_db_indexes_create(db) || !divoom_tf_db_prepare_all(db)) {
		s_tf_rebuilding = 0;
		divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
		return 0;
	}
	s_tf_rebuilding = 0;
	divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
	(void)divoom_database_tf_meta_get_u32("generation", &gen);
	divoom_database_tf_meta_set_u32("generation", gen + 1u);
	divoom_print("[TFDB] rebuild end tracks_gord=%d skipped=%d\n", gord, s_tf_skip_cnt);
	return 1;
}

static int divoom_tf_db_index_exists_on(sqlite3 *db, const char *name)
{
	sqlite3_stmt *st = NULL;
	int ok = 0;

	if (!db || !name)
		return 0;
	if (sqlite3_prepare_v2(db,
		"SELECT 1 FROM sqlite_master WHERE type='index' AND name=?1",
		-1, &st, NULL) != SQLITE_OK)
		return 0;
	sqlite3_bind_text(st, 1, name, -1, SQLITE_STATIC);
	ok = (sqlite3_step(st) == SQLITE_ROW) ? 1 : 0;
	sqlite3_finalize(st);
	return ok;
}

static int divoom_tf_db_gord_values_unique_on(sqlite3 *db)
{
	sqlite3_stmt *st = NULL;
	int total = 0;
	int distinct = 0;

	if (!db)
		return 0;
	if (sqlite3_prepare_v2(db,
		"SELECT COUNT(*), COUNT(DISTINCT gord) FROM tf_track",
		-1, &st, NULL) != SQLITE_OK)
		return 0;
	if (sqlite3_step(st) == SQLITE_ROW) {
		total = sqlite3_column_int(st, 0);
		distinct = sqlite3_column_int(st, 1);
	}
	sqlite3_finalize(st);
	return (total == distinct) ? 1 : 0;
}

static void divoom_tf_db_meta_put_u32_stmt(const char *key, uint32 value)
{
	sqlite3_stmt *st;
	char buf[16];

	st = s_tf_stmt[TFST_META_SET];
	if (!st || !key)
		return;
	snprintf(buf, sizeof(buf), "%u", (unsigned)value);
	divoom_tf_db_reset_bind(st);
	sqlite3_bind_text(st, 1, key, -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(st, 2, buf, -1, SQLITE_TRANSIENT);
	(void)sqlite3_step(st);
}

/* 开机只读：缺 ix_track_gord 才处理。gord 仍唯一则补索引，否则作废扫描指纹。 */
static void divoom_tf_db_heal_stale_on(sqlite3 *db)
{
	int unique_ok;

	if (!db)
		return;
	if (divoom_tf_db_index_exists_on(db, "ix_track_gord"))
		return;
	unique_ok = divoom_tf_db_gord_values_unique_on(db);
	if (unique_ok) {
		divoom_tf_db_indexes_drop(db);
		if (divoom_tf_db_indexes_create(db)) {
			divoom_print("[TFDB] restore indexes after missing ix_track_gord\n");
			return;
		}
	}
	divoom_tf_db_meta_put_u32_stmt("checksum", 0u);
	divoom_tf_db_meta_put_u32_stmt("songlist_ts", 0u);
	divoom_tf_db_meta_put_u32_stmt("nand_fp", 0u);
	divoom_print("[TFDB] stale gord cache invalidated unique=%d\n", unique_ok);
}

static int divoom_tf_db_open_on(sqlite3 *mdb)
{
	if (!divoom_tf_db_schema_ready(mdb))
		return 0;
	if (!divoom_tf_db_prepare_all(mdb))
		return 0;
	s_tf_rebuilding = 0;
	divoom_tf_db_heal_stale_on(mdb);
	return 1;
}

static void divoom_tf_db_close_locked(void)
{
	divoom_tf_db_finalize_all();
	s_tf_rebuilding = 0;
}
