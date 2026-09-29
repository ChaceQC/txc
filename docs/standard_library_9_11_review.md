# 标准库第 9～11 部分实现审查

审查日期：2026-09-28。代码基准：`678bb44`（`feat(stdlib): complete network phase 11.1-11.9`）。

本报告对应[标准库实施计划](standard_library_plan.md)的第 9 部分“密码学、密码与证书”、第 10 部分“线程、任务与进程间通信”、第 11 部分“低层网络、TLS 与协议客户端/服务端”，重点检查正确性、性能、安全和内存管理。

本次审查发现 10 项待修复问题：6 项 P1、4 项 P2。其中 4 项通过本机最小用例复现，6 项由代码调用顺序、所有权或构建配置确认。审查阶段未修改项目实现；后续顺序执行情况记录在第 4 节，修复状态以问题清单为准。

## 1. 优先级、证据与问题清单

- **P1：** 建议优先修复，涉及请求泄露、密码学并发安全、服务不可用、取消失效或内存安全。
- **P2：** 需要修复，涉及长连接资源回收、任务调度性能、错误传播或敏感内存清理。
- **已复现：** 本次审查运行了对应的最小用例，并记录实际结果。
- **静态确认：** 代码或构建配置中存在明确问题；未通过动态用例验证竞态时序、资源增长量或攻击后果。

| 编号 | 优先级 | 所属部分 | 问题 | 证据 | 修复状态 |
| --- | --- | --- | --- | --- | --- |
| R01 | P1 | 11 | 自定义 CA 验证晚于 HTTP 请求发送 | 已复现 | 已修复 |
| R02 | P1 | 9、10 | 密码学共享随机状态缺少并发保护 | 静态确认 | 已修复 |
| R03 | P1 | 11 | 失败 HTTP/2 会话持续阻断后续正常连接 | 已复现 | 已修复 |
| R04 | P1 | 10 | IOCP 启动与取消之间存在遗漏取消的窗口 | 静态确认 | 已修复 |
| R05 | P1 | 11 | HTTP/3 未完成请求形成强引用环 | 静态确认 | 已修复 |
| R06 | P1 | 11 | HTTP/2 在锁外修改共享流表 | 静态确认 | 已修复 |
| R07 | P2 | 11 | HTTP/3 已完成流的句柄延迟到连接销毁才释放 | 静态确认 | 已修复 |
| R08 | P2 | 10、11 | 异步网络等待占满通用任务池 | 已复现 | 已修复 |
| R09 | P2 | 10 | 文本和复合结果的线程、任务异常被覆盖 | 已复现 | 已修复 |
| R10 | P2 | 11 | 客户端证书密码存在未清零的普通字符串副本 | 静态确认 | 已修复 |

下文行号均对应上述代码基准；后续修改后应按函数名与调用关系重新定位。

## 2. 问题定位与修改建议

### R01：自定义 CA 验证发生在请求发送之后

**优先级：P1。类别：安全。证据：已复现。**

定位：

- [src/stdlib/httpx_client_tls.cpp](../src/stdlib/httpx_client_tls.cpp)：第 204 行，`configure()` 设置 `SECURITY_FLAG_IGNORE_UNKNOWN_CA`。
- [src/stdlib/httpx_client_session.cpp](../src/stdlib/httpx_client_session.cpp)：第 192 行发送请求头，第 327 行写入正文；第 350～357 行的 `finish()` 在接收响应后才调用 `httpx_client_tls::verify()`。

**问题与影响：** 配置自定义 CA 时先允许未知根证书，然后发送请求头和正文，直到收到响应才补做独立证书验证。不可信服务端可以在客户端最终报错前收到 Authorization、Cookie 或上传内容；事后返回验证失败无法撤回这些信息。

**复现条件与结果：** 在本机启动 TLS 服务端，客户端指定与该服务端证书无关的 CA，并发送仅用于审查的合成 Authorization 和正文。客户端最终返回 `security_error`，服务端已收到完整请求：

```text
client_error security_error
authorization_received = True
body_received = True
```

**如何修改：** 自定义 CA 路径必须在 TLS 握手阶段完成信任链验证，只有验证成功才能发送 HTTP 请求头与正文。应采用支持握手阶段自定义验证的传输方式，或接入已有安全流的验证能力。不能仅把当前校验挪到 `WinHttpSendRequest` 返回之后；此时请求头已经发出。失败时应立即关闭对应连接，并保留稳定的验证错误。

### R02：密码学随机数的共享状态没有并发保护

**优先级：P1。类别：密码学并发安全。证据：静态确认。**

定位：

- [src/stdlib/crypto_random.cpp](../src/stdlib/crypto_random.cpp)：第 44 行的 `psa_ready()` 使用 `call_once` 初始化；第 64 行的 `fill_random()` 直接调用 `psa_generate_random()`。
- [CMakeLists.txt](../CMakeLists.txt)：第 45 行登记 Mbed TLS 依赖，第 107 行引入构建；未配置相应线程支持。
- 审查时的依赖源码 `build/_deps/mbedtls-src/include/mbedtls/mbedtls_config.h`：第 2171、2182、3764 行的 `MBEDTLS_THREADING_ALT`、`MBEDTLS_THREADING_PTHREAD`、`MBEDTLS_THREADING_C` 均未启用。生成的 `build/build.ninja` 中对应对象的编译参数没有覆盖该配置。
- 依赖源码 `library/psa_crypto.c` 第 4423 行使用 `global_data.rng.drbg`；`library/ctr_drbg.c` 第 702 行起的互斥逻辑受 `MBEDTLS_THREADING_C` 控制。上述 `build/` 文件是当次构建证据，不属于需要保留的仓库源码。

**问题与影响：** `call_once` 仅保护初始化，不能保护初始化后的并发取数。多个 TX 线程或任务调用随机数入口时会访问同一个全局 DRBG 状态，而当前构建移除了其内部互斥。这构成数据竞争，影响随机密钥、salt、nonce 的安全前提；本次未实测随机数碰撞，也不把碰撞作为已观察事实。

**如何修改：** 在项目构建配置中统一启用 Mbed TLS 的线程支持和可用的互斥后端，并保证依赖库与所有调用方使用一致配置后重新构建。可采用当前 MinGW 环境适用的 pthread 后端，或正确初始化的 Windows 替代互斥实现。仅给 `fill_random()` 加锁不能完整覆盖 TLS 内部直接使用 PSA 状态的路径。

### R03：一个失败 HTTP/2 会话会持续阻断后续正常连接

**优先级：P1。类别：安全与可用性。证据：已复现。**

定位：

- [src/stdlib/http2_server.cpp](../src/stdlib/http2_server.cpp)：第 129～148 行，监听器轮询 `sessions`；非超时错误在第 141 行直接抛出，没有删除失败会话。
- [src/stdlib/http2_session.cpp](../src/stdlib/http2_session.cpp)：第 64 行的 `check_callback_error()` 持续重抛已保存错误；`accept()` 开始时调用的 `flush_output()` 也会检查该错误。

**问题与影响：** 会话发生非法帧、头或正文超限等错误后，仍留在监听器的会话列表中，同时永久保留 `callback_error_`。后续 `accept()` 再次访问该会话并抛出旧错误，无法继续接收其他正常连接。单个恶意客户端可以使该监听器持续不可用。

**复现条件与结果：** 同一 h2c 监听器先接收声明正文长度为 `8388609` 的超限请求，再接收另一条连接上的正常请求。连续两次 `accept()` 的结果为：

```text
accept_error 0 size_limit
accept_error 1 size_limit
```

**如何修改：** 将非超时的致命会话错误与可重试超时区分。致命错误发生后，把会话标为不可用、从监听器移除并关闭，清理其所属流。错误可向当前调用方交付一次；后续 `accept()` 应继续处理其他会话和新连接。不能简单清空错误后复用可能已经损坏的 nghttp2 会话。

### R04：IOCP 启动和取消之间存在遗漏取消的窗口

**优先级：P1。类别：取消与资源管理。证据：静态确认。**

定位：

- [src/backend/cpp/task_iocp.cpp](../src/backend/cpp/task_iocp.cpp)：第 115 行将操作加入登记表，第 117 行才调用 `begin()`；第 174～178 行无条件设置 `cancel_requested` 并调用 `CancelIoEx()`。
- [src/backend/cpp/task_file_iocp.cpp](../src/backend/cpp/task_file_iocp.cpp)：第 107～112 行、第 165～170 行分别在取消检查后调用 `ReadFile()`、`WriteFile()`。

**问题与影响：** 可触发时序如下：操作进入登记表；`begin()` 检查令牌时尚未取消；令牌随后被取消；完成线程对尚未提交的操作调用 `CancelIoEx()` 并标记 `cancel_requested=true`；原线程再提交 I/O。第一次取消可能因内核尚无对应操作而无效，后续循环又因标记已设置而不再尝试。慢文件或管道 I/O 可能让已取消的任务作用域迟迟无法退出。

**如何修改：** 为操作区分准备、已提交、完成等状态，将启动和“已提交”状态发布同步起来。取消必须覆盖提交前和提交后的两种情况，并在提交成功后补查待处理取消。检查 `CancelIoEx()` 的返回值；不能把“尚未找到操作”直接当作已经成功发出取消。完成与失败路径仍须保证只交付一次结果。

### R05：HTTP/3 未完成请求形成强引用环，断连后仍无法释放

**优先级：P1。类别：内存与资源管理。证据：静态确认。**

定位：

- [src/stdlib/http3_server_headers.cpp](../src/stdlib/http3_server_headers.cpp)：第 21 行将连接的 `shared_from_this()` 保存到请求的 `session`，第 23 行把请求加入连接的 `requests_`。
- [src/stdlib/http3_server_internal.hpp](../src/stdlib/http3_server_internal.hpp)：第 30 行的 `server_request::session`、第 159 行的 `server_connection::requests_` 构成双向强引用。
- [src/stdlib/http3_server_events.cpp](../src/stdlib/http3_server_events.cpp)：第 89～93 行的连接关闭完成事件只设置 `shutdown_`。

**问题与影响：** 正常响应或显式关闭请求会删除 `requests_` 中的记录，但对端发送部分请求后中止，没有对应的未完成请求清理。连接持有请求，请求又持有连接；回收线程即使移除连接记录，也无法打破这个环。请求正文、QUIC 连接和关联监听资源会被继续保留。反复建立并中断连接可累积资源占用。

**如何修改：** 调整反向引用的所有权，避免请求与连接互相强持有，并明确外部请求句柄需要存活时由谁持有连接。在流中止、连接关闭和解析失败路径统一清理未完成请求；不能仅依赖正常 `respond()` 或 `close_stream()`。清理还需遵守 MsQuic 回调与发送缓冲的存活期。

### R06：HTTP/2 在释放互斥锁后修改共享流表

**优先级：P1。类别：并发与内存安全。证据：静态确认。**

定位：[src/stdlib/http2_session_response.cpp](../src/stdlib/http2_session_response.cpp) 第 188 行的 `close_stream()`；互斥锁位于第 197 行的 `try` 作用域内，`streams_.erase()` 位于第 208 行、锁作用域之外。

**问题与影响：** 同一 HTTP/2 连接上的不同流允许在不同 TX 工作线程处理。一个线程关闭流时，另一个线程可能仍在查询、插入或删除 `streams_`。锁外修改共享 `unordered_map` 构成数据竞争，存在崩溃或内存破坏风险。当前问题不是对同一个请求句柄进行非法并发使用才会触发。

**如何修改：** 将流状态检查、nghttp2 关闭操作和流表删除纳入同一个互斥保护范围。捕获发送或协议异常后，仍应在持锁状态下完成流表清理，并保证所有访问该表的路径遵循同一锁规则。

### R07：HTTP/3 已完成流的句柄一直保留到连接销毁

**优先级：P2。类别：内存与资源管理。证据：静态确认。**

定位：

- [src/stdlib/http3_server_events.cpp](../src/stdlib/http3_server_events.cpp)：第 83～84 行把每条对端流加入 `stream_handles_` 和 `streams_`；服务端没有对应的流关闭完成清理分支。
- [src/stdlib/http3_server_connection.cpp](../src/stdlib/http3_server_connection.cpp)：第 50 行的 `StreamClose()` 只在整个连接析构时执行；正常响应结束只删除请求记录。

**问题与影响：** 即使请求正常处理完成，流对象和 QUIC 句柄仍不能及时回收。长连接重复处理请求时，这些记录持续保留到连接销毁；“最多 16 条并发流”不能代替对已完成流资源的释放。此问题属于正常完成路径，与 R05 的未完成请求引用环分别处理。

**如何修改：** 处理流关闭完成和中止事件，在确认发送缓冲及回调不再使用该流后，关闭句柄、删除两张流表中的记录，并同步通知 nghttp3 结束流。必要时使用延迟回收，避免在回调仍使用 `stream_state` 时释放它。

### R08：异步网络等待占满通用任务池，拖延无关任务

**优先级：P2。类别：性能与调度。证据：已复现。**

定位：

- [src/backend/cpp/socket_async_abi.cpp](../src/backend/cpp/socket_async_abi.cpp)：第 54 行把网络操作提交到通用任务队列，第 80 行在线程中直接执行等待式工作。
- [src/stdlib/socket_tcp.cpp](../src/stdlib/socket_tcp.cpp)：第 179 行等待 socket 就绪。
- [src/backend/cpp/task_executor.cpp](../src/backend/cpp/task_executor.cpp)：第 29 行将通用工作线程数限制为 2～8。

**问题与影响：** 网络异步操作在通用工作线程中等待 I/O 就绪，等待期间持续占用线程。足够多的慢连接会阻塞普通计算任务；若解除读取等待的写任务也排在同一队列中，还可能必须等待读取超时才能得到调度。有界线程数量限制了线程增长，但没有解决等待中的连接占用执行能力的问题。

**复现条件与结果：** 创建 8 对本机 TCP 连接，提交 8 个超时为 1200 毫秒、尚无数据可读的异步读取。稍后提交一个仅返回 `42` 的普通任务，该任务实际等待 1112 毫秒：

```text
unrelated_task_elapsed_ms 1112 42
read_completion timeout
```

该结果用于证明调度阻塞，不作为完整吞吐量基准。

**如何修改：** 把 socket 等待接入 IOCP 或独立网络事件循环，就绪后再调度短小的完成工作。独立的有界阻塞 I/O 池可以隔离对普通任务的影响，但仍需明确容量与背压，不能作为大量等待连接场景的最终解决方案。

### R09：文本和复合结果的线程、任务异常丢失原始类型与错误码

**优先级：P2。类别：正确性与错误传播。证据：文本结果路径已复现；复合结果存在相同静态分支。**

定位：

- [src/backend/cpp/concurrency_result.cpp](../src/backend/cpp/concurrency_result.cpp)：第 31～33 行和第 43～45 行，在文本或复合结果为空时抛出新的 `std::runtime_error`。
- [src/backend/cpp/thread_abi.cpp](../src/backend/cpp/thread_abi.cpp)：第 151～154 行把该异常保存为 `runtime_error/thread_failed`。
- [src/backend/cpp/task_runtime.cpp](../src/backend/cpp/task_runtime.cpp)：第 104～107 行把该异常保存为 `runtime_error/task_failed`。

**问题与影响：** TX 回调出错后可返回空结果指针，此时运行时已经记录了原始错误。再次抛“跨线程结果为空”会覆盖原来的类别、错误码、消息及调用栈。调用方按 `io_error` 等类别编写的处理逻辑因此失效。

**复现条件与结果：** 返回类型为 `str` 的回调主动产生 `io_error/audit_original`，分别通过线程 `join` 和任务 `wait` 接收，实际结果为：

```text
thread_runtime thread_failed
task_runtime task_failed
```

**如何修改：** 发现空结果时先检查当前运行时上下文是否已有待传播的 TX 错误。有则直接保留并交付原始错误；只有没有原始错误却返回空结果时才创建新的运行时错误。线程和任务的外层捕获逻辑也应避免覆盖已经确定的 TX 错误快照。

### R10：客户端证书密码被复制到未清零的普通字符串

**优先级：P2。类别：敏感内存清理。证据：静态确认。**

定位：[src/stdlib/httpx_client_tls.cpp](../src/stdlib/httpx_client_tls.cpp) 第 28～31 行的 `password_buffer` 构造函数，以及第 34～38 行的析构函数。

**问题与影响：** 构造函数从 `secret_bytes` 创建普通 `std::string text`，编码转换完成后该副本直接析构，没有安全擦除。宽字符串随后追加终止符也可能重新分配，留下旧缓冲。析构时清零当前 `value_` 无法清除这些已经释放的副本，削弱了秘密缓冲的受控清理保证。

**如何修改：** 先计算所需宽字符长度，直接从秘密字节转换到一次分配、包含终止符空间的受控宽字符缓冲，避免普通字符串中转及再次扩容。使用拥有缓冲区的 RAII 对象保证正常返回和构造、转换失败路径都能擦除敏感内容。

## 3. 验证范围与后续使用

本次只运行 R01、R03、R08、R09 的本机最小复现，没有运行全量测试，没有实施修复，也没有进行密码学碰撞实验、长期内存压力测试或竞态故障注入。R02、R04、R05、R06、R07、R10 的结论应按上文列出的静态证据使用，不把可能后果写成已经发生的实测结果。

复现使用合成请求内容与临时测试证书，不涉及真实业务凭据。临时复现源码已清理；复现过程与输出保留在本报告中，尚未转为仓库内的正式回归用例。

后续修复应按问题编号记录变更与直接相关的验证结果。只有对应缺陷被修复并完成针对性确认后，才更新本报告中的修复状态。原实施计划中的完成记录保留其历史含义，本报告补充当前实现仍存在的问题。

## 4. 修复顺序与分步实施方案

### 4.1 顺序、依赖与交付规则

建议按下列顺序交付。R01 的临时封堵只用于阻止继续泄露，不代表 R01 已修复；只有安全的自定义 CA 请求恢复并通过定向验收后，才能把 R01 标为“已修复”。

| 顺序 | 对应问题 | 本阶段交付边界 | 排序依据 |
| --- | --- | --- | --- |
| 0 | R01 临时封堵 | 自定义 CA 请求在发送前明确失败 | R01 的请求内容泄露已经复现，必须立即切断不安全路径 |
| 1 | R02 | Mbed TLS 依赖及调用方使用一致的线程安全配置 | 完整 R01 若复用现有 TLS 安全流，会使用同一密码学依赖 |
| 2 | R01 完整修复、R10 | 自定义 CA 在同一连接上先验证后发送；同时移除客户端证书密码副本 | 两项均涉及 HTTP 客户端 TLS 路径，合并核对身份与秘密数据生命周期 |
| 3 | R06 | HTTP/2 共享流表的所有访问遵守同一互斥规则 | 先消除并发内存风险，再扩大该会话的失败清理 |
| 4 | R03 | 致命 HTTP/2 会话退出监听循环，后续正常连接可继续 | 与 R06 共用 HTTP/2 会话生命周期 |
| 5 | R04 | IOCP 提交、取消与完成形成无遗漏的状态机 | R08 的网络 IOCP 方案应建立在可靠的取消语义上 |
| 6 | R05 | HTTP/3 未完成请求不再使连接形成强引用环 | 先明确连接和请求的所有权，才能安全回收流 |
| 7 | R07 | HTTP/3 已完成或中止的对端流及时释放 | 接续 R05 的连接关闭和回调存活期处理 |
| 8 | R09 | 文本及复合结果保留线程、任务的原始错误 | 独立的正确性修复，置于 P1 问题之后 |
| 9 | R08 | 异步网络等待不占用通用任务工作线程 | 涉及四类 socket 操作及调度架构，且依赖 R04 的取消基础 |

1. **每项开始前固定边界。**
   1. 对照第 2 节中的调用链重新定位代码；报告中的行号只适用于 `678bb44`。
   2. 先写清本项的成功、失败、取消、关闭和部分外部效果，再修改实现。若公开接口或错误语义需要改变，同步对应模块文档和示例；不得只让一个测试用例定义新契约。
   3. 保留现有用户改动与旧接口兼容性。R01 的传输改造须覆盖 `httpx` 和基于它的 `requests`，不能只修一个调用入口。
2. **每项结束时留下可核对的证据。**
   1. 记录实际修改文件、构建结果、一个正常路径及直接相关的失败或资源路径。并发问题优先增加能控制交错顺序的最小用例；不把偶然通过的压力循环当作竞态已消除的证明。
   2. 只运行本项必要的定向验证。只有构建配置或共享基础设施变化要求时，才扩大到其直接受影响的模块；不重复运行全量测试。
   3. 对应缺陷的行为、错误语义和清理均确认后，更新第 1 节状态及本节验收记录。原实施计划的历史完成记录不因本报告直接改写。

### 4.2 顺序 0：R01 先阻断自定义 CA 请求泄露

1. **确定封堵条件和位置。**
   1. `httpx.open_secure_session` 只要收到非空自定义信任锚，就进入当前不安全路径；即使 `include_system=true`，仍不能依赖“发送后再验证”。客户端证书单独配置且没有自定义锚时，仍由 WinHTTP 默认信任验证处理。
   2. 在创建安全会话时，或最迟在 `configure_request()` 调用 `WinHttpSendRequest` 前，明确拒绝该组合。失败要有稳定的 `security_error`，不得退回系统信任后继续请求，也不得静默忽略用户指定的 CA。
2. **封住全部入口。**
   1. 检查 `requests.verify` 的 PEM CA 文件路径、会话池重建、重定向与重试最终都经过该保护。
   2. 此阶段不改变默认系统根证书路径和无自定义锚的客户端证书路径；封堵保持到 4.4 的完整方案通过验收。
3. **定向验收。**
   1. 使用与服务端证书无关的临时 CA 和合成 Authorization/正文，确认客户端在发送前报错，服务端既未收到请求头也未收到正文。
   2. 用默认系统信任路径做一条代表性正常请求，确认封堵没有误伤该路径。此阶段 R01 状态仍为“待修复”。

**顺序 0 执行记录（2026-09-28）：** `src/stdlib/httpx_client_tls.cpp` 的公共设置构造入口现在对任意非空自定义锚立即抛出 `security_error`；该检查发生在创建 WinHTTP 会话和请求句柄之前。`httpx.open_secure_session` 与 `requests` 的 `verify` PEM 路径、会话池重建均汇入此入口，不会降级到系统信任。代码追踪还确认重试只接受 `timeout`、`connection_closed`、`operation_failed`，不会重试此 `security_error`；重定向必须先收到响应，而自定义锚请求在会话创建时已失败。未指定自定义锚的系统信任路径和仅客户端证书路径仍保留原行为。同步更新了 `docs/httpx_sessions.md`、`docs/requests.md`，并调整 `tests/network/requests_11_8_tls.tx` 与 `scripts/check_requests_11_8_tls.py` 以覆盖当前临时封堵。

Windows x64 `scripts/build.ps1 -Incremental` 构建通过；`python scripts/check_requests_11_8_tls.py` 输出 `REQUESTS_TLS_SEQ0_OK`。定向用例确认：系统信任访问 `https://example.com/` 成功；同一已建立系统信任会话更换为自定义 CA 后、独立 `requests.get(..., verify=...)` 均在发送前收到 `security_error`；本机 TLS 服务端未收到 HTTP 请求；仅配置客户端证书时仍执行系统信任，面对临时未知根服务端返回 `security_error`。未运行全量测试。R01 完整修复仍待同一连接先验证后发送，故第 1 节状态保持“待修复”。

### 4.3 顺序 1：R02 修复 Mbed TLS 的并发随机状态

1. **固定唯一的依赖配置。**
   1. 在 `CMakeLists.txt` 中为 Mbed TLS 3.6.5 固定线程支持和互斥后端。优先核对当前 MinGW 与已交付的 winpthread 运行时能否使用 Mbed TLS 内置 pthread 后端；若不适用，再实现完整初始化、加锁、解锁和释放语义的 Windows 互斥后端。
   2. 使 `mbedcrypto`、`mbedx509`、`mbedtls` 与包含 Mbed TLS 头文件的项目目标看到同一份配置。不能只对 `crypto_random.cpp` 增加编译宏，或只改依赖源码中的临时构建文件。
2. **重新构建并检查实际产物。**
   1. 从依赖配置阶段重新构建受影响的静态库和 `txstdlib`，核对有效配置头、编译参数及链接依赖；避免继续链接旧的无锁 Mbed TLS 对象。
   2. 保留 `psa_ready()` 的一次初始化语义，但不把 `std::call_once` 视作对后续 `psa_generate_random()` 的保护。检查直接使用 PSA 或 Mbed TLS TLS/DRBG 的调用路径也由库级互斥覆盖。
3. **定向验收。**
   1. 并发调用随机字节、密钥生成和代表性 TLS 握手，检查成功、错误映射与进程稳定性；这些结果用于确认接入与共享状态保护，不声称实测随机数碰撞或证明密码学强度。
   2. 以实际编译配置和库级线程支持作为主要证据；单纯给 `fill_random()` 加项目级锁不满足本项完成条件。

**顺序 1 执行记录（2026-09-28）：** `cmake/mbedtls_user_config.h` 为 MinGW 固定启用 `MBEDTLS_THREADING_C` 与 `MBEDTLS_THREADING_PTHREAD`。`CMakeLists.txt` 将该文件设为 Mbed TLS 的 `MBEDTLS_USER_CONFIG_FILE`，由依赖的公开目标配置传递给 `mbedcrypto`、`mbedx509`、`mbedtls` 和 `txstdlib`；`mbedcrypto` 同时声明 `Threads::Threads` 链接依赖。生成程序沿用驱动中的 `-lpthread`，发行目录包含 `libwinpthread-1.dll`。

`pwsh -NoProfile -File scripts/build.ps1` 从干净的 `build/` 目录完整配置和构建成功，更新 `tx/txc.exe` 与 `tx/libtxstdlib.a`，并在确认产物后清理临时目录。生成的编译命令确认 `psa_crypto.c`、`threading.c`、`x509_crt.c`、`ssl_tls.c` 和 `http2_transport.cpp` 都使用同一个配置头；`libmbedcrypto.a` 中可见 `mbedtls_mutex_*` 包装实现及对 `pthread_mutex_*` 的外部引用。

`python -X utf8 scripts/check_tls_stream.py` 的正常握手、错误主机名、错误信任根、缺少客户端证书和 ALPN 不匹配场景全部通过；新增的 `tests/crypto/random_tls_concurrency.tx` 在 8 个线程各自生成 1 MiB 随机字节并生成密钥时，由同一进程完成 TLS 握手和数据交换。`python -X utf8 scripts/check_crypto_acceptance.py` 通过，包括随机源失败映射与资源清理定向用例。未声称检测随机数碰撞，也未运行全量测试。

### 4.4 顺序 2a：R01 完整修复自定义 CA 的验证时序

1. **先固定传输契约。**
   1. 在 `docs/httpx_sessions.md` 与 `docs/requests.md` 明确：自定义或系统信任、主机名、用途和有效期验证全部成功之后，同一条 TLS 连接才允许写入 HTTP 请求头及正文。验证失败时关闭该连接，不能放回连接池。
   2. `require_http2`、`allow_http2`、代理、客户端证书、连接复用、重定向和重试必须沿用各自既有的公开语义。每次新建到目标源站的 TLS 连接都重新完成验证；不能先检查一条连接的证书，再让 WinHTTP 在另一条连接上发送请求。
2. **选择并接入能控制发送时机的路径。**
   1. 以现有 `tls::secure_connection` 的“握手、证书验证、再交付安全流”为候选基础，核对它与 HTTP 客户端所需的 SNI、可选 PKCS#12 身份和 ALPN 的组合；需要代理时，先完成 CONNECT，再在该隧道内建立到目标站的 TLS 连接。
   2. 自定义 CA 分支的 HTTP 序列化、流式上传/响应、连接池和显式 HTTP/2 须使用已验证的同一安全流。若保留 WinHTTP 处理默认系统信任路径，则在会话层明确区分两条传输实现，避免重新进入 `SECURITY_FLAG_IGNORE_UNKNOWN_CA` 加事后 `verify()` 的旧组合。
   3. 验证失败、ALPN 不符、上传中途失败及调用方提前关闭时，明确销毁请求/连接句柄和池记录，保持既有 `security_error`、`timeout`、`connection_closed` 与已发送数据的部分效果边界。
3. **定向验收和解除封堵。**
   1. 错误 CA、错误主机名和错误用途均须在服务端收到任何 HTTP 应用数据之前失败；正确 CA 的请求须能完整上传和读取。使用合成认证头与正文核对“零泄露”，不使用真实凭据。
   2. 对受该路径直接影响的代理 CONNECT、客户端证书、HTTP/1.1、显式 HTTP/2、重定向及连接复用各取代表性正常和失败路径；错误证书后的连接不得被下一次请求复用。
   3. 上述条件成立后，移除 4.2 的临时封堵，并同步改掉文档中“收到响应头前再校验”的旧描述。此时才可将 R01 改为“已修复”。

**顺序 2a 执行记录（2026-09-28）：** `src/stdlib/httpx_client_tls.cpp` 为自定义锚导入可选 PKCS#12 身份，并通过现有 `tls::secure_connection` 建立 Mbed TLS 连接。该安全流在返回前完成自定义/系统信任链、服务端用途、有效期及主机名检查；自定义 CA 路径不再调用 WinHTTP 的忽略未知根选项，也不在响应后补做验证。`src/stdlib/httpx_client_custom_tls.cpp` 只在验证成功后序列化 HTTP/1.1 请求头；HTTP/2 在同一已验证安全流上协商 `h2` 后由 nghttp2 发送。连接池按源站与代理隔离，HTTP/1.1 与 HTTP/2 均支持会话内顺序复用；重定向、重试继续由 `requests` 层驱动，每次新连接重新验证。显式代理先以 CONNECT 建立隧道；空代理设置通过 WinHTTP 系统代理配置解析。错误证书和 ALPN 失败的连接不会进入池。普通系统信任且没有自定义锚的会话继续使用 WinHTTP。

Windows x64 `pwsh -NoProfile -File scripts/build.ps1 -Incremental` 构建通过。`python -X utf8 scripts/check_requests_11_8_tls.py` 输出 `REQUESTS_TLS_SEQ2_OK`：正确 CA 与客户端证书请求完整上传合成 Authorization/正文；错误 CA、错误主机名和错误服务端用途下服务端未收到 HTTP 请求；同源 HTTP/1.1 连续请求复用相同 TCP 源端口；本机 CONNECT 代理、HTTP/2 ALPN 及同一 HTTP/2 会话连续请求通过；重定向和首次连接被关闭后的安全 GET 重试通过。未运行全量测试。自定义 CA 分支对 `decompress=true` 请求 `identity` 编码；显式要求其他压缩编码或服务端忽略该编码时会明确失败，详见 `docs/httpx_sessions.md`。

### 4.5 顺序 2b：R10 清理客户端证书密码的普通副本

1. **改造 `password_buffer` 的数据流。**
   1. 保留当前 4096 字节上限和内嵌 NUL 拒绝规则；对 `secret_bytes` 原始视图先严格计算 UTF-8 转宽字符所需长度，再一次分配包含终止符空间的受控宽字符缓冲。
   2. 直接写入受控缓冲并设置终止符，不创建中间 `std::string`，也不在转换后调用可能引发重新分配的 `push_back()`。控制缓冲的 RAII 对象负责正常析构及转换、证书导入失败时的清零。
2. **核对秘密的其余生命周期。**
   1. 检查该密码在 `PFXImportCertStore` 前后没有额外普通字符串副本、日志或异常消息回显。
   2. 对空密码、非法 UTF-8、边界长度及错误 PKCS#12 密码做定向验证；清零覆盖范围通过源码和必要的受控检查确认，不在测试输出中打印秘密字节。

**顺序 2b 执行记录（2026-09-28）：** `src/stdlib/httpx_client_tls.cpp` 的 `password_buffer` 不再构造普通 `std::string`。它先对秘密字节视图执行严格 UTF-8 长度计算，再一次分配带 NUL 终止位的 `sensitive_wide_buffer`，直接转换，并在 PKCS#12 导入返回后立即清零释放。`src/stdlib/x509_pkcs12.cpp` 的客户端身份导入也采用同样可在构造失败时清零的受控缓冲，覆盖自定义 CA 与客户端证书组合。4096 字节上限、内嵌 NUL 拒绝及空密码行为保留。

同一 TLS 定向用例验证空密码包可进入系统信任握手；非法 UTF-8 和 4097 字节输入在导入前以 `invalid_argument` 拒绝；错误 PKCS#12 密码以 `invalid_argument` 拒绝。4096 字节 UTF-8 值通过项目的长度与编码检查并到达 `PFXImportCertStore`；Windows 对测试生成的该长密码包返回 `invalid_argument`，因此没有把这个包记作成功导入。`scripts/check_requests_11_8_tls.py` 通过；未输出或记录任何密码值。R10 状态更新为“已修复”。

### 4.6 顺序 3：R06 统一 HTTP/2 流表锁规则

1. **明确共享状态不变量。**
   1. 在 `server_session` 内规定 `streams_` 的查询、插入、删除及关联的 nghttp2 会话操作都受 `io_mutex_` 保护。
   2. 按 `http2_session.cpp` 与 `http2_session_response.cpp` 的全部访问点核对该规则，特别检查回调、`accept()`、`respond()` 和 `close_stream()` 的调用关系，避免持锁回调再次进入同一锁。
2. **修复关闭路径。**
   1. 使 `close_stream()` 的响应状态判断、RST 提交、`flush_output()` 和 `streams_.erase()` 都处于同一受锁保护的状态转换中。
   2. 即使 RST 或发送失败，清理仍应在正确锁范围内完成；保持 `noexcept` 入口不抛异常，且不得把错误流状态留在表中。
3. **定向验收。**
   1. 同一 HTTP/2 连接的两个流在不同任务中交错响应、关闭及取消，确认没有无锁流表访问、重复删除或丢失仍有效的流。
   2. 保留一条正常多流请求与一条异常关闭路径即可；不以单次无崩溃作为唯一验收依据。

**顺序 3 执行记录（2026-09-29）：** `src/stdlib/http2_session.hpp` 记录了会话互斥不变量：同一 `io_mutex_` 串行保护 nghttp2 会话操作与 `streams_` 访问；同步回调沿用调用方持锁状态，不再次加锁。复核全部 `streams_` 访问后，插入、查询和正常响应删除均在该锁保护路径内。`close_stream()` 现在在持锁期间检查响应状态、提交 RST、刷新输出并删除表项；RST 提交失败时仍删除表项，刷新抛错也会在锁内完成清理，入口保持 `noexcept`。

Windows x64 `pwsh -NoProfile -File scripts/build.ps1 -Incremental` 构建通过。`python -X utf8 scripts/check_http_parallel.py` 通过：同一 h2c 连接的两条并行正常流均完成；新增 `tests/network/http2_close_parallel_server.tx` 用两个线程并发关闭一条流并响应另一条，客户端确认关闭流收到 `RST_STREAM(CANCEL)`、健康流收到 `200 healthy`。HTTP/1.1 并发用例也通过。未运行全量测试、竞态检测器或强制传输失败注入；提交失败与刷新异常路径由锁内清理代码静态核对。

### 4.7 顺序 4：R03 淘汰致命 HTTP/2 会话

1. **定义会话终态。**
   1. 区分本次轮询超时、可正常继续的流级拒绝和已经损坏会话状态的致命错误；明确哪些 nghttp2 回调错误、非法帧和限额错误要关闭整条会话。
   2. 致命错误只向当前 `accept()` 交付一次原始错误；会话进入不可用状态后不再调用保存了 `callback_error_` 的 `flush_output()` 或再次交付同一错误。
2. **修改监听器与活动句柄清理。**
   1. 在 `http2_server.cpp` 的 `sessions` 遍历中移出致命会话，关闭其传输，并释放列表中的容量，使监听器仍能接收新连接。
   2. 已交付给调用方的流句柄若仍引用该会话，后续操作应得到确定的关闭错误；内部流表与登记记录按既定锁顺序清理，不能仅清空 `callback_error_` 后复用旧会话。监听器关闭时也要清理已终止会话的登记项。
3. **定向验收。**
   1. 先发送报告中的超限请求，再从另一连接发送正常请求：第一次返回限额错误，下一次 `accept()` 能接收正常请求。
   2. 核对多次坏连接不会永久占满 16 个会话槽，监听器关闭后不遗留失败会话句柄。

**顺序 4 执行记录（2026-09-29）：** `server_session` 将 nghttp2 回调失败、协议解析失败和非超时传输失败设为终态；回调中的非法帧、请求头/正文限额及并发流限额错误均保留原始错误，并只由当前 `accept()` 交付一次。终止时关闭传输、清空活动流和完成队列。监听器遍历会话时移除终态会话，并从连接登记表清除其句柄；旧句柄后续操作返回 `connection_closed`。关闭监听器时会非阻塞地清理已终止会话，若 `accept()` 正在运行则由该调用观察关闭状态后清理。读取超时仍可重试。请求完整解析后的文本校验等单流错误会发送 `RST_STREAM(PROTOCOL_ERROR)` 并清理流，保持会话可继续使用。

Windows x64 `pwsh -NoProfile -File scripts/build.ps1 -Incremental` 构建通过。`python -X utf8 scripts/check_http_protocol_limits.py` 通过，包含新增的 `tests/network/http2_terminal_sessions_server.tx`：连续 17 个超限会话分别返回一次 `size_limit` 并被淘汰，之后同一监听器成功处理正常请求并在关闭时释放会话。既有 HTTP/1.1 与 HTTP/2 非法输入及限额用例也通过。未运行全量测试、竞态检测器或长时间资源压力测试。

### 4.8 顺序 5：R04 补齐 IOCP 提交与取消状态机

1. **拆分“取消请求”和“已向内核发出取消”。**
   1. 为每个 `task_io_operation` 记录准备、提交中、已提交、完成等状态，并让状态转换与 `operations_` 登记、完成移除遵守同一同步规则。
   2. 取消发生在提交前时，记录待取消并阻止提交，或在提交成功后立即补发取消；不能在尚无内核操作时就把 `cancel_requested` 永久记为“已处理”。
2. **处理提交和完成的所有结果。**
   1. `begin()` 的立即失败、同步成功、`ERROR_IO_PENDING` 和提交后立即完成分别进入确定的完成路径；结果、缓冲、文件句柄和任务状态仅交付或释放一次。
   2. 检查 `CancelIoEx()` 返回值。`ERROR_NOT_FOUND` 可能表示尚未提交，也可能表示完成已排队；只有结合操作状态或收到完成事件，才能结束待取消处理。
   3. 同时核对令牌取消、任务作用域取消、截止时间与事件循环停止，避免其中任一入口绕过上述状态机。
3. **定向验收。**
   1. 用同步屏障让取消固定落在“检查令牌之后、`ReadFile`/`WriteFile` 提交之前”，确认操作不会无限挂起，且只有一次任务结果。
   2. 再覆盖已挂起 I/O 的取消、提交立即失败和正常完成，检查句柄关闭与部分写入结果仍符合原有契约。

**顺序 5 执行记录（2026-09-29）：** `src/backend/cpp/task_iocp.hpp` 为 IOCP 操作增加准备、提交中、已提交、完成中和已完成状态；`src/backend/cpp/task_iocp.cpp` 在同一互斥锁下同步状态与 `operations_` 登记和移除。取消请求与向内核派发取消分别记录，提交中的操作只保留待取消标记；`begin()` 返回成功或挂起后唤醒事件循环，再对已提交操作调用 `CancelIoEx()`。`CancelIoEx()` 返回 `ERROR_NOT_FOUND` 时保留操作，等待其完成包；其他 API 错误会重新尝试取消，不提前释放内核仍可能使用的缓冲。若 IOCP 在 `begin()` 返回前收到完成包，则先保存字节数和错误，再由提交路径交付一次结果；立即失败和完成包两条路径都会移出登记表并关闭文件句柄。

Windows x64 `pwsh -NoProfile -File scripts/build.ps1 -Incremental` 构建通过。`python -X utf8 scripts/check_task_iocp.py` 通过：原生屏障用例把令牌取消固定在预检之后、读取提交之前，并等到事件循环观察到取消请求后才放行提交；另覆盖已挂起读取的任务作用域取消、截止时间取消、正常读取完成、提交立即失败、预提交取消及事件循环析构时取消挂起读取。同步完成时序用受控操作在 IOCP 已收取完成包后让 `begin()` 返回 `ERROR_SUCCESS`，检查提交路径使用已缓存的完成结果且只交付一次。验收同时确认操作句柄关闭；已有 `tests/stdlib/async_file_behavior.tx` 与 `tests/stdlib/async_file_cancel_pending.tx` 通过，覆盖实际异步文件读写、空操作、打开失败和取消结果。

新增 [部分写入结果注入](../tests/stdlib/async_file_partial_write.cpp)，直接向生产 `write_operation::complete()` 注入 12 字节请求仅完成 5 字节的成功、取消和磁盘满结果，分别确认 `written == 5`、`cancelled` 与 `error_code` 映射及句柄关闭；`python -X utf8 scripts/check_task_iocp.py` 中 `ASYNC_FILE_PARTIAL_WRITE_OK` 通过。这验证了完成结果处理，不模拟底层设备真实的短写调度。

全量检查通过：`txc test .\tests` 发现并通过 1 个测试组；运行全部 30 个 `scripts/check_*.py` 脚本均通过，包括本节 IOCP/部分写入注入脚本。未运行长时间压力测试。

### 4.9 顺序 6：R05 断开 HTTP/3 未完成请求的引用环

1. **重新画清所有权图。**
   1. 连接内部的 `requests_` 可以强持有尚在处理的请求，但该请求返回连接的引用应为弱引用，避免“连接 → 请求 → 连接”的环。
   2. 监听器的已完成请求队列和对外请求登记项要分别明确持有连接的强引用，保证请求已交付但尚未回复时，连接不会被回收线程提前析构；外部句柄关闭后释放该保活引用。
2. **补齐异常终止的清理。**
   1. 在对端流中止、连接关闭完成、解析失败、监听器关闭和响应异常路径中，统一使未完成请求退出 `requests_` 并唤醒可能等待回复的调用方。
   2. 清理顺序须避开持有连接锁时析构仍会进入 MsQuic 回调的对象；发送缓冲、回调上下文和连接句柄必须存活到相应完成事件。
3. **定向验收。**
   1. 对端只发送部分请求头或正文后断连，确认监听器回收连接后，请求、连接和关联句柄均能释放。
   2. 已交付给调用方但尚未回复的请求在监听器移除连接记录后仍可获得确定的成功或关闭结果；不出现悬空引用。

**顺序 6 执行记录（2026-09-29）：** `src/stdlib/http3_server_internal.hpp` 将请求反向连接引用改为弱引用，并以 request ticket 明确区分监听器队列和外部登记项的连接所有权；已交付 ticket 同时保活监听器配置。`src/stdlib/http3_server.cpp` 在登记前标记请求已交付，所有回复异常都会关闭流并释放登记。`src/stdlib/http3_server_headers.cpp`、`src/stdlib/http3_server_events.cpp` 和 `src/stdlib/http3_server_connection.cpp` 在解析失败、对端流中止及连接关闭完成时清理请求并唤醒等待者；关闭完成后回收 QUIC 流句柄、连接句柄及 nghttp3 状态。`src/stdlib/http3_server_listener.cpp` 关闭监听器时清空未交付队列并终止未交付流，已交付请求仍可回复，释放外部 ticket 后连接才回收。

`scripts/build.ps1 -Incremental` 构建通过；`python -X utf8 scripts/check_http3_server.py`、`python -X utf8 scripts/check_http3_server_resources.py` 和 `txc test tests --format json` 通过。全量检查按顺序运行 31 个 `scripts/check_*.py`：30 个首次运行通过；资源脚本因测试服务恰好在 60 秒等待期限退出，加入中途心跳后重跑通过，最终 31 个脚本均通过。另显式运行 `check_data_formats.py json csv`，覆盖两个数据格式组。

资源测量使用 `scripts/check_http3_server_resources.py`：10 次预热断连后，服务进程基线为 389 个句柄、Private Bytes 6.14 MiB、Working Set 17.50 MiB；之后执行 1,000 次部分 POST 正文后断连，每 100 次用正常请求确认监听器仍能服务并采样。句柄峰值为 410（较基线 +21），第 800～1,000 次为 408（+19）；Private Bytes 峰值约 8.17 MiB（+2.03 MiB），第 1,000 次为 8.11 MiB（+1.96 MiB）；Working Set 第 1,000 次为 19.33 MiB（+1.83 MiB）。再空闲观察 60 秒，句柄保持 408、Private Bytes 保持约 8.12 MiB、Working Set 保持约 19.34 MiB。进程级数据在初始增量后趋于平台，未见随 1,000 次循环或后续 60 秒空闲继续增长；这些计数不能把平台增量归因到某个具体 Windows/MsQuic 对象，也不替代小时级长时间运行测试。

### 4.10 顺序 7：R07 及时回收 HTTP/3 已完成流

1. **建立流关闭状态。**
   1. 区分正常完成、对端中止、本端主动中止和连接整体关闭；记录发送缓冲及 `SEND_COMPLETE` 是否全部收束。
   2. 将对端请求流与控制、编码器、解码器流区分处理；控制流的连接级存活期不能套用到每条已完成的请求流。
2. **在安全时机释放三处状态。**
   1. 收到流关闭完成事件后，或在回调退出后的延迟回收点，恰好一次调用 `StreamClose`，并移除 `stream_handles_` 与 `streams_` 中对应记录。
   2. 同步更新 nghttp3 的流结束状态和 `requests_` 状态。不能在 MsQuic 仍使用 `stream_state` 回调上下文或发送缓冲时删除该对象。
3. **定向验收。**
   1. 同一条长连接连续处理多条正常请求，完成后的流表数量和 QUIC 句柄数回到稳定基线，而不是一直积累到连接析构。
   2. 对端中止与本端提前关闭各取一条路径，确认没有双重关闭、回调访问已释放对象或未完成请求残留。

**顺序 7 执行记录（2026-09-29）：** `src/stdlib/http3_server_internal.hpp` 为每条流记录请求/对端单向控制流/本端控制与 QPACK 角色、关闭原因、待完成发送数、发送方向关闭及最终关闭状态。客户端双向请求流与单向流按 QUIC stream ID 方向分类；单向控制/QPACK 流不会进入 `requests_`，连接级关键流直到连接关闭。正常完成、对端 abort、本端主动关闭和连接关闭分别记录；对端取消会移除未交付队列项、使尚未回复的外部请求句柄以 `connection_closed` 结束，并只对请求流提交一次双向 abort。监听器关闭仍允许已交付请求回复，回复或显式关闭后若无剩余交付请求才关闭连接。

每条流的 `SHUTDOWN_COMPLETE` 是最终回调；处理完该回调的逻辑后设置一次性关闭标志、从 `stream_handles_` 和 `streams_` 移除记录，并调用一次 `StreamClose`。请求流和对端单向流同步调用 `nghttp3_conn_close_stream`；关键控制流关闭按 HTTP/3 规则关闭连接。`SEND_COMPLETE` 负责释放拥有发送字节的 `send_state` 并减少待完成计数；最终流回调前不得释放 `stream_state`。外部 `close_stream` 在锁外调用 `StreamShutdown` 时保活流句柄；`StreamClose` 和 `ConnectionShutdown` 进行中保活连接句柄；所有流回调和句柄关闭完成后再关闭连接和 nghttp3 状态。成功 `respond` 在响应 FIN 的 `SEND_COMPLETE` 后报告成功；失败或取消关闭对应请求流。服务器已执行的部分外部效果无法由流取消回滚，调用方不应自动重试非幂等操作。

监听器关闭时原先出现 `0xC0000005`：原构建脚本将另一套 MinGW 的 `libwinpthread-1.dll` 与当前 g++ 配套的 `libstdc++-6.dll` 一起打包；崩溃地址落在 `pthread_cond_wait` 调用的 `pthread_mutex_unlock`，切换为当前 g++ 同目录的 DLL 后消失。`scripts/build.ps1` 已固定两者来源一致，同时把 ICU 所需的另一套线程运行库独立命名为 `libwinpthread-u.dll`，改写 ICU 和私有 `libstdc++-u.dll` 的导入名；编译器输出和兼容清单均包含此依赖。定向脚本也改为在服务进程仍存活时读取最终句柄数，再额外发送结束请求触发监听器关闭。

`pwsh -NoProfile -File scripts/build.ps1 -Incremental` 构建通过；`python -X utf8 scripts/check_http3_server_streams.py` 通过：一条连接先发送部分 POST 后由对端 reset，再发出本端主动关闭请求；随后同一连接完成 256 条正常请求。暖机 16 条后，每 16 条采样服务进程句柄数，最后一次测量增量为 1～2，最终仍为 358 个（较基线 +2）；额外结束请求后监听器关闭且服务进程正常退出。`python -X utf8 scripts/check_http3_server.py` 通过基本互操作、二进制往返和监听器关闭后已交付请求回复。`python -X utf8 scripts/check_http3_parallel.py`、`python -X utf8 scripts/check_package_compatibility.py`、`tx/txc.exe test tests/bytes_file_stream/encoding_incremental.tx --format json` 和 `tx/txc.exe test tests/time/calendar.tx --format json` 均通过，覆盖同连接双流并行、运行时依赖清单和 ICU 转换/日历。未运行全量测试；顺序 7 定向结果不覆盖小时级长时间运行或并发故障注入。

**顺序 7 独立全量复核发现与修复（2026-09-29）：** 独立复核的增量构建及 `tx/txc.exe test tests --format json`（1/1）通过；32 个 `scripts/check_*.py` 中 31 个通过，`python -X utf8 scripts/check_static_runtime.py` 的 9 个 TX 用例及 LLVM IR 检查通过，但原生 `runtime_workspace.exe` 在正则工作线程退出时发生 `0xC0000005`。GDB 定位到 `match_workspace` 的 C++ `thread_local` 析构：析构回调拿到的对象存储已被释放，调用其中的 PCRE2 释放函数指针时崩溃。将本机 g++ 的 `libgcc_s_seh-1.dll` 单独替换进临时运行目录仍复现，不能将这次崩溃归因于该 DLL 的版本差异；临时换回另一套 `libwinpthread` 虽可让此用例通过，却会重新引入前述 HTTP/3 关闭崩溃。

`src/stdlib/regex_match.cpp` 改用 Windows FLS 为每个线程保存正则工作区，由 FLS 退出回调释放 PCRE2 资源；重入时仍使用临时工作区，FLS 分配失败时也回退到临时工作区。修复后 `pwsh -NoProfile -File scripts/build.ps1 -Incremental`、`python -X utf8 scripts/check_static_runtime.py`（含原先崩溃的工作线程退出与 `regex_cancel`）、`python -X utf8 scripts/check_http3_server_streams.py`、`python -X utf8 scripts/check_http3_server.py` 和 `python -X utf8 scripts/check_package_compatibility.py` 均通过。HTTP/3 同连接流回收最后采样为 358 个句柄，与暖机基线相同，结束请求后监听器正常关闭。

修复后的另一名独立测试者完成全量复验：增量构建退出码为 0；`tx/txc.exe test tests --format json` 通过 1/1 个测试组、失败 0 个；32 个 `scripts/check_*.py` 全部通过，其中 `check_static_runtime.py` 的 `runtime_workspace` 与 `regex_cancel` 均报告 PASS；另显式运行 `check_data_formats.py json csv`，两个数据格式组通过。R07 状态保持“已修复”；全量复验不替代小时级长时间运行或并发故障注入。

### 4.11 顺序 8：R09 保留线程和任务的原始 TX 错误

1. **固定空结果语义。**
   1. `invoke_concurrent_callback()` 收到文本或复合结果的空指针时，先检查当前运行时上下文是否已有 TX 错误；若有，原类别、错误码、消息和调用栈必须原样进入线程或任务结果。
   2. 只有空指针且没有原始错误时，才生成“跨线程结果为空”的新运行时错误；正常返回值和其他结果类型保持原行为。
2. **保护外层捕获与上下文。**
   1. `thread_abi.cpp` 和 `task_runtime.cpp` 的 `catch` 分支仅在没有待传播 TX 错误时设置 `thread_failed` 或 `task_failed`。
   2. 检查任务帮助执行及线程复用时的错误上下文边界，避免从上一回调继承错误，或在复制错误快照前将其清空。
3. **定向验收。**
   1. 文本、复合结果分别通过线程 `join` 和任务 `wait` 主动抛出同一个 `io_error/audit_original`，逐项核对类别、码、消息与调用栈。
   2. 再用“无原始错误却返回空指针”确认仍能得到明确的运行时错误，不把真正的 ABI 异常静默吞掉。

**顺序 8 执行记录（2026-09-29）：** `invoke_concurrent_callback()` 在文本或复合回调返回空指针时检查当前运行时错误；已有 TX 错误时返回占位结果，由线程或任务在清理前复制原始错误快照，没有原始错误时仍抛出明确的空结果异常。`thread_abi.cpp` 与 `task_runtime.cpp` 的两类外层 `catch` 仅在当前上下文尚无 TX 错误时写入 `thread_failed` 或 `task_failed`。线程各自使用独立的线程局部运行时上下文；任务每次回调都建立临时上下文，嵌套等待的帮助执行结束后恢复外层上下文，错误快照均在回调句柄和本地对象清理前复制。

Windows x64 `pwsh -NoProfile -File scripts/build.ps1 -Incremental` 退出码为 0。`python -X utf8 scripts/check_concurrency_errors.py` 通过：TX 用例以文本、复合结果分别走 `thread.join` 与 `task.wait` 主动产生相同 `io_error/audit_original`，核对原类别、代码、消息、两层 TX 调用栈及源码文件；后续正常线程/任务仍返回预期文本。原生 ABI 用例对两类结果和两种交付方式分别验证无原始错误的空指针得到 `runtime_error/thread_failed` 或 `runtime_error/task_failed` 及明确消息；另以已有 TX 错误后抛出标准和非标准 C++ 异常验证外层捕获不覆盖原始快照，并验证嵌套任务等待的上下文隔离。`tx/txc.exe test tests/stdlib/concurrency_results.tx --format json` 通过 1/1，覆盖既有正常文本、复合结果和异步任务路径。本项未运行全量测试；R08 的异步网络任务池改造仍属顺序 9。

**顺序 8 独立全量复核中的资源脚本修正（2026-09-29）：** 首轮独立复核的增量构建退出码为 0，`tx/txc.exe test tests --format json` 通过 1/1；33 个 `scripts/check_*.py` 中 32 个通过，`check_http3_server_resources.py` 约 900 次部分正文断连后在 aioquic `wait_connected()` 抛出 `ConnectionError`。旧脚本失败时没有保留服务进程退出状态及输出，因此不能事后确认该次握手失败是否由服务端退出、连接上限或网络瞬态造成。单独重跑原有 1,000 次断连和 60 秒空闲观察通过；每 100 次之间的正常请求约间隔 17～18 秒，未触及服务端单次 `accept` 的 60 秒期限。

服务端对尚未完成原生回收的连接设置 16 个上限；同一服务进程上的 24 路并发握手探针得到 16 次成功、8 次 QUIC 错误码 2（`CONNECTION_REFUSED`），服务进程保持运行，证实该上限也会表现为 aioquic 的 `wait_connected()` `ConnectionError`。`scripts/check_http3_server_resources.py` 现在读取握手终止码，只对错误码 2 在每次部分正文断连前最多等待 5 秒并重试，保持实际完成 1,000 次断连的计数；其他握手错误直接失败，失败时记录服务进程状态及输出。16 路暂占连接后的定向探针记录 3 次容量拒绝，随后一次断连成功且服务进程仍存活。修订后完整资源脚本通过：1,000 次断连、60 秒空闲观察、最大句柄增量 21、最终私有内存增量 1.84 MiB，容量拒绝计数为 0。此前全量失败的具体终止码未捕获，本次修正针对已复现的同症状容量拒绝路径。顺序 7 的流回收实现和顺序 8 的 TX 错误传播实现均未改动。

**顺序 8 修正后的独立全量复验（2026-09-29）：** 另一名独立测试者执行增量构建，退出码为 0；`tx/txc.exe test tests --format json` 通过 1/1 个测试组；33 个 `scripts/check_*.py` 全部通过，其中 `check_concurrency_errors.py` 通过；另显式运行 `check_data_formats.py json csv`，两个数据格式组通过。资源脚本完成 1,000 次部分正文断连和 60 秒空闲观察，容量拒绝计数为 0；暖机基线 374 个句柄，峰值较基线增加 24，最终 396 个（较基线增加 22），最终 Private Bytes 增加 2.18 MiB、Working Set 增加 2.11 MiB。R09 状态保持“已修复”；本次复验不替代小时级长时间运行或并发故障注入。

### 4.12 顺序 9：R08 将异步网络等待移出通用任务池

**实施前契约（2026-09-29）：** 四类 TCP 异步操作沿用同步接口的参数、错误和结果：连接/接受只在成功时交付新流，未交付的连接在失败、超时、取消时释放；读取可短读，对端发送方向关闭返回空数据及 EOF，重复读取继续返回 EOF；写入单次最多确认 16 KiB 且可短写。超时为 `io_error/timeout`，令牌或作用域取消为 `cancelled_error/cancelled`，令牌截止时间为 `cancelled_error/deadline_exceeded`；本端关闭后按流或监听句柄分别报 `connection_closed`、`closed_handle`。同一 socket 的同方向操作串行，读写可并行。取消、超时、关闭不能回滚已接受的连接、已消费的数据或已发出的字节，成功结果一经交付不再由取消改写。独立网络事件循环最多登记 1024 项，满额提交立即报 `io_error/network_queue_full`；异步期间保活 socket 及写入缓冲，关闭时收束挂起项。UDP 两类异步等待也从通用任务池移走，维持同步 UDP 报文语义。

1. **先固定四类操作的契约。**
   1. 列出 `connect_async`、`accept_async`、`read_async`、`write_async` 的超时、令牌及作用域取消、EOF、短读/短写、关闭后操作和部分外部效果，与现有同步 socket 行为逐项对齐。
   2. 保留同一 socket 上读、写方向的并发约束；异步挂起期间要持有底层 socket 与缓冲，但不占住通用任务工作线程。明确可登记操作数量和队列满时的背压错误。
2. **改造运行时调度。**
   1. 优先利用现有 `WSA_FLAG_OVERLAPPED` socket 和 R04 修复后的 IOCP 完成循环接入网络操作；如选独立网络事件循环，也须达到“等待不占通用任务线程”的同一目标。
   2. 按读取/写入、连接/接受两组落实提交、完成、取消和超时；只有就绪后的短小结果装配或用户回调进入通用任务队列。关闭 socket 时收束待处理操作，结果只交付一次。
   3. 检查 `socket_async_abi.cpp` 中所有直接执行 `work(probe)` 的等待式路径，不能只把 TCP 读取迁走而留下连接、接受或写入继续占满任务池。
3. **定向验收。**
   1. 复用报告中的 8 个慢读取加一个普通计算任务场景，确认普通任务在读取超时前完成，且慢读取仍按原超时语义结束。
   2. 分别核对取消、远端关闭、队列容量边界以及异步操作后 socket 句柄状态；只做直接受本项影响的验证，不以增加任务工作线程数作为修复结果。

**顺序 9 执行记录（2026-09-29）：** `src/backend/cpp/socket_async_abi.cpp` 的六个异步入口改为登记网络操作，不再向 `task_executor` 提交等待式 `work(probe)`。新增 `socket_async_loop.hpp/.cpp` 用一条独立线程对非阻塞 socket 执行 `WSAPoll` 就绪等待、令牌/作用域取消、超时、关闭检查与一次性任务交付；`socket_async_tcp.cpp` 实现连接、接受、读取、写入的短步骤，`socket_async_udp.cpp` 实现发送、接收的短步骤；`CMakeLists.txt` 纳入构建。操作登记表最多 1024 项，满额立即返回 `io_error/network_queue_full`。登记项持有 socket、输入缓冲及任务状态，同 socket 同方向按登记顺序推进；同步 API 的方向互斥锁仍保护每次非阻塞实际 I/O。`docs/socket.md` 和 `docs/task.md` 同步运行时与错误契约，原有同步实现未修改。

Windows x64 `pwsh -NoProfile -File scripts/build.ps1 -Incremental` 退出码为 0。`tx/txc.exe test tests/network/socket_behavior.tx --format json` 通过 1/1，核对既有 TCP/UDP 异步成功及预取消。新增 `socket_async_dispatch.tx` 通过 1/1：8 个 1200 毫秒慢读取挂起期间，普通任务返回 42 且等待少于 900 毫秒，随后 8 个读取均按 `io_error/timeout` 结束。`socket_async_contract.tx` 通过 1/1，核对四类 TCP 操作的令牌/作用域取消、令牌截止时间、监听器取消后复用、短读与最多 16 KiB 的短写、EOF 与重复 EOF、远端关闭、本端关闭时挂起任务收束和关闭后错误，并核对 UDP 两个入口的取消及正常报文。`socket_async_capacity.tx` 通过 1/1，1024 个挂起接受操作后第 1025 项得到 `network_queue_full`，作用域退出取消并收束挂起项。`git diff --check` 通过。本阶段仅做增量构建和这些定向用例；外网异常注入与高并发长时负载未纳入本阶段验证，独立全量复验记录见下文。

**顺序 9 独立全量复核中发现的间歇崩溃及修复（2026-09-29）：** 首轮增量构建退出码为 0、`tx/txc.exe test tests --format json` 通过 1/1；33 个 `scripts/check_*.py` 中 32 个通过，唯一失败为 `check_tls_stream.py` 的 `TLS_RANDOM_CONCURRENCY`：前五项 TLS 场景通过，服务端退出码 0，客户端退出码 `0xC0000374`（Windows 堆损坏）。该脚本单独重跑一次六项全通过，但不能据此判定故障消失。同一证书和编译产物连续运行随机并发客户端，第 194 次又得到 `0xC0000005`，服务端仍正常退出；该用例只使用同步 socket，未创建顺序 9 的 `WSAPoll` 网络循环。

GDB 与分步隔离确认了三个生命周期缺陷。`tls_identity.cpp::close_identity()` 原先在 `identity_state::mutex` 的锁卫士仍存活时从登记表删除最后一个 `shared_ptr`，随后解锁已析构的互斥锁；GDB 捕获到 `close_identity -> pthread_mutex_unlock` 的访问违规。单独修复此处后，工作线程退出仍在 `cycle_gc.cpp` 的非平凡 `thread_local optional<shared_lock>` 析构中崩于 `pthread_rwlock_unlock`；将读锁所有权移到 `concurrent_execution_scope` 的栈对象后，线程退出仍在 MinGW `__cxa_thread_atexit` 的 `free` 路径报告 `0xC0000374`。工作线程剩余的非平凡 C++ 线程局部运行时上下文改为 Windows FLS 槽管理，并在退出回调回收。三处修复均限于身份关闭、GC 锁和运行时上下文生命周期；密码学随机源、同步 socket 和顺序 9 网络循环的算法未改。

修复后 Windows x64 增量构建退出码为 0。GDB 下随机并发 TLS 客户端连续 5 次及普通运行连续 300 次均正常结束；完整 `check_tls_stream.py` 的 6 项通过。`socket_async_dispatch.tx`、`socket_async_contract.tx`、`socket_async_capacity.tx` 各通过 1/1，`check_concurrency_errors.py` 与 `check_runtime_context.py` 通过。本次仅作故障相关的定向复验；独立全量复验结果见下文。

**顺序 9 修复后的独立全量复验（2026-09-29）：** 另一名独立测试者执行增量构建，退出码为 0；`tx/txc.exe test tests --format json` 通过 1/1 个测试组；33 个 `scripts/check_*.py` 全部通过，其中 `check_tls_stream.py` 的 6 项（含 `TLS_RANDOM_CONCURRENCY`）均通过。另显式运行 `check_data_formats.py json csv`，两个数据格式组通过。HTTP/3 资源脚本完成 1,000 次部分正文断连和 60 秒空闲观察，容量拒绝计数为 0，最大句柄增量 18，最终 Private Bytes 增加 1.07 MiB；`socket_async_dispatch.tx`、`socket_async_contract.tx`、`socket_async_capacity.tx` 各通过 1/1。R08 状态保持“已修复”；本次复验不替代外网异常注入或高并发长时负载验证。
