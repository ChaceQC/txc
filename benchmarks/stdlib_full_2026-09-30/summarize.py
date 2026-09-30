"""生成四语言新增标准库报告，并将有效校准结果合并到本轮总报告。"""
from datetime import datetime, timezone, timedelta
import json
from pathlib import Path

root = Path(__file__).resolve().parents[2]
source = Path(__file__).resolve().parent
full = root / "benchmarks/performance_retest_2026-09-30_full_rerun"

details = {
    "thread_spawn_join": ("thread", "100 次启动线程并 join", "C++ std::thread；Java 平台线程；Python threading。含线程创建/退出。"),
    "mutex_uncontended": ("sync", "100000 次无竞争加锁、取值、加一、写回、解锁", "TX 为受保护值及 guard API；C++ std::mutex；Java ReentrantLock；Python Lock。衡量整段公开 API 路径，不是单条锁指令。"),
    "atomic_add": ("sync", "100000 次顺序一致 int64 加一", "C++ std::atomic、Java AtomicLong；Python 没有对应公开原子整数，使用 Lock 实现线程安全加法，不能当成原子指令性能。"),
    "channel_send_recv": ("channel", "容量 1，20000 次同线程 send/recv", "C++ mutex/condition_variable 队列；Java ArrayBlockingQueue；Python Queue。都是立即成功路径；TX 另有取消及句柄规则，不代表多生产者吞吐。"),
    "task_spawn_wait": ("task", "500 次提交并等待返回 1", "参考使用单工作线程池；TX 使用任务运行时和 scope。包含组内初次线程池启动，不代表饱和并发吞吐。"),
    "sqlite_insert": ("db", "单事务参数绑定插入 2000 行", "C++ 直接调用 TX 的同一 C++ 数据库后端；Java SQLite JDBC；Python sqlite3。SQLite 版本不同，事务内逐条执行，不使用批插入。"),
    "sqlite_read": ("db", "2000 行×10 遍，读取整数及文本并求和", "C++ 使用同一后端及行快照；Java JDBC、Python sqlite3。TX/C++ 差值包含 ABI、option、行读取接口与生成代码开销。"),
    "sqlite_savepoint": ("db", "100 次事务/保存点/写入/回滚/提交", "C++ 同后端；Java JDBC、Python sqlite3；在内存库中测量，不表示持久化落盘吞吐。"),
    "sqlite_pool": ("db", "100 次获取、SELECT 42、归还", "TX 与 C++ 测真实池；其 SQLite 归还策略为关闭重开。Java/Python 参考每次连接关闭，保持生命周期，未复制完整池等待/别名检查。"),
    "sqlite_async": ("db,task", "100 次异步 SELECT 42 并等待", "TX 任务运行时；C++ 同异步数据库后端加单线程执行器；Java/Python 单线程池。都包含单独事务与连接关闭。"),
    "migration_recheck": ("db", "100 次重查已应用迁移及 schema_version", "C++ 同迁移后端；Java/Python 实现相同长度分帧 SHA-256、两次 IMMEDIATE 事务及账本读取；只覆盖幂等成功路径。"),
    "postgres_insert": ("db", "TLS 单事务参数绑定插入 2000 行", "C++ 同 libpq 后端；Java PostgreSQL JDBC；Python psycopg。驱动 prepare 缓存策略不同；共用临时 PostgreSQL 18.4，不连接已有数据库。"),
    "postgres_read": ("db", "TLS 2000 行×10 遍，读取整数和文本", "TX/C++ libpq 单行模式；Java/Python 默认结果获取策略不同。C++ 同后端可用于定位包装层开销，跨驱动比例是端到端负载观察。"),
    "postgres_savepoint": ("db", "TLS 100 次保存点回滚事务", "C++ 同后端；Java/Python 各自驱动。所有操作在真实服务器执行，受本机网络调度影响。"),
    "secret_equal": ("secret", "20000 次 1024 字节常量时间比较", "C++ 同 secret 后端；Java MessageDigest.isEqual；Python hmac.compare_digest。Java/Python 普通字节数组没有 TX 秘密句柄的保护与清零生命周期。"),
    "argon2_hash_verify": ("password", "3 次 Argon2id 哈希并验证", "m=19456 KiB、t=2、p=1、v=19、16 字节随机 salt、32 字节输出；C++ 同 Argon2 后端，Java BouncyCastle，Python argon2-cffi；含 PHC 编解码。"),
    "ed25519_sign": ("public_key", "1024 字节消息签名 500 次", "共用临时随机 seed；C++ 同 libsodium 后端；Java JCA Ed25519；Python cryptography。密钥导入不计时，签名随后验签。"),
    "ed25519_verify": ("public_key", "1024 字节消息验签 500 次", "三类后端验证各自生成的签名；同一 seed 和消息，成功次数逐轮核对。"),
    "x509_parse_der": ("x509", "同一 DER 证书解析/编码 500 次", "C++ 同 Windows 证书后端；Java CertificateFactory 可能缓存；Python cryptography。测重复证书热路径，不外推到大量不同证书。"),
    "dns_localhost": ("dns", "100 次 localhost 地址解析", "TX 对 localhost 有专用路径；C++ getaddrinfo、Java InetAddress、Python getaddrinfo 各有不同缓存策略。不能用于比较远端 DNS。"),
    "udp_echo": ("socket", "500 次 1024 字节本机 UDP 回声", "四语言使用同一 Python 回声服务，逐次完整比较 payload。服务端调度也计入端到端时间。"),
    "ipc_echo": ("ipc", "500 次 TXIP 命名管道回声", "使用同一服务器、22 字节帧和 CBOR 整数 42。C++/Java/Python 固定整数编解码；TX 为通用 CBOR 与资源句柄，不代表任意对象 IPC 差距。"),
    "tls_handshake": ("tls", "10 次新 TCP+TLS 1.2 握手及单字节回声", "全部验证临时 CA 和 localhost，禁用服务端票据，Java 每次废弃 session；TX mbedTLS，C++/Python OpenSSL，Java JSSE。配置在计时外，库内部握手上下文分配计时。"),
    "async_file_rw": ("async_file", "32 次 64 KiB 定位写读，合计传输 4 MiB", "TX IOCP、Java AsynchronousFileChannel；C++/Python 工作线程执行定位 I/O。全部逐次等待并核对字节，不代表大量在途 I/O 吞吐。"),
    "test_parameterized": ("test", "20000 次带索引的成功回调", "参考使用 std::function / IntConsumer / Python 函数；TX 含真实测试框架和受检回调。参考没有 TX 错误上下文，不外推到失败报告。"),
    "test_property": ("test", "20000 次生成器与成功谓词", "固定 seed=1000，参考保留两个回调；本负载不执行失败缩减。TX 还保留受检回调及测试错误处理。"),
    "log_filtered": ("log", "100000 次被过滤的惰性 debug 日志", "采用校准样本：C++ 调用真实 TX 日志后端级别检查，Java Logger，Python logging；各参考仅在级别允许时构造字段，TX 另有闭包/错误边界。废弃初版 C++ volatile 检查倍率。"),
    "log_file": ("log", "2000 条带上下文和敏感字段遮蔽的 JSONL", "C++ 同日志后端；Java/Python 固定 schema 构造。全部每条 flush、进程内加锁；计时外核对行数、索引、请求标识、遮蔽值。未覆盖轮转压力。"),
    "profile_spans": ("profile", "1000 次分段计时并保留记录", "TX/C++ 启用真实分析器；Java/Python 仅保存时间段记录，没有同时启用 CPU/分配采样。仅为 API 负载参考，不能宣称完整 profiler 等价。"),
}


def read(path):
    return json.loads(path.read_text(encoding="utf-8"))


def table(headers, rows):
    return ["| " + " | ".join(headers) + " |", "| " + " | ".join(["---"] * len(headers)) + " |",
            *["| " + " | ".join(map(str, row)) + " |" for row in rows], ""]


def write(path, lines):
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main():
    results = read(source / "results.json")
    results.update(read(source / "calibration_results.json"))
    completion = read(source / "completed.json")
    manifest = read(source / "manifest.json")
    calibration = read(source / "calibration_completed.json")
    assert completion["cases"] == len(details) == 29
    assert set(results) == set(completion["groups"])
    gaps = []
    rows = []
    for group, data in results.items():
        for name, cases in data["cases"].items():
            assert set(cases) == {"TX", "C++", "Java", "Python"}
            assert all(len(case["samples_ms"]) == 5 for case in cases.values())
            assert len({case["checksum"] for case in cases.values()}) == 1
            tx = cases["TX"]["median_ms"]
            ratios = {language: tx / cases[language]["median_ms"] for language in ("C++", "Java", "Python")}
            rows.append([name, *[f"{cases[label]['median_ms']:.3f}" for label in ("TX", "C++", "Java", "Python")],
                         *[f"{ratios[label]:.2f}×" for label in ratios]])
            for label, ratio in ratios.items():
                if ratio >= 3:
                    gaps.append({"suite": "新增/" + group, "name": name, "tx_ms": tx,
                                 "reference_ms": cases[label]["median_ms"], "reference": label,
                                 "ratio": ratio, "note": details[name][2]})
    gaps.sort(key=lambda row: row["ratio"], reverse=True)
    (source / "final_results.json").write_text(json.dumps(results, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    (source / "gaps.json").write_text(json.dumps(gaps, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    zone = timezone(timedelta(hours=8))
    start = datetime.fromisoformat(manifest["started_utc"]).astimezone(zone)
    end = datetime.fromisoformat(completion["completed_utc"]).astimezone(zone)
    lines = ["# 新增标准库四语言全量补测", "",
             f"版本 `{manifest['head']}`。正式采样北京时间 {start:%Y-%m-%d %H:%M:%S} 至 {end:%H:%M:%S}；日志参考修正后的 diagnostics 定向校准完成于 {datetime.fromisoformat(calibration['completed_utc']).astimezone(zone):%H:%M:%S}。", "",
             "本次新增 29 个代表负载，覆盖原套件遗漏的 15 个模块及 test/log 扩展。每个负载都有 TX、C++、Java、Python 四方实现，预热 1 轮、正式 5 轮、轮换语言顺序；每次使用新进程和临时目录，逐项核对固定校验值。表中是程序内部计时中位数，单位 ms；不含进程启动。", "",
             "同后端 C++ 对照用于分离 TX 公开调用路径与原生调用的开销，不能当作另一套独立数据库或密码学实现。Java/Python 使用各自标准库及驱动；下方逐项说明不同语义、缓存和生命周期。Java 是短命 JVM 负载，未做 JMH 式充分 JIT 预热，不能外推为长驻服务器的稳态表现。", "",
             "## 四语言结果", ""]
    lines += table(["负载", "TX", "C++", "Java", "Python", "TX/C++", "TX/Java", "TX/Python"], rows)
    lines += ["## ≥3× 清单", "", "按未四舍五入的同组中位数比值筛选；同一负载对不同语言的条目不能累计为独立热点。", ""]
    lines += table(["项目", "参考", "TX ms", "参考 ms", "倍率"], [[row["name"], row["reference"],
        f"{row['tx_ms']:.3f}", f"{row['reference_ms']:.3f}", f"{row['ratio']:.2f}×"] for row in gaps])
    lines += ["## 工作量与可比边界", ""]
    lines += table(["负载", "模块", "工作量", "参考及边界"], [[name, *value] for name, value in details.items()])
    lines += ["## 版本、复现和证据", "",
        "- Windows 11 26200，Ryzen 7 6800H，8 核 / 16 线程；GCC 13.1.0 / C++23 -O3，TX clang 23.1.2 -O3，Java 23.0.2，Python 3.12.10。",
        "- TX/C++ SQLite 3.53.4、libpq 18.4；Java sqlite-jdbc 3.46.1.0、postgresql 42.7.4、BouncyCastle 1.78.1、slf4j-api 2.0.16；Python SQLite 3.49.1、psycopg 3.2.3（随包 libpq 14.12）、cryptography 48.0.1、argon2-cffi 25.1.0。版本不同属于结果边界。",
        "- 所有网络地址均为本机；数据库为辅助脚本建立的临时 TLS PostgreSQL 18.4。服务停止、临时数据目录清理均由上下文管理器完成。秘密夹具只在 tx_build 中，未归档、打印或写入报告。",
        "- 主采样与校准期间各自的源码、TX 工具链、参考二进制及 Java 依赖指纹均未改变；参见 [manifest.json](manifest.json)、[completed.json](completed.json)、[calibration_manifest.json](calibration_manifest.json)、[calibration_completed.json](calibration_completed.json)。",
        "- [results.json](results.json) 保留最初正式样本；[calibration_results.json](calibration_results.json) 为修正 C++ 过滤日志对照后的完整 diagnostics 五轮样本；[final_results.json](final_results.json) 以校准组覆盖原组，是本报告唯一数据源。最初简化日志参考不进入最终清单。",
        "- 准备入口：`python -X utf8 -B benchmarks/stdlib_full_2026-09-30/prepare.py`；正式入口：`python -X utf8 -B benchmarks/stdlib_full_2026-09-30/run.py`；汇总入口：`python -X utf8 -B benchmarks/stdlib_full_2026-09-30/summarize.py`。脚本拒绝覆盖完成记录；再次完整复测应使用新的归档目录。",
        "- 补测为所有模块提供代表性能负载，不代表覆盖每个 API、失败路径、长时间负载或所有网络协议。profile 的完整 CPU/分配采样开销、日志轮转压力等不由本次代表负载证明。", ""]
    write(source / "README.md", lines)
    old_gaps = read(full / "gaps.json")
    merged = sorted(old_gaps + gaps, key=lambda row: row["ratio"], reverse=True)
    (full / "all_gaps.json").write_text(json.dumps(merged, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    all_lines = ["# 本轮完整 ≥3× 差距清单（含新增标准库）", "",
        "倍率 = TX 中位耗时 / 对照中位耗时。只筛选未四舍五入值 ≥3 的条目；每条都保留对应的实现和语义边界。不同套件或语言的同名项目不是独立热点。", "",
        "新增负载来源为 [final_results.json](../stdlib_full_2026-09-30/final_results.json)，各自具备 C++、Java、Python 对照。历史套件原有缺失的个别语言实现仍显示在原报告中，没有伪造缺失倍率。", ""]
    all_lines += table(["套件", "项目", "参考", "TX ms", "参考 ms", "倍率", "边界"],
        [[row["suite"], row["name"], row["reference"], f"{row['tx_ms']:.6f}", f"{row['reference_ms']:.6f}",
          f"{row['ratio']:.2f}×", row["note"]] for row in merged])
    write(full / "all_gaps.md", all_lines)
    old_mapping = {
        "algorithm": "algorithm_sort", "array": "library/array", "bytes": "bytes_hex",
        "cancel": "cancel_status", "cbor": "cbor_roundtrip", "crypto": "crypto_sha256", "csv": "csv_parse",
        "debug": "debug_location", "decimal": "decimal_add", "dictionary": "dictionary_contains",
        "encoding": "encoding_utf8", "env": "env_get", "error": "error_stack", "file": "library/file",
        "file_stream": "file_stream_rw", "format": "format_text / format_contract", "fs": "library/fs",
        "httpx": "httpx_get", "io": "library/io", "json": "json_parse", "log": "log_event",
        "math": "math_sqrt", "parse": "parse_int", "path": "library/path", "process": "process_spawn",
        "random": "random_int / random_long", "regex": "regex_search", "requests": "requests_get",
        "serde": "serde_json / serde_short_text", "statistics": "statistics_mean（校准）",
        "string": "library/string", "system": "system_os", "test": "test_assert（不进入差距排名）",
        "time": "library/time", "unicode": "unicode_nfc", "websocket": "websocket_echo", "xml": "xml_parse",
    }
    mapping = {module: {"existing": load, "added": []} for module, load in old_mapping.items()}
    for name, (modules, _, _) in details.items():
        for module in modules.split(","):
            mapping.setdefault(module, {"existing": "—", "added": []})["added"].append(name)
    public = {path.stem for path in (root / "tx/stdlib").glob("*.txh")}
    assert set(mapping) == public, {"missing": sorted(public - set(mapping)), "extra": sorted(set(mapping) - public)}
    coverage = ["# 52 个公开模块的本轮性能覆盖映射", "",
        "原有性能负载覆盖 37 个模块，本轮新增 15 个遗漏模块，并扩展 test/log。下表逐模块列出实际运行的负载；有导入但没有独立工作量的辅助模块不单独计入新增覆盖。代表负载覆盖不等于所有 API 全覆盖。", ""]
    coverage += table(["模块", "原有本轮负载", "新增四语言负载"],
                      [[name, mapping[name]["existing"], ", ".join(mapping[name]["added"]) or "—"] for name in sorted(mapping)])
    write(full / "coverage.md", coverage)
    (full / "coverage.json").write_text(json.dumps(mapping, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    summary = ["# 本轮全量性能结果（含新增标准库）", "",
        f"当前提交 `{manifest['head']}`。已重建工具链并完成原有完整套件，以及新增标准库 29 个四语言负载。52/52 个公开模块均有本轮代表负载，见 [覆盖映射](coverage.md)。", "",
        "- [原有完整套件报告](README.md)：语言、标准库、模块、网络、外部题、综合、启动/编译/内存及优化契约；两道外部题 89 组正式数据核对通过。",
        "- [新增标准库四语言报告](../stdlib_full_2026-09-30/README.md)：29 项，每项都有 TX、C++、Java、Python 对照，包含正式样本与修正日志对照后的定向校准。",
        "- [全部 ≥3× 差距](all_gaps.md) / [JSON](all_gaps.json)：合并旧套件和新增套件，保留对照边界，未将同名重复采样累计为独立热点。", "",
        "## 新增标准库中达到 3× 的负载", "", "单位 ms；粗体倍率表示 ≥3×。C++ 同后端项目包括数据库、安全、日志和 profile，具体含义见补测报告。", ""]
    highlights = []
    for group, data in results.items():
        for name, cases in data["cases"].items():
            ratios = [cases["TX"]["median_ms"] / cases[label]["median_ms"] for label in ("C++", "Java", "Python")]
            if max(ratios) >= 3:
                highlights.append([name, f"{cases['TX']['median_ms']:.3f}",
                    *[(f"**{ratio:.2f}×**" if ratio >= 3 else f"{ratio:.2f}×") for ratio in ratios]])
    summary += table(["项目", "TX ms", "TX/C++", "TX/Java", "TX/Python"], highlights)
    summary += ["优先关注无竞争锁的值/guard 接口、原子 RMW、被过滤的惰性日志、通道、数据库逐行读取和 TLS 握手。测试框架成功回调也有较大差距，但 C++ 参考不包含 TX 的受检回调和错误上下文。", "",
        "## 原有套件仍有明显差距的代表项", ""]
    chosen = [("语言特性", "deinit"), ("综合", "serde_short_text"), ("契约/format_contract", "format_parameter"),
              ("契约/format_contract", "format_alternating"), ("语言特性", "string_conversion"),
              ("综合", "serde_long_text"), ("audit/compute", "regex_search"), ("契约/graph_contract", "graph_copy"),
              ("综合", "encoding_literal"), ("综合", "encoding_dynamic")]
    selected = [next(row for row in old_gaps if row["suite"] == group and row["name"] == name and row["reference"] == "C++")
                for group, name in chosen]
    summary += table(["项目", "TX ms", "C++ ms", "倍率"], [[row["name"], f"{row['tx_ms']:.3f}",
        f"{row['reference_ms']:.3f}", f"{row['ratio']:.2f}×"] for row in selected])
    summary += ["原有综合程序墙钟中位数为 TX 126.224 ms、C++ 59.731 ms，即 2.11×；它只是该综合程序，不能代表全库都低于 3×。", "",
        "单节点自环 GC、固定形状深拷贝、固定格式拼接等原始倍率也超过 3×，但参考简化了语义；完整清单明确注明这些边界，不把它们解释成通用机制性能。", "",
        "本次修改限于基准及报告，未修改编译器/标准库实现。build/ 已按正式构建流程清理；原始未提交的上一轮归档保留。", ""]
    write(full / "FULL_REPORT.md", summary)
    print(f"REPORT COMPLETE: 52 modules, 29 added four-language cases, {len(gaps)} new gap rows, {len(merged)} total gap rows")


if __name__ == "__main__":
    main()
