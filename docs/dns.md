# DNS 地址解析（11.1）

`dns.txh` 返回可用于 `socket` 模块的数值 IP 地址。Windows 使用系统 DNS 接口；Linux x86_64 使用 c-ares 异步解析，读取系统 DNS 和 hosts 配置。`resolve` 和 `resolve_with_cancel` 均为严格接口，解析失败时不返回普通空列表。

```tx
struct address
{
    ip: str
    family: str
    ttl_seconds: int
}

def resolve(host: str, timeout_ms: int) -> vector<address>
def resolve_with_cancel(host: str, timeout_ms: int,
                        token: cancel_token) -> vector<address>
```

`family` 为 `ipv4` 或 `ipv6`。DNS A/AAAA 记录的 `ttl_seconds` 取系统返回的剩余 TTL；IPv4/IPv6 字面量和 `localhost` 使用 `0`，表示没有可缓存的 DNS TTL。结果按 A、AAAA 及各自应答顺序排列，相同地址只保留一条，最多 128 条。调用方可以按 TTL 决定自己的缓存寿命，不应把 TTL 为零解释为永不过期。

`host` 为非空 UTF-8 主机名或数值 IP，不含端口、URL、路径及 NUL；`timeout_ms` 为 1～60000，限制 A 与 AAAA 两次查询的总时长。取消令牌可以来自 `cancel.source/token`，显式取消报 `cancelled_error/cancelled`，令牌截止时间报 `cancelled_error/deadline_exceeded`。查询已完成时保留成功结果；中途取消会请求系统取消并等待查询资源回收。DNS 不产生外部持久效果。

`io_error/name_not_found` 表示域名不存在；`io_error/no_records` 表示域名存在但无可用 A/AAAA；`io_error/timeout` 表示调用超时；`io_error/invalid_argument`、`invalid_utf8`、`size_limit` 表示输入无效；其他系统解析失败报 `io_error/operation_failed`。错误信息为中文，不包含系统环境中的敏感配置。Windows 和 Linux 使用相同错误码；Linux 实现与验证入口见[Linux 网络实现](linux_network.md)。

## 定向验证（2026-09-28）

Windows x64 增量构建通过。`tests/network/dns.tx` 编译并运行，覆盖 IPv4/IPv6 字面量、`localhost`、预取消及非法主机名；`examples/dns_lookup.tx` 编译运行并通过系统 DNS 取得 `example.com` 的 IPv4 地址与非负 TTL。外部 DNS 应答随当前网络配置变化，此示例不作为固定地址断言。
