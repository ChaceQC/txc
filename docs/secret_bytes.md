# 秘密字节

`secret_bytes` 是不透明的受控字节缓冲，由 `secret.txh` 暴露以下入口：

```tx
def from_bytes(data: bytes) -> secret_bytes
def random(count: int) -> secret_bytes
def to_bytes(value: secret_bytes) -> bytes
def size(value: secret_bytes) -> int
def equal(left: secret_bytes, right: secret_bytes) -> bool
def close(value: secret_bytes) -> void
```

长度允许 0～1 MiB。`from_bytes` 复制输入，`random` 直接填充受控缓冲。`to_bytes` 明确复制出普通 `bytes`；该副本可打印、共享和序列化，不再受到清零保证。不要把已经暴露在普通 `bytes` 的原始凭据误认为已被清除。`equal` 在长度相等时按内容恒定时间比较；长度不保密。

普通赋值、返回及参数传递共享同一缓冲。`close` 可重复调用，首次调用清零实际字节并使所有别名失效；最后一个别名释放时同样清零。关闭后除再次 `close` 外的访问抛出 `security_error/invalid_state`。输入过长或长度为负报 `security_error/size_limit`；随机源失败报 `security_error/random_failed`。错误消息不包含秘密内容。

`secret_bytes` 不能打印、装入 `any`、使用默认 `deep_copy`、作为结构体/类字段或容器元素，也不能进入 JSON/CSV/XML/CBOR/serde 和日志的动态值入口。密码与密钥处理模块应直接接收 `secret_bytes`，避免常态导出。

运行时尽力锁定缓冲所在内存页，释放时先用不易被优化掉的清零函数擦除，再解除锁定。平台可能因权限或配额拒绝锁页；换页、休眠、崩溃转储、其他进程拷贝和导入前的普通 `bytes` 不在此类型的控制范围。当前类型尚未通过跨线程释放验证，因此不满足 `Send/Sync`。

## 9.1 实施记录（2026-09-27）

已交付 `secret.txh`、C++23 缓冲、直接 C ABI、LLVM 调用、静态类型限制和示例。增量构建通过；`tests/crypto/secret_bytes.tx` 运行输出 `SECRET_OK`，覆盖导入/导出、别名关闭、关闭后错误、随机空值与越界。四个定向错误样例分别在源码位置拒绝 `any`、打印、`deep_copy` 和结构体字段；`examples/secret_bytes.tx` 编译运行通过。缓冲清零使用 Mbed TLS 的 `mbedtls_platform_zeroize`，Windows 使用 `VirtualLock` 尽力锁页，锁页失败时仍保持明确的清零和错误语义。平台换页与崩溃转储限制如上；随机源故障注入和跨线程资源清理留到 9.7/2.6 验收。
