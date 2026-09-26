# 标准库第四部分进度与 4.3 暂停记录

记录时间：2026-09-26。此文件保存当时的暂停快照；同日用户重新要求推进第四部分，4.3 已接续完成，当前状态以[实施清单](standard_library_plan.md#4-集合排序与组合算法)和[类型化容器](typed_containers.md#heap-和-queue)为准。

## 已完成并验证

- 4.1 的 `deque<T>` 是本轮开始时已有的未提交工作，保留在工作区。本轮没有撤销或覆盖它。
- 4.2 的哈希映射复合值、`entries() -> vector<entry<K,V>>`、有序映射/集合的默认或显式比较器、有序遍历和半开区间范围查询已经实现，见[类型化容器](typed_containers.md)。`scripts/build.ps1` 曾完整成功，生成 `tx/txc.exe`、`tx/libtxstdlib.a` 和兼容清单并清理临时 `build/`。
- 4.2 定向用例 `tests/containers/map_extended.tx` 与 `ordered_behavior.tx` 均编译运行成功。前者覆盖结构体原键修改、复合映射值、条目快照、独立深复制和自引用环；后者覆盖有序顺序、范围、结构体键、捕获闭包、比较器失败后容器仍可用、`any` 显式恢复和深复制。没有运行全量测试或非 Windows 平台验收。

## 4.3 现有草稿

- 已在[语法说明](syntax.md)和[类型化容器](typed_containers.md)中标注 4.3 为未验收草案。草案选择 `queue<T>(vector<T>)` 批量入队、`heap<T>(vector<T>)` 批量建堆、显式 `fn(T,T)->int` 比较器及 `priority_entry<T>` 稳定优先级条目。
- 编译器草稿涉及 `src/common/common.hpp`、`src/frontend/sema/sema_container.cpp` / `sema_registration.cpp` / `sema_expression.cpp`、`src/backend/llvm/codegen_container.cpp` / `codegen_sequence_declarations.cpp` 等；运行时草稿涉及 `src/stdlib/typed_heap.hpp` / `typed_sequence.hpp` / `priority_entry.hpp`、`src/backend/cpp/sequence_extended_abi.cpp` / `priority_entry_abi.cpp`。`tests/containers/sequence_extended.tx` 已写好但**未用新工具链编译或运行**。
- 仅对 `sequence_extended_abi.cpp`、`priority_entry_abi.cpp`、既有 `heap_abi.cpp` 做了 C++23 `-fsyntax-only` 检查，三者当时通过。这不能证明链接、TX 代码生成、运行时语义或兼容指纹正确。

## 构建与工作区状态

- 分支：`master`；暂停时 HEAD：`879279357ecfc8aef054667650c12858fcade471`。本轮改动均未提交；原有 4.1 未提交文件也保留。不要把整个工作区误认为只含 4.3 草稿。
- 4.3 的 `scripts/build.ps1` 构建在用户要求暂停后主动中断，停在 Ninja 约 `275/334`；**本次构建未成功**。`build/` 保留供以后排查。`tx/` 中 `txc.exe`、`libtxstdlib.a` 和兼容清单的最后完整生成时间早于这次中断，仍对应上一次成功的 4.2 工具链，不能用它们宣称 4.3 已可用。
- 4.3 还缺少完整构建、`sequence_extended.tx` 的编译运行、空容器/NaN/比较器重入与错误路径、复合载荷 `deep_copy` 和循环回收的定向验证；草稿代码可能仍有编译或行为问题。4.4～4.6 尚未开始。

## 接续点

只有用户重新要求推进 4.3 时，先检查当前未提交改动与本文状态，再完成并校正草稿实现。随后运行 `pwsh -NoProfile -File scripts/build.ps1`，只做 4.3 相关的定向正常/失败/资源验证，同步文档和实际产物。满足计划 4.3 全部要求后再勾选；在此之前保持未勾选。
