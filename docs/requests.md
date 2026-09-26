# `requests` 标准库设计与接口

本文先确定 `requests` 的契约，再按此契约实现。模块名采用 Python 的正确拼写 `requests`，通过 `import "requests.txh" as requests` 使用。公开接口放在 `tx/stdlib/requests.txh`；实现以 `.tx` 编写，源码放在 `src/stdlib/`，构建后并入 `tx/libtxstdlib.a`。`tx/stdlib/` 交付时仍只包含 `.txh`。

## 定位与兼容范围

`requests` 是建立在现有 `httpx` 之上的同步 HTTP 客户端。默认接口尽量贴近 Python Requests 的常用动作：`request/get/post/put/patch/delete/head/options`、请求参数、头、正文、JSON、表单、Basic 认证、Cookie、重定向、超时、会话以及响应辅助操作。公开命名参数按 Python Requests 使用 `params/data/json/headers/cookies` 等名称。TX 的默认参数规则允许 `params: dict = none`、`timeout: any = none` 这样的签名；便捷函数沿用 Python 各自的显式参数与 `**kwargs`。`session`、`response` 是类，分别通过 `requests.session()` 和请求函数创建，再调用对象方法。

HTTP/1.1 和显式 HTTP/2 均使用 `httpx`。状态码 4xx/5xx 会返回响应，只有传输、协议、编码和调用参数错误直接进入可捕获的 `io_error`。`raise_for_status` 可将 4xx/5xx 显式变为 `io_error`。TLS 默认按系统信任链验证；不提供关闭证书验证的开关。

| Python Requests 能力 | TX 设计 | 本次目标 |
| --- | --- | --- |
| `request` 与八种便捷方法 | 显式默认参数、`**kwargs` 和 `request_options` 重载 | 实现 |
| `params`、`headers`、`data`、`json` | 同名命名实参及可复用的 `request_options` | 实现 |
| `Response.status_code/url/headers/content/ok/history/cookies/elapsed` | `response` 字段 | 实现 |
| `Response.text/json()/raise_for_status()` | `response` 类方法 | 实现 |
| `Session`、默认头、Cookie 保留 | `requests.session()` 和类方法 | 实现 |
| 重定向与认证头保护 | 有上限的自动跳转，同源继承认证头 | 实现 |
| 文件下载、上传和大正文 | `download/upload` 接入 `httpx` 的 `binary_stream` | 部分实现，限制见下文 |
| `stream=True` 的惰性响应对象与 `iter_content` | 现有 `httpx` 只支持把流直接写入文件 | 暂不提供迭代式响应 |
| 连接池、`HTTPAdapter`、自定义重试策略 | `httpx` 客户端每次请求建立会话 | 暂不提供 |
| 显式代理、自定义 CA、客户端证书、Digest/NTLM、OAuth | 需要扩充底层传输及认证模块 | 暂不提供；不得伪装为已支持 |
| `files` multipart | `dict` 中的字节、文本或文件元组，内存正文限 8 MiB | 实现常用形式 |
| `Request/PreparedRequest`、hooks 和完整 CookieJar | 需要进一步确定 TX 对象/回调契约 | 暂不提供 |

上表的“暂不提供”指 Python API 的这些部分不会在本次交付中被悄悄模拟。若扩充底层接口，应保持已有 `httpx` 调用的行为兼容，并在此文档与 `network.md` 同步限制。

## 公共类型

```tx
struct request_options
{
    params: map<str, str>
    headers: map<str, str>
    cookies: map<str, str>
    body: bytes
    body_type: str
    timeout_ms: int
    allow_redirects: bool
    max_redirects: int
    http2: bool
    basic_auth: bool
    auth_user: str
    auth_password: str
}

class response
{
    public:
        status_code: int
        url: str
        headers: map<str, str>
        set_cookie: vector<str>
        cookies: map<str, str>
        content: bytes
        history: array
        elapsed_ms: int
        ok: bool

        def text() -> str
        def text(encoding: str) -> str
        def json() -> any
        def json_object() -> dict
        def raise_for_status() -> void
}

class session
{
    public:
        headers: map<str, str>
        cookies: array
        timeout_ms: int
        allow_redirects: bool
        max_redirects: int
        http2: bool
        basic_auth: bool
        auth_user: str
        auth_password: str

        def init() -> void
        def request(method: str, url: str,
                    params: dict = none, data: any = none,
                    headers: dict = none, cookies: dict = none,
                    files: any = none, auth: array = none,
                    timeout: any = none, allow_redirects: bool = none,
                    proxies: any = none, hooks: any = none,
                    stream: bool = none, verify: bool = none,
                    cert: any = none, json: any = none,
                    **kwargs: dict) -> response
        def request(method: str, url: str, options: request_options) -> response
        def get(url: str, **kwargs: dict) -> response
        def get(url: str, options: request_options) -> response
        def post(url: str, data: any = none, json: any = none, **kwargs: dict) -> response
        def post(url: str, options: request_options) -> response
        def put(url: str, data: any = none, **kwargs: dict) -> response
        def put(url: str, options: request_options) -> response
        def patch(url: str, data: any = none, **kwargs: dict) -> response
        def patch(url: str, options: request_options) -> response
        def delete(url: str, **kwargs: dict) -> response
        def delete(url: str, options: request_options) -> response
        def head(url: str, **kwargs: dict) -> response
        def head(url: str, options: request_options) -> response
        def options(url: str, **kwargs: dict) -> response
        def options(url: str, options: request_options) -> response
}
```

`default_options()` 返回空参数、空头、空 Cookie、无正文、30 秒超时、最多 10 次重定向、自动跟随重定向和 HTTP/1.1；`requests.session()` 由 `init()` 设置同样的默认值。`body_type` 限 `none/raw/form/json`，由 `with_*` 函数维护；传输时不会依据正文内容猜测类型。`response.history` 是跳转链中先前响应的快照数组；`response.content` 始终是原始字节。`ok` 等价于状态码小于 400，不表示 JSON 解析成功。

`session` 的 Cookie 数组由模块维护，元素记录名称、值、域、路径、安全属性及过期时间；不应手动构造其中的条目。独立 `request` 不会跨调用保留 Cookie；`se.request(...)` 等类方法会保存响应中的 Cookie，并仅向匹配域、路径和协议的请求发送。`request_options.cookies` 是本次请求的显式覆盖，不写回会话。HTTP 头名称比较不区分 ASCII 大小写。

## 请求 API

```tx
def default_options() -> request_options
def with_text(options: request_options, body: str) -> request_options
def with_bytes(options: request_options, body: bytes) -> request_options
def with_form(options: request_options, fields: map<str, str>) -> request_options
def with_json(options: request_options, value: dict) -> request_options
def with_json(options: request_options, value: array) -> request_options

def request(method: str, url: str, **kwargs: dict) -> response
def request(method: str, url: str, options: request_options) -> response
def get(url: str, params: dict = none, **kwargs: dict) -> response
def get(url: str, options: request_options) -> response
def post(url: str, data: any = none, json: any = none, **kwargs: dict) -> response
def post(url: str, options: request_options) -> response
def put(url: str, data: any = none, **kwargs: dict) -> response
def put(url: str, options: request_options) -> response
def patch(url: str, data: any = none, **kwargs: dict) -> response
def patch(url: str, options: request_options) -> response
def delete(url: str, **kwargs: dict) -> response
def delete(url: str, options: request_options) -> response
def head(url: str, **kwargs: dict) -> response
def head(url: str, options: request_options) -> response
def options(url: str, **kwargs: dict) -> response
def options(url: str, options: request_options) -> response

```

`Session.request` 按 Python Requests 2.32.3 的参数顺序声明 `params, data, headers, cookies, files, auth, timeout, allow_redirects, proxies, hooks, stream, verify, cert, json`；未提供时保持 `none`，以区别于显式传入空字典、空正文或 `false`。模块级 `get` 的 `params: dict = none`、`post` 的 `data/json` 以及 `put/patch` 的 `data` 也采用 Python 对应签名。便捷函数的其他参数走 `**kwargs`。没有 `body`、`bodys`、`jsons` 这些公开参数。未知名称和冲突组合报 `io_error/invalid_argument`，已识别但尚未实现的能力报 `io_error/unsupported_option`。

| 命名实参 | 接受的值 | 作用 |
| --- | --- | --- |
| `headers`、`cookies`、`params` | `Session.request` 的显式参数为 `dict = none`；便捷函数的 `**kwargs` 接受 `dict` 或 `map<str, str>` | 请求头、Cookie、查询参数 |
| `data` | `str`、`bytes`；字符串键的 `dict` 作为表单 | 请求正文 |
| `json` | JSON 可序列化值 | JSON 正文；不能与 `data` 同时提供 |
| `files` | 字符串键的 `dict`；值为 `bytes`、`str` 或 `[文件名, 内容, 可选类型]` 数组 | `multipart/form-data`；可与字段字典 `data` 合用 |
| `timeout` | `int` 或 `float` 秒 | 各网络阶段的超时；TX 扩展 `timeout_ms` 接受整数毫秒 |
| `max_redirects` | `int` | 重定向上限，TX 扩展 |
| `allow_redirects`、`http2` | `bool` | 跳转和协议选择 |
| `auth` | `[用户名, 密码]` 的两元素 `array` | Basic 认证；Digest 等认证方案另行实现 |
| `stream`、`verify` | `bool` | `stream=false`、`verify=true` 可用；惰性流读取和关闭 TLS 验证不支持 |
| `proxies`、`hooks`、`cert` | Python 原名 | 识别名称并报告 `unsupported_option`，不静默忽略 |

`params` 与表单字段按 UTF-8 字节进行 RFC 3986 百分号编码；查询参数追加到原 URL 的现有查询之后，URL 片段在发送前去掉。Cookie 名和值按原文本拼接，由底层头字段校验拒绝 CR/LF。表单设置 `application/x-www-form-urlencoded`，空格编码为 `+`；JSON 正文使用标准库 `json.stringify`，设置 `application/json; charset=utf-8`。用户提供的 `Content-Type` 优先。字符串正文使用 UTF-8，字节正文保持原始内容。

`files` 在内存中组装 multipart；每个字段名、文件名和媒体类型拒绝 CR、LF、双引号与反斜杠，防止写入额外的 MIME 头。未指定文件名时使用字段名加 `.bin`，未指定媒体类型时使用 `application/octet-stream`。文件元组的内容接受 `bytes` 或 `str`；`binary_stream` 可从当前游标读取到 EOF，但不会由 `requests` 关闭。`files` 不能和 `json` 合用；带 `files` 的 `data` 只接受字段字典。调用方不能自定与实际边界不一致的 `Content-Type`。组装后的正文仍受 `httpx` 的 8 MiB 上限约束，大文件使用下文的流式 `upload`。

`timeout_ms` 必须大于零，按底层接口分别约束连接、发送与接收阶段，不是整条重定向链的总时限。普通内存请求/响应仍受 `httpx` 的 8 MiB 正文限制；流式 API 使用调用方的明确上限。默认头不携带密钥；Basic 认证只在显式配置时构造 `Authorization`。传入的敏感值不写入诊断。

自动重定向处理 301、302、303、307、308；303 改为 GET，301/302 的 POST 按 Requests 惯例改为 GET，307/308 保留方法和正文。`HEAD` 默认不跟随重定向，其余便捷方法默认跟随。`Location` 可以是绝对 URL、根路径或相对路径；相对路径中的 `.`、`..` 目前未规范化。超过 `max_redirects`、循环或无效 `Location` 报 `io_error`；跨源跳转移除 `Authorization` 和显式 `Cookie`，重新按目标域匹配会话 Cookie。底层 `httpx` 拒绝 URL 内用户信息。HTTPS 降级到 HTTP 时也不转发认证信息。跳转链中的请求与响应都受同样的超时和大小限制。

## 响应 API

`response` 提供 `text()`、`text(encoding)`、`json()`、`json_object()` 和 `raise_for_status()` 方法，签名见前面的类声明。

`text` 默认严格按 UTF-8 解码；显式编码使用 `encoding` 模块当前支持的字符集。无效字节不会静默替换。`json` 先按 UTF-8 解码，再用 `json.parse`，合法 JSON `null` 返回 `none`，解析失败为 `parse_error`。`raise_for_status` 对 400～599 报 `io_error/http_error`，其他状态不报错。`response.cookies` 是该响应 `Set-Cookie` 的名称和值快照，不代表完整 Cookie 属性；完整匹配规则由会话内部维护。

`response.text()` 是方法，Python Requests 的 `Response.text` 是属性；TX 当前没有属性 getter。正文不自动解压缩，也不依据 `Content-Type` 猜测字符集。Cookie 保存 `Max-Age`，暂不解析 HTTP 日期形式的 `Expires`；没有公共后缀列表，使用 Domain Cookie 时应限定受信任站点。

## 文件流操作

`download(url, destination, max_bytes)` 及带 `request_options` 的重载调用 `httpx.get_stream`，从目标流当前位置写入；`upload(method, url, source, length, destination, max_response_bytes, options)` 调用 `httpx.send_stream`。设 `options.http2=true` 时使用显式 HTTP/2。返回 `response` 只保存状态、头和耗时，`content` 为空；正文已写入 `destination`，调用方负责关闭流。流式函数不自动跟随重定向，`download` 暂不接受自定义请求头、Cookie 或认证。传输失败可能留下部分文件内容，约束与[底层网络流](network.md#文件流分块传输)相同。

## 使用示例

```tx
import "requests.txh" as requests

def main() -> int
{
    dict query = {}
    query["page"] = "2"
    dict headers = {}
    headers["Accept"] = "application/json"
    requests.response result = requests.get("https://example.com/",
                                             params=query, headers=headers)
    result.raise_for_status()
    print(result.status_code, result.text())

    auto se = requests.session()
    requests.response first = se.get("https://example.com/")
    print(first.status_code)
    return 0
}
```

## 实现和验收

2026-09-26：本文先于代码写入；现已把 `requests_bridge.tx`、参数处理、URL 编码和 Cookie 处理编译到 `libtxstdlib.a`，公开目录仍只有 `.txh`。`scripts/build.ps1` 构建成功并按项目规则清理 `build/`；已确认归档包含 `requests_bridge.o`。

本机定向往返由 `tests/network/requests_server.tx` 与 `requests_client.tx` 完成，客户端退出码为 0。覆盖了 `requests.session()` 初始化、`Session.request` 的 `none` 默认参数、类方法和顶层函数的 Python 命名实参、UTF-8 查询编码、会话 Cookie、请求头、`data`、`json`、`files` multipart、Basic `auth`、整数秒 `timeout`、`stream=false`、`verify=true`、相对重定向、JSON 解析及 `raise_for_status`。`tests/default_parameters/behavior.tx` 还验证了 `int = none`、`float = none`、跨模块默认参数、类方法和 `fn` 函数值。未运行全量回归或性能基准；HTTP/2、文件流、跨源重定向、Cookie 过期、超时失败和超过 8 MiB 的边界尚未专项验证。

预编译类方法的调用目标在编译期确定。`files` 已覆盖内存正文、文件名元组及字节流读取，仍受 8 MiB 上限约束；`proxies`、`hooks`、`cert`、`stream=true` 及 `verify=false` 当前报 `unsupported_option`。连接池、惰性响应、完整 CookieJar、`PreparedRequest` 和 Requests 异常类仍是能力缺口。性能上，原始正文保持 `bytes`，URL 编码收集片段后一次拼接，multipart 分段进行平衡拼接；`session` 目前复用配置和 Cookie，不复用底层 HTTP 连接。
