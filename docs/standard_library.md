# 标准库

本文说明当前已实现的标准库接口。后续能力缺口、建议优先级和实现前提见[标准库扩展规划](standard_library_plan.md)。

`test/log/debug` 与 `txc test` 已支持参数化/性质测试、失败种子和反例缩减、夹具/可控时钟、并行/超时/进程隔离及结构化轮转日志，见[工具链补齐](toolchain_completion.md)。`profile.txh` 与 `txc profile` 提供 Windows x64 CPU 采样、C++ 分配/堆快照、源码归因及基准对照，见[性能分析](profile.md)。全库公开模块、发行物和跨模块业务链的证据见[终态验收记录](standard_library_acceptance.md)。

`Send/Sync`、唯一移动、跨线程释放与循环回收的 2.6 验收已完成；第 10 节的线程、同步原语和通道支持 `Send` 值，接口见[线程](thread.md)、[同步原语](synchronization.md)和[有界通道](channel.md)，类型及所有权规则见[Send/Sync](send_sync.md)。

`task.txh` 提供结构化作用域、任务组、取消、截止时间和异步计时；`async def` 与 `await` 接入有界工作线程和 Windows IOCP 事件循环。静态类型与使用边界见[结构化任务与异步函数](task.md)，示例见[task_async.tx](../examples/task_async.tx)。`async_file.txh` 通过同一 IOCP 循环提供有界、可取消的文件读写，结果报告已完成的外部效果，契约见[异步文件操作](async_file.md)。

`ipc.txh` 使用本机命名管道或子进程管道传送有版本、会话号和长度上限的规范 CBOR 消息。EOF、半包、超时续读、重连与超限规则见[本机 IPC](ipc.md)。

`httpx` 与 `websocket` 提供 HTTP/1.1、HTTP/2、HTTP/3、WS/WSS 客户端和对应服务端的文本、二进制与文件流接口。HTTP/2 支持 HTTPS ALPN、明文 h2c prior knowledge，以及使用 PEM 证书的 TLS 服务端；WebSocket 支持 WSS 服务端和与明文 HTTP/1.1 共享监听器的 Upgrade。WebSocket 连接可唯一移入有界任务执行异步收发。二进制正文和消息直接使用 `bytes` 或 `binary_stream`。完整签名、边界及定向验证状态见[网络模块说明](network.md)。公开接口分别为 [httpx.txh](../tx/stdlib/httpx.txh) 和 [websocket.txh](../tx/stdlib/websocket.txh)。

`requests` 在 `httpx` 上提供接近 Python Requests 的同步客户端、命名参数、可复用 `session`、惰性 `response` 分块、代理、PEM CA、PKCS#12 客户端证书、有界重试、JSON、表单、Cookie 和重定向。公开接口为 [requests.txh](../tx/stdlib/requests.txh)，`.tx` 实现构建进同一个标准库静态库。具体签名、Python API 对照及限制见[requests 模块说明](requests.md)。

`bytes`、`encoding` 和 `file_stream` 的接口及语义见[字节值与文件流](bytes_file_stream.md)；代码已构建，少量定向场景已通过。

`unicode` 的规范化、完整大小写、字素和显式 locale 比较见[Unicode、正则与增量编码](unicode_regex_encoding.md)；`string.lower/upper` 仍只处理 ASCII。

`regex` 的 UTF/UCP 编译、捕获、匹配、替换、拆分及执行限额也见[Unicode、正则与增量编码](unicode_regex_encoding.md)。

导入 [bytes.txh](../tx/stdlib/bytes.txh) 可构造、拼接、切片及进行 Hex/Base64 转换；[encoding.txh](../tx/stdlib/encoding.txh) 提供内存中的文本编码与解码；[file_stream.txh](../tx/stdlib/file_stream.txh) 提供二进制和文本文件流。`bytes` 也可作为 `vector<bytes>` 元素以及 `array`、`dict` 的值。

导入 [crypto.txh](../tx/stdlib/crypto.txh) 使用安全随机数、SHA-256/SHA-512、HMAC-SHA256、HKDF-SHA256、PBKDF2-HMAC-SHA256 与 AES-256-GCM 认证加密。原有单段接口使用 `bytes`；新增流式摘要/HMAC 与认证文件接口使用流或 `secret_bytes` 密钥。格式、错误码和边界见[密码学标准库](crypto.md)。现有 `random` 模块不是密码学安全随机源。

导入 [secret.txh](../tx/stdlib/secret.txh) 可创建不透明的 `secret_bytes`、显式导入/导出并清零共享缓冲。接口、静态禁用规则和平台内存限制见[秘密字节](secret_bytes.md)。

导入 [password.txh](../tx/stdlib/password.txh) 使用 Argon2id 创建和验证 PHC 密码哈希，并检测何时需要升级参数。密码以 `secret_bytes` 传入，参数和资源限制见[Argon2id 密码存储](password.md)。

导入 [public_key.txh](../tx/stdlib/public_key.txh) 使用 Ed25519 签名/验签及 X25519 交换后 HKDF 派生。私钥保存在 `secret_bytes` 句柄中，公开编码与安全边界见[公钥密码学](public_key.md)。

导入 [x509.txh](../tx/stdlib/x509.txh) 可读取 PEM/DER/PKCS#12 证书，按主机名、用途、有效期及系统/自定义信任锚验证证书链；未执行撤销查询会明确返回 `not_checked`。接口与平台边界见[证书读取与验证](x509.md)。

导入 [tls.txh](../tx/stdlib/tls.txh) 可配置系统或自定义信任锚、导入 PKCS#12 身份，并对服务端或客户端证书进行强制验证。TLS 安全流和握手仍按 11.3 实现；配置、失败语义与资源边界见[TLS 身份与验证配置](tls.md)。

内置 `map<K, V>`、`set<T>`、`ordered_map<K, V>`、`ordered_set<T>`、`heap<T>`、`queue<T>`、`deque<T>` 与 `vector<T>` 一样无需导入，接口见[类型化容器](typed_containers.md)。映射的 `entries()` 返回成对的只读条目快照；有序容器支持显式比较器和范围查询。它们的 C++23 实现随标准库静态库交付，普通模块可在 `.txh` 中使用这些类型。

## 解析与可恢复错误

解析及可恢复错误使用 `parse.txh`、`error.txh`，完整规则见[解析与可恢复错误](errors_and_parse.md)。`try_parse_int` 支持十进制和显式 2～36 进制，`try_parse_float` 支持有限十进制浮点数；返回具体的 `ok/value/error` 结果。严格 `parse_int/parse_float` 可由 `try { } exception 类型 as e { }` 捕获。文件库新增 `try_read_text/try_write_text/try_append_text`，复用相同的错误字段和结果结构。

## JSON

导入 [json.txh](../tx/stdlib/json.txh) 可解析和序列化 JSON；配置文件可通过 `file.read_text`、`json.parse_object` 得到普通 `dict`，直接索引和修改字段，再用 `json.stringify_pretty` 与 `file.write_text` 保存。`try_parse` 返回可区分合法 `null` 与失败的 `error.any_result`，`parse` 在错误时抛出含行、列和字节偏移的 `parse_error`。`contains`、`get` 及类型化 `get_*` 也可用于明确的字段读取。完整流程、类型对应和错误码见 [JSON 模块说明](json.md)。

大文件可使用 `read/write` 或根数组的 `reader/next`、`writer/write_value/finish`，并通过 `limits` 限制总量、单值和深度；`validate` 显式执行文档列出的 schema 约束。用法见 [JSON 增量示例](../examples/json_stream.tx)。

## CSV

导入 [csv.txh](../tx/stdlib/csv.txh) 后，可逐行读取 `option<vector<str>>`、取得表头快照或写入类型明确的字符串行。dialect 显式控制分隔符、引号、换行、表头、BOM 和输入限额；小数据可使用 `parse/stringify`，大数据使用文件流和游标。契约见 [CSV 模块说明](csv.md)，用法见 [CSV 增量示例](../examples/csv_stream.tx)。

## XML

导入 [xml.txh](../tx/stdlib/xml.txh) 后，可通过 `reader/next` 逐事件读取、`parse/read` 构建文档树，或用 `writer` 逐步生成文档。元素与属性分别保存本地名、命名空间 URI 和前缀；默认拒绝 DTD 和自定义实体，也不处理 XInclude。二进制输入支持 UTF-8/UTF-16，`limits` 控制总量、深度、节点和属性数。完整规则见 [XML 模块说明](xml.md)，用法见 [XML 流式示例](../examples/xml_stream.tx)。

## CBOR

导入 [cbor.txh](../tx/stdlib/cbor.txh) 后，可对 `int/float/str/bytes/array/dict/bool/none` 使用规范化 `encode/decode`，或用二进制文件流的 `read/write` 处理完整值。大数组使用 `reader/next` 逐项读取；写入前须给 `writer` 提供元素数量，再调用 `write_value/finish`。重复键、非规范编码和无效 UTF-8 会拒绝；限额和类型边界见 [CBOR 模块说明](cbor.md)。

## serde

导入 [serde.txh](../tx/stdlib/serde.txh) 后，可为标记 `serde(version=...)` 的结构体生成固定字段映射，并使用 `serialize_json/deserialize_json<T>`、`serialize_cbor/deserialize_cbor<T>`。字段编号、默认值、未知字段策略和显式跨版本迁移见 [serde 模块说明](serde.md)与[示例](../examples/serde_schema.tx)。解码先校验 schema 版本及字段类型，资源句柄和动态 `any` 字段在编译期拒绝。

## 容器算法

导入 [algorithm.txh](../tx/stdlib/algorithm.txh)，提供 `sort`、`sorted`、`find`、`count`、`lower_bound`、`upper_bound`、`reverse`，以及数值向量的 `sum`、`min_element`、`max_element`。排序与二分覆盖 `vector<int/float/str>`；查找、计数和原地反转还支持 `vector<bool>`。

`sort`、`reverse` 修改共享向量，`sorted` 返回独立向量。`find` 未命中返回 -1，二分边界未命中返回向量长度；最值函数返回元素值，空向量时报错。求和检查整数溢出与非有限浮点数。字符串按 UTF-8 字节序排序；浮点排序将 NaN 放在末尾。二分输入必须已经升序排列。完整签名、空容器规则、浮点边界和复杂度见[容器算法说明](algorithm.md)，与已有容器组合使用见[示例](../examples/algorithm.tx)。

新增的静态泛型接口提供 `stable_sort/stable_sorted`、返回 `option<int>` 的 `binary_search`、`equal_range`、`unique`、`rotate`、`partition` 以及 `map/filter/fold/all/any`。编译器由 `vector<T>` 与完整回调签名确定类型，复合元素可直接参与；详见[接口契约](algorithm.md#44-与-45-的接口契约)及[可运行示例](../examples/algorithm_extended.tx)。

## 数学运算

导入 [tx/stdlib/math.txh](../tx/stdlib/math.txh)。int 版本和 float 版本的同名函数按实参类型精确匹配，不进行隐式数值转换。

| 函数 | 行为 |
| --- | --- |
| abs(value: int 或 float) | 绝对值 |
| min(left, right)、max(left, right) | 两个同类型数值中的较小值、较大值 |
| clamp(value, lower, upper) | 将数值限制在闭区间内，要求 lower <= upper |
| mod(left: int, right: int) -> int | 整数余数，符号与 left 相同 |
| sqrt(value: float) -> float | 平方根 |
| pow(base: float, exponent: float) -> float | 浮点幂 |
| floor(value: float)、ceil(value: float) -> int | 向下、向上取整 |

上述旧接口中，abs 对最小 int、mod 对零除数或最小 int 与 -1 的组合会报运行错误。旧浮点接口要求参数及结果有限；sqrt 的参数不得为负，pow 的结果必须可表示为有限 float。floor、ceil 的结果还必须落在 int 范围内。

新增三角、反三角、双曲、指数/对数、`is_finite/is_nan/is_infinite`、`gcd/lcm`、`pow_int` 和显式模式的 `round_to_int`。新增浮点函数按逐函数约定处理 NaN 与无穷大；整数溢出报告稳定错误。完整签名与特殊值规则见[时间、数学、随机数与统计](time_math_statistics.md#64-数学函数)。

## 统计与十进制

导入 [statistics.txh](../tx/stdlib/statistics.txh) 可对 `vector<float>` 或 `iterator<float>` 求均值、中位数、总体/样本方差及标准差、R7 分位数、频数和直方图；私有状态的 `accumulator` 支持逐项累计。空样本、单样本及非有限输入规则见[统计契约](time_math_statistics.md#65-统计)，可运行用法见[统计示例](../examples/statistics_summary.tx)。

导入 [decimal.txh](../tx/stdlib/decimal.txh) 从十进制文本或整数创建不可变定点数，用指定小数位和舍入模式完成四则运算；不接受从二进制 `float` 的隐式构造。精度、错误码和舍入规则见[十进制契约](time_math_statistics.md#66-十进制定点数)，金额计算见[示例](../examples/decimal_money.tx)。

## 数组操作

导入 [tx/stdlib/array.txh](../tx/stdlib/array.txh)：

| 函数 | 行为 |
| --- | --- |
| concat(left: array, right: array) -> array | 按顺序拼接两个数组 |
| slice(values: array, start: int, end: int) -> array | 取半开区间 [start, end) |
| reverse(values: array) -> array | 反转元素顺序 |
| push_back(values: array, value: any) -> void | 原地追加一个值，摊还 O(1) |
| pop_back(values: array) -> void | 原地删除尾元素；空数组报运行错误 |
| insert(values: array, index: int, value: any) -> void | 在 0 到 len(values) 之间插入，O(n) |
| erase(values: array, index: int) -> void | 删除有效下标处的元素，O(n) |
| clear(values: array) -> void | 清空共享数组 |

`concat`、`slice`、`reverse` 返回新数组，不修改参数；其中的复合元素仍共享原对象。slice 要求 0 <= start <= end <= len(values)，否则报运行错误。数组默认按引用共享；需要递归复制时使用内置 `deep_copy(values)`。

新增的原地操作会通过所有共享引用生效。`push_back` 和 `insert` 的 `value` 接受任意非 void 值，包括基础值、none、数组、字典、结构体、类和 vector；编译期确定调用目标，在进入异构容器时保存该值。其他普通函数的参数匹配规则不变。实参从左到右各求值一次，插入位置按全部实参求值后的容器长度检查；复合元素保留共享关系。

`for` 仍记录进入循环时的长度，追加的元素不进入本次遍历；每次迭代按当前下标读取共享数组，插入和删除可能改变后续读取的元素，缩短导致待访问下标越界时报告运行错误。示例见[容器接口](../examples/container_interfaces.tx)。

字符串库也定义了 slice；同时导入两个模块时，建议用 `as` 别名，例如 `text.slice(...)` 和 `arrays.slice(...)`。

## 字典操作

导入 [tx/stdlib/dictionary.txh](../tx/stdlib/dictionary.txh)，建议使用 `as dictionary` 别名。带键的操作分别提供 `int`、`float`、`bool`、`str`、`none` 重载，按键的静态类型选用；字典仍保留原有的索引规则。

| 函数 | 行为 |
| --- | --- |
| `get(values: dict, key) -> any` | 返回对应值；缺键返回 `none`，不插入新键 |
| `contains(values: dict, key) -> bool` | 判断键是否存在，包括值为 `none` 的键 |
| `remove(values: dict, key) -> bool` | 删除键并返回 `true`；缺键返回 `false` |
| `keys(values: dict) -> array` | 返回键的快照，顺序不保证 |
| `values(values: dict) -> array` | 返回值的快照，顺序不保证 |
| `items(values: dict) -> array` | 返回条目快照，每个元素是 `[键, 值]` 数组，顺序不保证 |
| `clear(values: dict) -> void` | 清空共享字典 |

`get` 返回 `none` 时，可用 `contains` 区分缺键与已存储的 `none`。`keys` 和 `values` 返回独立数组，其中的复合值仍与原字典共享对象；两次独立调用的排列不能当作彼此配对的保证。`remove` 和 `clear` 会通过共享引用生效。普通键的查找、插入与删除平均为 O(1)，哈希碰撞极端情况下可能退化。完整用法见[字典操作示例](../examples/dictionary_operations.tx)。

`items` 的外层数组及每个键值对数组均独立于字典；替换快照中的键或值不会更新原字典，但快照内的复合值仍与原对象共享。它在一次遍历中取得成对条目，也保留无法再次按键查询的 NaN 条目。none 键可直接写为 `dictionary.get(values, none)`。

浮点 `NaN` 键保留现有语义：可以存入并计入长度与遍历，但由于它不等于自身，后续查找和删除都不会命中它。

## 文件系统与路径

导入 [tx/stdlib/fs.txh](../tx/stdlib/fs.txh) 和 [tx/stdlib/path.txh](../tx/stdlib/path.txh)，表中分别以 `fs` 和 `path` 为导入别名。路径参数使用 UTF-8 字符串，空路径或含 NUL 的路径会报运行错误。路径操作不查询目标文件；`absolute` 在处理相对路径时读取进程工作目录。

| 函数 | 行为 |
| --- | --- |
| fs.exists(path) -> bool | 路径存在时返回 true；不存在时返回 false |
| fs.is_file(path) -> bool | 路径指向普通文件时返回 true |
| fs.is_directory(path) -> bool | 路径指向目录时返回 true |
| fs.create_directories(path) -> void | 创建路径及缺失的父目录；目录已存在则无操作 |
| fs.list_directory(path) -> array | 返回目录下直接子项的名称，按 UTF-8 字节序排序 |
| fs.list_directory_vector(path) -> vector<str> | 与 list_directory 相同，直接返回字符串向量 |
| fs.walk_directory(path) -> vector<str> | 递归列举所有子项，返回相对根目录的路径，按 UTF-8 字节序排序 |
| fs.copy_file(source: str, destination: str, overwrite: bool) -> void | 复制普通文件；overwrite 为 false 时目标已存在即报错 |
| fs.rename(source: str, destination: str) -> void | 重命名或在同一文件系统内移动文件、目录；目标已存在时报错 |
| fs.remove(path) -> bool | 删除文件、符号链接或空目录；不存在返回 false |
| fs.remove_all(path) -> int | 递归删除并返回删除的文件、目录和链接数量；不存在返回 0 |
| fs.file_size(path) -> int | 返回普通文件的字节数 |
| fs.modified_millis(path) -> int | 返回最后修改时间的 Unix 毫秒时间戳 |
| path.join(left, right) -> str | 拼接两个路径；right 为绝对路径时以 right 为准 |
| path.parent(path) -> str | 返回父路径；没有父路径时返回空字符串 |
| path.file_name(path) -> str | 返回末级名称 |
| path.extension(path) -> str | 返回末级扩展名，含开头的点；没有则返回空字符串 |
| path.normalize(path) -> str | 词法整理分隔符、`.` 和可消去的 `..`，不解析符号链接 |
| path.is_absolute(path) -> bool | 判断是否为当前平台的绝对路径 |
| path.absolute(path) -> str | 基于当前工作目录生成规范化绝对路径，不要求目标存在 |
| path.relative(path: str, base: str) -> str | 规范化后计算词法相对路径；二者需同为相对路径或同根绝对路径 |
| path.replace_extension(path: str, extension: str) -> str | 替换扩展名；空字符串移除扩展名，缺少开头的点时自动补点 |

`list_directory` 及其 vector 版本不递归，元素均为 str。文件系统操作遇到权限、非目录或其他 I/O 错误时会报运行错误；is_file 和 is_directory 对不存在的路径返回 false。路径函数的结果统一使用正斜杠作分隔符，便于直接写入 .tx 源码中的路径字符串。

`walk_directory` 包含文件、子目录和链接本身，不进入子目录符号链接；结果不含根目录本身。读取中遇到错误即报错，不静默跳过。`remove_all` 删除符号链接本身，不递归其目标。复制、移动和写文件均不自动创建父目录；移动跨文件系统时报告错误。修改时间精度由文件系统决定，文件大小和删除数量超出 int 范围时报错。文件系统变更失败可能已经产生部分效果，接口不提供事务或并发隔离保证。

`relative` 不访问磁盘、不消解符号链接；相同路径返回 `.`，不同盘符等无法表达相对关系的情况报错。`replace_extension` 的扩展名不得含路径分隔符或 NUL。完整操作见[文件系统接口示例](../examples/filesystem_interfaces.tx)。

## 系统与环境变量

导入 [system.txh](../tx/stdlib/system.txh) 获取程序参数、当前工作目录、可执行文件路径、临时目录和用户主目录，或切换进程工作目录。`args()` 返回不含可执行文件名的独立 `vector<str>` 快照；入口仍为 `def main() -> int`。

导入 [env.txh](../tx/stdlib/env.txh) 使用 `contains/get/set/remove` 查询和修改当前进程的环境变量。`get(name)` 在缺失时报错，`get(name, default_value)` 仅在缺失时使用默认值，已设置的空字符串不会被默认值替换。所有文本采用 UTF-8，Windows 上通过宽字符 API 访问；完整签名、失败和进程范围规则见[系统与环境变量](system_env.md)，组合用法见[示例](../examples/system_env.tx)。

## 时间

导入 [tx/stdlib/time.txh](../tx/stdlib/time.txh)：

| 函数 | 行为 |
| --- | --- |
| unix_millis() -> int | 返回当前 Unix 时间戳，单位毫秒 |
| monotonic_millis() -> int | 返回单调时钟读数，单位毫秒 |
| monotonic_micros() -> int | 返回单调时钟读数，单位微秒 |
| sleep_millis(duration: int) -> void | 至少等待指定的非负毫秒数 |

unix_millis 使用系统时钟，系统时间调整可能让相邻读数倒退。monotonic_millis 和 monotonic_micros 的起点没有日历含义，只适合用两次读数之差测量经过时间；微秒单位不保证实际时钟具有微秒分辨率。sleep_millis(0) 无需等待；负数参数会报运行错误，实际等待时间可能比请求的更长。

新增 `duration/instant`、可取消睡眠、本地日期与时刻、偏移及 IANA 时区时间。单调刻度不能转换为墙上时间；ISO 8601 与夏令时歧义规则见[时间契约](time_math_statistics.md#61-时长与单调时间)和[日历示例](../examples/time_calendar.tx)。

## 伪随机数

导入 [tx/stdlib/random.txh](../tx/stdlib/random.txh)：

| 函数 | 行为 |
| --- | --- |
| seed(value: int) -> void | 用指定整数重新初始化当前随机序列 |
| random_int(lower: int, upper: int) -> int | 在含两端点的区间内均匀取整数 |
| random_float() -> float | 在半开区间 [0.0, 1.0) 内取浮点数 |

random_int 要求 lower <= upper，支持完整的 int 范围；区间无效时会报运行错误。未调用 seed 时，生成器由 C++ random_device 初始化，实际熵源取决于运行环境。用相同种子并按相同顺序调用，在同一构建环境中可重现结果。这些函数使用伪随机数生成器，不适合生成密钥、令牌或其他安全敏感值。

新增独立 `generator`、抽样、洗牌及分布函数，固定算法版本及共享/深复制规则见[随机数契约](time_math_statistics.md#63-独立随机生成器)。

## 终端输入输出

`input()` 从标准输入读取一行 UTF-8 文本，返回不含行尾换行的 `str`。`input("提示")` 先将提示写到标准输出并刷新。输入到文件末尾时，若已有字符但没有结尾换行，仍返回这最后一行；之后再次调用才报运行错误。读取失败或输入不是有效 UTF-8 时也报运行错误。

`input_or_none()` 和 `input_or_none("提示")` 使用同样的读取与提示规则，但在没有更多输入时返回 `none`；读到一行时返回 `str`，静态返回类型为 `any`。可先用 `is_none` 判断，再用 `as str` 取得文本。真正的读取失败和无效 UTF-8 仍报运行错误。

`print()` 输出空行；`print(value, ...)` 输出零个或多个值，默认用空格分隔、末尾换行。可以在值后指定 `sep` 和 `end` 字符串，如 `print("a", 2, sep=", ", end="!\n")`。不支持在 `print` 中使用 `*` 或 `**` 展开。数值和布尔值的文本格式与 `as str` 一致；顶层字符串原样输出，`none` 输出为 `none`。数组显示为 `[元素, ...]`，字典显示为 `{键: 值, ...}`，结构体显示为 `类型名(字段=值, ...)`；嵌套字符串以双引号括起，并转义反斜杠、引号及 ASCII 控制字符。字典不保证输出顺序，结构体保持字段声明顺序。复合值嵌套超过 64 层时报运行错误。`as str` 对复合值的现有转换规则不变。

需要控制换行、标准错误或刷新时，导入 [tx/stdlib/io.txh](../tx/stdlib/io.txh)：

```tx
import "io.txh" as io

def main() -> int {
    io.write("姓名> ")
    io.flush()
    auto name = input()
    io.write_line("你好，" + name)
    io.write_error("这条消息写入标准错误\n")
    return 0
}
```

| 函数 | 行为 |
| --- | --- |
| `write(text: str)` | 写入标准输出，不自动换行 |
| `write_line(text: str)` | 写入标准输出并追加 `\n` |
| `write_error(text: str)` | 写入标准错误，不自动换行 |
| `flush()` | 刷新标准输出 |

上述函数均返回 `void`；输出失败会报运行错误。标准输入、标准输出和标准错误使用 UTF-8；重定向时直接读写 UTF-8 字节。

多值输出与 EOF 输入的完整示例见 [print_input.tx](../examples/print_input.tx)。

## 文本文件输入输出

导入 [tx/stdlib/file.txh](../tx/stdlib/file.txh)。所有文件路径均是 UTF-8 `str`；Windows 上先严格解码为宽字符路径再打开文件，因此中文路径不依赖系统活动代码页。

```tx
import "file.txh" as file

def main() -> int {
    file.write_text("记录.txt", "你好\n", "utf-8")
    file.append_text("记录.txt", "世界\n", "utf-8")
    print(file.read_text("记录.txt", "utf-8"))
    return 0
}
```

| 函数 | 行为 |
| --- | --- |
| `read_text(path: str, encoding: str) -> str` | 读完整个文件，解码为 UTF-8 字符串 |
| `write_text(path: str, text: str, encoding: str) -> void` | 转换编码后创建或覆盖文件 |
| `append_text(path: str, text: str, encoding: str) -> void` | 转换编码后追加到文件；不存在则创建 |

`encoding` 不区分大小写，支持 `utf-8`、`utf-8-sig`、`utf-16`、`utf-16le`、`utf-16be`、`gbk` 和 `gb18030`；`utf8`、`utf8-sig`、`utf16`、`utf16le`、`utf16be` 是对应别名。`utf-8-sig` 写入 BOM，读取时去掉可选 BOM；`utf-16` 写入小端 BOM，读取时要求有小端或大端 BOM；显式 `utf-16le`、`utf-16be` 写入时不加 BOM，读取时允许去掉匹配的 BOM。追加到非空 `utf-16` 文件时沿用文件 BOM 指定的字节序；追加到空文件时写入小端 BOM。追加到非空 `utf-8-sig` 文件时不重复写 BOM。

文件函数不改变换行符，不自动建立父目录。未知字符集、无效编码、目标字符集无法表示文本、路径错误及读写失败都会报运行错误。编码转换会先完成，再打开待写文件，避免因编码错误覆盖现有文件。

## 显式转换

`as` 用于数值与字符串之间的转换，转换失败会报运行错误：

| 源类型 | 目标类型 | 规则 |
| --- | --- | --- |
| int | float | 转成双精度浮点数，可能损失精度 |
| float | int | 向零截断，检查是否超出 int 范围 |
| str | int | 解析完整的十进制整数，允许首尾 ASCII 空白和正负号 |
| str | float | 解析完整的有限浮点数，允许首尾 ASCII 空白 |
| int、float、bool | str | 转为对应的文本表示 |
| any 中实际存放上述值 | int、float、str | 按实际类型执行对应转换 |
| any 中实际存放 bool | bool | 严格取出布尔值；其他实际类型报运行时类型错误 |

例如 2 as float 输出 2.0，"42" as int 得到 42，3.5 as str 得到 "3.5"。对 none、数组或结构体做这些转换会报运行错误。

静态类型为 `any` 的表达式赋给明确的 `int`、`float`、`str` 目标时，可省略 `as`；显式变量声明、普通赋值和返回值采用相同的转换及错误规则。例如 `array values = [1, 2, 3.2, "666"]` 后，`int number = values[1]`、`float decimal = values[2]`、`str text = values[3]` 分别得到 `2`、`3.2`、`"666"`。静态类型已知的 `int`、`float`、`str` 之间仍需显式使用 `as`。

## 字符串基础操作

先导入 [tx/stdlib/string.txh](../tx/stdlib/string.txh)：

```tx
import "string.txh"
```

| 函数 | 返回值 | 说明 |
| --- | --- | --- |
| len(text) | int | UTF-8 字符数量；len(array) 返回数组长度，len(dict) 返回键数 |
| contains(text, part) | bool | 是否包含子串 |
| starts_with(text, prefix) | bool | 是否以指定文本开头 |
| ends_with(text, suffix) | bool | 是否以指定文本结尾 |
| find(text, part) | int | 首次出现的字符位置，找不到返回 -1 |
| slice(text, start, end) | str | 按字符位置取半开区间 [start, end) |
| replace(text, old, new) | str | 替换所有 old；old 不得为空 |
| split(text, separator) | array | 按非空分隔符拆分，保留空段 |
| split_vector(text, separator) | vector<str> | 直接生成字符串向量，保留空段，分隔符不得为空 |
| join(parts, separator) | str | 接受 array 或 vector<str>；array 中其他元素类型报错 |
| trim(text) | str | 去除首尾 ASCII 空白 |
| lower(text) / upper(text) | str | 转换 ASCII 字母大小写，其他字符保持原样 |

字符位置从 0 开始；slice 要求 0 <= start <= end <= len(text)。字符串使用 UTF-8 保存，以上按字符计数的操作会检查编码有效性。这些函数的实现位于编译得到的标准库静态库中，.txh 只提供接口。完整用法见 [导入与输入示例](../examples/import_io.tx)。

## 字符串格式化

导入 [tx/stdlib/format.txh](../tx/stdlib/format.txh)，调用 `format(template: str, *args: array, **kwargs: dict) -> str`：

```tx
import "format.txh" as fmt

def main() -> int {
    print(fmt.format("你好，{}！", "世界"))
    print(fmt.format("{name}: {score:.2f}", name="Ada", score=9.875))
    print(fmt.format("{name}: {score:.2f}", "Ada", 9.875))
    print(fmt.format("{0:>6} | {1:04d} | {{完成}}", "TX", 7))
    return 0
}
```

- `{}` 按顺序引用位置实参；`{0}`、`{1}` 按下标引用；`{name}` 优先引用同名命名实参。没有同名实参时，`{name}` 按该名称首次出现的顺序消耗下一个位置实参，后续相同名称复用该值。因此 `fmt.format("{name}: {score:.2f}", "Ada", 9.875)` 也得到 `Ada: 9.88`。`{}` 与这种名称回退可混用并共用顺序；它们不能与显式数字编号在同一模板中混用。同一实参可引用多次；多余的实参允许保留不用。名称回退是 TX 相对 Python `str.format` 的扩展。
- `{{` 和 `}}` 分别产生字面量 `{` 和 `}`。孤立的大括号、缺失的实参和不支持的字段语法会报运行错误。
- 字段可写 `!s`（默认）或 `!r`；`!r` 会给顶层字符串加双引号并转义，复合值仍按 `print` 的表示方式展开。
- `:` 后可写格式说明：可选单个 ASCII 填充字符与 `<`、`>`、`^` 对齐符，可选 `+` 或空格数值符号，可选 `0` 零填充、十进制宽度、`.精度` 和类型。宽度按 UTF-8 字符数计算，精度对字符串表示保留的字符数，对浮点类型表示小数位或有效位。宽度与精度的上限各为 1000000。
- 类型 `s` 用于字符串；`d`、`b`、`o`、`x`、`X` 用于整数；`f`、`F`、`e`、`E`、`g`、`G` 用于浮点数。省略类型时使用值的默认文本表示；对字符串可配合精度截断。浮点数指定类型但省略精度时默认 6 位。`0` 零填充用于数字的右对齐，负号和显式正号保持在零之前。

格式化只返回字符串，不输出。类型不匹配、无效格式说明、字段属性或下标访问、嵌套替换字段、无效 UTF-8 都会报可理解的运行错误。目前只覆盖上述 Python 风格常用语法；完整示例见 [format.tx](../examples/format.tx)。

格式模板为首个位置实参的字符串字面量、其余实参为可静态绑定的 `int/float/bool/str` 时，编译器直接生成文本片段与类型专用格式化调用；字段绑定、宽度、精度和转换说明在编译期确定，不构造 `args/kwargs` 或动态装箱参数。所有实参仍先按源码顺序求值，多余实参的副作用也保留。动态模板、参数展开、复合值以及非法模板继续走通用入口，原有运行时错误仍可捕获。

## 测试、日志与调试

导入 [test.txh](../tx/stdlib/test.txh)、[log.txh](../tx/stdlib/log.txh) 和 [debug.txh](../tx/stdlib/debug.txh) 后，TX 程序可以直接使用断言、结构化事件和源码级诊断。测试文件以 `_test.tx` 结尾时，`txc test <目录>` 会按稳定顺序发现；也可用 `txc test <文件.tx>` 显式运行单项。

```tx
import "test.txh" as test
import "log.txh" as log
import "debug.txh" as debug

def main() -> int
{
    test.assert_equal(4, 2 + 2)
    log.event("info", "计算完成", {"count": 4})
    debug.dump(4)
    return 0
}
```

断言失败会携带 TX 调用位置；日志在序列化前遮蔽常见敏感字段，`event_redacted` 可指定其他键。`debug.stack_trace()` 和 `debug.location()` 在 Release 产物中也保留 TX 源码位置。完整接口、机器可读报告与边界见[测试、日志与诊断](test_log_debug.md)。
