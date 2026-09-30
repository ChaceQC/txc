# TLS 身份、验证与安全流（9.6、11.3）

`tls.txh` 定义证书身份、信任、双向认证配置与 `socket.tcp_stream` 上的安全流。9.6 完成配置及离线证书验证，11.3 完成握手、ALPN 和加密收发。现有 `httpx`/`websocket` 接口仍使用各自的网络路径。

## 公开接口

```tx
struct identity
{
    id: int
}

struct trust
{
    system_roots: bool
    anchors: vector<bytes>
}

struct client_config
{
    hostname: str
    trust: trust
    client_identity: identity
}

struct server_config
{
    identity: identity
    client_trust: trust
    require_client_identity: bool
}

struct verification
{
    status: str
    revocation: str
    chain: vector<bytes>
}

def system_trust() -> trust
def custom_trust(anchors: vector<bytes>, include_system: bool) -> trust
def import_identity(package: bytes, password: secret_bytes) -> identity
def close_identity(value: identity) -> void
def client(hostname: str, roots: trust) -> client_config
def with_client_identity(config: client_config, value: identity) -> client_config
def server(value: identity, client_roots: trust,
           require_client_identity: bool) -> server_config
def verify_server(config: client_config, leaf: bytes,
                  intermediates: vector<bytes>) -> verification
def verify_client(config: server_config, leaf: bytes,
                  intermediates: vector<bytes>) -> verification
```

`system_trust()` 在 Windows 启用当前用户与本机的系统根证书，在 Linux 启用 OpenSSL 默认 CA 文件与目录（由发行版 `ca-certificates` 提供）；`custom_trust` 校验每张 DER 锚，并可选择是否同时使用系统根证书。没有系统根证书且自定义锚为空的信任配置被拒绝。自定义锚是明确授予的信任，调用方须保护其来源；传入的向量会复制，不会因后续修改而更改配置。配置没有关闭验证的开关。

`import_identity` 接收 PKCS#12 包及 `secret_bytes` 密码；应用应优先使用加密包。证书与私钥由同一包关联；首张证书为持有私钥的叶证书，其余为附带证书。私钥仅保存在运行时的不透明身份记录中，不作为结构体字段、`any` 或可打印值暴露。普通 `bytes` 中的 PKCS#12 输入仍由调用方负责避免输出。`identity.id` 只用来引用当前进程内的记录；即使结构体被序列化，编号也不能跨进程或重启重建身份，应用不得将它当作可持久化凭据。`close_identity` 可重复调用；关闭使全部复制的同一编号失效，并清零内部密钥。进程退出时未关闭的记录由运行时清理。已建立安全流的身份存活规则见[11.3 安全流契约](#113-安全流契约)。

`client` 要求非空的目标主机名，不接受 URL 或端口；该名称必须在后续握手中同时用于 SNI 和证书验证。默认没有客户端身份，`client_identity.id=0` 表示缺省；`with_client_identity` 添加已有身份。`server` 必须提供身份。`require_client_identity=true` 表示握手时必须收到客户端证书；若为 `false`，可以不发送，但只要提供证书就必须验证。身份句柄在构造和验证时均检查是否仍有效。配置结构的字段即使被调用方改写，验证入口也会重新检查，不把构造函数视为安全边界。

TLS 随机输入复用密码学模块已有、带线程互斥的 PSA CSPRNG，每次握手仍生成新随机数。
Mbed TLS 的 Curve25519 ECDH 启用随依赖提供的 Project Everest 已验证实现；
曲线、密码套件和验证策略不变，其余曲线继续使用原实现。
同一身份最多保留四份闲置的已解析证书/私钥状态，每份独占租给一个活动连接，避免
私钥盲化等内部状态在并发握手间共享。关闭身份立即清空闲置状态及秘密字节；活动流
持有的独立状态在流关闭时销毁，不再归还缓存。每条连接仍完整验证当前证书链、主机名、
用途、有效期和 ALPN，不缓存握手成功结论，也不复用随机输出。

`verify_server` 使用 `x509.verify(..., hostname, "server_auth", ...)`；`verify_client` 使用 `x509.verify(..., "", "client_auth", ...)`。仅当链、用途、主机名和当前有效期均合格时返回 `status="valid"`；其他结果抛出 `security_error`，`code` 为 `hostname_mismatch`、`wrong_purpose`、`unknown_issuer`、`expired`、`not_yet_valid` 或 `invalid_chain`。没有证书时报告 `certificate_required`。格式和参数错误沿用 X.509 的稳定错误码。返回的 `revocation="not_checked"` 明确表示没有 CRL/OCSP 查询；需要撤销检查的调用方必须拒绝该状态。验证入口不收发数据，不应被当作已完成的 TLS 握手。

X.509 链后端在 Windows 使用 CryptoAPI，在 Linux 使用 OpenSSL 3；两者统一接入 Mbed TLS 安全流。TLS 1.2/1.3、ALPN、握手失败清理和未经验证的数据不交付见[11.3 安全流契约](#113-安全流契约)。

## 9.6 定向验证（2026-09-27）

Windows x64 构建通过。`scripts/check_tls.py` 临时生成根证书、无关根证书、服务端和客户端证书及加密 PKCS#12，编译运行 `tests/crypto/tls.tx` 和 `examples/tls_config.tx`。用例覆盖有效的双向证书验证，错误主机名、用途、签发者、缺失证书、空或畸形信任锚、调用方改写配置后的重新检查、错误密码以及身份关闭后拒绝复用。证书仅保存在测试临时目录，结束后删除。未进行网络握手或跨平台测试。

## 11.3 安全流契约

```tx
import "socket.txh" as socket

struct secure_stream
{
    id: int
}

struct read_result
{
    data: bytes
    eof: bool
}

def connect(peer: socket.tcp_stream, config: client_config,
            alpn: vector<str>, timeout_ms: int) -> secure_stream
def accept(peer: socket.tcp_stream, config: server_config,
           alpn: vector<str>, timeout_ms: int) -> secure_stream
def read(peer: secure_stream, max_bytes: int,
         timeout_ms: int) -> read_result
def write(peer: secure_stream, data: bytes, timeout_ms: int) -> int
def negotiated_alpn(peer: secure_stream) -> str
def close(peer: secure_stream, timeout_ms: int) -> void
```

`connect/accept` 成功前，原始 `socket.tcp_stream` 会被消费，所有旧别名失效。配置和 ALPN 输入错误在消费前拒绝；开始握手后失败会关闭底层连接，绝不交付半建立的安全流。`secure_stream` 也不满足 `Send/Sync`，复制的 TX 结构体共享关闭状态；重复 `close` 无效果。`close_identity` 在握手完成后不撤销已建立的安全流，因为 TLS 上下文已持有自己的证书/私钥状态。最后一个流句柄释放或进程退出时兜底清理。

客户端的 `client_config.hostname` 同时用于 SNI 和证书主机名验证；系统或自定义锚通过原有 `trust` 生效。服务端使用 `server_config.identity`，按配置请求及验证客户端证书；客户端证书可选时，收到的证书仍必须有效。握手完成后调用现有 X.509 链验证，只有用途、有效期、主机名及信任链全部合格才返回流，不交付验证前应用数据。撤销仍为 `not_checked`。支持 TLS 1.2/1.3；ALPN 列表最多 16 个协议，每项为 1～255 个可见 ASCII 字节且无 NUL，空列表表示不协商。本端检测到没有共同协议时报 `security_error/alpn_mismatch`；若对端先发终止警报，则报 `security_error/handshake_failed`。`negotiated_alpn` 返回选中协议或空字符串。

`read` 单次至多返回 `min(max_bytes, 16 KiB)` 字节，短读正常；`eof=true` 仅代表收到 TLS `close_notify`。底层 TCP 意外 EOF 报 `security_error/truncated_close`。`write` 单次至多尝试 16 KiB，返回确认写入字节数；应用循环发送余下数据。`max_bytes` 限 1～16 MiB，写入数据限 16 MiB。超时为 1～60000 毫秒，限制整个握手、读、写或关闭调用。`close` 发送 `close_notify` 并关闭底层连接，不等待对端回应；若发送超时或失败，仍把句柄标记关闭并报告错误。应用数据失败后安全流转为失败/关闭状态，不能继续复用。

证书错误沿用 `security_error/hostname_mismatch`、`wrong_purpose`、`unknown_issuer`、`expired`、`not_yet_valid`、`invalid_chain`、`certificate_required`；握手协议错误使用 `handshake_failed`，非正常截断使用 `truncated_close`。网络等待超时为 `io_error/timeout`，关闭后为 `io_error/connection_closed`。绝不提供关闭证书验证的开关。

## 11.3 定向验证（2026-09-28）

Windows x64 增量构建通过。`scripts/check_tls_stream.py` 使用临时生成的根证书、服务端/客户端证书和 PKCS#12，验证客户端与服务端双向握手、ALPN、二进制数据收发、`close_notify` EOF、握手后关闭身份仍可使用安全流，以及错误主机名、无关 CA、缺客户端证书和 ALPN 不匹配的拒绝路径。`tests/network/tls_preflight.tx` 验证非法 ALPN 在消费 TCP 前拒绝；`tests/network/tls_bad_send.tx` 在源码位置拒绝跨线程传递安全流。原 9.6 的 `scripts/check_tls.py` 同轮复核通过。验证使用本机回环与自定义 CA；系统根的实际公网握手和非 Windows 平台不在本次定向范围内。证书撤销仍未检查。
