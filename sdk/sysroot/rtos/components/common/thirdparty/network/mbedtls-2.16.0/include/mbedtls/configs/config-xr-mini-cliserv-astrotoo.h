/* graph.microsoft.com 现发 ECDSA 证书，原精简配置只有 ECDHE-RSA，握手会 PK_TYPE_MISMATCH。 */
/* 原 config-xr-mini-cliserv.h 在共享盘上无法覆盖，先打开缺的宏再包含原配置。 */
#define MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
#define MBEDTLS_ECDSA_C
/* Microsoft 中间证书/ServerKeyExchange 常用 SHA-384，SHA-384 依赖 SHA512 模块。 */
#define MBEDTLS_SHA512_C
#include "config-xr-mini-cliserv.h"
