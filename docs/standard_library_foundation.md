# 标准库公共契约与现状清单

本文件落实[实施顺序第 1 节](standard_library_plan.md#1-盘点与公共契约)。下表保留第 1 节盘点时的基线，后续完成状态以实施清单为准。基线依据仓库中的 `tx/stdlib/*.txh`、`src/stdlib/`、模块文档与构建配置逐项核对；“已有”仅指接口和实现入口存在，不等于终态验收通过。后续小项按本文的模块、错误、资源和兼容规则落地，具体功能范围仍以[终态设计](standard_library_complete_design.md)为准。

## 1. 终态能力、现有入口与证据缺口

| 终态能力 | 已有公开接口与实现入口 | 缺失接口或能力 | 当前证据与待补验收证据 |
| --- | --- | --- | --- |
| 集合与算法 | 内置 `vector/map/set/heap/queue`；`array.txh`、`dictionary.txh`、`algorithm.txh`；`src/stdlib/array.cpp`、`dictionary.cpp`、`algorithm_numeric.cpp` 及类型化容器 ABI。见[容器](typed_containers.md)、[算法](algorithm.md) | `map` 的复合值、`heap/queue` 的复合元素、`deque`、有序容器、稳定排序和组合算法 | 第 1 节盘点时已有 `scripts/check_vectors.py` 等既往记录；后续 2.1 对复合 `vector`、2.3 对用户哈希键、2.4 对 `iterator<T>` 补充了定向证据，其他容器仍待对应小项验证 |
| 文本与编码 | `string.txh`、`format.txh`、`bytes.txh`、`encoding.txh`；`src/stdlib/string.cpp`、`format.cpp`、`bytes.cpp`、`encoding.cpp`。见[字节与流](bytes_file_stream.md) | Unicode 规范化、完整大小写、字素、正则、UTF-32、增量转换与 URL 安全 Base64 | 现有编码/字节示例与 `tests/bytes_file_stream/`；跨块残缺、Unicode 数据版本、正则限额及多语言位置单位未专项验证 |
| 文件与系统 | `file.txh`、`file_stream.txh`、`fs.txh`、`path.txh`、`system.txh`、`env.txh`；`src/stdlib/file*.cpp`、`filesystem*.cpp`、`path.cpp`、`system.cpp`、`env.cpp`。见[流](bytes_file_stream.md)、[系统](system_env.md) | `process`、管道、原子替换、文件监视、权限/符号链接/元信息与能力查询 | 已有 `scripts/check_system_env.py` 的既往定向记录；短读短写、符号链接竞争、断电/跨卷和子进程清理仍待专项证据 |
| 时间与数学 | `time.txh`、`random.txh`、`math.txh`；`src/stdlib/time.cpp`、`random.cpp`、`math.cpp` | 日期时区、独立随机数生成器、扩展数学、`statistics`、`decimal` | 现有 `examples/time_random.tx` 和标准库示例；夏令时歧义、随机算法版本、统计空样本与十进制舍入未验证 |
| 网络 | `httpx.txh`、`websocket.txh`、`requests.txh`；`src/stdlib/httpx*.cpp`、`http2*.cpp`、`ws*.cpp`、`requests_*.tx`。见[网络](network.md)、[Requests](requests.md) | 公开 `dns/socket/tls`、异步 I/O、连接池、代理/证书配置、HTTP/3、服务端并发与完整流式会话 | 有 `tests/network/` 的既有本地场景；HTTP/2 并行流、非法帧、慢连接、重定向凭据和不可信输入限额尚无终态专项证据。`requests` 的部分选项仍以 `unsupported_option` 拒绝 |
| 数据格式 | `json.txh`、`parse.txh`；`src/stdlib/json_*.cpp`、`parse.cpp`。见[JSON](json.md)、[解析](errors_and_parse.md) | JSON 增量和 schema、`csv/xml/cbor/serde` | 有 `tests/json/`、`scripts/check_parse_errors.py` 的既往场景；大数据流、XML 实体禁用、CBOR 规范化与跨 schema 迁移未验证 |
| 并发 | 无公开线程/任务/通道模块；现有同步网络和无捕获顶层函数值见[函数值](function_values.md) | `thread/task/channel`、同步原语、取消、`async/await`、IPC | 无 TX 用户级并发验收证据；数据竞争、取消竞态、GC/析构和句柄上限待第 10 节 |
| 数据库 | 无公开数据库模块或对应 `src/stdlib` 实现 | 统一 `db.txh` 的 SQLite/PostgreSQL 驱动、池与迁移接口 | 无验收证据；参数绑定、事务回滚、NULL/无行、TLS、池耗尽与断连待第 12 节 |
| 测试与诊断 | `test.txh`、`log.txh`、`debug.txh` 和 `txc test`；原有 `scripts/check_*.py` 与 `tests/` 是仓库自身验证工具。见[测试、日志与诊断](test_log_debug.md) | `profile.txh`、`txc profile`；参数化/性质测试、可配置隔离/并行/超时、轮转文件与并发日志 | 第 3 节已有断言位置、错误/编译错误统计、结构化遮蔽和 Release 栈的定向证据；原生崩溃注入、并发完整性与采样精度待第 13 节专项验证 |
| 安全 | `crypto.txh`；`src/stdlib/crypto_*.cpp`，Mbed TLS 后端。见[密码学](crypto.md) | `secret_bytes/password/public_key/x509/tls`、流式认证加密 | 有 `tests/crypto/` 的既有本地场景；标准向量、跨库互操作、证书失败、整体认证与秘密清理限制待第 9 节 |

上表中的测试脚本和示例只是证据入口，本次盘点没有重新运行它们。每个后续小项应在完成记录中分别列出接口、实现、构建、正常与失败路径、资源清理、平台限制；不能用上表“已有”代替终态矩阵验收。

## 2. 模块、类型、命名与兼容

公开文件统一为小写 snake_case 的 `模块名.txh`，用 `import "模块名.txh" as 模块名` 访问；公开函数、字段、类型与新文件也用小写 snake_case。现有 `requests` 名称及其外部兼容参数名保留。用户本地 `.txh` 仍优先于同名标准库文件；同名未命名导入的歧义仍在使用处报告。`error.txh` 是跨模块错误类型和结果信息的唯一来源，新模块不得各自定义同名 `error_info`。`bytes`、`str`、`duration`、`option<T>`、`result<T>` 等类型只按语言规则共享；数据库行等确实动态的数据可继续使用 `any/array/dict`。

| 领域 | 固定的新增公开模块/类型 | 直接依赖 |
| --- | --- | --- |
| 类型与集合 | 内置 `option<T>/result<T>/iterator<T>/deque<T>/ordered_map<K,V>/ordered_set<T>`；现有 `vector/map/set/heap/queue` 扩展 | `error.error_info`、编译期比较/哈希和所有权规则 |
| 运行时基础 | `cancel.txh` 与内置 `cancel_source/cancel_token`；后续线程和任务接口接入同一取消状态 | 单调时钟、`cancelled_error`、资源部分效果规则 |
| 文本与数据 | `unicode.txh`、`regex.txh`、`csv.txh`、`xml.txh`、`cbor.txh`、`serde.txh` | `string/bytes/encoding`、文件流、`error` |
| 时间与计算 | `statistics.txh`、`decimal.txh`；`time` 内增加 `duration/instant` 与日历类型；`random` 内增加生成器类型 | `error`；时区数据随工具链 |
| 系统与进程 | `process.txh`；扩展 `file/file_stream/fs/path/system/env` | `bytes`、文件流、`error`、取消令牌 |
| 网络 | `dns.txh`、`socket.txh`、`tls.txh`；扩展 `httpx/websocket/requests` | `bytes`、流、任务/取消、证书与 `error` |
| 并发与数据库 | `thread.txh`、`task.txh`、`channel.txh`、`sync.txh`、`ipc.txh`；统一的 `db.txh` 提供 SQLite/PostgreSQL 驱动、池与迁移 | `Send/Sync`、取消、`result`、`cbor`、`error` |
| 工具与安全 | `test.txh`、`log.txh`、`debug.txh`、`profile.txh`；内置 `secret_bytes` 类型及 `password.txh`、`public_key.txh`、`x509.txh` | 源码映射、时间、`error`、秘密遮蔽规则 |

这些名称是后续新增接口的注册表，不表示文件现在已经存在。跨模块方向从基础类型/错误/流指向上层模块；`requests → httpx → socket/tls`，`db` 的连接池依赖 SQLite/PostgreSQL 驱动，不得形成反向依赖。旧签名、返回类型、错误 `kind/code`、编码、排序和共享语义保留；若新语义不能兼容，采用新名字，例如 `unicode.normalize` 不改变 `string.lower`，`random` 的生成器接口不改变 `random.seed`，协议协商不改变 `httpx.send_http2`，大文件加密不覆盖 `crypto.encrypt` 的 `TXCG` 格式。每个不等价替换须给出迁移示例。

终态设计中的未绑定 `T/K/V` 目前只是记法。当前 `.txh` 可声明**具体实例**，例如 `vector<point>`、`option<int>`、`result<int>`；不能直接把未绑定的 `vector<T>`、`result<T>` 或 `def name<T, U>(...)` 写入接口。第 2.1～2.2 项已扩展编译器的内置参数化类型实例化；后续真实 `.txh` 的泛型自由函数采用受限的 `def name<T, U>(...) -> ...` 声明，类型变量由调用实参的静态类型确定，约束由编译器已登记的相等、哈希、排序或跨线程契约检查，不支持用户自定义模板实现。该声明语法落地前先更新 `syntax.md` 与可运行示例；新泛型自由函数在真实 `.txh` 可解析并由静态检查选定前，不计为已交付。

## 3. 错误类别与稳定代码登记

错误身份是 `(kind, code)` 二元组；`message` 为中文人类说明，允许改进措辞，程序不能按它分支。保留原 ABI 数值 `runtime=1`、`parse=2`、`io=3`；新增 `process=4`、`database=5`、`security=6`、`cancelled=7`。`error.txh` 中对应类型均有 `kind/code/message` 字段，`runtime_error` 继续作为兜底捕获。新类别已可声明和捕获，但对应未来模块尚未实现，不能把类别存在理解为操作可用。

| 来源 | 已用且保留的 `kind/code` | 新接口预留的 `kind/code` |
| --- | --- | --- |
| `parse`、`json`、`bytes` | `parse_error/empty_input`、`invalid_base`、`invalid_syntax`、`out_of_range`、`non_finite`、`invalid_escape`、`invalid_utf8`、`missing_field`、`type_mismatch`、`depth_limit`、`invalid_hex`、`invalid_base64` | `parse_error/invalid_encoding`、`size_limit`、`duplicate_key`、`schema_mismatch`；CSV/XML/CBOR/regex 须在模块文档补充输入位置与限额 |
| 文件流与现有网络 | `io_error/not_found`、`permission_denied`、`invalid_path`、`invalid_mode`、`closed_stream`、`invalid_encoding`、`size_limit`、`timeout`、`connection_closed`、`protocol_error`、`invalid_header`、`invalid_argument`、`invalid_utf8`、`unsupported_option`、`operation_failed` | `io_error/closed_handle`、`short_read`、`short_write`；新 socket/TLS/文件操作沿用可适用的旧码 |
| 通用运行时、JSON 写入、现有 `crypto`、测试与日志 | `runtime_error/allocation_failed`、`unknown_error`、`operation_failed`、`invalid_argument`、`size_limit`、`invalid_indent`、`cyclic_value`、`unsupported_value`、`invalid_key`、`random_failed`、`authentication_failed`、`invalid_format`、`assertion_failed`、`invalid_tolerance`、`invalid_name`、`invalid_level`、`invalid_field` | `runtime_error/invalid_state`；现有 `crypto` 错误不悄悄改类，新安全 API 才使用 `security_error` |
| 子进程 | 尚无公开入口 | `process_error/spawn_failed`、`wait_failed`、`terminated`、`output_limit`、`invalid_state`、`timeout` |
| 数据库 | 尚无公开入口 | `database_error/connection_failed`、`query_failed`、`constraint_violation`、`busy`、`pool_exhausted`、`invalid_state` |
| 新密码/证书/TLS 接口 | 尚无公开入口 | `security_error/invalid_key`、`authentication_failed`、`invalid_certificate`、`certificate_expired`、`hostname_mismatch`、`untrusted_issuer`、`random_failed` |
| 取消与截止时间 | 尚无公开入口 | `cancelled_error/cancelled`、`deadline_exceeded`；已发生的外部效果由操作结果另行说明 |
| 时间与计算扩展 | 6.1～6.6 已使用 `runtime_error/invalid_argument`、`out_of_range`、`timezone_failed`、`non_finite`、`empty_sample`、`insufficient_sample`、`division_by_zero`、`size_limit`、`invalid_state`；`parse_error/invalid_syntax`、`out_of_range`、`invalid_zone`、`nonexistent_time`、`ambiguous_time`；`cancelled_error/cancelled`、`deadline_exceeded` | 后续新增错误码须先在对应模块文档固定语义 |

同一 `code` 可出现于不同 `kind`，所以只按完整二元组判断。新增代码先登记到本表与模块文档，不重复赋予不同含义；旧的 `operation_failed` 保持兼容，新接口优先使用能区分失败原因的代码。缺键、EOF、无匹配、查询无行是正常分支，使用 `option<T>` 或显式状态；可能失败的严格接口抛上述错误，`try_*`/`result<T>` 使用完全相同的二元组。现有具体 `error.*_result` 保留到无损转换桥就绪。取消与超时分开，取消不能吞掉已经完成的外部效果。

新代码的判定边界也固定：`process_error/spawn_failed` 表示未交付子进程，`wait_failed` 表示回收失败，`terminated` 表示非正常结束，`output_limit` 表示捕获上限耗尽，`timeout` 不隐式杀进程。`database_error/connection_failed` 是建连失败，`query_failed` 是 SQL 执行失败，`constraint_violation` 是约束冲突，`busy` 是可重试的锁/繁忙状态，`pool_exhausted` 是池无法在约定期限内提供连接。`security_error/invalid_key` 是密钥格式或长度不合法，`authentication_failed` 是认证标签不匹配；`invalid_certificate` 是证书解析或用途不合法，`certificate_expired`、`hostname_mismatch`、`untrusted_issuer` 分别表示过期、主机名不符和信任链缺失，`random_failed` 表示密码学随机源失效。`cancelled_error/cancelled` 来自显式取消，`deadline_exceeded` 来自截止时间到达。各模块仍须在其接口文档写明 `invalid_state`、短读短写和部分效果的具体状态迁移。

## 4. 资源句柄状态表

新文件、socket、进程、线程和数据库句柄共同遵循以下状态规则。已有 `file_stream` 的重复 `close` 无操作、关闭后操作报 `io_error/closed_stream` 等行为保持原样；旧 HTTP/WS 的 `id` 包装和其他既有例外不在本轮重解释，迁移时逐项记录。

| 起点与动作 | 结果状态 | 所有权、错误和外部效果 |
| --- | --- | --- |
| 创建成功 | `open`，唯一拥有者 | 分配和外部创建全部成功后才交付句柄；失败不交付半成品，已建资源由实现清理 |
| 借用 | 所有者仍 `open`，借用视图不拥有 | 借用不能关闭/转移；不得越过所有者生命周期或跨线程规则；所有者关闭后借用失效 |
| 显式共享 | 多个受控别名指向同一状态 | 仅类型明确允许时共享；关闭底层资源后所有别名同步失效，引用计数或同步细节不暴露给 TX |
| `close` 成功或关闭过程中失败 | `closed` | `close` 可重复调用且重复调用无效果；第一次关闭失败仍标记关闭并报告稳定错误，不能假定写入已持久化 |
| `closed` 后操作 | 仍 `closed` | 返回对应模块的 `closed_handle`/既有 `closed_stream`/`connection_closed`，不得访问失效原生句柄 |
| 普通操作失败 | 通常仍 `open` | 若协议或底层资源已不可恢复，转 `failed` 并记录具体错误；每个接口文档写明能否重试以及已读写的前缀 |
| `failed` 后操作 | `failed` 或显式 `closed` | 只允许查询错误、状态和 `close`；不隐式恢复。重连/重开创建新句柄 |
| 作用域结束 | `closed` | 自动清理只作兜底；文件持久化用显式 `flush/close`，事务用显式 `commit/rollback`，清理失败不得伪装为成功 |

进程 `wait` 成功后进入 `completed`，重复 `wait` 返回缓存退出状态；`terminate/kill` 仅请求结束，仍须 `wait` 回收。线程 `join` 或 `detach` 各只允许一次，之后再调用报 `invalid_state`，作用域清理不留下未知后台任务。数据库连接的关闭使所属语句/游标失效；事务错误后必须显式回滚或按已记录状态清理。流式写入、网络发送、子进程启动和事务提交可能已有外部效果；错误/取消结果至少说明写入前缀、进程是否已启动、事务是否确定提交，不能把异常等同回滚。新模块可细化状态，但不得改变表中所有权和重复关闭规则。

## 5. 包兼容指纹与依赖登记

`scripts/build.ps1` 在标准库静态库全部归档完成后写出 `tx/package.compat`。`txc` 内嵌构建时按文件名排序得到的每个 `.txh` SHA-256 与运行时 ABI 指纹；包清单登记 ABI 指纹、`txc.exe` SHA-256、最终 `libtxstdlib.a` SHA-256。运行 `check`、`emit-llvm` 和编译前逐项验证；不匹配时以中文指出具体产物并停止。构建中使用的 `emit-library-llvm` 只验证 `.txh`，因为最终静态库尚未生成。ABI 指纹包含错误类别、C ABI 头和 LLVM 代码生成源码的保守内容摘要；仅保证同一构建包的一致性，不作为恶意篡改的签名。升级 ABI 或接口必须重新构建并整体交付 `tx/`。

| 已引入的第三方依赖 | 版本与下载 SHA-256 | 许可证与发行方式 |
| --- | --- | --- |
| nghttp2 | `1.68.0`；`5511d3128850e01b5b26ec92bf39df15381c767a63441438b25ad6235def902c` | MIT；源码由 CMake `FetchContent` 校验后静态归档，`tx/NGHTTP2-LICENSE` 随包 |
| Mbed TLS | `3.6.5`；`4a11f1777bb95bf4ad96721cac945a26e04bf19f57d905f241fe77ebeddf46d8` | Apache-2.0 或 GPL-2.0-or-later，发行采用 Apache-2.0；静态归档，`tx/MBEDTLS-LICENSE` 随包 |

未来的 ICU、PCRE2、libxml2、QCBOR、SQLite、libpq、Argon2、libsodium、MsQuic 等当前**未引入**。各模块实现前须在构建配置中固定准确版本、源码归档 SHA-256、许可证选择及交付文件，再列入本表；不得以开发机已安装的随机版本充当发行条件。系统 Win32/IOCP 与仓库已有工具链另按平台发行说明处理。

### 第 1 节完成记录（2026-09-26）

- **代码：** 错误类别 4～7、`error.txh` 公开结构、模块加载器和语义分析已接通；CMake 生成接口与 ABI 摘要，`txc` 校验 `package.compat` 与实际文件。资源状态和未实现模块的错误码是后续实现必须遵守的契约，尚未宣称对应功能已经存在。
- **构建：** `scripts/build.ps1` 成功生成 `txc.exe`、合并后的 `libtxstdlib.a` 和 `package.compat`，确认产物后清理 `build/`。
- **定向验证：** `examples/parse_errors.tx` 编译并运行，退出码为 0；`scripts/check_package_compatibility.py` 的正常包、接口错配、ABI 错配、编译器错配、静态库错配五个场景通过。未运行与此改动无关的全量测试。
- **边界：** 当前校验面向 Windows x64 的本仓库构建包；不覆盖 `clang.exe`、`link/` 或运行时 DLL 的独立完整性，也不是发布签名。未引入新的第三方依赖；未来依赖仍须逐项锁定版本、校验值、许可证与发行内容。
- **终态验收：** 第 2～15 节及十类能力的终态矩阵仍待后续小项实施与专项验证。
