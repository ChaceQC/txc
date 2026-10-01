"""对齐已有 Windows/Linux TX 性能归档，不重新运行负载。"""
from pathlib import Path
import json
import statistics

root = Path(__file__).resolve().parents[1]
linux = root / "benchmarks/linux_full_2026-10-01"
windows = root / "benchmarks/performance_retest_2026-09-30_static_execution"
stdlib = root / "benchmarks/stdlib_retest_2026-09-30_static_execution"


def read(path):
    return json.loads(path.read_text(encoding="utf-8"))


def main():
    win = {}
    language = read(windows / "language_features.json")
    win["language"] = {name: statistics.median(samples) for name, samples in language["TX"].items()}
    audit = read(windows / "performance_retest.json")
    win["library"] = audit["library"]["results"]["medians_ms"]["TX"]
    for section in ("audit", "network"):
        for name, data in audit[section]["results"].items():
            if "TX" in data["medians_ms"]:
                win["ws" if name == "websocket" else name] = data["medians_ms"]["TX"]
    win["diverse"] = {name: data["TX"]["median_ms"] for name, data in read(windows / "diverse.json")["diverse"]["cases"].items()}
    for name, data in read(windows / "special.json").items():
        win[name] = {"random_long": statistics.median(data["samples_ms"]["TX"])} if name == "random_long" else data["medians_ms"]
    for name, label in (("graph_contract", "tx_runtime"), ("format_contract", "tx")):
        win[name] = read(windows / (name + ".json"))["medians_ms"][label]
    for name, data in read(stdlib / "results.json").items():
        win[name] = {case: values["TX"]["median_ms"] for case, values in data["cases"].items()}
    for name, data in audit["external"]["results"]["tasks"].items():
        name = {"mini": "mini-filesystem", "stage": "not-yet-on-stage"}.get(name, name)
        win[name] = {case: values["median_ms"]["TX"] for case, values in data.items()}
    ldata = read(linux / "final_results.json")
    primary = read(linux / "results.json")
    wmanifest = read(windows / "manifest.json")
    lmanifest = read(linux / "manifest.json")
    whashes = {key.replace("\\", "/"): value for key, value in wmanifest["sha256"].items()}
    whashes.update({key.replace("\\", "/"): value for key, value in read(stdlib / "manifest.json")["sha256"].items()})
    lhashes = lmanifest["sha256"]
    rows, missing = [], []
    for suite, data in ldata.items():
        timings = data.get("medians_ms", {name: value["medians_ms"] for name, value in data.get("cases", {}).items()})
        for name, values in timings.items():
            if suite == "library" and name == "dict_hash":
                continue
            name = "dict" if suite == "library" and name == "dict_dynamic" else name
            if name not in win.get(suite, {}):
                missing.append([suite, name])
                continue
            lt = values.get("TX", values.get("TX-runtime"))
            wt = win[suite][name]
            source = primary.get(suite, {}).get("source", data.get("source"))
            same = bool(source and whashes.get(source) == lhashes.get(source) and source in whashes)
            note = ""
            if suite == "system":
                note = "进程路径已移植；debug_location/system_os 的输出长度随平台改变"
            elif suite == "postgres":
                note = "服务器 Windows 18.4 / Linux 16.15；临时 TLS 数据库"
            elif name == "x509_parse_der":
                note = "独立生成的证书夹具；比较相同次数，字节长度可能不同"
            elif name in ("file", "fs", "file_stream_rw", "async_file_rw", "log_file", "process_spawn") or suite in ("file_io", "network", "http", "ws"):
                note = "系统/网络/文件负载；Linux 为 WSL2 /mnt/e，平台后端也不同"
            elif suite == "graph_contract":
                note = "直接 TX runtime 契约；仅内存采集移植为 getrusage"
            elif suite == "library":
                note = "TX 使用整数毫秒；短项比例精度有限"
            rows.append({"suite": suite, "case": name, "windows_ms": wt, "linux_ms": lt,
                         "linux_over_windows": lt / wt if wt else None,
                         "change_percent": (lt / wt - 1) * 100 if wt else None,
                         "workload_source_hash_equal": same, "note": note})
    groups = []
    for suite in ldata:
        selected = [row for row in rows if row["suite"] == suite and row["linux_over_windows"] is not None]
        if selected:
            groups.append({"suite": suite, "cases": len(selected),
                "median_ratio": statistics.median(row["linux_over_windows"] for row in selected),
                "faster_10_percent": sum(row["linux_over_windows"] < 0.9 for row in selected),
                "slower_10_percent": sum(row["linux_over_windows"] > 1.1 for row in selected)})
    selected = [row for row in rows if row["linux_over_windows"] is not None]
    summary = {"matched": len(rows), "missing": missing, "windows_head": wmanifest["head"], "linux_worktree_head": lmanifest["head"],
               "faster_10_percent": sum(row["linux_over_windows"] < 0.9 for row in selected),
               "slower_10_percent": sum(row["linux_over_windows"] > 1.1 for row in selected),
               "groups": groups, "rows": rows}
    resources = read(windows / "resource_comparison.json")
    lresources = read(linux / "extra_resources.json")
    lines = ["# Linux 与 Windows TX 性能归档对照", "",
        "本报告只读取已有结果，没有重新运行性能测试。比较的是两轮实际测量，不是同一提交、同一工具链的操作系统 A/B 实验。", "",
        f"- Windows：2026-09-30，提交 `{wmanifest['head']}`，Windows 11 26200；Clang 23.1.2。",
        f"- Linux：2026-10-01，工作树 `{lmanifest['head']}` 的现有 Linux 包；WSL2 Ubuntu 24.04，Clang 18.1.3，程序位于 `/mnt/e`。",
        "- Windows 原有主套件多为 7 轮，新增标准库/契约为 5 轮；Linux 为 5 轮。均取中位数。Linux 基础实现已经过平台适配，差异不能单独归因于操作系统。",
        f"- 对齐 {len(rows)} 个 TX 条目；以 ±10% 作描述性区间，Linux 耗时降低超过 10% 的有 {summary['faster_10_percent']} 项、增加超过 10% 的有 {summary['slower_10_percent']} 项。不同专项重复出现相似负载，这不是独立统计样本或显著性检验。",
        "- 下表 L/W 小于 1 表示 Linux 更快，大于 1 表示 Linux 更慢。分组中位倍率是逐项倍率的中位数，不是总耗时比或语言综合得分。", "",
        "## 分组对比", "", "| 组 | 条目数 | L/W 中位倍率 | Linux 快 >10% | Linux 慢 >10% |", "| --- | ---: | ---: | ---: | ---: |"]
    for group in groups:
        lines.append(f"| {group['suite']} | {group['cases']} | {group['median_ratio']:.2f}× | {group['faster_10_percent']} | {group['slower_10_percent']} |")
    lines += ["", "## 启动、进程墙钟和内存", "",
              "进程墙钟包含启动及输出，不能和内部计时混用。Linux GNU time 峰值 RSS 与 Windows 2 ms 采样的进程树工作集口径不同，内存值只并列展示。", "",
              "| 指标 | Windows | Linux |", "| --- | ---: | ---: |"]
    for suite, key in (("空程序启动", "startup"), ("进程运行", "diverse")):
        for metric, unit, lfield, scale in (("wall_ms", "ms", "wall_seconds", 1000), ("sampled_tree_peak_mib", "MiB", "peak_rss_kib", 1/1024)):
            value = next(row["tx"] for row in resources if row["suite"] == suite and row["metric"] == metric)
            lv = statistics.median(row[lfield] for row in lresources[key]) * scale
            lines.append(f"| {suite} {unit} | {value:.3f} | {lv:.3f} |")
    lines += ["", "Linux 完整编译仅记录每个程序一次，含链接和复制运行库；Windows 编译基准的构建阶段/重复次数不同，本报告不制造编译速度倍率。", "",
              "## 全部同名负载", "", "| 组 | 负载 | Windows ms | Linux ms | L/W | 变化 | 说明 |", "| --- | --- | ---: | ---: | ---: | ---: | --- |"]
    for row in rows:
        ratio = "—" if row["linux_over_windows"] is None else f"{row['linux_over_windows']:.2f}×"
        change = "—" if row["change_percent"] is None else f"{row['change_percent']:+.1f}%"
        lines.append(f"| {row['suite']} | {row['case']} | {row['windows_ms']:.3f} | {row['linux_ms']:.3f} | {ratio} | {change} | {row['note']} |")
    lines += ["", "## 解读边界", "",
        "计算、容器和对象操作的差异同时受编译器版本、生成代码及 CPU/调度状态影响。文件、日志、异步 I/O 和启动受到 /mnt/e 跨文件系统及共享库加载影响，不能由这轮数据认定原生 Linux 文件性能更差。PostgreSQL 主版本不同；网络是端到端回环负载，包含服务端线程调度。",
        "原始来源：Windows `performance_retest_2026-09-30_static_execution`、`stdlib_retest_2026-09-30_static_execution`；Linux `final_results.json`。机器可读结果及逐项源码哈希一致性字段见 `windows_comparison.json`；哈希一致仅说明负载源码相同，不说明编译器实现相同。", ""]
    (linux / "windows_comparison.json").write_text(json.dumps(summary, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    (linux / "windows_comparison.md").write_text("\n".join(lines), encoding="utf-8")
    print(json.dumps({key: value for key, value in summary.items() if key != "rows"}, ensure_ascii=False))


if __name__ == "__main__":
    main()
