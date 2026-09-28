#ifndef TX_MBEDTLS_USER_CONFIG_H
#define TX_MBEDTLS_USER_CONFIG_H

/* MinGW 的 winpthread 为 PSA 全局 DRBG 和 Mbed TLS 共享状态提供互斥。 */
#define MBEDTLS_THREADING_C
#define MBEDTLS_THREADING_PTHREAD

#endif
