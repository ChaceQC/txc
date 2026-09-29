# 性能优化 05：不可变文本共享与安全借用

日期：2026-09-29。05.1～05.5 已实施，定向结构、行为、包兼容和同轮性能检查通过。工作树仍含先前未提交的 02～04 改动；本项从该状态继续，未改公开语法或 `.txh` 接口。

## 实现与所有权

首次创建文本时，一次分配建立线程本地根和独立计数的内容。`str` 克隆只分配新根并增加内容的原子引用，不复制字节。各根仅在创建线程的清理链表中登记和注销；原根注销后，只要其他根或容器还持有内容，内容及其存储继续存活。最后一个引用可以在另一线程释放。`text_reference` 仍占一个指针槽位，但持有内容而非根；槽位中的借用指针由内部标记区分，不能作为根释放。目标线程需要 TX 根时，用该借用指针克隆并在本地登记。对应实现为 `runtime_abi_internal.hpp`、`runtime_abi.cpp`、`text_reference.hpp/.cpp`，线程与任务结果边界也改用新根类型。

格式化静态路径使用独占 builder，所有片段追加完毕后由 `txrt_format_finish` 发布。拼接、解码等先在独占 `std::string` 中完成，再建立已发布的文本根；发布后只通过只读视图访问。原生边界原先直接把 `void*` 转为 `std::string*` 的读点，已按模块迁移到 `text_value`；格式化写点改用 `text_builder`。迁移覆盖基础运算、容器/向量、解析、格式化、编码、serde、文件与路径、网络与 TLS、正则、时间等 ABI；`src/backend/cpp` 和 `src/stdlib` 中已无旧的文本句柄强转读写点。

语义分析为 parse、内存 encoding、bytes 的明确只读入口标注借用属性；代码生成仅在后续实参稳定、无用户回调且不保存参数时借用字符串。静态格式化的文本参数按同样条件借用；字典字符串查询沿用既有借用路径。向量槽位借用保留拥有者寿命，不能把借用地址交给会保存它的入口。具体规则见[跨线程契约](../../docs/send_sync.md)。

为控制短文本的首次创建成本，最终实现把原根和内容合为一次分配，克隆仍只创建本地根；以下数据均来自最终实现。

## 构建身份与结构核对

实施时 HEAD 为 `e5068a35ac660647cf789971e7bd3c8ce84a3dd2`，候选包含工作树改动。`benchmarks/diverse_performance.tx` SHA-256 为 `2e940bff73a793788cf78c37c6a34c98cab38fb1c1aaacc184042cf89f13b499`。最终产物：

| 产物 | SHA-256 |
| --- | --- |
| 04 对照 `tx_build/diverse_performance_04.exe` | `6258ead78ed49f71fbfcec101a9ea1304a8036e680feed08d4f9bd871fdcbe62` |
| 05 `tx_build/diverse_performance_05.exe` | `6b4d1877697c2ac503b995b3fd523fb87cb297be3ef69cc0de42188ee8ed0192` |
| 05 `tx/txc.exe` | `b9982ab9c484a9f22cd2b4cffcb15dc3a01b64ca89c0a6838c78e50d1430b593` |
| 05 `tx/libtxstdlib.a` | `dcc707d466f45a732fa0b47f787e0d012c7149ecd54338e4d2881b46493afb89` |

包兼容 ABI 指纹为 `34950dd72747057350ac8d14b2d43fbff4c8023cc696769c244ac3a4167e3df3`。完整构建更新了编译器、标准库、三个预编译 TX 桥接对象和发布目录；兼容检查覆盖当前包及四种错配情形。

原生检查确认短、长文本克隆得到不同根但同一个内容地址；根释放后容器引用仍可读取，工作线程能从内容引用创建本地根，并由另一线程完成最后释放。最终 IR 中，`bench_dictionary_keys` 无 `txrt_str_clone`；`bench_parse_paths` 的两个 `txrt_parse_try_parse_int` 调用直接使用向量槽位借用指针，函数内仍有一次用于其他操作的 clone；`bench_format_paths` 保留一次其他用途的 clone，并在静态格式化末尾调用一次 `txrt_format_finish`。没有把所有文本持有一概取消。

## 定向行为与性能

`scripts/build.ps1 -Incremental` 和最终 `scripts/build.ps1` 成功，临时 `build/` 已清理；`tests/stdlib/text_sharing.cpp` 与既有 `handle_lifecycle.cpp` 原生编译运行通过，覆盖根链表、共享内容、已发布内容禁止追加、容器别名、线程退出/最后释放、错误清理和析构读取。TX 定向用例 `tests/containers/behavior.tx`、`tests/stdlib/send_sync_move.tx`、`tests/stdlib/parse_errors.tx`、`tests/bytes_file_stream/encodings.tx` 均通过。`python -X utf8 scripts/check_package_compatibility.py` 通过。

从同一份 TX 基准源码分别编译 04/05 程序，各预热一次，然后按 04/05、05/04 交替测五轮；每项校验值一致。下表是程序内部耗时中位数，单位毫秒，原始样本见 [samples.json](samples.json)。

| 负载 | 04 | 05 | 04/05 |
| --- | ---: | ---: | ---: |
| format_literal | 12.627 | 9.832 | 1.284 |
| format_dynamic | 56.739 | 52.577 | 1.079 |
| encoding_literal | 15.159 | 13.187 | 1.150 |
| encoding_dynamic | 21.377 | 14.364 | 1.488 |
| parse_valid | 73.432 | 63.757 | 1.152 |
| parse_invalid | 82.723 | 73.855 | 1.120 |
| serde_short_text | 19.088 | 18.934 | 1.008 |
| serde_long_text | 36.446 | 36.046 | 1.011 |

另以 `g++ -std=c++23 -O3 -DNDEBUG` 重建 C++ 对照，`--contract-check` 通过；与 05 TX 各预热一次并交替测五轮，样本和源码哈希见 [cpp_samples.json](cpp_samples.json)。parse 使用完整结果契约、serde 使用等价 schema；format 和 encoding 的 C++ 实现仍是固定场景算法参考，倍率不表示公开接口完全等价。

| 负载 | C++ 中位数 ms | 同轮 TX 中位数 ms | TX/C++ |
| --- | ---: | ---: | ---: |
| format_literal | 0.696 | 9.708 | 13.95 |
| format_dynamic | 1.011 | 52.152 | 51.58 |
| encoding_literal | 2.828 | 13.479 | 4.77 |
| encoding_dynamic | 2.734 | 14.776 | 5.40 |
| parse_valid | 1.702 | 63.603 | 37.37 |
| parse_invalid | 6.193 | 71.357 | 11.52 |
| serde_short_text | 0.917 | 18.973 | 20.69 |
| serde_long_text | 4.199 | 36.270 | 8.64 |

综合负载进程工作集按 2 ms 间隔抽样，两次 04 样本峰值为 8.54/8.55 MiB，两次 05 为 8.49/8.50 MiB；抽样精度不足以据此承诺稳定内存收益。编译成本未做独立对照。未新增公开 ABI 参数或语法，但内部文本布局与 ABI 指纹已改变，不能混用旧包。

外部 mini-filesystem 的 `solution_benchmark.tx` 用最终 05 工具链编译，对 `fanout-2000`、`linklong-2000`、`moves-2000` 三份正式输入运行，输出分别与 `.out` 相符。单次内部计时为 19.866/48.053/6.191 ms，受运行时波动影响；没有留存同源码的 04 程序，此处只验证对应输入的行为，不计算前后加速比。

05 消除了克隆长文本的字节复制，并保持短文本相关负载在本轮无明显整体回退。解析结果对象、格式化字段临时文本及 serde 中间树仍存在，分别由 06～08 继续处理；本项不把这些后续目标记为已完成。
