"""只用本轮原始样本生成完整报告和 >=3 倍清单。"""
from datetime import datetime, timedelta, timezone
import json

from run import archive, load, previous, root, stdlib_archive


def read(path):
    return json.loads(path.read_text(encoding="utf-8"))


def save(path, value):
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def write(path, lines):
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def table(headers, rows):
    return ["| " + " | ".join(headers) + " |", "| " + " | ".join(["---"] * len(headers)) + " |",
            *["| " + " | ".join(map(str, row)) + " |" for row in rows], ""]


def gap_table(rows):
    return table(["套件", "项目", "参考", "TX ms", "参考 ms", "倍率", "可比边界"],
                 [[row["suite"], row["name"], row["reference"], f"{row['tx_ms']:.6f}",
                   f"{row['reference_ms']:.6f}", f"{row['ratio']:.2f}×", row["note"]] for row in rows])


def main():
    base = load("committed_base_summary", root / "benchmarks/performance_retest_2026-09-30_after_completion/summarize.py")
    base.out = archive
    base.previous = previous
    base.completion = previous

    def current_write(name, lines):
        lines = [line for line in lines if not line.startswith((
            "单节点自环 GC 17.60×", "接近阈值的编码项目随采样", "`map_hit_128` 的校准轮变慢"))]
        lines = [line.replace("../performance_retest_2026-09-30/README.md", "../performance_retest_2026-09-30_full_rerun/README.md")
                 .replace("benchmarks/performance_retest_2026-09-30_after_completion/run.py", "benchmarks/performance_retest_2026-09-30_committed/run.py")
                 .replace("新增契约使用上一轮专项记录", "新增契约使用上一轮全量记录") for line in lines]
        lines = [("本页覆盖原有套件；新增标准库 29 项四语言负载见 [标准库报告](../stdlib_retest_2026-09-30_committed/README.md)。"
                  "52 个公开模块的代表负载及完整差距汇总见 [总报告](FULL_REPORT.md)。")
                 if line.startswith("“全量”为现有性能套件") else line for line in lines]
        write(archive / name, lines)

    base.write = current_write
    base.main()
    details = load("stdlib_details", root / "benchmarks/stdlib_full_2026-09-30/summarize.py").details
    results = read(stdlib_archive / "results.json")
    manifest = read(stdlib_archive / "manifest.json")
    completed = read(stdlib_archive / "completed.json")
    assert completed["source_toolchain_and_references_unchanged"]
    assert manifest["head"] == read(archive / "manifest.json")["head"]
    assert completed["cases"] == len(details) == 29
    rows, new_gaps = [], []
    for group, data in results.items():
        for name, cases in data["cases"].items():
            assert set(cases) == {"TX", "C++", "Java", "Python"}
            assert all(len(case["samples_ms"]) == 5 for case in cases.values())
            assert len({case["checksum"] for case in cases.values()}) == 1
            tx = cases["TX"]["median_ms"]
            ratios = {label: tx / cases[label]["median_ms"] for label in ("C++", "Java", "Python")}
            rows.append([name, *[f"{cases[label]['median_ms']:.3f}" for label in ("TX", "C++", "Java", "Python")],
                         *[f"{ratio:.2f}×" for ratio in ratios.values()]])
            for label, ratio in ratios.items():
                if ratio >= 3:
                    new_gaps.append({"suite": "新增/" + group, "name": name, "reference": label,
                                     "tx_ms": tx, "reference_ms": cases[label]["median_ms"],
                                     "ratio": ratio, "note": details[name][2].replace("采用校准样本：", "")})
    new_gaps.sort(key=lambda row: row["ratio"], reverse=True)
    save(stdlib_archive / "gaps.json", new_gaps)
    write(stdlib_archive / "README.md", ["# 提交后标准库完整复测", "",
          f"版本 `{manifest['head']}`。预热 1 轮、正式 5 轮，轮换四语言执行顺序，内部计时中位数，单位 ms。",
          "29 项全部完成，逐轮校验值一致。使用当前已修正的日志参考，不混入历史校准样本。",
          "同后端 C++ 对照衡量公开调用路径成本；Java 为短命 JVM，不能外推到充分预热的稳态服务器。", "",
          *table(["负载", "TX", "C++", "Java", "Python", "TX/C++", "TX/Java", "TX/Python"], rows),
          "## ≥3× 项目", "", *gap_table(new_gaps), "## 工作量及边界", "",
          *table(["负载", "模块", "工作量", "边界"],
                 [[name, *[text.replace("采用校准样本：", "") for text in value]] for name, value in details.items()]),
          "原始数据见 [results.json](results.json)、[manifest.json](manifest.json)、[completed.json](completed.json)。",
          "采样期间源码、工具链和参考程序指纹未变化。服务为本机临时实例，秘密夹具不进入归档。"])
    gaps = sorted(read(archive / "gaps.json") + new_gaps, key=lambda row: row["ratio"], reverse=True)
    save(archive / "all_gaps.json", gaps)
    write(archive / "all_gaps.md", ["# 本轮全部 ≥3× 差距", "",
          "倍率 = 同组 TX 中位耗时 / 参考中位耗时，按未四舍五入的比值筛选 ≥3。",
          "同名项目在不同套件/语言中的记录不累计为独立热点；保留工作量和语义边界。", "", *gap_table(gaps)])
    cpp = [row for row in gaps if "C++" in row["reference"]]
    unique = {}
    for row in cpp:
        unique.setdefault(row["name"], row)
    save(archive / "cpp_gaps_by_name.json", list(unique.values()))
    coverage = read(previous / "coverage.json")
    assert set(coverage) == {path.stem for path in (root / "tx/stdlib").glob("*.txh")}
    assert {name for data in results.values() for name in data["cases"]} == set(details)
    save(archive / "coverage.json", coverage)
    write(archive / "coverage.md", ["# 本轮 52 个公开模块的代表负载", "",
          "复用既有覆盖映射，并已在本轮执行原有套件及新增全部 29 个负载；不表示所有 API 和失败路径全覆盖。", "",
          *table(["模块", "原有负载", "新增负载"], [[name, value["existing"], ", ".join(value["added"]) or "—"]
                 for name, value in sorted(coverage.items())])])
    summary = read(archive / "summary.json")
    zone = timezone(timedelta(hours=8))
    start = datetime.fromisoformat(read(archive / "manifest.json")["started_utc"]).astimezone(zone)
    end = datetime.fromisoformat(completed["completed_utc"]).astimezone(zone)
    write(archive / "FULL_REPORT.md", ["# 提交后全量性能测试结果", "",
          f"测试提交：`{manifest['head']}`。先提交，再正式重建工具链，随后串行执行所有既有性能套件。",
          f"采样时间（北京时间）：{start:%Y-%m-%d %H:%M:%S} 至 {end:%H:%M:%S}。",
          "52/52 个公开模块有代表性能负载。新增标准库 29 项均为四语言各 5 轮；原有套件维持原轮数，包含语言、库、网络、外部题、综合、启动/编译/内存、优化专项及契约对照。",
          "两道外部题全部 89 组正式答案核对通过；两套完成记录均确认采样期间源码与工具链指纹不变。", "",
          f"综合程序墙钟中位数：TX **{summary['tx_process_wall_ms']:.3f} ms**，C++ **{summary['cpp_process_wall_ms']:.3f} ms**，倍率 **{summary['tx_cpp_process_ratio']:.2f}×**。这不是全库平均倍率。", "",
          f"全部参考语言共有 {len(gaps)} 条 ≥3× 记录，按名称去重 {len({row['name'] for row in gaps})} 项；其中 C++ 对照 {len(cpp)} 条、按名称去重 **{len(unique)} 项**。", "",
          "## 对 C++ 达到 3× 的全部项目", "",
          "同名有多组采样时，下表展示最高倍率及其对应套件，避免重复计数；主测与 GCC/clang 配对轮的完整记录见全部清单。临界值不能视为稳定超过阈值。", "",
          *gap_table(list(unique.values())), "## 完整数据", "",
          "- [原有套件逐项报告](README.md)",
          "- [新增标准库四语言逐项报告](../stdlib_retest_2026-09-30_committed/README.md)",
          "- [全部 ≥3× 记录及语义边界](all_gaps.md)",
          "- [52 模块覆盖映射](coverage.md)",
          "- [与上一轮 TX 耗时比较](comparison.md)", "",
          "通用图复制、确定性析构、动态实参绑定及 guard 生命周期的成本不能由简化 C++ 参考完全分离。",
          "HTTP 为客户端端到端对照，数据库跨语言驱动及缓存策略不同；短项和网络测量受调度扰动，本轮不据此直接断言根因。",
          "本轮未修改编译器或标准库实现，仅修正汇总脚本的历史假设并新增复测归档。"])
    print(f"FULL REPORT COMPLETE: {len(gaps)} gap rows; {len(unique)} distinct C++ gap names", flush=True)


if __name__ == "__main__":
    main()
