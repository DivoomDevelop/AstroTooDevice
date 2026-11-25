#include "divoom_build_config.h"

#if defined(DIVOOM_DATABASE_MANAGE_INCLUDED_FROM_RTOS_STUB)
/* Compiled via divoom_database_manage_rtos_stub.c on RTOS. */
#elif DIVOOM_BUILD_RTOS_DEVICE
#error "On RTOS, compile database via divoom_database_manage_rtos_stub.c only"
#endif

#include "divoom.h"
#include "divoom_device_error.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#if 1
#include "sqlite3.h"
#if DIVOOM_BUILD_RTOS_DEVICE
#include "divoom_sqlite_rtos.h"
#include "divoom_product_test_userdata_wifi.h"
#if defined(CONFIG_DRIVERS_AIC8800D80)
#include "divoom_recovery_integrity.h"
#endif
#endif
#ifdef DIVOOM_SIMULATOR
#include "sim_sqlite.h"
#define sqlite3_open(_path, _ppdb) sim_sqlite3_open_mapped((_path), (_ppdb))
#endif

static int divoom_database_exec_pragma(sqlite3 *db, const char *sql)
{
	char *errmsg = NULL;
	int rc;

	rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
	if (rc != SQLITE_OK) {
		divoom_print("[DB] pragma failed: %s; rc=%d, err=%s\n",
			sql, rc, errmsg ? errmsg : sqlite3_errmsg(db));
	}
	if (errmsg)
		sqlite3_free(errmsg);
	return rc;
}

static int divoom_database_apply_pragmas(sqlite3 *db)
{
#if DIVOOM_BUILD_RTOS_DEVICE
	/*
	 * Keep a rollback journal on /userdata so an interrupted transaction can
	 * be recovered after an unexpected power loss. PERSIST avoids repeatedly
	 * creating/removing the journal on FAT, while FULL makes every committed
	 * transaction reach the VFS xSync -> fsync -> FatFs f_sync path.
	 */
	if (divoom_database_exec_pragma(db, "PRAGMA journal_mode = PERSIST;") != SQLITE_OK)
		return SQLITE_ERROR;
	if (divoom_database_exec_pragma(db, "PRAGMA synchronous = FULL;") != SQLITE_OK)
		return SQLITE_ERROR;
	if (divoom_database_exec_pragma(db, "PRAGMA cache_size = -64;") != SQLITE_OK)
		return SQLITE_ERROR;
#else
	if (divoom_database_exec_pragma(db, "PRAGMA synchronous = FULL;") != SQLITE_OK)
		return SQLITE_ERROR;
#endif
	return SQLITE_OK;
}

/*
 * On RTOS the DB lives on FAT /userdata. Open read-write, creating it on
 * first boot. FatFs' non-POSIX lseek/open quirks are handled transparently by
 * the "divoom-fat" VFS shim (see sqlite_rtos_init.c), so a plain open here is
 * sufficient and correct.
 */
static int divoom_database_open_rw(const char *db_name, sqlite3 **ppdb)
{
#if DIVOOM_BUILD_RTOS_DEVICE
	return sqlite3_open_v2(db_name, ppdb,
		SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
#else
	return sqlite3_open(db_name, ppdb);
#endif
}

/* Trigger hot-journal recovery, verify the schema, then touch one row in each
 * table. This is intentionally a startup accessibility check, not a full scan. */
static int divoom_database_check_accessible(sqlite3 *db)
{
	sqlite3_stmt *tables = NULL;
	sqlite3_stmt *probe = NULL;
	char *sql = NULL;
	char *bad_table = NULL;
	int rc;

	/* SQLite may silently fall back from read-write to a read-only open. */
	if (sqlite3_db_readonly(db, "main") == 1)
		return SQLITE_READONLY;
check_tables:
	rc = sqlite3_prepare_v2(db,
		"SELECT name FROM sqlite_master WHERE type='table' AND rootpage>0 "
		"AND name NOT GLOB 'sqlite_*'",
		-1, &tables, NULL);
	if (rc != SQLITE_OK)
		return rc;
	while ((rc = sqlite3_step(tables)) == SQLITE_ROW) {
		const char *name = (const char *)sqlite3_column_text(tables, 0);
		if (!name) {
			rc = SQLITE_NOMEM;
			break;
		}
		/* A covering index must not hide an unreadable table page. */
		sql = sqlite3_mprintf("SELECT 1 FROM \"%w\" NOT INDEXED LIMIT 1", name);
		if (!sql) {
			rc = SQLITE_NOMEM;
			break;
		}
		rc = sqlite3_prepare_v2(db, sql, -1, &probe, NULL);
		sqlite3_free(sql);
		if (rc == SQLITE_OK) {
			rc = sqlite3_step(probe);
			if (rc == SQLITE_ROW || rc == SQLITE_DONE)
				rc = SQLITE_OK;
		}
		sqlite3_finalize(probe);
		probe = NULL;
		if ((rc & 0xff) == SQLITE_CORRUPT || (rc & 0xff) == SQLITE_NOTADB) {
			bad_table = sqlite3_mprintf("%s", name);
			if (!bad_table)
				rc = SQLITE_NOMEM;
			break;
		}
		if (rc != SQLITE_OK)
			break;
	}
	sqlite3_finalize(tables);
	tables = NULL;
	if (bad_table) {
		sql = sqlite3_mprintf("DROP TABLE \"%w\"", bad_table);
		if (!sql) {
			sqlite3_free(bad_table);
			return SQLITE_NOMEM;
		}
		divoom_print("[DB] table %s is inaccessible; dropping it\n", bad_table);
		rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
		sqlite3_free(sql);
		if (rc != SQLITE_OK)
			divoom_print("[DB] failed to drop table %s: rc=%d, err=%s\n",
				bad_table, rc, sqlite3_errmsg(db));
		sqlite3_free(bad_table);
		bad_table = NULL;
		if (rc == SQLITE_OK)
			goto check_tables;
		return rc;
	}
	return rc == SQLITE_DONE ? SQLITE_OK : rc;
}

static int divoom_database_format_failed_userdata(const char *db_name);

/* Resource/locking errors must not destroy otherwise healthy database files. */
static int divoom_database_resource_error(int rc)
{
	int primary = rc & 0xff;
	return rc == SQLITE_IOERR_NOMEM || primary == SQLITE_NOMEM ||
		primary == SQLITE_BUSY || primary == SQLITE_LOCKED;
}

static int divoom_database_file_error(int rc)
{
	if (divoom_database_resource_error(rc))
		return 0;
	switch (rc & 0xff) {
	case SQLITE_CORRUPT:
	case SQLITE_NOTADB:
	case SQLITE_CANTOPEN:
	case SQLITE_IOERR:
	case SQLITE_FULL:
	case SQLITE_READONLY:
		return 1;
	default:
		return 0;
	}
}

/* Only a recreated primary DB needs the surviving photo files reindexed.
 * The table itself is the durable pending flag, removed with the import commit. */
static int divoom_database_mark_photo_rebuild(const char *db_name, sqlite3 *db)
{
	if (strcmp(db_name, DIVOOM_THUMB_DATA_BASE_FILE_NAME))
		return SQLITE_OK;
	return sqlite3_exec(db,
		"CREATE TABLE IF NOT EXISTS divoom_photo_rebuild(pending INTEGER)",
		NULL, NULL, NULL);
}

static int divoom_database_open_checked(const char *db_name, sqlite3 **ppdb)
{
	static const char *suffixes[] = { "-journal", "-wal", "-shm", "" };
	sqlite3_vfs *vfs = NULL;
	char *path, *file;
	const char *opened_path;
	int rc, i, exists, attempt, file_failed;
	rc = divoom_database_open_rw(db_name, ppdb);
	if (rc == SQLITE_OK)
		rc = divoom_database_check_accessible(*ppdb);
	if (rc == SQLITE_OK || !divoom_database_file_error(rc))
		return rc;
	divoom_print("[DB] open/check failed: %s rc=%d; trying file recovery\n", db_name, rc);

	/* A failed open may have no filename/VFS attached yet. Resolve the same
	 * logical path through the port in that case (including simulator mapping). */
	opened_path = *ppdb ? sqlite3_db_filename(*ppdb, "main") : NULL;
	if (*ppdb)
		sqlite3_file_control(*ppdb, "main", SQLITE_FCNTL_VFS_POINTER, &vfs);
	if (!vfs)
		vfs = sqlite3_vfs_find(NULL);
	if (!vfs || !vfs->xFullPathname)
		return rc;
	if (!opened_path || !opened_path[0])
		opened_path = divoom_port_path_resolve(db_name);
	if (!opened_path)
		return rc;
	path = sqlite3_malloc(vfs->mxPathname + 1);
	if (!path)
		return SQLITE_NOMEM;
	rc = vfs->xFullPathname(vfs, opened_path, vfs->mxPathname + 1, path);
	if (rc != SQLITE_OK) {
		sqlite3_free(path);
		return rc;
	}

	for (attempt = 0; attempt < 3; attempt++) {
		file_failed = 0;
		if (*ppdb) {
			rc = sqlite3_close(*ppdb);
			if (rc != SQLITE_OK)
				break; /* A live connection must never lose its files. */
			*ppdb = NULL;
		}
		/* Remove sidecars first. An absent file is already clean. */
		for (i = 0; i < (int)(sizeof(suffixes) / sizeof(suffixes[0])); i++) {
			file = sqlite3_mprintf("%s%s", path, suffixes[i]);
			if (!file) {
				rc = SQLITE_NOMEM;
				break;
			}
			rc = vfs->xAccess(vfs, file, SQLITE_ACCESS_EXISTS, &exists);
			if (rc == SQLITE_OK && exists) {
				rc = vfs->xDelete(vfs, file, 1);
				if (rc != SQLITE_OK && !divoom_database_resource_error(rc)) {
					int access_rc = vfs->xAccess(vfs, file, SQLITE_ACCESS_EXISTS, &exists);
					if (access_rc != SQLITE_OK) {
						rc = access_rc;
					} else if (!exists) {
						/* unlink may succeed even if its directory sync fails. */
						divoom_print("[DB] file removed despite VFS error: %s rc=%d\n", file, rc);
						rc = SQLITE_OK;
					}
				}
			}
			/* Repeated status-query failures also require volume recovery. */
			if (rc != SQLITE_OK && !divoom_database_resource_error(rc))
				file_failed = 1;
			sqlite3_free(file);
			if (rc != SQLITE_OK)
				break;
		}
		if (rc == SQLITE_OK) {
			rc = divoom_database_open_rw(db_name, ppdb);
			if (rc == SQLITE_OK)
				rc = divoom_database_check_accessible(*ppdb);
			if (rc == SQLITE_OK)
				rc = divoom_database_mark_photo_rebuild(db_name, *ppdb);
			if (rc == SQLITE_OK) {
				divoom_print("[DB] file recovery succeeded: %s attempt=%d\n", db_name, attempt + 1);
				break;
			}
		}
		divoom_print("[DB] file recovery failed: %s attempt=%d/3 rc=%d\n", db_name, attempt + 1, rc);
		if (divoom_database_resource_error(rc) ||
		    (!file_failed && !divoom_database_file_error(rc)))
			break;
		if (attempt < 2)
			divoom_system_delay_ms(200);
	}
	sqlite3_free(path);
	/* Three file-status/deletion/reopen failures exhaust file-level recovery.
	 * Resource exhaustion, locking and unsuccessful close never permit format. */
	if (attempt == 3) {
		/* Music initialization owns a local connection, not yet in the global
		 * controller. Close it too before any volume operation. */
		if (*ppdb) {
			int close_rc = sqlite3_close(*ppdb);
			if (close_rc != SQLITE_OK)
				return close_rc;
			*ppdb = NULL;
		}
		divoom_database_format_failed_userdata(db_name);
	}
	return rc;
}

typedef struct _divoom_database_manage_ctrl
{
    void *lock_handle;
    char sql_str[1024];
    sqlite3 *db_handle;
    sqlite3 *music_db_handle;
}divoom_database_manage_ctrl, *Pdivoom_database_manage_ctrl;


static Pdivoom_database_manage_ctrl pdivoom_database_manage_handle = NULL;
static int s_db_txn_depth = 0;
static int s_music_txn_depth = 0;

#if DIVOOM_BUILD_RTOS_DEVICE
typedef struct {
	const char *path;
	char *data;
	size_t size;
} divoom_database_saved_config;

static void divoom_database_save_config(divoom_database_saved_config *saved)
{
	divoom_port_stat_t st;
	void *file;
	divoom_cJSON *json;
	int attempt;
	if (!strcmp(saved->path, DIVOOM_SYS_CFG_SAVE_PATH)) {
		saved->data = divoom_product_test_userdata_config_snapshot();
		if (saved->data) {
			saved->size = strlen(saved->data);
			divoom_print("[DB] config saved in RAM: %s bytes=%u\n", saved->path, (unsigned)saved->size);
		} else {
			divoom_print("[DB] config backup unavailable: %s; recovery is best effort\n", saved->path);
		}
		return;
	}
	/* A corrupt length must not turn last-resort recovery into an OOM. */
	for (attempt = 0; attempt < 3; attempt++) {
		if (divoom_port_stat(saved->path, &st) != 0 || st.is_dir ||
		    st.size <= 0 || st.size > 64 * 1024)
			continue;
		saved->data = divoom_system_malloc((size_t)st.size + 1);
		if (!saved->data)
			break;
		file = divoom_port_file_open(saved->path, "rb");
		if (file) {
			size_t got = divoom_port_file_read(saved->data, 1, (size_t)st.size, file);
			int closed = divoom_port_file_close(file);
			if (got == (size_t)st.size && closed == 0) {
				saved->data[got] = '\0';
				json = divoom_cJSON_Parse(saved->data);
				if (json && (json->type & divoom_cJSON_Object)) {
					divoom_cJSON_Delete(json);
					saved->size = got;
					divoom_print("[DB] config saved in RAM: %s bytes=%u\n", saved->path, (unsigned)got);
					return;
				}
				if (json)
					divoom_cJSON_Delete(json);
			}
		}
		divoom_system_free(saved->data);
		saved->data = NULL;
	}
	divoom_print("[DB] config backup unavailable: %s; recovery is best effort\n", saved->path);
}

static int divoom_database_config_matches(const char *path, const divoom_database_saved_config *saved)
{
	divoom_port_stat_t st;
	unsigned char chunk[256];
	void *file;
	size_t offset = 0;
	int ok = 1;
	if (divoom_port_stat(path, &st) != 0 || st.size != (long)saved->size)
		return 0;
	file = divoom_port_file_open(path, "rb");
	if (!file)
		return 0;
	while (offset < saved->size) {
		size_t count = saved->size - offset;
		if (count > sizeof(chunk))
			count = sizeof(chunk);
		if (divoom_port_file_read(chunk, 1, count, file) != count ||
		    memcmp(chunk, saved->data + offset, count)) {
			ok = 0;
			break;
		}
		offset += count;
	}
	if (divoom_port_file_close(file) != 0)
		ok = 0;
	return ok;
}

static int divoom_database_restore_config(const divoom_database_saved_config *saved)
{
	char temp[256];
	void *file;
	int attempt, length, ok = 0;
	if (!saved->data)
		return 1; /* No readable original configuration was available. */
	length = snprintf(temp, sizeof(temp), "%s.db-recovery.tmp", saved->path);
	if (length < 0 || length >= (int)sizeof(temp))
		return 0;
	for (attempt = 1; attempt <= 3; attempt++) {
		file = divoom_port_file_open(temp, "wb");
		if (!file)
			continue;
		ok = divoom_port_file_write(saved->data, 1, saved->size, file) == saved->size;
		if (ok && divoom_port_file_sync(file) != 0)
			ok = 0;
		if (divoom_port_file_close(file) != 0)
			ok = 0;
		if (ok)
			ok = divoom_database_config_matches(temp, saved);
		/* FatFs rename cannot replace a seed file that already exists. */
		if (ok && divoom_port_file_exists(saved->path))
			ok = divoom_port_remove_file(saved->path) == 0;
		if (ok)
			ok = divoom_port_rename_file(temp, saved->path) == 0;
		if (ok) {
			ok = divoom_port_sync() == 0 && divoom_database_config_matches(saved->path, saved);
			if (ok)
				break;
		}
	}
	divoom_print("[DB] config restore %s: %s\n", ok ? "verified" : "failed", saved->path);
	return ok;
}
#endif

static int divoom_database_format_failed_userdata(const char *db_name)
{
#if DIVOOM_BUILD_RTOS_DEVICE
	divoom_database_saved_config saved[] = {
		{ DIVOOM_WPA_SUPPLICANT_CONF, NULL, 0 },
		{ DIVOOM_SYS_CFG_SAVE_PATH, NULL, 0 }
	};
	int i, restored = 1;
	const char *step = "format/remount";
	/* Called before Wi-Fi model initialization; its repair worker only waits
	 * for networking here, so no product-test pause/resume is required. */
	if (divoom_main_get_finish_start_up() ||
	    (strcmp(db_name, DIVOOM_THUMB_DATA_BASE_FILE_NAME) &&
	     strcmp(db_name, DIVOOM_TF_MUSIC_DATA_BASE_FILE_NAME))) {
		divoom_print("[DB] userdata format denied outside startup/database paths\n");
		return 0;
	}
	if (!strcmp(db_name, DIVOOM_THUMB_DATA_BASE_FILE_NAME))
		divoom_device_error_record(DIVOOM_SD_NAND_ERR_SQL_DB);
	divoom_database_manage_close(NULL);
	if (pdivoom_database_manage_handle &&
	    (pdivoom_database_manage_handle->db_handle ||
	     pdivoom_database_manage_handle->music_db_handle)) {
		divoom_print("[DB] userdata format denied: database is still open\n");
		return 0;
	}
#if defined(CONFIG_DRIVERS_AIC8800D80)
	if (!divoom_recovery_integrity_protect_wifi(0)) {
		divoom_print("[DB] userdata format denied: no verified Wi-Fi firmware on /usr\n");
		return 0;
	}
#endif
	divoom_log_close();
	for (i = 0; i < (int)(sizeof(saved) / sizeof(saved[0])); i++)
		divoom_database_save_config(&saved[i]);
	divoom_print("[DB] file recovery failed 3 times; formatting /userdata\n");
	if (divoom_port_fs_format_userdata() != 0)
		goto restore_failed;
	/* Seed before restoring credentials: next boot must not overwrite them. */
	step = "seed restore";
	if (divoom_port_copy_tree("/usr/userdata_seed", DIVOOM_USER_ROOT) != 0 ||
	    !divoom_port_file_exists(DIVOOM_THUMB_DATA_BASE_FILE_NAME) ||
	    divoom_port_mkdir_p(DIVOOM_USER_SYSTEM_DIR) != 0)
		goto restore_failed;
	step = "config restore";
	for (i = 0; i < (int)(sizeof(saved) / sizeof(saved[0])); i++)
		if (!divoom_database_restore_config(&saved[i]))
			restored = 0;
	if (!restored)
		goto restore_failed;
	for (i = 0; i < (int)(sizeof(saved) / sizeof(saved[0])); i++)
		divoom_system_free(saved[i].data);
	divoom_port_sync();
	divoom_print("[DB] userdata restored; rebooting to reopen databases\n");
	divoom_system_reboot();
	return 0; /* The caller exits this boot if reboot unexpectedly returns. */

restore_failed:
	/* mkfs can succeed even if remount fails. Keep the only config copies. */
	divoom_print("[DB] %s failed; startup stopped, config retained in RAM\n", step);
	for (;;)
		divoom_system_delay_ms(1000);
#else
	(void)db_name;
	return 0; /* Never erase a simulator host directory automatically. */
#endif
}

static int divoom_database_sqlite_exec(sqlite3 *db, const char *sql,
	int (*callback)(void *, int, char **, char **), void *callback_arg,
	char **errmsg)
{
	if (!db) {
		if (errmsg)
			*errmsg = sqlite3_mprintf("database is closed");
		divoom_print("[DB] rejected SQL execution: database is closed\n");
		return SQLITE_MISUSE;
	}
	return sqlite3_exec(db, sql, callback, callback_arg, errmsg);
}

static int divoom_database_sqlite_get_table(sqlite3 *db, const char *sql,
	char ***result, int *row_count, int *column_count, char **errmsg)
{
	if (!db) {
		if (result)
			*result = NULL;
		if (row_count)
			*row_count = 0;
		if (column_count)
			*column_count = 0;
		if (errmsg)
			*errmsg = sqlite3_mprintf("database is closed");
		divoom_print("[DB] rejected table query: database is closed\n");
		return SQLITE_MISUSE;
	}
	return sqlite3_get_table(db, sql, result, row_count, column_count, errmsg);
}

static int divoom_database_sqlite_prepare_v2(sqlite3 *db, const char *sql,
	int sql_bytes, sqlite3_stmt **stmt, const char **sql_tail)
{
	if (!db) {
		if (stmt)
			*stmt = NULL;
		if (sql_tail)
			*sql_tail = NULL;
		divoom_print("[DB] rejected statement preparation: database is closed\n");
		return SQLITE_MISUSE;
	}
	return sqlite3_prepare_v2(db, sql, sql_bytes, stmt, sql_tail);
}

static const char *divoom_database_sqlite_errmsg(sqlite3 *db)
{
	return db ? sqlite3_errmsg(db) : "database is closed";
}

/* Keep legacy callers safe after divoom_database_manage_close() clears a DB. */
#define sqlite3_exec divoom_database_sqlite_exec
#define sqlite3_get_table divoom_database_sqlite_get_table
#define sqlite3_prepare_v2 divoom_database_sqlite_prepare_v2
#define sqlite3_errmsg divoom_database_sqlite_errmsg

static void divoom_database_manage_format_sql(const char *zFormat, ...)
{
	va_list ap;

	if (!pdivoom_database_manage_handle)
		return;
	va_start(ap, zFormat);
	sqlite3_vsnprintf((int)sizeof(pdivoom_database_manage_handle->sql_str),
		pdivoom_database_manage_handle->sql_str, zFormat, ap);
	va_end(ap);
}

static int divoom_db_is_ready(void)
{
	return pdivoom_database_manage_handle != NULL &&
	       pdivoom_database_manage_handle->db_handle != NULL;
}

static int divoom_music_db_is_ready(void)
{
	return pdivoom_database_manage_handle != NULL &&
	       pdivoom_database_manage_handle->music_db_handle != NULL;
}

static int divoom_database_apply_music_pragmas(sqlite3 *db)
{
	/* Disable lookaside so the extra connection does not add ~48KB. */
	sqlite3_db_config(db, SQLITE_DBCONFIG_LOOKASIDE, (void *)0, 0, 0);
#if DIVOOM_BUILD_RTOS_DEVICE
	if (divoom_database_exec_pragma(db, "PRAGMA journal_mode = PERSIST;") != SQLITE_OK)
		return SQLITE_ERROR;
#endif
	if (divoom_database_exec_pragma(db, "PRAGMA synchronous = FULL;") != SQLITE_OK)
		return SQLITE_ERROR;
	if (divoom_database_exec_pragma(db, "PRAGMA cache_size = -96;") != SQLITE_OK)
		return SQLITE_ERROR;
	return SQLITE_OK;
}

#include "divoom_tf_music_db.inc.c"

static int divoom_database_open_music(void)
{
	int rc;
	sqlite3 *mdb = NULL;

	rc = divoom_database_open_checked(DIVOOM_TF_MUSIC_DATA_BASE_FILE_NAME, &mdb);
	if (rc) {
		divoom_print("Can't open database:%s err: %d, %s\n",
			DIVOOM_TF_MUSIC_DATA_BASE_FILE_NAME, rc,
			mdb ? sqlite3_errmsg(mdb) : "");
		sqlite3_close(mdb);
		return 0;
	}
	rc = divoom_database_apply_music_pragmas(mdb);
	if (rc != SQLITE_OK) {
		divoom_print("[DB] failed to enable tf_music.db pragmas for %s\n",
			DIVOOM_TF_MUSIC_DATA_BASE_FILE_NAME);
		sqlite3_close(mdb);
		return 0;
	}
	if (!divoom_tf_db_open_on(mdb)) {
		divoom_print("[DB] failed to init tf_music schema for %s\n",
			DIVOOM_TF_MUSIC_DATA_BASE_FILE_NAME);
		sqlite3_close(mdb);
		return 0;
	}
	pdivoom_database_manage_handle->music_db_handle = mdb;
	return 1;
}

int check_table_exists(const char *table_name)
{
    char sql[256];
    sqlite3_stmt *stmt;
    int exists = 0;

    if (!divoom_db_is_ready())
        return 0;
    
    sprintf(sql, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='%s'", table_name);
    
    if (sqlite3_prepare_v2(pdivoom_database_manage_handle->db_handle, sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            exists = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    
    return exists;
}


int divoom_database_manage_reset(char *db_name)
{
	int result;
#if !DIVOOM_BUILD_RTOS_DEVICE
	char reset_cmd[128];
#endif
	char sql_str[512];
	sqlite3 *db_handle = NULL;
	sqlite3 *source_handle = NULL;
	sqlite3_stmt *stmt = NULL;

	result = sqlite3_open(DIVOOM_THUMB_DATA_BASE_FILE_NAME, &source_handle);
	if(result)
	{
		divoom_print("Can't open database:%s err: %d, %s\n", DIVOOM_THUMB_DATA_BASE_FILE_NAME, result, sqlite3_errmsg(source_handle));
		sqlite3_close(source_handle);
		return 0;
	}
	
	result = sqlite3_open(db_name, &db_handle);
	if(result)
	{
		divoom_print("Can't open database:%s err: %d, %s\n", db_name, result, sqlite3_errmsg(db_handle));
        sqlite3_close(db_handle);
		sqlite3_close(source_handle);
		return 0;
	}
	// 获取源表结构
	sprintf(sql_str, "SELECT sql FROM sqlite_master WHERE type='table' AND name='%s'", DIVOOM_ALBUM_HEAD_TABLE_NAME);
    result = sqlite3_prepare_v2(source_handle, sql_str, -1, &stmt, NULL);
    if (result != SQLITE_OK) {
		divoom_print("reset fail: %s, %s\n", sql_str, sqlite3_errmsg(db_handle));
		sqlite3_close(source_handle);
		sqlite3_close(db_handle);
		return 0;
    } else {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *meta_sql = (const char *)sqlite3_column_text(stmt, 0);
            if (meta_sql) {
                // 执行元数据创建语句
                result = sqlite3_exec(db_handle, meta_sql, NULL, NULL, NULL);
                if (result != SQLITE_OK) {
         			divoom_print("reset fail: %s, %s\n", meta_sql, sqlite3_errmsg(db_handle));
					sqlite3_close(source_handle);
					sqlite3_close(db_handle);
					return 0;
                }
            }
        }
		else
		{
			divoom_print("Source table '%s' not found.\n", DIVOOM_ALBUM_HEAD_TABLE_NAME);
		}
        sqlite3_finalize(stmt);
    }

	//附加源数据库
	sprintf(sql_str, "ATTACH DATABASE '%s' AS source_db", DIVOOM_THUMB_DATA_BASE_FILE_NAME);
	result = sqlite3_exec(db_handle, sql_str, NULL, NULL, NULL);
	if (result != SQLITE_OK) {
        divoom_print("reset fail: %s, %s\n", sql_str, sqlite3_errmsg(db_handle));
        sqlite3_close(db_handle);
		sqlite3_close(source_handle);
        return 0;
    }
	// 复制数据
	//sprintf(sql_str, "CREATE TABLE %s AS SELECT * FROM source_db.%s", DIVOOM_ALBUM_HEAD_TABLE_NAME, DIVOOM_ALBUM_HEAD_TABLE_NAME);
	sprintf(sql_str, "INSERT INTO main.%s SELECT * FROM source_db.%s", DIVOOM_ALBUM_HEAD_TABLE_NAME, DIVOOM_ALBUM_HEAD_TABLE_NAME);
	result = sqlite3_exec(db_handle, sql_str, NULL, NULL, NULL);
	if (result != SQLITE_OK) {
        divoom_print("reset fail: %s, %s\n", sql_str, sqlite3_errmsg(db_handle));
        sqlite3_close(db_handle);
		sqlite3_close(source_handle);
        return 0;
    }
	
	///// 获取源表结构
	sprintf(sql_str, "SELECT sql FROM sqlite_master WHERE type='table' AND name='%s'", DIVOOM_ALBUM_DATA_TABLE_NAME);
	result = sqlite3_prepare_v2(source_handle, sql_str, -1, &stmt, NULL);
	if (result != SQLITE_OK) {
		divoom_print("reset fail: %s, %s\n", sql_str, sqlite3_errmsg(db_handle));
		sqlite3_close(source_handle);
		sqlite3_close(db_handle);
		return 0;
	} else {
		if (sqlite3_step(stmt) == SQLITE_ROW) {
			const char *meta_sql = (const char *)sqlite3_column_text(stmt, 0);
			if (meta_sql) {
				// 执行元数据创建语句
				result = sqlite3_exec(db_handle, meta_sql, NULL, NULL, NULL);
				if (result != SQLITE_OK) {
					divoom_print("reset fail: %s, %s\n", meta_sql, sqlite3_errmsg(db_handle));
					sqlite3_close(source_handle);
					sqlite3_close(db_handle);
					return 0;
				}
			}
		}
		else
		{
			divoom_print("Source table '%s' not found.\n", DIVOOM_ALBUM_DATA_TABLE_NAME);
		}
		sqlite3_finalize(stmt);
	}

	// 复制数据
	sprintf(sql_str, "INSERT INTO main.%s SELECT * FROM source_db.%s", DIVOOM_ALBUM_DATA_TABLE_NAME, DIVOOM_ALBUM_DATA_TABLE_NAME);
	result = sqlite3_exec(db_handle, sql_str, NULL, NULL, NULL);
	if (result != SQLITE_OK) {
        divoom_print("reset fail: %s, %s\n", sql_str, sqlite3_errmsg(db_handle));
        sqlite3_close(db_handle);
		sqlite3_close(source_handle);
        return 0;
    }

	/////// 获取源表结构
	sprintf(sql_str, "SELECT sql FROM sqlite_master WHERE type='table' AND name='%s'", DIVOOM_PIC_DATA_TABLE_NAME);
	result = sqlite3_prepare_v2(source_handle, sql_str, -1, &stmt, NULL);
	if (result != SQLITE_OK) {
		divoom_print("reset fail: %s, %s\n", sql_str, sqlite3_errmsg(db_handle));
		sqlite3_close(source_handle);
		sqlite3_close(db_handle);
		return 0;
	} else {
		if (sqlite3_step(stmt) == SQLITE_ROW) {
			const char *meta_sql = (const char *)sqlite3_column_text(stmt, 0);
			if (meta_sql) {
				// 执行元数据创建语句
				result = sqlite3_exec(db_handle, meta_sql, NULL, NULL, NULL);
				if (result != SQLITE_OK) {
					divoom_print("reset fail: %s, %s\n", meta_sql, sqlite3_errmsg(db_handle));
					sqlite3_close(source_handle);
					sqlite3_close(db_handle);
					return 0;
				}
			}
		}
		else
		{
			divoom_print("Source table '%s' not found.\n", DIVOOM_PIC_DATA_TABLE_NAME);
		}
		sqlite3_finalize(stmt);
	}
	// 复制数据
	sprintf(sql_str, "INSERT INTO main.%s SELECT * FROM source_db.%s", DIVOOM_PIC_DATA_TABLE_NAME, DIVOOM_PIC_DATA_TABLE_NAME);
	result = sqlite3_exec(db_handle, sql_str, NULL, NULL, NULL);
	if (result != SQLITE_OK) {
        divoom_print("reset fail: %s, %s\n", sql_str, sqlite3_errmsg(db_handle));
        sqlite3_close(db_handle);
		sqlite3_close(source_handle);
        return 0;
    }

	//////// 获取源表结构
	sprintf(sql_str, "SELECT sql FROM sqlite_master WHERE type='table' AND name='%s'", DIVOOM_CUSTOM_PHOTO_TABLE_NAME);
	result = sqlite3_prepare_v2(source_handle, sql_str, -1, &stmt, NULL);
	if (result != SQLITE_OK) {
		divoom_print("reset fail: %s, %s\n", sql_str, sqlite3_errmsg(db_handle));
		sqlite3_close(source_handle);
		sqlite3_close(db_handle);
		return 0;
	} else {
		if (sqlite3_step(stmt) == SQLITE_ROW) {
			const char *meta_sql = (const char *)sqlite3_column_text(stmt, 0);
			if (meta_sql) {
				// 执行元数据创建语句
				result = sqlite3_exec(db_handle, meta_sql, NULL, NULL, NULL);
				if (result != SQLITE_OK) {
					divoom_print("reset fail: %s, %s\n", meta_sql, sqlite3_errmsg(db_handle));
					sqlite3_close(source_handle);
					sqlite3_close(db_handle);
					return 0;
				}
			}
		}
		else
		{
			divoom_print("Source table '%s' not found.\n", DIVOOM_CUSTOM_PHOTO_TABLE_NAME);
		}
		sqlite3_finalize(stmt);
	}
	// 复制数据
	sprintf(sql_str, "INSERT INTO main.%s SELECT * FROM source_db.%s", DIVOOM_CUSTOM_PHOTO_TABLE_NAME, DIVOOM_CUSTOM_PHOTO_TABLE_NAME);
	result = sqlite3_exec(db_handle, sql_str, NULL, NULL, NULL);
	if (result != SQLITE_OK) {
        divoom_print("reset fail: %s, %s\n", sql_str, sqlite3_errmsg(db_handle));
        sqlite3_close(db_handle);
		sqlite3_close(source_handle);
        return 0;
    }

	//分离附加的数据库
	sprintf(sql_str, "DETACH DATABASE source_db");
	result = sqlite3_exec(db_handle, sql_str, NULL, NULL, NULL);
	if (result != SQLITE_OK) {
        divoom_print("reset fail: %s, %s\n", sql_str, sqlite3_errmsg(db_handle));
        sqlite3_close(db_handle);
		sqlite3_close(source_handle);
        return 0;
    }

	sqlite3_close(db_handle);
	sqlite3_close(source_handle);
#if DIVOOM_BUILD_RTOS_DEVICE
	divoom_port_remove_file(DIVOOM_THUMB_DATA_BASE_FILE_NAME);
	if (divoom_port_rename_file(db_name, DIVOOM_THUMB_DATA_BASE_FILE_NAME) != 0) {
		divoom_print("reset fail: rename %s -> %s\n", db_name, DIVOOM_THUMB_DATA_BASE_FILE_NAME);
		return 0;
	}
	divoom_port_sync();
#else
	sprintf(reset_cmd, "rm -f %s", DIVOOM_THUMB_DATA_BASE_FILE_NAME);
	divoom_exec_cmd(reset_cmd, NULL, 0);
	sprintf(reset_cmd, "chmod 777 %s", db_name);
	divoom_exec_cmd(reset_cmd, NULL, 0);
	sprintf(reset_cmd, "mv -f %s %s", db_name, DIVOOM_THUMB_DATA_BASE_FILE_NAME);
	divoom_exec_cmd(reset_cmd, NULL, 0);
#endif

	return 1;
}

void divoom_database_manage_add_table_index(char *table_name, char *item_name)
{
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    divoom_database_manage_format_sql( "CREATE INDEX idx_%s_%s ON %s(%s)", table_name, item_name, table_name, item_name);
    sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
}

void divoom_database_manage_run_sql_command(char *cmd_name)
{
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    sqlite3_exec(pdivoom_database_manage_handle->db_handle, cmd_name, NULL, NULL, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
}

/*********************************************************************
  Function:        divoom_database_manage_check_count_err
  Description:     检查符合这个命令的记录是否存在，
  Input:           cmd_info
                   
  Return:          存在返回1， 否则返回0
**********************************************************************/

int divoom_database_manage_check_count_err(char *cmd_info)
{
    int i, j, rc, pnRow, pnColumn, index, pic_id;
    
    char **dbResult;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);


    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, cmd_info, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {        
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return 0;
    } 
    sqlite3_free_table(dbResult);
    return pnRow > 0;
}

static int divoom_database_photo_file_type(const char *path)
{
	static const char *images[] = { ".jpg", ".jpeg", ".png", ".bmp", ".webp", ".gif" };
	const char *ext = strrchr(path, '.');
	unsigned int i;
	if (!ext)
		return 0;
	if (!strcasecmp(ext, ".mp4"))
		return 2;
	for (i = 0; i < sizeof(images) / sizeof(images[0]); i++)
		if (!strcasecmp(ext, images[i]))
			return 1;
	return 0;
}

static int divoom_database_scan_photos(sqlite3 *db, sqlite3_stmt *insert,
	const char *parent, unsigned int depth, int *added)
{
	const divoom_port_dirent_t *entry;
	divoom_port_stat_t st;
	void *dir;
	char *child;
	int rc = SQLITE_OK, length, type;
	/* Match the existing photo consumers' 256-byte path buffers. */
	if (depth > 16)
		return SQLITE_TOOBIG;
	child = divoom_system_malloc(256);
	if (!child)
		return SQLITE_NOMEM;
	dir = divoom_port_dir_open(parent);
	if (!dir) {
		divoom_system_free(child);
		return SQLITE_IOERR_READ;
	}
	for (;;) {
		errno = 0;
		entry = divoom_port_dir_read(dir);
		if (!entry) {
			if (errno)
				rc = SQLITE_IOERR_READ;
			break;
		}
		if (!strcmp(entry->name, ".") || !strcmp(entry->name, ".."))
			continue;
		/* dir_read uses shared storage: copy the name before recursing. */
		length = snprintf(child, 256, "%s/%s", parent, entry->name);
		if (length < 0 || length >= 256) {
			rc = SQLITE_TOOBIG;
			break;
		}
		if (divoom_port_stat(child, &st) != 0) {
			rc = SQLITE_IOERR_FSTAT;
			break;
		}
		if (st.is_dir) {
			if (!strcmp(entry->name, "divoom_thumb") || !strcmp(entry->name, "divoom"))
				continue; /* Legacy generated image directories are not originals. */
			rc = divoom_database_scan_photos(db, insert, child, depth + 1, added);
		} else if (st.size > 0 && (type = divoom_database_photo_file_type(child)) != 0) {
			rc = sqlite3_reset(insert);
			if (rc == SQLITE_OK)
				rc = sqlite3_bind_int64(insert, 1, (sqlite3_int64)st.mtime * 1000);
			if (rc == SQLITE_OK)
				rc = sqlite3_bind_int64(insert, 2, (sqlite3_int64)st.mtime);
			if (rc == SQLITE_OK)
				rc = sqlite3_bind_text(insert, 3, child, -1, SQLITE_TRANSIENT);
			if (rc == SQLITE_OK)
				rc = sqlite3_bind_int(insert, 4, type == 2);
			if (rc == SQLITE_OK)
				rc = sqlite3_step(insert);
			if (rc == SQLITE_DONE) {
				*added += sqlite3_changes(db);
				rc = SQLITE_OK;
			}
		}
		if (rc != SQLITE_OK)
			break;
	}
	if (divoom_port_dir_close(dir) != 0 && rc == SQLITE_OK)
		rc = SQLITE_IOERR_CLOSE;
	divoom_system_free(child);
	return rc;
}

static int divoom_database_restore_local_photos(sqlite3 *db)
{
	static const char *clear_dirs[] = { DIVOOM_USER_ROOT "/divoom/12", DIVOOM_USER_ROOT "/divoom/18" };
	sqlite3_stmt *stmt = NULL;
	unsigned int i;
	int rc, pending = 0, added = 0;
	rc = sqlite3_prepare_v2(db, "SELECT 1 FROM sqlite_master WHERE type='table' "
		"AND name='divoom_photo_rebuild'", -1, &stmt, NULL);
	if (rc == SQLITE_OK) {
		rc = sqlite3_step(stmt);
		pending = rc == SQLITE_ROW;
		if (rc == SQLITE_ROW || rc == SQLITE_DONE)
			rc = SQLITE_OK;
	}
	sqlite3_finalize(stmt);
	stmt = NULL;
	if (rc != SQLITE_OK || !pending)
		return rc;
	divoom_print("[DB] restoring local photos from %s\n", DIVOOM_SD_APP_PIC_DIR);
	for (i = 0; i < sizeof(clear_dirs) / sizeof(clear_dirs[0]); i++) {
		/* Creating first distinguishes an absent directory from an I/O failure. */
		if (divoom_port_mkdir_p(clear_dirs[i]) != 0 ||
		    divoom_port_remove_tree(clear_dirs[i]) != 0 ||
		    divoom_port_mkdir_p(clear_dirs[i]) != 0)
			return SQLITE_IOERR_DELETE;
		divoom_print("[DB] cleared %s\n", clear_dirs[i]);
	}
	if (divoom_port_mkdir_p(DIVOOM_SD_APP_PIC_DIR) != 0)
		return SQLITE_IOERR_READ;
	rc = sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, NULL);
	if (rc != SQLITE_OK)
		return rc;
	rc = sqlite3_exec(db,
		"CREATE INDEX IF NOT EXISTS idx_pic_info_path_name ON pic_info(path_name);"
		"INSERT INTO album_head(type,name) SELECT 1,'All' WHERE NOT EXISTS "
		"(SELECT 1 FROM album_head WHERE type=1)", NULL, NULL, NULL);
	if (rc == SQLITE_OK)
		rc = sqlite3_prepare_v2(db,
			"INSERT INTO pic_info(save_time,photo_time,path_name,pic_video_flag,pic_title) "
			"SELECT ?1,?2,?3,?4,'' WHERE NOT EXISTS "
			"(SELECT 1 FROM pic_info WHERE path_name=?3)", -1, &stmt, NULL);
	if (rc == SQLITE_OK)
		rc = divoom_database_scan_photos(db, stmt, DIVOOM_SD_APP_PIC_DIR, 0, &added);
	sqlite3_finalize(stmt);
	if (rc == SQLITE_OK)
		rc = sqlite3_exec(db, "DROP TABLE divoom_photo_rebuild", NULL, NULL, NULL);
	if (rc == SQLITE_OK)
		rc = sqlite3_exec(db, "COMMIT", NULL, NULL, NULL);
	if (rc != SQLITE_OK) {
		sqlite3_exec(db, "ROLLBACK", NULL, NULL, NULL);
		divoom_print("[DB] local photo recovery failed: rc=%d; pending retained\n", rc);
		return rc;
	}
	divoom_print("[DB] local photo recovery complete: %d files added\n", added);
	return SQLITE_OK;
}

int divoom_database_manage_init(char *db_name)
{
    int rc;
    
#if DIVOOM_BUILD_RTOS_DEVICE
    if (divoom_sqlite_rtos_init() != 0) {
        divoom_print("sqlite rtos init failed\n");
        return 0;
    }
#endif

    if (pdivoom_database_manage_handle) {
        divoom_database_manage_close(NULL);
        if (pdivoom_database_manage_handle->db_handle ||
            pdivoom_database_manage_handle->music_db_handle) {
            divoom_print("[DB] cannot reinitialize while a database is still open\n");
            return 0;
        }
        divoom_system_destroy_lock(pdivoom_database_manage_handle->lock_handle);
        divoom_system_free(pdivoom_database_manage_handle);
        pdivoom_database_manage_handle = NULL;
    }

    pdivoom_database_manage_handle = divoom_system_malloc(sizeof(divoom_database_manage_ctrl));
    if (!pdivoom_database_manage_handle) {
        divoom_print("[DB] failed to allocate database control\n");
        return 0;
    }
    memset(pdivoom_database_manage_handle, 0, sizeof(divoom_database_manage_ctrl));
    pdivoom_database_manage_handle->lock_handle = divoom_system_create_lock();
    if (!pdivoom_database_manage_handle->lock_handle) {
        divoom_print("[DB] failed to create database lock\n");
        divoom_system_free(pdivoom_database_manage_handle);
        pdivoom_database_manage_handle = NULL;
        return 0;
    }
    rc = divoom_database_open_checked(db_name, &pdivoom_database_manage_handle->db_handle);
    if (rc) {
        divoom_print("Can't open database:%s err: %d, %s\n", db_name, rc, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        sqlite3_close(pdivoom_database_manage_handle->db_handle);
        divoom_system_destroy_lock(pdivoom_database_manage_handle->lock_handle);
        divoom_system_free(pdivoom_database_manage_handle);
        pdivoom_database_manage_handle = NULL;
        return 0;
    }

    rc = divoom_database_apply_pragmas(pdivoom_database_manage_handle->db_handle);
    if (rc != SQLITE_OK) {
        divoom_print("[DB] failed to enable power-loss-safe persistence for %s\n", db_name);
        sqlite3_close(pdivoom_database_manage_handle->db_handle);
        divoom_system_destroy_lock(pdivoom_database_manage_handle->lock_handle);
        divoom_system_free(pdivoom_database_manage_handle);
        pdivoom_database_manage_handle = NULL;
        return 0;
    }
    //如果相簿表不存在的话，就创建一个相簿表
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, "CREATE TABLE IF NOT EXISTS album_head(id INTEGER PRIMARY KEY AUTOINCREMENT, type INTEGER, name TEXT)", NULL, NULL, NULL);
    if (rc) {
        divoom_print("Can't open create:%s err: %d, %s\n", "album_head", rc, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        goto init_failed;
    }
    #if 0
    //我们默认添加三个相簿，如果APP连接后将自动删除    
    divoom_database_manage_insert_album_item(1, 1, "");
    divoom_database_manage_insert_album_item(4, 0, "First");
    divoom_database_manage_insert_album_item(5, 0, "Second");
    divoom_database_manage_insert_album_item(6, 0, "third");
    #endif
    
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, "CREATE TABLE IF NOT EXISTS pic_info(id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "save_time BIGINT DEFAULT 0," 
        "photo_time BIGINT DEFAULT 0, "
        "like_time INTEGER DEFAULT 0, "
        "hide_time INTEGER DEFAULT 0, "
        "like_flag INTEGER DEFAULT 0, "
        "hide_flag INTEGER DEFAULT 0, "
        "fill_flag INTEGER DEFAULT 0, "
        "mirror_flag INTEGER DEFAULT 0, "
        "rotate_flag INTEGER DEFAULT 0, "
        "photo_x INTEGER DEFAULT 0, "
        "photo_y INTEGER DEFAULT 0, "
        "pic_video_flag INTEGER DEFAULT 0, "
        "width_flag INTEGER  DEFAULT 0, "
        "user_id INTEGER DEFAULT 0, "
        "pic_title CHAR(256), "
        "path_name CHAR(256) NOT NULL)", NULL, NULL, NULL);
    if (rc) {
        divoom_print("Can't open create:%s err: %d, %s\n", "pic_info", rc, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        goto init_failed;
    }

    //检查这个字段是否存在
    divoom_database_manage_check_column_in_table("pic_info", "width_flag", "INTEGER  DEFAULT 0");
    sqlite3_exec(pdivoom_database_manage_handle->db_handle, "CREATE INDEX idx_pic_info_width_flag ON pic_info(width_flag)", NULL, NULL, NULL);
    sqlite3_exec(pdivoom_database_manage_handle->db_handle, "CREATE INDEX idx_pic_info_save_time ON pic_info(save_time)", NULL, NULL, NULL);
    sqlite3_exec(pdivoom_database_manage_handle->db_handle, "CREATE INDEX idx_pic_info_photo_time ON pic_info(photo_time)", NULL, NULL, NULL);
    
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, "CREATE TABLE IF NOT EXISTS album_pic(id INTEGER PRIMARY KEY AUTOINCREMENT, album_id INTEGER NOT NULL, pic_id INTEGER NOT NULL)", NULL, NULL, NULL);
    if (rc) {
        divoom_print("Can't open create:%s err: %d, %s\n", "album_pic", rc, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        goto init_failed;
    }
    sqlite3_exec(pdivoom_database_manage_handle->db_handle, "CREATE INDEX idx_album_pic_album_id ON album_pic(album_id)", NULL, NULL, NULL);
    sqlite3_exec(pdivoom_database_manage_handle->db_handle, "CREATE INDEX idx_album_pic_pic_id ON album_pic(pic_id)", NULL, NULL, NULL);

    rc = divoom_database_restore_local_photos(pdivoom_database_manage_handle->db_handle);
    if (rc != SQLITE_OK) {
        divoom_print("[DB] failed to restore local photos: rc=%d\n", rc);
        goto init_failed;
    }
    if (!divoom_database_open_music()) {
        divoom_print("[DB] failed to open %s\n", DIVOOM_TF_MUSIC_DATA_BASE_FILE_NAME);
        goto init_failed;
    }
    return 1;

init_failed:
    sqlite3_close(pdivoom_database_manage_handle->db_handle);
    divoom_system_destroy_lock(pdivoom_database_manage_handle->lock_handle);
    divoom_system_free(pdivoom_database_manage_handle);
    pdivoom_database_manage_handle = NULL;
    return 0;
}

/*********************************************************************
  Function:        divoom_database_manage_check_column_in_table
  Description:     检查是否包含某个字段，如果没有包括，将增加
  Input:           table_name
                   column_name
                   new_column_definition
  Return:          
**********************************************************************/

int divoom_database_manage_check_column_in_table(char *table_name, char *column_name, char *new_column_definition)
{
    int i, index, rc, pnRow, pnColumn;
    char **dbResult;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    divoom_database_manage_format_sql( "PRAGMA table_info(%s)", table_name);

    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return 0;
    }
    if(pnRow)
    {
        index = pnColumn;
        for(i = 0; i < pnRow; i++)
        {
            if(strcmp(dbResult[index + 1], column_name) == 0)
            {
                break;
            }
            index += pnColumn;
        }
        if(i >= pnRow)
        {
            
            divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
            divoom_database_manage_format_sql( "ALTER TABLE %s ADD COLUMN %s %s",
                     table_name, column_name, new_column_definition);
            divoom_print("insert info: %s\n",pdivoom_database_manage_handle->sql_str);
            
            rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL); // No callback needed here
            divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
        }
    }
    sqlite3_free_table(dbResult);
    return 1;
}

/*********************************************************************
  Function:        divoom_database_manage_insert_custom_photo_item
  Description:     插入用户自定义相簿
  Input:           clock_info
                   save_time
                   photo_time
                   file_name
  Return:          返回插入的ID
**********************************************************************/

int divoom_database_manage_insert_custom_photo_item(Pdivoom_clock_id_info clock_info, double save_time, int photo_time, char *file_name)
{
    static const char insert_sql[] =
        "INSERT INTO " DIVOOM_CUSTOM_PHOTO_TABLE_NAME
        "(parent_id,pos,clock_id,save_time,photo_time,file_name)"
        " VALUES(?,?,?,?,?,?)";
    sqlite3_stmt *stmt = NULL;
    int rc;
    int record_id = 0;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

    rc = sqlite3_prepare_v2(pdivoom_database_manage_handle->db_handle,
                            insert_sql, -1, &stmt, NULL);
    if (rc == SQLITE_OK)
        rc = sqlite3_bind_int(stmt, 1, clock_info->parent_id);
    if (rc == SQLITE_OK)
        rc = sqlite3_bind_int(stmt, 2, clock_info->clock_pos);
    if (rc == SQLITE_OK)
        rc = sqlite3_bind_int(stmt, 3, clock_info->clock_id);
    if (rc == SQLITE_OK)
        rc = sqlite3_bind_int64(stmt, 4, (sqlite3_int64)save_time);
    if (rc == SQLITE_OK)
        rc = sqlite3_bind_int(stmt, 5, photo_time);
    if (rc == SQLITE_OK)
        rc = sqlite3_bind_text(stmt, 6, file_name ? file_name : "",
                               -1, SQLITE_TRANSIENT);
    if (rc == SQLITE_OK)
        rc = sqlite3_step(stmt);

    if (rc == SQLITE_DONE) {
        record_id = (int)sqlite3_last_insert_rowid(
            pdivoom_database_manage_handle->db_handle);
    } else {
        divoom_print("Insert custom photo failed: path=%s ; %s\n",
                     file_name ? file_name : "",
                     sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
    }

    if (stmt != NULL)
        sqlite3_finalize(stmt);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    return record_id;
}

/*********************************************************************
  Function:        divoom_database_manage_insert_vision_photo_item
  Description:     插入用户自定义相簿
  Input:           clock_info
                   save_time
                   photo_time
                   file_name
  Return:          返回插入的ID
**********************************************************************/

int divoom_database_manage_insert_vision_photo_item(char *table_name,int vision_id, char *file_name)
{
    int rc, pnRow, pnColumn;
    char **dbResult;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    
    divoom_database_manage_format_sql( "INSERT INTO %s(id,file_name) VALUES(%d,'%s')", table_name,
        vision_id, file_name);
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
        divoom_print("Insert failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return 0;
    } else {
        int record_id = sqlite3_last_insert_rowid(pdivoom_database_manage_handle->db_handle);
        divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
        return  record_id;
    }
}


/*********************************************************************
  Function:        divoom_database_manage_insert_vision_photo_item_with_time
  Description:     插入用户自定义相簿
  Input:           clock_info
                   save_time
                   photo_time
                   file_name
  Return:          返回插入的ID
**********************************************************************/

int divoom_database_manage_insert_vision_photo_item_with_time(char *table_name,int vision_id, char *file_name, int update_time)
{
    int rc, pnRow, pnColumn;
    char **dbResult;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    
    divoom_database_manage_format_sql( "INSERT INTO %s(id,update_time,file_name) VALUES(%d,%d,'%s')", table_name,
        vision_id, update_time, file_name);
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
        divoom_print("Insert failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return 0;
    } else {
        int record_id = sqlite3_last_insert_rowid(pdivoom_database_manage_handle->db_handle);
        divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
        return  record_id;
    }
}

/*********************************************************************
  Function:        divoom_database_manage_delete_custom_photo_item
  Description:     删除某个记录
  Input:           id
  Return:          none
**********************************************************************/

void divoom_database_manage_delete_vision_photo_item(char *table_name,int id)
{
    int rc;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    divoom_database_manage_format_sql( "DELETE FROM %s WHERE id=%d", table_name, id);
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("Insert failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
    }
}

/*********************************************************************
  Function:        divoom_database_manage_update_vision_photo_item_download
  Description:     更新下载标记
  Input:           id
  Return:          none
**********************************************************************/

void divoom_database_manage_update_vision_photo_item_download(char *table_name, int id)
{
    int rc;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    divoom_database_manage_format_sql( "UPDATE %s SET download=1 WHERE id=%d", table_name, id);
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("Insert failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
    }
}

/*********************************************************************
  Function:        divoom_database_manage_get_vision_photo_name
  Description:     获取记录的ＯＳＳ_ＮＡＭＥ
  Input:           oss_name
  Return:          ０　ＯＲ　１
**********************************************************************/


int divoom_database_manage_get_vision_photo_name(char *table_name, int vision,char *oss_name)
{
    int rc, pnRow, pnColumn;
    char **dbResult;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    divoom_database_manage_format_sql( "SELECT file_name FROM %s WHERE id=%d", table_name, vision);

    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return 0;
    }
    if(pnRow)
    {
        strcpy(oss_name, dbResult[pnColumn]);
    }
    sqlite3_free_table(dbResult);
    return 1;
}

/*********************************************************************
  Function:        divoom_database_manage_get_vision_last_update_time
  Description:     获取记录的最大更新时间
  Input:           none
  Return:          update_time
**********************************************************************/


int divoom_database_manage_get_vision_last_update_time(char *table_name)
{
    int rc, pnRow, pnColumn, update_time = 0;
    char **dbResult;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    divoom_database_manage_format_sql( "SELECT update_time FROM %s ORDER BY update_time DESC LIMIT 1", table_name);

    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return 0;
    }
    if(pnRow)
    {
        update_time = atoi(dbResult[pnColumn]);
    }
    sqlite3_free_table(dbResult);
    return update_time;
}


/*********************************************************************
  Function:        divoom_database_manage_get_vision_photo_list
  Description:     获取所有记录，返回ID
  Input:           pic_num
  Return:          list
**********************************************************************/


Pdivoom_vision_item divoom_database_manage_get_vision_photo_list(char *table_name,int *pic_num)
{
    Pdivoom_vision_item pic_list = NULL;
    int i, index, rc, pnRow, pnColumn;
    char **dbResult;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    divoom_database_manage_format_sql( "SELECT id,download FROM %s ORDER BY id ASC", table_name);

    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return NULL;
    }
    *pic_num = pnRow;
    if(pnRow)
    {
        pic_list = divoom_system_malloc(pnRow * sizeof(divoom_vision_item));
        index = pnColumn;
        for(i = 0; i < pnRow; i++)
        {
            pic_list[i].vision_id = atoi(dbResult[index]);
            index ++;
            pic_list[i].download_flag = atoi(dbResult[index]);
            index ++;
        }
    }
    sqlite3_free_table(dbResult);
    return pic_list;
}


/*********************************************************************
  Function:        divoom_database_manage_delete_custom_photo_item
  Description:     删除某个记录
  Input:           id
  Return:          none
**********************************************************************/

void divoom_database_manage_delete_custom_photo_item(int id)
{
    int rc;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    divoom_database_manage_format_sql( "DELETE FROM %s WHERE id=%d", DIVOOM_CUSTOM_PHOTO_TABLE_NAME, id);
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("Insert failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
    }
}

/*********************************************************************
  Function:        divoom_database_manage_delete_custom_photo_all
  Description:     清除整个记录
  Input:           clock_info
  Return:          none
**********************************************************************/

void divoom_database_manage_delete_custom_photo_all(Pdivoom_clock_id_info clock_info)
{
    int rc;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    divoom_database_manage_format_sql( "DELETE FROM %s WHERE parent_id=%d AND pos=%d AND clock_id=%d", DIVOOM_CUSTOM_PHOTO_TABLE_NAME, clock_info->parent_id, clock_info->clock_pos, clock_info->clock_id);
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("Insert failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
    }
}

/*********************************************************************
  Function:        divoom_database_manage_get_total_custom_pic_num
  Description:     照片总数
  Input:           clock_info
                   order_flag
                   reverse_flag
                   save_time <0 全部，否则返回比这个时间之前或之后的数目
                   
  Return:          照片总数
**********************************************************************/

int divoom_database_manage_get_total_custom_pic_num(Pdivoom_clock_id_info clock_info, int order_flag, int reverse_flag, double save_time)
{
    int i, j, rc, pnRow, pnColumn, index, pic_num;
    
    char **dbResult;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

    if(save_time > 0)
    {
        if(order_flag == DIVOOM_PIC_PHOTO_SLIDE_DISP_PHTOT_TIME)
        {
            if(reverse_flag==0)
            {
                divoom_database_manage_format_sql( "SELECT COUNT(*) FROM %s WHERE parent_id=%d AND pos=%d AND clock_id=%d AND photo_time>%lld", DIVOOM_CUSTOM_PHOTO_TABLE_NAME,
                    clock_info->parent_id, clock_info->clock_pos, clock_info->clock_id, (long long)save_time);
            }
            else
            {
                divoom_database_manage_format_sql( "SELECT COUNT(*) FROM %s WHERE parent_id=%d AND pos=%d AND clock_id=%d AND photo_time<%lld", DIVOOM_CUSTOM_PHOTO_TABLE_NAME,
                    clock_info->parent_id, clock_info->clock_pos, clock_info->clock_id, (long long)save_time);
            }
        }
        else
        {
            if(reverse_flag==0)
            {
                divoom_database_manage_format_sql( "SELECT COUNT(*) FROM %s WHERE parent_id=%d AND pos=%d AND clock_id=%d AND save_time>%lld", DIVOOM_CUSTOM_PHOTO_TABLE_NAME,
                    clock_info->parent_id, clock_info->clock_pos, clock_info->clock_id, (long long)save_time);
            }
            else
            {
                divoom_database_manage_format_sql( "SELECT COUNT(*) FROM %s WHERE parent_id=%d AND pos=%d AND clock_id=%d AND save_time<%lld", DIVOOM_CUSTOM_PHOTO_TABLE_NAME,
                    clock_info->parent_id, clock_info->clock_pos, clock_info->clock_id, (long long)save_time);
            }
        }
    }
    else
    {
        divoom_database_manage_format_sql( "SELECT COUNT(*) FROM %s WHERE parent_id=%d AND pos=%d AND clock_id=%d ", DIVOOM_CUSTOM_PHOTO_TABLE_NAME, 
            clock_info->parent_id, clock_info->clock_pos, clock_info->clock_id);
    }

    divoom_print("sql info: %d, %d, %s\n", order_flag, reverse_flag, pdivoom_database_manage_handle->sql_str);

    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    if (rc != SQLITE_OK) {        
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
        return 0;
    }
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    index = pnColumn;
    if(pnRow > 0)
    {
        pic_num = atoi(dbResult[index]);
    }
    else
    {
        pic_num = 0;
    }
    sqlite3_free_table(dbResult);
    return pic_num;
}

/*********************************************************************
  Function:        divoom_database_manage_get_custom_photo_list
  Description:     获取所有这个表盘的记录，返回ID
  Input:           clock_info
                   pic_num
                   order_flag
                   order_reverse
  Return:          none
**********************************************************************/


uint32 *divoom_database_manage_get_custom_photo_list(Pdivoom_clock_id_info clock_info, int *pic_num, int order_flag, int order_reverse, double start_time)
{
    uint32 *pic_list = NULL;
    int i, index, rc, pnRow, pnColumn;
    char **dbResult;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    if(order_flag == DIVOOM_PIC_PHOTO_SLIDE_DISP_PHTOT_TIME)
    {
        if(start_time < 0)
        {
            divoom_database_manage_format_sql( "SELECT id FROM %s WHERE parent_id=%d AND pos=%d AND clock_id=%d ORDER BY photo_time %s LIMIT %d", DIVOOM_CUSTOM_PHOTO_TABLE_NAME, 
                clock_info->parent_id, clock_info->clock_pos, clock_info->clock_id, order_reverse==0?"DESC":"ASC", *pic_num);
        }
        else
        {
            divoom_database_manage_format_sql( "SELECT id FROM %s WHERE parent_id=%d AND pos=%d AND clock_id=%d AND photo_time%s%lld ORDER BY photo_time %s LIMIT %d", DIVOOM_CUSTOM_PHOTO_TABLE_NAME,
                clock_info->parent_id, clock_info->clock_pos, clock_info->clock_id, order_reverse==0?"<":">", (long long)start_time, order_reverse==0?"DESC":"ASC", *pic_num);
        }
    }
    else
    {
        if(start_time < 0)
        {
            divoom_database_manage_format_sql( "SELECT id FROM %s WHERE parent_id=%d AND pos=%d AND clock_id=%d ORDER BY save_time %s LIMIT %d", DIVOOM_CUSTOM_PHOTO_TABLE_NAME, 
                clock_info->parent_id, clock_info->clock_pos, clock_info->clock_id, order_reverse==0?"DESC":"ASC", *pic_num);
        }
        else
        {
            divoom_database_manage_format_sql( "SELECT id FROM %s WHERE parent_id=%d AND pos=%d AND clock_id=%d AND save_time%s%lld ORDER BY save_time %s LIMIT %d", DIVOOM_CUSTOM_PHOTO_TABLE_NAME,
                clock_info->parent_id, clock_info->clock_pos, clock_info->clock_id, order_reverse==0?"<":">", (long long)start_time, order_reverse==0?"DESC":"ASC", *pic_num);
        }
    }
    divoom_print("request: %s \n", pdivoom_database_manage_handle->sql_str);

    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    if (rc != SQLITE_OK) {
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
        return NULL;
    }
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    *pic_num = pnRow;
    if(pnRow)
    {
        pic_list = divoom_system_malloc(pnRow * sizeof(uint32));
        index = pnColumn;
        for(i = 0; i < pnRow; i++)
        {
            pic_list[i] = atoi(dbResult[index]);
            index ++;
        }
    }
    sqlite3_free_table(dbResult);
    return pic_list;
}

/*********************************************************************
  Function:        divoom_database_manage_get_custom_photo_list_reverse
  Description:     到序获取前面一些记录
  Input:           clock_info
                   pic_num
                   order_flag
                   order_reverse
  Return:          none
**********************************************************************/


uint32 *divoom_database_manage_get_custom_photo_list_reverse(Pdivoom_clock_id_info clock_info, int *pic_num, int order_flag, int order_reverse, double start_time)
{
    uint32 *pic_list = NULL;
    int i, index, rc, pnRow, pnColumn;
    char **dbResult;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    if(order_flag == DIVOOM_PIC_PHOTO_SLIDE_DISP_PHTOT_TIME)
    {
        if(start_time < 0)
        {
            divoom_database_manage_format_sql( "SELECT id FROM %s WHERE parent_id=%d AND pos=%d AND clock_id=%d ORDER BY photo_time %s LIMIT %d", DIVOOM_CUSTOM_PHOTO_TABLE_NAME, 
                clock_info->parent_id, clock_info->clock_pos, clock_info->clock_id, order_reverse==0?"ASC":"DESC", *pic_num);
        }
        else
        {
            divoom_database_manage_format_sql( "SELECT id FROM %s WHERE parent_id=%d AND pos=%d AND clock_id=%d AND photo_time%s%lld ORDER BY photo_time %s LIMIT %d", DIVOOM_CUSTOM_PHOTO_TABLE_NAME,
                clock_info->parent_id, clock_info->clock_pos, clock_info->clock_id, order_reverse==0?">":"<", (long long)start_time, order_reverse==0?"ASC":"DESC", *pic_num);
        }
    }
    else
    {
        if(start_time < 0)
        {
            divoom_database_manage_format_sql( "SELECT id FROM %s WHERE parent_id=%d AND pos=%d AND clock_id=%d ORDER BY save_time %s LIMIT %d", DIVOOM_CUSTOM_PHOTO_TABLE_NAME, 
                clock_info->parent_id, clock_info->clock_pos, clock_info->clock_id, order_reverse==0?"ASC":"DESC", *pic_num);
        }
        else
        {
            divoom_database_manage_format_sql( "SELECT id FROM %s WHERE parent_id=%d AND pos=%d AND clock_id=%d AND save_time%s%lld ORDER BY save_time %s LIMIT %d", DIVOOM_CUSTOM_PHOTO_TABLE_NAME,
                clock_info->parent_id, clock_info->clock_pos, clock_info->clock_id, order_reverse==0?">":"<", (long long)start_time, order_reverse==0?"ASC":"DESC", *pic_num);
        }
    }
    divoom_print("request: %s \n", pdivoom_database_manage_handle->sql_str);

    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    if (rc != SQLITE_OK) {
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
        return NULL;
    }
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    *pic_num = pnRow;
    if(pnRow)
    {
        pic_list = divoom_system_malloc(pnRow * sizeof(uint32));
        index = pnColumn;
        for(i = 0; i < pnRow; i++)
        {
            pic_list[i] = atoi(dbResult[index]);
            index ++;
        }
    }
    sqlite3_free_table(dbResult);
    return pic_list;
}


/*********************************************************************
  Function:        divoom_database_manage_get_custom_local_photo_list
  Description:     获取所有这个表盘的记录，返回ID
  Input:           clock_info
                   pic_num
                   start_time
  Return:          none
**********************************************************************/


Pdivoom_pic_id_name_info divoom_database_manage_get_custom_local_photo_list(Pdivoom_clock_id_info clock_info, int *pic_num,double start_time, int order_flag, int order_reverse)
{
    Pdivoom_pic_id_name_info pic_list = NULL;
    int i, index, rc, pnRow, pnColumn;
    char **dbResult;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    
    if(order_flag == DIVOOM_PIC_PHOTO_SLIDE_DISP_PHTOT_TIME)
    {
        if(order_reverse == 0)
        {
            if(start_time < 0)
            {
                divoom_database_manage_format_sql( "SELECT id,photo_time,file_name FROM %s WHERE parent_id=%d AND pos=%d AND clock_id=%d ORDER BY photo_time DESC LIMIT %d", DIVOOM_CUSTOM_PHOTO_TABLE_NAME, 
                    clock_info->parent_id, clock_info->clock_pos, clock_info->clock_id, *pic_num);
            }
            else
            {
                divoom_database_manage_format_sql( "SELECT id,photo_time,file_name FROM %s WHERE parent_id=%d AND pos=%d AND clock_id=%d AND photo_time<%d ORDER BY photo_time DESC LIMIT %d", DIVOOM_CUSTOM_PHOTO_TABLE_NAME, 
                    clock_info->parent_id, clock_info->clock_pos, clock_info->clock_id, (int)start_time,  *pic_num);
            }
        }
        else
        {
            if(start_time < 0)
            {
                divoom_database_manage_format_sql( "SELECT id,photo_time,file_name FROM %s WHERE parent_id=%d AND pos=%d AND clock_id=%d ORDER BY photo_time LIMIT %d", DIVOOM_CUSTOM_PHOTO_TABLE_NAME, 
                    clock_info->parent_id, clock_info->clock_pos, clock_info->clock_id, *pic_num);
            }
            else
            {
                divoom_database_manage_format_sql( "SELECT id,photo_time,file_name FROM %s WHERE parent_id=%d AND pos=%d AND clock_id=%d AND photo_time>%d ORDER BY photo_time LIMIT %d", DIVOOM_CUSTOM_PHOTO_TABLE_NAME, 
                    clock_info->parent_id, clock_info->clock_pos, clock_info->clock_id, (int)start_time,  *pic_num);
            }
        }
    }
    else
    {
        if(order_reverse == 0)
        {
            if(start_time < 0)
            {
                divoom_database_manage_format_sql( "SELECT id,save_time,file_name FROM %s WHERE parent_id=%d AND pos=%d AND clock_id=%d ORDER BY save_time DESC LIMIT %d", DIVOOM_CUSTOM_PHOTO_TABLE_NAME, 
                    clock_info->parent_id, clock_info->clock_pos, clock_info->clock_id, *pic_num);
            }
            else
            {
                divoom_database_manage_format_sql( "SELECT id,save_time,file_name FROM %s WHERE parent_id=%d AND pos=%d AND clock_id=%d AND save_time<%lld ORDER BY save_time DESC LIMIT %d", DIVOOM_CUSTOM_PHOTO_TABLE_NAME,
                    clock_info->parent_id, clock_info->clock_pos, clock_info->clock_id, (long long)start_time,  *pic_num);
            }
        }
        else
        {
            if(start_time < 0)
            {
                divoom_database_manage_format_sql( "SELECT id,save_time,file_name FROM %s WHERE parent_id=%d AND pos=%d AND clock_id=%d ORDER BY save_time LIMIT %d", DIVOOM_CUSTOM_PHOTO_TABLE_NAME, 
                    clock_info->parent_id, clock_info->clock_pos, clock_info->clock_id, *pic_num);
            }
            else
            {
                divoom_database_manage_format_sql( "SELECT id,save_time,file_name FROM %s WHERE parent_id=%d AND pos=%d AND clock_id=%d AND save_time>%lld ORDER BY save_time LIMIT %d", DIVOOM_CUSTOM_PHOTO_TABLE_NAME,
                    clock_info->parent_id, clock_info->clock_pos, clock_info->clock_id, (long long)start_time,  *pic_num);
            }
        }
    }
    divoom_print("aj test: %s, start_time:%lld\n",
                 pdivoom_database_manage_handle->sql_str,
                 (long long)start_time);

    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    if (rc != SQLITE_OK) {
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
        return NULL;
    }
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    *pic_num = pnRow;
    if(pnRow)
    {
        pic_list = divoom_system_malloc(pnRow * sizeof(divoom_pic_id_name_info));
        index = pnColumn;
        for(i = 0; i < pnRow; i++)
        {
            pic_list[i].id = atoi(dbResult[index]);
            index ++;
            if(order_flag == DIVOOM_PIC_PHOTO_SLIDE_DISP_PHTOT_TIME)
            {
                pic_list[i].time = atoi(dbResult[index])/ 1000;
            }
            else
            {
                pic_list[i].time = atof(dbResult[index])/ 1000;
            }
            index ++;
            strcpy(pic_list[i].name , dbResult[index]);
            index ++;
        }
    }
    sqlite3_free_table(dbResult);
    return pic_list;
}

/*********************************************************************
  Function:        divoom_database_manage_get_custom_photo_list
  Description:     获取所有这个表盘的记录，返回ID
  Input:           clock_info
                   pic_num
                   order_flag
                   order_reverse
  Return:          none
**********************************************************************/


int divoom_database_manage_get_custom_photo_name(int id, char *file_name)
{
    int rc, pnRow, pnColumn;
    char **dbResult;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    //const char *sql_query = "SELECT * FROM employees WHERE id=1";
    divoom_database_manage_format_sql( "SELECT file_name FROM %s WHERE id=%d", DIVOOM_CUSTOM_PHOTO_TABLE_NAME, id);

    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return 0;
    }
    if(pnRow)
    {
        strcpy(file_name, dbResult[pnColumn]);
    }
    sqlite3_free_table(dbResult);
    return 1;
}


/*********************************************************************
  Function:        divoom_database_manage_get_custom_photo_info
  Description:     获取所有这个表盘的记录，返回ID
  Input:           clock_info
                   pic_info
  Return:          none
**********************************************************************/


int divoom_database_manage_get_custom_photo_info(int id, divoom_custom_photo_info *pic_info)
{
    int rc, pnRow, pnColumn;
    char **dbResult;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    //const char *sql_query = "SELECT * FROM employees WHERE id=1";
    divoom_database_manage_format_sql( "SELECT save_time,photo_time FROM %s WHERE id=%d", DIVOOM_CUSTOM_PHOTO_TABLE_NAME, id);

    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return 0;
    }
    if(pnRow)
    {
        pic_info->save_time = atof(dbResult[pnColumn]);
        pic_info->photo_time = atoi(dbResult[pnColumn+1]);
        divoom_print("aj test! %lld, %d\n",
                     (long long)pic_info->save_time, pic_info->photo_time);
    }
    else
    {
        divoom_print("aj test!\n");
    }
    sqlite3_free_table(dbResult);
    return 1;
}


/*********************************************************************
  Function:        divoom_database_manage_insert_music_item
  Description:     插入一首歌曲信息
  Input:           item_data
  Return:          返回插入的ID
**********************************************************************/

int divoom_database_manage_insert_music_item(char *item_data, const char *root_name)
{
    int rc;
    int record_id;
    const char *proot = (root_name != NULL) ? root_name : "";

    if (!divoom_music_db_is_ready())
        return 0;

    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

    divoom_database_manage_format_sql(
        "INSERT INTO local_music(name, root_name) VALUES('%s', '%s')", item_data, proot);
    rc = sqlite3_exec(pdivoom_database_manage_handle->music_db_handle,
        pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    if (rc != SQLITE_OK)
    {
        divoom_database_manage_format_sql(
            "INSERT INTO local_music(name) VALUES('%s')", item_data);
        rc = sqlite3_exec(pdivoom_database_manage_handle->music_db_handle,
            pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    }
    if (rc != SQLITE_OK)
    {
        divoom_print("Insert failed: %s ; %s\n",
            pdivoom_database_manage_handle->sql_str,
            sqlite3_errmsg(pdivoom_database_manage_handle->music_db_handle));
        divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
        return 0;
    }
    record_id = (int)sqlite3_last_insert_rowid(pdivoom_database_manage_handle->music_db_handle);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    return record_id;
}


/*********************************************************************
  Function:        divoom_database_manage_delete_music_item
  Description:     删除某首歌曲
  Input:           id
  Return:          none
**********************************************************************/

void divoom_database_manage_delete_music_item(int id)
{
    int rc;

    if (!divoom_music_db_is_ready())
        return;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    divoom_database_manage_format_sql( "DELETE FROM local_music WHERE id=%d", id);
    rc = sqlite3_exec(pdivoom_database_manage_handle->music_db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("Insert failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->music_db_handle));
    }
}


/*********************************************************************
  Function:        divoom_database_manage_update_music_like
  Description:     更新某首歌曲的收藏标志
  Input:           id
                   like_flag
  Return:          none
**********************************************************************/

void divoom_database_manage_update_music_like(int id, int like_flag)
{
    int rc;

    if (!divoom_music_db_is_ready())
        return;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    divoom_database_manage_format_sql( "UPDATE local_music SET like_flag=%d WHERE id=%d", like_flag, id);
    rc = sqlite3_exec(pdivoom_database_manage_handle->music_db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("Insert failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->music_db_handle));
    }
}

/*********************************************************************
  Function:        divoom_database_manage_get_music_list
  Description:     获取所有歌曲
  Input:           pcallback
                   para
  Return:          none
**********************************************************************/


void divoom_database_manage_get_music_list(divoom_database_manage_get_music_list_callback *pcallback, void *para)
{
    sqlite3_stmt *stmt = NULL;
    int rc;
    int id;
    int like;
    const unsigned char *name_raw;
    const unsigned char *root_raw;
    char *name_ptr;
    char *root_ptr;

    if (!divoom_music_db_is_ready() || pcallback == NULL)
        return;

    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    rc = sqlite3_prepare_v2(pdivoom_database_manage_handle->music_db_handle,
        "SELECT id, like_flag, name, root_name FROM local_music ORDER BY id",
        -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        divoom_print("get failed: local_music ;%s\n",
            sqlite3_errmsg(pdivoom_database_manage_handle->music_db_handle));
        divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
        return;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        id = sqlite3_column_int(stmt, 0);
        like = sqlite3_column_int(stmt, 1);
        name_raw = sqlite3_column_text(stmt, 2);
        root_raw = sqlite3_column_text(stmt, 3);
        name_ptr = (char *)(name_raw ? name_raw : (const unsigned char *)"");
        root_ptr = (char *)(root_raw ? root_raw : (const unsigned char *)"");
        pcallback(para, id, like, name_ptr, root_ptr);
    }
    sqlite3_finalize(stmt);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
}

int divoom_database_manage_get_local_music_count(void)
{
    return divoom_database_tf_total();
}

void divoom_database_manage_txn_begin(void)
{
    int rc;

    if (!divoom_db_is_ready())
        return;

    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    if (s_db_txn_depth == 0)
    {
        rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle,
            "BEGIN IMMEDIATE", NULL, NULL, NULL);
        if (rc != SQLITE_OK)
        {
            divoom_print("db txn begin failed: %s\n",
                sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
            divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
            return;
        }
    }
    s_db_txn_depth++;
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
}

void divoom_database_manage_txn_end(void)
{
    int rc;

    if (!divoom_db_is_ready())
        return;

    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    if (s_db_txn_depth <= 0)
    {
        divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
        return;
    }
    s_db_txn_depth--;
    if (s_db_txn_depth == 0)
    {
        rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle,
            "COMMIT", NULL, NULL, NULL);
        if (rc != SQLITE_OK)
        {
            divoom_print("db txn commit failed: %s\n",
                sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
            sqlite3_exec(pdivoom_database_manage_handle->db_handle,
                "ROLLBACK", NULL, NULL, NULL);
        }
    }
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
}

void divoom_database_manage_local_music_batch_begin(void)
{
    int rc;

    if (!divoom_music_db_is_ready())
        return;

    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    if (s_music_txn_depth == 0)
    {
        rc = sqlite3_exec(pdivoom_database_manage_handle->music_db_handle,
            "BEGIN IMMEDIATE", NULL, NULL, NULL);
        if (rc != SQLITE_OK)
        {
            divoom_print("music txn begin failed: %s\n",
                sqlite3_errmsg(pdivoom_database_manage_handle->music_db_handle));
            divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
            return;
        }
    }
    s_music_txn_depth++;
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
}

void divoom_database_manage_local_music_batch_end(void)
{
    int rc;

    if (!divoom_music_db_is_ready())
        return;

    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    if (s_music_txn_depth <= 0)
    {
        divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
        return;
    }
    s_music_txn_depth--;
    if (s_music_txn_depth == 0)
    {
        rc = sqlite3_exec(pdivoom_database_manage_handle->music_db_handle,
            "COMMIT", NULL, NULL, NULL);
        if (rc != SQLITE_OK)
        {
            divoom_print("music txn commit failed: %s\n",
                sqlite3_errmsg(pdivoom_database_manage_handle->music_db_handle));
            sqlite3_exec(pdivoom_database_manage_handle->music_db_handle,
                "ROLLBACK", NULL, NULL, NULL);
        }
    }
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
}

int divoom_database_manage_clear_local_music_reset_id(void)
{
    int rc;

    if (!divoom_music_db_is_ready()) {
        return -1;
    }

    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

    divoom_database_manage_format_sql( "DELETE FROM local_music");
    rc = sqlite3_exec(pdivoom_database_manage_handle->music_db_handle, pdivoom_database_manage_handle->sql_str,
                      NULL, NULL, NULL);

    if (rc != SQLITE_OK) {
        divoom_print("delete local_music rows failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str,
                      sqlite3_errmsg(pdivoom_database_manage_handle->music_db_handle));
        divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
        return -1;
    }

    divoom_database_manage_format_sql( "DELETE FROM sqlite_sequence WHERE name='local_music'");
    rc = sqlite3_exec(pdivoom_database_manage_handle->music_db_handle, pdivoom_database_manage_handle->sql_str,
                      NULL, NULL, NULL);

    if (rc != SQLITE_OK) {
        divoom_print("reset sqlite_sequence failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str,
                      sqlite3_errmsg(pdivoom_database_manage_handle->music_db_handle));
        divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
        return -1;
    }

    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    return 0;
}

int divoom_database_manage_add_new_table(char *table_name, char *table_item)
{
    // 创建表
    int rc;
    //"CREATE TABLE IF NOT EXISTS employees(id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, position TEXT)"

    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    divoom_database_manage_format_sql( "CREATE TABLE IF NOT EXISTS %s(%s)", table_name, table_item);
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("create error: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return 0;
    } else {
        divoom_print("Table created successfully\n");
    }
}

int divoom_database_manage_insert_new_item(char *table_name, char *item_data)
{
    int rc;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    divoom_database_manage_format_sql( "INSERT INTO %s VALUES(%s)", table_name, item_data);
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
        divoom_print("Insert failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return 0;
    } else {
        int record_id = sqlite3_last_insert_rowid(pdivoom_database_manage_handle->db_handle);
        divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
        return  record_id;
    }
}

void divoom_database_manage_update_record(char *table_name, char *item_data)
{
    int rc;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    //const char *sql_update = "UPDATE users SET email = 'new_email@example.com' WHERE id = 1";
    divoom_database_manage_format_sql( "UPDATE %s SET %s", table_name, item_data);
    
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        divoom_print("update failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str , sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
    } 
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    
}

int divoom_database_manage_get_record(char *table_name, char *item_data,int id, DIVOOM_DATABASE_MANAGE_GET_DATA_CALLBACK *pcallback, void *para)
{
    int i, j, rc, pnRow, pnColumn, index;
    char **dbResult;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    //const char *sql_query = "SELECT * FROM employees WHERE id=1";
    divoom_database_manage_format_sql( "SELECT * FROM  %s WHERE id=%d", table_name, id);

    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return 0;
    }
    

    index = pnColumn;
    for(i = 0; i < pnRow; i++)
    {
        for(j = 0; j < pnColumn; j++)
        {
            pcallback(para, i, j, dbResult[index]);
            index ++;
        }
    }
    sqlite3_free_table(dbResult);
    return 1;
}

int divoom_database_manage_get_all_record(char *cmd_data, DIVOOM_DATABASE_MANAGE_GET_DATA_CALLBACK *pcallback, void *para)
{
    int i, j, rc, pnRow, pnColumn, index;
    char **dbResult;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, cmd_data, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("get failed: %s ;%s\n", cmd_data, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return 0;
    } else {
        divoom_print("Record inserted successfully\n");
    }

    index = pnColumn;
    for(i = 0; i < pnRow; i++)
    {
        for(j = 0; j < pnColumn; j++)
        {
            pcallback(para, i, j, dbResult[index]);
            index ++;
        }
    }
    sqlite3_free_table(dbResult);
    return 1;
}

void divoom_database_manage_remove_record_with_key(char *table_name, char *cmd_info)
{
    int rc;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    //const char *sql_delete = "DELETE FROM employees WHERE id=1";
    divoom_database_manage_format_sql( "DELETE FROM %s WHERE %s", table_name, cmd_info);
    
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        divoom_print("Insert failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
    } 
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    
}

void divoom_database_manage_remove_record(char *table_name, int id)
{
    int rc;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    //const char *sql_delete = "DELETE FROM employees WHERE id=1";
    divoom_database_manage_format_sql( "DELETE FROM %s WHERE id=%d", table_name, id);
    
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        divoom_print("delete failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
    }
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    
}

void divoom_database_manage_remove_table(char *table_name)
{
    int rc;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    //const char *sql_delete = "DROP TABLE IF EXISTS employees";
    divoom_database_manage_format_sql( "DROP TABLE IF EXISTS %s", table_name);
    
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        divoom_print("remove failed: %s: %s\n", pdivoom_database_manage_handle->sql_str,sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
    } 
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    
}

static int divoom_database_manage_close_handle(sqlite3 **db_handle,
    int *txn_depth, const char *db_label)
{
    sqlite3_stmt *stmt;
    sqlite3_stmt *next_stmt;
    int rc;

    if (!db_handle || !*db_handle) {
        if (txn_depth)
            *txn_depth = 0;
        return 1;
    }

    if (txn_depth && *txn_depth > 0) {
        rc = sqlite3_exec(*db_handle, "ROLLBACK", NULL, NULL, NULL);
        if (rc == SQLITE_OK)
            *txn_depth = 0;
        else
            divoom_print("[DB] %s rollback failed before close: rc=%d, err=%s\n",
                db_label, rc, sqlite3_errmsg(*db_handle));
    }

    rc = sqlite3_db_cacheflush(*db_handle);
    if (rc != SQLITE_OK)
        divoom_print("[DB] %s cache flush failed: rc=%d, err=%s\n",
            db_label, rc, sqlite3_errmsg(*db_handle));

    rc = sqlite3_close(*db_handle);
    if (rc == SQLITE_BUSY) {
        divoom_print("[DB] %s has unfinished statements; finalizing them\n",
            db_label);
        stmt = sqlite3_next_stmt(*db_handle, NULL);
        while (stmt) {
            next_stmt = sqlite3_next_stmt(*db_handle, stmt);
            sqlite3_finalize(stmt);
            stmt = next_stmt;
        }
        rc = sqlite3_close(*db_handle);
    }
    if (rc != SQLITE_OK) {
        divoom_print("[DB] %s close failed: rc=%d, err=%s\n",
            db_label, rc, sqlite3_errmsg(*db_handle));
        return 0;
    }

    *db_handle = NULL;
    if (txn_depth)
        *txn_depth = 0;
    divoom_print("[DB] %s closed\n", db_label);
    return 1;
}

void divoom_database_manage_close(char *db_name)
{
    Pdivoom_database_manage_ctrl handle;

    (void)db_name;
    handle = pdivoom_database_manage_handle;
    if (!handle || !handle->lock_handle)
        return;

    divoom_system_get_lock(handle->lock_handle);
    divoom_tf_db_close_locked();
    divoom_database_manage_close_handle(&handle->music_db_handle,
        &s_music_txn_depth, "tf_music database");
    divoom_database_manage_close_handle(&handle->db_handle,
        &s_db_txn_depth, "main database");
    divoom_system_free_lock(handle->lock_handle);
    divoom_port_sync();
}

/*********************************************************************
  Function:        divoom_database_manage_get_user_head_list
  Description:     得到最近访问的用户头像的列表
  Input:           user_num  返回得到用户的数目
  Return:          用户列表
**********************************************************************/

uint32 *divoom_database_manage_get_user_head_list(int *user_num)
{
    int i, j, rc, pnRow, pnColumn, index;
    char **dbResult;
    uint32 *user_list = NULL;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

    divoom_database_manage_format_sql( "SELECT user_id FROM %s ORDER BY update_time DESC limit %d", DIVOOM_USER_HEAD_IMAGE_TABLE_NAME, *user_num);

    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {        
        *user_num = 0;
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return NULL;
    } 
    
    if(pnRow > 0)
    {
        user_list = divoom_system_malloc(pnRow * sizeof(uint32));
        index = pnColumn;
        *user_num = pnRow;
        for(i = 0; i < pnRow; i++)
        {
            for(j = 0; j < pnColumn; j++)
            {
                user_list[i] = atoi(dbResult[index]);
                index ++;
            }
        }
    }
    sqlite3_free_table(dbResult);
    return user_list;
}

/*********************************************************************
  Function:        divoom_database_manage_get_user_head_item
  Description:     得到用户头像的信息
  Input:           user_id
                   user_name
                   file_id
  Return:          用户列表
**********************************************************************/

int divoom_database_manage_get_user_head_item(int user_id, char *user_name, char *file_id)
{
    int i, j, rc, pnRow, pnColumn, index;
    char **dbResult;
    
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

    divoom_database_manage_format_sql( "SELECT user_name,file_id FROM %s WHERE user_id=%d", DIVOOM_USER_HEAD_IMAGE_TABLE_NAME, user_id);

    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {        
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return 0;
    } 
    
    if(pnRow > 0)
    {
        index = pnColumn;
        strcpy(user_name ,dbResult[index]);
        index ++;
        strcpy(file_id ,dbResult[index]);
    }
    sqlite3_free_table(dbResult);
    return pnRow > 0;
}

/*********************************************************************
  Function:        divoom_database_manage_insert_user_head_item
  Description:     向数据库中插入用户信息
  Input:           user_id
                   user_name
                   file_id
  Return:          none
**********************************************************************/

void divoom_database_manage_insert_user_head_item(int user_id, char *user_name, char *file_id)
{
    int rc;    
    char new_user_name[200], new_file_id[200];

    divoom_app_com_add_escape_quote_safe(user_name, new_user_name, sizeof(new_user_name));
    divoom_app_com_add_escape_quote_safe(file_id, new_file_id, sizeof(new_file_id));
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    divoom_database_manage_format_sql( "INSERT INTO %s VALUES(%d,'%s','%s',%d)", DIVOOM_USER_HEAD_IMAGE_TABLE_NAME, user_id, new_user_name, new_file_id, (int)time(0));
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        divoom_print("Insert failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
    } 
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    
}


/*********************************************************************
  Function:        divoom_database_manage_update_user_head_item
  Description:     更新用户的相关信息
  Input:           user_id
                   user_name
                   file_id
  Return:          none
**********************************************************************/

void divoom_database_manage_update_user_head_item(int user_id, char *user_name, char *file_id)
{
    int rc;
    char new_user_name[256], new_file_id[256];

    divoom_app_com_add_escape_quote_safe(user_name, new_user_name, sizeof(new_user_name));
    divoom_app_com_add_escape_quote_safe(file_id, new_file_id, sizeof(new_file_id));
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    divoom_database_manage_format_sql( "UPDATE %s  SET user_name='%s', file_id='%s',update_time=%d WHERE user_id=%d", DIVOOM_USER_HEAD_IMAGE_TABLE_NAME, 
        new_user_name, new_file_id, (int)time(0),user_id);
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        divoom_print("Insert failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
    } 
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    
}

/*********************************************************************
  Function:        divoom_database_manage_update_user_head_time
  Description:     更新用户头像使用时间
  Input:           user_id
  Return:          none
**********************************************************************/

void divoom_database_manage_update_user_head_time(int user_id)
{
    int rc;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    divoom_database_manage_format_sql( "UPDATE %s  SET update_time=%d WHERE user_id=%d", DIVOOM_USER_HEAD_IMAGE_TABLE_NAME, (int)time(0),user_id);
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        divoom_print("Insert failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
    } 
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    
}

/***********************************************************
相簿数据库表:
album_head:
id, type, name

相簿下面相片数据表，要以两个关键字做为主键
表名：album_+album_id
album_id, pic_id,
************************************************************/
/*********************************************************************
  Function:        divoom_database_manage_insert_new_item
  Description:     向数据库中插入一个相簿
  Input:           id
                   type
                   album_name
  Return:          返回句柄
**********************************************************************/

void divoom_database_manage_insert_album_item(int  id, int type, char *album_name)
{
    int rc;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    divoom_database_manage_format_sql( "INSERT INTO album_head VALUES(%d,%d,'%s')", id, type, album_name);
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        divoom_print("Insert failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
    } 
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    
}
/*********************************************************************
  Function:        divoom_database_manage_update_album_record
  Description:     更新相簿的名称相簿
  Input:           id
                   album_name
  Return:          none
**********************************************************************/

void divoom_database_manage_update_album_name_record(int  id, char *album_name)
{
    int rc;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    //const char *sql_update = "UPDATE users SET email = 'new_email@example.com' WHERE id = 1";
    divoom_database_manage_format_sql( "UPDATE album_head SET name='%s' WHERE id=%d", album_name, id);
    
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        divoom_print("update failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str , sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
    }
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    
}

/*********************************************************************
  Function:        divoom_database_manage_delete_album_record
  Description:     删除相簿
  Input:           id
  Return:          none
**********************************************************************/

void divoom_database_manage_delete_album_table(int  id)
{
    //向相簿头里面删除一条记录，并相簿表中包含这个记录的所有元素
    char  del_key_str[64];
    
    divoom_database_manage_remove_record(DIVOOM_ALBUM_HEAD_TABLE_NAME, id);    
    sprintf(del_key_str, "album_id=%d", id);
    divoom_database_manage_remove_record_with_key(DIVOOM_ALBUM_DATA_TABLE_NAME, del_key_str);
}

/*********************************************************************
  Function:        divoom_database_manage_remove_album_record
  Description:     将相片从相簿中移除
  Input:           album_id
                   pic_id
  Return:          none
**********************************************************************/

void divoom_database_manage_remove_album_record(int  album_id, int pic_id)
{
    //向相簿头里面删除一条记录，并删除对应相簿表
    char  del_key_str[128];
    
    sprintf(del_key_str, "album_id=%d AND pic_id=%d", album_id, pic_id);
    divoom_database_manage_remove_record_with_key(DIVOOM_ALBUM_DATA_TABLE_NAME, del_key_str);
}

/*********************************************************************
  Function:        divoom_database_manage_remove_album_record
  Description:     将相片从相簿中移除
  Input:           album_id
                   pic_id
  Return:          none
**********************************************************************/

/* The caller must only publish preload completion after this transaction
 * commits. Keep the DB lock throughout so unrelated writes cannot join it. */
int divoom_database_manage_add_album_records_batch(int album_id,
    const uint32 *pic_list, int pic_num)
{
    sqlite3_stmt *stmt = NULL;
    sqlite3 *db;
    int i, rc, began = 0;

    if (pic_num < 0 || (pic_num > 0 && !pic_list))
        return 0;
    if (pic_num == 0)
        return 1;
    if (!divoom_db_is_ready())
        return 0;

    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    db = pdivoom_database_manage_handle->db_handle;
    /* Do not commit or roll back a transaction opened by a legacy caller. */
    if (!db || !sqlite3_get_autocommit(db))
    {
        divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
        divoom_print("[DB] preload batch unavailable: database closed or transaction active\n");
        return 0;
    }
    rc = sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, NULL);
    if (rc != SQLITE_OK)
        goto finish;
    began = 1;
    rc = sqlite3_prepare_v2(db,
        "INSERT INTO " DIVOOM_ALBUM_DATA_TABLE_NAME "(album_id,pic_id) "
        "SELECT ?1,?2 WHERE NOT EXISTS (SELECT 1 FROM "
        DIVOOM_ALBUM_DATA_TABLE_NAME " WHERE album_id=?1 AND pic_id=?2)",
        -1, &stmt, NULL);
    if (rc != SQLITE_OK)
        goto finish;
    rc = sqlite3_bind_int(stmt, 1, album_id);
    if (rc != SQLITE_OK)
        goto finish;
    for (i = 0; i < pic_num; i++)
    {
        rc = sqlite3_bind_int(stmt, 2, (int)pic_list[i]);
        if (rc != SQLITE_OK)
            goto finish;
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE)
            goto finish;
        rc = sqlite3_reset(stmt);
        if (rc != SQLITE_OK)
            goto finish;
    }
    rc = sqlite3_finalize(stmt);
    stmt = NULL;
    if (rc == SQLITE_OK)
        rc = sqlite3_exec(db, "COMMIT", NULL, NULL, NULL);
finish:
    sqlite3_finalize(stmt);
    if (rc != SQLITE_OK)
    {
        divoom_print("[DB] preload batch failed album=%d rc=%d err=%s\n",
            album_id, rc, sqlite3_errmsg(db));
        if (began && !sqlite3_get_autocommit(db))
        {
            int rollback_rc = sqlite3_exec(db, "ROLLBACK", NULL, NULL, NULL);
            if (rollback_rc != SQLITE_OK)
                divoom_print("[DB] preload batch rollback failed rc=%d\n", rollback_rc);
        }
    }
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    return rc == SQLITE_OK;
}

int divoom_database_manage_add_album_record(int  album_id, int pic_id)
{
    //向相簿头里面删除一条记录，并删除对应相簿表
    char  add_key_str[128], table_name[128];

    if(divoom_database_manage_check_album_include_pic(album_id, pic_id))
    {
        return 0;
    }
    sprintf(add_key_str, "%d,%d", album_id, pic_id);
    sprintf(table_name, "%s(album_id, pic_id)",DIVOOM_ALBUM_DATA_TABLE_NAME);

    divoom_database_manage_insert_new_item(table_name, add_key_str);
    return 1;
}

/*********************************************************************
  Function:        divoom_database_manage_get_pic_num_from_album_table
  Description:     从相簿中得到所有照片张数
  Input:           album_id
                   hide_flag    1:只取不隐藏的相片；0：所有照片
  Return:          相片张数
**********************************************************************/

int divoom_database_manage_get_pic_num_from_album_table(int  album_id, int hide_flag)
{
    int rc, pnRow, pnColumn, pic_num;
    char **dbResult;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

    if(hide_flag)
    {
        if(album_id)
        {
            divoom_database_manage_format_sql( "SELECT count(*) FROM album_pic WHERE album_id=%d AND hide_flag=0", album_id);
        }
        else
        {
            strcpy(pdivoom_database_manage_handle->sql_str, "SELECT count(*) FROM pic_info WHERE hide_flag=0 ");
        }
    }
    else
    {
        if(album_id)
        {
            divoom_database_manage_format_sql( "SELECT count(*) FROM album_pic WHERE album_id=%d", album_id);
        }
        else
        {
            strcpy(pdivoom_database_manage_handle->sql_str, "SELECT count(*) FROM pic_info ");
        }
    }
    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {        
        pic_num = 0;
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return 0;
    } 
    
    pic_num  = atoi(dbResult[1]);
    sqlite3_free_table(dbResult);
    return pic_num;
}

/*********************************************************************
  Function:        divoom_database_manage_get_all_pic_from_album_table
  Description:     从相簿中得到所有照片，并按相关排序
  Input:           album_id
                   pic_num   相片数目
                   order_flag   1:按拍摄时间， 按照片时间
                   desc_flag    1:升序与降序
                   like_flag    1: 只取收藏的相片列表
                   hide_flag    1:只取隐藏的相片列表
  Return:          none
**********************************************************************/

uint32 *divoom_database_manage_get_all_pic_from_album_table(int  album_id, int *pic_num, int order_flag, int desc_flag, int like_flag, int hide_flag)
{
    int i, j, rc, pnRow, pnColumn, index;
    uint32 *pic_list;
    char **dbResult;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

    if(album_id == 0 || album_id == divoom_pic_album_manage_get_album_id_from_type(DIVOOM_PIC_ALBUM_TYPE_ALL))
    {
        strcpy(pdivoom_database_manage_handle->sql_str, "SELECT id FROM pic_info ORDER BY save_time DESC");
    }
    else if(hide_flag)
    {
        strcpy(pdivoom_database_manage_handle->sql_str, "SELECT id FROM pic_info WHERE hide_flag=1 ORDER BY hide_time DESC");
    }
    else if(like_flag)
    {
        strcpy(pdivoom_database_manage_handle->sql_str, "SELECT id FROM pic_info WHERE like_flag=1 ORDER BY like_time DESC");
    }
    else
    {
        divoom_database_manage_format_sql( "SELECT pic_info.id FROM pic_info LEFT JOIN album_pic ON album_pic.pic_id = pic_info.id WHERE album_pic.album_id=%d AND hide_flag=0  ORDER BY %s %s", album_id, order_flag?"pic_info.save_time":"pic_info.photo_time", desc_flag==0?"DESC":"");
        divoom_print("sql info: %s\n", pdivoom_database_manage_handle->sql_str);
    }
    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    if (rc != SQLITE_OK) {        
        *pic_num = 0;
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
        return NULL;
    }
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    
    pic_list = divoom_system_malloc(sizeof(uint32)*pnRow);
    index = pnColumn;
    *pic_num = pnRow;
    for(i = 0; i < pnRow; i++)
    {
        for(j = 0; j < pnColumn; j++)
        {
            pic_list[i] = atoi(dbResult[index]);
            index ++;
        }
    }
    sqlite3_free_table(dbResult);
    return pic_list;
}


/*********************************************************************
  Function:        divoom_database_manage_get_all_pic_from_album_table
  Description:     从相簿中得到所有照片，并按相关排序
  Input:           album_id
                   pic_num   相片数目
                   order_flag   1:按拍摄时间， 按照片时间
                   desc_flag    1:升序与降序
                   like_flag    1: 只取收藏的相片列表
                   hide_flag    1:只取隐藏的相片列表
  Return:          none
**********************************************************************/


/* LAN 只读快照直接查询数据库，不触发云端相簿更新；调用方负责释放 JSON。 */
divoom_cJSON *divoom_database_manage_get_lan_album_list(void)
{
    divoom_cJSON *list = NULL;
    divoom_cJSON *item;
    sqlite3_stmt *stmt = NULL;
    int rc;
    if(pdivoom_database_manage_handle == NULL || pdivoom_database_manage_handle->db_handle == NULL)
    {
        return NULL;
    }
    list = divoom_cJSON_CreateArray();
    if(list == NULL)
    {
        return NULL;
    }
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    rc = sqlite3_prepare_v2(pdivoom_database_manage_handle->db_handle,
        "SELECT id,type,name FROM album_head ORDER BY CASE WHEN type=1 THEN 0 ELSE 1 END,id DESC", -1, &stmt, NULL);
    if(rc == SQLITE_OK)
    {
        while((rc = sqlite3_step(stmt)) == SQLITE_ROW)
        {
            const char *name = (const char *)sqlite3_column_text(stmt, 2);
            item = divoom_cJSON_CreateObject();
            if(item == NULL)
            {
                rc = SQLITE_NOMEM;
                break;
            }
            divoom_cJSON_AddNumberToObject(item, "ClockId", sqlite3_column_int(stmt, 0));
            divoom_cJSON_AddNumberToObject(item, "AlbumType", sqlite3_column_int(stmt, 1));
            divoom_cJSON_AddStringToObject(item, "ClockName", name ? name : "");
            divoom_cJSON_AddItemToArray(list, item);
        }
    }
    sqlite3_finalize(stmt);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if(rc != SQLITE_DONE)
    {
        divoom_cJSON_Delete(list);
        return NULL;
    }
    return list;
}

/* HTTP 分页按位置读取，以 id 消除同时间记录歧义；不共享旧 App 的时间游标。 */
divoom_cJSON *divoom_database_manage_get_lan_photo_list(int album_id, int start, int end)
{
    divoom_cJSON *list = NULL;
    divoom_cJSON *item;
    sqlite3_stmt *stmt = NULL;
    int rc;
    int album_type = -1;
    char sql[768];
    char thumb[512];
    divoom_port_stat_t file_st;
    const char *filter;
    if(pdivoom_database_manage_handle == NULL || pdivoom_database_manage_handle->db_handle == NULL || start < 1 || end < start || end - start >= 100)
    {
        return NULL;
    }
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    rc = sqlite3_prepare_v2(pdivoom_database_manage_handle->db_handle,
        "SELECT type FROM album_head WHERE id=?", -1, &stmt, NULL);
    if(rc == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, album_id);
        if(sqlite3_step(stmt) == SQLITE_ROW)
        {
            album_type = sqlite3_column_int(stmt, 0);
        }
    }
    sqlite3_finalize(stmt);
    stmt = NULL;
    if(album_type < 0)
    {
        divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
        return NULL;
    }
    filter = album_type == DIVOOM_PIC_ALBUM_TYPE_ALL ? "1" :
        album_type == DIVOOM_PIC_ALBUM_TYPE_LIKE ? "p.like_flag=1" :
        album_type == DIVOOM_PIC_ALBUM_TYPE_HIDE ? "p.hide_flag=1" :
        "EXISTS(SELECT 1 FROM album_pic a WHERE a.pic_id=p.id AND a.album_id=?)";
    snprintf(sql, sizeof(sql), "SELECT p.id,p.save_time,p.path_name,p.pic_video_flag FROM pic_info p WHERE %s ORDER BY p.save_time DESC,p.id DESC LIMIT %d OFFSET %d", filter, end - start + 1, start - 1);
    rc = sqlite3_prepare_v2(pdivoom_database_manage_handle->db_handle, sql, -1, &stmt, NULL);
    list = divoom_cJSON_CreateArray();
    if(rc == SQLITE_OK && list != NULL)
    {
        if(album_type != DIVOOM_PIC_ALBUM_TYPE_ALL && album_type != DIVOOM_PIC_ALBUM_TYPE_LIKE && album_type != DIVOOM_PIC_ALBUM_TYPE_HIDE)
        {
            sqlite3_bind_int(stmt, 1, album_id);
        }
        while((rc = sqlite3_step(stmt)) == SQLITE_ROW)
        {
            const char *name = (const char *)sqlite3_column_text(stmt, 2);
            if(name == NULL || strncmp(name, DIVOOM_SD_APP_PIC_DIR "/", strlen(DIVOOM_SD_APP_PIC_DIR) + 1) != 0)
            {
                continue;
            }
            item = divoom_cJSON_CreateObject();
            if(item == NULL)
            {
                rc = SQLITE_NOMEM;
                break;
            }
            divoom_thumb_cache_get_thumb_name((char *)name, thumb);
            if(divoom_port_stat(thumb, &file_st) != 0 || file_st.size == 0)
            {
                /* 视频封面与 MP4 同目录同名，扩展名为 webp。 */
                const char *extension = strrchr(name, '.');
                if(sqlite3_column_int(stmt, 3) && extension != NULL)
                {
                    snprintf(thumb, sizeof(thumb), "%.*s.webp", (int)(extension - name), name);
                }
                else
                {
                    snprintf(thumb, sizeof(thumb), "%s", name);
                }
            }
            divoom_cJSON_AddNumberToObject(item, "PhotoId", sqlite3_column_int(stmt, 0));
            divoom_cJSON_AddNumberToObject(item, "Time", sqlite3_column_double(stmt, 1) / 1000.0);
            divoom_cJSON_AddStringToObject(item, "FileId", thumb);
            divoom_cJSON_AddStringToObject(item, "OriginalFileId", name);
            divoom_cJSON_AddStringToObject(item, "MediaType", sqlite3_column_int(stmt, 3) ? "video" : "photo");
            divoom_cJSON_AddItemToArray(list, item);
        }
    }
    sqlite3_finalize(stmt);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if(rc != SQLITE_DONE || list == NULL)
    {
        divoom_cJSON_Delete(list);
        return NULL;
    }
    return list;
}

Pdivoom_pic_id_name_info divoom_database_manage_get_all_pic_id_and_name(int  album_id, int *pic_num, int num, int order_flag, int desc_flag, double start_time, int start_id)
{
    int i, j, rc, pnRow, pnColumn, index, all_album_id = divoom_pic_album_manage_get_album_id_from_type(DIVOOM_PIC_ALBUM_TYPE_ALL);
    Pdivoom_pic_id_name_info pic_info;
    char **dbResult;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

    if(album_id == 0 || album_id == all_album_id)
    {
        if(order_flag == DIVOOM_PIC_PHOTO_SLIDE_DISP_PHTOT_TIME)
        {
            if(desc_flag==0)
            {
                if(start_time < 0)
                {
                    divoom_database_manage_format_sql( "SELECT id,photo_time,path_name FROM pic_info  ORDER BY photo_time DESC,id DESC LIMIT %d",num);
                }
                else
                {
                    divoom_database_manage_format_sql( "SELECT id,photo_time,path_name FROM pic_info WHERE (photo_time<%d OR (photo_time=%d AND id<%d)) ORDER BY photo_time DESC,id DESC LIMIT %d", (int)start_time, (int)start_time, start_id, num);
                }
            }
            else
            {
                if(start_time < 0)
                {
                    divoom_database_manage_format_sql( "SELECT id,photo_time,path_name FROM pic_info ORDER BY photo_time ASC,id ASC LIMIT %d", num);
                }
                else
                {
                    divoom_database_manage_format_sql( "SELECT id,photo_time,path_name FROM pic_info WHERE (photo_time>%d OR (photo_time=%d AND id>%d)) ORDER BY photo_time ASC,id ASC LIMIT %d", (int)start_time, (int)start_time, start_id, num);
                }
            }
        }
        else
        {
            if(desc_flag==0)
            {
                if(start_time < 0)
                {
                    divoom_database_manage_format_sql( "SELECT id,save_time,path_name FROM pic_info ORDER BY save_time DESC,id DESC LIMIT %d", num);
                }
                else
                {
                    divoom_database_manage_format_sql( "SELECT id,save_time,path_name FROM pic_info WHERE (save_time<%lld OR (save_time=%lld AND id<%d)) ORDER BY save_time DESC,id DESC LIMIT %d", (long long)start_time, (long long)start_time, start_id, num);
                }
            }
            else
            {
                if(start_time < 0)
                {
                    divoom_database_manage_format_sql( "SELECT id,save_time,path_name FROM pic_info ORDER BY save_time ASC,id ASC LIMIT %d", num);
                }
                else
                {
                    divoom_database_manage_format_sql( "SELECT id,save_time,path_name FROM pic_info WHERE (save_time>%lld OR (save_time=%lld AND id>%d)) ORDER BY save_time ASC,id ASC LIMIT %d", (long long)start_time, (long long)start_time, start_id, num);
                }
            }
        }
    }
    else
    {

        if(order_flag == DIVOOM_PIC_PHOTO_SLIDE_DISP_PHTOT_TIME)
        {
            if(desc_flag==0)
            {
                if(start_time < 0)
                {
                    divoom_database_manage_format_sql( "SELECT pic_info.id,pic_info.photo_time,pic_info.path_name FROM pic_info LEFT JOIN album_pic ON album_pic.pic_id = pic_info.id WHERE album_pic.album_id=%d ORDER BY photo_time DESC,pic_info.id DESC LIMIT %d", album_id, num);
                }
                else
                {
                    divoom_database_manage_format_sql( "SELECT pic_info.id,pic_info.photo_time,pic_info.path_name FROM pic_info LEFT JOIN album_pic ON album_pic.pic_id = pic_info.id WHERE album_pic.album_id=%d AND (photo_time<%d OR (photo_time=%d AND pic_info.id<%d)) ORDER BY photo_time DESC,pic_info.id DESC LIMIT %d", album_id, (int)start_time, (int)start_time, start_id, num);
                }
            }
            else
            {
                if(start_time < 0)
                {
                    divoom_database_manage_format_sql( "SELECT pic_info.id,pic_info.photo_time,pic_info.path_name FROM pic_info LEFT JOIN album_pic ON album_pic.pic_id = pic_info.id WHERE album_pic.album_id=%d ORDER BY photo_time ASC,pic_info.id ASC LIMIT %d", album_id, num);
                }
                else
                {
                    divoom_database_manage_format_sql( "SELECT pic_info.id,pic_info.photo_time,pic_info.path_name FROM pic_info LEFT JOIN album_pic ON album_pic.pic_id = pic_info.id WHERE album_pic.album_id=%d  AND (photo_time>%d OR (photo_time=%d AND pic_info.id>%d)) ORDER BY photo_time ASC,pic_info.id ASC LIMIT %d", album_id, (int)start_time, (int)start_time, start_id, num);
                }
            }
        }
        else
        {
            if(desc_flag==0)
            {
                if(start_time < 0)
                {
                    divoom_database_manage_format_sql( "SELECT pic_info.id,pic_info.save_time,pic_info.path_name FROM pic_info LEFT JOIN album_pic ON album_pic.pic_id = pic_info.id WHERE album_pic.album_id=%d ORDER BY save_time DESC,pic_info.id DESC LIMIT %d", album_id, num);
                }
                else
                {
                    divoom_database_manage_format_sql( "SELECT pic_info.id,pic_info.save_time,pic_info.path_name FROM pic_info LEFT JOIN album_pic ON album_pic.pic_id = pic_info.id WHERE album_pic.album_id=%d AND (save_time<%lld OR (save_time=%lld AND pic_info.id<%d)) ORDER BY save_time DESC,pic_info.id DESC LIMIT %d", album_id, (long long)start_time, (long long)start_time, start_id, num);
                }
            }
            else
            {
                if(start_time < 0)
                {
                    divoom_database_manage_format_sql( "SELECT pic_info.id,pic_info.save_time,pic_info.path_name FROM pic_info LEFT JOIN album_pic ON album_pic.pic_id = pic_info.id WHERE album_pic.album_id=%d ORDER BY save_time ASC,pic_info.id ASC LIMIT %d", album_id, num);
                }
                else
                {
                    divoom_database_manage_format_sql( "SELECT pic_info.id,pic_info.save_time,pic_info.path_name FROM pic_info LEFT JOIN album_pic ON album_pic.pic_id = pic_info.id WHERE album_pic.album_id=%d AND (save_time>%lld OR (save_time=%lld AND pic_info.id>%d)) ORDER BY save_time ASC,pic_info.id ASC LIMIT %d", album_id, (long long)start_time, (long long)start_time, start_id, num);
                }
            }
        }
    }
    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    if (rc != SQLITE_OK) {        
        *pic_num = 0;
        divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return NULL;
    } 
    pic_info = divoom_system_malloc(sizeof(divoom_pic_id_name_info)*pnRow);
    if(pic_info == NULL)
    {
        *pic_num = 0;
        sqlite3_free_table(dbResult);
        divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
        return NULL;
    }
    index = pnColumn;
    *pic_num = pnRow;
    for(i = 0; i < pnRow; i++)
    {
        pic_info[i].id = atoi(dbResult[index]);
        index ++;
        pic_info[i].time = atof(dbResult[index]);
        index ++;
        if(dbResult[index])
        {
            strncpy(pic_info[i].name, dbResult[index], sizeof(pic_info[i].name) - 1);
            pic_info[i].name[sizeof(pic_info[i].name) - 1] = 0;
        }
        else
        {
            pic_info[i].name[0] = 0;
        }
        index ++;
    }
    sqlite3_free_table(dbResult);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    return pic_info;
}

/*********************************************************************
  Function:        divoom_database_manage_get_total_pic_num
  Description:     照片总数
  Input:           album_id
                   pic_flag  0:全部照片，1：宽的，2：长的
                   
  Return:          照片总数
**********************************************************************/

/* Display positions use the same time/id order in both directions. */
static int divoom_database_manage_query_disp_order(int album_id, int order_flag,
    int reverse_flag, int movie_flag, int pos, int pic_id)
{
    char filter[256];
    const char *time_column = order_flag == DIVOOM_PIC_PHOTO_SLIDE_DISP_PHTOT_TIME ? "photo_time" : "save_time";
    const char *direction = reverse_flag ? "ASC" : "DESC";
    const char *compare = reverse_flag ? "<" : ">";
    sqlite3_stmt *stmt = NULL;
    int rc, result = -1;

    if(album_id == divoom_pic_album_manage_get_album_id_from_type(DIVOOM_PIC_ALBUM_TYPE_ALL))
    {
        album_id = 0;
    }
    snprintf(filter, sizeof(filter),
        "(%d=0 OR EXISTS (SELECT 1 FROM album_pic a WHERE a.album_id=%d AND a.pic_id=p.id)) %s",
        album_id, album_id, movie_flag ? "" : "AND p.pic_video_flag=0");
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    if(pic_id > 0)
    {
        divoom_database_manage_format_sql(
            "SELECT COUNT(*) FROM pic_info p, pic_info t WHERE t.id=%d AND %s "
            "AND (p.%s%s t.%s OR (p.%s=t.%s AND p.id%s t.id)) "
            "HAVING EXISTS (SELECT 1 FROM pic_info p WHERE p.id=%d AND %s)",
            pic_id, filter, time_column, compare, time_column, time_column, time_column,
            compare, pic_id, filter);
    }
    else
    {
        divoom_database_manage_format_sql(
            "SELECT p.id FROM pic_info p WHERE %s ORDER BY p.%s %s, p.id %s LIMIT 1 OFFSET %d",
            filter, time_column, direction, direction, pos);
    }
    rc = sqlite3_prepare_v2(pdivoom_database_manage_handle->db_handle,
        pdivoom_database_manage_handle->sql_str, -1, &stmt, NULL);
    if(rc == SQLITE_OK)
    {
        rc = sqlite3_step(stmt);
        if(rc == SQLITE_ROW)
        {
            result = sqlite3_column_int(stmt, 0);
        }
    }
    if(rc != SQLITE_ROW && rc != SQLITE_DONE)
    {
        divoom_print("get display order failed: %s\n", sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
    }
    sqlite3_finalize(stmt);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    return result;
}

int divoom_database_manage_get_disp_order_pic_id(int album_id, int order_flag,
    int reverse_flag, int movie_flag, int pos)
{
    if(pos < 0)
    {
        return 0;
    }
    int id = divoom_database_manage_query_disp_order(album_id, order_flag, reverse_flag, movie_flag, pos, 0);
    return id > 0 ? id : 0;
}

int divoom_database_manage_get_disp_order_pic_pos(int album_id, int order_flag,
    int reverse_flag, int movie_flag, int pic_id)
{
    if(pic_id <= 0)
    {
        return -1;
    }
    return divoom_database_manage_query_disp_order(album_id, order_flag, reverse_flag, movie_flag, 0, pic_id);
}

int divoom_database_manage_get_total_pic_num(int album_id, int order_flag, int reverse_flag, double save_time, int pic_flag, int movie_flag)
{
    int i, j, rc, pnRow, pnColumn, index, pic_id;
    
    char **dbResult;
    
    if(album_id == divoom_pic_album_manage_get_album_id_from_type(DIVOOM_PIC_ALBUM_TYPE_ALL))
    {
        album_id = 0;
    }
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

    if(save_time > 0)
    {
        if(album_id == 0)
        {
            if(pic_flag == 0)
            {
                divoom_database_manage_format_sql( "SELECT COUNT(*)  FROM pic_info WHERE %s%s%lld %s", order_flag == DIVOOM_PIC_PHOTO_SLIDE_DISP_PHTOT_TIME?"photo_time":"save_time",reverse_flag==0?">":"<",(long long)save_time, movie_flag?"": " AND pic_video_flag=0 ");
            }
            else
            {
                divoom_database_manage_format_sql( "SELECT COUNT(*) FROM pic_info WHERE %s%s%lld AND width_flag=%d %s", order_flag == DIVOOM_PIC_PHOTO_SLIDE_DISP_PHTOT_TIME?"photo_time":"save_time", reverse_flag==0?">":"<", (long long)save_time,pic_flag==1?1:0 , movie_flag?"": " AND pic_video_flag=0 ");
            }
        }
        else
        {
            if(pic_flag == 0)
            {
                divoom_database_manage_format_sql( "SELECT COUNT(*) FROM pic_info LEFT JOIN album_pic ON album_pic.pic_id = pic_info.id WHERE album_pic.album_id=%d AND %s%s%lld %s", album_id, order_flag == DIVOOM_PIC_PHOTO_SLIDE_DISP_PHTOT_TIME?"photo_time":"save_time", reverse_flag==0?">":"<", (long long)save_time, movie_flag?"": " AND pic_video_flag=0 ");
            }
            else
            {
                divoom_database_manage_format_sql( "SELECT COUNT(*) FROM pic_info LEFT JOIN album_pic ON album_pic.pic_id = pic_info.id WHERE album_pic.album_id=%d AND %s%s%lld AND width_flag=%d %s", album_id, order_flag == DIVOOM_PIC_PHOTO_SLIDE_DISP_PHTOT_TIME?"photo_time":"save_time", reverse_flag==0?">":"<", (long long)save_time, pic_flag==1?1:0, movie_flag?"": " AND pic_video_flag=0 ");
            }
        }
    }
    else
    {
        
        if(album_id == 0)
        {
            if(pic_flag == 0)
            {
                divoom_database_manage_format_sql( "SELECT COUNT(*) FROM pic_info %s", movie_flag?"": " WHERE pic_video_flag=0 ");
            }
            else
            {
                divoom_database_manage_format_sql( "SELECT COUNT(*) FROM pic_info WHERE width_flag=%d %s",pic_flag==1?1:0, movie_flag?"": " AND pic_video_flag=0 ");
            }
        }
        else
        {
            if(pic_flag == 0)
            {
                divoom_database_manage_format_sql( "SELECT COUNT(*) FROM pic_info LEFT JOIN album_pic ON album_pic.pic_id = pic_info.id WHERE album_pic.album_id=%d %s", album_id, movie_flag?"": " AND pic_video_flag=0 ");
            }
            else
            {
                divoom_database_manage_format_sql( "SELECT COUNT(*) FROM pic_info LEFT JOIN album_pic ON album_pic.pic_id = pic_info.id WHERE album_pic.album_id=%d AND width_flag=%d %s", album_id, pic_flag==1?1:0, movie_flag?"": " AND pic_video_flag=0 ");
            }
        }
    }


    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    //divoom_print("sql info: %s\n", pdivoom_database_manage_handle->sql_str);
    if (rc != SQLITE_OK) {        
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
        return 0;
    }
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    index = pnColumn;
    if(pnRow > 0)
    {
        pic_id = atoi(dbResult[index]);
    }
    else
    {
        pic_id = 0;
    }
    sqlite3_free_table(dbResult);
    return pic_id;
}

/*********************************************************************
  Function:        divoom_database_manage_get_first_pic_id
  Description:     从相簿中得到最后一张照片
  Input:           
                   
  Return:          id
**********************************************************************/

int divoom_database_manage_get_first_pic_id(int album_id)
{
    int i, j, rc, pnRow, pnColumn, index, pic_id;
    
    char **dbResult;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

    if(album_id == 0 || album_id == divoom_pic_album_manage_get_album_id_from_type(DIVOOM_PIC_ALBUM_TYPE_ALL))
    {
        strcpy(pdivoom_database_manage_handle->sql_str, "SELECT id FROM pic_info ORDER BY save_time DESC LIMIT 1");
    }
    else
    {
        divoom_database_manage_format_sql( "SELECT pic_info.id FROM pic_info LEFT JOIN album_pic ON album_pic.pic_id = pic_info.id WHERE album_pic.album_id=%d ORDER BY  save_time DESC LIMIT 1", album_id);
    }

    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {        
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return -1;
    } 
    index = pnColumn;
    if(pnRow > 0)
    {
        pic_id = atoi(dbResult[index]);
    }
    else
    {
        pic_id = -1;
    }
    sqlite3_free_table(dbResult);
    return pic_id;
}

/*********************************************************************
  Function:        divoom_database_manage_get_max_pic_id
  Description:     得到相片中最大ID的相片ID
  Input:           
                   
  Return:          id
**********************************************************************/

int divoom_database_manage_get_max_pic_id(void)
{
    int i, j, rc, pnRow, pnColumn, index, pic_id;
    
    char **dbResult;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    
    strcpy(pdivoom_database_manage_handle->sql_str, "SELECT id FROM pic_info ORDER BY id DESC LIMIT 1");


    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {        
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return -1;
    } 
    index = pnColumn;
    if(pnRow > 0)
    {
        pic_id = atoi(dbResult[index]);
    }
    else
    {
        pic_id = -1;
    }
    sqlite3_free_table(dbResult);
    return pic_id;
}

/*********************************************************************
  Function:        divoom_database_manage_get_all_pic_id
  Description:     从相簿中得到所有照片，并按相关排序
  Input:           album_id
                   pic_num   相片数目
                   order_flag       按save_time或者photo_time排序
                   desc_flag        1：是降序；0：升序
                   start_time       从这个时间之后的记录
                   pic_flag 0:所有的；1：是宽的；2：长的
  Return:          列表
**********************************************************************/

uint32 *divoom_database_manage_get_all_pic_id(int  album_id, int *pic_num, int order_flag, int desc_flag, double start_time, int pic_flag, int movie_flag)
{
    int i, j, rc, pnRow, pnColumn, index;
    uint32 *pic_info;
    char **dbResult, *order_str;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

    if(pic_flag)
    {
        if(album_id == 0 || album_id == divoom_pic_album_manage_get_album_id_from_type(DIVOOM_PIC_ALBUM_TYPE_ALL))
        {
            if(order_flag != DIVOOM_PIC_PHOTO_SLIDE_DISP_PHTOT_TIME)
            {
                if(start_time < 0)
                {
                    
                    divoom_database_manage_format_sql( "SELECT id FROM pic_info WHERE width_flag=%d %s ORDER BY save_time %s LIMIT %d ",pic_flag==1?1:0, movie_flag?"": " AND pic_video_flag=0 ", desc_flag==0?"DESC":"ASC", *pic_num);
                }
                else
                {
                    
                    divoom_database_manage_format_sql( "SELECT id FROM pic_info WHERE save_time %s %lld AND width_flag=%d %s ORDER BY save_time %s LIMIT %d ", desc_flag==0?"<":">", (long long)start_time,pic_flag==1?1:0, movie_flag?"": " AND pic_video_flag=0 ", desc_flag==0?"DESC":"", *pic_num);
                }
            }
            else
            {
                if(start_time < 0)
                {
                    
                    divoom_database_manage_format_sql( "SELECT id FROM pic_info WHERE width_flag=%d %s ORDER BY photo_time %s LIMIT %d ",pic_flag==1?1:0, movie_flag?"": " AND pic_video_flag=0 ", desc_flag==0?"DESC":"", *pic_num);
                }
                else
                {
                    
                    divoom_database_manage_format_sql( "SELECT id FROM pic_info WHERE photo_time %s %d AND width_flag=%d %s ORDER BY photo_time %s LIMIT %d ", desc_flag==0?"<":">", (int)start_time,pic_flag==1?1:0, movie_flag?"": " AND pic_video_flag=0 ", desc_flag==0?"DESC":"", *pic_num);
                }
            }
        }
        else
        {
            if(order_flag !=DIVOOM_PIC_PHOTO_SLIDE_DISP_PHTOT_TIME)
            {
                if(start_time < 0)
                {
                    
                    divoom_database_manage_format_sql( "SELECT pic_info.id FROM pic_info LEFT JOIN album_pic ON album_pic.pic_id = pic_info.id WHERE album_pic.album_id=%d AND width_flag=%d %s ORDER BY  save_time %s LIMIT %d", album_id,pic_flag==1?1:0, movie_flag?"": " AND pic_video_flag=0 ", desc_flag==0?"DESC":"", *pic_num);
                }
                else
                {
                    
                    divoom_database_manage_format_sql( "SELECT pic_info.id FROM pic_info LEFT JOIN album_pic ON album_pic.pic_id = pic_info.id WHERE album_pic.album_id=%d AND save_time %s %lld AND width_flag=%d %s  ORDER BY  save_time %s LIMIT %d", album_id, desc_flag==0?"<":">", (long long)start_time,pic_flag==1?1:0, movie_flag?"": " AND pic_video_flag=0 ", desc_flag==0?"DESC":"", *pic_num);
                }
            }
            else
            {
                if(start_time < 0)
                {
                    
                    divoom_database_manage_format_sql( "SELECT pic_info.id FROM pic_info LEFT JOIN album_pic ON album_pic.pic_id = pic_info.id WHERE album_pic.album_id=%d AND width_flag=%d %s  ORDER BY  photo_time %s LIMIT %d", album_id,pic_flag==1?1:0, movie_flag?"": " AND pic_video_flag=0 ", desc_flag==0?"DESC":"", *pic_num);
                }
                else
                {
                    
                    divoom_database_manage_format_sql( "SELECT pic_info.id FROM pic_info LEFT JOIN album_pic ON album_pic.pic_id = pic_info.id WHERE album_pic.album_id=%d AND photo_time %s %d AND width_flag=%d %s ORDER BY  photo_time %s LIMIT %d", album_id, desc_flag==0?"<":">", (int)start_time,pic_flag==1?1:0, movie_flag?"": " AND pic_video_flag=0 ", desc_flag==0?"DESC":"", *pic_num);
                }
            }
        }
    }
    else
    {
        if(album_id == 0 || album_id == divoom_pic_album_manage_get_album_id_from_type(DIVOOM_PIC_ALBUM_TYPE_ALL))
        {
            if(order_flag != DIVOOM_PIC_PHOTO_SLIDE_DISP_PHTOT_TIME)
            {
                if(start_time < 0)
                {
                    
                    divoom_database_manage_format_sql( "SELECT id FROM pic_info %s ORDER BY save_time %s LIMIT %d ", movie_flag?"": " WHERE pic_video_flag=0 ", desc_flag==0?"DESC":"", *pic_num);
                }
                else
                {
                    
                    divoom_database_manage_format_sql( "SELECT id FROM pic_info WHERE save_time %s %lld %s ORDER BY save_time %s LIMIT %d ", desc_flag==0?"<":">", (long long)start_time, movie_flag?"": " AND pic_video_flag=0 ", desc_flag==0?"DESC":"", *pic_num);
                }
            }
            else
            {
                if(start_time < 0)
                {
                    
                    divoom_database_manage_format_sql( "SELECT id FROM pic_info %s ORDER BY photo_time %s LIMIT %d ", movie_flag?"": " WHERE pic_video_flag=0 ", desc_flag==0?"DESC":"", *pic_num);
                }
                else
                {
                    
                    divoom_database_manage_format_sql( "SELECT id FROM pic_info WHERE photo_time %s %d %s ORDER BY photo_time %s LIMIT %d ", desc_flag==0?"<":">", (int)start_time, movie_flag?"": " AND pic_video_flag=0 ", desc_flag==0?"DESC":"", *pic_num);
                }
            }
        }
        else
        {
            if(order_flag != DIVOOM_PIC_PHOTO_SLIDE_DISP_PHTOT_TIME)
            {
                if(start_time < 0)
                {
                    
                    divoom_database_manage_format_sql( "SELECT pic_info.id FROM pic_info LEFT JOIN album_pic ON album_pic.pic_id = pic_info.id WHERE album_pic.album_id=%d %s ORDER BY  save_time %s LIMIT %d", album_id, movie_flag?"": " AND pic_video_flag=0 ", desc_flag==0?"DESC":"", *pic_num);
                }
                else
                {
                    
                    divoom_database_manage_format_sql( "SELECT pic_info.id FROM pic_info LEFT JOIN album_pic ON album_pic.pic_id = pic_info.id WHERE album_pic.album_id=%d AND save_time %s %lld %s ORDER BY  save_time %s LIMIT %d", album_id, desc_flag==0?"<":">", (long long)start_time, movie_flag?"": " AND pic_video_flag=0 ", desc_flag==0?"DESC":"", *pic_num);
                }
            }
            else
            {
                if(start_time < 0)
                {
                    
                    divoom_database_manage_format_sql( "SELECT pic_info.id FROM pic_info LEFT JOIN album_pic ON album_pic.pic_id = pic_info.id WHERE album_pic.album_id=%d %s ORDER BY  photo_time %s LIMIT %d", album_id, movie_flag?"": " AND pic_video_flag=0 ", desc_flag==0?"DESC":"", *pic_num);
                }
                else
                {
                    
                    divoom_database_manage_format_sql( "SELECT pic_info.id FROM pic_info LEFT JOIN album_pic ON album_pic.pic_id = pic_info.id WHERE album_pic.album_id=%d AND photo_time %s %d %s ORDER BY  photo_time %s LIMIT %d", album_id, desc_flag==0?"<":">", (int)start_time, movie_flag?"": " AND pic_video_flag=0 ", desc_flag==0?"DESC":"", *pic_num);
                }
            }
        }
    }
    //divoom_print("sql info: %s\n", pdivoom_database_manage_handle->sql_str);
    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    if (rc != SQLITE_OK) {        
        *pic_num = 0;
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
        return NULL;
    }
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    pic_info = divoom_system_malloc(sizeof(uint32)*pnRow);
    index = pnColumn;
    *pic_num = pnRow;
    for(i = 0; i < pnRow; i++)
    {
        pic_info[i] = atoi(dbResult[index]);
        index ++;
    }
    sqlite3_free_table(dbResult);
    return pic_info;
}

/*********************************************************************
  Function:        divoom_database_manage_get_all_pic_id_reverse
  Description:     从相簿中得到所有照片，并按相关排序
  Input:           album_id
                   pic_num   相片数目
  Return:          列表
**********************************************************************/

uint32 *divoom_database_manage_get_all_pic_id_reverse(int  album_id, int *pic_num, int order_flag, int desc_flag, double start_time, int movie_flag)
{
    int i, j, rc, pnRow, pnColumn, index;
    uint32 *pic_info;
    char **dbResult, *order_str;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

    if(album_id == 0 || album_id == divoom_pic_album_manage_get_album_id_from_type(DIVOOM_PIC_ALBUM_TYPE_ALL))
    {
        if(order_flag != DIVOOM_PIC_PHOTO_SLIDE_DISP_PHTOT_TIME)
        {
            if(start_time < 0)
            {
                
                divoom_database_manage_format_sql( "SELECT id FROM pic_info %s ORDER BY save_time %s LIMIT %d ", movie_flag?"": " WHERE pic_video_flag=0 ", desc_flag==0?"":"DESC", *pic_num);
            }
            else
            {
                
                divoom_database_manage_format_sql( "SELECT id FROM pic_info WHERE save_time %s %lld %s ORDER BY save_time %s LIMIT %d ", desc_flag==0?">":"<", (long long)start_time, movie_flag?"": " AND pic_video_flag=0 ", desc_flag==0?"":"DESC", *pic_num);
            }
        }
        else
        {
            if(start_time < 0)
            {
                
                divoom_database_manage_format_sql( "SELECT id FROM pic_info %s ORDER BY photo_time %s LIMIT %d ", movie_flag?"": " WHERE pic_video_flag=0 ", desc_flag==0?"":"DESC", *pic_num);
            }
            else
            {
                
                divoom_database_manage_format_sql( "SELECT id FROM pic_info WHERE photo_time %s %d %s ORDER BY photo_time %s LIMIT %d ", desc_flag==0?">":"<", (int)start_time, movie_flag?"": " AND pic_video_flag=0 ", desc_flag==0?"":"DESC", *pic_num);
            }
        }
    }
    else
    {
        if(order_flag != DIVOOM_PIC_PHOTO_SLIDE_DISP_PHTOT_TIME)
        {
            if(start_time < 0)
            {
                
                divoom_database_manage_format_sql( "SELECT pic_info.id FROM pic_info LEFT JOIN album_pic ON album_pic.pic_id = pic_info.id WHERE album_pic.album_id=%d %s ORDER BY  save_time %s LIMIT %d", album_id, movie_flag?"": " AND pic_video_flag=0 ", desc_flag==0?"":"DESC", *pic_num);
            }
            else
            {
                
                divoom_database_manage_format_sql( "SELECT pic_info.id FROM pic_info LEFT JOIN album_pic ON album_pic.pic_id = pic_info.id WHERE album_pic.album_id=%d AND save_time %s %lld %s ORDER BY  save_time %s LIMIT %d", album_id, desc_flag==0?">":"<", (long long)start_time, movie_flag?"": " AND pic_video_flag=0 ", desc_flag==0?"":"DESC", *pic_num);
            }
        }
        else
        {
            if(start_time < 0)
            {
                
                divoom_database_manage_format_sql( "SELECT pic_info.id FROM pic_info LEFT JOIN album_pic ON album_pic.pic_id = pic_info.id WHERE album_pic.album_id=%d %s ORDER BY  photo_time %s LIMIT %d", album_id, movie_flag?"": " AND pic_video_flag=0 ", desc_flag==0?"":"DESC", *pic_num);
            }
            else
            {
                
                divoom_database_manage_format_sql( "SELECT pic_info.id FROM pic_info LEFT JOIN album_pic ON album_pic.pic_id = pic_info.id WHERE album_pic.album_id=%d AND photo_time %s %d %s ORDER BY  photo_time %s LIMIT %d", album_id, desc_flag==0?">":"<", (int)start_time, movie_flag?"": " AND pic_video_flag=0 ", desc_flag==0?"":"DESC", *pic_num);
            }
        }
    }
    
    //divoom_print("sql info: %s\n", pdivoom_database_manage_handle->sql_str);
    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    if (rc != SQLITE_OK) {        
        *pic_num = 0;
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
        return NULL;
    }
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    pic_info = divoom_system_malloc(sizeof(uint32)*pnRow);
    index = pnColumn;
    *pic_num = pnRow;
    for(i = 0; i < pnRow; i++)
    {
        pic_info[i] = atoi(dbResult[index]);
        index ++;
    }
    sqlite3_free_table(dbResult);
    return pic_info;
}

/*********************************************************************
  Function:        divoom_database_manage_get_all_pic_from_album_table
  Description:     从相簿中得到所有照片，并按相关排序
  Input:           album_id
                   pic_num   相片数目
                   order_flag
                   desc_flag
                   pic_flag 0:所有的；1：是宽的；2：长的
  Return:          列表
**********************************************************************/

Pdivoom_pic_id_width_info divoom_database_manage_get_all_pic_id_width(int  album_id, int *pic_num, int order_flag, int desc_flag, int pic_flag)
{
    int i, j, rc, pnRow, pnColumn, index;
    divoom_pic_id_width_info *pic_info;
    char **dbResult;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

    if(pic_flag)
    {
        if(album_id == 0 || album_id == divoom_pic_album_manage_get_album_id_from_type(DIVOOM_PIC_ALBUM_TYPE_ALL))
        {
            divoom_database_manage_format_sql( "SELECT id,width_flag FROM pic_info ORDER BY %s %s", order_flag?"save_time":"photo_time", desc_flag==0?"DESC":"");
        }
        else
        {
            divoom_database_manage_format_sql( "SELECT pic_info.id,pic_info.width_flag FROM pic_info LEFT JOIN album_pic ON album_pic.pic_id = pic_info.id WHERE album_pic.album_id=%d ORDER BY %s %s", album_id, order_flag?"pic_info.save_time":"pic_info.photo_time", desc_flag==0?"DESC":"");
        }
    }
    else
    {
        if(album_id == 0 || album_id == divoom_pic_album_manage_get_album_id_from_type(DIVOOM_PIC_ALBUM_TYPE_ALL))
        {
            divoom_database_manage_format_sql( "SELECT id,width_flag FROM pic_info ORDER BY %s %s", order_flag?"save_time":"photo_time", desc_flag==0?"DESC":"");
        }
        else
        {
            divoom_database_manage_format_sql( "SELECT pic_info.id,pic_info.width_flag FROM pic_info LEFT JOIN album_pic ON album_pic.pic_id = pic_info.id WHERE album_pic.album_id=%d ORDER BY %s %s", album_id, order_flag?"pic_info.save_time":"pic_info.photo_time", desc_flag==0?"DESC":"");
        }
    }
    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {        
        *pic_num = 0;
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return NULL;
    } 
    pic_info = divoom_system_malloc(sizeof(divoom_pic_id_width_info)*pnRow);
    index = pnColumn;
    *pic_num = pnRow;
    for(i = 0; i < pnRow; i++)
    {
        pic_info[i].id = atoi(dbResult[index]);
        index ++;
        pic_info[i].width_flag = atoi(dbResult[index]);
        index ++;
    }
    sqlite3_free_table(dbResult);
    return pic_info;
}

/*********************************************************************
  Function:        divoom_database_manage_check_album_include_pic
  Description:     检查相簿是否包含这个相片
  Input:           pic_id
                   album_id   
  Return:          相簿列表
**********************************************************************/

int divoom_database_manage_check_album_include_pic(int album_id,int pic_id)
{
    int rc, pnRow, pnColumn;
    char **dbResult;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    
    divoom_database_manage_format_sql( "SELECT album_id FROM album_pic WHERE pic_id=%d AND album_id=%d",pic_id, album_id);

    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {        
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return 0;
    } 
    
    sqlite3_free_table(dbResult);
    return pnRow >= 1;
}

/*********************************************************************
  Function:        divoom_database_manage_get_album_pic_list
  Description:     从相簿中得到指定数量未隐藏的相片
  Input:           album_id
                   start
                   pic_num   相片数目
  Return:          照片列表ID
**********************************************************************/

uint32 *divoom_database_manage_get_album_pic_list(int  album_id, int start,int *pic_num)
{
    int i, j, rc, pnRow, pnColumn, index;
    uint32 *pic_list;
    char **dbResult;
    
    if(*pic_num == 0)
    {
        return NULL;
    }
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

    divoom_database_manage_format_sql( "SELECT pic_info.id FROM pic_info LEFT JOIN album_pic ON album_pic.pic_id = pic_info.id WHERE album_pic.album_id=%d AND hide_flag=0  ORDER BY pic_info.save DESC limit %d offset %d", album_id, *pic_num, start);
    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return NULL;
    } 
    
    pic_list = divoom_system_malloc(sizeof(uint32)*pnRow);
    index = pnColumn;
    *pic_num = pnRow;
    for(i = 0; i < pnRow; i++)
    {
        for(j = 0; j < pnColumn; j++)
        {
            pic_list[i] = atoi(dbResult[index]);
            index ++;
        }
    }
    sqlite3_free_table(dbResult);
    return pic_list;
}

/*********************************************************************
  Function:        divoom_database_manage_get_subscribe_image_last_update_time
  Description:     获取某个类型订阅最后更新的时间
  Input:           none
  Return:          最后更新时间
**********************************************************************/

int divoom_database_manage_get_subscribe_image_last_update_time(char *table_name, int image_type)
{
    int rc, update_time;
    int pnRow, pnColumn;
    char **dbResult;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

    divoom_database_manage_format_sql( "SELECT update_time FROM %s where image_type=%d ORDER BY update_time DESC limit 1",table_name, image_type);
    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return 0;
    } 
    if(pnRow == 0)
    {
        update_time = 0;
    }
    else
    {
        update_time = atoi(dbResult[1]);
    }
    sqlite3_free_table(dbResult);
    return update_time;
}

/*********************************************************************
  Function:        divoom_database_manage_get_subscribe_image_pic_list
  Description:     获取某个专辑的所有照片ID
  Input:           clock_id
                   clock_pos
                   parent_id
                   pic_num   相片数目
  Return:          照片列表ID
**********************************************************************/

static char *divoom_database_manage_copy_image_name(const char *image_name)
{
    uint32 name_size;
    char *name_copy;

    if(image_name == NULL)
    {
        image_name = "";
    }
    name_size = (uint32)strlen(image_name) + 1;
    name_copy = divoom_system_malloc(name_size);
    if(name_copy != NULL)
    {
        memcpy(name_copy, image_name, name_size);
    }
    return name_copy;
}

static void divoom_database_manage_free_loaded_image_list(
    Pdivoom_image_hot_item image_list, int32 image_num)
{
    int32 i;

    if(image_list == NULL)
    {
        return;
    }
    for(i = 0; i < image_num; i++)
    {
        if(image_list[i].image_name != NULL)
        {
            divoom_system_free(image_list[i].image_name);
        }
    }
    divoom_system_free(image_list);
}

Pdivoom_image_hot_item divoom_database_manage_get_subscribe_image_pic_list(
    char *table_name, int image_type, int *pic_num)
{
    int i, rc, pnRow, pnColumn, index;
    int32 has_image_size;
    Pdivoom_image_hot_item pic_list;
    char **dbResult;

    if(pic_num == NULL)
    {
        return NULL;
    }
    *pic_num = 0;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

    has_image_size = strcmp(table_name, DIVOOM_HOT_IMAGE_TABLE_NAME) == 0;
    if(has_image_size)
    {
        divoom_database_manage_format_sql(
            "SELECT image_size,image_id,author_id,image_name FROM %s WHERE image_type=%d ORDER BY image_id ",
            table_name, image_type);
    }
    else
    {
        divoom_database_manage_format_sql(
            "SELECT image_id,author_id,image_name FROM %s WHERE image_type=%d ORDER BY image_id ",
            table_name, image_type);
    }
    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return NULL;
    } 
    
    *pic_num = pnRow;
    if (pnRow <= 0)
    {
        sqlite3_free_table(dbResult);
        return NULL;
    }
    if((uint32)pnRow >
        U32_MAX / (uint32)sizeof(divoom_image_hot_item))
    {
        *pic_num = 0;
        sqlite3_free_table(dbResult);
        return NULL;
    }
    pic_list = divoom_system_malloc(
        (uint32)pnRow * sizeof(divoom_image_hot_item));
    if (pic_list == NULL)
    {
        *pic_num = 0;
        sqlite3_free_table(dbResult);
        return NULL;
    }
    index = pnColumn;
    for(i = 0; i < pnRow; i++)
    {
        pic_list[i].image_size = 0;
        if(has_image_size)
        {
            pic_list[i].image_size = (uint32)atoi(dbResult[index]);
            index++;
        }
        pic_list[i].image_id = (uint32)atoi(dbResult[index]);
        index ++;
        pic_list[i].user_id = (uint32)atoi(dbResult[index]);
        index ++;
        pic_list[i].image_name =
            divoom_database_manage_copy_image_name(dbResult[index]);
        index++;
        if(pic_list[i].image_name == NULL)
        {
            *pic_num = 0;
            divoom_database_manage_free_loaded_image_list(pic_list, i);
            sqlite3_free_table(dbResult);
            return NULL;
        }
    }
    sqlite3_free_table(dbResult);
    return pic_list;
}

/*********************************************************************
  Function:        divoom_database_manage_get_subscribe_image_pic_list_by_updatetime
  Description:     获取某个专辑的所有照片ID
  Input:           clock_id
                   clock_pos
                   parent_id
                   pic_num   相片数目
  Return:          照片列表ID
**********************************************************************/

Pdivoom_image_hot_item
divoom_database_manage_get_subscribe_image_pic_list_by_updatetime(
    char *table_name, int image_type, int *pic_num)
{
    int i, rc, pnRow, pnColumn, index;
    Pdivoom_image_hot_item pic_list;
    char **dbResult;

    if(pic_num == NULL)
    {
        return NULL;
    }
    *pic_num = 0;
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

    divoom_database_manage_format_sql(
        "SELECT image_id,author_id,image_name FROM %s WHERE image_type=%d ORDER BY update_time desc ",
        table_name, image_type);
    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return NULL;
    } 
    
    *pic_num = pnRow;
    if (pnRow <= 0)
    {
        sqlite3_free_table(dbResult);
        return NULL;
    }
    if((uint32)pnRow >
        U32_MAX / (uint32)sizeof(divoom_image_hot_item))
    {
        *pic_num = 0;
        sqlite3_free_table(dbResult);
        return NULL;
    }
    pic_list = divoom_system_malloc(
        (uint32)pnRow * sizeof(divoom_image_hot_item));
    if (pic_list == NULL)
    {
        *pic_num = 0;
        sqlite3_free_table(dbResult);
        return NULL;
    }
    index = pnColumn;
    for(i = 0; i < pnRow; i++)
    {
        pic_list[i].image_size = 0;
        pic_list[i].image_id = (uint32)atoi(dbResult[index]);
        index ++;
        pic_list[i].user_id = (uint32)atoi(dbResult[index]);
        index ++;
        pic_list[i].image_name =
            divoom_database_manage_copy_image_name(dbResult[index]);
        index ++;
        if(pic_list[i].image_name == NULL)
        {
            *pic_num = 0;
            divoom_database_manage_free_loaded_image_list(pic_list, i);
            sqlite3_free_table(dbResult);
            return NULL;
        }
    }
    sqlite3_free_table(dbResult);
    return pic_list;
}

/*********************************************************************
  Function:        divoom_database_manage_delete_subscribe_image_item
  Description:     删除专辑的某个资源
  Input:           album_id
                   image_id
  Return:          1 or 0
**********************************************************************/

/* Caller holds the subscription lock (and HOT state lock for HOT).
 * Delete at most 20 physical files, then commit their rows with one DELETE.
 * Keep rows for failed/in-flight files so a later cleanup can retry them.
 */
int divoom_database_manage_clear_unused_subscribe_batch(
    int mode, uint32 *deleted_ids, int *deleted_num)
{
    static const char *tables[] = {
        DIVOOM_HOT_IMAGE_TABLE_NAME, DIVOOM_FAVORITE_IMAGE_TABLE_NAME,
        DIVOOM_EXPERT_IMAGE_TABLE_NAME, DIVOOM_ALBUM_MANAGE_TABLE_NAME,
        DIVOOM_USERIMAGELIST_TABLE_NAME
    };
    static const uint32 models[] = {
        DIVOOM_MANAGE_IMAGE_SUBSCRIBE_DEFAULT_16,
        DIVOOM_MANAGE_IMAGE_SUBSCRIBE_FAVORITE, 0,
        DIVOOM_MANAGE_IMAGE_ALBUM_ID_LIST_PIC,
        DIVOOM_MANAGE_IMAGE_USER_IMAGE_LIST_PIC
    };
    sqlite3_stmt *stmt = NULL;
    uint32 image_ids[DIVOOM_SUBSCRIBE_CLEAN_BATCH_SIZE];
    uint32 image_types[DIVOOM_SUBSCRIBE_CLEAN_BATCH_SIZE];
    char sql[2048];
    int count = 0, i, rc, used, failed = 0;
    int expert = mode == DIVOOM_DISP_SUBSCRIBE_MODE_EXPERT;

    *deleted_num = 0;
    if(!divoom_db_is_ready() || mode < DIVOOM_DISP_SUBSCRIBE_MODE_HOT ||
        mode > DIVOOM_DISP_SUBSCRIBE_MODE_PLAY_LIST)
    {
        return -1;
    }
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    /* Shared modes stay protected, including configurations not currently open.
     * Missing/invalid configurations are not permission to delete resources.
     */
    snprintf(sql, sizeof(sql),
        "SELECT DISTINCT %s,image_id FROM %s WHERE "
        "EXISTS (SELECT 1 FROM %s) AND NOT EXISTS "
        "(SELECT 1 FROM %s WHERE mode=%d OR mode IS NULL OR mode<%d OR mode>%d) "
        "ORDER BY image_id LIMIT %d",
        expert ? "image_type" : "0", tables[mode],
        DIVOOM_SUBSCRIBE_IMAGE_TABLE_NAME, DIVOOM_SUBSCRIBE_IMAGE_TABLE_NAME,
        mode, DIVOOM_DISP_SUBSCRIBE_MODE_HOT,
        DIVOOM_DISP_SUBSCRIBE_MODE_PLAY_LIST, DIVOOM_SUBSCRIBE_CLEAN_BATCH_SIZE);
    rc = sqlite3_prepare_v2(pdivoom_database_manage_handle->db_handle,
        sql, -1, &stmt, NULL);
    if(rc != SQLITE_OK)
    {
        goto finish;
    }
    while((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        image_types[count] = (uint32)sqlite3_column_int64(stmt, 0);
        image_ids[count] = (uint32)sqlite3_column_int64(stmt, 1);
        count++;
    }
    sqlite3_finalize(stmt);
    stmt = NULL;
    if(rc != SQLITE_DONE)
    {
        goto finish;
    }
    used = snprintf(sql, sizeof(sql), "DELETE FROM %s WHERE ", tables[mode]);
    for(i = 0; i < count; i++)
    {
        uint32 model = expert ? image_types[i] : models[mode];
        if(!divoom_manage_image_del_clock_resource(model, image_ids[i]))
        {
            failed = 1;
            continue;
        }
        used += snprintf(sql + used, sizeof(sql) - used,
            "%s(image_id=%d%s", *deleted_num ? " OR " : "", (int32)image_ids[i],
            expert ? " AND " : ")");
        if(expert)
        {
            used += snprintf(sql + used, sizeof(sql) - used,
                "image_type=%d)", (int32)image_types[i]);
        }
        deleted_ids[(*deleted_num)++] = image_ids[i];
    }
    rc = SQLITE_OK;
    if(*deleted_num > 0)
    {
        rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle,
            sql, NULL, NULL, NULL);
    }
finish:
    sqlite3_finalize(stmt);
    if(rc != SQLITE_OK && rc != SQLITE_DONE)
    {
        divoom_print("subscribe cleanup database failed: mode=%d rc=%d\n", mode, rc);
        failed = 1;
    }
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    return failed ? -1 : *deleted_num;
}

void divoom_database_manage_delete_subscribe_image_item(char *table_name, int  image_type, int  image_id)
{
    int rc;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    if(image_type < 0)
    {
        divoom_database_manage_format_sql("DELETE FROM %s WHERE image_id=%d", table_name, image_id);
    }
    else
    {
        divoom_database_manage_format_sql("DELETE FROM %s WHERE image_type=%d AND image_id=%d", table_name, image_type, image_id);
    }
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        divoom_print("Insert failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
    } 
    
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
}

/*********************************************************************
  Function:        divoom_database_manage_delete_subscribe_image_all
  Description:     删除某个专辑的所有资源
  Input:           album_id
  Return:          1 or 0
**********************************************************************/

void divoom_database_manage_delete_subscribe_image_all(char *table_name, int  image_type)
{
    int rc;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    divoom_database_manage_format_sql( "DELETE FROM %s WHERE image_type=%d", table_name, image_type);
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        divoom_print("Insert failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
    } 
    
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
}
/*********************************************************************
  Function:        divoom_database_manage_add_subscribe_image_item
  Description:     增加某个资源
  Input:           album_id
                   image_id
                   update_time
                   file_addr
  Return:          1 or 0
**********************************************************************/

void divoom_database_manage_add_subscribe_image_item(char *table_name, int  image_type, int  image_id, int update_time, char *file_addr, int user_id, char *image_name)
{
    int rc;
    char new_image_name[200];

    divoom_app_com_add_escape_quote_safe(image_name, new_image_name, sizeof(new_image_name));
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    divoom_database_manage_format_sql(
        "INSERT INTO %s(image_type,image_id,author_id,image_name,update_time,file_addr) VALUES(%d,%d,%d,'%s',%d,'%s')",
        table_name, image_type, image_id, user_id,
        new_image_name, update_time, file_addr);
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle,
        pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    if(rc != SQLITE_OK)
    {
        divoom_print("Insert failed: %s ; %s\n",
            pdivoom_database_manage_handle->sql_str,
            sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
    }
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
}

void divoom_database_manage_add_subscribe_image_item_with_size(
    char *table_name, int image_type, int image_size, int image_id,
    int update_time, char *file_addr, int user_id, char *image_name)
{
    int rc;
    char new_image_name[200];

    divoom_app_com_add_escape_quote_safe(image_name, new_image_name, sizeof(new_image_name));
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    divoom_database_manage_format_sql(
        "INSERT INTO %s(image_type,image_size,image_id,author_id,image_name,update_time,file_addr) VALUES(%d,%d,%d,%d,'%s',%d,'%s')",
        table_name, image_type, image_size, image_id, user_id,
        new_image_name, update_time, file_addr);
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        divoom_print("Insert failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
    } 
    
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    
}

void divoom_database_manage_update_subscribe_image_size(char *table_name, int image_id, int image_type, int image_size)
{
    int rc;

    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    divoom_database_manage_format_sql(
        "UPDATE %s SET image_type=%d,image_size=%d WHERE image_id=%d",
        table_name, image_type, image_size, image_id);
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    if(rc != SQLITE_OK)
    {
        divoom_print("Update failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
    }
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
}

/*********************************************************************
  Function:        divoom_database_manage_update_subscribe_image_name_by_user_id
  Description:     按作者 USERID 批量更新订阅图片记录中的作者名称
  Input:           table_name
                   user_id
                   image_name
  Return:          none
**********************************************************************/

void divoom_database_manage_update_subscribe_image_name_by_user_id(
    char *table_name, int user_id, char *image_name)
{
    int rc;
    char new_image_name[200];

    if(table_name == NULL || image_name == NULL || user_id == 0)
    {
        return;
    }
    divoom_app_com_add_escape_quote_safe(
        image_name, new_image_name, sizeof(new_image_name));
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    divoom_database_manage_format_sql(
        "UPDATE %s SET image_name='%s' WHERE author_id=%d",
        table_name, new_image_name, user_id);
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle,
        pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    if(rc != SQLITE_OK)
    {
        divoom_print("Update failed: %s ; %s\n",
            pdivoom_database_manage_handle->sql_str,
            sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
    }
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
}

/*********************************************************************
  Function:        divoom_database_manage_get_subscribe_image_pic_addr
  Description:     获取某个专辑的照片地址
  Input:           image_id
                   file_addr
  Return:          1 or 0
**********************************************************************/

int divoom_database_manage_get_subscribe_image_pic_addr(char *table_name, int image_id, char *file_addr)
{
    int ret = 0, rc, pnRow, pnColumn;
    uint32 *pic_list;
    char **dbResult;

    /* Product-test startup does not initialize the subscription database. */
    if (!pdivoom_database_manage_handle ||
        !pdivoom_database_manage_handle->lock_handle ||
        !pdivoom_database_manage_handle->db_handle)
        return 0;

    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

    divoom_database_manage_format_sql( "SELECT file_addr FROM %s WHERE image_id=%d",table_name, image_id);
    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return 0;
    } 
    if(pnRow > 0)
    {
        if(file_addr)
        {
            strcpy(file_addr, dbResult[pnColumn]);
        }
        ret = 1;
    }
    sqlite3_free_table(dbResult);
    return 1;
}

/*********************************************************************
  Function:        divoom_database_manage_update_subscribe_config_record
  Description:     更新记录，不存在就插入
  Input:           table_name
                   clock_id
                   clock_pos
                   parent_id
                   play_time
  Return:          1 or 0
**********************************************************************/

int divoom_database_manage_update_subscribe_play_time_record(char *table_name, int clock_id, int clock_pos, int parent_id, int play_time, int show_time)
{
    //其中expert_list保存JSON数据
    int rc, ret;
    
    if(divoom_database_manage_get_subscribe_config_record(table_name, clock_id, clock_pos, parent_id, NULL))
    {
        //更新s
        divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
        divoom_database_manage_format_sql( "UPDATE %s SET play_time=%d,show_time=%d WHERE clock_id=%d AND clock_pos=%d AND parent_id=%d", table_name,
            play_time, show_time, clock_id, clock_pos, parent_id);
        rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
        divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
        if (rc != SQLITE_OK) {
            divoom_print("Insert failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        }
        ret = 0;
    }
    else
    {
        //插入
        divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
        divoom_database_manage_format_sql( "INSERT INTO %s(clock_id,clock_pos,parent_id,play_time,show_time) VALUES(%d,%d,%d,%d,%d)", table_name,
            clock_id, clock_pos, parent_id, play_time, show_time);
        rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
        if (rc != SQLITE_OK) {
            divoom_print("Insert failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        } 
        
        divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
        
        ret = 1;
    }

    return ret;
}


/*********************************************************************
  Function:        divoom_database_manage_update_subscribe_config_record
  Description:     更新记录，不存在就插入
  Input:           table_name
                   clock_id
                   clock_pos
                   parent_id
                   pinfo
  Return:          1 or 0
**********************************************************************/

int divoom_database_manage_update_subscribe_config_record(char *table_name, int clock_id, int clock_pos, int parent_id, Pdivoom_disp_subscribe_data pinfo)
{
    //其中expert_list保存JSON数据
    int rc, ret;
    char *expert_list_str;
    divoom_cJSON *root; 
    
    root = divoom_cJSON_CreateObject();
	divoom_cJSON_AddItemToObject(root, "ExpertList", divoom_cJSON_CreateIntArray(pinfo->expert_list, pinfo->expert_num));
	expert_list_str = divoom_cJSON_Print(root);
	divoom_cJSON_Delete(root);
    
    if(divoom_database_manage_get_subscribe_config_record(table_name, clock_id, clock_pos, parent_id, NULL))
    {
        //更新s
        divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
        divoom_database_manage_format_sql( "UPDATE %s SET mode=%d,disp=%d,play_time=%d,show_time=%d,album_id=%d,play_id=%d,expert_list='%s' WHERE clock_id=%d AND clock_pos=%d AND parent_id=%d", table_name,
            pinfo->mode, pinfo->disp, pinfo->play_time, pinfo->show_time,
            pinfo->album_id, pinfo->play_id, expert_list_str,
            clock_id, clock_pos, parent_id);
        rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
        divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
        if (rc != SQLITE_OK) {
            divoom_print("Insert failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        }
        ret = 0;
    }
    else
    {
        //插入
        divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
        divoom_database_manage_format_sql( "INSERT INTO %s(clock_id,clock_pos,parent_id,mode,disp,play_time,show_time,album_id,play_id,expert_list) VALUES(%d,%d,%d,%d,%d,%d,%d,%d,%d,'%s')", table_name,
            clock_id, clock_pos, parent_id, pinfo->mode, pinfo->disp,
            pinfo->play_time, pinfo->show_time, pinfo->album_id,
            pinfo->play_id, expert_list_str);
        rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
        if (rc != SQLITE_OK) {
            divoom_print("Insert failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        } 
        
        divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
        
        ret = 1;
    }
    divoom_system_free(expert_list_str);

    return ret;
}


/*********************************************************************
  Function:        divoom_database_manage_get_subscribe_config_record
  Description:     得到记录
  Input:           table_name
                   clock_id
                   clock_pos
                   parent_id
                   pinfo
  Return:          1 or 0
**********************************************************************/

int divoom_database_manage_get_subscribe_config_record(char *table_name, int clock_id, int clock_pos, int parent_id, Pdivoom_disp_subscribe_data pinfo)
{
    int ret = 0, rc, pnRow, pnColumn, index;
    divoom_cJSON *root, *list, *item; 
    char **dbResult;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

    divoom_database_manage_format_sql( "SELECT mode,disp,play_time,show_time,album_id,play_id,expert_list FROM %s WHERE clock_id=%d AND clock_pos=%d AND parent_id=%d LIMIT 1",table_name, clock_id, clock_pos, parent_id);
    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return 0;
    } 
    index = pnColumn;
    if(pnRow > 0)
    {
        if(pinfo)
        {
            pinfo->mode = atoi(dbResult[index]);
            index ++;
            pinfo->disp = atoi(dbResult[index]);
            index ++;
            pinfo->play_time = atoi(dbResult[index]);
            index ++;
            pinfo->show_time = atoi(dbResult[index]);
            index ++;
            pinfo->album_id = atoi(dbResult[index]);
            index ++;
            pinfo->play_id = atoi(dbResult[index]);
            index ++;
            root = divoom_cJSON_Parse(dbResult[index]);
            if(root)
            {
                list = divoom_cJSON_GetObjectItem(root, "ExpertList");
                if(list)
                {
                    pinfo->expert_num = divoom_cJSON_GetArraySize(list);
                    for(int i= 0; i < pinfo->expert_num; i++)
                    {
                        item = divoom_cJSON_GetArrayItem(list, i);
                        if(item)
                        {
                            pinfo->expert_list[i] = item->valueint;
                        }
                    }
                }
                divoom_cJSON_Delete(root);
            }
            else
            {
                pinfo->expert_num = 0;
            }
        }
        
        ret = 1;
    }
    sqlite3_free_table(dbResult);
    return ret;
}

/*********************************************************************
  Function:        divoom_database_manage_get_subscribe_config_update_time
  Description:     得到记录
  Input:           table_name
  Return:          update time
**********************************************************************/

int divoom_database_manage_get_subscribe_config_update_time(char *table_name)
{
    int rc, pnRow, pnColumn, index, update_time = 0;
    char **dbResult;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

    divoom_database_manage_format_sql( "SELECT update_time FROM %s ORDER BY update_time DESC LIMIT 1",table_name);
    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return 0;
    } 
    index = pnColumn;
    if(pnRow > 0)
    {
        
        update_time = atoi(dbResult[index]);
    }
    sqlite3_free_table(dbResult);
    return update_time;
}

/*********************************************************************
  Function:        divoom_database_manage_get_subscribe_config_update_time
  Description:     得到记录
  Input:           table_name
                   id
  Return:          update time
**********************************************************************/

int divoom_database_manage_get_item_update_time(char *table_name, int id)
{
    int rc, pnRow, pnColumn, index, update_time = 0;
    char **dbResult;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

    divoom_database_manage_format_sql( "SELECT update_time FROM %s WHERE id=%d",table_name, id);
    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return 0;
    } 
    index = pnColumn;
    if(pnRow > 0)
    {
        
        update_time = atoi(dbResult[index]);
    }
    sqlite3_free_table(dbResult);
    return update_time;
}

/*********************************************************************
  Function:        divoom_database_manage_set_subscribe_config_update_time
  Description:     得到记录
  Input:           table_name
  Return:          update time
**********************************************************************/

void divoom_database_manage_set_subscribe_config_update_time(char *table_name, int id, int update_time)
{
    int rc, pnRow, pnColumn, index;
    char **dbResult;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

    divoom_database_manage_format_sql( "SELECT update_time FROM %s WHERE id=%d",table_name, id);
    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return ;
    } 
    index = pnColumn;
    sqlite3_free_table(dbResult);
    if(pnRow > 0)
    {
        //更新s
        divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
        divoom_database_manage_format_sql( "UPDATE %s SET update_time=%d WHERE id=%d", table_name, update_time , id);
        rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
        divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
        if (rc != SQLITE_OK) {
            divoom_print("Insert failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        }
    }
    else
    {
        //插入
        divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
        divoom_database_manage_format_sql( "INSERT INTO %s(id,update_time) VALUES(%d,%d)", table_name , id, update_time);
        rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
        if (rc != SQLITE_OK) {
            divoom_print("Insert failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        } 
        
        divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
        
    }
}


/*********************************************************************
  Function:        divoom_database_manage_delete_eq_image_item
  Description:     删除某个资源
  Input:           album_id
                   image_id
  Return:          1 or 0
**********************************************************************/

void divoom_database_manage_delete_eq_image_item(char *table_name,int  image_id)
{
    int rc;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    divoom_database_manage_format_sql( "DELETE FROM %s WHERE id=%d", table_name, image_id);
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        divoom_print("Insert failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
    } 
    
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
}

/*********************************************************************
  Function:        divoom_database_manage_get_eq_image_addr
  Description:     得到某个资源地址
  Input:           table_name
                   image_id
                   file_addr
  Return:          1 or 0
**********************************************************************/

void divoom_database_manage_get_eq_image_addr(char *table_name,int  image_id, char *file_addr)
{
    int rc, pnRow, pnColumn, index = 0;
    char **dbResult;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

    divoom_database_manage_format_sql( "SELECT file_addr FROM %s WHERE id=%d",table_name,image_id);
    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        pnRow = 0;
        return ;
    } 
    index = pnColumn;
    if(pnRow > 0)
    {
        strcpy(file_addr, dbResult[index]);
    }
    sqlite3_free_table(dbResult);
    return ;
}



/*********************************************************************
  Function:        divoom_database_manage_add_eq_image_item
  Description:     添加一个元素
  Input:           table_name
                   clock_id
                   clock_pos
                   parent_id
                   update_time
                   file_id
  Return:          1 or 0
**********************************************************************/

int divoom_database_manage_add_eq_image_item(char *table_name,int clock_id, int clock_pos, int parent_id, int update_time, char *file_id, int user_id, char *image_name)
{
    int rc, pnRow, pnColumn;
    char **dbResult;
    char new_image_name[200];

    divoom_app_com_add_escape_quote_safe(image_name, new_image_name, sizeof(new_image_name));
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    
    divoom_database_manage_format_sql( "INSERT INTO %s(clock_id,clock_pos,parent_id,update_time,author_id,image_name,file_addr) VALUES(%d,%d,%d,%d,%d,'%s','%s')", table_name, clock_id, clock_pos, parent_id, update_time,user_id, new_image_name,file_id);
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
        divoom_print("Insert failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return 0;
    } else {
        int record_id = sqlite3_last_insert_rowid(pdivoom_database_manage_handle->db_handle);
        divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
        return  record_id;
    }
}

/*********************************************************************
  Function:        divoom_database_manage_get_eq_mod_index
  Description:     获取EQ的选择项
  Input:           table_name
                   clock_id
                   clock_pos
                   parent_id
                   
  Return:          mode
**********************************************************************/

int divoom_database_manage_get_eq_mode_index(char *table_name,int clock_id, int clock_pos, int parent_id)
{
    int rc, pnRow, pnColumn, index = 0, mode = 0;
    char **dbResult;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

    divoom_database_manage_format_sql( "SELECT mode FROM %s WHERE clock_id=%d AND clock_pos=%d AND parent_id=%d LIMIT 1",table_name,clock_id, clock_pos, parent_id);
    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        pnRow = 0;
        return 0;
    } 
    index = pnColumn;
    if(pnRow > 0)
    {
        mode = atoi(dbResult[index]);
    }
    sqlite3_free_table(dbResult);
    return mode;
}

/*********************************************************************
  Function:        divoom_database_manage_update_eq_mode_index
  Description:     更新EQ的选择项
  Input:           table_name
                   clock_id
                   clock_pos
                   parent_id
                   mode
  Return:          none
**********************************************************************/

void divoom_database_manage_update_eq_mode_index(char *table_name,int clock_id, int clock_pos, int parent_id, int mode)
{
    int rc, pnRow, pnColumn, index = 0;
    char **dbResult;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

    divoom_database_manage_format_sql( "SELECT mode FROM %s WHERE clock_id=%d AND clock_pos=%d AND parent_id=%d LIMIT 1",table_name,clock_id, clock_pos, parent_id);
    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    if (rc != SQLITE_OK) {
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        pnRow = 0;
        divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
        return ;
    } 
    index = pnColumn;
    sqlite3_free_table(dbResult);
    if(pnRow > 0)
    {
        //更新
        divoom_database_manage_format_sql( "UPDATE %s SET mode=%d WHERE clock_id=%d AND clock_pos=%d AND parent_id=%d", table_name, mode, clock_id, clock_pos, parent_id);
        rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
        if (rc != SQLITE_OK) {
            divoom_print("Insert failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        } 
    }
    else
    {
        //插入
        divoom_database_manage_format_sql( "INSERT INTO %s(clock_id,clock_pos,parent_id,mode) VALUES(%d,%d,%d,%d)", table_name, clock_id, clock_pos, parent_id, mode);
        rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
        if (rc != SQLITE_OK) {
            divoom_print("Insert failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        } 
    }
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
}

/*********************************************************************
  Function:        divoom_database_manage_get_eq_image_list
  Description:     添加一个元素
  Input:           table_name
                   clock_id
                   clock_pos
                   parent_id
                   list_num
  Return:          list
**********************************************************************/

void *divoom_database_manage_get_eq_image_list(char *table_name,int clock_id, int clock_pos, int parent_id, int *list_num)
{
    int rc, pnRow, pnColumn, index, update_time = 0;
    char **dbResult;
    Pdivoom_eq_image_manage_image_item plist = NULL;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

    divoom_database_manage_format_sql( "SELECT id,author_id,image_name,file_addr FROM %s WHERE clock_id=%d AND clock_pos=%d AND parent_id=%d ORDER BY id",table_name,clock_id, clock_pos, parent_id);
    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        pnRow = 0;
        return NULL;
    } 
    index = pnColumn;
    *list_num = pnRow;
    if(pnRow > 0)
    {
        plist = divoom_system_malloc(sizeof(divoom_eq_image_manage_image_item) * pnRow);
        for(int i = 0; i < pnRow; i++)
        {
            plist[i].image_id = atoi(dbResult[index]);
            index ++;
            plist[i].user_id = atoi(dbResult[index]);
            index ++;
            strcpy(plist[i].image_name, dbResult[index]);
            index ++;
            strcpy(plist[i].file_id, dbResult[index]);
            index ++;
        }
    }
    sqlite3_free_table(dbResult);
    return plist;
}


/*********************************************************************
  Function:        divoom_database_manage_get_divoom_image_image_type_list
  Description:     得到所有的image_type列表
  Input:           table_name
                   list_num
  Return:          list
**********************************************************************/

int *divoom_database_manage_get_divoom_image_image_type_list(char *table_name, int *list_num)
{
    int rc, pnRow, pnColumn, index, update_time = 0;
    char **dbResult;
    int * plist = NULL;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

    divoom_database_manage_format_sql( "SELECT image_type FROM %s ",table_name);
    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        pnRow = 0;
        return NULL;
    } 
    index = pnColumn;
    *list_num = pnRow;
    if(pnRow > 0)
    {
        plist = divoom_system_malloc(sizeof(int) * pnRow);
        for(int i = 0; i < pnRow; i++)
        {
            plist[i] = atoi(dbResult[index]);
            index ++;
        }
    }
    sqlite3_free_table(dbResult);
    return plist;
}

/*********************************************************************
  Function:        divoom_database_manage_check_divoom_image_config_exist
  Description:     检查这个配置项，
  Input:           image_id
                   file_addr
  Return:          1 or 0
**********************************************************************/

int divoom_database_manage_check_divoom_image_config_exist(char *table_name, int clock_id, int clock_pos, int parent_id, int image_type)
{
    int ret = 0, rc, pnRow, pnColumn;
    char **dbResult;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

    divoom_database_manage_format_sql( "SELECT image_type FROM %s WHERE clock_id=%d AND clock_pos=%d AND parent_id=%d AND image_type=%d limit 1",table_name, clock_id, clock_pos, parent_id, image_type);
    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return 0;
    } 
    if(pnRow > 0)
    {
        ret = 1;
    }
    sqlite3_free_table(dbResult);
    return ret;
}

/*********************************************************************
  Function:        divoom_database_manage_get_divoom_image_type_id
  Description:     得到image_type，
  Input:           image_id
                   file_addr
  Return:          1 or 0
**********************************************************************/

int divoom_database_manage_get_divoom_image_type_id(char *table_name, int clock_id, int clock_pos, int parent_id)
{
    int ret = 0, rc, pnRow, pnColumn, image_type = 0;
    char **dbResult;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

    divoom_database_manage_format_sql( "SELECT image_type FROM %s WHERE clock_id=%d AND clock_pos=%d AND parent_id=%d limit 1",table_name, clock_id, clock_pos, parent_id);
    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return 0;
    } 
    if(pnRow > 0)
    {
        image_type = atoi(dbResult[1]);
    }
    sqlite3_free_table(dbResult);
    return image_type;
}

/*********************************************************************
  Function:        divoom_database_manage_get_divoom_image_update_tiem
  Description:     得到最后更新时间，
  Input:           image_id
                   image_type
  Return:          update_time
**********************************************************************/

int divoom_database_manage_get_divoom_image_update_tiem(char *table_name, int image_type)
{
    int ret = 0, rc, pnRow, pnColumn, update_time = 0;
    char **dbResult;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

    divoom_database_manage_format_sql( "SELECT update_time FROM %s WHERE image_type=%d limit 1",table_name,image_type);
    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return 0;
    } 
    if(pnRow > 0)
    {
        update_time = atoi(dbResult[1]);
    }
    sqlite3_free_table(dbResult);
    return update_time;
}

/*********************************************************************
  Function:        divoom_database_manage_check_divoom_image_config_exist
  Description:     检查这个配置项，
  Input:           image_id
                   file_addr
  Return:          1 or 0
**********************************************************************/

int divoom_database_manage_check_divoom_image_type_exist(char *table_name, int image_type)
{
    int ret = 0, rc, pnRow, pnColumn;
    char **dbResult;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

    divoom_database_manage_format_sql( "SELECT image_type FROM %s WHERE image_type=%d limit 1",table_name,image_type);
    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return 0;
    } 
    if(pnRow > 0)
    {
        ret = 1;
    }
    sqlite3_free_table(dbResult);
    return ret;
}

/*********************************************************************
  Function:        divoom_database_manage_check_divoom_image_config_exist
  Description:     检查这个配置项，
  Input:           image_id
                   file_addr
  Return:          1 or 0
**********************************************************************/

int divoom_database_manage_add_divoom_image_config_item(char *table_name, int clock_id, int clock_pos, int parent_id, int image_type)
{
    char  add_key_str[128];
    char  dest_table_name[128];
    
    sprintf(add_key_str, "%d,%d,%d,%d", clock_id, clock_pos, parent_id, image_type);
    sprintf(dest_table_name, "%s(clock_id,clock_pos,parent_id,image_type)",table_name);
    
    divoom_database_manage_insert_new_item(dest_table_name, add_key_str);
    return 1;
}

/*********************************************************************
  Function:        divoom_database_manage_update_divoom_image_config_item
  Description:     更新自定义播放时间
  Input:           clock_id
                   clock_pos
                   parent_id
                   image_type
  Return:          none
**********************************************************************/

int divoom_database_manage_update_divoom_image_config_item(char *table_name, int clock_id, int clock_pos, int parent_id, int image_type)
{
    int rc;

    if(divoom_database_manage_check_divoom_image_config_exist(table_name, clock_id, clock_pos, parent_id, image_type))
    {
        divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
        
        
        divoom_database_manage_format_sql( "UPDATE %s SET image_type=%d WHERE clock_id=%d AND pos=%d AND parent_id=%d", table_name, image_type, clock_id, clock_pos, parent_id);
        
        rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
        if (rc != SQLITE_OK) {
            divoom_print("update failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str , sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        } 
        divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    }
    else
    {
        return divoom_database_manage_add_divoom_image_config_item(table_name, clock_id, clock_pos, parent_id, image_type);
    }
    return 1;
}

/*********************************************************************
  Function:        divoom_database_manage_update_divoom_image_config_update_time
  Description:     更新自定义播放时间
  Input:           clock_id
                   clock_pos
                   parent_id
                   image_type
  Return:          none
**********************************************************************/

int divoom_database_manage_update_divoom_image_config_update_time(char *table_name, int image_type,int update_time)
{
    int rc;

    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    
    
    divoom_database_manage_format_sql( "UPDATE %s SET update_time=%d WHERE image_type=%d", table_name,update_time, image_type);
    
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        divoom_print("update failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str , sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
    } 
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    return 1;
}

/*********************************************************************
  Function:        divoom_database_manage_clear_all_divoom_image
  Description:     清空某个配置项
  Input:           table_name
  Return:          1 or 0
**********************************************************************/

void divoom_database_manage_clear_all_divoom_image(char *table_name)
{
    int rc;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    divoom_database_manage_format_sql( "DELETE FROM %s ", table_name);
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        divoom_print("Delete failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
    } 
    
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
}

/*********************************************************************
  Function:        divoom_database_manage_delete_divoom_image_config_item
  Description:     删除某个配置项
  Input:           table_name
                   clock_id
                   clock_pos
                   parent_id
                   image_type
  Return:          1 or 0
**********************************************************************/

void divoom_database_manage_delete_divoom_image_config_item(char *table_name, int clock_id, int clock_pos, int parent_id, int image_type)
{
    int rc;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    divoom_database_manage_format_sql( "DELETE FROM %s WHERE clock_id=%d AND clock_pos=%d AND parent_id=%d AND image_type=%d", table_name, clock_id, clock_pos, parent_id, image_type);
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        divoom_print("Insert failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
    } 
    
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
}


/*********************************************************************
  Function:        divoom_database_manage_get_user_define_last_update_time
  Description:     获取自定义最后更新的时间
  Input:           none
  Return:          最后更新时间
**********************************************************************/

int divoom_database_manage_get_user_define_last_update_time(void)
{
    int rc, update_time;
    int pnRow, pnColumn;
    char **dbResult;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

    divoom_database_manage_format_sql( "SELECT update_time FROM %s ORDER BY update_time DESC limit 1",DIVOOM_USER_DEFINE_CONFIG_TABLE_NAME);
    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return 0;
    } 
    if(pnRow == 0)
    {
        update_time = 0;
    }
    else
    {
        update_time = atoi(dbResult[1]);
    }
    sqlite3_free_table(dbResult);
    return update_time;
}

/*********************************************************************
  Function:        divoom_database_manage_get_user_define_pic_list
  Description:     获取某个自定义的所有照片ID
  Input:           clock_id
                   clock_pos
                   parent_id
                   pic_num   相片数目
  Return:          照片列表ID
**********************************************************************/

Pdivoom_image_hot_item divoom_database_manage_get_user_define_pic_list(
    int clock_id, int clock_pos, int parent_id, int *pic_num,
    char **pvoice_list)
{
    int i, rc, pnRow, pnColumn, index;
    char *voice_list;
    Pdivoom_image_hot_item pic_list;
    char **dbResult;

    if(pic_num == NULL || pvoice_list == NULL)
    {
        return NULL;
    }
    *pic_num = 0;
    *pvoice_list = NULL;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

    divoom_database_manage_format_sql(
        "SELECT id,author_id,image_name,voice_addr FROM %s WHERE clock_id=%d AND pos=%d AND parent_id=%d ORDER BY id ",
        DIVOOM_USER_DEFINE_TABLE_NAME, clock_id, clock_pos, parent_id);
    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return NULL;
    } 
    
    *pic_num = pnRow;
    if(pnRow <= 0)
    {
        sqlite3_free_table(dbResult);
        return NULL;
    }
    if((uint32)pnRow >
        U32_MAX / (uint32)sizeof(divoom_image_hot_item) ||
        (uint32)pnRow > U32_MAX / 64)
    {
        *pic_num = 0;
        sqlite3_free_table(dbResult);
        return NULL;
    }
    pic_list = divoom_system_malloc(
        (uint32)pnRow * sizeof(divoom_image_hot_item));
    voice_list = divoom_system_malloc((uint32)pnRow * 64);
    if(pic_list == NULL || voice_list == NULL)
    {
        *pic_num = 0;
        if(pic_list != NULL)
        {
            divoom_system_free(pic_list);
        }
        if(voice_list != NULL)
        {
            divoom_system_free(voice_list);
        }
        sqlite3_free_table(dbResult);
        return NULL;
    }
    index = pnColumn;
    divoom_print("pic num:%d, %d\n", pnRow, pnColumn);
    for(i = 0; i < pnRow; i++)
    {
        pic_list[i].image_size = 0;
        pic_list[i].image_id = (uint32)atoi(dbResult[index]);
        index ++;
        pic_list[i].user_id = (uint32)atoi(dbResult[index]);
        index ++;
        pic_list[i].image_name =
            divoom_database_manage_copy_image_name(dbResult[index]);
        index ++;
        if(pic_list[i].image_name == NULL)
        {
            *pic_num = 0;
            divoom_database_manage_free_loaded_image_list(pic_list, i);
            divoom_system_free(voice_list);
            sqlite3_free_table(dbResult);
            return NULL;
        }
        if(dbResult[index] != NULL)
        {
            strncpy(&voice_list[i * 64], dbResult[index], 63);
        }
        index++;
    }
    sqlite3_free_table(dbResult);
    *pvoice_list = voice_list;
    return pic_list;
}

/*********************************************************************
  Function:        divoom_database_manage_get_user_define_pic_addr
  Description:     获取某个自定义的照片地址
  Input:           id
                   file_addr
  Return:          1 or 0
**********************************************************************/

int divoom_database_manage_get_user_define_pic_addr(int  id, char *file_addr)
{
    int ret = 0, rc, pnRow, pnColumn;
    uint32 *pic_list;
    char **dbResult;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

    divoom_database_manage_format_sql( "SELECT file_addr FROM %s WHERE id=%d",DIVOOM_USER_DEFINE_TABLE_NAME, id);
    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return 0;
    } 
    if(pnRow > 0)
    {
        if(file_addr)
        {
            strcpy(file_addr, dbResult[1]);
        }
        ret = 1;
    }
    sqlite3_free_table(dbResult);
    return ret;
}

/*********************************************************************
  Function:        divoom_database_manage_update_user_define_pic_addr
  Description:     更新一个资源地址
  Input:           clock_id
                   clock_pos
                   parent_id
                   id
                   file_addr
                   update_time
  Return:          1 or 0
**********************************************************************/

void divoom_database_manage_update_user_define_pic_addr(
    int clock_id, int clock_pos, int parent_id, int id, char *file_addr,
    int update_time, int author_id, char *image_name, char *voice_id)
{
    int rc;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    divoom_database_manage_format_sql(
        "UPDATE %s SET file_addr='%s',author_id=%d,image_name='%s',voice_addr='%s' WHERE id=%d",
        DIVOOM_USER_DEFINE_TABLE_NAME, file_addr, author_id, image_name,
        voice_id, id);
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        divoom_print("Insert failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
    } 
    
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    
    divoom_database_manage_update_user_define_update_time(clock_id, clock_pos, parent_id, update_time);
}

/*********************************************************************
  Function:        divoom_database_manage_delete_user_define_item
  Description:     删除某个资源
  Input:           clock_id
                   clock_pos
                   parent_id
                   id
                   update_time
  Return:          1 or 0
**********************************************************************/

void divoom_database_manage_delete_user_define_item(int  clock_id, int clock_pos, int parent_id, int  id, int update_time)
{
    int rc;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    divoom_database_manage_format_sql( "DELETE FROM %s WHERE id=%d", DIVOOM_USER_DEFINE_TABLE_NAME, id);
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        divoom_print("Insert failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
    } 
    
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    divoom_database_manage_update_user_define_update_time(clock_id, clock_pos, parent_id, update_time);
}

/*********************************************************************
  Function:        divoom_database_manage_insert_user_define_pic_item
  Description:     插入某个资源
  Input:           clock_id
                   clock_pos
                   parent_id
                   id
                   file_addr
                   update_time
  Return:          none
**********************************************************************/

void divoom_database_manage_insert_user_define_pic_item(
    int clock_id, int clock_pos, int parent_id, int id, char *file_addr,
    int update_time, int author_id, char *image_name, char *voice_id)
{
    int rc;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    divoom_database_manage_format_sql(
        "INSERT INTO %s(id,parent_id,pos,clock_id,author_id,image_name,file_addr,voice_addr) VALUES(%d,%d,%d,%d,%d,'%s','%s','%s')",
        DIVOOM_USER_DEFINE_TABLE_NAME, id, parent_id, clock_pos, clock_id,
        author_id, image_name, file_addr, voice_id);
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        divoom_print("Insert failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
    } 
    
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    
    divoom_database_manage_update_user_define_update_time(clock_id, clock_pos, parent_id, update_time);
}

/*********************************************************************
  Function:        divoom_database_manage_empty_user_define_item
  Description:     删除某个表盘资源
  Input:           clock_id
                   clock_pos
                   parent_id
                   update_time
  Return:          1 or 0
**********************************************************************/

void divoom_database_manage_empty_user_define_item(int  clock_id, int clock_pos, int parent_id, int update_time)
{
    int rc;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    divoom_database_manage_format_sql( "DELETE FROM %s WHERE clock_id=%d AND pos=%d AND parent_id=%d", DIVOOM_USER_DEFINE_TABLE_NAME, clock_id, clock_pos, parent_id);
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        divoom_print("Insert failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
    } 
    
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    divoom_database_manage_update_user_define_update_time(clock_id, clock_pos, parent_id, update_time);
}

/*********************************************************************
  Function:        divoom_database_manage_check_user_define_config_record
  Description:     检查这个记录是否存在，不存在将插入
  Input:           clock_id
                   clock_pos
                   parent_id
  Return:          none
**********************************************************************/

void divoom_database_manage_check_user_define_config_record(int  clock_id, int clock_pos, int parent_id)
{
    int rc, pnRow, pnColumn;
    
    char **dbResult;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

    divoom_database_manage_format_sql( "SELECT play_time FROM %s WHERE clock_id=%d AND pos=%d AND parent_id=%d",DIVOOM_USER_DEFINE_CONFIG_TABLE_NAME, clock_id, clock_pos, parent_id);
    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return ;
    } 
    sqlite3_free_table(dbResult);
    
    /* sqlite3_get_table 在零记录时仍会返回列数，必须用行数判断。 */
    if(pnRow == 0)
    {
        divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
        
        divoom_database_manage_format_sql( "INSERT INTO %s(parent_id,pos,clock_id) VALUES(%d,%d,%d)", DIVOOM_USER_DEFINE_CONFIG_TABLE_NAME, parent_id, clock_pos, clock_id);
        rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
        
        
        divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    }
}

/*********************************************************************
  Function:        divoom_database_manage_update_user_define_play_time
  Description:     更新自定义播放时间
  Input:           clock_id
                   clock_pos
                   parent_id
                   play_time
  Return:          none
**********************************************************************/

void divoom_database_manage_update_user_define_play_time(int clock_id, int clock_pos, int parent_id, int play_time, int show_time)
{
    int rc;
    
    divoom_database_manage_check_user_define_config_record(clock_id, clock_pos, parent_id);
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    
    divoom_database_manage_format_sql( "UPDATE %s SET play_time=%d,show_time=%d WHERE clock_id=%d AND pos=%d AND parent_id=%d", DIVOOM_USER_DEFINE_CONFIG_TABLE_NAME, play_time, show_time, clock_id, clock_pos, parent_id);
    
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        divoom_print("update failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str , sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
    } 
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
}

/*********************************************************************
  Function:        divoom_database_manage_update_user_define_play_time
  Description:     更新自定义播放时间
  Input:           clock_id
                   clock_pos
                   parent_id
                   play_time
  Return:          none
**********************************************************************/

void divoom_database_manage_update_user_define_update_time(int  clock_id, int clock_pos, int parent_id, int update_time)
{
    int rc;

    divoom_database_manage_check_user_define_config_record(clock_id, clock_pos, parent_id);
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    
    
    divoom_database_manage_format_sql( "UPDATE %s SET update_time=%d WHERE clock_id=%d AND pos=%d AND parent_id=%d", DIVOOM_USER_DEFINE_CONFIG_TABLE_NAME, update_time, clock_id, clock_pos, parent_id);
    
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        divoom_print("update failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str , sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
    } 
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
}

/*********************************************************************
  Function:        divoom_database_manage_get_user_define_play_time
  Description:     获取自定义播放时间
  Input:           clock_id
                   clock_pos
                   parent_id
  Return:          1 or 0
**********************************************************************/

int divoom_database_manage_get_user_define_play_time(int clock_id, int clock_pos, int parent_id, int *show_time)
{
    int rc, pnRow, pnColumn, play_time;
    
    char **dbResult;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

    divoom_database_manage_format_sql( "SELECT play_time,show_time FROM %s WHERE clock_id=%d AND pos=%d AND parent_id=%d",DIVOOM_USER_DEFINE_CONFIG_TABLE_NAME, clock_id, clock_pos, parent_id);
    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return 0;
    } 
    if(pnRow == 0)
    {
        play_time = 0;
        *show_time = 0;
    }
    else
    {
        play_time =  atoi(dbResult[pnColumn]);
        *show_time =  atoi(dbResult[pnColumn+1]);
    }
    sqlite3_free_table(dbResult);
    return play_time;
}

/*********************************************************************
  Function:        divoom_database_manage_get_all_album_info
  Description:     得到所有的相簿相关信息
  Input:           album_id
                   pic_num   相片数目
                   order_flag   1:按拍摄时间， 按照片时间
                   desc_flag    1:升序与降序
  Return:          none
**********************************************************************/

Pdivoom_pic_album_manage_head_item divoom_database_manage_get_all_album_info(int *album_num)
{
    int i, j, rc, pnRow, pnColumn, index;
    Pdivoom_pic_album_manage_head_item palbum_list;
    char **dbResult = NULL;

    if(album_num == NULL)
    {
        return NULL;
    }
    *album_num = 0;
    if(!divoom_db_is_ready())
    {
        return NULL;
    }
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, "SELECT * FROM album_head", 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        sqlite3_free_table(dbResult);
        return NULL;
    }

    if(pnRow == 0)
    {
        sqlite3_free_table(dbResult);
        return NULL;
    }
    palbum_list = divoom_system_malloc(sizeof(divoom_pic_album_manage_head_item)*pnRow);
    if(palbum_list == NULL)
    {
        sqlite3_free_table(dbResult);
        return NULL;
    }
    index = pnColumn;
    for(i = 0; i < pnRow; i++)
    {
        palbum_list[i].id = atoi(dbResult[index]);
        palbum_list[i].type = atoi(dbResult[index  + 1]);
        strcpy(palbum_list[i].album_name, dbResult[index  + 2]);
        index += pnColumn;
    }
    sqlite3_free_table(dbResult);
    *album_num = pnRow;
    return palbum_list;
}

/*********************************************************************
  Function:        divoom_database_manage_get_pic_info
  Description:     得到照片相关信息
  Input:           pic_id
                   pic_info
  Return:          none
**********************************************************************/

int divoom_database_manage_get_pic_info(int pic_id, void *para)
{
    int rc, pnRow, pnColumn, index;
    char **dbResult;
    Pdivoom_pic_phone_manage_item_offset pic_info = (Pdivoom_pic_phone_manage_item_offset)para;
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

//    id(key),save_time(order),photo_time(order),,like_flag(order),hide_flag(order),fill_flag,mirror_flag,rotate_flag,pic_video_flag,pathname
    divoom_database_manage_format_sql( "SELECT id, save_time, photo_time,like_flag,hide_flag,fill_flag,mirror_flag,rotate_flag,pic_video_flag,width_flag FROM pic_info WHERE id = %d", pic_id);
    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return 0;
    } 
    
    /*
    
    "save_time INTEGER KEY DEFAULT 0," 
    "photo_time INTEGER KEY DEFAULT 0, "
    "like_time INTEGER KEY DEFAULT 0, "
    "hide_time INTEGER KEY DEFAULT 0, "
    "like_flag INTEGER KEY DEFAULT 0, "
    "hide_flag INTEGER KEY DEFAULT 0, "
    "fill_flag INTEGER DEFAULT 0, "
    "mirror_flag INTEGER DEFAULT 0, "
    "rotate_flag INTEGER DEFAULT 0, "
    "pic_video_flag INTEGER DEFAULT 0, "
    */
    index = pnColumn;
    if(pnRow >= 1)
    {
        pic_info->id = atoi(dbResult[index]);
        index ++;
        pic_info->save_time = atof(dbResult[index]);
        index ++;
        pic_info->photo_time = atoi(dbResult[index]);
        index ++;
        pic_info->like_flag = atoi(dbResult[index]);
        index ++;
        pic_info->hide_flag = atoi(dbResult[index]);
        index ++;
        pic_info->fill_flag = atoi(dbResult[index]);
        index ++;
        pic_info->mirror_flag = atoi(dbResult[index]);
        index ++;
        pic_info->rotate_flag = atoi(dbResult[index]);
        index ++;
        pic_info->pic_video_flag = atoi(dbResult[index]);
        index ++;
        pic_info->width_flag = atoi(dbResult[index]);
    }
    sqlite3_free_table(dbResult);

    return pnRow > 0;
}

/*********************************************************************
  Function:        divoom_database_manage_get_pic_path_name
  Description:     得到照片路径信息
  Input:           pic_id
                   path_name
  Return:          result
**********************************************************************/

int divoom_database_manage_get_pic_path_name(int pic_id, char *path_name)
{
    int i, j, rc, pnRow, pnColumn, index;
    char **dbResult;
divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

//    id(key),save_time(order),photo_time(order),,like_flag(order),hide_flag(order),fill_flag,mirror_flag,rotate_flag,pic_video_flag,pathname
    divoom_database_manage_format_sql( "SELECT path_name FROM pic_info WHERE id = %d", pic_id);
    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return 0;
    }
    index = pnColumn;
    if(pnRow >= 1)
    {
        strcpy(path_name, dbResult[index]);
    }
    else
    {
        path_name[0] = 0;
    }
    sqlite3_free_table(dbResult);
    return 1;
}

/*********************************************************************
  Function:        divoom_database_manage_get_pic_id_from_path_name
  Description:     得到照片ID
  Input:           
                   path_name
  Return:          pic_id
**********************************************************************/

int divoom_database_manage_get_pic_id_from_path_name(char *path_name)
{
    int i, j, rc, pnRow, pnColumn, index, pic_id = 0;
    char **dbResult;
divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

//    id(key),save_time(order),photo_time(order),,like_flag(order),hide_flag(order),fill_flag,mirror_flag,rotate_flag,pic_video_flag,pathname
    divoom_database_manage_format_sql( "SELECT id FROM pic_info WHERE path_name = '%s'", path_name);
    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return 0;
    }
    index = pnColumn;
    if(pnRow >= 1)
    {
        pic_id = atoi(dbResult[index]);
    }
    sqlite3_free_table(dbResult);
    return pic_id;
}

/*********************************************************************
  Function:        divoom_database_manage_get_pic_title
  Description:     得到照片标题
  Input:           pic_id
                   pic_title
  Return:          result
**********************************************************************/

int divoom_database_manage_get_pic_title(int pic_id, char *pic_title)
{
    int i, j, rc, pnRow, pnColumn, index;
    char **dbResult;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

//    id(key),save_time(order),photo_time(order),,like_flag(order),hide_flag(order),fill_flag,mirror_flag,rotate_flag,pic_video_flag,pathname
    divoom_database_manage_format_sql( "SELECT pic_title FROM pic_info WHERE id = %d", pic_id);
    rc = sqlite3_get_table(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, 
        &dbResult, &pnRow, &pnColumn, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("get failed: %s ;%s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
        return 0;
    } 
    
    index = pnColumn;
    if(pnRow >= 1)
    {
        strcpy(pic_title, dbResult[index]);
    }
    else
    {
        pic_title[0] = 0;
    }
    sqlite3_free_table(dbResult);
    return 1;
}

int divoom_database_manage_add_pic_record_finish_callback(void* para,int num,char **msg1,char **msg2)
{
    divoom_system_put_sem(para);
    return 1;
}

/*********************************************************************
  Function:        divoom_database_manage_add_pic_record
  Description:     增加一张照片信息
  Input:           pic_info
  Return:          返回照片ID
**********************************************************************/

int divoom_database_manage_add_pic_record(double save_time, int photo_time, char  *path_name, int photo_x, int photo_y, int user_id,char *pic_path, int video_flag, int width_flag)
{
    static const char insert_sql[] =
        "INSERT INTO pic_info(save_time, photo_time, path_name, photo_x, photo_y,"
        "user_id,pic_video_flag,width_flag,pic_title) VALUES(?,?,?,?,?,?,?,?,?)";
    sqlite3_stmt *stmt = NULL;
    int rc;
    int id = 0;

    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

    rc = sqlite3_prepare_v2(pdivoom_database_manage_handle->db_handle,
                            insert_sql, -1, &stmt, NULL);
    if (rc == SQLITE_OK)
        rc = sqlite3_bind_int64(stmt, 1, (sqlite3_int64)save_time);
    if (rc == SQLITE_OK)
        rc = sqlite3_bind_int(stmt, 2, photo_time);
    if (rc == SQLITE_OK)
        rc = sqlite3_bind_text(stmt, 3, path_name ? path_name : "",
                               -1, SQLITE_TRANSIENT);
    if (rc == SQLITE_OK)
        rc = sqlite3_bind_int(stmt, 4, photo_x);
    if (rc == SQLITE_OK)
        rc = sqlite3_bind_int(stmt, 5, photo_y);
    if (rc == SQLITE_OK)
        rc = sqlite3_bind_int(stmt, 6, user_id);
    if (rc == SQLITE_OK)
        rc = sqlite3_bind_int(stmt, 7, video_flag);
    if (rc == SQLITE_OK)
        rc = sqlite3_bind_int(stmt, 8, width_flag);
    if (rc == SQLITE_OK)
        rc = sqlite3_bind_text(stmt, 9, pic_path ? pic_path : "",
                               -1, SQLITE_TRANSIENT);
    if (rc == SQLITE_OK)
        rc = sqlite3_step(stmt);

    if (rc == SQLITE_DONE) {
        id = (int)sqlite3_last_insert_rowid(
            pdivoom_database_manage_handle->db_handle);
        divoom_print("insert pic ok: id=%d, path=%s\n",
                     id, path_name ? path_name : "");
    } else {
        divoom_print("Insert pic failed: path=%s ; %s\n",
                     path_name ? path_name : "",
                     sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
    }

    if (stmt != NULL)
        sqlite3_finalize(stmt);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    return id;
}


/*********************************************************************
  Function:        divoom_database_manage_update_pic_title
  Description:     修改照片标题
  Input:           pic_info
  Return:          返回照片ID
**********************************************************************/

void divoom_database_manage_update_pic_title(int pic_id, char *pic_title)
{
    int rc;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    divoom_database_manage_format_sql( "UPDATE pic_info SET pic_title='%s' WHERE id=%d", pic_title, pic_id);
    
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("update failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str , sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
    } 
    
}

/*********************************************************************
  Function:        divoom_database_manage_update_pic_attr
  Description:     修改照片属性
  Input:           pic_info
  Return:          返回照片ID
**********************************************************************/

void divoom_database_manage_update_pic_attr(Pdivoom_pic_phone_manage_item_offset pic_info, int attr_flag)
{
    int rc;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    switch(attr_flag)
    {
        case DIVOOM_PIC_PHONE_ATTR_ROTATE_FLAG:
            divoom_database_manage_format_sql( "UPDATE pic_info SET rotate_flag=%d WHERE id=%d", pic_info->rotate_flag, pic_info->id);
            break;
            
        case DIVOOM_PIC_PHONE_ATTR_MIRROR_FLAG:
            divoom_database_manage_format_sql( "UPDATE pic_info SET mirror_flag=%d WHERE id=%d", pic_info->mirror_flag, pic_info->id);
            break;
            
        case DIVOOM_PIC_PHONE_ATTR_FILL_FLAG:
            divoom_database_manage_format_sql( "UPDATE pic_info SET fill_flag=%d WHERE id=%d", pic_info->fill_flag, pic_info->id);
            break;
            
        case DIVOOM_PIC_PHONE_ATTR_HIDE_FLAG:
            pic_info->hide_time = time(0);
            divoom_database_manage_format_sql( "UPDATE pic_info SET hide_flag=%d,hide_time=%d WHERE id=%d", pic_info->hide_flag, pic_info->hide_time,pic_info->id);
            break;
            
        case DIVOOM_PIC_PHONE_ATTR_LIKE_FLAG:
            pic_info->like_time = time(0);
            divoom_database_manage_format_sql( "UPDATE pic_info SET like_flag=%d, like_time = %d WHERE id=%d", pic_info->like_flag, pic_info->like_time,pic_info->id);
            break;
            
    }
    
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    if (rc != SQLITE_OK) {
        divoom_print("update failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str , sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
    } 
    
}

/*********************************************************************
  Function:        divoom_database_manage_delete_pic
  Description:     删除照片
  Input:           pic_id
  Return:          none
**********************************************************************/

void divoom_database_manage_delete_pic(int pic_id)
{
    int rc;
    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);

    //要先删除照片数据库，再删除和这个照片关联的相簿信息
    divoom_database_manage_format_sql( "DELETE FROM pic_info WHERE id=%d", pic_id);
    
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        divoom_print("delete failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
    } 
    
    
    divoom_database_manage_format_sql( "DELETE FROM album_pic WHERE pic_id=%d", pic_id);
    
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, pdivoom_database_manage_handle->sql_str, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        divoom_print("delete failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
    } 
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    

}

/*********************************************************************
  Function:        divoom_database_manage_reset_pic_info
  Description:     照片数据全部丢失，将删除两个表的内容
  Input:           none
  Return:          none
**********************************************************************/

void divoom_database_manage_reset_pic_info(void)
{
    int rc;

    
    divoom_system_get_lock(pdivoom_database_manage_handle->lock_handle);
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, "DELETE FROM pic_info", NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        divoom_print("delete failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
    } 
    
    
    rc = sqlite3_exec(pdivoom_database_manage_handle->db_handle, "DELETE FROM album_pic ", NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        divoom_print("delete failed: %s ; %s\n", pdivoom_database_manage_handle->sql_str, sqlite3_errmsg(pdivoom_database_manage_handle->db_handle));
    } 
    divoom_system_free_lock(pdivoom_database_manage_handle->lock_handle);
    

}
#endif 
