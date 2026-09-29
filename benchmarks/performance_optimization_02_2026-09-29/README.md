# 性能计划 02：GC 安全点实施与定向验收

日期：2026-09-29。基线源码提交为 `e5068a35ac660647cf789971e7bd3c8ce84a3dd2`，本项代码和报告尚未另行提交。02.1～02.4 已实施并通过定向行为检查；02.5 评估后暂不拆分登记表。**整项性能验收暂未通过**：map 对照有稳定的约 0.4 ms 回退，归因尚未确定。

## 改动与构建身份

- 安全点先检查 `collecting`、执行深度和 64 次分配的最小阈值，再读取原子登记数量提示。提示为零或尚未达到按数量计算的阈值时直接返回；需要回收时仍在登记表锁内核对真实数量并取得对象图快照。`collection_interval` 保持 64，显式 `collect_cycles()` 和退出回收不使用自动轮询的提前返回。
- 新的 `txrt_gc_safepoint_context(ptr %tx_context)` 使用生成程序已有的运行时上下文。执行深度移入上下文，嵌套任务换上下文时继承深度；作用域析构恢复进入前的深度。原无参入口保留给直接使用该 ABI 的现有程序。
- 安全点仍按可能执行用户 `deinit` 的操作处理。调用包装返回传入上下文中的待传播错误；异常时仍按原错误类别记录，不清除既有错误。
- 登记表分区留待登记和快照被独立证实仍为热点时实施。本项四个受益查询负载的热循环不再因安全点访问登记表；按 owner 的快照复制与全局执行门仍保持原实现，未对循环回收本身宣称提速。

| 产物或源码 | 改动前 SHA-256 | 本项 SHA-256 |
| --- | --- | --- |
| `txc.exe` | `58ff0459fcee33b82a2a887f73109c3a71143cd10135b6ac23edb32161c771d3` | `45a159d204f0e6d621b7fcfdcef1cea761d029bc0561dbd4bdbe29f9ae8d49e8` |
| `libtxstdlib.a` | `ec5f0804addcca0997178d951ab0e6e5a108fc12481a38bf4994eca6bb3519c1` | `83ee01f706b0ce035c679478b227c4a68ba21699bc82d1f26cb3a7c700ee89f1` |
| `benchmarks/call_borrowing.tx` | `6e77763d77f435006016519a717723d14337336312d4589a8836345d9882bfa8` | 未变 |

本项修改了 `cycle_gc.cpp/.hpp`、`runtime_context.hpp`、`runtime_abi_internal.hpp`、`error_abi.cpp/.hpp`、`task_runtime.cpp`、`codegen.cpp`，新增一个原生边界用例。完整执行 `scripts/build.ps1`，重新构建编译器、静态库和 `httpx_bridge`、`websocket_bridge`、`requests_bridge` 三个 TX 对象；兼容指纹为 `cf15758967995793206a27e8567f0e21e7c14ce862211a54e93c273905e4944d`。本项新增一个进程级原子计数和每个运行时上下文中的执行深度，移除独立的执行深度 TLS；未测量内存峰值或与基线可比的编译耗时。

## 结构与行为

`emit-llvm` 输出中的安全点调用为 `call i32 @txrt_gc_safepoint_context(ptr %tx_context)`。`txrt_gc_safepoint_context` 的优化后机器码先比较上下文中的 `collecting`、`concurrent_depth` 和分配计数，再读取原子数量提示；这些条件不满足时跳到直接读取上下文错误类别并返回的分支。该快速分支没有登记表加锁或 TLS 查询；提示触发后仍有登记表锁、真实数量检查和完整回收路径。map 计时循环内部既有和新产物均只调用 `txrt_map_read_i64_i64`，没有安全点；map 循环及该读取入口的热路径指令序列相同。

下列检查均通过：

| 检查 | 覆盖范围 |
| --- | --- |
| `scripts/check_runtime_context.py` | 传参边界、待传播错误、闭包循环和 `deinit` 复活 |
| `scripts/check_call_borrowing.py` | 借用失效、查询语义及生成 IR |
| `scripts/check_concurrency_errors.py` | `thread.join`、`task.wait` 原始错误和后续任务 |
| `tests/stdlib/gc_safepoint_context.cpp` | 显式上下文中的既有和新异常错误、达到 64 次且无登记节点、嵌套执行门和异常退出 |
| `examples/cycle_collection.tx`、`tests/stdlib/task_graph_lifecycle.tx` | 循环对象可达性、退出回收、含循环对象的嵌套任务及等待 |
| `scripts/check_package_compatibility.py` | 当前包及四种 ABI / 文件错配 |

## 同轮性能

旧程序为 `tx_build/call_borrowing_20260929.exe`，SHA-256 `4b1fbef79a4ebf4d6b92a7bb615dad0f4279823b96193c8bd03872c8719db577`；新程序 SHA-256 为 `41530dc71586c5219aca7a81862ae3b985a5a2c1c711ed794f2fbda7d340b61b`。两者使用同一份未变的 TX 基准源码，所有项目的校验值一致。每个程序预热一次，按旧/新、新/旧交替测 7 轮；表中为程序内部计时的中位数，单位 ms。逐轮样本、顺序、时间和哈希见 [7 轮原始记录](call_borrowing_7_rounds.json)。

| 专项 | 旧程序 | 新程序 | 旧/新 |
| --- | ---: | ---: | ---: |
| dictionary | 37.230 | 5.073 | 7.34 |
| set | 37.520 | 4.016 | 9.34 |
| queue | 11.652 | 1.785 | 6.53 |
| cancel | 39.015 | 6.161 | 6.33 |
| map | 3.426 | 3.748 | 0.91 |

对 map 又固定单个 CPU 核交替测 5 轮，中位数为旧 3.264 ms、新 3.734 ms；原始记录见 [固定核心记录](call_borrowing_affinity_5_rounds.json)。map 热循环没有新增工作，但这两组计时仍显示回退；代码地址、内存布局或处理器状态的影响尚未分离，不能将其解释成已证明的 GC 算法回退，也不能称 map 已无退化。本专项没有语义相同的 C++ 成对程序，因此不计算等价 C++ 倍率。后续若要关闭 02 性能验收，需先定位 map 差值并复测，不能只用四个受益项的提升代替该门槛。
