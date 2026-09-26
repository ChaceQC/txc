# 密码学标准库设计

本文是 `crypto` 标准库的实现契约。第一版提供安全随机数、摘要、消息认证、密钥派生和带认证的对称加密。实现复用项目已经固定版本并静态链接的 Mbed TLS 3.6.5，不自行实现密码学原语。

## 公开接口

导入 `crypto.txh` 后可使用下列函数。输入和输出一律为原始 `bytes`；需要文本时由调用方显式使用 `encoding.encode`、`bytes.to_hex` 或 `bytes.to_base64`。`str` 不承载任意密文或原始密钥。

```tx
def random_bytes(count: int) -> bytes
def generate_key() -> bytes

def sha256(data: bytes) -> bytes
def sha512(data: bytes) -> bytes
def hmac_sha256(key: bytes, data: bytes) -> bytes
def secure_equal(left: bytes, right: bytes) -> bool

def hkdf_sha256(ikm: bytes, salt: bytes, info: bytes, length: int) -> bytes
def pbkdf2_sha256(password: bytes, salt: bytes, iterations: int, length: int) -> bytes

def encrypt(key: bytes, plaintext: bytes, aad: bytes) -> bytes
def decrypt(key: bytes, encrypted: bytes, aad: bytes) -> bytes
```

- `random_bytes` 使用操作系统熵源支撑的密码学安全随机数。`count` 允许 0～1 MiB；空请求返回空 `bytes`。`generate_key` 固定返回 32 个随机字节，用作 AES-256-GCM 密钥。现有 `random` 模块使用可设种子的伪随机数，不能生成密钥、nonce 或 salt。
- `sha256`、`sha512` 分别返回 32、64 字节摘要。`hmac_sha256` 返回 32 字节 MAC，允许空密钥和空消息，以保持标准算法语义。摘要不能代替带密钥的消息认证。
- `secure_equal` 对**长度相同**的字节值按内容恒定时间比较；长度不同时返回 `false`，长度本身不是保密信息。普通 `bytes == bytes` 不承诺恒定时间。
- `hkdf_sha256` 遵循 RFC 5869，`salt` 与 `info` 可以为空，`length` 为 1～8160，即 255 个 SHA-256 摘要块。它适用于已有高熵输入密钥材料的扩展与域分离，不用作用户密码哈希。
- `pbkdf2_sha256` 使用 HMAC-SHA256，`iterations` 为 1～10,000,000，`length` 为 1～1024。显式 salt 和迭代次数用于兼容已有协议或从密码导出加密密钥；新系统存储用户登录密码应另行采用 Argon2id。salt 应由 `random_bytes` 生成，调用方必须保存 salt 与参数。

## 认证加密格式

`encrypt` 使用 AES-256-GCM。密钥必须恰好为 32 字节；每次调用内部生成独立的 12 字节随机 nonce，认证标签为 16 字节。公开接口不允许调用方传 nonce，以免重复使用同一密钥和 nonce。长期或高频使用同一密钥时，应由应用控制密钥轮换；本模块不持久化每个密钥的使用次数。

返回值是可存储、可传输的单段二进制格式：

| 偏移 | 长度 | 内容 |
| --- | ---: | --- |
| 0 | 4 | ASCII `TXCG` |
| 4 | 1 | 格式版本 `1` |
| 5 | 1 | 算法编号 `1`，表示 AES-256-GCM |
| 6 | 12 | 随机 nonce |
| 18 | 可变 | 密文，与明文等长 |
| 末尾 | 16 | GCM 认证标签 |

固定开销为 34 字节，空明文合法。GCM 的附加认证数据为**前 18 字节完整头部后接调用方的 `aad`**；这样版本、算法编号、nonce 和调用方指定的上下文均受认证。`decrypt` 先检查最小长度、魔数、版本与算法编号，然后验证标签；只有认证成功才返回明文。密文、标签、头部或 `aad` 被改动时不能交付未经认证的明文。解密时必须传入与加密时完全相同的 `aad`；没有附加数据时传 `bytes.empty()`。

本格式只用于内存中的单段消息。大文件分块加密需要另行设计包含每块序号、唯一 nonce 和最终块标志的格式，不能直接对各块重复调用本接口。Hex/Base64 是编码，不提供保密性。

## 语言边界和错误语义

现有语言已经具备此接口需要的 `bytes`、`int`、模块导入、静态类型检查和 `try`/`exception`。本阶段无需增加字节字面量、隐式文本转换或新的语法。所有公开函数使用明确的 `bytes` 参数及返回类型；长度与迭代次数使用有符号 64 位 `int`，在进入 Mbed TLS 前检查范围并安全转换。编译期由函数签名确定调用目标，运行时只检查实际长度、格式、随机源与认证结果。

失败沿用可捕获的 `error.runtime_error`，`code` 取以下稳定值：

| code | 情况 |
| --- | --- |
| `invalid_argument` | 密钥长度、随机字节数、派生长度或迭代次数不合法 |
| `invalid_format` | 密文过短、魔数错误或不支持的版本、算法编号 |
| `authentication_failed` | GCM 标签验证失败，包括错误的密钥或 AAD |
| `random_failed` | 密码学随机源初始化或取数失败 |
| `operation_failed` | 其他底层密码学运算失败 |
| `size_limit` | 输入或输出长度超出运行时可表示的范围 |

错误消息使用中文且不能含密钥、明文、密码或密文内容。失败的解密缓冲必须清理，C++ 中暂存的敏感数据在退出时清零。现有 `bytes` 是不可变且可共享的拥有型值：传递或返回原始密钥后，运行时**无法保证**所有别名及释放后的内存都已清零。因此本阶段不宣称安全内存擦除，也不自动打印任何密钥。若以后需要可强制清零的长期密钥类型，应设计独立的不透明句柄，明确共享、释放、禁止打印及 `any` 装箱规则；不能把普通 `bytes` 伪称为该类型。密码学功能本身不依赖这个新语言类型。

## 实现与验证边界

`tx/stdlib/crypto.txh` 只包含公开签名。`src/stdlib/` 按随机数、摘要与 MAC、KDF、AEAD 分文件实现；`src/backend/cpp/` 提供相同名称的直接 C ABI，LLVM 后端声明并生成静态目标调用。PSA 随机源初始化与现有 TLS 服务端共用一次初始化函数；所有 Mbed TLS 上下文使用 RAII 清理。构建脚本将现有 `mbedcrypto` 静态依赖随标准库一起归档，不新增运行时密码学 DLL。

只做与本模块相关的验证：SHA-256/SHA-512、HMAC、HKDF、PBKDF2 和 AES-GCM 的公开标准向量；空输入、非法参数、不同密钥或 AAD、被修改的头部/密文/标签；一次 TX 接口编译运行。随机数验证只检查长度与错误路径，不用“两个结果不同”冒充随机性证明。全量回归及大文件格式不属于本阶段。

公钥签名、密钥交换和 Argon2id 密码存储留待独立契约；其中 Argon2id 需要另外引入经过维护的实现，不用 PBKDF2 冒充同等的密码存储接口。
