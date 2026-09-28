# `requests` 标准库设计与接口

本文固定 `requests` 的公开契约。模块名采用 Python 的正确拼写 `requests`，通过 `import "requests.txh" as requests` 使用。公开接口放在 `tx/stdlib/requests.txh`；实现以 `.tx` 编写，源码放在 `src/stdlib/`，构建后并入 `tx/libtxstdlib.a`。`tx/stdlib/` 交付时仍只包含 `.txh`。

## 11.8 扩展契约

`session` 惰性创建客户端会话，连续请求使用同一连接池；无自定义 CA 时由 WinHTTP 提供连接池，自定义 CA 时由已验证 TLS 连接池复用同源连接。`session.close()` 关闭池，重复关闭无害，关闭后请求报 `connection_closed`。单次模块级调用使用临时会话。默认请求仍使用 HTTP/1.1；显式 `http2=true` 要求 HTTPS ALPN `h2`。池配置由代理、CA、客户端身份和协议选择确定；配置切换会关闭旧池，已返回的流式响应独立持有请求句柄。调用方须显式关闭流式响应。

`stream=true` 在收到响应头后返回，`response.content` 初始为空；`response.next_chunk(max_bytes)` 每次读取 1～16384 字节，返回 `{data: bytes, eof: bool}`，空正文以 `eof=true` 表示。`response.read_all()` 在尚未逐块读取时把剩余正文读入 `content`，受 `max_response_bytes` 限制；`text()`、`json()` 和 `json_object()` 会先调用它。逐块读取后再调用整体读取或文本/JSON 方法报 `invalid_state`。`response.close()` 可重复调用，提前关闭会丢弃剩余正文。非流式调用自动读完并关闭请求句柄；默认正文上限保持 8 MiB。

命名参数 `proxies` 接受字典或 `map<str, str>`，只允许 `http`、`https` 键；值为 `http://host:port`、`direct` 或空串（系统代理）。按当前目标 URL 协议选用代理，重定向后重新选择。`verify=true` 使用系统信任；`verify` 为 UTF-8 PEM CA 文件路径时使用自定义信任锚。客户端在同一 TLS 连接完成证书链、用途、有效期和主机名验证之后才发送 HTTP 请求头和正文；验证失败的连接会关闭，认证信息和正文不会到达服务端。`verify=false` 明确报 `unsupported_option`。`cert` 接受 PKCS#12 文件路径，或 `[路径, 密码]` 两元素数组；自定义 CA 与客户端证书可同时使用。仅配置客户端证书时仍走 WinHTTP 系统信任。无自定义 CA 的 WinHTTP 导入会创建当前用户的临时密钥容器，连接池释放时删除；进程异常终止可能留下该容器。密码不进入诊断。代理认证暂不提供，带凭据的代理 URL 明确拒绝。

`max_retries` 是 0～3 的显式整数，默认为 0；只在 GET/HEAD/OPTIONS/PUT/DELETE 且内存正文可重播时，对连接关闭、超时或网络操作失败重试。不会根据 HTTP 状态码重试，不会重试证书错误、参数错误、正文已开始交付或文件流上传；每次重试都重新创建请求句柄，重试次数不含首次尝试。`retry_backoff_ms` 为 0～1000，控制两次尝试之间的等待。旧 `hooks` 仍以 `unsupported_option` 拒绝，不接受后静默忽略。

Cookie jar 匹配 Domain、Path、Secure、Max-Age 和常见 HTTP 日期 Expires，并处理删除语义、`__Host-`/`__Secure-` 前缀、单条 4096 字节和每会话 180 条上限。WinHTTP 自带 Cookie 状态已禁用，只有此 jar 会自动发送 Cookie。请求级显式 Cookie 只覆盖本次同名值；跨源重定向不转发显式 Cookie 或认证头，会按新目标重新匹配会话 Cookie。SameSite、HttpOnly 不按浏览器页面来源执行；当前没有公共后缀列表，Domain Cookie 仅应接收受信任站点的响应。

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
| `stream=True` 的惰性响应对象与 `iter_content` | `stream=true` 返回惰性响应；反复调用 `next_chunk` 直到 EOF | 实现逐块读取；不提供生成器对象 |
| 连接池、`HTTPAdapter`、自定义重试策略 | 会话持有底层池；`max_retries` 限定安全重试 | 实现池与有界重试；不提供 `HTTPAdapter` |
| 显式代理、自定义 CA、客户端证书、Digest/NTLM、OAuth | 按协议选代理、PEM CA、PKCS#12 客户端身份 | 实现代理、自定义 CA 与客户端身份；其余认证方案暂不提供 |
| `files` multipart | `dict` 中的字节、文本或文件元组，内存正文限 8 MiB | 实现常用形式 |
| `Request/PreparedRequest`、hooks 和完整 CookieJar | 需要进一步确定 TX 对象/回调契约 | 暂不提供 |

上表的“暂不提供”能力会明确拒绝，不会接受后忽略。现有 `httpx` 独立调用的行为保持不变。

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
    stream: bool
    proxy_http: str
    proxy_https: str
    ca_file: str
    client_pfx_file: str
    client_pfx_password: str
    max_response_bytes: int
    max_retries: int
    retry_backoff_ms: int
}

struct body_chunk
{
    data: bytes
    eof: bool
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
        streamed: bool
        stream_open: bool
        stream_started: bool
        stream_finished: bool
        pending: httpx.client_request

        def next_chunk(max_bytes: int) -> body_chunk
        def read_all() -> bytes
        def close() -> void
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
        transport: httpx.client_session
        transport_key: str
        closed: bool

        def init() -> void
        def close() -> void
        def request(method: str, url: str,
                    params: dict = none, data: any = none,
                    headers: dict = none, cookies: dict = none,
                    files: any = none, auth: array = none,
                    timeout: any = none, allow_redirects: bool = none,
                    proxies: any = none, hooks: any = none,
                    stream: bool = none, verify: any = none,
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

`default_options()` 返回空参数、空头、空 Cookie、无正文、30 秒超时、最多 10 次重定向、自动跟随重定向、HTTP/1.1、8 MiB 正文上限和零次重试；`requests.session()` 由 `init()` 设置会话默认值。`body_type` 限 `none/raw/form/json`，由 `with_*` 函数维护；传输时不会依据正文内容猜测类型。`response.history` 是跳转链中先前响应的快照数组；非流式响应的 `content` 是原始字节，流式响应初始为空。`ok` 等价于状态码小于 400，不表示 JSON 解析成功。`transport`、`pending` 等公开布局字段由模块维护，不应由调用方直接修改。

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
| `stream` | `bool` | `true` 在响应头后返回，调用 `next_chunk/read_all/close` 管理正文 |
| `verify` | `true` 或 PEM CA 文件路径 | `true` 使用系统信任；PEM 路径在同一连接发送请求前完成自定义链验证；`false` 报 `unsupported_option` |
| `proxies` | 仅含 `http`/`https` 键的字典或 `map<str, str>` | 按 URL 协议选显式代理、直连或系统代理 |
| `cert` | PKCS#12 路径或 `[路径, 密码]` | 设置 HTTPS 客户端身份 |
| `max_response_bytes`、`max_retries`、`retry_backoff_ms` | `int` | 正文总量和安全重试上限；分别默认为 8 MiB、0、0 毫秒 |
| `hooks` | Python 原名 | 明确报告 `unsupported_option` |

`params` 与表单字段按 UTF-8 字节进行 RFC 3986 百分号编码；查询参数追加到原 URL 的现有查询之后，URL 片段在发送前去掉。Cookie 名和值按原文本拼接，由底层头字段校验拒绝 CR/LF。表单设置 `application/x-www-form-urlencoded`，空格编码为 `+`；JSON 正文使用标准库 `json.stringify`，设置 `application/json; charset=utf-8`。用户提供的 `Content-Type` 优先。字符串正文使用 UTF-8，字节正文保持原始内容。

`files` 在内存中组装 multipart；每个字段名、文件名和媒体类型拒绝 CR、LF、双引号与反斜杠，防止写入额外的 MIME 头。未指定文件名时使用字段名加 `.bin`，未指定媒体类型时使用 `application/octet-stream`。文件元组的内容接受 `bytes` 或 `str`；`binary_stream` 可从当前游标读取到 EOF，但不会由 `requests` 关闭。`files` 不能和 `json` 合用；带 `files` 的 `data` 只接受字段字典。调用方不能自定与实际边界不一致的 `Content-Type`。组装后的正文仍受 `httpx` 的 8 MiB 上限约束，大文件使用下文的流式 `upload`。

`timeout_ms` 必须大于零，按底层接口分别约束连接、发送与接收阶段，不是整条重定向链的总时限。普通内存请求/响应仍受 `httpx` 的 8 MiB 正文限制；流式 API 使用调用方的明确上限。默认头不携带密钥；Basic 认证只在显式配置时构造 `Authorization`。传入的敏感值不写入诊断。

自动重定向处理 301、302、303、307、308；303 改为 GET，301/302 的 POST 按 Requests 惯例改为 GET，307/308 保留方法和正文。`HEAD` 默认不跟随重定向，其余便捷方法默认跟随。`Location` 可以是绝对 URL、根路径或相对路径；相对路径中的 `.`、`..` 目前未规范化。超过 `max_redirects`、循环或无效 `Location` 报 `io_error`；跨源跳转移除 `Authorization` 和显式 `Cookie`，重新按目标域匹配会话 Cookie。底层 `httpx` 拒绝 URL 内用户信息。HTTPS 降级到 HTTP 时也不转发认证信息。跳转链中的请求与响应都受同样的超时和大小限制。

## 响应 API

`response` 提供 `text()`、`text(encoding)`、`json()`、`json_object()` 和 `raise_for_status()` 方法，签名见前面的类声明。

`text` 默认严格按 UTF-8 解码；显式编码使用 `encoding` 模块当前支持的字符集。无效字节不会静默替换。`json` 先按 UTF-8 解码，再用 `json.parse`，合法 JSON `null` 返回 `none`，解析失败为 `parse_error`。`raise_for_status` 对 400～599 报 `io_error/http_error`，其他状态不报错。`response.cookies` 是该响应 `Set-Cookie` 的名称和值快照，不代表完整 Cookie 属性；完整匹配规则由会话内部维护。

`response.text()` 是方法，Python Requests 的 `Response.text` 是属性；TX 当前没有属性 getter。正文不自动解压缩，也不依据 `Content-Type` 猜测字符集。Cookie 日期识别常见 RFC 1123、RFC 850 和 asctime 形式；没有公共后缀列表，使用 Domain Cookie 时应限定受信任站点。

## 文件流操作

`download(url, destination, max_bytes)` 及带 `request_options` 的重载从目标流当前位置写入；`upload(method, url, source, length, destination, max_response_bytes, options)` 从源流当前位置上传并逐块写入响应目标。两者复用 11.8 的代理、CA、客户端证书、请求头与认证配置；`download` 可在交付正文前按 GET 规则重试，文件上传拒绝自动重试和惰性响应选项。设 `options.http2=true` 时要求 HTTPS HTTP/2。返回 `response` 只保存状态、头和耗时，`content` 为空；调用方负责关闭文件流。流式函数不自动跟随重定向。传输失败可能留下部分目标文件内容，约束与[底层网络流](network.md#文件流分块传输)相同。

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

原有本机定向往返由 `tests/network/requests_server.tx` 与 `requests_client.tx` 完成，覆盖 `requests.session()`、类方法和模块函数、命名实参、URL 编码、Cookie、正文格式、认证、重定向和响应辅助方法。`tests/default_parameters/behavior.tx` 验证默认参数和跨模块方法；这些记录对应 2026-09-26 的初始实现。

**11.8 定向验证（2026-09-28）：** Windows x64 增量构建成功，`scripts/check_requests_11_8.py` 与 `check_requests_11_8_tls.py` 均通过。HTTP 用例核对相同 TCP 源端口的连接复用、惰性分块及整体读取、提前关闭、过期/路径/显式覆盖 Cookie、代理、连接失败重试、非幂等和 503 不重试、`hooks` 明确拒绝，以及大正文下载和文件上传。TLS 用例用临时 CA 和双向证书验证自定义 CA、PKCS#12 客户端身份、错误 CA、缺少客户端身份、默认系统信任拒绝和 `verify=false` 明确拒绝；11.9 又核对错误主机名。私钥不写入日志。旧 `requests_server/client.tx` 再次编译运行通过。未运行全量或跨平台测试；公共后缀列表、代理认证、浏览器 SameSite 策略及进程异常终止后的临时密钥容器清理仍未覆盖。11.9 的本机网络边界证据见[网络边界专项](network_11_9.md)。

**标准库审查顺序 2 定向验证（2026-09-28）：** Windows x64 增量构建及 `python -X utf8 scripts/check_requests_11_8_tls.py` 通过，输出 `REQUESTS_TLS_SEQ2_OK`。本地合成证书用例确认自定义 CA 和客户端身份的 HTTP/1.1、显式 HTTP/2、同源连接复用、HTTP CONNECT 代理、重定向和一次 GET 重试；错误 CA、错误主机名及错误服务端用途均未使服务端收到 HTTP 请求。测试还覆盖空 PKCS#12 密码、4096 字节输入进入 `PFXImportCertStore`、4097 字节长度拒绝、错误密码和非法 UTF-8。Windows 对本次生成的 4096 字节密码包返回 `invalid_argument`，该用例只据错误消息确认密码已通过本项目长度与编码检查并进入导入器，不把它记为成功导入。未运行全量测试。
