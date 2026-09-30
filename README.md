# TX 编译器

使用 C++23 实现的静态强类型 .tx 语言编译器。当前版本解析、检查并编译仓库中的 example.tx，生成原生可执行文件。

## 快速使用

Windows x64 用户可从 [GitHub Releases](https://github.com/ChaceQC/txc/releases) 下载
`txc-vX.Y.Z-windows-x64.zip`，解压后直接使用其中完整的 `tx/` 目录，无需安装构建工具。
ZIP 同时包含本文、[说明文档](docs/usage.md)、[语法文档](docs/syntax.md)、标准库文档及示例。
自动构建与 tag 发布流程见 [GitHub CI/CD](docs/releasing.md)。

运行 `scripts/build.ps1` 构建 txc 后，在仓库根目录执行：

```powershell
.\tx\txc.exe check .\example.tx
.\tx\txc.exe .\example.tx
.\tx_build\example.exe
```

check 会检查语法和静态类型；`example.tx` 是当前语言的完整可运行语法展示。完整构建步骤和命令参数见 [编译与运行](docs/usage.md)，当前语言支持的语法和类型规则见 [语法说明](docs/syntax.md)。

用户测试可运行 `tx/txc.exe test examples/toolchain_test.tx --format json`；性能分析可运行 `tx/txc.exe profile examples/profile_workload.tx --warmup 1 --samples 3`。参数化/性质测试、并行/超时、日志轮转见[工具链说明](docs/toolchain_completion.md)，CPU/内存统计范围与基准开销见[性能分析](docs/profile.md)。构建包同时交付 `tx/examples/` 和 `tx/docs/`。

主示例也展示容器原地操作、字典条目快照和 none 键、类型化 map/set、大小顶堆、FIFO 队列、algorithm 排序查找与数值统计，以及文件系统与路径接口；运行时在 `tx_build/` 下创建独立示例目录，完成文件操作后自行清理。

Unicode 规范化、字素和显式 locale 比较见 [Unicode 示例](examples/unicode.tx)；PCRE2 捕获与替换见 [正则示例](examples/regex.tx)。

最终编译器产物 `txc.exe`、`clang.exe`、`libtxstdlib.a`、链接组件和运行时 DLL 位于 `tx/`；构建成功后清理 `build/` 中间文件。txc 默认将 .tx 程序放在 `tx_build/`。

开发时可用 `scripts/build.ps1 -Incremental` 保留 `build/` 并启用并行编译与持久编译缓存；默认构建仍会清理中间目录。见[构建说明](docs/usage.md#构建编译器)。

txc 的默认编译路径使用 LLVM 目标文件生成器和随 `tx/` 分发的链接组件；运行 txc 编译 `.tx` 时不需要安装 g++。构建与发行结构见 [编译与运行](docs/usage.md)和[原生后端说明](docs/native_backend.md)。

字符串、浮点数、混合类型数组和结构体的用法见 [数据类型示例](examples/data_types.tx)。
复合值的默认共享、显式 `deep_copy` 和循环关系见 [内存管理示例](examples/memory_management.tx)；运行时错误时的析构见 [错误清理示例](examples/memory_error_cleanup.tx)，后者预期以非零状态退出。
循环引用的自动回收规则见 [垃圾回收说明](docs/garbage_collection.md)。
类、数组和字典循环的运行示例见 [循环回收示例](examples/cycle_collection.tx)。
类的封装、多继承、接口、抽象方法、方法重载、析构与运行时转换见 [class 说明](docs/classes.md)和 [综合示例](examples/advanced_classes.tx)；跨模块接口与实现见 [模块示例](examples/advanced_class_module/main.tx)。
`struct` 与 `class` 在 `.txh` 和配对 `.tx` 中的不同写法见 [模块文件分工](docs/modules.md)，对应的可运行示例为 [struct 模块](examples/struct_module/main.tx)和 [class 模块](examples/class_module/main.tx)。
`+=`、`-=`、`++`、`--` 的用法见 [更新运算符示例](examples/update_operators.tx)。
数组解包、字典、命名实参与 `*args`、`**kwargs` 见 [可变参数示例](examples/variadic_unpack.tx)。
类型化 `map/set`、小顶堆和大顶堆、FIFO 队列见[类型化容器说明](docs/typed_containers.md)及[示例](examples/typed_containers.tx)。
基于现有类型化 vector 的排序、查找、二分、反转和数值统计见[algorithm 说明](docs/algorithm.md)及[示例](examples/algorithm.tx)；其他类型化容器可通过已有快照接口组合使用。

模块导入、终端输入输出、可指定字符集的文本文件读写、字符串、数学、数组、文件系统、时间和随机数操作见 [模块说明](docs/modules.md)、[标准库说明](docs/standard_library.md)和[可运行示例](examples/import_io.tx)。
覆盖十类通用能力的现状缺口与完整终态契约见[标准库终态设计](docs/standard_library_complete_design.md)，细分工作项见[标准库终态实施顺序](docs/standard_library_plan.md)。

Python 风格的 HTTP 客户端见 [requests 模块](docs/requests.md)和[请求示例](examples/requests.tx)。
安全随机数、摘要、密钥派生与 AES-256-GCM 认证加密见[密码学标准库](docs/crypto.md)和[示例](examples/crypto.tx)。
终端与文件操作的单独示例见 [终端 I/O](examples/terminal_io.tx)、[文件 I/O](examples/file_io.tx)和[指定字符集读写](examples/file_encodings.tx)。
两个模块拥有同名函数时的调用见 [别名导入示例](examples/import_alias.tx)。
数学、数组、目录与路径标准库的组合用法见 [标准库示例](examples/stdlib_modules.tx)；运行时会在 tx_build/ 下创建示例目录。
数组原地增删、字典条目快照和 none 键见[容器接口示例](examples/container_interfaces.tx)；文件复制、移动、删除、递归列举和路径转换见[文件系统接口示例](examples/filesystem_interfaces.tx)。
程序参数、环境变量、工作目录和常用系统路径见[系统接口说明](docs/system_env.md)与[示例](examples/system_env.tx)。
`parse`、统一的 `ok/value/error` 结果，以及 `try { } exception type as e { }` 的规则见[解析与可恢复错误](docs/errors_and_parse.md)与[示例](examples/parse_errors.tx)。实现已接入，6 个定向场景通过，验证范围见该文档末尾。
JSON 解析、序列化和字段读取见 [JSON 模块说明](docs/json.md)与[示例](examples/json.tx)。
JSON 增量数组与 schema 校验见 [JSON 增量接口](docs/json.md#增量读写与-schema)与[示例](examples/json_stream.tx)；CSV 的流式读写、表头、dialect 与限额见 [CSV 模块](docs/csv.md)与[示例](examples/csv_stream.tx)。
XML 的拉取式读取、文档树、命名空间和流式写入见 [XML 模块](docs/xml.md)与[示例](examples/xml_stream.tx)。
可序列化结构体的编译期字段元数据、JSON/CBOR 映射和跨版本迁移见 [serde 模块](docs/serde.md)与[示例](examples/serde_schema.tx)。
毫秒计时和可设种子的随机数用法见 [时间与随机数示例](examples/time_random.tx)。
时区、独立随机生成器、扩展数学、统计及十进制金额计算见[第六部分接口契约](docs/time_math_statistics.md)、[统计示例](examples/statistics_summary.tx)和[金额示例](examples/decimal_money.tx)。
语言特性和运行时的 TX/C++ 性能对照见 [语言特性基准](benchmarks/language_features/README.md)；标准库与 C++ Release 的对照见 [标准库基准](benchmarks/library_compare/README.md)。
`import "io.txh"` 和 `import "xx/xx.txh"` 会先查找源码同目录的接口，再查找 `tx/stdlib/` 下的对应路径；见 [本地接口优先示例](examples/local_priority/main.tx)。
函数的直接递归、相互递归和无返回值递归见 [递归示例](examples/recursion.tx)。
同名函数按参数类型列表重载的用法见 [函数重载示例](examples/overload.tx)。
`int` 和 `float` 的范围见 [语法与类型规则](docs/syntax.md)，最小 `int` 的直接写法见 [数值边界示例](examples/numeric_bounds.tx)。

## 代码布局

- `src/common`：共用类型和错误位置。
- `src/frontend`：词法分析、语法树、语法分析和语义检查。
- `src/backend/llvm`：LLVM IR 代码生成。
- `src/backend/cpp`：生成程序使用的 C++23 运行时与 C ABI 接口。
- `src/stdlib`：标准库二进制实现；公开接口位于 `tx/stdlib/*.txh`。
- `src/driver`：`txc` 命令行入口。
