# 类型化 map、set、堆和队列

在内置 `vector<T>` 的类型机制上增加 `map<K, V>`、`set<T>`、`ordered_map<K, V>`、`ordered_set<T>`、`heap<T>`、`queue<T>` 和 `deque<T>`。它们是内置参数化类型，不需要 import，不引入用户泛型。`map/set` 的键支持 `int`、`float`、`bool`、`str` 和满足[哈希键契约](hash_key_contract.md)的用户结构体；映射值与 `deque/vector/heap/queue` 元素一样支持可持有的具体非函数类型。`any`、`none`、可变容器和类不能作为哈希键。

## 类型、共享和动态边界

- 局部变量、函数参数和返回值、struct/class 字段及普通模块的 `.txh` 接口保留完整类型。键、元素和值必须精确匹配，不隐式转换；标量沿用原生表示，结构体键沿用复合值句柄并在插入时复制。
- `map<K, V>()`、`set<T>()`、`queue<T>()` 创建空容器。`ordered_map<K,V>()`、`ordered_set<K>()` 使用默认比较，构造时也可传入 `fn(K,K)->int`。`heap<T>()` 创建小顶堆，`heap<T>(true)` 创建大顶堆，`heap<T>(false)` 等同默认构造。未显式赋值的 class 容器字段为空，堆字段默认为小顶堆。
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

map/set 的查询、插入和删除平均 O(1)，极端哈希冲突时 O(n)；快照和清空 O(n)。`m.entries()` 一次取得 `vector<entry<K,V>>` 快照，每个条目有只读的 `key`、`value` 字段。快照中的结构体键独立于原容器；复合值按普通容器赋值语义共享，需要独立对象图时显式 `deep_copy`。`keys()` 与 `values()` 是分别取得的快照，不能假定两次调用之间键值的配对关系。循环中删除键后再读取该键仍报缺键错误。

## 有序 map 和 set

`ordered_map<K,V>()` 与 `ordered_set<K>()` 使用内置顺序或结构体的 `operator <`；`ordered_map<K,V>(compare)` 与 `ordered_set<K>(compare)` 接受 `fn(K,K)->int`。显式比较器在容器整个存活期由容器持有，支持闭包。比较器返回负、零、正，形成严格弱序；排序关系不能依赖后来改变的捕获状态，比较期间不得修改同一容器。比较器失败沿 TX 错误传播，失败的查找/插入/删除不改变容器，但比较器自身可能已有外部效果。比较器视为相等的键只保存首次插入的键，更新其值不替换键。

有序容器提供与无序容器相同的 `size/empty/clear/contains/remove`；映射还提供索引读写、`get/keys/values/entries`，集合提供 `insert/to_vector`。`range(low, high)` 返回 `[low, high)` 的独立快照：映射为 `vector<entry<K,V>>`，集合为 `vector<K>`。若 `high` 排在 `low` 之前则返回空快照。`for` 遍历有序容器时取得开始时的有序键快照，遍历期间的修改不影响此次键序列。

有序容器的查找、插入和删除为 O(log n) 次比较；`range` 为 O(log n + r)，`keys/values/entries/to_vector` 为 O(n)。内置 `str` 仍按 UTF-8 无符号字节排序，浮点 NaN 作键报运行错误，`+0.0` 与 `-0.0` 是同一键。结构体键在插入和返回快照时深复制，比较器读取独立键值。普通赋值共享容器，`deep_copy` 复制容器对象图；显式比较器闭包也随对象图复制。

## heap 和 queue

以下接口在原有标量容器之上扩展，不改变原有构造或排序规则。

`queue<T>(values: vector<T>)` 按向量下标顺序批量入队，O(n)。`heap<T>(values: vector<T>)` 以 O(n) 建堆；原有 `heap<T>()`、`heap<T>(descending: bool)` 保留。`heap<T>(compare: fn(T,T)->int)`、`heap<T>(values: vector<T>, compare: fn(T,T)->int)` 使用显式比较器；比较器返回负/零/正、形成严格弱序，不得修改正在比较的堆或改变比较关系。复合元素默认顺序要求同类型 `operator <`，也可传显式比较器。`queue/heap` 的复合载荷按普通容器赋值语义共享，`deep_copy` 才独立复制内部对象图。

`priority_entry<T>(priority, value)` 是只读内置类型；`heap<priority_entry<T>>` 先按整数 `priority` 排序，数值相同按插入先后顺序出堆。小顶堆先取低优先级，大顶堆先取高优先级。批量建堆以输入向量下标作为初始插入顺序；后续 `push` 继续使用递增序号。序号超出 `uint64` 上限时插入失败，容器不变。`top()`、`to_vector()` 返回条目本身，不暴露序号。普通 `heap<T>` 对排序等价的元素不保证稳定性；需要稳定次序时使用优先级条目。

空 `heap` 的 `top/pop` 和空 `queue` 的 `front/back/pop` 报运行错误；`to_vector` 返回独立快照。浮点 NaN 不能进入默认排序的堆，显式比较器同样拒绝 NaN 元素。比较器失败时批量构建不生成容器，单次 `push` 保持原堆不变；对已存在堆的 `to_vector` 比较失败不修改堆。比较器自身的外部效果不能回滚。

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

堆插入 O(log n)，队列尾部插入 O(1)。空堆 top/pop、空队列 front/back/pop 报中文运行错误，沿用错误退出和析构清理规则。堆不接受 NaN；字符串按 UTF-8 字节序排列，bool 按 false 小于 true。堆和队列不支持下标访问。

## deque 双端队列

`deque<T>()` 创建空队列。`T` 的限制与 `vector<T>` 相同；`deque` 的普通赋值共享底层存储，`deep_copy` 递归复制元素并保留对象图的共享关系，`any` 恢复必须写 `as deque<T>`。`for value in deque` 和 `to_vector()` 在进入遍历或调用时制作独立快照，之后修改原队列不会改动快照。`deque` 不比较元素，允许存入 NaN。

| 操作 | 行为与复杂度 |
| --- | --- |
| `len(d)`、`d.size()`、`d.empty()` | 元素数或是否为空，O(1) |
| `d.push_front(value)`、`d.push_back(value)` | 两端插入，O(1) |
| `d.pop_front()`、`d.pop_back()` | 两端删除，O(1) |
| `d.front()`、`d.back()`、`d[index]` | 读取 T，O(1) |
| `d[index] = value` | 原地替换，O(1) |
| `d.insert(index, value)`、`d.erase(index)` | 中间插入或删除，O(n)；插入位置允许等于长度 |
| `d.clear()`、`d.to_vector()` | 清空或复制快照，O(n) |

索引从 0 开始，不接受负数。空队列上的读取和删除、越界索引都抛出可捕获的中文运行错误，并保持原队列不变。插入/复制期间内存不足按现有运行时错误处理；不承诺分配失败后的强异常保证。普通结构体、类和嵌套容器元素沿用 TX 的共享与 `deep_copy` 规则。

## 实现与验证

语义分析确定完整类型和调用目标，LLVM 调用对应的类型化 C ABI。C++23 标准库以原生标量或不可变文本引用保存元素，普通操作不逐元素 any 装箱或运行时名称分派。旧 array/dict、打印、显式 any 转换和对象图复制保留动态边界。这些容器自身不能成环，不登记循环 GC；外部持有它们的 array/dict/class 仍使用原来的回收机制。

用法见[类型化容器示例](../examples/typed_containers.tx)；根目录 [example.tx](../example.tx) 的 `typed_container_demo` 同样展示 map/set、大小顶堆、FIFO 队列、共享与复制、快照遍历和 any 恢复。

2026-09-26：上述代码已实现，`scripts/build.ps1` 构建通过，已更新 `tx/txc.exe` 和 `tx/libtxstdlib.a`，成功后已清理 `build/`。代码交付阶段按用户要求暂未执行测试。

同日经用户授权进行少量测试，13 个定向场景全部通过：

- 3 个正常运行场景：独立容器示例；跨 `.txh` 的组合行为；补充后的根目录 `example.tx`。覆盖四种基础类型的代表性组合、类字段默认值、接收者重绑定和实参求值顺序、rehash 后写入、快照遍历、字符串生命周期、共享与 deep_copy、动态打印和 any 恢复、堆与 FIFO 顺序。主示例的新增输出及示例目录清理也已核对。
- 7 个预期运行错误：map 缺键、空 heap pop、空 queue front、map/set/heap 拒绝 NaN、any 恢复到错误的 map 值类型。均以退出码 1 报中文错误，且析构对象仍能读取队列中的字符串。
- 3 个编译诊断：错误的 map 键类型、错误的 heap 元素类型、嵌套容器类型参数。均包含源码文件和中文错误。

用 `python -X utf8 scripts/check_typed_containers.py` 复现上述场景，也可指定 `example`、`behavior`、`errors`、`diagnostics`、`main_example` 分组。用例位于 [tests/containers](../tests/containers/)。本轮没有发现需要修复的实现问题，未运行全量回归或性能基准。

2026-09-26 完成计划 4.1 的 `deque<T>`：编译器按完整元素类型选择标量或复合元素 ABI，双端增删和下标直接调用 C++23 存储；复合元素参与 `deep_copy` 和循环回收。`scripts/build.ps1` 成功生成 `txc.exe`、标准库静态库与兼容指纹，成功后已清理 `build/`。[双端队列示例](../examples/deque.tx)和 `tests/containers/deque_behavior.tx` 均编译运行通过；后者核对四种标量、类字段、空队列、负下标、`any` 类型不符和复制引用环。`tests/containers/deque_wrong_type.tx` 在第 4 行静态报中文类型错误。本次仅做这些定向验证，尚未做跨平台或第 4 节总验收。

2026-09-26 完成计划 4.2：哈希映射复合值、只读 `entry<K,V>`、有序映射/集合按具体类型选择静态 ABI；结构体默认比较目标在编译期绑定，显式 `fn(K,K)->int` 可持有捕获闭包。`deep_copy` 对含复合值或比较器的容器先建立空副本再复制对象图，容器值及闭包纳入循环回收。`scripts/build.ps1` 构建成功，兼容指纹同步，成功后清理 `build/`。`tests/containers/map_extended.tx` 和 `ordered_behavior.tx` 编译运行，核对条目配对、原键修改、半开范围、闭包顺序、比较失败不改容器、`any` 恢复、对象图独立复制和自引用环。未执行全量或非 Windows 平台验证；4.6 的集合与算法联合边界验收仍单独推进。
