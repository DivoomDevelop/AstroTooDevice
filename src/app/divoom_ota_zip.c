#include "divoom.h"
#include "divoom_ota_zip.h"
#include "divoom_ota_rtos.h"
#include "divoom_port_fs.h"

#include <string.h>

#if !DIVOOM_BUILD_SIMULATOR
#include "miniz.h"
#endif

int divoom_ota_is_zip_file(const char *path)
{
	unsigned char hdr[4];
	void *fp;
	size_t n;

#if DIVOOM_BUILD_SIMULATOR
	(void)path;
	return 0;
#else
	if (!path || !path[0])
		return 0;
	if (!divoom_port_file_exists(path))
		return 0;

	fp = divoom_port_file_open(path, "rb");
	if (!fp)
		return 0;

	n = divoom_port_file_read(hdr, 1, sizeof(hdr), fp);
	divoom_port_file_close(fp);

	if (n != sizeof(hdr))
		return 0;

	return (hdr[0] == DIVOOM_OTA_ZIP_MAGIC0 && hdr[1] == DIVOOM_OTA_ZIP_MAGIC1 &&
		hdr[2] == DIVOOM_OTA_ZIP_MAGIC2 && hdr[3] == DIVOOM_OTA_ZIP_MAGIC3) ? 1 : 0;
#endif
}

static int divoom_ota_zip_member_name_ok(const char *name)
{
	const char *slash;

	if (!name || !name[0])
		return 0;

	slash = strrchr(name, '/');
	if (slash)
		name = slash + 1;

	return (strcmp(name, DIVOOM_OTA_RTOS_PACKAGE_NAME) == 0) ? 1 : 0;
}

#if !DIVOOM_BUILD_SIMULATOR
typedef struct {
	void *fp;
	mz_uint64 uncomp_size;
	mz_uint64 written;
	divoom_ota_zip_progress_fn progress;
	void *progress_ud;
	int last_percent;
} divoom_ota_zip_write_ctx;

static void divoom_ota_zip_report_progress(divoom_ota_zip_write_ctx *ctx, int percent)
{
	if (!ctx || !ctx->progress)
		return;
	if (percent < 0)
		percent = 0;
	if (percent > 100)
		percent = 100;
	if (percent == ctx->last_percent)
		return;
	ctx->last_percent = percent;
	ctx->progress(percent, ctx->progress_ud);
}

static size_t divoom_ota_zip_file_write_progress(void *pOpaque, mz_uint64 ofs,
						 const void *pBuf, size_t n)
{
	divoom_ota_zip_write_ctx *ctx = (divoom_ota_zip_write_ctx *)pOpaque;
	size_t wr;
	int percent;

	(void)ofs;
	if (!ctx || !ctx->fp || !pBuf || n == 0)
		return 0;

	wr = divoom_port_file_write(pBuf, 1, n, ctx->fp);
	if (wr == 0)
		return 0;

	ctx->written += (mz_uint64)wr;
	if (ctx->uncomp_size > 0) {
		percent = (int)((ctx->written * 100ULL) / ctx->uncomp_size);
		divoom_ota_zip_report_progress(ctx, percent);
	}
	return wr;
}
#endif

int divoom_ota_zip_extract_fex(const char *zip_path, const char *out_fex_path,
			       divoom_ota_zip_progress_fn progress, void *progress_ud)
{
#if DIVOOM_BUILD_SIMULATOR
	(void)zip_path;
	(void)out_fex_path;
	(void)progress;
	(void)progress_ud;
	return -1;
#else
	mz_zip_archive zip;
	mz_uint file_index;
	mz_zip_archive_file_stat stat;
	divoom_ota_zip_write_ctx write_ctx;
	void *fp = NULL;
	mz_bool ok;
	int ret = -1;

	if (!zip_path || !out_fex_path || !zip_path[0] || !out_fex_path[0])
		return -1;

	memset(&zip, 0, sizeof(zip));
	if (!mz_zip_reader_init_file(&zip, zip_path, 0)) {
		divoom_print("ota_zip: open fail %s err=%d\n", zip_path,
			     (int)mz_zip_get_last_error(&zip));
		return -1;
	}

	file_index = mz_zip_reader_locate_file(&zip, DIVOOM_OTA_RTOS_PACKAGE_NAME, NULL, 0);
	if (file_index == (mz_uint)-1) {
		divoom_print("ota_zip: member %s not found in %s\n",
			     DIVOOM_OTA_RTOS_PACKAGE_NAME, zip_path);
		goto cleanup_zip;
	}

	if (!mz_zip_reader_file_stat(&zip, file_index, &stat)) {
		divoom_print("ota_zip: file_stat fail\n");
		goto cleanup_zip;
	}

	if (!divoom_ota_zip_member_name_ok(stat.m_filename)) {
		divoom_print("ota_zip: reject unsafe member name %s\n", stat.m_filename);
		goto cleanup_zip;
	}

	if (stat.m_is_directory || stat.m_is_encrypted || !stat.m_is_supported) {
		divoom_print("ota_zip: unsupported entry dir=%d enc=%d sup=%d method=%u\n",
			     (int)stat.m_is_directory, (int)stat.m_is_encrypted,
			     (int)stat.m_is_supported, (unsigned)stat.m_method);
		goto cleanup_zip;
	}

	divoom_print("ota_zip: extract %s method=%u comp=%llu uncomp=%llu\n",
		     stat.m_filename, (unsigned)stat.m_method,
		     (unsigned long long)stat.m_comp_size,
		     (unsigned long long)stat.m_uncomp_size);

	divoom_port_remove_file(out_fex_path);
	fp = divoom_port_file_open(out_fex_path, "wb");
	if (!fp) {
		divoom_print("ota_zip: open out fail %s\n", out_fex_path);
		goto cleanup_zip;
	}

	memset(&write_ctx, 0, sizeof(write_ctx));
	write_ctx.fp = fp;
	write_ctx.uncomp_size = stat.m_uncomp_size;
	write_ctx.progress = progress;
	write_ctx.progress_ud = progress_ud;
	write_ctx.last_percent = -1;
	divoom_ota_zip_report_progress(&write_ctx, 0);

	ok = mz_zip_reader_extract_to_callback(&zip, file_index,
					       divoom_ota_zip_file_write_progress,
					       &write_ctx, 0);
	divoom_port_file_close(fp);
	fp = NULL;

	if (!ok || write_ctx.written != stat.m_uncomp_size) {
		divoom_print("ota_zip: extract fail err=%d method=%u written=%llu uncomp=%llu\n",
			     (int)mz_zip_get_last_error(&zip), (unsigned)stat.m_method,
			     (unsigned long long)write_ctx.written,
			     (unsigned long long)stat.m_uncomp_size);
		divoom_port_remove_file(out_fex_path);
		goto cleanup_zip;
	}

	divoom_port_sync();
	divoom_ota_zip_report_progress(&write_ctx, 100);

	divoom_print("ota_zip: extracted %s -> %s (%llu bytes)\n",
		     DIVOOM_OTA_RTOS_PACKAGE_NAME, out_fex_path,
		     (unsigned long long)stat.m_uncomp_size);
	ret = 0;

cleanup_zip:
	if (fp)
		divoom_port_file_close(fp);
	mz_zip_reader_end(&zip);
	return ret;
#endif
}
