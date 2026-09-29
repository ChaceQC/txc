# 定向验证记录

2026-09-29 本地 Windows，GCC 13.1、发布 clang 23.1.2。没有运行全仓库行为套件。

## 解析及 UTF-8 修订

`scripts/check_performance_remaining.py` 在解析/UTF-8 修订后执行并通过以下步骤：

- 新增 `performance_remaining/behavior.tx`：map 空/单/多元素转换、别名、缺失键、UTF-8 长度和 BOM。
- `stdlib/parse_scalar.tx`、`containers/map_extended.tx`、`bytes_file_stream/encodings.tx`、`performance_15_16/libraries.tx`。
- 原生 `performance_remaining/native.cpp`：全部 Unicode 标量的拼接输入；所有双字节组合在短输入、长输入首部和末尾；非法编码、截断和边界位置；30000 个固定种子的不同长度字节串与原逐码点规则差分；传入上下文与 TLS 上下文不同且已有错误时的解析返回状态。
- 既有 `stdlib/parse_scalar.cpp`：2～36 进制、最大/最小整数、溢出后非法字符的错误优先级、浮点非有限值、成功和失败零分配。
- 优化后 IR：局部解析调用 `txrt_parse_int_scalar_context`，不出现结果物化、clone 或 GC 安全点。
- `scripts/check_parse_errors.py`：6 个解析及异常场景，包括跨模块、嵌套异常、析构、未捕获错误和示例。

上述步骤执行完成后，原脚本的最终桥接对象字节比较失败。原因是 MinGW `ar p` 的 stdout 将二进制中的 LF 转为 CRLF（例如 httpx 对象从 44953 B 变成 44972 B），不是桥接内容不一致。检查改为在独立目录执行 `ar x` 后读原始二进制；单独重跑包检查通过。随后数字哈希修订只重跑受影响的容器行为和最终包验证，不重复解析、UTF-8 穷举。

## 最终数字哈希修订

`python -X utf8 scripts/check_performance_remaining.py --containers-only` 全部通过，结构化结果见 [checks.json](checks.json) 和 [container_checks.json](container_checks.json)：

- 更新的新用例额外验证 NaN 仍报错，且原值没有被破坏。
- `containers/map_extended.tx`。
- `containers/behavior.tx` 的 `maps / fields / order` 均为 true，覆盖回调期间容器变化、跨模块别名、不同基础键和值、any 恢复、快照、深拷贝及 set 正负零。
- 当前包及接口、ABI、编译器、静态库 4 种错配检查通过。
- 三个 TX 桥接对象各恰好出现一次，提取二进制与本次 build 内对象逐字节一致。

最后分离批量核、优化 ASCII 与恢复短输入直接扫描后，执行 `--text-only` 检查通过，见 [text_checks.json](text_checks.json) 和最终 [checks.json](checks.json)。该模式包含新增行为、七种编码、15～16 原有库边界，以及重新编译运行的原生 UTF-8 差分和上下文用例。

检查脚本默认仍提供完整定向流程；`--containers-only`、`--text-only` 和 `--package-only` 供仅变化相应部分时运行。测试结果按实际执行范围记录。

## 构建中断和恢复

首次构建期间 E 盘掉线，发布包未完成；用户恢复设备后重新执行完整打包。掉线期间未清理 build 或将部分包作为可用产物。重复构建暴露了依赖归档重复合并问题，修正后连续构建的库体积和成员数相同，见 [build_repeat.json](build_repeat.json)。最终库、DLL、ABI 指纹和桥接对象以本轮最终记录为准。

最终 build 清理被自动审批以 `blocked by policy` 拒绝，未执行；构建目录保留。此拒绝不影响已完成的构建、行为检查及性能采样。
