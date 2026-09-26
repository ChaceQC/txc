# 类型化容器算法

导入 `algorithm.txh`，建议使用 `import "algorithm.txh" as algorithm`。本模块复用已有 `vector<T>` 的连续存储、共享引用和字符串所有权。既有重载保持原语义；新增操作使用编译期具体类型与明确签名的回调。

## 接口

下面的 T 仅用于说明重载范围，公开 `.txh` 中分别声明具体类型。实参按完整静态类型精确匹配，不隐式转换数值类型，也不自动将 array、map、set、heap 或 queue 转为 vector。

| 函数 | 元素类型 | 返回值及行为 | 复杂度 |
| --- | --- | --- | --- |
| `sort(values: vector<T>) -> void` | int、float、str | 原地升序排列，不保证稳定性 | O(n log n) 次比较 |
| `sorted(values: vector<T>) -> vector<T>` | int、float、str | 返回独立的升序向量，保留原向量及其别名的顺序 | O(n log n) 次比较，O(n) 新存储 |
| `find(values: vector<T>, value: T) -> int` | int、float、bool、str | 第一个相等元素的下标；未找到返回 -1 | O(n) 次比较 |
| `count(values: vector<T>, value: T) -> int` | int、float、bool、str | 相等元素个数 | O(n) 次比较 |
| `lower_bound(values: vector<T>, value: T) -> int` | int、float、str | 升序向量中第一个不小于 value 的下标 | O(log n) 次比较 |
| `upper_bound(values: vector<T>, value: T) -> int` | int、float、str | 升序向量中第一个大于 value 的下标 | O(log n) 次比较 |
| `reverse(values: vector<T>) -> void` | int、float、bool、str | 原地反转元素顺序 | O(n) |
| `sum(values: vector<T>) -> T` | int、float | 按下标从左到右累加，返回同类型数值 | O(n) |
| `min_element(values: vector<T>) -> T` | int、float | 返回最小元素的值 | O(n) |
| `max_element(values: vector<T>) -> T` | int、float | 返回最大元素的值 | O(n) |

所有下标从 0 开始。二分边界的结果范围为 `[0, len(values)]`；没有对应元素时返回 `len(values)`，不是 -1。空向量的 sort/reverse 无操作，sorted 返回新的空向量，find 返回 -1，count 和两种二分边界返回 0，sum 分别返回整数 0 或浮点数 0.0；空向量的 min_element/max_element 报运行错误。最值接口返回元素值，不是下标；多个相等的最值保留首次出现的值。

## 排序与相等规则

- int 使用有符号整数升序。str 按 UTF-8 无符号字节的字典序比较，区分大小写，不采用拼音、区域设置或 Unicode 规范化；查找和计数按完整文本字节相等判断。字符串比较还需计入实际检查的文本长度。
- float 的排序和二分使用同一顺序：普通数值从小到大排列，允许正负无穷，所有 NaN 排在最后并属于同一排序等价组；正负零排序等价。该规则保证含 NaN 时比较器仍满足严格弱序。
- float 的 find/count 沿用语言的数值相等规则：正负零相等，NaN 不等于任何值，包括自身。因此 NaN 的 find 返回 -1，count 返回 0；lower_bound/upper_bound 可以定位已排序向量末尾的 NaN 区间。
- 二分查找要求调用方提供按上述规则升序排列的向量。接口不扫描验证整个向量，也不会自动排序；传入无序向量时结果不作保证。对升序向量调用 reverse 后，需再次排序才能使用二分接口。

## 共享、错误与复用

sort/reverse 修改已有共享容器，别名、函数参数和字段均能观察到变更；长度、容量和容器身份不变。遍历仍按 vector 原有规则读取当前下标，循环中重排会影响后续读到的元素。sorted 复制元素存储，后续修改结果不会修改源向量；字符串元素复用不可变文本引用，不逐个复制文本或装箱。

int 的 sum 在每一步相加前检查 int64 溢出；中间结果越界即报运行错误，即使后续元素可能抵消该值也不会继续。float 的 sum/min_element/max_element 与 math 模块保持一致，要求元素均为有限值；sum 每一步的结果也必须有限，否则报运行错误。浮点求和使用 double，保留通常的舍入误差。错误沿用当前运行时的中文错误和退出清理流程，不增加尚未确定的可恢复结果类型。

map 可先取 keys()/values()，set/heap/queue 可先取 to_vector()，再交给 algorithm。这些是已有容器的独立快照，对快照排序或反转不会修改原容器。完整示例见 [algorithm.tx](../examples/algorithm.tx)；根目录 [example.tx](../example.tx) 的 `algorithm_demo()` 同样展示全部算法名称、四种元素及 map 快照的组合用法。

## 4.4 与 4.5 的接口契约

下面的 `T`、`U` 是 `.txh` 中的静态类型变量，调用处由编译器从 `vector<T>` 和回调签名确定。受限 `def name<T, U>(...)` 只用于标准库算法的外部接口声明，不开放用户泛型函数实现；`T`、`U` 可以是现有 `vector` 支持的具体非函数类型。静态已知元素不会逐项装箱为 `any`。

| 操作 | 静态签名 | 修改和返回 |
| --- | --- | --- |
| `stable_sort` | `(vector<T>[, fn(T,T)->int]) -> void` | 原地稳定排序 |
| `stable_sorted` | `(vector<T>[, fn(T,T)->int]) -> vector<T>` | 返回独立向量 |
| `binary_search` | `(vector<T>, T[, fn(T,T)->int]) -> option<int>` | 返回首个等价元素下标；未找到为空 |
| `equal_range` | `(vector<T>, T[, fn(T,T)->int]) -> vector<int>` | 返回两个下标 `[first, past_last]` |
| `unique` | `(vector<T>[, fn(T,T)->int]) -> int` | 原地删除连续等价元素，返回新长度 |
| `rotate` | `(vector<T>, middle: int) -> void` | 原地将 `[middle, n)` 移到前面 |
| `partition` | `(vector<T>, fn(T)->bool) -> int` | 原地分区，返回第一个不满足谓词的下标；不保证组内顺序 |
| `map` | `(vector<T>, fn(T)->U) -> vector<U>` | 返回新向量 |
| `filter` | `(vector<T>, fn(T)->bool) -> vector<T>` | 返回新向量 |
| `fold` | `(vector<T>, U, fn(U,T)->U) -> U` | 按下标从左到右累计 |
| `all`、`any` | `(vector<T>, fn(T)->bool) -> bool` | 短路求值；空向量分别为 true、false |

默认排序接受 int、float、bool、str 或声明同类型 `operator <` 的值结构体；其余具体类型须提供显式比较器。比较器用负、零、正表示顺序，需形成严格弱序。默认 float 顺序与旧 `sort` 一样将 NaN 放在最后，正负零等价。显式比较器遇到 NaN 元素报运行错误。`binary_search/equal_range` 的输入须按同一比较器排序；编译器不扫描验证。`unique` 只删除相邻等价项，通常先排序。`rotate` 的 middle 必须在 `[0, len]`；空向量允许 middle 为 0。

回调错误按原错误类型传播；本组原地操作先在工作副本中处理，成功后一次替换原向量内容，因此比较器或谓词失败时原向量不变。回调可产生的外部效果不能回滚。原地操作的回调不得修改正在操作的源向量，也不得在一次调用中改变比较关系；这两条由调用方保证。二分查询和 `map/filter/fold/all/any` 在调用开始时取得元素快照，回调修改源向量不会改变本次处理的元素序列。`map/filter/fold` 的结果共享规则与普通向量及元素赋值一致；要独立复制嵌套对象图时使用 `deep_copy`。元素数和下标超过 int64 可表示范围时报溢出错误。

## 实现状态

2026-09-26：按上述契约接入具体 `.txh` 重载、C++23 算法、类型化 C ABI 和 LLVM 静态调用。复用现有类型检查与参数绑定，不在运行时按算法名或元素类型分派。

`scripts/build.ps1` 构建通过，已更新 `tx/txc.exe`、`tx/libtxstdlib.a`，成功后已清理 `build/`。初次代码交付时按用户要求暂不测试。

同日获用户授权后完成 8 个定向场景，最终全部通过，可用 `python -X utf8 scripts/check_algorithm.py` 复现：

- 3 个正常运行场景：独立 algorithm 示例、根目录 example.tx、综合边界用例。合计调用了全部 30 个公开重载，核对共享修改和独立副本、其他容器的快照复用、空向量、整数极值、NaN/无穷/正负零、UTF-8 文本与字符串所有权，以及命名和展开实参的绑定、字段重绑定时的求值顺序。
- 4 个预期运行错误：空向量取最小值、整数累加中间溢出、有限浮点累加溢出、含 NaN 的浮点最大值。均检查退出码、中文错误信息及错误退出时的类析构清理。
- 1 个静态诊断：向 vector<int> 查找接口传入 float，编译器拒绝并报告文件、行和列。

根目录 example.tx 新增的算法输出、原有主流程退出码及自建目录清理均已核对。验证过程中修正了用例误用的 vector.empty()，改用已有 size()；算法实现无需调整。已通过的示例未重复执行，未运行全量回归或性能测试。

### 4.4～4.6 实施记录（2026-09-26）

`algorithm.txh` 增加受限的 `def name<T, U>` 静态泛型声明；编译器从具体实参与回调签名选择 C ABI，用户自定义泛型函数体仍不支持。旧具体重载和错误规则保留。`src/stdlib/algorithm_extended.hpp` 用具体元素类型实现稳定排序、二分范围、去重、旋转、分区和组合算法；标量与 bytes 保持直接元素表示，复合值使用已有对象句柄。原地操作在工作副本成功后提交，只读组合算法按调用开始时的元素快照运行。

`scripts/build.ps1` 在 Windows x64 完整构建并更新 `tx/` 发行物。`tests/algorithm/extended.tx`、`integration.tx` 及 `examples/algorithm_extended.tx` 编译运行通过；覆盖结构体稳定排序、显式比较器、`option<int>` 二分结果、bytes 与复合值的映射/累计、共享和深复制、动态恢复、迭代中修改、NaN、比较器错误、整数溢出以及结构体所持类成员的析构。错误回调类型和用户泛型函数体分别在 `extended_wrong_type.tx`、`generic_user_rejected.tx` 中于文件、行、列报中文诊断。4.3 的堆/队列边界证据见[类型化容器](typed_containers.md#heap-和-queue)。未运行无关全量测试、性能基准或非 Windows 平台验收。
