# 性能优化 07：格式化直接追加、静态融合与动态计划缓存

日期：2026-09-29。07.1～07.5 已实施，定向结构、行为与包兼容验收通过。起点为 `43b5eb0eb14fed6dd944aed5d69df3858424c442`，初始工作树干净；候选包含本项未提交改动。未更改公开语法、`.txh` 签名、格式说明及错误契约。

## 实现与范围

| 小点 | 实现及主要文件 |
| --- | --- |
| 07.1 | `src/stdlib/format_append.cpp` 直接把整数 `to_chars` 的栈缓冲、布尔常量和文本追加到同一个结果；基础字段不再先构造临时 `std::string`。`format_internal.hpp` 声明类型明确的内部入口。 |
| 07.2 | `codegen_format_plan.cpp/.hpp` 将字段后的字面量融合到该字段；`codegen_format.cpp` 将前导字面量、容量和 builder 创建合并到一次 `txrt_format_begin`，由 `format_static_abi.cpp/.hpp` 实现。容量累计静态字节和布尔上界，检查加法与 `max_size`；整数和运行时文本按需增长，复杂宽度不用于盲目预留。 |
| 07.3 | `common/format_spec.hpp` 共享专用路径适用条件；LLVM 为默认整数、布尔、原样文本生成 `plain_i64/bool/str/bytes` 直接调用，没有运行时类型选择。复杂 spec 继续调用原 `format_field_value`。原样文本仍验证 UTF-8，只省去无用的码点计数。 |
| 07.4 | `codegen_format.cpp` 在格式化之前按源码顺序求值，未引用的有副作用实参仍执行；复用 05 的借用判断和不稳定后缀持有规则。只以原样文本使用的字符串字面量直接传静态字节，不创建 TX 文本根。 |
| 07.5 | `codegen_format_constant.cpp`、`codegen_statement.cpp`、`codegen.hpp` 对整个函数中无赋值、解包写入、循环变量重用或同名遮蔽的局部模板和常量别名传播内容；存在无法满足证明条件的赋值时保留动态路径。`stdlib/format.cpp` 每次重新绑定实参，`format_plan_cache.cpp/.hpp` 缓存成功计划。 |

动态缓存为每线程 LRU，最多 32 项、256 KiB 容量预算，单模板不超过 4096 字节。预算计入计划和容器结构、字符串与片段数组的容量，对小字符串存储有保守重复计数；不包含分配器元数据和线程本身的成本。完整模板字节参与比较，包含嵌入 NUL；当前没有需要分离的第二种模式。缓存只保留字面量、字段名、spec 和转换字符，不保存调用参数、TX 根或借用地址。活动调用持有 `shared_ptr<const plan>`，即使重入淘汰当前缓存项也可继续读取；线程之间没有共享缓存和缓存锁，线程退出释放缓存。

冷调用沿用从左到右的扫描、字段解析、参数绑定和格式化顺序，只在整次格式化成功后保存计划。无效模板不缓存，避免将后面的格式错误提前到前面参数错误之前；命中时仍验证本次实参数量、类型、命名优先级和文本编码。超出容量的模板继续正常格式化。本项没有优化动态实参容器、浮点转换及复杂宽度/精度产生的临时字符串，也没有引入跨库内联。

## 构建身份与验证

基线标识及 DLL 哈希见 [baseline.json](baseline.json)，候选、程序哈希及每轮数据见 [samples_final.json](samples_final.json)。同一份 [format_paths.tx](format_paths.tx) SHA-256 为 `8ded463f7580f5390c796cd48997ca03a2ff865e2dfd9096e1d14eee2c593bf1`。

| 产物 | SHA-256 |
| --- | --- |
| 06 基线程序 | `390e80f5b0342f7fb27ded6def8d4066bce625d7c37aecb5e67fa6e6a9e06476` |
| 07 候选程序 | `8c9b211e1e1c01fb04c2dc798308d0d53d5ba4f3b5e999f2f67bf020a7ea58db` |
| 07 `tx/txc.exe` | `d99f64b6e9c2c765cc8a976ecd072931270f4594a6b7950261a2b9f506ae7a09` |
| 07 `tx/libtxstdlib.a` | `1911c05347d7a094517227ce7c3f5df60e7b549edc83478a172cd44e3baf77ae` |

静态格式化 C ABI 参数发生变化。当前兼容指纹为 `d88e07f71b2b5f7520fa9ecfe639c7568c6afd8ed951faca20969321e3573a9b`，已有 CMake 指纹覆盖修改的 ABI 头、公共 spec 头及 LLVM 源码。整体构建更新了编译器、标准库、`httpx_bridge.o`、`websocket_bridge.o` 和 `requests_bridge.o`，不能混用旧包。最终相关检查：

- `scripts/build.ps1 -Incremental` 成功；一次完整构建后调整了容量策略并消除新增初始化警告，增量重建及三个桥接对象重新归档成功。确认产物和定向检查后尝试清理临时 `build/`，但自动审批先后拦截带路径核验的命令和明确绝对路径的 `Remove-Item -Recurse`，返回 `blocked by policy`；目录保留，尚未完成清理，不影响已生成产物和本项验收结果。
- `python -X utf8 scripts/check_format_optimized.py` 通过。复用 `tests/containers/static_runtime.tx`，新增 `format_optimized.tx` 覆盖整数极值、布尔、空串、转义、命名字段、Unicode 宽度、精度、转换、源码实参顺序、未使用实参、可变模板和后续实参修改前值。
- 同一脚本运行 `format_cache.cpp`，覆盖缓存命中后的类型/缺参错误、错误优先级、非法 UTF-8、嵌入 NUL、容量淘汰、大模板绕过、活动计划在淘汰后继续持有、双线程并发，以及超大容量失败后的文本根释放。重入验证模拟持有活动计划期间发生嵌套缓存插入和淘汰；没有增加原格式化不支持的用户回调入口。
- IR 断言及计数见 [structure.json](structure.json)：基础格式没有动态实参数组/字典、装箱及通用字段调用；字面量实参使用 `plain_bytes`，无文本创建；不变局部模板使用静态入口，可变模板保留 `txrt_format_format`。既有 `check_static_runtime.py` 对字符串专用入口的断言同步更新。
- 对最终程序只读反汇编 `txrt_format_plain_i64`，确认直接调用 `append_format_integer`，没有调用通用字段格式化器；仍保留 builder 校验、实际扩容、尾部字节追加与异常处理。没有将扩容路径中的分配误计为每字段临时字符串。
- `python -X utf8 scripts/check_package_compatibility.py` 通过当前包和四种错配场景。未运行无关全量套件。

## 同轮性能

AMD Ryzen 7 6800H，Windows 11 10.0.26200。TX 沿用发布工具链默认 clang `-O3`；C++ 固定场景参考为 GCC 13.1.0，参数 `-std=c++23 -O3 -DNDEBUG`。每项 50,000 次，三种程序各预热一次，正序/倒序交替测 5 轮，校验值均为 `488890`。下表为程序内部耗时中位数，单位毫秒。

| 路径 | 06 | 07 | 06/07 | C++ 固定场景参考 | 07/参考 |
| --- | ---: | ---: | ---: | ---: | ---: |
| 字面量 `format_literal` | 10.721 | 5.316 | 2.02 | 0.431 | 12.33 |
| 局部不变模板 `format_local` | 51.714 | 4.878 | 10.60 | 0.466 | 10.47 |
| 函数参数传入且轮换模板 `format_dynamic` | 54.484 | 50.751 | 1.07 | 0.425 | 119.41 |

原综合负载中的 `format_dynamic` 实际是可证明不变的局部模板。本次用 `format_local` 单列它，并增加经函数参数传递的两种轮换模板，避免把静态传播的收益归给动态缓存。真正动态路径本轮耗时下降 6.85%，仍有动态数组、字典、装箱、根持有和跨函数调用成本，未确认缓存能带来稳定的大幅整体提速。

[format_reference.cpp](format_reference.cpp) 仅实现固定 ASCII 文本与整数拼接，不含完整格式语法、动态容器、GC、错误及 Unicode 契约；参考倍率不是等价公开 API 倍率，不能据此宣称已达到等价 C++ 低于 5 倍的目标。本项没有新的完整契约 C++ 性能对照。

未固定 CPU 亲和性和电源状态；全部异常样本保留。例如正式轮次的 06 局部模板为 50.917～70.741 ms，06 真正动态模板为 53.582～76.148 ms。初轮采样与最后的原生检查存在短暂重叠，保存在 [samples.json](samples.json) 供追溯，未用于上表；检查结束后只补测一次，正式记录为 `samples_final.json`。没有重复运行全量性能套件。

内存方面确认基础字段不产生临时字符串/字面量根，builder 仍有一次根分配及按需扩容；动态缓存增加受上限约束的每线程常驻内存，命中时不把参数延长到下一次调用。容量和持有边界已有定向检查，本轮未独立采样进程峰值工作集或编译/链接成本，不据此承诺峰值内存下降。所有计时均覆盖调用本身，未将构造结果移出循环；缓存预热成本未混入热调用中位数，冷模板仍执行原解析流程及计划构造。

## 重现

1. 从 `43b5eb0` 构建 06 工具链，将本目录基准材料带入，执行 `python -X utf8 benchmarks/performance_optimization_07_2026-09-29/measure.py --baseline`，保存程序、配套 DLL 和标识；脚本拒绝覆盖已存在的基线程序。
2. 使用当前源码执行 `scripts/build.ps1`，再运行 `python -X utf8 scripts/check_format_optimized.py` 和 `python -X utf8 scripts/check_package_compatibility.py`。
3. 确保检查及构建已经结束，执行 `python -X utf8 benchmarks/performance_optimization_07_2026-09-29/measure.py`。默认更新正式 `samples_final.json`；可用 `--output` 指定另一份样本文件名。脚本不会重建或覆盖旧程序。

旧可执行文件及配套 DLL 位于被 Git 忽略的 `tx_build/performance_07/baseline/`。版本化材料保存源代码、命令、身份和原始样本，重新克隆后需要按上述步骤重建旧程序。
