#if !defined(_WIN32)
#include <sys/errno.h>
#endif
#include "divoom_port_fs.h"
#include "divoom_malloc.h"

#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef DIVOOM_PORT_DIR_CHECKSUM_READ_CHUNK
#define DIVOOM_PORT_DIR_CHECKSUM_READ_CHUNK (32U * 1024U)
#endif

#ifndef DIVOOM_PORT_DIR_CHECKSUM_PATH_MAX
#define DIVOOM_PORT_DIR_CHECKSUM_PATH_MAX 512U
#endif

#ifndef DIVOOM_PORT_DIR_CHECKSUM_MAX_MANIFEST
#define DIVOOM_PORT_DIR_CHECKSUM_MAX_MANIFEST (4U * 1024U * 1024U)
#endif

#ifndef DIVOOM_PORT_DIR_CHECKSUM_MAX_NODES
#define DIVOOM_PORT_DIR_CHECKSUM_MAX_NODES 100000U
#endif

#ifndef DIVOOM_PORT_DIR_CHECKSUM_MAX_DEPTH
#define DIVOOM_PORT_DIR_CHECKSUM_MAX_DEPTH 64U
#endif

#define DCRC_FORMAT_VERSION 1U
#define DCRC_ALGORITHM_CRC32 1U
#define DCRC_NODE_FILE 1U
#define DCRC_NODE_DIRECTORY 2U
#define DCRC_HEADER_SIZE 6U
#define DCRC_TRAILER_SIZE 4U

static const unsigned char g_dcrc_magic[4] = {'D', 'C', 'R', 'C'};
static const unsigned char g_dcrc_directory_prefix[] = {
	'D', 'C', 'R', 'C', '-', 'D', 'I', 'R', 1
};

typedef struct dcrc_live_node dcrc_live_node_t;

struct dcrc_live_node {
	uint64_t size;
	uint32_t crc32;
	uint32_t file_count;
	uint32_t child_count;
	uint32_t name_len;
	unsigned char type;
	dcrc_live_node_t **children;
	char name[1];
};

typedef struct dcrc_scan_entry {
	char *name;
} dcrc_scan_entry_t;

typedef struct dcrc_reader {
	const unsigned char *data;
	size_t size;
	size_t offset;
	uint32_t node_count;
} dcrc_reader_t;

typedef struct dcrc_manifest_node {
	const unsigned char *name;
	size_t children_offset;
	size_t end_offset;
	uint64_t size;
	uint32_t crc32;
	uint32_t file_count;
	uint32_t child_count;
	uint32_t name_len;
	unsigned char type;
} dcrc_manifest_node_t;

typedef struct dcrc_context {
	uint32_t crc_table[8][256];
	unsigned char *read_buffer;
	size_t read_buffer_size;
    int profile_scan;
    size_t io_chunk_size;
    uint32_t checked_files;
    uint64_t checked_bytes, handle_us, open_us, read_us, crc_us, close_us;
	uint32_t live_node_count;
	divoom_port_dir_change_cb callback;
	void *user_data;
	divoom_port_dir_compare_result_t *result;
	char callback_path[DIVOOM_PORT_DIR_CHECKSUM_PATH_MAX];
	size_t callback_path_len;
    int include_directories;
} dcrc_context_t;

static void *dcrc_alloc(size_t size)
{
	if (size == 0U)
		size = 1U;
	if (size > UINT32_MAX)
		return NULL;
	return divoom_malloc_raw((uint32)size, 1, __LINE__, __FILE__, 0);
}

static void dcrc_free(void *ptr)
{
	if (ptr)
		divoom_system_free(ptr);
}

static void dcrc_crc32_init(dcrc_context_t *ctx)
{
    uint32_t i;
    unsigned int slice;
    for (i = 0; i < 256U; i++) {
        uint32_t value = i;
        unsigned int bit;
        for (bit = 0; bit < 8U; bit++)
            value = (value >> 1) ^ ((value & 1U) ? 0xEDB88320U : 0U);
        ctx->crc_table[0][i] = value;
    }
    for (slice = 1; slice < 8; ++slice) {
        for (i = 0; i < 256U; ++i) {
            uint32_t value = ctx->crc_table[slice - 1][i];
            ctx->crc_table[slice][i] = (value >> 8) ^ ctx->crc_table[0][value & 0xFFU];
        }
    }
}

static uint32_t dcrc_crc32_update(dcrc_context_t *ctx, uint32_t crc,
    const void *data, size_t size)
{
    const unsigned char *p = data;
    crc ^= 0xFFFFFFFFU;
    /* Eight independent table lookups replace the serial byte-at-a-time chain.
     * Explicit little-endian loads also support unaligned buffers and hosts. */
    while (size >= 8U) {
        crc ^= (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
            ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
        crc = ctx->crc_table[7][crc & 0xFFU] ^
            ctx->crc_table[6][(crc >> 8) & 0xFFU] ^
            ctx->crc_table[5][(crc >> 16) & 0xFFU] ^
            ctx->crc_table[4][crc >> 24] ^
            ctx->crc_table[3][p[4]] ^ ctx->crc_table[2][p[5]] ^
            ctx->crc_table[1][p[6]] ^ ctx->crc_table[0][p[7]];
        p += 8; size -= 8U;
    }
    while (size-- > 0U)
        crc = (crc >> 8) ^ ctx->crc_table[0][(crc ^ *p++) & 0xFFU];
    return crc ^ 0xFFFFFFFFU;
}

static size_t dcrc_encode_varint(uint64_t value, unsigned char out[10])
{
	size_t size = 0;

	do {
		unsigned char byte = (unsigned char)(value & 0x7FU);

		value >>= 7;
		if (value != 0U)
			byte |= 0x80U;
		out[size++] = byte;
	} while (value != 0U);
	return size;
}

static uint32_t dcrc_crc32_varint(dcrc_context_t *ctx, uint32_t crc,
	uint64_t value)
{
	unsigned char encoded[10];
	size_t size = dcrc_encode_varint(value, encoded);

	return dcrc_crc32_update(ctx, crc, encoded, size);
}

static uint32_t dcrc_get_le32(const unsigned char *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
		((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint32_t dcrc_crc32_le32(dcrc_context_t *ctx, uint32_t crc,
	uint32_t value)
{
	unsigned char encoded[4];

	encoded[0] = (unsigned char)value;
	encoded[1] = (unsigned char)(value >> 8);
	encoded[2] = (unsigned char)(value >> 16);
	encoded[3] = (unsigned char)(value >> 24);
	return dcrc_crc32_update(ctx, crc, encoded, sizeof(encoded));
}

static int dcrc_name_valid(const unsigned char *name, uint32_t name_len)
{
	uint32_t i;

	if (!name || name_len == 0U || name_len >= DIVOOM_PORT_DIR_CHECKSUM_PATH_MAX)
		return 0;
	if ((name_len == 1U && name[0] == '.') ||
	    (name_len == 2U && name[0] == '.' && name[1] == '.'))
		return 0;
	for (i = 0; i < name_len; i++) {
		if (name[i] == '\0' || name[i] == '/' || name[i] == '\\')
			return 0;
	}
	return 1;
}

static int dcrc_name_compare(const unsigned char *left, uint32_t left_len,
	const unsigned char *right, uint32_t right_len)
{
	size_t common = left_len < right_len ? left_len : right_len;
	int result = memcmp(left, right, common);

	if (result != 0)
		return result;
	if (left_len < right_len)
		return -1;
	if (left_len > right_len)
		return 1;
	return 0;
}

static int dcrc_reader_bytes(dcrc_reader_t *reader, size_t size,
	const unsigned char **out)
{
	if (reader->offset > reader->size ||
	    size > reader->size - reader->offset)
		return DIVOOM_PORT_DIR_COMPARE_ERR_FORMAT;
	*out = reader->data + reader->offset;
	reader->offset += size;
	return 0;
}

static int dcrc_reader_byte(dcrc_reader_t *reader, unsigned char *out)
{
	const unsigned char *value;
	int rc = dcrc_reader_bytes(reader, 1U, &value);

	if (rc != 0)
		return rc;
	*out = value[0];
	return 0;
}

static int dcrc_reader_varint(dcrc_reader_t *reader, uint64_t *out)
{
	uint64_t value = 0;
	unsigned int shift;

	for (shift = 0; shift <= 63U; shift += 7U) {
		unsigned char byte;
		int rc = dcrc_reader_byte(reader, &byte);

		if (rc != 0)
			return rc;
		if (shift == 63U && byte > 1U)
			return DIVOOM_PORT_DIR_COMPARE_ERR_FORMAT;
		value |= (uint64_t)(byte & 0x7FU) << shift;
		if ((byte & 0x80U) == 0U) {
			*out = value;
			return 0;
		}
	}
	return DIVOOM_PORT_DIR_COMPARE_ERR_FORMAT;
}

static uint32_t dcrc_manifest_descriptor_crc(dcrc_context_t *ctx,
	uint32_t crc, const dcrc_manifest_node_t *node)
{
	crc = dcrc_crc32_update(ctx, crc, &node->type, 1U);
	crc = dcrc_crc32_varint(ctx, crc, node->name_len);
	crc = dcrc_crc32_update(ctx, crc, node->name, node->name_len);
	crc = dcrc_crc32_varint(ctx, crc,
		node->type == DCRC_NODE_FILE ? node->size : node->file_count);
	return dcrc_crc32_le32(ctx, crc, node->crc32);
}

static int dcrc_parse_node(dcrc_context_t *ctx, dcrc_reader_t *reader,
	dcrc_manifest_node_t *node, uint32_t depth)
{
	const unsigned char *field;
	uint64_t value;
	int rc;

	if (depth > DIVOOM_PORT_DIR_CHECKSUM_MAX_DEPTH ||
	    ++reader->node_count > DIVOOM_PORT_DIR_CHECKSUM_MAX_NODES)
		return DIVOOM_PORT_DIR_COMPARE_ERR_FORMAT;
	memset(node, 0, sizeof(*node));
	rc = dcrc_reader_byte(reader, &node->type);
	if (rc != 0)
		return rc;
	rc = dcrc_reader_varint(reader, &value);
	if (rc != 0 || value > UINT32_MAX)
		return DIVOOM_PORT_DIR_COMPARE_ERR_FORMAT;
	node->name_len = (uint32_t)value;
	rc = dcrc_reader_bytes(reader, node->name_len, &node->name);
	if (rc != 0 || !dcrc_name_valid(node->name, node->name_len))
		return DIVOOM_PORT_DIR_COMPARE_ERR_FORMAT;
	if (node->type == DCRC_NODE_FILE) {
		rc = dcrc_reader_varint(reader, &node->size);
		if (rc != 0)
			return rc;
		rc = dcrc_reader_bytes(reader, 4U, &field);
		if (rc != 0)
			return rc;
		node->crc32 = dcrc_get_le32(field);
		node->end_offset = reader->offset;
		return 0;
	}
	if (node->type != DCRC_NODE_DIRECTORY)
		return DIVOOM_PORT_DIR_COMPARE_ERR_FORMAT;
	rc = dcrc_reader_varint(reader, &value);
	if (rc != 0 || value > UINT32_MAX)
		return DIVOOM_PORT_DIR_COMPARE_ERR_FORMAT;
	node->file_count = (uint32_t)value;
	rc = dcrc_reader_bytes(reader, 4U, &field);
	if (rc != 0)
		return rc;
	node->crc32 = dcrc_get_le32(field);
	rc = dcrc_reader_varint(reader, &value);
	if (rc != 0 || value > DIVOOM_PORT_DIR_CHECKSUM_MAX_NODES)
		return DIVOOM_PORT_DIR_COMPARE_ERR_FORMAT;
	node->child_count = (uint32_t)value;
	node->children_offset = reader->offset;
	{
		const unsigned char *previous_name = NULL;
		uint32_t previous_name_len = 0;
		uint32_t computed_count = 0;
		uint32_t computed_crc;
		uint32_t i;

		computed_crc = dcrc_crc32_update(ctx, 0U, g_dcrc_directory_prefix,
			sizeof(g_dcrc_directory_prefix));
		computed_crc = dcrc_crc32_varint(ctx, computed_crc, node->child_count);
		for (i = 0; i < node->child_count; i++) {
			dcrc_manifest_node_t child;

			rc = dcrc_parse_node(ctx, reader, &child, depth + 1U);
			if (rc != 0)
				return rc;
			if (previous_name && dcrc_name_compare(previous_name,
				previous_name_len, child.name, child.name_len) >= 0)
				return DIVOOM_PORT_DIR_COMPARE_ERR_FORMAT;
			previous_name = child.name;
			previous_name_len = child.name_len;
			if (child.type == DCRC_NODE_FILE) {
				if (computed_count == UINT32_MAX)
					return DIVOOM_PORT_DIR_COMPARE_ERR_FORMAT;
				computed_count++;
			} else {
				if (UINT32_MAX - computed_count < child.file_count)
					return DIVOOM_PORT_DIR_COMPARE_ERR_FORMAT;
				computed_count += child.file_count;
			}
			computed_crc = dcrc_manifest_descriptor_crc(ctx, computed_crc, &child);
		}
		if (computed_count != node->file_count || computed_crc != node->crc32)
			return DIVOOM_PORT_DIR_COMPARE_ERR_FORMAT;
	}
	node->end_offset = reader->offset;
	return 0;
}

static dcrc_live_node_t *dcrc_live_node_alloc(const char *name, size_t name_len)
{
	dcrc_live_node_t *node;

	if (name_len > UINT32_MAX ||
	    !dcrc_name_valid((const unsigned char *)name, (uint32_t)name_len) ||
	    sizeof(*node) + name_len > UINT32_MAX)
		return NULL;
	node = (dcrc_live_node_t *)dcrc_alloc(sizeof(*node) + name_len);
	if (!node)
		return NULL;
	memset(node, 0, sizeof(*node));
	memcpy(node->name, name, name_len);
	node->name[name_len] = '\0';
	node->name_len = (uint32_t)name_len;
	return node;
}

static void dcrc_live_node_free(dcrc_live_node_t *node)
{
	uint32_t i;

	if (!node)
		return;
	for (i = 0; i < node->child_count; i++)
		dcrc_live_node_free(node->children[i]);
	dcrc_free(node->children);
	dcrc_free(node);
}

static char *dcrc_string_dup(const char *value)
{
	size_t length = strlen(value);
	char *copy;

	if (length > UINT32_MAX ||
	    !dcrc_name_valid((const unsigned char *)value, (uint32_t)length))
		return NULL;
	copy = (char *)dcrc_alloc(length + 1U);
	if (copy)
		memcpy(copy, value, length + 1U);
	return copy;
}

static int dcrc_scan_entry_compare(const void *left, const void *right)
{
	const dcrc_scan_entry_t *a = (const dcrc_scan_entry_t *)left;
	const dcrc_scan_entry_t *b = (const dcrc_scan_entry_t *)right;

	return strcmp(a->name, b->name);
}

static void dcrc_scan_entries_free(dcrc_scan_entry_t *entries, uint32_t count)
{
	uint32_t i;

	for (i = 0; i < count; i++)
		dcrc_free(entries[i].name);
	dcrc_free(entries);
}

static int dcrc_scan_entries_grow(dcrc_scan_entry_t **entries,
	uint32_t count, uint32_t *capacity)
{
	dcrc_scan_entry_t *grown;
	uint32_t new_capacity;

	if (count < *capacity)
		return 0;
	new_capacity = *capacity == 0U ? 16U : *capacity * 2U;
	if (new_capacity < *capacity ||
	    new_capacity > DIVOOM_PORT_DIR_CHECKSUM_MAX_NODES)
		new_capacity = DIVOOM_PORT_DIR_CHECKSUM_MAX_NODES;
	if (new_capacity <= count)
		return DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM;
	grown = (dcrc_scan_entry_t *)dcrc_alloc(
		(size_t)new_capacity * sizeof(*grown));
	if (!grown)
		return DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM;
	if (*entries) {
		memcpy(grown, *entries, (size_t)count * sizeof(*grown));
		dcrc_free(*entries);
	}
	*entries = grown;
	*capacity = new_capacity;
	return 0;
}

static int dcrc_collect_entries(const char *path, int ignore_usr_bookkeeping,
	dcrc_scan_entry_t **out_entries, uint32_t *out_count)
{
	dcrc_scan_entry_t *entries = NULL;
	uint32_t count = 0;
	uint32_t capacity = 0;
	const divoom_port_dirent_t *entry;
	void *dir;
	int rc = 0;

	dir = divoom_port_dir_open(path);
	if (!dir)
		return DIVOOM_PORT_DIR_COMPARE_ERR_IO;
	while ((entry = divoom_port_dir_read(dir)) != NULL) {
		if (!strcmp(entry->name, ".") || !strcmp(entry->name, ".."))
			continue;
		if (ignore_usr_bookkeeping &&
		    (!strcmp(entry->name, "file_crc.bin") ||
		     !strcmp(entry->name, "file_crc_version.txt")))
			continue;
		if (count >= DIVOOM_PORT_DIR_CHECKSUM_MAX_NODES) {
			rc = DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM;
			break;
		}
		rc = dcrc_scan_entries_grow(&entries, count, &capacity);
		if (rc != 0)
			break;
		entries[count].name = dcrc_string_dup(entry->name);
		if (!entries[count].name) {
			rc = DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM;
			break;
		}
		count++;
	}
	if (divoom_port_dir_close(dir) != 0 && rc == 0)
		rc = DIVOOM_PORT_DIR_COMPARE_ERR_IO;
	if (rc != 0) {
		dcrc_scan_entries_free(entries, count);
		return rc;
	}
	if (count > 1U)
		qsort(entries, count, sizeof(*entries), dcrc_scan_entry_compare);
	{
		uint32_t i;

		for (i = 1; i < count; i++) {
			if (!strcmp(entries[i - 1U].name, entries[i].name)) {
				dcrc_scan_entries_free(entries, count);
				return DIVOOM_PORT_DIR_COMPARE_ERR_IO;
			}
		}
	}
	*out_entries = entries;
	*out_count = count;
	return 0;
}

static int dcrc_join_path(const char *parent, const char *name,
	char *out, size_t out_size)
{
	size_t parent_len = strlen(parent);
	size_t name_len = strlen(name);
	int needs_separator = parent_len > 0U && parent[parent_len - 1U] != '/' &&
		parent[parent_len - 1U] != '\\';
	size_t required = parent_len + (needs_separator ? 1U : 0U) + name_len + 1U;

	if (required > out_size)
		return DIVOOM_PORT_DIR_COMPARE_ERR_PATH_TOO_LONG;
	memcpy(out, parent, parent_len);
	if (needs_separator)
		out[parent_len++] = '/';
	memcpy(out + parent_len, name, name_len + 1U);
	return 0;
}

static uint32_t dcrc_live_descriptor_crc(dcrc_context_t *ctx,
	uint32_t crc, const dcrc_live_node_t *node)
{
	crc = dcrc_crc32_update(ctx, crc, &node->type, 1U);
	crc = dcrc_crc32_varint(ctx, crc, node->name_len);
	crc = dcrc_crc32_update(ctx, crc, node->name, node->name_len);
	crc = dcrc_crc32_varint(ctx, crc,
		node->type == DCRC_NODE_FILE ? node->size : node->file_count);
	return dcrc_crc32_le32(ctx, crc, node->crc32);
}

static int dcrc_scan_file(dcrc_context_t *ctx, const char *path,
	const char *name, const divoom_port_stat_t *before, dcrc_live_node_t **out)
{
	dcrc_live_node_t *node;
	divoom_port_stat_t after;
	void *file;
	uint64_t bytes_read = 0;
	uint32_t crc = 0;
	size_t read_size;

	if (before->size < 0)
		return DIVOOM_PORT_DIR_COMPARE_ERR_IO;
	node = dcrc_live_node_alloc(name, strlen(name));
	if (!node)
		return DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM;
	node->type = DCRC_NODE_FILE;
	file = divoom_port_file_open(path, "rb");
	if (!file) {
		dcrc_live_node_free(node);
		return DIVOOM_PORT_DIR_COMPARE_ERR_IO;
	}
	while ((read_size = divoom_port_file_read(ctx->read_buffer, 1,
		ctx->read_buffer_size, file)) > 0U) {
		bytes_read += read_size;
		crc = dcrc_crc32_update(ctx, crc, ctx->read_buffer, read_size);
	}
    {
        int read_error = ferror((FILE *)file);
        int close_error = divoom_port_file_close(file);
        if (read_error || close_error != 0 || divoom_port_stat(path, &after) != 0) {
            dcrc_live_node_free(node);
            return DIVOOM_PORT_DIR_COMPARE_ERR_IO;
        }
    }
	if (before->size != after.size || before->mtime != after.mtime ||
	    bytes_read != (uint64_t)before->size) {
		dcrc_live_node_free(node);
		return DIVOOM_PORT_DIR_COMPARE_ERR_CHANGED;
	}
	node->size = bytes_read;
	node->crc32 = crc;
	node->file_count = 1U;
	*out = node;
	return 0;
}

static uint64_t dcrc_time_us(void)
{
#if defined(CLOCK_MONOTONIC) && !defined(_WIN32)
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) == 0)
        return (uint64_t)now.tv_sec * 1000000U + (uint64_t)now.tv_nsec / 1000U;
#endif
    return 0;
}

static int dcrc_read_file_crc32(dcrc_context_t *ctx, const char *path,
    const divoom_port_stat_t *before, void *file, uint64_t *size_out, uint32_t *crc32_out)
{
    divoom_port_stat_t after;
    /* Scan callers already own the opened file and only verify its bytes.
     * Standalone CRC/download verification keeps its strict path checks. */
    int check_path = file == NULL;
    uint64_t bytes_read = 0;
    uint32_t crc = 0;
    size_t read_size;
    /* Allocate only after the manifest is loaded. Under fragmentation the
     * preferred block may fail while the smaller buffer still fits. */
    if (!ctx->read_buffer) {
        for (;;) {
            ctx->read_buffer = (unsigned char *)dcrc_alloc(ctx->read_buffer_size);
            if (ctx->read_buffer) break;
            if (ctx->read_buffer_size <= DIVOOM_PORT_DIR_CHECKSUM_READ_CHUNK) {
                if (file) divoom_port_file_close(file);
                errno = ENOMEM;
                return DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM;
            }
            ctx->read_buffer_size /= 2U;
            if (ctx->read_buffer_size < DIVOOM_PORT_DIR_CHECKSUM_READ_CHUNK)
                ctx->read_buffer_size = DIVOOM_PORT_DIR_CHECKSUM_READ_CHUNK;
        }
        errno = 0; /* A successful 32KB fallback must not retain ENOMEM. */
    }
    uint64_t started;
    if (!file) {
        started = ctx->profile_scan ? dcrc_time_us() : 0;
        file = divoom_port_file_open(path, "rb");
        if (ctx->profile_scan) ctx->open_us += dcrc_time_us() - started;
        if (!file) return DIVOOM_PORT_DIR_COMPARE_ERR_IO;
    }
    size_t chunk = ctx->read_buffer_size;
    if (ctx->profile_scan) {
        /* Newlib nano fread refills its default 1KB buffer, even for a 256KB
         * request. Split the existing allocation into disjoint CRC input and
         * stdio refill buffers so FatFs receives large reads. fclose releases
         * the stream before either half is reused; no extra allocation. */
        size_t half = ctx->read_buffer_size / 2U;
        if (setvbuf((FILE *)file, (char *)ctx->read_buffer + half, _IOFBF, half) == 0)
            chunk = half;
        errno = 0;
        ctx->io_chunk_size = chunk;
    }
    errno = 0;
    for (;;) {
        started = ctx->profile_scan ? dcrc_time_us() : 0;
        read_size = divoom_port_file_read(ctx->read_buffer, 1, chunk, file);
        if (ctx->profile_scan) ctx->read_us += dcrc_time_us() - started;
        if (!read_size) break;
        bytes_read += read_size;
        started = ctx->profile_scan ? dcrc_time_us() : 0;
        crc = dcrc_crc32_update(ctx, crc, ctx->read_buffer, read_size);
        if (ctx->profile_scan) {
            ctx->crc_us += dcrc_time_us() - started;
            ctx->checked_bytes += read_size;
        }
    }
    {
        int read_error = ferror((FILE *)file);
        int read_errno = read_error ? (errno ? errno : EIO) : 0;
        started = ctx->profile_scan ? dcrc_time_us() : 0;
        errno = 0;
        int close_error = divoom_port_file_close(file);
        int close_errno = close_error ? (errno ? errno : EIO) : 0;
        if (ctx->profile_scan) ctx->close_us += dcrc_time_us() - started;
        if (read_error || close_error != 0) {
            errno = read_error ? read_errno : close_errno;
            return DIVOOM_PORT_DIR_COMPARE_ERR_IO;
        }
    }
    if (check_path) {
        if (divoom_port_stat(path, &after) != 0) return DIVOOM_PORT_DIR_COMPARE_ERR_IO;
        if (before->size != after.size || before->mtime != after.mtime)
            return DIVOOM_PORT_DIR_COMPARE_ERR_CHANGED;
    }
    if (bytes_read != (uint64_t)before->size)
        return DIVOOM_PORT_DIR_COMPARE_ERR_CHANGED;
    *size_out = bytes_read;
    *crc32_out = crc;
    return 0;
}

static dcrc_context_t *dcrc_file_reader_create(size_t read_size)
{
    dcrc_context_t *ctx = (dcrc_context_t *)dcrc_alloc(sizeof(*ctx));
    if (!ctx) return NULL;
    memset(ctx, 0, sizeof(*ctx));
    dcrc_crc32_init(ctx);
    ctx->read_buffer_size = read_size;
    return ctx;
}

void *divoom_port_file_verifier_create(void)
{
    dcrc_context_t *ctx = dcrc_file_reader_create(256U * 1024U);
    if (ctx) ctx->profile_scan = 1;
    return ctx;
}

void divoom_port_file_verifier_destroy(void *verifier)
{
    dcrc_context_t *ctx = verifier;
    if (!ctx) return;
    if (ctx->profile_scan && ctx->checked_files) {
        /* One summary per scan, never per-file serial output. Timings include
         * time descheduled inside each operation, not just CPU execution. */
        printf("[USR_CRC] files=%u bytes=%llu buffer_kb=%u chunk_kb=%u "
            "handle_ms=%llu open_ms=%llu read_ms=%llu crc_ms=%llu close_ms=%llu\n",
            (unsigned)ctx->checked_files, (unsigned long long)ctx->checked_bytes,
            (unsigned)(ctx->read_buffer_size / 1024U), (unsigned)(ctx->io_chunk_size / 1024U),
            (unsigned long long)(ctx->handle_us / 1000U), (unsigned long long)(ctx->open_us / 1000U),
            (unsigned long long)(ctx->read_us / 1000U), (unsigned long long)(ctx->crc_us / 1000U),
            (unsigned long long)(ctx->close_us / 1000U));
    }
    dcrc_free(ctx->read_buffer);
    dcrc_free(ctx);
}

int divoom_port_file_verifier_verify(void *verifier, const char *path,
    uint64_t expected_size, uint32_t expected_crc, int allow_changed_size)
{
    dcrc_context_t *ctx = verifier;
    divoom_port_stat_t before;
    uint64_t size;
    uint32_t crc;
    int rc;
    if (!ctx || !ctx->read_buffer_size || !path || !path[0])
        return DIVOOM_PORT_DIR_COMPARE_ERR_ARGUMENT;
    /* Self-check accepts either known font revision or an absent font.
     * Existing files still need a full CRC; download/install checks stay strict. */
    int font143_selfcheck = allow_changed_size &&
        !strcmp(path, "/usr/share/divoom_app/divoom/21/143.bin");
    errno = 0;
    uint64_t started = ctx->profile_scan ? dcrc_time_us() : 0;
    void *file = divoom_port_file_open(path, "rb");
    int open_errno = errno;
    if (ctx->profile_scan) {
        ++ctx->checked_files;
        ctx->open_us += dcrc_time_us() - started;
    }
    if (!file) {
        errno = open_errno;
        if (font143_selfcheck && errno == ENOENT) { errno = 0; return 0; }
        return errno == ENOMEM || errno == EMFILE || errno == ENFILE ?
            DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM : DIVOOM_PORT_DIR_COMPARE_ERR_IO;
    }
    started = ctx->profile_scan ? dcrc_time_us() : 0;
    int stat_error = divoom_port_file_stat(file, &before);
    if (ctx->profile_scan) ctx->handle_us += dcrc_time_us() - started;
    if (stat_error != 0 || before.is_dir || before.size < 0 ||
        (allow_changed_size && !font143_selfcheck && (uint64_t)before.size != expected_size)) {
        int saved_errno = errno;
        rc = stat_error != 0 || before.is_dir || before.size < 0 ?
            DIVOOM_PORT_DIR_COMPARE_ERR_IO : 0;
        started = ctx->profile_scan ? dcrc_time_us() : 0;
        int close_error = divoom_port_file_close(file);
        if (ctx->profile_scan) ctx->close_us += dcrc_time_us() - started;
        if (rc) errno = saved_errno;
        if (rc || close_error != 0) return errno == ENOMEM || errno == EMFILE || errno == ENFILE ?
            DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM : DIVOOM_PORT_DIR_COMPARE_ERR_IO;
        return 0;
    }
    rc = dcrc_read_file_crc32(ctx, path, &before, file, &size, &crc);
    if (rc == DIVOOM_PORT_DIR_COMPARE_ERR_IO &&
        (errno == ENOMEM || errno == EMFILE || errno == ENFILE))
        return DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM;
    if (rc) return rc;
    if (font143_selfcheck)
        return (size == 416076U && crc == 0x60f2a032U) ||
            (size == 386220U && crc == 0x4005d859U) ? 0 : DIVOOM_PORT_DIR_COMPARE_DIFFERENT;
    return size == expected_size && crc == expected_crc ? 0 : DIVOOM_PORT_DIR_COMPARE_DIFFERENT;
}

int divoom_port_file_crc32(const char *path, uint64_t *size_out,
    uint32_t *crc32_out)
{
    divoom_port_stat_t before;
    dcrc_context_t *ctx;
    int rc;
    if (!path || !path[0] || !size_out || !crc32_out)
        return DIVOOM_PORT_DIR_COMPARE_ERR_ARGUMENT;
    if (divoom_port_stat(path, &before) != 0 || before.is_dir || before.size < 0)
        return DIVOOM_PORT_DIR_COMPARE_ERR_IO;
    ctx = dcrc_file_reader_create(DIVOOM_PORT_DIR_CHECKSUM_READ_CHUNK);
    if (!ctx) return DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM;
    rc = dcrc_read_file_crc32(ctx, path, &before, NULL, size_out, crc32_out);
    int saved_errno = errno;
    divoom_port_file_verifier_destroy(ctx);
    errno = saved_errno;
    return rc;
}

static int dcrc_scan_node(dcrc_context_t *ctx, const char *path,
	const char *name, uint32_t depth, dcrc_live_node_t **out)
{
	divoom_port_stat_t before;
	dcrc_live_node_t *node = NULL;
	dcrc_scan_entry_t *entries = NULL;
	uint32_t entry_count = 0;
	uint32_t i;
	int rc;

	if (depth > DIVOOM_PORT_DIR_CHECKSUM_MAX_DEPTH ||
	    ++ctx->live_node_count > DIVOOM_PORT_DIR_CHECKSUM_MAX_NODES)
		return DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM;
	if (divoom_port_stat(path, &before) != 0)
		return DIVOOM_PORT_DIR_COMPARE_ERR_IO;
	if (!before.is_dir)
		return dcrc_scan_file(ctx, path, name, &before, out);
	rc = dcrc_collect_entries(path, depth == 0U && !strcmp(name, "usr"),
		&entries, &entry_count);
	if (rc != 0)
		return rc;
	node = dcrc_live_node_alloc(name, strlen(name));
	if (!node) {
		dcrc_scan_entries_free(entries, entry_count);
		return DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM;
	}
	node->type = DCRC_NODE_DIRECTORY;
	if (entry_count > 0U) {
		node->children = (dcrc_live_node_t **)dcrc_alloc(
			(size_t)entry_count * sizeof(*node->children));
		if (!node->children) {
			dcrc_scan_entries_free(entries, entry_count);
			dcrc_live_node_free(node);
			return DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM;
		}
	}
	for (i = 0; i < entry_count; i++) {
		char child_path[DIVOOM_PORT_DIR_CHECKSUM_PATH_MAX];
		dcrc_live_node_t *child = NULL;

		rc = dcrc_join_path(path, entries[i].name, child_path,
			sizeof(child_path));
		if (rc == 0)
			rc = dcrc_scan_node(ctx, child_path, entries[i].name,
				depth + 1U, &child);
		if (rc != 0) {
			dcrc_scan_entries_free(entries, entry_count);
			dcrc_live_node_free(node);
			return rc;
		}
		node->children[node->child_count++] = child;
		if (child->type == DCRC_NODE_FILE) {
			if (node->file_count == UINT32_MAX) {
				dcrc_scan_entries_free(entries, entry_count);
				dcrc_live_node_free(node);
				return DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM;
			}
			node->file_count++;
		} else {
			if (UINT32_MAX - node->file_count < child->file_count) {
				dcrc_scan_entries_free(entries, entry_count);
				dcrc_live_node_free(node);
				return DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM;
			}
			node->file_count += child->file_count;
		}
	}
	dcrc_scan_entries_free(entries, entry_count);
	node->crc32 = dcrc_crc32_update(ctx, 0U, g_dcrc_directory_prefix,
		sizeof(g_dcrc_directory_prefix));
	node->crc32 = dcrc_crc32_varint(ctx, node->crc32, node->child_count);
	for (i = 0; i < node->child_count; i++)
		node->crc32 = dcrc_live_descriptor_crc(ctx, node->crc32,
			node->children[i]);
	{
		divoom_port_stat_t after;

		if (divoom_port_stat(path, &after) != 0) {
			dcrc_live_node_free(node);
			return DIVOOM_PORT_DIR_COMPARE_ERR_IO;
		}
		if (!after.is_dir || before.mtime != after.mtime) {
			dcrc_live_node_free(node);
			return DIVOOM_PORT_DIR_COMPARE_ERR_CHANGED;
		}
	}
	*out = node;
	return 0;
}

static int dcrc_path_push(dcrc_context_t *ctx, const unsigned char *name,
	uint32_t name_len, size_t *saved_len)
{
	size_t required;

	*saved_len = ctx->callback_path_len;
	required = ctx->callback_path_len +
		(ctx->callback_path_len ? 1U : 0U) + name_len + 1U;
	if (required > sizeof(ctx->callback_path))
		return DIVOOM_PORT_DIR_COMPARE_ERR_PATH_TOO_LONG;
	if (ctx->callback_path_len)
		ctx->callback_path[ctx->callback_path_len++] = '/';
	memcpy(ctx->callback_path + ctx->callback_path_len, name, name_len);
	ctx->callback_path_len += name_len;
	ctx->callback_path[ctx->callback_path_len] = '\0';
	return 0;
}

static void dcrc_path_pop(dcrc_context_t *ctx, size_t saved_len)
{
	ctx->callback_path_len = saved_len;
	ctx->callback_path[saved_len] = '\0';
}

static int dcrc_emit_change(dcrc_context_t *ctx,
	divoom_port_dir_change_type_t type, uint64_t old_size, uint32_t old_crc,
	uint64_t new_size, uint32_t new_crc)
{
	divoom_port_dir_change_t change;

	ctx->result->change_count++;
	if (!ctx->callback)
		return 0;
	memset(&change, 0, sizeof(change));
	change.type = type;
	change.relative_path = ctx->callback_path;
	change.old_size = old_size;
	change.new_size = new_size;
	change.old_crc32 = old_crc;
	change.new_crc32 = new_crc;
	if (ctx->callback(&change, ctx->user_data) != 0)
		return DIVOOM_PORT_DIR_COMPARE_ERR_CALLBACK;
	return 0;
}

static int dcrc_manifest_child(dcrc_context_t *ctx,
	const unsigned char *data, size_t size, size_t *offset,
	dcrc_manifest_node_t *child)
{
	dcrc_reader_t reader;
	int rc;

	memset(&reader, 0, sizeof(reader));
	reader.data = data;
	reader.size = size;
	reader.offset = *offset;
	rc = dcrc_parse_node(ctx, &reader, child, 0U);
	if (rc == 0)
		*offset = reader.offset;
	return rc;
}

static int dcrc_emit_manifest_files(dcrc_context_t *ctx,
	const unsigned char *data, size_t size, const dcrc_manifest_node_t *node)
{
	int rc;

	if (node->type == DCRC_NODE_FILE)
		return dcrc_emit_change(ctx, DIVOOM_PORT_DIR_CHANGE_REMOVED,
			node->size, node->crc32, 0U, 0U);
    if (ctx->include_directories && ctx->callback_path_len) {
        rc = dcrc_emit_change(ctx, DIVOOM_PORT_DIR_CHANGE_ADDED, 0, 0, 0, 0);
        if (rc) return rc;
    }
	{
		size_t offset = node->children_offset;
		uint32_t i;

		for (i = 0; i < node->child_count; i++) {
			dcrc_manifest_node_t child;
			size_t saved_len;

			rc = dcrc_manifest_child(ctx, data, size, &offset, &child);
			if (rc != 0)
				return rc;
			rc = dcrc_path_push(ctx, child.name, child.name_len, &saved_len);
			if (rc == 0)
				rc = dcrc_emit_manifest_files(ctx, data, size, &child);
			dcrc_path_pop(ctx, saved_len);
			if (rc != 0)
				return rc;
		}
	}
	return 0;
}

static int dcrc_emit_live_files(dcrc_context_t *ctx,
	const dcrc_live_node_t *node)
{
	uint32_t i;
	int rc;

	if (node->type == DCRC_NODE_FILE)
		return dcrc_emit_change(ctx, DIVOOM_PORT_DIR_CHANGE_ADDED,
			0U, 0U, node->size, node->crc32);
	for (i = 0; i < node->child_count; i++) {
		size_t saved_len;
		const dcrc_live_node_t *child = node->children[i];

		rc = dcrc_path_push(ctx, (const unsigned char *)child->name,
			child->name_len, &saved_len);
		if (rc == 0)
			rc = dcrc_emit_live_files(ctx, child);
		dcrc_path_pop(ctx, saved_len);
		if (rc != 0)
			return rc;
	}
	return 0;
}

static int dcrc_compare_node(dcrc_context_t *ctx,
	const unsigned char *data, size_t size,
	const dcrc_manifest_node_t *expected, const dcrc_live_node_t *current);

static int dcrc_compare_directory(dcrc_context_t *ctx,
	const unsigned char *data, size_t size,
	const dcrc_manifest_node_t *expected, const dcrc_live_node_t *current)
{
	size_t offset = expected->children_offset;
	uint32_t expected_index = 0;
	uint32_t current_index = 0;
	dcrc_manifest_node_t old_child;
	int have_old_child = 0;
	int rc;

	if (expected->file_count == current->file_count &&
	    expected->crc32 == current->crc32)
		return 0;
	while (expected_index < expected->child_count ||
	       current_index < current->child_count) {
		const dcrc_live_node_t *new_child = NULL;
		int order;

		if (!have_old_child && expected_index < expected->child_count) {
			rc = dcrc_manifest_child(ctx, data, size, &offset, &old_child);
			if (rc != 0)
				return rc;
			have_old_child = 1;
		}
		if (current_index < current->child_count)
			new_child = current->children[current_index];
		if (!have_old_child)
			order = 1;
		else if (!new_child)
			order = -1;
		else
			order = dcrc_name_compare(old_child.name, old_child.name_len,
				(const unsigned char *)new_child->name, new_child->name_len);
		if (order < 0) {
			size_t saved_len;

			rc = dcrc_path_push(ctx, old_child.name, old_child.name_len,
				&saved_len);
			if (rc == 0)
				rc = dcrc_emit_manifest_files(ctx, data, size, &old_child);
			dcrc_path_pop(ctx, saved_len);
			have_old_child = 0;
			expected_index++;
		} else if (order > 0) {
			size_t saved_len;

			rc = dcrc_path_push(ctx, (const unsigned char *)new_child->name,
				new_child->name_len, &saved_len);
			if (rc == 0)
				rc = dcrc_emit_live_files(ctx, new_child);
			dcrc_path_pop(ctx, saved_len);
			current_index++;
		} else {
			size_t saved_len;

			rc = dcrc_path_push(ctx, old_child.name, old_child.name_len,
				&saved_len);
			if (rc == 0)
				rc = dcrc_compare_node(ctx, data, size, &old_child, new_child);
			dcrc_path_pop(ctx, saved_len);
			have_old_child = 0;
			expected_index++;
			current_index++;
		}
		if (rc != 0)
			return rc;
	}
	return 0;
}

static int dcrc_compare_node(dcrc_context_t *ctx,
	const unsigned char *data, size_t size,
	const dcrc_manifest_node_t *expected, const dcrc_live_node_t *current)
{
	int rc;

	if (expected->type == current->type) {
		if (expected->type == DCRC_NODE_FILE) {
			if (expected->size == current->size &&
			    expected->crc32 == current->crc32)
				return 0;
			return dcrc_emit_change(ctx, DIVOOM_PORT_DIR_CHANGE_MODIFIED,
				expected->size, expected->crc32,
				current->size, current->crc32);
		}
		return dcrc_compare_directory(ctx, data, size, expected, current);
	}
	if (expected->type == DCRC_NODE_FILE) {
		rc = dcrc_emit_manifest_files(ctx, data, size, expected);
		if (rc != 0)
			return rc;
		return dcrc_emit_live_files(ctx, current);
	}
	rc = dcrc_emit_manifest_files(ctx, data, size, expected);
	if (rc != 0)
		return rc;
	return dcrc_emit_live_files(ctx, current);
}

static int dcrc_root_name(const char *root_path, const char **name,
	size_t *name_len)
{
	size_t length;
	size_t start;

	if (!root_path || !root_path[0])
		return DIVOOM_PORT_DIR_COMPARE_ERR_ARGUMENT;
	length = strlen(root_path);
	while (length > 0U && (root_path[length - 1U] == '/' ||
	       root_path[length - 1U] == '\\'))
		length--;
	if (length == 0U)
		return DIVOOM_PORT_DIR_COMPARE_ERR_ARGUMENT;
	start = length;
	while (start > 0U && root_path[start - 1U] != '/' &&
	       root_path[start - 1U] != '\\')
		start--;
	*name = root_path + start;
	*name_len = length - start;
	return dcrc_name_valid((const unsigned char *)*name,
		(uint32_t)*name_len) ? 0 : DIVOOM_PORT_DIR_COMPARE_ERR_ARGUMENT;
}

int divoom_port_dir_checksum_compare(const char *manifest_path,
	const char *root_path, divoom_port_dir_change_cb callback, void *user_data,
	divoom_port_dir_compare_result_t *result)
{
	divoom_port_dir_compare_result_t local_result;
	dcrc_manifest_node_t expected_root;
	dcrc_live_node_t *current_root = NULL;
	dcrc_context_t *ctx = NULL;
	dcrc_reader_t reader;
	char *manifest_data = NULL;
	const char *root_name;
	size_t root_name_len;
	int manifest_len = 0;
	size_t payload_size;
	uint32_t stored_crc;
	uint32_t actual_crc;
	int rc;

	if (!result)
		result = &local_result;
	memset(result, 0, sizeof(*result));
	if (!manifest_path || !manifest_path[0] || !root_path || !root_path[0])
		return DIVOOM_PORT_DIR_COMPARE_ERR_ARGUMENT;
	rc = dcrc_root_name(root_path, &root_name, &root_name_len);
	if (rc != 0)
		return rc;
    errno = 0;
	manifest_data = divoom_port_file_read_all_bounded(manifest_path, &manifest_len,
		DIVOOM_PORT_DIR_CHECKSUM_MAX_MANIFEST);
	if (!manifest_data)
		return errno == EFBIG ? DIVOOM_PORT_DIR_COMPARE_ERR_FORMAT :
		    errno == ENOMEM || errno == EMFILE || errno == ENFILE ?
            DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM : DIVOOM_PORT_DIR_COMPARE_ERR_IO;
	if (manifest_len < (int)(DCRC_HEADER_SIZE + DCRC_TRAILER_SIZE) ||
	    (uint32_t)manifest_len > DIVOOM_PORT_DIR_CHECKSUM_MAX_MANIFEST) {
		rc = DIVOOM_PORT_DIR_COMPARE_ERR_FORMAT;
		goto out;
	}
	ctx = (dcrc_context_t *)dcrc_alloc(sizeof(*ctx));
	if (!ctx) {
		rc = DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM;
		goto out;
	}
	memset(ctx, 0, sizeof(*ctx));
	ctx->callback = callback;
	ctx->user_data = user_data;
	ctx->result = result;
	dcrc_crc32_init(ctx);
	payload_size = (size_t)manifest_len - DCRC_TRAILER_SIZE;
	if (memcmp(manifest_data, g_dcrc_magic, sizeof(g_dcrc_magic)) != 0 ||
	    (unsigned char)manifest_data[4] != DCRC_FORMAT_VERSION ||
	    (unsigned char)manifest_data[5] != DCRC_ALGORITHM_CRC32) {
		rc = DIVOOM_PORT_DIR_COMPARE_ERR_FORMAT;
		goto out;
	}
	stored_crc = dcrc_get_le32((const unsigned char *)manifest_data + payload_size);
	actual_crc = dcrc_crc32_update(ctx, 0U, manifest_data, payload_size);
	if (stored_crc != actual_crc) {
		rc = DIVOOM_PORT_DIR_COMPARE_ERR_FORMAT;
		goto out;
	}
	memset(&reader, 0, sizeof(reader));
	reader.data = (const unsigned char *)manifest_data;
	reader.size = payload_size;
	reader.offset = DCRC_HEADER_SIZE;
	rc = dcrc_parse_node(ctx, &reader, &expected_root, 0U);
	if (rc != 0 || expected_root.type != DCRC_NODE_DIRECTORY ||
	    reader.offset != payload_size) {
		rc = DIVOOM_PORT_DIR_COMPARE_ERR_FORMAT;
		goto out;
	}
	ctx->read_buffer_size = DIVOOM_PORT_DIR_CHECKSUM_READ_CHUNK;
	ctx->read_buffer = (unsigned char *)dcrc_alloc(ctx->read_buffer_size);
	if (!ctx->read_buffer) {
		rc = DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM;
		goto out;
	}
	{
		char root_name_copy[DIVOOM_PORT_DIR_CHECKSUM_PATH_MAX];

		memcpy(root_name_copy, root_name, root_name_len);
		root_name_copy[root_name_len] = '\0';
		rc = dcrc_scan_node(ctx, root_path, root_name_copy, 0U, &current_root);
	}
	if (rc != 0)
		goto out;
	result->manifest_file_count = expected_root.file_count;
	result->current_file_count = current_root->file_count;
	result->manifest_crc32 = expected_root.crc32;
	result->current_crc32 = current_root->crc32;
	if (dcrc_name_compare(expected_root.name, expected_root.name_len,
	    (const unsigned char *)current_root->name, current_root->name_len) == 0 &&
	    expected_root.file_count == current_root->file_count &&
	    expected_root.crc32 == current_root->crc32) {
		rc = DIVOOM_PORT_DIR_COMPARE_MATCH;
		goto out;
	}
	if (callback) {
		rc = dcrc_compare_directory(ctx,
			(const unsigned char *)manifest_data, payload_size,
			&expected_root, current_root);
		if (rc != 0)
			goto out;
	}
	rc = DIVOOM_PORT_DIR_COMPARE_DIFFERENT;

out:
	dcrc_live_node_free(current_root);
	if (ctx) {
		dcrc_free(ctx->read_buffer);
		dcrc_free(ctx);
	}
	dcrc_free(manifest_data);
	return rc;
}

static int dcrc_emit_manifest_change(dcrc_context_t *ctx,
	const unsigned char *data, size_t size,
	const dcrc_manifest_node_t *node, divoom_port_dir_change_type_t type)
{
	int rc;

	if (node->type == DCRC_NODE_FILE) {
		if (type == DIVOOM_PORT_DIR_CHANGE_ADDED)
			return dcrc_emit_change(ctx, type, 0U, 0U,
				node->size, node->crc32);
		return dcrc_emit_change(ctx, type, node->size, node->crc32,
			0U, 0U);
	}
	{
		size_t offset = node->children_offset;
		uint32_t i;

		for (i = 0; i < node->child_count; i++) {
			dcrc_manifest_node_t child;
			size_t saved_len;

			rc = dcrc_manifest_child(ctx, data, size, &offset, &child);
			if (rc != 0)
				return rc;
			rc = dcrc_path_push(ctx, child.name, child.name_len, &saved_len);
			if (rc == 0)
				rc = dcrc_emit_manifest_change(ctx, data, size,
					&child, type);
			dcrc_path_pop(ctx, saved_len);
			if (rc != 0)
				return rc;
		}
	}
	return 0;
}

static int dcrc_compare_manifest_node(dcrc_context_t *ctx,
	const unsigned char *old_data, size_t old_size,
	const dcrc_manifest_node_t *old_node,
	const unsigned char *new_data, size_t new_size,
	const dcrc_manifest_node_t *new_node);

static int dcrc_compare_manifest_directory(dcrc_context_t *ctx,
	const unsigned char *old_data, size_t old_size,
	const dcrc_manifest_node_t *old_node,
	const unsigned char *new_data, size_t new_size,
	const dcrc_manifest_node_t *new_node)
{
	size_t old_offset = old_node->children_offset;
	size_t new_offset = new_node->children_offset;
	uint32_t old_index = 0;
	uint32_t new_index = 0;
	dcrc_manifest_node_t old_child;
	dcrc_manifest_node_t new_child;
	int have_old = 0;
	int have_new = 0;
	int rc;

	if (old_node->file_count == new_node->file_count &&
	    old_node->crc32 == new_node->crc32)
		return 0;
	while (old_index < old_node->child_count ||
	       new_index < new_node->child_count) {
		int order;
		size_t saved_len;

		if (!have_old && old_index < old_node->child_count) {
			rc = dcrc_manifest_child(ctx, old_data, old_size,
				&old_offset, &old_child);
			if (rc != 0)
				return rc;
			have_old = 1;
		}
		if (!have_new && new_index < new_node->child_count) {
			rc = dcrc_manifest_child(ctx, new_data, new_size,
				&new_offset, &new_child);
			if (rc != 0)
				return rc;
			have_new = 1;
		}
		if (!have_old)
			order = 1;
		else if (!have_new)
			order = -1;
		else
			order = dcrc_name_compare(old_child.name, old_child.name_len,
				new_child.name, new_child.name_len);

		if (order < 0) {
			rc = dcrc_path_push(ctx, old_child.name,
				old_child.name_len, &saved_len);
			if (rc == 0)
				rc = dcrc_emit_manifest_change(ctx, old_data,
					old_size, &old_child,
					DIVOOM_PORT_DIR_CHANGE_REMOVED);
			dcrc_path_pop(ctx, saved_len);
			have_old = 0;
			old_index++;
		} else if (order > 0) {
			rc = dcrc_path_push(ctx, new_child.name,
				new_child.name_len, &saved_len);
			if (rc == 0)
				rc = dcrc_emit_manifest_change(ctx, new_data,
					new_size, &new_child,
					DIVOOM_PORT_DIR_CHANGE_ADDED);
			dcrc_path_pop(ctx, saved_len);
			have_new = 0;
			new_index++;
		} else {
			rc = dcrc_path_push(ctx, old_child.name,
				old_child.name_len, &saved_len);
			if (rc == 0)
				rc = dcrc_compare_manifest_node(ctx,
					old_data, old_size, &old_child,
					new_data, new_size, &new_child);
			dcrc_path_pop(ctx, saved_len);
			have_old = 0;
			have_new = 0;
			old_index++;
			new_index++;
		}
		if (rc != 0)
			return rc;
	}
	return 0;
}

static int dcrc_compare_manifest_node(dcrc_context_t *ctx,
	const unsigned char *old_data, size_t old_size,
	const dcrc_manifest_node_t *old_node,
	const unsigned char *new_data, size_t new_size,
	const dcrc_manifest_node_t *new_node)
{
	int rc;

	if (old_node->type == new_node->type) {
		if (old_node->type == DCRC_NODE_FILE) {
			if (old_node->size == new_node->size &&
			    old_node->crc32 == new_node->crc32)
				return 0;
			return dcrc_emit_change(ctx,
				DIVOOM_PORT_DIR_CHANGE_MODIFIED,
				old_node->size, old_node->crc32,
				new_node->size, new_node->crc32);
		}
		return dcrc_compare_manifest_directory(ctx,
			old_data, old_size, old_node,
			new_data, new_size, new_node);
	}
	rc = dcrc_emit_manifest_change(ctx, old_data, old_size, old_node,
		DIVOOM_PORT_DIR_CHANGE_REMOVED);
	if (rc != 0)
		return rc;
	return dcrc_emit_manifest_change(ctx, new_data, new_size, new_node,
		DIVOOM_PORT_DIR_CHANGE_ADDED);
}

static int dcrc_load_manifest(dcrc_context_t *ctx, const char *path,
	char **data_out, size_t *payload_size_out,
	dcrc_manifest_node_t *root_out)
{
	dcrc_reader_t reader;
	char *data;
	int length = 0;
	size_t payload_size;
	uint32_t stored_crc;
	uint32_t actual_crc;
	int rc;

    errno = 0;
	data = divoom_port_file_read_all_bounded(path, &length,
		DIVOOM_PORT_DIR_CHECKSUM_MAX_MANIFEST);
	if (!data)
		return errno == EFBIG ? DIVOOM_PORT_DIR_COMPARE_ERR_FORMAT :
		    errno == ENOMEM || errno == EMFILE || errno == ENFILE ?
            DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM : DIVOOM_PORT_DIR_COMPARE_ERR_IO;
	if (length < (int)(DCRC_HEADER_SIZE + DCRC_TRAILER_SIZE) ||
	    (uint32_t)length > DIVOOM_PORT_DIR_CHECKSUM_MAX_MANIFEST) {
		dcrc_free(data);
		return DIVOOM_PORT_DIR_COMPARE_ERR_FORMAT;
	}
	payload_size = (size_t)length - DCRC_TRAILER_SIZE;
	if (memcmp(data, g_dcrc_magic, sizeof(g_dcrc_magic)) != 0 ||
	    (unsigned char)data[4] != DCRC_FORMAT_VERSION ||
	    (unsigned char)data[5] != DCRC_ALGORITHM_CRC32) {
		dcrc_free(data);
		return DIVOOM_PORT_DIR_COMPARE_ERR_FORMAT;
	}
	stored_crc = dcrc_get_le32((const unsigned char *)data + payload_size);
	actual_crc = dcrc_crc32_update(ctx, 0U, data, payload_size);
	if (stored_crc != actual_crc) {
		dcrc_free(data);
		return DIVOOM_PORT_DIR_COMPARE_ERR_FORMAT;
	}
	memset(&reader, 0, sizeof(reader));
	reader.data = (const unsigned char *)data;
	reader.size = payload_size;
	reader.offset = DCRC_HEADER_SIZE;
	rc = dcrc_parse_node(ctx, &reader, root_out, 0U);
	if (rc != 0 || root_out->type != DCRC_NODE_DIRECTORY ||
	    reader.offset != payload_size) {
		dcrc_free(data);
		return rc != 0 ? rc : DIVOOM_PORT_DIR_COMPARE_ERR_FORMAT;
	}
	*data_out = data;
	*payload_size_out = payload_size;
	return 0;
}

int divoom_port_dir_manifest_file_info(const char *manifest, const char *root,
	const char *relative_path, uint64_t *size_out, uint32_t *crc32_out)
{
	dcrc_context_t *ctx;
	dcrc_manifest_node_t node;
	char *data = NULL;
	size_t size = 0, root_name_len;
	const char *root_name, *part = relative_path;
	int rc;
	if (!manifest || !size_out || !crc32_out || !part || !part[0] ||
	    strlen(part) >= DIVOOM_PORT_DIR_CHECKSUM_PATH_MAX)
		return DIVOOM_PORT_DIR_COMPARE_ERR_ARGUMENT;
	*size_out = 0;
	*crc32_out = 0;
	rc = dcrc_root_name(root, &root_name, &root_name_len);
	if (rc != 0) return rc;
	ctx = dcrc_alloc(sizeof(*ctx));
	if (!ctx) return DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM;
	memset(ctx, 0, sizeof(*ctx));
	dcrc_crc32_init(ctx);
	rc = dcrc_load_manifest(ctx, manifest, &data, &size, &node);
	if (rc != 0) goto out;
	if (node.name_len != root_name_len || memcmp(node.name, root_name, root_name_len) != 0) {
		rc = DIVOOM_PORT_DIR_COMPARE_ERR_FORMAT;
		goto out;
	}
	while (*part) {
		const char *slash = strchr(part, '/');
		size_t length = slash ? (size_t)(slash - part) : strlen(part);
		size_t offset = node.children_offset;
		uint32_t i, count = node.child_count;
		int found = 0;
		if (!dcrc_name_valid((const unsigned char *)part, (uint32_t)length) ||
		    (slash && !slash[1])) {
			rc = DIVOOM_PORT_DIR_COMPARE_ERR_ARGUMENT;
			goto out;
		}
		if (node.type != DCRC_NODE_DIRECTORY) {
			rc = DIVOOM_PORT_DIR_COMPARE_ERR_NOT_FOUND;
			goto out;
		}
		for (i = 0; i < count; ++i) {
			dcrc_manifest_node_t child;
			rc = dcrc_manifest_child(ctx, (unsigned char *)data, size, &offset, &child);
			if (rc != 0) goto out;
			if (child.name_len == length && memcmp(child.name, part, length) == 0) {
				node = child;
				found = 1;
				break;
			}
		}
		if (!found) {
			rc = DIVOOM_PORT_DIR_COMPARE_ERR_NOT_FOUND;
			goto out;
		}
		part = slash ? slash + 1 : part + length;
	}
	if (node.type != DCRC_NODE_FILE) {
		rc = DIVOOM_PORT_DIR_COMPARE_ERR_NOT_FOUND;
		goto out;
	}
	*size_out = node.size;
	*crc32_out = node.crc32;
	rc = 0;
out:
	dcrc_free(data);
	dcrc_free(ctx);
	return rc;
}

static int dcrc_prepare_dirs(dcrc_context_t *ctx, const unsigned char *data,
	size_t size, const dcrc_manifest_node_t *node, const char *root, int create)
{
	char path[DIVOOM_PORT_DIR_CHECKSUM_PATH_MAX];
	size_t offset = node->children_offset;
	uint32_t i;
	int rc;
	if (dcrc_join_path(root, ctx->callback_path, path, sizeof(path)) != 0) {
		return DIVOOM_PORT_DIR_COMPARE_ERR_PATH_TOO_LONG;
	}
	if (node->type == DCRC_NODE_FILE) {
		return 0;
	}
	if (create && divoom_port_mkdir_p(path) != 0) {
		return DIVOOM_PORT_DIR_COMPARE_ERR_IO;
	}
	for (i = 0; i < node->child_count; i++) {
		dcrc_manifest_node_t child;
		size_t saved_len;
		rc = dcrc_manifest_child(ctx, data, size, &offset, &child);
		if (rc != 0) {
			return rc;
		}
		rc = dcrc_path_push(ctx, child.name, child.name_len, &saved_len);
		if (rc == 0) {
			rc = dcrc_prepare_dirs(ctx, data, size, &child, root, create);
		}
		dcrc_path_pop(ctx, saved_len);
		if (rc != 0) {
			return rc;
		}
	}
	return 0;
}

int divoom_port_dir_manifest_prepare_dirs(const char *manifest, const char *root, int create)
{
	dcrc_context_t *ctx;
	dcrc_manifest_node_t node;
	char *data = NULL;
	size_t size;
	const char *root_name;
	size_t root_name_len;
	int rc = dcrc_root_name(root, &root_name, &root_name_len);
	if (rc != 0) {
		return rc;
	}
	ctx = dcrc_alloc(sizeof(*ctx));
	if (!ctx) {
		return DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM;
	}
	memset(ctx, 0, sizeof(*ctx));
	dcrc_crc32_init(ctx);
	rc = dcrc_load_manifest(ctx, manifest, &data, &size, &node);
	if (rc == 0) {
		if (node.name_len != root_name_len || memcmp(node.name, root_name, root_name_len) != 0) {
			rc = DIVOOM_PORT_DIR_COMPARE_ERR_FORMAT;
		} else {
			/* 先检查完整路径树，再创建，避免验证失败时半途写目录。 */
			rc = dcrc_prepare_dirs(ctx, (unsigned char *)data, size, &node, root, 0);
			if (rc == 0 && create) {
				rc = dcrc_prepare_dirs(ctx, (unsigned char *)data, size, &node, root, 1);
			}
		}
	}
	dcrc_free(data);
	dcrc_free(ctx);
	return rc;
}

int divoom_port_dir_manifest_compare(const char *old_manifest_path,
	const char *new_manifest_path, divoom_port_dir_change_cb callback,
	void *user_data, divoom_port_dir_compare_result_t *result)
{
	divoom_port_dir_compare_result_t local_result;
	dcrc_context_t *ctx = NULL;
	dcrc_manifest_node_t old_root;
	dcrc_manifest_node_t new_root;
	char *old_data = NULL;
	char *new_data = NULL;
	size_t old_size = 0;
	size_t new_size = 0;
	int rc;

	if (!result)
		result = &local_result;
	memset(result, 0, sizeof(*result));
	if (!old_manifest_path || !old_manifest_path[0] ||
	    !new_manifest_path || !new_manifest_path[0])
		return DIVOOM_PORT_DIR_COMPARE_ERR_ARGUMENT;
	ctx = (dcrc_context_t *)dcrc_alloc(sizeof(*ctx));
	if (!ctx)
		return DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM;
	memset(ctx, 0, sizeof(*ctx));
	ctx->callback = callback;
	ctx->user_data = user_data;
	ctx->result = result;
	dcrc_crc32_init(ctx);
	rc = dcrc_load_manifest(ctx, old_manifest_path,
		&old_data, &old_size, &old_root);
	if (rc != 0)
		goto out;
	rc = dcrc_load_manifest(ctx, new_manifest_path,
		&new_data, &new_size, &new_root);
	if (rc != 0)
		goto out;
	if (dcrc_name_compare(old_root.name, old_root.name_len,
	    new_root.name, new_root.name_len) != 0) {
		rc = DIVOOM_PORT_DIR_COMPARE_ERR_FORMAT;
		goto out;
	}
	result->manifest_file_count = old_root.file_count;
	result->current_file_count = new_root.file_count;
	result->manifest_crc32 = old_root.crc32;
	result->current_crc32 = new_root.crc32;
	if (old_root.file_count == new_root.file_count &&
	    old_root.crc32 == new_root.crc32) {
		rc = DIVOOM_PORT_DIR_COMPARE_MATCH;
		goto out;
	}
	if (callback) {
		rc = dcrc_compare_manifest_directory(ctx,
			(const unsigned char *)old_data, old_size, &old_root,
			(const unsigned char *)new_data, new_size, &new_root);
		if (rc != 0)
			goto out;
	}
	rc = DIVOOM_PORT_DIR_COMPARE_DIFFERENT;

out:
	dcrc_free(new_data);
	dcrc_free(old_data);
	dcrc_free(ctx);
	return rc;
}

static int dcrc_visit_manifest(const char *manifest, const char *root,
    divoom_port_dir_change_cb callback, void *user_data, int include_directories)
{
    dcrc_context_t *ctx;
    dcrc_manifest_node_t node;
    divoom_port_dir_compare_result_t result = {0};
    char *data = NULL;
    size_t size, root_len;
    const char *root_name;
    int rc;
    if (!callback) return DIVOOM_PORT_DIR_COMPARE_ERR_ARGUMENT;
    rc = dcrc_root_name(root, &root_name, &root_len);
    if (rc) return rc;
    ctx = dcrc_alloc(sizeof(*ctx));
    if (!ctx) return DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM;
    memset(ctx, 0, sizeof(*ctx));
    dcrc_crc32_init(ctx);
    rc = dcrc_load_manifest(ctx, manifest, &data, &size, &node);
    if (!rc && (node.name_len != root_len || memcmp(node.name, root_name, root_len)))
        rc = DIVOOM_PORT_DIR_COMPARE_ERR_FORMAT;
    if (!rc) {
        ctx->result = &result;
        ctx->include_directories = include_directories;
        ctx->callback = callback;
        ctx->user_data = user_data;
        rc = dcrc_emit_manifest_files(ctx, (unsigned char *)data, size, &node);
    }
    dcrc_free(data); dcrc_free(ctx);
    return rc;
}

int divoom_port_dir_manifest_visit(const char *manifest, const char *root,
    divoom_port_dir_change_cb callback, void *user_data)
{
    return dcrc_visit_manifest(manifest, root, callback, user_data, 0);
}

int divoom_port_dir_manifest_visit_paths(const char *manifest, const char *root,
    divoom_port_dir_change_cb callback, void *user_data)
{
    return dcrc_visit_manifest(manifest, root, callback, user_data, 1);
}

int divoom_port_file_verify(const char *path, uint64_t expected_size,
    uint32_t expected_crc, int relaxed)
{
    divoom_port_stat_t before, after;
    unsigned char *buffer;
    uint64_t bytes = 0, size;
    uint32_t crc;
    size_t n;
    void *fp;
    int ok;
    if (divoom_port_stat(path, &before) != 0 || before.is_dir || before.size < 0)
        return DIVOOM_PORT_DIR_COMPARE_ERR_IO;
    if (!relaxed || (uint64_t)before.size == expected_size) {
        int rc = divoom_port_file_crc32(path, &size, &crc);
        if (rc) return rc;
        return size == expected_size && crc == expected_crc ? 0 : DIVOOM_PORT_DIR_COMPARE_DIFFERENT;
    }
    buffer = dcrc_alloc(DIVOOM_PORT_DIR_CHECKSUM_READ_CHUNK);
    if (!buffer) return DIVOOM_PORT_DIR_COMPARE_ERR_NOMEM;
    fp = divoom_port_file_open(path, "rb");
    if (!fp) { dcrc_free(buffer); return DIVOOM_PORT_DIR_COMPARE_ERR_IO; }
    while ((n = divoom_port_file_read(buffer, 1, DIVOOM_PORT_DIR_CHECKSUM_READ_CHUNK, fp)) > 0) {
        bytes += n;
        if (bytes > (uint64_t)before.size) break;
    }
    ok = !ferror((FILE *)fp) && bytes == (uint64_t)before.size;
    if (divoom_port_file_close(fp) != 0) ok = 0;
    if (divoom_port_stat(path, &after) != 0 || after.size != before.size ||
        after.mtime != before.mtime) ok = 0;
    dcrc_free(buffer);
    return ok ? 0 : DIVOOM_PORT_DIR_COMPARE_ERR_IO;
}
