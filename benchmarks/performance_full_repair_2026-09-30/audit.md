# 完整差距逐项核对

原报告去重 51 个热点均对应到本轮配对样本，单位 ms。主样本旧/新各一次预热、三次交替采样；network/diverse/format 使用后续五轮校准；HTTP 使用最终封包后三轮。
倍率是同轮旧程序/新程序，不是当前 TX/C++ 倍率。校验值及外部题输出一致才进入结果。
保留现有语义的条目不代表性能差距消失；表内明确记录没有新增算法或仍有差距的项目。

| 热点 | 旧 ms | 新 ms | 旧/新 | 样本 | 代码定位与处置 |
| --- | ---: | ---: | ---: | --- | --- |
| mutex_uncontended | 88.850 | 21.294 | 4.17× | [concurrency](final.json) | [src/frontend/sema/sema_call_properties.cpp](../../src/frontend/sema/sema_call_properties.cpp)：修复标量借用和读写效果；ABI 借用 shared_ptr；通道临时 option 直接取值。guard 身份/关闭检查、取消和溢出检查保留。 |
| atomic_add | 20.191 | 0.291 | 69.38× | [concurrency](final.json) | [src/frontend/sema/sema_call_properties.cpp](../../src/frontend/sema/sema_call_properties.cpp)：修复标量借用和读写效果；ABI 借用 shared_ptr；通道临时 option 直接取值。guard 身份/关闭检查、取消和溢出检查保留。 |
| log_filtered | 48.423 | 1.032 | 46.92× | [diagnostics](final.json) | [src/backend/cpp/checked_callback.hpp](../../src/backend/cpp/checked_callback.hpp)：回调使用内部入口及当前上下文；已知 bind 目标直接调用，安全引用捕获借用；过滤字面量日志推迟字段闭包和文本句柄构造。保留回调错误与副作用顺序。 |
| test_property | 4.301 | 0.665 | 6.47× | [diagnostics](final.json) | [src/backend/cpp/checked_callback.hpp](../../src/backend/cpp/checked_callback.hpp)：回调使用内部入口及当前上下文；已知 bind 目标直接调用，安全引用捕获借用；过滤字面量日志推迟字段闭包和文本句柄构造。保留回调错误与副作用顺序。 |
| test_parameterized | 4.065 | 0.376 | 10.81× | [diagnostics](final.json) | [src/backend/cpp/checked_callback.hpp](../../src/backend/cpp/checked_callback.hpp)：回调使用内部入口及当前上下文；已知 bind 目标直接调用，安全引用捕获借用；过滤字面量日志推迟字段闭包和文本句柄构造。保留回调错误与副作用顺序。 |
| channel_send_recv | 23.152 | 2.038 | 11.36× | [concurrency](final.json) | [src/frontend/sema/sema_call_properties.cpp](../../src/frontend/sema/sema_call_properties.cpp)：修复标量借用和读写效果；ABI 借用 shared_ptr；通道临时 option 直接取值。guard 身份/关闭检查、取消和溢出检查保留。 |
| deinit | 16.479 | 16.636 | 0.99× | [language](final.json) | [src/backend/cpp/class_abi.cpp](../../src/backend/cpp/class_abi.cpp)：已检查析构视图、根释放和循环登记。仍需保证继承析构、复活及可变引用图；没有用显式 close 或仅删除自环替代通用语义。该项差距仍在。 |
| cycle_gc | 1.597 | 1.527 | 1.05× | [language](final.json) | [src/backend/cpp/class_abi.cpp](../../src/backend/cpp/class_abi.cpp)：已检查析构视图、根释放和循环登记。仍需保证继承析构、复活及可变引用图；没有用显式 close 或仅删除自环替代通用语义。该项差距仍在。 |
| variadic_unpack | 11.820 | 12.169 | 0.97× | [language](final.json) | [src/backend/cpp/call_abi.cpp](../../src/backend/cpp/call_abi.cpp)：目标符号已静态绑定，动态 *array/**dict 仍须按运行时键绑定、检查重名并独立持有可变参数包；当前源值可变，不能按字面量展开。差距仍在。 |
| deep_copy | 27.881 | 26.015 | 1.07× | [language](final.json) | [src/backend/cpp/deep_copy.cpp](../../src/backend/cpp/deep_copy.cpp)：把常见数组/字典图分派提前，减少依次试探向量和外部资源类型；保留共享身份表、环、资源和类析构规则。 |
| format_literal | 7.263 | 8.065 | 0.90× | [diverse](calibration.json) | [src/stdlib/format_append.cpp](../../src/stdlib/format_append.cpp)：保留已有静态格式计划和单次执行入口；文本直接验证 UTF-8 并追加。对照固定拼接不是完整格式器，残余调用和字符串所有权成本保留。 |
| tls_handshake | 250.601 | 116.375 | 2.15× | [network](calibration.json) | [src/stdlib/x509_verification_context.cpp](../../src/stdlib/x509_verification_context.cpp)：阶段计时定位到重复证书引擎初始化。仅缓存完全一致的叶/中间证书/显式根输入，每次重新验证时间、用途、主机名；系统根实时读取。 |
| serde_short_text | 8.944 | 7.869 | 1.14× | [diverse](calibration.json) | [src/stdlib/format_stream.cpp](../../src/stdlib/format_stream.cpp)：JSON 普通 ASCII 连续片段批量读取/追加，保持转义、Unicode、位置与字节限额；沿用静态 schema 直接编解码。 |
| sqlite_read | 70.293 | 26.857 | 2.62× | [sqlite](final.json) | [src/backend/cpp/db_value_abi.cpp](../../src/backend/cpp/db_value_abi.cpp)：同步读取借用行句柄；get_int/float/bool/str(...).value() 合并为受检取值，删除临时 option；行快照仍独立持有。 |
| fanout-2000 | 13.877 | 12.306 | 1.13× | [mini](final.json) | [src/backend/llvm/codegen_ownership.cpp](../../src/backend/llvm/codegen_ownership.cpp)：使用原题源码和原始输入输出验证编译器改动，不修改题解算法；字符串、容器身份和受检操作构成端到端余量。 |
| random-2000-1 | 7.412 | 6.924 | 1.07× | [mini](final.json) | [src/backend/llvm/codegen_ownership.cpp](../../src/backend/llvm/codegen_ownership.cpp)：使用原题源码和原始输入输出验证编译器改动，不修改题解算法；字符串、容器身份和受检操作构成端到端余量。 |
| format_dynamic | 7.188 | 7.959 | 0.90× | [diverse](calibration.json) | [src/stdlib/format_append.cpp](../../src/stdlib/format_append.cpp)：保留已有静态格式计划和单次执行入口；文本直接验证 UTF-8 并追加。对照固定拼接不是完整格式器，残余调用和字符串所有权成本保留。 |
| regex_search | 56.946 | 52.551 | 1.08× | [compute](final.json) | [src/backend/cpp/regex_abi.cpp](../../src/backend/cpp/regex_abi.cpp)：借用 pattern/text；捕获文本和边界数组转移所有权；结束标量偏移仅扫描匹配区。仍返回完整捕获组、组名、字节/标量位置并保留限额取消。 |
| decimal_add | 19.231 | 12.181 | 1.58× | [compute](final.json) | [src/stdlib/decimal_abi.cpp](../../src/stdlib/decimal_abi.cpp)：固定私有 bool/str/int 字段不登记循环 GC，预留字段容量并借用原生运算输入；保留 38 位十进制与舍入、范围检查。 |
| format_parameter | 22.954 | 17.379 | 1.32× | [format](calibration.json) | [src/stdlib/format_append.cpp](../../src/stdlib/format_append.cpp)：补齐基础类型动态格式化的借用；整数进制、符号、填充直接追加到结果，避免临时数字字符串及对 ASCII 数字重复计码点。 |
| format_alternating | 22.282 | 16.602 | 1.34× | [format](calibration.json) | [src/stdlib/format_append.cpp](../../src/stdlib/format_append.cpp)：补齐基础类型动态格式化的借用；整数进制、符号、填充直接追加到结果，避免临时数字字符串及对 ASCII 数字重复计码点。 |
| moves-2000 | 6.588 | 6.329 | 1.04× | [mini](final.json) | [src/backend/llvm/codegen_ownership.cpp](../../src/backend/llvm/codegen_ownership.cpp)：使用原题源码和原始输入输出验证编译器改动，不修改题解算法；字符串、容器身份和受检操作构成端到端余量。 |
| copy_cycle | 11.950 | 10.037 | 1.19× | [language](final.json) | [src/backend/cpp/deep_copy.cpp](../../src/backend/cpp/deep_copy.cpp)：把常见数组/字典图分派提前，减少依次试探向量和外部资源类型；保留共享身份表、环、资源和类析构规则。 |
| random | 2.000 | 2.000 | 1.00× | [library](final.json) | [src/stdlib/random.cpp](../../src/stdlib/random.cpp)：与 random_int 相同路径；原组合负载仅整数毫秒计时，附带长负载验证，不解释成精确微小差距。 |
| serde_long_text | 27.553 | 9.852 | 2.80× | [diverse](calibration.json) | [src/stdlib/format_stream.cpp](../../src/stdlib/format_stream.cpp)：JSON 普通 ASCII 连续片段批量读取/追加，保持转义、Unicode、位置与字节限额；沿用静态 schema 直接编解码。 |
| string_conversion | 17.805 | 17.238 | 1.03× | [language](final.json) | [src/stdlib/format_spec.cpp](../../src/stdlib/format_spec.cpp)：沿用原有文本/数字转换路径，数字宽度无需再次按 Unicode 扫描；字符串结果的独立所有权及浮点显示规则保留，未改写基准。 |
| sqlite_insert | 11.313 | 11.424 | 0.99× | [sqlite](final.json) | [src/stdlib/db_query.cpp](../../src/stdlib/db_query.cpp)：核实同后端 C++ 原差距低于 1.4 倍；Python 批次/包装路径不同。保留事务协调、insert_id 判断和错误契约，没有删除这些工作。 |
| bytes_hex | 9.809 | 9.803 | 1.00× | [compute](final.json) | [src/stdlib/bytes.cpp](../../src/stdlib/bytes.cpp)：沿用 bytes 直接编码/解码与只读借用；结果拥有独立存储，输入格式检查保留。对照未包含全部 TX 句柄成本。 |
| postgres_read | 99.238 | 42.899 | 2.31× | [postgres](final.json) | [src/backend/llvm/codegen_sum.cpp](../../src/backend/llvm/codegen_sum.cpp)：复用数据库直接取值与 option/result 已知引用类型直接交付根；保留 libpq 单行读取与行快照。 |
| env_get | 12.857 | 10.911 | 1.18× | [compute](final.json) | [src/stdlib/env.cpp](../../src/stdlib/env.cpp)：256 wchar 短值使用栈缓冲，长值按需扩容并在并发增长时重读；每次查询实际 Windows 环境，不缓存可能变化的值。 |
| parse_int | 11.318 | 12.537 | 0.90× | [compute](final.json) | [src/backend/llvm/codegen_native_parse.cpp](../../src/backend/llvm/codegen_native_parse.cpp)：确认已使用静态标量解析结果和惰性错误文本；保留失败类型、进制与无效输入校验，没有把受检解析换成不校验转换。 |
| map_hit_128 | 1.725 | 1.973 | 0.87× | [diverse](calibration.json) | [src/stdlib/typed_map.hpp](../../src/stdlib/typed_map.hpp)：确认是整数特化 unordered_map、标量槽位和借用接收者；保留缺键异常与可能 rehash 的边界。C++ 内联查找与 TX ABI 的成本仍有差异。 |
| queue_push_pop | 1.514 | 1.566 | 0.97× | [features](final.json) | [src/frontend/ast/call_properties.cpp](../../src/frontend/ast/call_properties.cpp)：确认标量容器特化、借用和静态效果已生效；扩容、空队列检查与独立迭代快照按契约保留，没有把快照改成借用视图。 |
| encoding_dynamic | 13.097 | 12.596 | 1.04× | [diverse](calibration.json) | [src/backend/cpp/encoding_memory_abi.cpp](../../src/backend/cpp/encoding_memory_abi.cpp)：确认编码名静态绑定、32 字节短载荷内嵌和字面量借用已有；保留 UTF-8 验证、BOM 与不可变 bytes/文本的拥有关系。没有新增改变编码语义的捷径。 |
| encoding_literal | 13.805 | 12.760 | 1.08× | [diverse](calibration.json) | [src/backend/cpp/encoding_memory_abi.cpp](../../src/backend/cpp/encoding_memory_abi.cpp)：确认编码名静态绑定、32 字节短载荷内嵌和字面量借用已有；保留 UTF-8 验证、BOM 与不可变 bytes/文本的拥有关系。没有新增改变编码语义的捷径。 |
| graph_copy | 9.673 | 7.322 | 1.32× | [graph](final.json) | [src/backend/cpp/deep_copy.cpp](../../src/backend/cpp/deep_copy.cpp)：同一通用图复制分派修复，并沿用保留共享和环的原生图对照；仍有动态节点和身份表成本。 |
| string | 9.000 | 9.000 | 1.00× | [library](final.json) | [src/frontend/sema/sema_call_properties.cpp](../../src/frontend/sema/sema_call_properties.cpp)：只读 string 操作借用同步实参，保留 Unicode 切片、验证、结果分配。该组合包含多种字符串操作，不能按单次 C++ 拼接解释。 |
| class_methods | 0.679 | 0.648 | 1.05× | [language](final.json) | [src/backend/llvm/codegen_gc.cpp](../../src/backend/llvm/codegen_gc.cpp)：确认生成代码使用固定槽位、直接调用和受检算术；没有发现运行时名称分派。保留共享对象身份与错误语义，短负载受跨 ABI 调用成本影响。 |
| vector_scan_1k | 0.278 | 0.266 | 1.05× | [diverse](calibration.json) | [src/backend/llvm/codegen_vector.cpp](../../src/backend/llvm/codegen_vector.cpp)：确认扫描为原生连续元素和只读借用，保留受检累计；千元素重复短负载与跨 ABI 边界成本仍在。 |
| random_long | 34.000 | 35.000 | 0.97× | [random_long](final.json) | [src/stdlib/random.cpp](../../src/stdlib/random.cpp)：五百万次固定种子同序列验证；保留范围检查和分布行为。该路径已有直接 context 入口，本轮未引入新算法。 |
| requests_get / http_get | 1208.572 | 1202.814 | 1.00× | [http](delivery_http.json) | [src/stdlib/httpx_client.cpp](../../src/stdlib/httpx_client.cpp)：单次便捷 API 每次建立 WinHTTP 会话并采用自动代理；对照库及客户端生命周期不同。保留单次会话隔离，不把全局长连接池当作等价修复。 |
| map_hit_8192 | 2.234 | 2.381 | 0.94× | [diverse](calibration.json) | [src/stdlib/typed_map.hpp](../../src/stdlib/typed_map.hpp)：确认是整数特化 unordered_map、标量槽位和借用接收者；保留缺键异常与可能 rehash 的边界。C++ 内联查找与 TX ABI 的成本仍有差异。 |
| sqlite_savepoint | 2.860 | 2.785 | 1.03× | [sqlite](final.json) | [src/stdlib/db_query.cpp](../../src/stdlib/db_query.cpp)：核实同后端 C++ 原差距低于 1.4 倍；Python 批次/包装路径不同。保留事务协调、insert_id 判断和错误契约，没有删除这些工作。 |
| vector_push | 1.933 | 1.893 | 1.02× | [features](final.json) | [src/frontend/ast/call_properties.cpp](../../src/frontend/ast/call_properties.cpp)：确认标量容器特化、借用和静态效果已生效；扩容、空队列检查与独立迭代快照按契约保留，没有把快照改成借用视图。 |
| array_destructure | 4.120 | 0.114 | 36.14× | [language](final.json) | [src/backend/llvm/codegen_ownership.cpp](../../src/backend/llvm/codegen_ownership.cpp)：修复首次解构声明被计为重绑定，标量快照不再立即装箱；真实重赋值仍物化根，已检查快照独立性与后续覆盖。 |
| statistics_mean | 9.714 | 8.238 | 1.18× | [compute](final.json) | [src/stdlib/statistics_internal.hpp](../../src/stdlib/statistics_internal.hpp)：补齐向量/迭代器均值输入借用和无分配效果；保留有限值检查及 long double 补偿求和，避免大数抵消丢失小项。 |
| httpx_get / http_get | 1148.070 | 1186.496 | 0.97× | [http](delivery_http.json) | [src/stdlib/httpx_client.cpp](../../src/stdlib/httpx_client.cpp)：单次便捷 API 每次建立 WinHTTP 会话并采用自动代理；对照库及客户端生命周期不同。保留单次会话隔离，不把全局长连接池当作等价修复。 |
| random_int | 1.396 | 1.479 | 0.94× | [compute](final.json) | [src/stdlib/random.cpp](../../src/stdlib/random.cpp)：已采用当前上下文直接入口；保留 mt19937_64 的固定种子序列、均匀分布及边界检查，没有替换随机算法。 |
| forced-increasing-max | 5.012 | 5.076 | 0.99× | [stage](final.json) | [src/backend/llvm/codegen_map.cpp](../../src/backend/llvm/codegen_map.cpp)：使用原题源码和原始输入输出复测，核实容器直接特化及受检整数路径；没有更改题意、输入或解法来压低耗时。 |
| class_operator | 0.866 | 0.787 | 1.10× | [language](final.json) | [src/backend/llvm/codegen_gc.cpp](../../src/backend/llvm/codegen_gc.cpp)：确认生成代码使用固定槽位、直接调用和受检算术；没有发现运行时名称分派。保留共享对象身份与错误语义，短负载受跨 ABI 调用成本影响。 |
| iterator_snapshot | 0.171 | 0.174 | 0.98× | [features](final.json) | [src/frontend/ast/call_properties.cpp](../../src/frontend/ast/call_properties.cpp)：确认标量容器特化、借用和静态效果已生效；扩容、空队列检查与独立迭代快照按契约保留，没有把快照改成借用视图。 |
