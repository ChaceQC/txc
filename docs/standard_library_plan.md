# 标准库扩展规划

本文基于 2026-09-25 的标准库接口、实现和语言文档，记录当前能力、缺口及建议优先级。现有接口的正式说明见[标准库说明](standard_library.md)。

本文中的新增模块、接口名称、结果类型和分期均为规划建议，尚未代表已经实现或确定的语言契约。涉及类型、语法或运行时规则时，应先明确规则并同步语言文档和示例，再修改编译器。

2026-09-26 更新：现有容器和文件接口、类型化 map/set、堆和队列、algorithm、system/env、parse 与 JSON 已进入实现交付；统一错误结果和异常处理语法的契约见[解析与可恢复错误](errors_and_parse.md)，JSON 契约见[JSON 模块](json.md)。下面的初始缺口分析保留作为规划依据，实施状态以第四、五、八、九、十、十一节记录为准。

当前基础标准库已经成形。容器算法、系统交互、文件操作、可恢复错误、JSON、字节值、文件流和网络文本、二进制及端到端流式接口已有代码实现；网络协议边界与验证状态见[网络模块说明](network.md)。

## 一、当前能力

初始评估时公开了以下 11 个标准库模块；后续新增的模块以 `tx/stdlib/*.txh` 为准。接口位于 `tx/stdlib/`，实现源码位于 `src/stdlib/`，标准库以静态库提供实现。

| 现有模块 | 已覆盖能力 |
| --- | --- |
| `io`、`format` | 终端输出、标准错误、刷新；位置参数、命名参数、宽度和精度格式化 |
| `string` | 查找、切片、替换、拆分、拼接、去空白、ASCII 大小写转换 |
| `math` | 绝对值、最值、区间限制、余数、平方根、幂、向上和向下取整 |
| `array`、`dictionary` | 数组拼接、切片、反转；字典查询、删除、键和值快照、清空 |
| `file`、`fs`、`path` | 多字符集文本文件读写、目录创建和列举、基础路径处理 |
| `time`、`random` | 时间戳、单调计时、休眠、可设种子的伪随机数 |

此外，语言内置的类型化 `vector` 已经支持追加、删除、插入、调整长度、预留容量和共享容器操作。当前支持 `vector<int>`、`vector<float>`、`vector<bool>`、`vector<str>`，这些能力不应重复列为尚未实现的基础容器功能。

`vector<T>` 是内置参数化类型，尚未提供用户泛型、迭代器或复合元素向量。具体规则见[类型化 vector](typed_vectors.md)；类型化哈希容器的后续实施见第八节。

## 二、建议优先级

优先级依据当前仓库中的算法示例、终端程序和文件处理场景排列。表中的模块名及函数名为建议名称。

| 优先级 | 建议补充 | 主要能力 | 价值 |
| --- | --- | --- | --- |
| 第一批 | `algorithm` | `sort`、`sorted`、`find`、`count`、`lower_bound`、`upper_bound`、`reverse`，数值向量的 `sum`、`min_element`、`max_element` | 容器已有存储和修改能力，排序、查找、统计仍需用户反复实现 |
| 第一批 | `system` / `env` | 程序参数、环境变量、当前工作目录、可执行文件路径、临时目录 | 支持接收输入文件、输出目录和运行选项，便于编写命令行工具 |
| 第一批 | `parse` 与统一错误结果 | `try_parse_int`、`try_parse_float`、指定进制解析，以及供文件等库使用的错误结果约定 | 支持输入错误后重试、单个文件失败后继续处理等行为 |
| 第一批 | 扩充 `fs`、`path` | 文件复制、重命名、删除、文件大小、修改时间、递归遍历；路径规范化、绝对路径、相对路径、替换扩展名 | 补全文件整理、批处理和缓存维护所需的操作 |
| 第二批 | `json` | 解析、序列化、缩进输出、带位置的解析错误、明确的字段读取接口 | 支持配置文件、结构化数据交换和结果保存 |
| 第二批 | 类型化 `map`、`set` | `map<str, int>` 等常见映射、集合成员判断、插入和删除 | 减少现有 `dict` 的 `any` 转换，让更多类型错误在编译期暴露 |
| 第二批 | `bytes`、文件流与 `encoding` | 二进制读写、分块读取、逐行读取、定位、内存中的编码转换、十六进制与 Base64 | 支持大文件、二进制格式和文本以外的数据处理 |
| 第二批 | `process` | 启动子进程、传递参数、等待退出、取得退出码、捕获标准输出和标准错误 | 支持构建脚本、命令包装工具和系统工具调用 |
| 第二批 | `test`、`log` | 断言、相等断言、测试失败汇总；日志级别、时间和输出目标 | 方便 TX 用户验证自己编写的模块并定位问题 |
| 按场景补充 | 数学、日期、文本扩展 | 三角函数、对数、指数、舍入、`gcd/lcm`；日期解析和格式化；正则表达式、Unicode 规范化 | 按实际程序需求逐项扩充 |

## 三、先确定可恢复的错误处理

### 初始限制与本轮实施

2026-09-26：按[解析与可恢复错误契约](errors_and_parse.md)实施 parse、统一具体结果、文件 try 接口以及 `try { } exception type as e { }`。本轮代码和经用户授权完成的少量验证见第十节。以下保留初始分析。

初始评估时，字符串到数字的转换、文件 I/O 等操作失败后会进入运行时错误路径，输出错误、清理对象并调用 `std::exit(1)`，实现见 [runtime_abi.cpp](../src/backend/cpp/runtime_abi.cpp)。当时尚无供 TX 程序使用的异常捕获语法；本轮接入的行为见第十节。

因此，初始版本难以处理以下常见场景：

- 用户输入了无效数字，提示后重新输入。
- 批量处理文件时记录某个文件的错误，然后继续处理其余文件。
- 配置文件不存在或 JSON 格式错误时，采用默认配置或显示更具体的提示。

### 建议方向

为可预期的外部失败提供程序可以检查的返回结果，并统一解析、文件、进程等模块的错误表达方式。

短期可采用带 `ok`、`value`、`error` 字段的具体结果结构体，例如整数解析结果、文本读取结果。结果类型需要接入标准库 ABI；通用的 `result<T>` 可在类型系统支持后再统一，不将用户泛型作为第一批库扩展的前提。

需要区分以下情况：

- 成功返回了一个合法的空值，例如 JSON 的 `null`。
- 查询目标不存在，例如某个环境变量没有设置。
- 操作执行失败，例如权限不足或文本编码无效。

这些情况不应全部用同一个 `none` 表示。已有接口的失败语义应在新增可恢复接口时保持兼容，任何行为调整需要单独明确并同步文档。

## 四、围绕类型化 vector 扩展容器算法

### 2026-09-26 实施记录

- 按[容器算法契约](algorithm.md)和[示例](../examples/algorithm.tx)实现 `algorithm.txh`：int/float/str 的 sort、sorted、find、count、lower_bound、upper_bound、reverse；bool 的 find、count、reverse；int/float 的 sum、min_element、max_element。
- 复用已有 vector 存储与字符串引用。sort/reverse 原地修改，sorted 返回独立容器；其他类型化容器通过已有 keys/values/to_vector 快照组合使用。
- 明确空容器、未找到、二分有序前提、UTF-8 字节序、NaN 排序及整数溢出规则。最值返回元素值；数值统计要求有限 float，错误沿用当前运行时。
- 具体重载在编译期选择，通过类型化 C ABI 接入标准库静态库。`scripts/build.ps1` 构建通过，已更新 `tx/txc.exe` 和 `tx/libtxstdlib.a`，成功后已清理 `build/`；初次代码交付时按用户要求暂不测试。
- 同日获用户授权后，8 个定向场景全部通过：3 个正常运行场景、4 个运行错误与析构清理场景、1 个静态诊断，合计调用全部 30 个公开重载。根目录 `example.tx` 已新增 `algorithm_demo()`，编译运行及新增输出、自建目录清理均已核对。可用 `python -X utf8 scripts/check_algorithm.py` 复现，具体覆盖见[容器算法验证记录](algorithm.md#实现状态)。未运行全量回归或性能测试。以下保留初始规划依据。

### 第一阶段接口范围

优先为 `vector<int>`、`vector<float>`、`vector<str>` 提供明确的算法重载，按静态类型编译到对应的 C++ 实现。布尔向量按具体操作需要扩展。

排序、查找和统计可以先覆盖以下能力：

- 排序：原地排序 `sort`、返回新容器的 `sorted`。
- 查找：线性查找、计数，以及有序向量上的 `lower_bound`、`upper_bound`。
- 顺序操作：反转等常见变换。
- 数值统计：求和、最小元素、最大元素。

### 需要明确的行为

- 建议 `sort` 修改共享容器，其他别名能够观察到修改；`sorted` 返回新容器。
- 字符串排序第一版可明确采用 UTF-8 字节序。
- 算法接口应明确空容器、未找到元素及数值溢出的处理方式，并与统一错误约定衔接。

自定义比较器以及 `map/filter/reduce` 等接口，可以在函数值或其他明确的回调机制确定后再扩展。现阶段不通过运行时名称分派和逐元素动态装箱来模拟尚未确定的泛型算法系统。

## 五、补齐现有容器和文件接口

### 2026-09-26 实施记录

本轮已补充代码和接口：

- `array.push_back/pop_back/insert/erase/clear`，支持共享容器原地修改；删除元素时先完成容器修改，再释放元素，避免析构回调访问修改中的容器。
- `dictionary.items` 成对快照，以及 `get/contains/remove` 的 none 键重载。
- `fs.list_directory_vector/walk_directory/copy_file/rename/remove/remove_all/file_size/modified_millis`。
- `path.normalize/is_absolute/absolute/relative/replace_extension`。
- none 类型标注、上述标准库函数的静态调用检查、直接 C ABI 和 LLVM 声明，以及容器、文件系统两个示例。

此处记录最初规划时的顺序；类型化 `map/set`、程序参数与环境变量、可恢复错误结果、字节值和文件流现已实现。当前状态以各模块说明和本文件末尾的网络模块章节为准。

交付状态：`scripts/build.ps1` 构建通过，已更新 `tx/txc.exe` 和 `tx/libtxstdlib.a`，成功后已清理 `build/`。

2026-09-26 经用户授权执行少量测试：7 个定向场景全部通过，可用 `python -X utf8 scripts/check_stdlib_interfaces.py` 复现，覆盖两个新增示例、共享修改及快照、none 键、实参求值顺序、命名和展开调用、析构回调、空数组删除、删除位置越界，以及复制和重命名遇到已有目标时保留原文件。主示例 `example.tx` 已接入本轮新增接口和 none 类型标注，另外单独编译、执行一次通过，退出码为 0，新增接口的关键输出和自建目录清理均已核对。合计 8 个运行场景通过，未运行全量回归。

### 异构 array 的原地增删

当前 [array.txh](../tx/stdlib/array.txh) 只有 `concat`、`slice`、`reverse`，三者都返回新数组。

现有文档中的追加写法 `values = concat(values, [value])` 会复制原数组，见 [array.cpp](../src/stdlib/array.cpp)。从空数组开始连续追加 n 个元素，会产生 O(n²) 的累计元素复制成本。

基础类型列表已经可以使用类型化 `vector`。对于混合值列表，建议为 `array` 补充 `push_back`、`pop_back`、`clear` 等原地操作，并明确它们对共享引用及遍历行为的影响。

### dictionary 的接口完整性

建议补充以下能力：

- 成对的条目快照 `items`。当前独立取得的 `keys` 和 `values` 不保证可以按下标配对，不能要求调用方自行假设排列一致。
- 补齐 `none` 键的操作范围。语言允许 `none` 作为字典键，但当前 [dictionary.txh](../tx/stdlib/dictionary.txh) 的 `get`、`contains`、`remove` 仅提供 `int`、`float`、`bool`、`str` 重载。

类型化 `map`、`set` 是另一项需要编译器配合的扩展。它们需要接入类型检查、代码生成、运行时表示及 `deep_copy` 等机制，不能仅增加 `.txh` 声明。

### 文件系统和系统信息

当前 `fs`、`path` 已覆盖基础查询、目录创建、直接子项列举和路径文本拆分，下一步补全复制、移动、删除、元信息、递归遍历和路径转换。

`fs.list_directory` 可增加返回 `vector<str>` 的接口，与现有 `string.split_vector` 的类型化路径保持一致。

目前程序入口要求 `def main() -> int`，见[语法说明](syntax.md)。程序参数、环境变量和进程信息需要由运行时提供公开入口；设计这些标准库接口时应明确其与入口初始化的衔接方式。

## 六、文本、编码和二进制数据

### 复用已有文本能力

当前字符串已经支持 UTF-8 码点计数和按码点位置切片；`trim`、`lower`、`upper` 的处理范围是 ASCII，见[标准库说明](standard_library.md)。

后续可按需求补充：

- 字符拆分和码点转换。
- 反向查找和按行拆分。
- Unicode 大小写转换和规范化。

扩展时应继续明确索引单位。码点与用户看到的完整字符可能不同，不能将现有码点计数直接描述为字素簇计数。

### 复用已有编码实现

文件模块已经实现 UTF-8、UTF-16、GBK、GB18030 等字符集转换。内部接口见 [encoding.hpp](../src/stdlib/encoding.hpp)。

`encoding` 模块已复用这些实现，公开内存中的文本与字节转换能力；少量定向场景已通过。

### 明确字节容器和流接口

原有 [file.txh](../tx/stdlib/file.txh) 继续公开整文件文本接口。现已新增内置 `bytes`、`vector<bytes>` 和 [file_stream.txh](../tx/stdlib/file_stream.txh)；`array`、`dict` 也可保存 `bytes` 值。

字节值、内存编码及二进制/文本流的具体接口、EOF、定位和资源规则见[字节值与文件流](bytes_file_stream.md)。代码及接口已接入并完成构建，少量定向场景已通过；现有文本文件接口继续承担便捷的整文件操作。

## 七、建议实施顺序

以下为初始建议安排；已实施部分见第四、五、八、九、十、十一节，其余项目继续按后续任务安排。

1. 确定可恢复错误的返回约定，先用于数字解析等边界清晰的接口。
2. 增加 `algorithm`，优先服务现有类型化 `vector`。
3. 增加 `system/env`，补齐程序参数、环境变量和常用系统路径。
4. 扩充 `fs/path`，并补齐现有容器接口的小缺口。
5. 增加 JSON、类型化 `map/set`、字节容器与文件流。
6. 增加子进程、测试和日志能力，按实际需求补充数学、日期和文本模块。

如果近期以算法程序为主，可以提前类型化 `map/set`、堆和队列；如果主要编写实用工具，则优先 JSON 和子进程。

每一项落地前应明确接口、类型、共享或复制语义及失败行为；涉及语言能力变化时先同步语言文档和示例。实现沿用 `.txh` 公开接口、C++23 静态库实现和明确的 C ABI 分层，验证只覆盖本次新增行为及必要的兼容边界。

## 八、类型化 map/set、堆和队列实施

2026-09-26：先确定[类型和接口规则](typed_containers.md)并编写[示例](../examples/typed_containers.tx)，再接入编译器和运行时。

- 支持 `map<K, V>`、`set<T>`、`heap<T>`、`queue<T>`；K、V、T 均限 int/float/bool/str，map 覆盖 16 种键值组合。
- map 提供索引读写、查询、删除、键值快照；set 提供插入、成员查询、删除；heap 提供小顶/大顶堆；queue 提供 FIFO 操作。共同支持长度、判空、清空、类型化快照遍历。
- 接入普通模块 `.txh` 签名、struct/class 字段、完整 any 类型检查、打印、共享引用与 deep_copy。与 vector 一样使用内置参数化类型入口，不添加无法表达用户泛型的伪 `.txh` 声明。
- C++23 原生标量存储和类型化 C ABI 随标准库静态库交付。静态调用在编译期确定，运行时保留缺键、空容器、NaN 等实际值检查。
- `scripts/build.ps1` 构建通过，已更新 `tx/txc.exe` 和 `tx/libtxstdlib.a`，成功后已清理 `build/`。代码交付阶段按用户要求暂未执行测试。
- 同日用户授权少量测试后，13 个定向场景全部通过：3 个正常运行场景、7 个运行错误与析构清理场景、3 个编译诊断。根目录 `example.tx` 已补充 `typed_container_demo`，编译、执行一次通过，新增输出及自建目录清理均已核对。可用 `python -X utf8 scripts/check_typed_containers.py` 复现；具体覆盖范围见[类型化容器验证记录](typed_containers.md#实现与验证)。未运行全量回归或性能基准。

## 九、system/env 实施

2026-09-26：按[系统与环境变量契约](system_env.md)增加 `system.txh`、`env.txh` 和[示例](../examples/system_env.tx)，完成代码后执行少量定向验证。

- `system.args()` 返回不含程序名的独立 `vector<str>` 快照；生成入口在 TX main 前初始化宽字符命令行并转换 UTF-8，语言入口仍为 `def main() -> int`。
- `system.current_directory/set_current_directory/executable_path/temp_directory/home_directory` 补齐进程工作目录及常用系统路径，路径统一使用 UTF-8 和正斜杠。
- `env.contains/get/set/remove` 提供进程环境变量查询与修改；严格 get 和带默认值的 get 分别处理缺失情况，空字符串保留为已设置的合法值。非法名称、NUL、编码和系统错误沿用运行错误语义。
- 接入 C++23 标准库、直接 C ABI、LLVM 声明与调用及构建接口检查；具体重载在编译期选定，运行时只处理实际系统状态。
- `scripts/build.ps1` 构建通过，已更新 `tx/txc.exe`、`tx/libtxstdlib.a`，确认产物后清理 `build/`。
- `python -X utf8 scripts/check_system_env.py` 的 7 个定向场景全部通过：2 个正常场景、4 个运行错误、1 个静态诊断，覆盖全部 11 个公开签名。未运行全量回归或性能测试。
- 同日修复编译驱动内部启动工具时的参数引号处理，保留空格路径边界，并处理引号前和末尾的反斜杠。重新构建后，上述 7 个场景通过；脚本直接编译到中文带空格的输出目录和文件名，同时验证中文带空格的编译临时目录。另外以双引号包裹路径执行一次 PowerShell 编译和运行，退出码为 0，参数和实际可执行路径正确。

## 十、parse、统一错误结果与异常处理实施

2026-09-26：先保存已有工作的 Git 基线，再按[解析与可恢复错误契约](errors_and_parse.md)实现，示例见 [parse_errors.tx](../examples/parse_errors.tx) 及主示例的 `parse_error_demo()`。

- 新增 `error.txh`：统一 kind/code/message 信息以及 int/float/str/bool 的具体 ok/value/error 结果。
- 新增 `parse.txh`：整数默认十进制与 2～36 进制重载、有限浮点解析、返回结果的 try 接口及失败时可捕获的严格接口；完整消费文本并区分空文本、语法、范围和进制错误。
- 新增 `try { } exception type as e { }`：前端检查错误类型、独立作用域、重复和不可达分支及函数返回路径；LLVM 静态选择类别匹配，支持嵌套以及函数、方法间传播。
- 在原 C ABI 上增加错误类别和代码；状态码路径、数组/字典/向量等快速路径及随机数错误使用统一传播机制。生成代码释放退出作用域的拥有句柄，保留借用引用和外层修改；处理分支错误交给外层，未处理错误继续保留原非零退出语义。
- `file.try_read_text/try_write_text/try_append_text` 使用统一结果；原 file 严格接口的错误归入 io_error。成功的空文本与读取失败明确区分。
- 本轮先完成代码及构建交付；同日用户授权少量测试后，`python -X utf8 scripts/check_parse_errors.py` 的 6 个定向场景全部通过：3 个正常运行、1 个未捕获错误运行、2 个静态诊断，覆盖解析边界、统一文件结果、跨模块与嵌套传播、快速错误路径和析构清理。主示例及独立示例均已编译运行，关键输出和文本文件清理已核对。构建产物已更新，`build/` 已清理；未运行全量回归或性能基准。详细范围见[验证记录](errors_and_parse.md#实现与验证)。

## 十一、JSON 模块实施

2026-09-26：按[JSON 模块契约](json.md)增加 `json.txh` 和[独立示例](../examples/json.tx)，复用现有 `any`、`array`、`dict` 及 `error.any_result`。

- 解析完整 JSON，支持标准字符串转义、严格 UTF-8、有限数值和最多 128 层嵌套；`try_parse` 区分合法 `null` 与失败，错误携带行、列及字节偏移。
- 支持紧凑及缩进序列化，对象字段按 UTF-8 字节序输出；拒绝非字符串键、非有限数值、不支持的类型和循环引用。
- `parse_object` 使对象根值直接成为 `dict`，可与文件模块组合后按字典规则读写；`any` 中的嵌套对象或数组可直接赋给明确类型的 `dict` / `array`，运行时核对类型并保留共享引用。`contains`、`get` 及静态返回类型的字段读取函数区分缺失、`null` 和类型错误。调用目标由语义分析确定，经直接 C ABI 接入标准库。
- `pwsh -NoProfile -File scripts/build.ps1` 构建通过，已更新 `tx/txc.exe` 和 `tx/libtxstdlib.a`，成功后清理 `build/`。经用户允许运行两个定向场景：文件配置示例完成读、改、写；边界用例输出 `JSON_OK`。具体覆盖见 [JSON 验证记录](json.md#实现与验证)，未运行全量回归或性能测试。

## 十二、网络模块设计

HTTP/1.1、HTTP/2 客户端与服务端以及 WebSocket 客户端与服务端已按[网络模块说明](network.md)接入文本、二进制和文件流接口。公开模块名为 `httpx`、`websocket`；HTTP/2 包含 h2c prior knowledge 和 PEM/TLS 服务端。并发流、h2c Upgrade、非法帧专项验证和性能基准仍在后续范围。
