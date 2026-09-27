# 调用属性与安全借用：首批验收

日期：2026-09-27。平台：Windows x64。基线提交：`fc03e008bda755181ded064561e032de271dfc0f`。

本轮交付调用属性、安全借用、标量字典键入口与相关叶子错误路径。编译器、标准库及 HTTP/WebSocket/requests 的预编译 TX 桥接对象已重建并写入 `tx/`。这是优化清单的第一批实现，不代表整个清单完成。

## 实现范围

- 语义分析在调用完成类型和重载绑定后记录分配、用户回调、用户析构、参数修改、参数保存及持有/借用关系；未知入口默认保守。LLVM 根据这些属性及后续实参的稳定性选择借用或持有。
- 基础类型 `map/set/queue/deque` 操作不再无条件克隆接收者；`vector`、容器长度、字典 `get/contains` 和 `cancel.status` 使用同一套证明依据。命名实参仍按源码顺序求值，再放入对应形参位置。
- 对象键索引不再被泛化成稳定表达式，可能回调或析构用户对象的操作保留持有。对象向量修改同样保守，避免析构期间重绑定字段使底层存储失效。
- 字典 `get/contains/remove` 的 `int/float/bool` 键使用九个类型专用入口及异构哈希查找，无需临时 `std::any` 键句柄；字典实际存储仍按键类型区分。NaN 不命中，正负零相等，缺失键返回及混合键行为保持。
- 基础类型 map/set/queue/deque 由具体存储类型选择叶子错误包装；对象值、自定义键和比较器保持待传播错误检查。字典删除继续检查用户析构错误。
- `cancel.status` 借用令牌及其状态引用，保留状态锁、截止时间和状态优先级；`cancel.wait` 继续独立持有状态引用。

## 结构和语义验证

运行 `python -X utf8 scripts/check_call_borrowing.py`，两组验证通过：

1. 行为：接收者字段在后续实参中重绑定、共享容器、命名实参、混合键、NaN 与正负零、缺失键、空队列、取消前后状态、对象键回调错误、删除动态字典值产生的析构错误，以及析构期间重绑定对象向量。
2. LLVM IR：`query_map/query_set/query_queue/query_length/query_dictionary/query_named/query_cancel` 七个函数中没有 `txrt_value_clone`、`txrt_value_box_*` 或临时 `txrt_value_release` 调用；`rebind_map/rebind_dictionary/callback_index` 仍有必要的持有；九个标量键入口均被实际调用。

运行 `python -X utf8 scripts/check_typed_containers.py behavior errors`，8/8 通过，涵盖跨模块、字段、求值顺序、rehash、快照、对象图复制，以及缺失 map 键、空 heap/queue、NaN 键、从 any 恢复错误类型时的诊断和析构清理。

`git diff --check` 通过。没有运行全量测试。

## 同源码性能对照

源码：[call_borrowing.tx](call_borrowing.tx)。在改动前先用原有成品编译基线程序，改动后用新成品编译同一源码。最终统计在构建和语义测试完成后按“旧程序、新程序”交替运行三轮，取中位数。两组程序均按正常 Release 工具链生成，不修改优化开关。

计时使用 `time.monotonic_micros()`，表中折算成毫秒。字典、map、set 和取消状态各执行 100 万次查询；queue 执行 10 万次 push、front 和 pop。各轮校验值一致：字典 `1000000`，map `3000000`，set `1000000`，queue `5000050000`，cancel `0`。

| 负载 | 修改前中位数（ms） | 修改后中位数（ms） | 前/后耗时比 |
|---|---:|---:|---:|
| dictionary.contains(int) | 258.245 | 16.073 | 16.07 |
| map<int,int> 索引读取 | 142.933 | 4.005 | 35.69 |
| set<int>.contains | 154.546 | 16.277 | 9.49 |
| queue<int> push/front/pop | 47.758 | 5.665 | 8.43 |
| cancel.status | 157.333 | 18.570 | 8.47 |

最终三轮原始计时，单位微秒：

| 版本/轮次 | dictionary | map | set | queue | cancel |
|---|---:|---:|---:|---:|---:|
| 修改前 1 | 263546 | 142933 | 154546 | 49166 | 165421 |
| 修改后 1 | 16358 | 4005 | 16725 | 5665 | 18697 |
| 修改前 2 | 256359 | 142567 | 154360 | 46386 | 154678 |
| 修改后 2 | 16073 | 3991 | 16169 | 5696 | 18570 |
| 修改前 3 | 258245 | 145057 | 160240 | 47758 | 157333 |
| 修改后 3 | 15971 | 4012 | 16277 | 5344 | 18201 |

这是 TX 改动前后的局部负载对照，包含消除临时句柄、键装箱及叶子路径带来的共同收益；没有逐项拆分贡献，也没有据此推算相对 C++、Python 或 Java 的倍数。固定键查询不代表大型字典或所有容器负载。

## 成品与证据

构建命令：`scripts/build.ps1 -Incremental`。验证产物后尝试清理临时 `build/`，清理命令已包含绝对路径、工作区范围和重解析点校验，但执行工具返回 `blocked by policy`，因此目录保留。生成的行为程序与 IR 位于 `tx_build/call_borrowing_checks/`；前后基准程序保留在 `tx_build/call_borrowing_before.exe` 和 `tx_build/call_borrowing_after.exe`。

| 文件 | SHA-256 |
|---|---|
| benchmarks/call_borrowing.tx | `6e77763d77f435006016519a717723d14337336312d4589a8836345d9882bfa8` |
| tx/txc.exe | `8a91a2a006704edd2ee215e83931c5f6b7900e2b198aba6b624b89ea0ebff5ee` |
| tx/libtxstdlib.a | `cf64c7fbf77734d037d9e32250101489c01daaba9ef3fb8d871c56e842760b18` |
| tx_build/call_borrowing_before.exe | `595d55d84a02376a0cf6a12c9650afbbaab32a0a1e0722c33f0177201126e422` |
| tx_build/call_borrowing_after.exe | `e23099af2ba28c1ab6025e8f2ea80975850ad78e0e34d441b99cec0e05147639` |

ABI 指纹：`5118013904eb9577a676ef3a35da516ee20ae37ec98b14a9e64ac244085a924f`。`tx/package.compat` 中编译器、标准库哈希与实际文件一致。

## 尚未完成

类型化容器本体仍保留现有动态句柄表示，操作入口仍需恢复内部类型；本轮首先消除了安全调用的临时句柄。向量头缓存与循环范围证明、静态 serde、字面量 format、常量配置专用化、decimal 表示、统计内核、文件缓冲、regex 工作区和网络会话仍按[后续顺序](../docs/runtime_optimization.md#后续工作顺序)推进。旧的指令字符串错误分类也尚未整体替换。
