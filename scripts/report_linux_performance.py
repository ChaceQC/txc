"""从 Linux 原始样本生成报告；保留首轮失败及补测来源。"""
from pathlib import Path
import json
import math
import re
import statistics

root = Path(__file__).resolve().parents[1]
archive = root / "benchmarks/linux_full_2026-10-01"


def read(path):
    return json.loads(path.read_text(encoding="utf-8"))


def main():
    primary = read(archive / "results.json")
    extra = read(archive / "extra_results.json")
    suites = {name: value for name, value in primary.items() if value["status"] == "passed"}
    replacements = {"random_long_calibrated": "random_long", "diagnostics_calibrated": "diagnostics",
                    "security_calibrated": "security", "library_paired": "library",
                    "features_paired": "features", "diverse_paired": "diverse",
                    "format_contract_paired": "format_contract", "language_python_paired": "language",
                    "graph_contract_native": "graph_contract"}
    for name, value in extra.items():
        if value["status"] == "passed":
            suites[replacements.get(name, name)] = value
    unresolved = {name: value for name, value in primary.items() if value["status"] != "passed" and name not in suites}
    unresolved.update({name: value for name, value in extra.items() if value["status"] != "passed"})
    manifest = read(archive / "manifest.json")
    end = read(archive / "extra_completed.json")
    stable = read(archive / "extra_primary_end.json")["unchanged"] and end["unchanged"]
    all_cases = {}
    for suite, value in suites.items():
        for name, timings in value.get("medians_ms", {}).items():
            all_cases.setdefault(name, []).append(suite)
    coverage = []
    text = (root / "benchmarks/performance_retest_2026-09-30_static_execution/coverage.md").read_text(encoding="utf-8")
    for line in text.splitlines():
        if not re.match(r"\| [a-z][a-z0-9_]* \|", line):
            continue
        cells = [part.strip() for part in line.split("|")[1:-1]]
        names = re.findall(r"[a-z][a-z_0-9]*(?:/[a-z_0-9]+)?", " ".join(cells[1:]))
        hits = sorted({name.split("/")[-1] for name in names} & all_cases.keys())
        coverage.append({"module": cells[0], "cases": hits, "covered": bool(hits)})
    # 历史样本只用于校验确定性结果，不用于跨平台性能比值。
    historical = {}
    def collect(value):
        if isinstance(value, dict):
            checks = value.get("checksums", {}).get("TX", {}) if isinstance(value.get("checksums"), dict) else {}
            if isinstance(checks, dict):
                for name, checksum in checks.items():
                    if isinstance(checksum, (str, int, float)):
                        historical.setdefault(name, set()).add(str(checksum))
            for child in value.values():
                collect(child)
        elif isinstance(value, list):
            for child in value:
                collect(child)
    previous = root / "benchmarks/performance_retest_2026-09-30_static_execution"
    collect(read(previous / "performance_retest.json"))
    checked = []
    mismatches = []
    for suite in ("compute", "features"):
        for name, actual in suites[suite]["checksums"].items():
            if name in historical:
                if not any(math.isclose(float(actual), float(old), rel_tol=1e-12, abs_tol=1e-6) for old in historical[name]):
                    mismatches.append(suite + "/" + name)
                checked.append(suite + "/" + name)
    comparisons = []
    for suite, value in suites.items():
        timing_cases = value.get("medians_ms", {name: case["medians_ms"] for name, case in value.get("cases", {}).items()})
        for name, timings in timing_cases.items():
            tx = timings.get("TX", timings.get("TX-runtime"))
            if tx is not None and timings.get("C++", 0) > 0:
                comparisons.append({"suite": suite, "case": name, "tx_ms": tx, "cpp_ms": timings["C++"], "ratio": tx / timings["C++"]})
    comparisons.sort(key=lambda item: item["ratio"], reverse=True)
    count = sum(len(value.get("medians_ms", value.get("cases", {}))) for value in suites.values())
    summary = {"suites": len(suites), "cases_including_specialized_repeats": count,
               "covered_modules": sum(row["covered"] for row in coverage), "total_modules": len(coverage),
               "inputs_stable": stable, "unresolved": unresolved, "historical_checks": checked,
               "historical_checksum_mismatches": mismatches, "comparisons": comparisons, "coverage": coverage}
    (archive / "summary.json").write_text(json.dumps(summary, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    (archive / "final_results.json").write_text(json.dumps(suites, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    lines = ["# Linux 全量代表负载性能测试 — 2026-10-01", "",
             f"测试对象为现有 `tx/linux` 原生工具包，工作树提交 `{manifest['head']}`。本次没有重建或修改编译器实现，工具包实际文件 SHA-256 已记录。", "",
             f"完成 **{len(suites)} 组、{count} 个性能条目**（不同专项含重复负载），覆盖现有映射中 **{sum(row['covered'] for row in coverage)}/{len(coverage)} 个公开模块**。每组预热 1 轮、正式串行采样 5 轮，以内部计时中位数报告。", "",
             f"输入指纹稳定：**{stable}**；未解决失败：**{len(unresolved)}**；计算/容器与历史确定性校验值核对：{len(checked)} 项，差异 {len(mismatches)} 项。历史 Windows 耗时不参与性能倍率计算。", "",
             "## 环境与范围", "",
             f"- {manifest['platform']}；Ubuntu 24.04，GCC 13.3.0、Clang 18.1.3；完整 CPU、版本与输入指纹见 `manifest.json`。",
             "- 运行位置为 WSL2 的 `/mnt/e` Windows 挂载盘。文件、日志、进程启动和编译耗时含 WSL/跨文件系统开销，不代表原生 Linux ext4 磁盘成绩。",
             "- TX 使用现有工具包默认优化/ThinLTO 路径；C++ 对照为 GCC `-O3 -DNDEBUG`。C++ 对照覆盖语言、容器、综合、库、格式及图契约；语言特性另含 Python 对照。没有执行 Java/JavaScript 或完整标准库四语言对照。",
             "- 外部题使用原有 10 个性能场景，并核对全部正式输入输出。PostgreSQL 使用本次独立解包的 16.x 临时服务端，TLS、UDP、IPC、HTTP、WebSocket 均为本机服务，结束后关闭。",
             "- 覆盖指每个模块具有代表性性能负载，不是每个 API、所有错误路径、饱和并发、长时间压力或所有网络协议全覆盖。",
             "- 对照语义沿用各基准 README：C++ 的部分对象/深拷贝/循环回收为专门实现；固定格式拼接与通用格式器不同；倍率仅针对相应程序。", "",
             "## 同轮 C++ 对照中最大的差距", "", "| 套件 | 项目 | TX ms | C++ ms | TX/C++ |", "| --- | --- | ---: | ---: | ---: |"]
    for item in comparisons[:15]:
        lines.append(f"| {item['suite']} | {item['case']} | {item['tx_ms']:.3f} | {item['cpp_ms']:.3f} | {item['ratio']:.2f}× |")
    lines += ["", "## 全部性能条目", "", "| 套件 | 项目 | TX / TX runtime ms | C++ ms | Python ms |", "| --- | --- | ---: | ---: | ---: |"]
    for suite, value in suites.items():
        for name, timings in value.get("medians_ms", {}).items():
            cells = [timings.get("TX", timings.get("TX-runtime")), timings.get("C++"), timings.get("Python")]
            lines.append(f"| {suite} | {name} | " + " | ".join("—" if cell is None else f"{cell:.3f}" for cell in cells) + " |")
        if "official_cases" in value:
            lines += [f"| {suite} | 正式数据答案核对 {value['official_cases']} 组通过 | — | — | — |"]
            for name, item in value["cases"].items():
                lines.append(f"| {suite} | {name} | {item['medians_ms']['TX']:.3f} | {item['medians_ms']['C++']:.3f} | — |")
    lines += ["", "## 编译、启动与内存", "", "编译为每个 TX 基准单次完整编译墙钟，包含链接及随包共享库安装，不是五轮编译基准。", "", "| 程序 | 完整编译 ms |", "| --- | ---: |"]
    for name, value in {**primary, **extra}.items():
        if "compile_ms" in value:
            lines.append(f"| {name} | {value['compile_ms']:.1f} |")
    resources = read(archive / "extra_resources.json")
    lines += ["", "GNU time 记录独立进程资源；墙钟精度约 10 ms，极短启动仅供参考。RSS 为内核高水位，不与 Windows 工作集直接比较。", "", "| 程序 | 墙钟中位数 ms | 峰值 RSS 中位数 MiB |", "| --- | ---: | ---: |"]
    for name, samples in resources.items():
        lines.append(f"| {name} | {statistics.median(s['wall_seconds'] for s in samples)*1000:.1f} | {statistics.median(s['peak_rss_kib'] for s in samples)/1024:.2f} |")
    lines += ["", "## 迁移修正与证据", "",
              "首轮原始失败保留在 `results.json`：图契约误用不存在的 TX 源码、random_long 两行格式、证书长度校验和日志跨轮累积。PostgreSQL 因缺少 initdb 中断首轮；补充入口分别修复夹具并重测，结果见 `extra_results.json`。初始失败样本不进入最终表格。", "",
              "图契约使用原 C++ 基准的 TX runtime/reference 两种模式，只把 Windows 内存采集替换为 Linux getrusage；旧 pagefile 字段在 Linux 不适用。日志每轮删除前次测试文件，并核对 2000 行索引、请求上下文和遮蔽字段。证书校验为 DER 字节数 × 500。", "",
              "- `manifest.json`、`extra_manifest.json`：环境、源码、工具包与外部输入指纹。",
              "- `extra_primary_end.json`、`extra_completed.json`：首轮结束及补充阶段的指纹核对。",
              "- `results.json`、`extra_results.json`：逐轮原始输出、计时与校验值。",
              "- `extra_resources.json`、`extra_postgres_version.json`：资源样本与隔离数据库版本。",
              "- `summary.json`：合并结果、差距和覆盖映射。", "",
              "复现入口：WSL Ubuntu 中运行本目录 `run.py`，再用 `uv run --no-project --with websockets --with cryptography python scripts/run_linux_performance_extra.py` 执行补充，最后运行 `scripts/report_linux_performance.py`。入口拒绝覆盖归档；下一轮须使用新的归档目录。", "",
              "## 模块覆盖", "", "| 模块 | 已采样代表负载 |", "| --- | --- |"]
    lines.extend(f"| {row['module']} | {', '.join(row['cases']) or '未覆盖'} |" for row in coverage)
    if unresolved or mismatches or not stable:
        lines += ["", "## 未解决项", "", "```json", json.dumps({"failures": unresolved, "checksum_mismatches": mismatches, "stable": stable}, ensure_ascii=False, indent=2), "```"]
    (archive / "README.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(json.dumps({k: v for k, v in summary.items() if k not in ("comparisons", "coverage", "historical_checks")}, ensure_ascii=False))


if __name__ == "__main__":
    main()
