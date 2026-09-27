# Ed25519 与 X25519 公钥密码学

`public_key.txh` 使用固定版本的 libsodium 实现 RFC 8032 Ed25519 和 RFC 7748 X25519。私钥由现有不透明 `secret_bytes` 句柄持有；共享、显式关闭、清零和平台限制遵循[秘密字节](secret_bytes.md)。模块不会把私钥隐式装入普通 `bytes`、`any` 或日志。两个算法的私钥都是 32 字节原始值，调用时按算法解释；应用应分别保存和标记，不要复用同一私钥材料。

```tx
def ed25519_generate() -> secret_bytes
def ed25519_import_seed(seed: bytes) -> secret_bytes
def ed25519_public(private_key: secret_bytes) -> bytes
def ed25519_sign(private_key: secret_bytes, message: bytes) -> bytes
def ed25519_verify(public_key: bytes, message: bytes, signature: bytes) -> bool
def x25519_generate() -> secret_bytes
def x25519_import_private(raw: bytes) -> secret_bytes
def x25519_public(private_key: secret_bytes) -> bytes
def x25519_derive(private_key: secret_bytes, peer_public: bytes,
                  salt: bytes, info: bytes) -> secret_bytes
```

Ed25519 公钥使用 RFC 8032 的 32 字节压缩点编码，签名是 64 字节 `R || S`，私钥导入/显式导出采用 32 字节 seed。X25519 公钥和私钥采用 RFC 7748 的 32 字节 little-endian u 坐标和私有标量编码；导入的私有标量在运算时按 RFC 7748 钳位。外层文本可显式使用 `bytes.to_base64`/`from_base64` 或 Hex，不接受隐式编码转换。

`x25519_derive` 不公开裸共享秘密。底层拒绝低阶公钥与全零结果，随后以 HKDF-SHA256、调用方的 `salt`/`info` 导出固定 32 字节密钥并直接放入 `secret_bytes`。协议应使用独立随机 salt、在 `info` 中绑定协议版本与双方身份；不得把交换结果直接作为长期密钥。签名验证仅在编码长度正确且签名有效时返回 `true`；长度错误报 `security_error/invalid_argument`，有效长度但验证不通过返回 `false`。低阶 X25519 公钥报 `security_error/invalid_key`，底层失败报 `security_error/operation_failed`，随机源失败报 `security_error/random_failed`。关闭后的私钥报 `security_error/invalid_state`。错误消息不含密钥或消息内容。

新密码学原语由 libsodium 静态归档，旧 `crypto` 的 Mbed TLS 接口保持兼容。跨库互操作、随机源故障注入和跨平台总验收由 9.7 统一完成。

## 9.4 实施记录（2026-09-27）

固定使用 MSYS2 MinGW x64 的 libsodium `1.0.22-3` 静态包，SHA-256 为 `e8d8bc169fa122eccfc3e4252615937a62fa0bd6ca21ed4912bac48d6ed2f870`，构建后交付 ISC 许可证。为兼容现有 Argon2 参考实现，构建时将其重名的 Argon2/BLAKE2 符号设为项目私有名；当前 CRT 不导出 `memset_explicit`，由专用实现用 volatile 写入完成擦除。生成程序的导入表没有 libsodium DLL。

Windows x64 完整与增量构建成功。`tests/crypto/public_key.tx` 和 `examples/public_key.tx` 编译运行通过，包含 RFC 8032 Ed25519 空消息签名/公钥向量、RFC 7748 X25519 公钥向量、双方交换后同密钥、错误签名、低阶公钥与关闭句柄。`tests/crypto/password.tx` 通过，确认 Argon2 旧接口仍可链接运行。未运行与本项无关的大套件；跨库、故障注入与跨平台验证留在 9.7。
