"""把原始 51 个去重热点逐项对应到本轮代码处置和配对样本。"""
import json
from pathlib import Path
import statistics

root = Path(__file__).resolve().parents[1]
archive = root / "benchmarks/performance_full_repair_2026-09-30"
samples = json.loads((archive / "final.json").read_text(encoding="utf-8"))
sample_files = {group: "final.json" for group in samples if group != "manifest"}
calibration_path = archive / "calibration.json"
if calibration_path.exists():
    calibration = json.loads(calibration_path.read_text(encoding="utf-8"))
    assert calibration["manifest"].get("completed_utc"), "calibration incomplete"
    for group, values in calibration.items():
        if group != "manifest":
            samples[group] = values
            sample_files[group] = "calibration.json"
delivery_http_path = archive / "delivery_http.json"
if delivery_http_path.exists():
    delivery_http = json.loads(delivery_http_path.read_text(encoding="utf-8"))
    assert delivery_http["manifest"].get("completed_utc"), "HTTP measurement incomplete"
    samples["http"] = delivery_http["http"]
    sample_files["http"] = "delivery_http.json"
gaps = json.loads((root / "benchmarks/performance_retest_2026-09-30_full_rerun/all_gaps.json").read_text(encoding="utf-8"))

# 名称只用于归并报告；没有根据基准名称选择编译器或运行时行为。
groups = {}


def register(group, names, source, action):
    assert (root / source).is_file(), source
    for name in names.split():
        groups[name] = (group, source, action)


register("concurrency", "mutex_uncontended atomic_add channel_send_recv",
         "src/frontend/sema/sema_call_properties.cpp",
         "修复标量借用和读写效果；ABI 借用 shared_ptr；通道临时 option 直接取值。guard 身份/关闭检查、取消和溢出检查保留。")
register("diagnostics", "test_parameterized test_property log_filtered",
         "src/backend/cpp/checked_callback.hpp",
         "回调使用内部入口及当前上下文；已知 bind 目标直接调用，安全引用捕获借用；过滤字面量日志推迟字段闭包和文本句柄构造。保留回调错误与副作用顺序。")
register("sqlite", "sqlite_read", "src/backend/cpp/db_value_abi.cpp",
         "同步读取借用行句柄；get_int/float/bool/str(...).value() 合并为受检取值，删除临时 option；行快照仍独立持有。")
register("postgres", "postgres_read", "src/backend/llvm/codegen_sum.cpp",
         "复用数据库直接取值与 option/result 已知引用类型直接交付根；保留 libpq 单行读取与行快照。")
register("sqlite", "sqlite_insert sqlite_savepoint", "src/stdlib/db_query.cpp",
         "核实同后端 C++ 原差距低于 1.4 倍；Python 批次/包装路径不同。保留事务协调、insert_id 判断和错误契约，没有删除这些工作。")
register("network", "tls_handshake", "src/stdlib/x509_verification_context.cpp",
         "阶段计时定位到重复证书引擎初始化。仅缓存完全一致的叶/中间证书/显式根输入，每次重新验证时间、用途、主机名；系统根实时读取。")
register("http", "requests_get httpx_get", "src/stdlib/httpx_client.cpp",
         "单次便捷 API 每次建立 WinHTTP 会话并采用自动代理；对照库及客户端生命周期不同。保留单次会话隔离，不把全局长连接池当作等价修复。")
register("language", "deinit cycle_gc", "src/backend/cpp/class_abi.cpp",
         "已检查析构视图、根释放和循环登记。仍需保证继承析构、复活及可变引用图；没有用显式 close 或仅删除自环替代通用语义。该项差距仍在。")
register("language", "deep_copy copy_cycle", "src/backend/cpp/deep_copy.cpp",
         "把常见数组/字典图分派提前，减少依次试探向量和外部资源类型；保留共享身份表、环、资源和类析构规则。")
register("graph", "graph_copy", "src/backend/cpp/deep_copy.cpp",
         "同一通用图复制分派修复，并沿用保留共享和环的原生图对照；仍有动态节点和身份表成本。")
register("language", "variadic_unpack", "src/backend/cpp/call_abi.cpp",
         "目标符号已静态绑定，动态 *array/**dict 仍须按运行时键绑定、检查重名并独立持有可变参数包；当前源值可变，不能按字面量展开。差距仍在。")
register("language", "class_methods class_operator", "src/backend/llvm/codegen_gc.cpp",
         "确认生成代码使用固定槽位、直接调用和受检算术；没有发现运行时名称分派。保留共享对象身份与错误语义，短负载受跨 ABI 调用成本影响。")
register("language", "array_destructure", "src/backend/llvm/codegen_ownership.cpp",
         "修复首次解构声明被计为重绑定，标量快照不再立即装箱；真实重赋值仍物化根，已检查快照独立性与后续覆盖。")
register("diverse", "format_literal format_dynamic", "src/stdlib/format_append.cpp",
         "保留已有静态格式计划和单次执行入口；文本直接验证 UTF-8 并追加。对照固定拼接不是完整格式器，残余调用和字符串所有权成本保留。")
register("format", "format_parameter format_alternating", "src/stdlib/format_append.cpp",
         "补齐基础类型动态格式化的借用；整数进制、符号、填充直接追加到结果，避免临时数字字符串及对 ASCII 数字重复计码点。")
register("language", "string_conversion", "src/stdlib/format_spec.cpp",
         "沿用原有文本/数字转换路径，数字宽度无需再次按 Unicode 扫描；字符串结果的独立所有权及浮点显示规则保留，未改写基准。")
register("library", "string", "src/frontend/sema/sema_call_properties.cpp",
         "只读 string 操作借用同步实参，保留 Unicode 切片、验证、结果分配。该组合包含多种字符串操作，不能按单次 C++ 拼接解释。")
register("diverse", "serde_short_text serde_long_text", "src/stdlib/format_stream.cpp",
         "JSON 普通 ASCII 连续片段批量读取/追加，保持转义、Unicode、位置与字节限额；沿用静态 schema 直接编解码。")
register("diverse", "encoding_literal encoding_dynamic", "src/backend/cpp/encoding_memory_abi.cpp",
         "确认编码名静态绑定、32 字节短载荷内嵌和字面量借用已有；保留 UTF-8 验证、BOM 与不可变 bytes/文本的拥有关系。没有新增改变编码语义的捷径。")
register("compute", "bytes_hex", "src/stdlib/bytes.cpp",
         "沿用 bytes 直接编码/解码与只读借用；结果拥有独立存储，输入格式检查保留。对照未包含全部 TX 句柄成本。")
register("compute", "parse_int", "src/backend/llvm/codegen_native_parse.cpp",
         "确认已使用静态标量解析结果和惰性错误文本；保留失败类型、进制与无效输入校验，没有把受检解析换成不校验转换。")
register("compute", "regex_search", "src/backend/cpp/regex_abi.cpp",
         "借用 pattern/text；捕获文本和边界数组转移所有权；结束标量偏移仅扫描匹配区。仍返回完整捕获组、组名、字节/标量位置并保留限额取消。")
register("compute", "decimal_add", "src/stdlib/decimal_abi.cpp",
         "固定私有 bool/str/int 字段不登记循环 GC，预留字段容量并借用原生运算输入；保留 38 位十进制与舍入、范围检查。")
register("compute", "env_get", "src/stdlib/env.cpp",
         "256 wchar 短值使用栈缓冲，长值按需扩容并在并发增长时重读；每次查询实际 Windows 环境，不缓存可能变化的值。")
register("compute", "statistics_mean", "src/stdlib/statistics_internal.hpp",
         "补齐向量/迭代器均值输入借用和无分配效果；保留有限值检查及 long double 补偿求和，避免大数抵消丢失小项。")
register("compute", "random_int", "src/stdlib/random.cpp",
         "已采用当前上下文直接入口；保留 mt19937_64 的固定种子序列、均匀分布及边界检查，没有替换随机算法。")
register("library", "random", "src/stdlib/random.cpp",
         "与 random_int 相同路径；原组合负载仅整数毫秒计时，附带长负载验证，不解释成精确微小差距。")
register("random_long", "random_long", "src/stdlib/random.cpp",
         "五百万次固定种子同序列验证；保留范围检查和分布行为。该路径已有直接 context 入口，本轮未引入新算法。")
register("diverse", "map_hit_128 map_hit_8192", "src/stdlib/typed_map.hpp",
         "确认是整数特化 unordered_map、标量槽位和借用接收者；保留缺键异常与可能 rehash 的边界。C++ 内联查找与 TX ABI 的成本仍有差异。")
register("diverse", "vector_scan_1k", "src/backend/llvm/codegen_vector.cpp",
         "确认扫描为原生连续元素和只读借用，保留受检累计；千元素重复短负载与跨 ABI 边界成本仍在。")
register("features", "queue_push_pop vector_push iterator_snapshot", "src/frontend/ast/call_properties.cpp",
         "确认标量容器特化、借用和静态效果已生效；扩容、空队列检查与独立迭代快照按契约保留，没有把快照改成借用视图。")
register("mini", "fanout-2000 moves-2000 random-2000-1", "src/backend/llvm/codegen_ownership.cpp",
         "使用原题源码和原始输入输出验证编译器改动，不修改题解算法；字符串、容器身份和受检操作构成端到端余量。")
register("stage", "forced-increasing-max", "src/backend/llvm/codegen_map.cpp",
         "使用原题源码和原始输入输出复测，核实容器直接特化及受检整数路径；没有更改题意、输入或解法来压低耗时。")

rows = []
for name in dict.fromkeys(gap["name"] for gap in gaps):
    measured_name = name.split(" / ")[0]
    group, source, action = groups[measured_name]
    paired = samples[group]
    before = statistics.median(sample["cases"][measured_name]["ms"] for sample in paired["old"])
    after = statistics.median(sample["cases"][measured_name]["ms"] for sample in paired["new"])
    rows.append({"name": name, "group": group, "source": source, "action": action,
                 "old_ms": before, "new_ms": after, "speedup": before / after if after else None,
                 "sample_file": sample_files[group]})
lines = ["# 完整差距逐项核对", "",
         f"原报告去重 {len(rows)} 个热点均对应到本轮配对样本，单位 ms。主样本旧/新各一次预热、三次交替采样；network/diverse/format 使用后续五轮校准；HTTP 使用最终封包后三轮。",
         "倍率是同轮旧程序/新程序，不是当前 TX/C++ 倍率。校验值及外部题输出一致才进入结果。",
         "保留现有语义的条目不代表性能差距消失；表内明确记录没有新增算法或仍有差距的项目。", "",
         "| 热点 | 旧 ms | 新 ms | 旧/新 | 样本 | 代码定位与处置 |",
         "| --- | ---: | ---: | ---: | --- | --- |"]
for row in rows:
    ratio = f'{row["speedup"]:.2f}×' if row["speedup"] is not None else "低于计时分辨率"
    lines.append(f'| {row["name"]} | {row["old_ms"]:.3f} | {row["new_ms"]:.3f} | {ratio} | '
                 f'[{row["group"]}]({row["sample_file"]}) | '
                 f'[{row["source"]}](../../{row["source"]})：{row["action"]} |')
(archive / "audit.json").write_text(json.dumps(rows, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
(archive / "audit.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
print(f"Mapped {len(rows)} / {len(set(gap['name'] for gap in gaps))} unique gaps")

summary = ["# 性能修复结果（2026-09-30）", "",
           "原报告的 51 个去重热点已逐项核对，16 组配对样本包含原有、新增标准库与真实程序。",
           "本轮修复已确认的冗余路径；仍有性能差距的项目保留现状和原因，没有宣称全部达标。", "",
           "| 项目 | 旧 ms | 新 ms | 旧/新 |", "| --- | ---: | ---: | ---: |"]
selected = {"mutex_uncontended", "atomic_add", "channel_send_recv", "log_filtered", "test_parameterized",
            "array_destructure", "sqlite_read", "postgres_read", "tls_handshake", "serde_long_text",
            "decimal_add", "format_parameter"}
for row in rows:
    if row["name"] in selected:
        summary.append(f'| {row["name"]} | {row["old_ms"]:.3f} | {row["new_ms"]:.3f} | {row["speedup"]:.2f}× |')
summary.extend(["", "倍率只表示本轮旧/新程序的配对结果。主表三轮，network/diverse/format 使用五轮校准；所有轮次均先预热。",
    "", "- [完整 51 项代码定位、处置和未改善结果](audit.md)",
    "- [实现与验证说明](../../docs/performance_full_repair.md)",
    "- [主样本与源码/程序摘要](final.json)、[后续定向校准](calibration.json)",
    "- [相关行为及数据库、跨线程、兼容性检查](checks.json)",
    "", "尚存：析构、动态参数展开、guard 拥有状态、部分容器和字符串的 ABI/分配成本；单次 HTTP 会话生命周期。",
    "五轮校准的固定格式化约慢 10%，map 两个负载分别约慢 14% 和 7%；完整表保留这些回退，未将其计为优化成功。",
    "同轮原子溢出、NULL、取消、UTF-8、证书信任、析构与复活契约均保留。",
    "", "阶段 profiling 文件仅用来定位证书引擎初始化；正式源码没有 profiling 打印。"])
(archive / "README.md").write_text("\n".join(summary) + "\n", encoding="utf-8")
