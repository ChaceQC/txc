# 类型化 map、set、堆和队列

在内置 `vector<T>` 的类型机制上增加 `map<K, V>`、`set<T>`、`heap<T>` 和 `queue<T>`。它们是内置参数化类型，不需要 import，不引入用户泛型。`map/set` 的键支持 `int`、`float`、`bool`、`str` 和满足[哈希键契约](hash_key_contract.md)的用户结构体；map 值与 heap/queue 元素仍只支持四种标量。`any`、`none`、可变容器和类不能作为哈希键。

## 类型、共享和动态边界

- 局部变量、函数参数和返回值、struct/class 字段及普通模块的 `.txh` 接口保留完整类型。键、元素和值必须精确匹配，不隐式转换；标量沿用原生表示，结构体键沿用复合值句柄并在插入时复制。
- `map<K, V>()`、`set<T>()`、`queue<T>()` 创建空容器。`heap<T>()` 创建小顶堆，`heap<T>(true)` 创建大顶堆，`heap<T>(false)` 等同默认构造。未显式赋值的 class 容器字段为空，堆字段默认为小顶堆。
- 普通赋值、传参、返回和字段存储共享同一容器；`deep_copy` 创建独立容器，并保留复制对象图内部的共享关系。不可变字符串允许共享文本载荷。
- 容器可放入旧 array/dict，通过元素读取进入 any；恢复时必须显式 `as map<K, V>` 等，运行时检查容器种类及全部类型参数。`print`、动态 `len` 和 `deep_copy` 支持这些值。
- 方法只接受普通位置实参。接收者、键和其他实参从左到右各求值一次；后续实参重新绑定原变量或字段时，已求值的接收者仍保持有效。`get` 的默认值也会求值。

## map 和 set

map/set 使用哈希容器，不保证遍历或输出顺序，不提供有序映射的区间查询。字符串按完整 UTF-8 字节内容比较和哈希。浮点 +0.0 与 -0.0 是同一个键；NaN 不能用作 map 键或 set 元素，查询、插入和删除遇到 NaN 都报运行错误。无穷大可作键。map 的浮点值及 queue 的浮点元素允许 NaN。

| 操作 | 行为 |
| --- | --- |
| `len(c)`、`c.size()`、`c.empty()` | 元素数（int）或是否为空（bool） |
| `c.clear()` | 原地清空，无返回值 |
| `m[key]` | 返回 V；缺键报运行错误，不插入默认值 |
| `m[key] = value` | 插入或替换；数值支持 `+=/-=/++/--`，字符串支持 `+=`，更新操作要求键已存在 |
| `m.contains(key)`、`s.contains(value)` | 返回 bool |
| `m.get(key, fallback)` | 返回已存储的 V 或类型为 V 的 fallback，不修改 map |
| `m.remove(key)`、`s.remove(value)` | 返回是否删除了元素；不存在时返回 false |
| `s.insert(value)` | 新增返回 true，已有元素返回 false |
| `m.keys()`、`m.values()` | 独立的 `vector<K>`、`vector<V>` 快照；两次调用之间没有修改时，位置一一对应 |
| `s.to_vector()` | 独立的 `vector<T>` 快照 |
| `for key in m`、`for value in s` | 遍历进入循环时的键或元素快照，变量分别为 K、T；循环内增删和 clear 不影响本次快照 |

map/set 的查询、插入和删除平均 O(1)，极端哈希冲突时 O(n)；快照和清空 O(n)。第一版不提供复合条目类型，可用 `for key in m` 配合 `m[key]` 查询值；循环中删除键后再读取该键仍报缺键错误。

## heap 和 queue

| 操作 | 行为 |
| --- | --- |
| `len(c)`、`c.size()`、`c.empty()`、`c.clear()` | 与 map/set 相同 |
| `h.push(value)`、`q.push(value)` | 插入一个 T，无返回值 |
| `h.top()` | 返回堆顶的 T，O(1) |
| `h.pop()` | 删除堆顶，无返回值，O(log n) |
| `q.front()`、`q.back()` | 返回队首、队尾的 T，O(1) |
| `q.pop()` | 删除队首，无返回值，O(1) |
| `h.to_vector()` | 按出堆顺序返回独立快照，O(n log n)，原堆不变 |
| `q.to_vector()` | 按 FIFO 顺序返回独立快照，O(n)，原队列不变 |
| `for value in h`、`for value in q` | 遍历上述快照，变量为 T，循环内修改不改变本次遍历 |

堆插入 O(log n)，队列尾部插入 O(1)。空堆 top/pop、空队列 front/back/pop 报中文运行错误，沿用错误退出和析构清理规则。堆不接受 NaN；字符串按 UTF-8 字节序排列，bool 按 false 小于 true。暂不支持自定义比较器、优先级与载荷配对、下标访问堆或队列。

## 实现与验证

语义分析确定完整类型和调用目标，LLVM 调用对应的类型化 C ABI。C++23 标准库以原生标量或不可变文本引用保存元素，普通操作不逐元素 any 装箱或运行时名称分派。旧 array/dict、打印、显式 any 转换和对象图复制保留动态边界。这些容器自身不能成环，不登记循环 GC；外部持有它们的 array/dict/class 仍使用原来的回收机制。

用法见[类型化容器示例](../examples/typed_containers.tx)；根目录 [example.tx](../example.tx) 的 `typed_container_demo` 同样展示 map/set、大小顶堆、FIFO 队列、共享与复制、快照遍历和 any 恢复。

2026-09-26：上述代码已实现，`scripts/build.ps1` 构建通过，已更新 `tx/txc.exe` 和 `tx/libtxstdlib.a`，成功后已清理 `build/`。代码交付阶段按用户要求暂未执行测试。

同日经用户授权进行少量测试，13 个定向场景全部通过：

- 3 个正常运行场景：独立容器示例；跨 `.txh` 的组合行为；补充后的根目录 `example.tx`。覆盖四种基础类型的代表性组合、类字段默认值、接收者重绑定和实参求值顺序、rehash 后写入、快照遍历、字符串生命周期、共享与 deep_copy、动态打印和 any 恢复、堆与 FIFO 顺序。主示例的新增输出及示例目录清理也已核对。
- 7 个预期运行错误：map 缺键、空 heap pop、空 queue front、map/set/heap 拒绝 NaN、any 恢复到错误的 map 值类型。均以退出码 1 报中文错误，且析构对象仍能读取队列中的字符串。
- 3 个编译诊断：错误的 map 键类型、错误的 heap 元素类型、嵌套容器类型参数。均包含源码文件和中文错误。

用 `python -X utf8 scripts/check_typed_containers.py` 复现上述场景，也可指定 `example`、`behavior`、`errors`、`diagnostics`、`main_example` 分组。用例位于 [tests/containers](../tests/containers/)。本轮没有发现需要修复的实现问题，未运行全量回归或性能基准。
