"""生成本轮完整对比文档，所有数值来自本轮原始样本。"""

from datetime import datetime, timedelta, timezone
import json

from run import archive, load, previous, root, stdlib_archive
from comparison_pairs import collect


def read(path):
    return json.loads(path.read_text(encoding="utf-8"))


def save(name, value):
    (archive / name).write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n",
                               encoding="utf-8")


def write(path, lines):
    path.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")


def table(headers, rows):
    return ["| " + " | ".join(headers) + " |",
            "| " + " | ".join(["---"] * len(headers)) + " |",
            *["| " + " | ".join(map(str, row)) + " |" for row in rows], ""]


def embed(lines):
    return ["#" + line if line.startswith("#") else line for line in lines]


def main():
    report = load("static_execution_report", previous / "summarize.py")

    def current_write(path, lines):
        lines = [line.replace("../performance_retest_2026-09-30_full_rerun/README.md",
                              "../performance_retest_2026-09-30_committed/README.md")
                 .replace("../stdlib_retest_2026-09-30_committed/",
                          "../stdlib_retest_2026-09-30_static_execution/")
                 .replace("benchmarks/performance_retest_2026-09-30_committed/run.py",
                          "benchmarks/performance_retest_2026-09-30_static_execution/run.py")
                 .replace("本轮未修改编译器或标准库实现，仅修正汇总脚本的历史假设并新增复测归档。",
                          "正式采样前补齐 ThinLTO emutls 配置，密码学初始化改用线程安全局部静态对象；头文件引入 call_once 外部 TLS 的编译单元保留原生对象。详见 [构建预检](preflight_failure.md)。")
                 .replace("上一轮专项 TX", "上一轮 TX") for line in lines]
        write(path, lines)

    report.write = current_write
    report.main()
    manifest = read(archive / "manifest.json")
    completed = read(archive / "completed.json")
    stdlib_manifest = read(stdlib_archive / "manifest.json")
    stdlib_completed = read(stdlib_archive / "completed.json")
    assert completed["source_and_toolchain_unchanged"]
    assert completed["reference_programs_and_external_inputs_unchanged"]
    assert stdlib_completed["source_toolchain_and_references_unchanged"]
    for current in (manifest, stdlib_manifest):
        assert "tx\\libtxstdlib_lto.a" in current["sha256"]
        assert "tx\\link\\ld.lld.exe" in current["sha256"]
    old_stdlib = read(root / "benchmarks/stdlib_retest_2026-09-30_committed/results.json")
    new_stdlib = read(stdlib_archive / "results.json")
    comparisons = read(archive / "comparison.json")
    stdlib_comparisons = []
    for group, data in new_stdlib.items():
        for name, cases in data["cases"].items():
            before = old_stdlib[group]["cases"][name]["TX"]["median_ms"]
            after = cases["TX"]["median_ms"]
            stdlib_comparisons.append({"suite": "新增/" + group, "name": name,
                "previous_ms": before, "current_ms": after,
                "change_percent": (after / before - 1) * 100,
                "note": "上一轮为 da798cd 提交后的完整采样"})
    comparisons.extend(stdlib_comparisons)
    save("comparison.json", comparisons)
    write(archive / "comparison.md", ["# 本轮与上一轮的全部 TX 耗时对比", "",
          "上一轮为 da798cd 的完整采样；本轮为静态执行体系及 ThinLTO 提交后的采样。",
          "单位 ms；负值表示变快。两轮机器状态不同，跨轮差值不单独证明优化或回退的因果。", "",
          *table(["套件", "项目", "上一轮 TX", "本轮 TX", "变化"],
                 [[row["suite"], row["name"], f"{row['previous_ms']:.6f}",
                   f"{row['current_ms']:.6f}", f"{row['change_percent']:+.1f}%"
                   if row["change_percent"] is not None else "—"] for row in comparisons])])
    diverse = read(archive / "diverse.json")
    resource_rows = []
    for group, data in (("进程运行", diverse["diverse"]["processes"]),
                        ("空程序启动", diverse["startup"]), ("编译", diverse["compilation"])):
        tx_label = "TX full" if group == "编译" else "TX"
        refs = ("C++ full", "Java full") if group == "编译" else ("C++", "Java", "Python")
        for metric, unit in (("wall_ms", "ms"), ("sampled_tree_peak_mib", "MiB")):
            tx = data[tx_label][metric]["median"]
            for label in refs:
                reference = data[label][metric]["median"]
                ratio = tx / reference if reference and tx is not None else None
                resource_rows.append({"suite": group, "metric": metric, "unit": unit,
                    "tx": tx, "reference": label, "reference_value": reference, "ratio": ratio})
    save("resource_comparison.json", resource_rows)
    gaps = read(archive / "all_gaps.json")
    details = load("static_execution_details", root / "benchmarks/stdlib_full_2026-09-30/summarize.py").details
    pairs = collect(archive, stdlib_archive, read, details)
    by_key = {(row["suite"], row["name"], row["reference"]): row for row in pairs}
    assert len(by_key) == len(pairs)
    assert {(row["suite"], row["name"], row["reference"]) for row in gaps} == {
        key for key, row in by_key.items() if row["ratio"] is not None and row["ratio"] >= 3}
    for row in gaps:
        pair = by_key[(row["suite"], row["name"], row["reference"])]
        assert pair["tx_ms"] == row["tx_ms"] and pair["reference_ms"] == row["reference_ms"]
    save("all_comparisons.json", pairs)
    summary = read(archive / "summary.json")
    full_minutes = (datetime.fromisoformat(stdlib_completed["completed_utc"])
                    - datetime.fromisoformat(manifest["started_utc"])).total_seconds() / 60
    summary.update({"core_measure_minutes": summary["measure_minutes"],
                    "measure_minutes": full_minutes,
                    "all_suites_completed": True, "public_modules": 52,
                    "additional_stdlib_cases": stdlib_completed["cases"],
                    "cross_language_comparison_rows": len(pairs),
                    "comparison_rows": len(comparisons), "all_gap_rows": len(gaps),
                    "distinct_cpp_gap_names": len(read(archive / "cpp_gaps_by_name.json")),
                    "stdlib_completed_utc": stdlib_completed["completed_utc"],
                    "resource_gap_rows": sum(row["ratio"] is not None and row["ratio"] >= 3
                                             for row in resource_rows)})
    save("summary.json", summary)
    core_lines = (archive / "README.md").read_text(encoding="utf-8").splitlines()
    stdlib_lines = (stdlib_archive / "README.md").read_text(encoding="utf-8").splitlines()
    full_lines = (archive / "FULL_REPORT.md").read_text(encoding="utf-8").splitlines()
    zone = timezone(timedelta(hours=8))
    started = datetime.fromisoformat(manifest["started_utc"]).astimezone(zone)
    ended = datetime.fromisoformat(stdlib_completed["completed_utc"]).astimezone(zone)
    full_lines[0] = "# 静态执行体系提交后的全量性能对比报告"
    full_lines += ["", "## 测量方法及完整性", "",
          f"采样从 {started:%Y-%m-%d %H:%M:%S} 至 {ended:%Y-%m-%d %H:%M:%S}（北京时间），"
          f"共 {(ended-started).total_seconds()/60:.2f} 分钟。",
          "默认 TX 程序使用 clang -O3、ThinLTO 及 ld.lld；C++ 参考使用现有 -O3 Release 配置，"
          "校准组分别使用 GCC 和同版本 clang。C++ ThinLTO 编译使用 -femulated-tls，"
          "链接使用 --plugin-opt=-emulated-tls；密码学一次性初始化使用线程安全局部静态对象。"
          "ws_client.cpp 因 libstdc++ 外部 TLS 依赖保留 GCC Release 原生对象；"
          "标准库其余 316 个模块使用 bitcode，第三方依赖继续使用原生对象。"
          "图契约直接链接普通 TX 运行时静态库。",
          "本轮额外记录 ThinLTO 静态库、LLVM bitcode、ld.lld 和启动对象的 SHA-256，"
          "两套采样完成后均检查输入指纹不变；全部数据使用本轮结果，不混入历史校准值。",
          "跨语言倍率 = 同组 TX 中位耗时 / 对应参考中位耗时，按未四舍五入值筛选 ≥3；"
          "小于 1 表示 TX 更快。启动、编译和内存另列，避免与内部耗时混算。",
          "全量指执行仓库全部现有性能套件，以及覆盖 52 个公开标准库模块的代表负载。"
          "不表示每个 API、失败路径、并发饱和吞吐或生产环境均被测量。", "",
          "## 进程、启动、编译和内存的跨语言倍率", "",
          "内存为每 2 ms 轮询进程树工作集的近似峰值；缺失或零值不计算倍率。"
          "TX check 与完整编译、Python 字节码与原生编译的工作不同，保留在逐项表中，不计算对应倍率。", "",
          *table(["范围", "指标", "TX", "参考", "参考值", "TX/参考", "≥3×"],
                 [[row["suite"], "墙钟 ms" if row["unit"] == "ms" else "峰值 MiB",
                   f"{row['tx']:.3f}" if row["tx"] is not None else "—", row["reference"],
                   f"{row['reference_value']:.3f}" if row["reference_value"] is not None else "—",
                   f"{row['ratio']:.2f}×" if row["ratio"] is not None else "—",
                   "是" if row["ratio"] is not None and row["ratio"] >= 3 else ""]
                  for row in resource_rows]),
          "## 全部既有套件逐项对比", "",
          *embed(core_lines[core_lines.index("## 覆盖与采样"):]), "",
          "## 29 项标准库四语言完整对比", "",
          *embed(stdlib_lines[2:]), "",
          "## 与上一轮的全部 TX 耗时变化", "",
          *table(["套件", "项目", "上一轮 TX ms", "本轮 TX ms", "变化"],
                 [[row["suite"], row["name"], f"{row['previous_ms']:.6f}",
                   f"{row['current_ms']:.6f}", f"{row['change_percent']:+.1f}%"
                   if row["change_percent"] is not None else "—"] for row in comparisons]),
          "## 全部 ≥3× 跨语言耗时记录", "",
          "下表保留每个套件及参考语言；前面的按名称去重表只用于扫描热点。", "",
          *report.gap_table(gaps), "## 全部有效运行时对照倍率", "",
          "包含低于 3× 的对照。零耗时参考不计算倍率；成功断言可被优化消除、旧简单求和参考"
          "和没有参考实现的专项不进入有效倍率表。全部原始耗时仍保留在上方逐项表和 JSON 中。", "",
          *table(["套件", "项目", "参考", "TX ms", "参考 ms", "TX/参考", "边界"],
                 [[row["suite"], row["name"], row["reference"], f"{row['tx_ms']:.6f}",
                   f"{row['reference_ms']:.6f}", f"{row['ratio']:.2f}×"
                   if row["ratio"] is not None else "—", row["note"]] for row in pairs])]
    # 嵌入标准库报告后，原先相对该目录的数据链接需指向实际归档。
    stdlib_data_line = "原始数据见 [results.json](results.json)、[manifest.json](manifest.json)、[completed.json](completed.json)。"
    full_lines = [line.replace(stdlib_data_line,
                  "标准库原始数据见 [results.json](../stdlib_retest_2026-09-30_static_execution/results.json)、"
                  "[manifest.json](../stdlib_retest_2026-09-30_static_execution/manifest.json)、"
                  "[completed.json](../stdlib_retest_2026-09-30_static_execution/completed.json)。")
                  for line in full_lines]
    write(archive / "FULL_REPORT.md", full_lines)
    print(f"COMPLETE: {len(comparisons)} comparison rows, {len(gaps)} >=3x records", flush=True)


if __name__ == "__main__":
    main()
