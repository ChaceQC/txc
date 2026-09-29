# 性能优化 08：serde 直接编解码

日期：2026-09-29。08.1～08.5 已实施，定向结构、行为与包兼容验收通过。起点为 `2992f876d040c5c656dfcd0d3fac1ae7be1c85a4`，初始工作树干净；保存基线时仅新增本项目基准材料，编译器、运行时及标准库尚未修改。候选包含本项未提交改动。公开语法、`.txh` 签名、字段可变性、共享引用和错误契约保持。

## 实现与范围

| 小点 | 实现及主要文件 |
| --- | --- |
| 08.1 | `codegen_serde.cpp`、`serde_abi.hpp` 为现有静态类型描述绑定 `serde_codec`，直接选择整数、浮点、布尔、文本、bytes、option、结构体及各元素类型 vector 的处理入口；同时生成 JSON 名称顺序和 CBOR 编号顺序。复用字段索引、版本、默认值及未知字段策略，没有增加 schema JSON 缓存。 |
| 08.2 | `serde_writer.cpp`、`serde_struct_writer.cpp`、`serde_codecs.cpp`、`serde_vector.cpp` 按索引读取实际字段，直接写同一个有界输出缓冲。已知字段使用静态顺序；只有实际未知字段非空时才建立借用值指针的排序数组。JSON 数字/转义从原 writer 提取到 `json_scalar.cpp` 共享；CBOR 继续使用原最短整数/浮点及 UTF-8 规则。 |
| 08.3 | `serde_reader.cpp`、`serde_struct_reader.cpp` 复用 JSON 值/字符串解析及 CBOR 头部/值解析。字段按静态有序描述二分匹配，字段槽的名称指针兼作已出现标记，版本单独标记。缺失项按声明顺序填默认值/none，或报缺字段。 |
| 08.4 | 已解码字段、嵌套对象、向量和未知内容均由局部 RAII 值持有；完整字段成功后创建结构体，确认根值之后无尾随内容才返回。失败释放局部结果。文本、字符串向量及 bytes 取得持有存储，不保留输入文本的借用地址。 |
| 08.5 | 所有当前支持的基础类型、静态嵌套结构、option/vector/bytes 已接入。`unknown=preserve` 仅实际未知内容进入动态解析；`unknown=ignore` 使用 `json_skip.cpp`、`cbor_skip.cpp` 校验并跳过内容。未知对象的重复键、UTF-8、CBOR 最短编码/键序、深度和大小检查继续执行。 |

编码和解码均保留 serde 的逻辑嵌套计数，以及 JSON/CBOR 的格式深度，两种计数不能合并：结构体和 option 的原逻辑层级不一定等于 wire 层级。输出按追加检查 16 MiB 上限，输入先检查总字节数，CBOR 容器头在预留空间之前检查元素上限。复用原循环检测规则，不改变 GC 登记或执行门。

原实现先解析整棵输入再校验版本和字段。流式解码可能较早发现字段错误，所以公开入口在直接路径抛出 `runtime_failure` 后先销毁局部结果，再运行原路径裁定诊断；编码也保留相同的失败诊断回退。成功路径不调用该回退，失败路径仍可能创建动态树并重复解析。没有宣称非法输入也获得提速。原 `serde_value.cpp` 和树转换函数保留用于诊断兼容及对照。

现有 `dynamic_struct` 字段槽仍为 `std::any`。类型专用入口仍需从这种存储取出已绑定的类型，结构体身份/字段数检查也保留；没有实施计划 09 的固定字段布局。向量标量编码直接读取类型化存储，避免逐元素装成 `std::any`。解码标量词法结果、最终字段槽和真实未知值仍可使用 `std::any`，本项消除的是整棵中间映射/数组树。

## 结构与行为证据

`python -X utf8 scripts/check_serde_direct.py` 通过，详细结果见 [structure.json](structure.json)。只运行以下相关材料：

- 既有 `tests/serde/behavior.tx`、`tests/serde/module.tx`、`tests/formats/serde_migration.tx`：默认值、嵌套、option、对象向量、bytes、未知字段三策略、版本、迁移、环及输入限额。
- 新增 `tests/serde/direct.tx`：浮点/布尔/整数/bytes 向量，INT64_MIN/MAX，Unicode 和转义，JSON 与 CBOR 顺序不同的字段编号，输入释放后的字段寿命、默认值和字段修改。
- `tests/serde/direct_native.cpp` 直接调用新路径，避免公开入口回退掩盖问题。固定字段编码的 GC 分配计数增量为 **0**，解码为 **1**（最终结构体）；JSON/CBOR 忽略嵌套未知内容的解码增量也为 **1**。计数针对 TX 管理的节点/容器，不表示 C++ 堆分配为零。
- 原生检查比较新旧 wire 字节，覆盖未知键排在 `$schema` 之前、CBOR 23/24 编号边界、未知字段冲突/环、重复和转义后的同名字段、非法 UTF-8、整数溢出、尾随内容、限额，以及后续字段失败后嵌套结构的弱引用过期。19 个 JSON 和 8 个 CBOR 错误输入逐一比较原路径与公开新入口的错误类别、代码和完整消息。
- 忽略 CBOR 内容时也检查 `+0.0`/`-0.0` 的语义重复；两者规范字节不同，不能仅用相邻原始字节比较代替字典键相等规则。
- IR 检查确认字段内嵌静态 codec 指针和 JSON/CBOR 顺序表。候选程序的 [整数编码入口反汇编](integer_codec.asm) 显示从现有 any 槽取值后直接调用 `serde_writer::integer`，没有原来创建映射、插入键值或按所有 serde 类型逐项选择的路径；仍有 any 存储的类型确认、深度检查及失败处理。

没有运行无关全量套件。与本次相同的共享 JSON 标量编码逻辑由格式往返、转义和浮点用例覆盖，未重新测试未改动的完整 JSON 流状态机。

## 构建与兼容性

整体构建 `scripts/build.ps1 -Incremental` 成功，后续补充未知键边界与指纹覆盖后增量重建。编译器、标准库、`httpx_bridge.o`、`websocket_bridge.o`、`requests_bridge.o` 及 `package.compat` 已更新。`python -X utf8 scripts/check_package_compatibility.py` 通过当前包和四种错配场景。

确认产物和检查后尝试清理临时 `build/`：已核对其为仓库根目录下的普通目录，且没有受 Git 管理的文件。自动审批先后拒绝带路径核验的清理命令及明确绝对路径的 `Remove-Item -Recurse`，均返回 `blocked by policy`。本项构建目录因此保留，未完成临时目录清理；不影响 `tx/` 已生成产物及上述验收结果。

`serde_type` 从 32 增至 40 字节，`serde_field` 从 88 增至 96 字节，`serde_schema` 从 72 增至 88 字节；新增静态顺序表和函数入口描述。C++ 静态断言与 LLVM 常量同步，CMake 指纹除原 ABI 头和 LLVM 文件外，新增覆盖 `serde_direct.hpp`。当前 ABI 指纹为 `3db75708bcc52c7e19146f3976e6a9557ddee2840c38695cc374272220abbfb5`，旧编译器和新标准库不能混用。

| 产物 | SHA-256 |
| --- | --- |
| 07 基线程序 | `9e83170f4cccf19b10cfe73f4d2ac0fb139da3bfbd2c5599f4e3b671693d9a98` |
| 08 候选程序 | `efb6c189bbb791e57100c5eaf1aec7b01718dee70872bd4f309041e6607c7d5d` |
| 08 `tx/txc.exe` | `05361b98727eb23935676d4d79985c0d22f86c0e2acedac07929492746fb3434` |
| 08 `tx/libtxstdlib.a` | `1a54f4f24780159d3eb9e45a01643e6b0564b9938a313a5b3b92ab9b5f2fcd79` |

完整基线标识和配套 DLL 哈希见 [baseline.json](baseline.json)，候选和程序标识见 [samples.json](samples.json)，本项修改及新增的实现文件哈希见 [source_manifest.json](source_manifest.json)。本轮未独立测量编译/链接耗时、包体积或进程峰值工作集；不能由 TX 节点计数推断所有内存成本。新增常量描述占用静态空间，解析器键集合、排序数组、最终字段及输出缓冲仍按需分配。

## 同轮性能

AMD Ryzen 7 6800H，Windows 11 10.0.26200。同一份 [serde_paths.tx](serde_paths.tx) SHA-256 为 `ffcf80a1a1bc549650a25e3ebd8e0e8bfe05f70073cd773a6e0c5ba9285f2326`。JSON 两个项目保留综合负载原来的 schema、输入、5000 次往返和校验算法；另加相同输入的 CBOR 往返。TX 使用发布工具链默认 clang `-O3`，标准库为 GCC Release；C++ 对照使用 GCC 13.1.0、`-std=c++23 -O3 -DNDEBUG`。

构建及行为检查结束后，每种程序预热 1 次，正序/倒序交替测 5 轮，计时覆盖每次序列化、解码、结果读取及循环中的临时对象生命周期。短/长文本各轮校验值分别为 `40000` / `655000`。下表为内部耗时中位数，单位毫秒。

| 往返 | 07 | 08 | 07/08 | C++ 完整契约原生调用 | 08/C++ |
| --- | ---: | ---: | ---: | ---: | ---: |
| JSON 短文本 | 18.353 | 9.796 | 1.87 | 7.973 | 1.23 |
| JSON 长文本 | 36.415 | 26.824 | 1.36 | 24.411 | 1.10 |
| CBOR 短文本 | 13.713 | 5.733 | 2.39 | 3.427 | 1.67 |
| CBOR 长文本 | 19.447 | 10.653 | 1.83 | 7.087 | 1.50 |

[serde_contract.cpp](serde_contract.cpp) 使用相同字段、版本、拒绝未知字段策略，直接调用当前 C++ serde 实现，执行相同 JSON/CBOR 解析、校验和 GC 安全点；它省去 TX 外围句柄、字段 ABI 与诊断调用成本。这是完整契约的原生调用对照，与候选共享编解码器，不能据此断言已接近最优手写 C++，也不能把 1.10～1.67 当成对独立优化 C++ 库的结论。历史固定 JSON 拼接/搜索的 44～88 倍不用于本项验收。进一步的独立 C++ codec 对照仍未建立。

全部原始样本保留于 [samples.json](samples.json)。未固定 CPU 亲和性和电源状态，存在较慢样本：08 JSON 短文本第一正式轮为 16.024 ms，其余约 9.659～9.827 ms；07 CBOR 短文本有一轮为 25.726 ms，其余约 13.311～13.844 ms。没有删除异常值或重复跑全套。JSON 长文本仍有逐字节词法/转义、文本所有权和跨函数成本；失败诊断回退未独立计时。

## 重现与后续

1. 从 `2992f876` 构建 07 工具链，带入本目录的 `serde_paths.tx` 和 `measure.py`，运行 `python -X utf8 benchmarks/performance_optimization_08_2026-09-29/measure.py --baseline`；基线程序已存在时脚本拒绝覆盖。
2. 从候选源码构建，执行 `python -X utf8 scripts/check_serde_direct.py` 与 `python -X utf8 scripts/check_package_compatibility.py`。
3. 确保构建和检查结束，再执行 `python -X utf8 benchmarks/performance_optimization_08_2026-09-29/measure.py`。基线程序及 DLL 保留在被 Git 忽略的 `tx_build/performance_08/baseline/`；克隆仓库后须重建。

计划 09 可以在现有字段索引和 codec 的边界上替换 `dynamic_struct` 存储访问，并同步解码对象提交。当前不再需要为固定结构体保留中间 wire 树；字段 any 存储、原生调用边界、输出缓冲及实际未知内容仍是后续成本。
