# TLS 身份与验证配置（9.6）

`tls.txh` 定义后续 `socket/httpx` 共用的证书身份、信任和双向认证配置。当前只提供配置构造及对已取得的对端证书进行验证；安全流、握手、SNI、ALPN 和应用数据收发在 11.3 接入。现有 `httpx`/`websocket` 接口不因本模块自动改用新配置。

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

`system_trust()` 启用当前 Windows 用户与本机的系统根证书；`custom_trust` 校验每张 DER 锚，并可选择是否同时使用系统根证书。没有系统根证书且自定义锚为空的信任配置被拒绝。自定义锚是明确授予的信任，调用方须保护其来源；传入的向量会复制，不会因后续修改而更改配置。配置没有关闭验证的开关。

`import_identity` 接收 PKCS#12 包及 `secret_bytes` 密码；应用应优先使用加密包。证书与私钥由同一包关联；首张证书为持有私钥的叶证书，其余为附带证书。私钥仅保存在运行时的不透明身份记录中，不作为结构体字段、`any` 或可打印值暴露。普通 `bytes` 中的 PKCS#12 输入仍由调用方负责避免输出。`identity.id` 只用来引用当前进程内的记录；即使结构体被序列化，编号也不能跨进程或重启重建身份，应用不得将它当作可持久化凭据。`close_identity` 可重复调用；关闭使全部复制的同一编号失效，并清零内部密钥。进程退出时未关闭的记录由运行时清理。建立长期 TLS 流时的身份存活规则将在 11.3 固定。

`client` 要求非空的目标主机名，不接受 URL 或端口；该名称必须在后续握手中同时用于 SNI 和证书验证。默认没有客户端身份，`client_identity.id=0` 表示缺省；`with_client_identity` 添加已有身份。`server` 必须提供身份。`require_client_identity=true` 表示握手时必须收到客户端证书；若为 `false`，可以不发送，但只要提供证书就必须验证。身份句柄在构造和验证时均检查是否仍有效。配置结构的字段即使被调用方改写，验证入口也会重新检查，不把构造函数视为安全边界。

`verify_server` 使用 `x509.verify(..., hostname, "server_auth", ...)`；`verify_client` 使用 `x509.verify(..., "", "client_auth", ...)`。仅当链、用途、主机名和当前有效期均合格时返回 `status="valid"`；其他结果抛出 `security_error`，`code` 为 `hostname_mismatch`、`wrong_purpose`、`unknown_issuer`、`expired`、`not_yet_valid` 或 `invalid_chain`。没有证书时报告 `certificate_required`。格式和参数错误沿用 X.509 的稳定错误码。返回的 `revocation="not_checked"` 明确表示没有 CRL/OCSP 查询；需要撤销检查的调用方必须拒绝该状态。验证入口不收发数据，不应被当作已完成的 TLS 握手。

当前 X.509 链后端只在 Windows x64 实现。其他平台对需要证书解析或验证的操作报告 `security_error/unsupported_platform`。TLS 1.2/1.3、密码套件、ALPN、握手失败清理和未经验证的数据不交付由 11.3 实现并验证。

## 9.6 定向验证（2026-09-27）

Windows x64 构建通过。`scripts/check_tls.py` 临时生成根证书、无关根证书、服务端和客户端证书及加密 PKCS#12，编译运行 `tests/crypto/tls.tx` 和 `examples/tls_config.tx`。用例覆盖有效的双向证书验证，错误主机名、用途、签发者、缺失证书、空或畸形信任锚、调用方改写配置后的重新检查、错误密码以及身份关闭后拒绝复用。证书仅保存在测试临时目录，结束后删除。未进行网络握手或跨平台测试。
