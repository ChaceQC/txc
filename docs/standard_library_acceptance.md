# 标准库终态验收记录

日期：2026-09-30。目标平台：Windows x64。依据[终态设计第 14 节](standard_library_complete_design.md#14-终态验收矩阵)逐类核对。未改动模块复用仓库内已有专项记录；本轮运行与新增工具链、运行时分配/回收、跨模块链和新增输入变异直接相关的检查，没有重跑全库脚本。

## 十类能力映射

| 类别 | 公开实现、中文契约与已有证据 | 本轮补充 |
| --- | --- | --- |
| 集合与数据结构 | 内置类型及 `algorithm/array/dictionary`；[类型化容器](typed_containers.md)、[算法](algorithm.md)，计划 4.1～4.6 的复合值、排序/哈希、迭代、共享/深复制、比较失败及析构记录 | 安装包 `algorithm_extended`；运行时闭包循环、析构复活与哈希回调错误复核 |
| 字符串与编码 | `string/bytes/encoding/unicode/regex`；[文本与编码](unicode_regex_encoding.md)，计划 5.1～5.5 的非法编码、增量尾块、字素/字节单位及正则限额记录 | 安装包 Unicode/字节流示例；128 个输入的正则变异与限额处理 |
| 文件与系统 | `file/file_stream/fs/path/system/env/process/async_file`；[文件系统](filesystem_path.md)、[进程](process.md)，7.1～7.7 的短读/写、原子替换、权限、链接竞争、背压、清理记录 | 安装包异步文件；新运行器的 cwd/进程树隔离、SEH、超时与输出上限；IOCP 部分写入/取消回归 |
| 时间与数学 | `time/random/math/statistics/decimal`；[时间数学统计](time_math_statistics.md)，6.1～6.6 的 DST、日历溢出、随机版本、NaN/空样本、舍入记录 | 安装包十进制示例；可控测试时钟与单调分段计时 |
| 网络 | `dns/socket/httpx/websocket/requests/tls`；[网络](network.md)、[专项验收](network_11_9.md)，11.1～11.9 的 HTTP/1.1/2/3、WS、代理、TLS、限额、取消和大文件记录 | 并发 HTTP 入库链、双向 TLS 链；8 个 HTTP/2 畸形前言；安装包 TCP/UDP/HTTP/WS 入口 |
| 数据格式 | `json/csv/xml/cbor/serde/parse`；[JSON](json.md)、[CSV](csv.md)、[XML](xml.md)、[CBOR](cbor.md)、[serde](serde.md)，8.1～8.6 的深度/长度、重复/未知/缺字段、实体拒绝、迁移与部分写入记录 | JSON 文件入库并核对 CSV/XML；独立 CBOR 示例；128 个输入 × 4 格式变异 |
| 并发 | `thread/sync/channel/task/ipc`；[线程](thread.md)、[同步](synchronization.md)、[通道](channel.md)、[任务](task.md)，10.1～10.7 及 R04/R08/R09 修复记录 | 新上下文继承和恢复；运行时 GC/错误栈回归；IOCP 取消状态机；安装包线程/锁/通道/IPC 示例 |
| 数据库 | `db`；[数据库](db.md)、[第十二部分验收](database_acceptance.md)，SQLite 与 PostgreSQL TLS、绑定、NULL、流式结果、迁移、池化、断连及取消记录 | 两条业务链核对事务及跨线程池使用；128 个 SQLite prepare 变异；8 个 TLS 后 PostgreSQL 畸形认证/长度帧 |
| 测试与调试 | `test/log/debug/profile`；[工具链](toolchain_completion.md)、[性能分析](profile.md) | 参数/性质/夹具/时钟、失败种子与缩减、并行/隔离/超时/崩溃分类、轮转/脱敏/上下文、CPU/内存源码报告及普通/分析基准样本 |
| 安全 | `secret/password/crypto/public_key/x509/tls`；[密码学](crypto.md)、[秘密字节](secret_bytes.md)、[证书](x509.md)，9.1～9.7 与 R01/R02/R10 记录 | 再运行标准向量、跨库签名/KDF/认证加密、PKCS#8、随机失败及资源清理；Argon2id→证书解锁→双向 TLS→认证加密文件链 |

全部公开 `.txh` 使用独立安装包的同一个导入程序经过语义分析和 LLVM 发射；再把生成的 `txrt_*` 声明与发布静态库的全局定义逐项比较。各模块的声明数量、接口 SHA-256、中文文档及直接使用它的公开示例由 `scripts/check_release_package.py` 输出到 `tx_build/release_inventory.json`。脚本要求每个模块都有中文文档和公开示例，安装目录只有 `tx/`、随包示例/文档及验证工作区，不创建 `src/` 或 `tests/`。

最终结果：52 个公开模块、2214 个 `txrt_*` 声明均匹配发布静态库；17 个代表性公开示例从复制后的随包目录编译并运行通过。工具链、接口、库和 ABI 的正常兼容及四种错配检查均通过。第十三部分 13.1～13.6 的 Windows x64 完成记录已同步到实施清单。

## 本轮可复现的聚焦证据

| 检查入口 | 验证内容 |
| --- | --- |
| `scripts/check_toolchain_completion.py` | 用户测试 API、两个相同种子反例报告、夹具/时钟、独占目录、并行固定顺序、超时及编译失败统计、CPU/堆/源码/基准报告 |
| `scripts/check_completion_chains.py` | 三条业务链；真实原生崩溃、5 MiB 输出截断、正常退出后的后代清理。`native` 参数只重跑原生边界 |
| `scripts/check_completion_corpus.py` | seed=20260930，128 个有界变异输入分别通过 JSON/CSV/XML/CBOR/正则/SQLite prepare；验证返回或结构化失败，不要求所有变异都被拒绝 |
| `scripts/check_completion_protocols.py` | seed=20260930，8 个 HTTP/2 前言变异；临时 CA 验证后注入 8 个 PostgreSQL 畸形认证/长度帧，确认有界数据库错误 |
| `scripts/check_runtime_context.py` | TX 错误位置、错误回调、闭包循环、析构复活、哈希错误和内部上下文传递 |
| `scripts/check_task_iocp.py` | 完成/取消状态机、停止时在途操作、部分写入、TX 异步文件行为及在途取消 |
| `scripts/check_crypto_acceptance.py` | 标准向量与跨库互操作、固定种子密文变异、密钥/证书/认证失败及清理 |
| `scripts/check_release_package.py`、`scripts/check_package_compatibility.py` | 52 个模块的接口/符号、依赖许可证、独立安装包运行示例、正常指纹与四种错配 |

## 支持状态与不能扩大解释的边界

- Windows x64 是当前可构建和交付的平台。库接口、静态符号、固定依赖、运行时 DLL、`package.compat`、中文文档和公开示例一起交付；源码、编译器和二进制库必须来自同一包。`scripts/build.ps1` 是构建入口，不承诺不同宿主编译器生成字节完全一致的二进制。
- HTTP/3/MsQuic、证书系统信任、进程/IOCP 等使用 Windows 后端；Linux/macOS 没有本轮构建或验收结果，不能把 Windows 的勾选当作这些平台已支持。跨网络 QUIC 迁移、操作系统断电/崩溃恢复、长期随机源统计与外部代理部署需在对应实际环境验证。
- 这里的模糊验证是固定种子的有界变异与已有恶意输入用例，附有可复现入口；不是覆盖引导式持续 fuzz，不证明不存在其他输入缺陷。历史 HTTP/3 1,000 次断连/60 秒资源观察及 TLS 并发 300 次复核见 R08 的修复记录，本轮没有把它们重写成新运行结果。
- 取消不能撤销已经发送的网络字节、已经写入的前缀或在途数据库提交。各模块的原有部分效果与关闭契约仍生效。新运行器清理自己的子进程树与临时目录，不回滚被测程序写入的外部文件或远端系统。
- 秘密缓冲保持不可装箱/默认打印；日志统一在 JSON 序列化前递归遮蔽敏感键。任意秘密若被调用方主动写入普通消息字符串，系统不能自动识别。性能报告不记录内存内容；其分配/堆范围、开销和采样精度按 `profile.md`。

上述平台和外部状态边界是明确的支持范围，不登记为已经验证的行为。十类 Windows x64 契约的完成依据是公开实现、已有专项证据与本轮补充共同覆盖，而不是把任意一个 smoke 等同于终态验收。
