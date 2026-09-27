# 静态描述、格式化及库内热点优化验收

日期：2026-09-27。平台：Windows x64。仓库基线提交：`fc03e008bda755181ded064561e032de271dfc0f`。

本轮接续[调用属性与安全借用](call_borrowing_2026-09-27.md)的未提交改动，交付稳定向量头、静态 serde 描述、基础类型字面量 format、已知编码名选路，以及均值、UTF-8、文件读取和 regex 工作区优化。编译器、标准库和三个预编译 TX 桥接对象均已更新到 `tx/`。下面区分本轮完成范围与尚待推进的项目，不把局部加速视为整份优化清单完成。

## 实现与语义

- 向量局部变量和参数保存稳定的存储头地址，重绑定时更新缓存。扩容、缩短及共享别名修改仍从当前头读取数据和长度。对整个循环体仅有无用户调用、无容器修改、无对象析构的标量操作的 `for item in vector<int/float/bool>`，提前取得数据和长度，按已证明的范围直接读取；其他循环保留边界检查。字段接收者没有改为无条件缓存。
- serde 的结构体、字段、嵌套类型、版本、未知字段策略与标量默认值由 LLVM 全局常量描述，同一模块内按类型复用。运行时 schema JSON 解析已删除，不是解析缓存。默认值保留整数值、浮点位模式和带长度文本；字段名静态存储的生命周期覆盖反序列化对象及其深复制。JSON/CBOR 的实际数据解析与中间动态树尚未消除。
- 首个位置实参为模板字面量、其他实参为可静态绑定的 `int/float/bool/str` 时，format 在编译期绑定字段并解析格式说明，直接调用类型专用写入函数，无动态 args/kwargs、参数查找或标量装箱。仍先按源码顺序求值全部实参；动态模板、展开、复合值与非法模板保留通用入口。类型不符仍在原有运行时错误边界报告。静态和动态格式说明使用同一个解析器。
- 已知编码名字面量由编译器和运行时共用的解析器选为枚举；不存在编码名字串构造和运行时名称解析。动态与非法名字保留原入口。UTF-8 路径直接校验合法 Unicode 标量，编码直接生成字节结果；保留 BOM、长度、非法序列和不可表示字符的错误契约。
- 一次性均值采用 `long double` Neumaier 补偿累计，避免方差状态和逐项长双精度除法。编译期证明指数范围能容纳最多 `int64` 个有限 `double` 的和；范围不足的平台使用加权均值后备分支。本机验证扩展精度路径，保留空输入、非有限值、计数和结果范围检查。已有多指标累计器继续保留 Welford 状态。
- 文件读取使用 64 KiB 分块并按实际数据增长；整流读取复用缓冲，不为短文件和 EOF 分配、清零 8 MiB 临时串。公开单次上限、读写方向及 EOF 语义不变。
- PCRE2 每线程复用一份匹配数据和上下文，容量按捕获组需要增长；重入使用临时工作区。每次设置限额和取消指针，退出时清除指针；结果文本和捕获组继续独立拥有。
- C ABI 对 serde 和 format 描述布局进行静态断言；兼容指纹包括新增共享格式和编码枚举定义。

## 定向验证

`python -X utf8 scripts/check_static_runtime.py` 提供本轮全部定向验证。实际执行时先完成 9 个 TX 行为程序与 IR 检查，随后只对新增编码选路和均值修改补跑相应程序，并运行 2 个原生程序，没有运行全量套件。

通过的 TX 程序：

- `tests/containers/static_runtime.tx`：向量视图缓存、重绑定、共享扩容和修改、空向量、越界捕获；静态与动态 format 对照、命名回退、Unicode、进制、精度、转换、实参顺序及错误；serde 默认值、位于第零槽的未知字段、深复制；极端有限均值和大数正负抵消；静态编码名、BOM、无效 UTF-8 和未知编码错误。
- `tests/containers/call_borrowing.tx`：前一批的别名、回调、析构与借用行为。
- `tests/serde/behavior.tx`、`tests/serde/module.tx`、`tests/formats/serde_migration.tx`：嵌套结构体/向量/option、JSON/CBOR 往返、版本、缺失、类型、重复及未知字段、跨模块和显式迁移。
- `tests/statistics/basic.tx`：现有统计行为与迭代器、错误契约。
- `tests/bytes_file_stream/behavior.tx`、`tests/bytes_file_stream/encodings.tx`：字节/文件流边界及七种编码往返。
- `tests/strings/regex_behavior.tx`：匹配、捕获、Unicode 与限额。

原生程序 `tests/strings/runtime_workspace.cpp` 验证 UTF-8 非法序列和 NUL、跨读取分块、重复 EOF、零长度读取的模式错误、同模式并发匹配、切换捕获组容量、未参与捕获和取消后恢复；`tests/strings/regex_cancel.cpp` 验证执行中取消。

生成 IR 已确认：

- `vector_sum/vector_reads/changing_vector` 各只有一次视图恢复；稳定遍历不含逐项越界错误调用，可修改遍历保留检查。
- `direct_format` 使用整数和文本专用入口，不调用动态装箱、args/kwargs 构造或通用 format。
- `constant_encoding` 使用枚举入口，不构造编码名字串。
- serde 实参指向只读全局字段描述；运行时不存在 `serde_parse_schema` 实现。

最后一次增量构建通过，未出现新增编译警告。`git diff --check` 通过。

## 同源码前后对照

源码：[static_runtime.tx](static_runtime.tx)。本轮首次重建前，用前一批成品编译 `static_runtime_before.exe`；修改完成后编译同一源码为 `static_runtime_after.exe`。完成构建和验证后交替运行三轮，使用 `time.monotonic_micros()`，下表取中位数并换算成毫秒。

- foreach 遍历 10 万元素的向量 50 次；serde 对含整数和文本字段的结构体做 1 万次 JSON 往返。
- format 做 10 万次 `"{}:{}"` 格式化；均值对 10 万项向量计算 30 次；UTF-8 对短 Unicode 文本编码 10 万次。
- 校验值依次为 `15000000 / 70000 / 988895 / 90 / 1800000`，六次执行一致。

| 固定负载 | 修改前（ms） | 修改后（ms） | 前/后耗时比 |
|---|---:|---:|---:|
| vector<int> foreach | 12.840 | 1.262 | 10.17 |
| serde JSON 往返 | 249.361 | 36.584 | 6.82 |
| 字面量 format | 113.746 | 20.437 | 5.57 |
| statistics.mean | 22.730 | 18.256 | 1.25 |
| UTF-8 编码 | 61.107 | 30.640 | 1.99 |

原始样本，单位微秒：

| 版本/轮次 | vector_foreach | serde_json | format_text | statistics_mean | encoding_utf8 |
|---|---:|---:|---:|---:|---:|
| 修改前 1 | 12704 | 249361 | 113746 | 22257 | 60894 |
| 修改后 1 | 1244 | 36523 | 20437 | 18223 | 30640 |
| 修改前 2 | 13009 | 245446 | 115173 | 22730 | 65389 |
| 修改后 2 | 1262 | 36584 | 20431 | 18256 | 30457 |
| 修改前 3 | 12840 | 266626 | 113735 | 23313 | 61107 |
| 修改后 3 | 1284 | 36753 | 20437 | 18744 | 30806 |

这些结果只代表上述固定负载，包括相应编译路径和运行时实现的共同变化。未推算相对其他语言的倍数，也未把稳定 foreach 的收益推广到有回调、修改或重绑定的所有索引循环。文件流和正则完成了实现及行为验证，本轮没有单独测量其性能。

## 成品与临时目录

构建使用 `scripts/build.ps1 -Incremental`，同时重建 `httpx_bridge/websocket_bridge/requests_bridge` 并刷新 `tx/package.compat`。清理 `build/` 的命令已包含绝对路径、仓库边界和重解析点校验，但自动审批策略返回 `blocked by policy`，未提供进一步原因；临时目录保留。验收程序和 IR 保存在 `tx_build/static_runtime_checks/`。

ABI 指纹：`26b332e45dafd8a24c1c73d6f51784d54059d02fa0efcf0ed2421d6f69076226`。清单中的编译器与标准库哈希已同实际文件核对。

| 文件 | SHA-256 |
|---|---|
| benchmarks/static_runtime.tx | `c89a9f68f889735c0663a1c65bbb8dfd206fba254de234e1845315ff4047e016` |
| tx/txc.exe | `895d469f04aa55018cbcff288d014625410b4e1241088d793c80893ee7998939` |
| tx/libtxstdlib.a | `1a475511779d3f7e273a73df79b873ef4153dea9a81bbe17dbeb1ad5a98d2d9d` |
| tx_build/static_runtime_before.exe | `54f3652d1beac152436224cabd99c5bd1de897dcffa84afe2b1c66a59dc85213` |
| tx_build/static_runtime_after.exe | `a352656c152bdb4c76f44bd5f5f81d20ef1c2ad75cc356b63d1c0ac254e70ce3` |

## 仍待完成

1. map/set/queue 等容器本体的类型化表示、普通索引循环的更多别名/范围证明，以及旧指令字符串错误分类收敛。
2. serde 直接字段编解码与中间动态树移除；复合格式参数、静态展开和完整常量求值。
3. decimal 的数值系数与 scale 表示、常量舍入模式；按需构造 regex 结果字段；网络会话生命周期。
4. 已知闭包与 bind 的专用环境、跨库内联及剩余 bytes/parse/string/random 热点。

上述事项继续按[运行时优化文档](../docs/runtime_optimization.md)推进，本轮没有宣称已经完成。
