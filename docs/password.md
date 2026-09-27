# Argon2id 密码存储

`password.txh` 使用官方 Argon2 参考实现的 Argon2id v=19。密码必须以 `secret_bytes` 传入；哈希结果是可存储的 PHC 字符串，不包含原始密码。

```tx
def hash_password(value: secret_bytes) -> str
def hash_password_with_params(value: secret_bytes, memory_kib: int,
                              iterations: int, parallelism: int) -> str
def verify_password(value: secret_bytes, encoded: str) -> bool
def needs_rehash(encoded: str, memory_kib: int, iterations: int,
                 parallelism: int) -> bool
```

默认新哈希使用 64 MiB、3 轮、1 路并行、独立的 16 字节密码学随机 salt 和 32 字节标签。创建新哈希时参数范围为内存 19～256 MiB、轮数 2～10、并行度 1～4。`hash_password_with_params` 用于应用按机器能力选择参数；仅接受不低于最低安全参数且不超出资源上限的组合。

`verify_password` 对密码不匹配返回 `false`。它接受受限的 Argon2id v=16 或 v=19 PHC 字符串，先解析并限制内存、轮数、并行度、salt 与标签长度，再调用 Argon2；支持较弱旧参数，以便成功验证后用新参数迁移。`needs_rehash` 验证 PHC 结构并在算法版本或参数与指定目标不同时返回 `true`；应用可在登录成功后生成新哈希并原子更新存储。新哈希的 PHC 字符串示例格式为 `$argon2id$v=19$m=65536,t=3,p=1$...$...`，salt 每次独立生成。

参数不合法报 `security_error/invalid_argument`，PHC 结构或编码错误报 `security_error/invalid_format`，随机源失败报 `security_error/random_failed`，底层计算失败报 `security_error/operation_failed`。所有错误消息不包含密码或 PHC 内容。PHC 字符串本身不保密，但不应接受无限长的外部输入。`secret_bytes` 关闭规则见[秘密字节](secret_bytes.md)。现有 `crypto.pbkdf2_sha256` 保留用于已有协议兼容，不作为新密码存储默认值。

## 9.2 实施记录（2026-09-27）

官方 Argon2 参考实现固定在 `20190702` 的 Git 提交 `62358ba2123abd17fccf2a108a301d4b52c01a7c`，以静态库归档，并随工具链交付 `ARGON2-LICENSE`。默认新哈希及自定义参数入口使用每次独立生成的 16 字节 salt；验证外来 PHC 字符串时先限制整体长度、算法、版本、计算参数、salt/标签长度和 Base64 编码，再调用 Argon2。参考实现默认清除内部内存，原始密码继续由 `secret_bytes` 持有。

增量构建通过。`tests/crypto/password.tx` 运行输出 `PASSWORD_OK`，覆盖 argon2-cffi 25.1.0 产生的固定 v=19/v=16 PHC 向量、错误密码、参数升级、生成后验证、超限输入和错误参数；`examples/password.tx` 编译运行通过。没有把两次 salt 不同当作随机源质量证明。安全随机源故障注入留待 9.7；Argon2 版本升级仍需重新审核密码参数和资源上限。
