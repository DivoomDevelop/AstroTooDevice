/* OTA 专用 miniz：符号与 eyesee-mpp tinfl 隔离，保证 zip 与 inflate 同版本 */
#define divoom_ota_mz_sym(name) divoom_ota_##name
#define tinfl_decompress divoom_ota_mz_sym(tinfl_decompress)
#define tinfl_decompress_mem_to_heap divoom_ota_mz_sym(tinfl_decompress_mem_to_heap)
#define tinfl_decompress_mem_to_mem divoom_ota_mz_sym(tinfl_decompress_mem_to_mem)
#define tinfl_decompress_mem_to_callback divoom_ota_mz_sym(tinfl_decompress_mem_to_callback)
#define tinfl_decompressor_alloc divoom_ota_mz_sym(tinfl_decompressor_alloc)
#define tinfl_decompressor_free divoom_ota_mz_sym(tinfl_decompressor_free)

#include "miniz_tinfl.c"
#include "miniz_zip.c"
