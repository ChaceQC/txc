#ifndef TX_MBEDTLS_USER_CONFIG_H
#define TX_MBEDTLS_USER_CONFIG_H

/* MinGW 的 winpthread 为 PSA 全局 DRBG 和 Mbed TLS 共享状态提供互斥。 */
#define MBEDTLS_THREADING_C
#define MBEDTLS_THREADING_PTHREAD

/* 使用依赖自带的 Curve25519 实现，保持原曲线和证书验证契约。 */
#define MBEDTLS_ECDH_VARIANT_EVEREST_ENABLED

#endif
