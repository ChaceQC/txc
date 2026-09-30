"""汇总当前工作区全量样本，沿用既有表格与跨语言可比边界。"""

from datetime import datetime, timedelta, timezone
import json

from run import archive, load, previous, root, source_snapshot, stdlib_archive
import hashlib


def read(path):
    return json.loads(path.read_text(encoding="utf-8"))


def save(name, value):
    (archive / name).write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def write(path, lines):
    path.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")


def table(headers, rows):
    return ["| " + " | ".join(headers) + " |", "| " + " | ".join(["---"] * len(headers)) + " |",
            *["| " + " | ".join(map(str, row)) + " |" for row in rows], ""]


def load_report_module(name, path):
    module = load(name, path)
    if hasattr(module, "details"):
        notes = {
            "sqlite_pool": "TX/C++ 使用当前同一数据库池后端，已证明无会话副作用的只读连接可复用；Java/Python 仍每次关闭连接。跨语言生命周期不同，仅作端到端观察。",
            "sqlite_async": "TX 任务运行时；C++ 同异步数据库后端加单线程执行器；Java/Python 单线程池。TX/C++ 当前可复用安全的只读池连接，Java/Python 仍关闭连接；跨语言不代表相同连接生命周期。",
        }
        for key, note in notes.items():
            module.details[key] = (*module.details[key][:2], note)
    return module


def current_write(path, lines):
    adjusted = []
    for line in lines:
        if line.startswith("#"):
            line = line.replace("提交后", "根因修复工作区")
            line = line.replace("2026-09-30 根因修复工作区", "根因修复工作区")
        line = line.replace("先提交优化及已有记录，再完成正式重建、封包和串行采样。",
                            "该 HEAD 上的未提交根因修复工作区已正式重建、封包后串行采样；版本以 build_manifest.json 中的源码指纹为准。")
        line = line.replace("测试提交：", "基准 HEAD：").replace(
            "先提交，再正式重建工具链，随后串行执行所有既有性能套件。",
            "测试包含未提交根因修复的当前工作区；源码指纹见 build_manifest.json。正式重建后串行采样。")
        line = line.replace("上一轮专项 TX", "上一轮 TX")
        line = line.replace("../performance_retest_2026-09-30_full_rerun/README.md",
                            "../performance_retest_2026-09-30_static_execution/README.md")
        line = line.replace("../stdlib_retest_2026-09-30_committed/", "stdlib/")
        line = line.replace("benchmarks/performance_retest_2026-09-30_committed/run.py",
                            "benchmarks/performance_retest_2026-09-30_root_cause/run.py")
        line = line.replace("本轮未修改编译器或标准库实现，仅修正汇总脚本的历史假设并新增复测归档。",
                            "本轮只新增测试入口与归档，保留原有未提交实现；未修改编译器或标准库实现。")
        if path.parent == stdlib_archive and line.startswith("版本 `"):
            line = line.replace("版本 `", "基准 HEAD `") + " 当前未提交源码身份见上级目录 build_manifest.json。"
        adjusted.append(line)
    write(path, adjusted)


def main():
    report = load("root_cause_base_report", root / "benchmarks/performance_retest_2026-09-30_committed/summarize.py")
    report.load = load_report_module
    report.write = current_write
    report.main()
    manifest = read(archive / "manifest.json")
    completed = read(archive / "completed.json")
    stdlib_manifest = read(stdlib_archive / "manifest.json")
    stdlib_completed = read(stdlib_archive / "completed.json")
    assert completed["source_and_toolchain_unchanged"]
    assert completed["reference_programs_and_external_inputs_unchanged"]
    assert stdlib_completed["source_toolchain_and_references_unchanged"]
    assert source_snapshot() == read(archive / "build_manifest.json")["source_sha256"]
    core_inputs = {key.replace("\\", "/"): value for key, value in manifest["sha256"].items()}
    stdlib_inputs = {key.replace("\\", "/"): value for key, value in stdlib_manifest["sha256"].items()}
    shared = [key for key in core_inputs.keys() & stdlib_inputs.keys()
              if key.startswith(("src/", "cmake/", "tx/")) or key == "CMakeLists.txt"]
    assert all(core_inputs[key] == stdlib_inputs[key] for key in shared)
    core_runner_key = (archive.relative_to(root) / "run.py").as_posix()
    assert hashlib.sha256((archive / "run_core_snapshot.py").read_bytes()).hexdigest() == core_inputs[core_runner_key]
    old_stdlib = read(root / "benchmarks/stdlib_retest_2026-09-30_static_execution/results.json")
    comparisons = read(archive / "comparison.json")
    for group, data in read(stdlib_archive / "results.json").items():
        for name, cases in data["cases"].items():
            before = old_stdlib[group]["cases"][name]["TX"]["median_ms"]
            after = cases["TX"]["median_ms"]
            comparisons.append({"suite": "新增/" + group, "name": name, "previous_ms": before,
                "current_ms": after, "change_percent": (after / before - 1) * 100,
                "note": "与根因修复前完整采样比较；跨轮变化不单独证明因果"})
    save("comparison.json", comparisons)
    comparison_lines = ["# 与根因修复前全量记录的 TX 耗时对比", "",
        "基线为 performance_retest_2026-09-30_static_execution 及其配套标准库全量采样。",
        "单位 ms；负百分比为变快，跨轮机器状态不同，短项、文件和网络负载会受调度扰动。", "",
        *table(["套件", "项目", "上轮 TX ms", "本轮 TX ms", "变化"],
            [[r["suite"], r["name"], f"{r['previous_ms']:.6f}", f"{r['current_ms']:.6f}",
              f"{r['change_percent']:+.1f}%" if r["change_percent"] is not None else "—"] for r in comparisons])]
    write(archive / "comparison.md", comparison_lines)
    details = load_report_module("root_cause_details", root / "benchmarks/stdlib_full_2026-09-30/summarize.py").details
    collector = load("root_cause_comparisons", previous / "comparison_pairs.py")
    pairs = collector.collect(archive, stdlib_archive, read, details)
    gaps = read(archive / "all_gaps.json")
    identity = lambda r: (r["suite"], r["name"], r["reference"])
    assert len({identity(r) for r in pairs}) == len(pairs)
    assert {identity(r) for r in gaps} == {identity(r) for r in pairs if r["ratio"] is not None and r["ratio"] >= 3}
    save("all_comparisons.json", pairs)
    diverse = read(archive / "diverse.json")
    resources = []
    for group, data in (("进程运行", diverse["diverse"]["processes"]),
                        ("空程序启动", diverse["startup"]), ("编译", diverse["compilation"])):
        tx_label = "TX full" if group == "编译" else "TX"
        labels = ("C++ full", "Java full") if group == "编译" else ("C++", "Java", "Python")
        for metric in ("wall_ms", "sampled_tree_peak_mib"):
            tx = data[tx_label][metric]["median"]
            for label in labels:
                reference = data[label][metric]["median"]
                resources.append({"suite": group, "metric": metric, "tx": tx, "reference": label,
                    "reference_value": reference, "ratio": tx / reference if reference and tx is not None else None})
    save("resource_comparison.json", resources)
    summary = read(archive / "summary.json")
    start = datetime.fromisoformat(manifest["started_utc"])
    end = datetime.fromisoformat(stdlib_completed["completed_utc"])
    core_end = datetime.fromisoformat(completed["completed_utc"])
    stdlib_start = datetime.fromisoformat(stdlib_manifest["started_utc"])
    active_minutes = ((core_end - start) + (end - stdlib_start)).total_seconds() / 60
    summary.update({"working_tree_with_uncommitted_changes": True, "public_modules": 52,
        "additional_stdlib_cases": stdlib_completed["cases"], "comparison_rows": len(comparisons),
        "cross_language_comparison_rows": len(pairs), "all_gap_rows": len(gaps),
        "distinct_cpp_gap_names": len(read(archive / "cpp_gaps_by_name.json")),
        "measure_minutes": (end - start).total_seconds() / 60,
        "active_suite_minutes": active_minutes,
        "between_suite_minutes": (stdlib_start - core_end).total_seconds() / 60,
        "stdlib_completed_utc": stdlib_completed["completed_utc"]})
    save("summary.json", summary)
    save("completed_all.json", {"all_suites_completed": True, "compiler_source_matches_build": True,
         "shared_source_and_toolchain_inputs_match": True, "shared_input_count": len(shared),
         "core_runner_snapshot_matches_manifest": True, "core_completed": "completed.json",
         "stdlib_completed": "stdlib/completed.json", "summary": "summary.json"})
    full = (archive / "FULL_REPORT.md").read_text(encoding="utf-8").splitlines()
    zone = timezone(timedelta(hours=8))
    for index, line in enumerate(full):
        if line.startswith("采样时间（北京时间）"):
            full[index] = f"采样时间（北京时间）：{start.astimezone(zone):%Y-%m-%d %H:%M:%S} 至 {end.astimezone(zone):%Y-%m-%d %H:%M:%S}，共 {summary['measure_minutes']:.2f} 分钟。"
    full += ["", "## 构建与解释边界", "",
        "首次 CMake 配置自动混用了 clang C 与 GCC C++，在编译前失败；记录见 build.log。随后原生构建完成，但发行目录缺少配套 LLVM 工具，封包停止，见 build_retry.log。显式指定配套 GCC 与完整 LLVM 目录后继续正式构建，成功日志见 build_completed.json 的 build_log 字段。失败配置保留在 tx_build/root_cause_configure_failure；成功的 build/ 已清理。",
        "当前工作区未提交；build_manifest.json 保存基准 HEAD、初始 Git 状态和全部实现/构建输入哈希。构建前后与两套采样期间均检查输入稳定。默认 TX 使用原有 -O3、ThinLTO 和 ld.lld。",
        f"两套实际执行时间合计 {active_minutes:.2f} 分钟，中间续跑间隔 {(stdlib_start-core_end).total_seconds()/60:.2f} 分钟。原有套件完成后，标准库在开始采样前触发历史目录防覆盖检查；随后只调整归档入口并续跑 29 项，没有重跑或覆盖已完成样本。见 stdlib_resume.json、run.log 和 stdlib_run.log。",
        "run_core_snapshot.py 保留原有套件执行时的入口代码，其哈希与原 manifest 相符；两套采样的共同实现、配置和工具链哈希一致，最终源码仍匹配正式构建。统一完成核验见 completed_all.json。",
        "格式化和 UTF-8 部分原始负载只消费结果长度，当前编译器可依法消除完整结果物化；名称包含 dynamic 的不可变编码名/局部模板仍可静态特化。不能据此推断任意动态模板、编码名或完整输出内容的成本。",
        "SQLite 池与异步只读查询现在可复用安全连接；C++ 同后端同步获得此行为，而 Java/Python 参考仍关闭连接。跨语言端到端比例保留该生命周期差异。",
        "全量指仓库现有全部性能套件及 52 个公开模块的代表负载，不表示每个 API、失败路径或生产压力全覆盖。内存为 2 ms 进程树工作集采样峰值。",
        "复现：新归档目录先执行 run.py --build-only，再执行 run.py，最后 summarize.py；本目录拒绝覆盖已有正式记录。依赖仓库既有归档源码、本机固定参考和 E:/Project/problems 正式数据。", "",
        "## 本轮全部 TX 耗时变化", "", *comparison_lines[2:], "",
        "## 全部有效跨语言对照", "",
        *table(["套件", "项目", "参考", "TX ms", "参考 ms", "倍率", "可比边界"],
            [[r["suite"], r["name"], r["reference"], f"{r['tx_ms']:.6f}", f"{r['reference_ms']:.6f}",
              f"{r['ratio']:.2f}×" if r["ratio"] is not None else "—", r["note"]] for r in pairs])]
    write(archive / "FULL_REPORT.md", full)
    print(json.dumps(summary, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
