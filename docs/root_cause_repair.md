# 2026-09-30 根因报告修复

依据 `benchmarks/performance_retest_2026-09-30_static_execution/root_cause_analysis.md`。
不改语言语法、静态类型规则和公开标准库接口；保留原始诊断作为修复前证据。

局部类只在对象整体不被观察、构造/析构不泄漏 self 时取消共享对象与 GC 登记；
保留初始化位置、字段默认值、正常/异常清理和析构顺序。可复活或存在别名的类维持共享表示。

## 逐项处理

| 原报告项 | 本次处理 | 正确性边界 |
|---|---|---|
| 1：同名 guard 阻止栈存储 | 局部引用、赋值目标和解包记录 SSA 绑定身份，局部表示检查使用绑定对应的声明 | 内外层同名变量独立判断；wait、整体传递和真正重绑定仍回退 |
| 2：默认 heap 被标记 captured | 在 SSA 上检查全部来源是否为无自定义比较器的标量堆；区分接收者修改、元素包含边和对外保存；清理阶段沿用已证明的比较器事实 | 未知来源、参数堆、自定义比较器、混合重绑定仍使用保守效果；真实逃逸不启用局部借用 |
| 3：snapshot iterator 回退 | 完整识别 iterator/option/result；为标量内建操作建立效果、独立结果及内容别名；清理阶段识别标量迭代器不执行用户析构 | snapshot 与源容器独立，live 保留源容器引用；close 后访问、空 option、共享游标保留原检查；通用 sum 的分配和 GC 登记效果仍保守记录 |
| 4：LTO 归因 | 延续默认 ThinLTO，不通过关闭 LTO 掩盖优化适用性缺陷 | 本机库和 bitcode 库一起构建；不把跨轮扰动解释为因果 |
| 5：serde 弱特化覆盖强特化 | 优先选择已有双字段整对象编解码器，其余符合预算的 schema 使用生成的字段函数 | 保留版本、重复/未知字段、类型检查和失败清理 |
| 6：短生命周期文本/bytes | 只被 len 消费的局部及临时结果可只生成长度；简单格式化直接调用类型化长度操作，UTF-8 编解码不分配输出内容 | 保留源码顺序、未使用实参副作用、Unicode 标量长度、BOM 和非法编码错误；其他编码仍执行真实转换；复杂格式和整体使用回退 |
| 6：deinit | 对只操作字段、最多一个用户析构器、self 不逃逸的局部类使用栈上存储；生成直接的析构调用；引用字段和数组字段使用已有静态视图 | 保留字段默认值、初始化失败清理和原异常；别名、复活、多析构器及不支持的字段初始化回退到共享对象 |
| 6：deep_copy | 根类型直接选择复制种类；身份表内联保存定型节点句柄，用连续哈希表避免逐节点分配；扩容无异常移动，字典预留容量 | 保留共享边、循环、资源复制限制和复制失败时不执行半成品 deinit 的契约 |
| 6：cycle_gc | 复用有容量上限的图工作区和连续身份索引，析构复活后的重新扫描复用容量 | 工作区由已有 FLS 上下文释放；退出扫描立即撤销强引用；超过 8192 节点容量或 65536 边容量就释放缓存；复活仍重新判定可达性 |
| 6：property/parameterized | 同步批次只设置和恢复一次错误传播模式，复用已绑定目标与环境 | 每次回调仍检查和消费自己的错误；零次调用不强制绑定，反例缩减和失败报告保留 |
| 7：TLS | 复用线程安全 PSA CSPRNG；每个身份最多缓存四份独占使用的已解析证书/私钥；启用依赖已有的 Everest Curve25519 ECDH 实现 | 仍用 Mbed TLS，同一密码套件和曲线；随机输出不缓存；证书链、主机名、有效期、用途、ALPN 逐连接验证；关闭身份清除闲置密钥，活动流保留独立状态直到关闭 |
| 7：SQLite pool | 复用经授权器和修改计数证明没有会话副作用的只读连接；每次借用使用独立状态，并重新绑定超时和操作回调 | 写入、DDL、临时表、未知 pragma 或清理失败仍关闭；事务回滚、旧连接/语句/游标别名失效和跨线程限制保留 |
| 7：dictionary / 字符串转换 | 数字键查询直接使用缓存字典引用，数字键读取不装箱；稳定键表达式和字符串转数值可借用输入 | 混合键类型区分、NaN、缺键、转换失败、UTF-8 和实参顺序保留 |
| 7：随机数 | ThinLTO 内联已有整数随机数 ABI，使固定上下界进入优化；继续使用原 mt19937_64 和 uniform_int_distribution | 不改变随机序列、种子或无偏采样规则；动态非法上下界和首次随机源初始化仍可失败 |
| 7：外部题 | 将同一编译器改动用于原始 mini-filesystem / not-yet-on-stage 计时解，对代表性正式输入复测并核对答案 | 不修改外部题目源码、数据或算法，不把算法差异算成编译器收益 |

局部长度在原初始化位置计算，不能延迟到后续 len 时重新读取可变化的输入。
原始对象一旦整体使用、传参、别名或重绑定，就维持正常拥有存储。
普通格式化长度特化只接受预算内的 int/bool/str 普通规格；其他模板继续完整格式化。

## 验证

使用 `scripts/check_root_cause_repair.py` 同时检查运行语义、生成 IR 与分析摘要。
用例包括不同作用域同名 guard、需 wait 的遮蔽 guard、默认/自定义/重绑定/逃逸堆、
比较器异常、快照独立性、共享游标、关闭后访问、元素保存关系、option 返回别名、
双字段 JSON/CBOR、重复/未知字段、长度投影和完整物化回退。

已通过：

- `scripts/check_root_cause_repair.py`：新增两组行为、效果摘要与 IR 检查。
- `scripts/check_static_execution.py`：原生 record、guard 正常及异常清理、静态格式、宽 schema、正则投影及格式绑定缓存。
- `scripts/check_program_analysis.py` 的 `check_analysis()`：跨模块递归、保存/返回别名、异常和循环中的移动生命周期，ThinLTO/普通链接语义一致。
- `tests/performance_12_14/iterators.tx`、`tests/serde/direct.tx`、`tests/serde/behavior.tx`、`tests/performance_completion_gc.tx`、`tests/static_borrowing_lifetime.tx`：活迭代器、空/关闭迭代器、schema 回退、图复制和析构复活、借用生命周期。
- `scripts/check_root_cause_lifecycle.py`：局部类正常及失败初始化清理、原异常保护、逃逸回退、图身份、循环和复活、数字键与字符串转换 IR、SQLite 物理复用与旧租约失效、TLS 私钥状态独占和关闭清理。
- `scripts/check_tls_stream.py`：正常双向 TLS、主机名/信任/客户端证书/ALPN 拒绝，以及八个随机数线程并行时的握手与线程退出。
- `scripts/check_call_effects.py`：标量查询和 pop 的借用/安全点消除，以及对象回调/析构继续保留安全点和错误。

原始 features/diverse/concurrency 负载与留存旧二进制做 1 轮预热、3 轮交替采样，
每轮核对全部输出校验值。结果见 `benchmarks/root_cause_repair_2026-09-30/README.md`。
续接的对象、图、回调、数据库、TLS、随机数及字典样本由
`scripts/measure_root_cause_remaining.py` 记录，外部题由
`scripts/measure_root_cause_external.py` 记录。三组记录分别保存源码、程序和输入指纹；
没有重复运行全部性能套件，也没有把历史跨轮变化当作单点因果。

## 最终续接核验

恢复 TLS 对照期间临时关闭的 Everest 配置，重建原生和 ThinLTO 标准库；
同步本机生命周期检查所需的 Everest 头文件路径。
最终包重新通过 `check_root_cause_lifecycle.py`、`check_tls_stream.py` 和
`check_root_cause_repair.py`，包括 TLS 随机数并发线程退出。
配置选择数据保存在 `tls_selection.json`；最终包用 `measure_tls_repair.py --final`
单独采样到 `tls_final.json`，不覆盖关闭 Everest 的对照记录。
最终 TLS 配对中位数为 110.485→105.933 ms，网络波动明显，不承诺固定收益。
两批性能表和外部题结果已汇总到基准目录 README；外部题部分持平或略慢，
现有证据不支持声称全部性能差距已消除。
