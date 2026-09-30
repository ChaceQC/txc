"""生成提交后全量复测的绝对耗时、跨轮变化和同轮差距报告。"""

from datetime import datetime, timezone, timedelta
import json
from pathlib import Path
import statistics

root = Path(__file__).resolve().parents[2]
out = root / "benchmarks/performance_retest_2026-09-30_after_completion"
previous = root / "benchmarks/performance_retest_2026-09-30"
completion = root / "benchmarks/performance_completion_2026-09-30"


def read(directory, name):
    return json.loads((directory / name).read_text(encoding="utf-8"))


def save(name, value):
    (out / name).write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def write(name, lines):
    (out / name).write_text("\n".join(lines) + "\n", encoding="utf-8")


def fmt(value):
    if value is None:
        return "—"
    return f"{value:.3f}" if isinstance(value, (int, float)) else str(value)


def delta(old, new):
    return (new / old - 1) * 100 if old else None


def percent(value):
    return f"{value:+.1f}%" if value is not None else "—"


def table(headers, rows):
    return ["| " + " | ".join(headers) + " |",
            "| " + " | ".join(["---"] * len(headers)) + " |",
            *["| " + " | ".join(map(str, row)) + " |" for row in rows], ""]


def main():
    manifest = read(out, "manifest.json")
    completed = read(out, "completed.json")
    language = read(out, "language_features.json")
    old_language = read(previous, "language_features.json")
    audit = read(out, "performance_retest.json")
    old_audit = read(previous, "performance_retest.json")
    diverse = read(out, "diverse.json")
    old_diverse = read(previous, "diverse.json")
    special = read(out, "special.json")
    old_special = read(previous, "special.json")
    equivalent = read(out, "equivalence.json")["results"]
    old_equivalent = read(previous, "equivalence.json")["results"]
    comparisons = []
    gaps = []

    def compare(group, name, current, old, notes=""):
        value = delta(old, current)
        comparisons.append({"suite": group, "name": name, "previous_ms": old,
                            "current_ms": current, "change_percent": value, "note": notes})
        return [fmt(old), percent(value)]

    def gap(group, name, current, reference, label, notes=""):
        if reference > 0 and current / reference >= 3:
            gaps.append({"suite": group, "name": name, "tx_ms": current,
                         "reference_ms": reference, "reference": label,
                         "ratio": current / reference, "note": notes})

    started = datetime.fromisoformat(manifest["started_utc"])
    ended = datetime.fromisoformat(completed["completed_utc"])
    local_zone = timezone(timedelta(hours=8))
    lines = ["# 2026-09-30 提交后全量性能复测", "",
             f"测试版本：`{manifest['head']}`。先提交优化及已有记录，再完成正式重建、封包和串行采样。",
             "本轮所有套件正常退出；逐轮校验值及两道外部题的正式数据答案核对通过。", "",
             f"- 采样时间（北京时间）：{started.astimezone(local_zone):%Y-%m-%d %H:%M:%S} 至 {ended.astimezone(local_zone):%H:%M:%S}，共 {(ended-started).total_seconds()/60:.2f} 分钟。",
             "- 构建：执行 `scripts/build.ps1` 成功，当前编译器、标准库及配套组件已封包至 `tx/`，临时 `build/` 已清理。",
             "- 测量期间源码、工具链、固定参考程序及外部输入的 SHA-256 未改变，见 [manifest.json](manifest.json) 和 [completed.json](completed.json)。",
             "- 本报告与[紧邻上一轮](../performance_retest_2026-09-30/README.md)比较 TX 耗时；负百分比表示变快。两轮间源码和机器状态均可能变化，跨轮差值不单独证明优化因果。",
             "- 耗时单位 ms。主要数据为程序内部计时中位数；启动、编译、进程运行单独使用墙钟时间。",
             "- Windows 11 26200，8 核 / 16 线程；GCC 13.1.0、随包 clang 23.1.2、Python 3.12.10、Java 23.0.2、Node 24.13.0。具体环境见 manifest。", "",
             "## 覆盖与采样", ""]
    lines += table(["套件", "范围", "预热 / 正式轮数"], [
        ["语言特性", "22 项，TX/C++/Python/Java", "1 / 7，交替顺序"],
        ["标准库组合", "10 项 TX；C++ 字典另有两种参考", "1 / 7，交替顺序"],
        ["模块与新增特性", "21 项计算、10 项特性、6 项系统；另测 Python/Java 历史组合", "1 / 3，交替顺序"],
        ["本机网络", "HTTP、Requests、WebSocket 共 3 项 TX", "1 / 3，交替顺序"],
        ["外部题", "2 题各 5 组，5 种语言；普通 TX 程序核对全部 89 组正式数据", "2 / 17，轮换顺序"],
        ["综合负载", "18 项，TX/C++/Python/Java", "1 / 5，轮换顺序"],
        ["启动 / 编译", "4 语言空程序启动 / 5 类编译操作，含进程树内存采样", "1 / 7；1 / 3"],
        ["优化专项", "借用、静态运行时、解析、格式化、serde、容器/调用、迭代/算术/拷贝、编码/统计、文件流、长随机数", "1 / 7；静态运行时 1 / 3"],
        ["校准对照", "18 项综合、堆、补偿统计，GCC 和 clang", "1 / 5，交替顺序"],
        ["新增契约", "真正动态格式化两项、128 节点共享/循环图复制和 GC 两项", "1 / 5，交替顺序"],
    ])
    lines += ["“全量”为现有性能套件及后来增加的两组契约。沿用的模块审计覆盖 37 个公开模块，未为当前全部公开模块新建独立基准；并发、DNS、TLS 等原来没有代表负载的模块不据此宣称性能达标。",
              "语言、标准库组合、综合、校准和新增契约的参考重新构建；模块审计与外部题的 C++/Java 固定参考沿用，其源码和二进制哈希已记录。", "",
              "## 语言特性", "", "四语言逐项核对固定校验值，7 轮原始数据见 [language_features.json](language_features.json)。", ""]
    rows = []
    for name in language["checksums"]:
        med = {label: statistics.median(language[label][name]) for label in ("TX", "CPP", "Python", "Java")}
        old = statistics.median(old_language["TX"][name])
        rows.append([name, *[fmt(med[label]) for label in med], *compare("语言特性", name, med["TX"], old)])
        for label in ("CPP", "Python", "Java"):
            notes = ""
            if name == "variadic_unpack" and label in ("CPP", "Java"):
                notes = "参考不执行 TX 动态命名实参绑定"
            if name == "deep_copy" and label in ("CPP", "Java"):
                notes = "参考只复制已知形状；通用图复制另见新增契约"
            if name == "cycle_gc" and label in ("CPP", "Java"):
                notes = "参考仅处理单节点自环；通用图 GC 另见新增契约"
            if name == "deinit" and label == "Java":
                notes = "Java 显式 close 回调，不是 JVM 确定性析构"
            gap("语言特性", name, med["TX"], med[label], "C++" if label == "CPP" else label, notes)
    lines += table(["项目", "TX", "C++", "Python", "Java", "上一轮 TX", "变化"], rows)
    lines += ["结构体、模块调用和有界递归的静态表示允许 LLVM 继续内联、折叠或循环化，结果不能换算为一般程序的每次调用成本。语言基准中的通用语义差异见 [负载说明](../language_features/README.md)。", ""]
    for section, title in (("library", "标准库组合"), ("audit", "模块与新增特性"), ("network", "本机网络")):
        lines += [f"## {title}", ""]
        groups = {"组合": audit[section]["results"]} if section == "library" else audit[section]["results"]
        old_groups = {"组合": old_audit[section]["results"]} if section == "library" else old_audit[section]["results"]
        for group, data in groups.items():
            lines += [f"### {group}", ""]
            med = data["medians_ms"]
            labels = list(med)
            names = list(dict.fromkeys(name for values in med.values() for name in values))
            rows = []
            for name in names:
                tx = med.get("TX", {}).get(name)
                old = old_groups[group]["medians_ms"].get("TX", {}).get(name)
                change_columns = compare(section + "/" + group, name, tx, old) if tx is not None and old is not None else ["—", "—"]
                rows.append([name, *[fmt(med[label].get(name, "—")) for label in labels], *change_columns])
                if tx is None:
                    continue
                for label in labels:
                    if label == "TX" or name not in med[label] or name == "test_assert":
                        continue
                    if name == "statistics_mean" and label in ("C++", "Java"):
                        continue
                    notes = "整数毫秒短项，倍率精度有限" if section == "library" else ""
                    if name == "random_int" and label != "C++":
                        notes = "随机序列不同，不作等价结论"
                    gap(section + "/" + group, name, tx, med[label][name], label, notes)
            lines += table(["项目", *labels, "上一轮 TX", "变化"], rows)
            if data["cross_language_checksum_differences"]:
                lines += ["跨语言校验值差异：`" + json.dumps(data["cross_language_checksum_differences"], ensure_ascii=False) + "`。", ""]
    http = audit["network"]["results"]["http"]["medians_ms"]
    for name in ("httpx_get", "requests_get"):
        gap("网络观察", name + " / http_get", http["TX"][name], http["Java"]["http_get"], "Java",
            "客户端实现与生命周期不同，仅作 HTTP 端到端观察")
    lines += ["标准库组合为整数毫秒计时。`test_assert` 的成功断言参考可被优化消除，不列入有效差距排名；统计均值使用后面的补偿统计校准结果。HTTP 客户端命名与实现不同，差距清单将对应请求次数的比例注明为端到端观察。", "",
              "## 综合负载", "", "完整 64 位容器、解析结果及 payload schema 对照，18 项每轮核对固定校验值。历史 `format_dynamic` 为局部不可变模板，本次会进入静态路径；真正动态模板另见新增契约。", ""]
    rows = []
    for name, cases in diverse["diverse"]["cases"].items():
        current = cases["TX"]["median_ms"]
        old = old_diverse["diverse"]["cases"][name]["TX"]["median_ms"]
        rows.append([name, *[fmt(cases[label]["median_ms"]) for label in ("TX", "C++", "Python", "Java")], *compare("综合", name, current, old)])
        for label in ("C++", "Python", "Java"):
            notes = "局部不可变模板，不能代表真正动态格式化" if name == "format_dynamic" else ""
            if label == "C++" and name == "format_literal":
                notes = "C++ 直接拼接固定内容，不是通用格式器"
            if label == "C++" and name == "format_dynamic":
                notes += "；C++ 仅实现本负载的两占位符格式"
            gap("综合", name, current, cases[label]["median_ms"], label, notes)
    lines += table(["项目", "TX", "C++", "Python", "Java", "上一轮 TX", "变化"], rows)
    lines += ["### 同轮 GCC / clang 校准", "", "GCC 与 clang 各有独立配对采样。下表 TX 耗时来自 GCC 配对轮；倍率各自使用对应配对轮 TX 中位数。", ""]
    rows = []
    for group in ("diverse", "heap", "statistics"):
        for name, data in equivalent[group]["cases"].items():
            clang = equivalent[group + "_clang"]["cases"][name]
            rows.append([name, fmt(data["TX_median_ms"]), fmt(data["CPP_median_ms"]), fmt(clang["CPP_median_ms"]),
                         f"{data['TX_median_ms']/data['CPP_median_ms']:.2f}×", f"{clang['TX_median_ms']/clang['CPP_median_ms']:.2f}×"])
            for suffix, label in (("", "GCC C++"), ("_clang", "clang C++")):
                current = equivalent[group + suffix]["cases"][name]
                old = old_equivalent[group + suffix]["cases"][name]["TX_median_ms"]
                compare("校准/" + group + suffix, name, current["TX_median_ms"], old)
                notes = ""
                if name == "format_literal":
                    notes = "C++ 直接拼接固定内容，不是通用格式器"
                if name == "format_dynamic":
                    notes = "TX 局部不可变模板；C++ 仅实现本负载的两占位符格式"
                gap("校准/" + group + suffix, name, current["TX_median_ms"], current["CPP_median_ms"], label, notes)
    lines += table(["项目", "TX（GCC 轮）", "GCC", "clang", "TX/GCC", "TX/clang"], rows)
    lines += ["## 外部题", "", "本轮重建 TX 普通程序和计时程序。mini-filesystem 正式数据 44/44、not-yet-on-stage 45/45；五语言每次计时执行均核对答案。", ""]
    for task, cases in audit["external"]["results"]["tasks"].items():
        lines += [f"### {task}", ""]
        rows = []
        for name, values in cases.items():
            med = values["median_ms"]
            old = old_audit["external"]["results"]["tasks"][task][name]["median_ms"]["TX"]
            rows.append([name, *[fmt(med[label]) for label in ("TX", "C++", "Python", "Java", "JavaScript")], *compare("外部题/" + task, name, med["TX"], old)])
            for label, value in med.items():
                if label != "TX":
                    gap("外部题/" + task, name, med["TX"], value, label)
        lines += table(["输入", "TX", "C++", "Python", "Java", "JavaScript", "上一轮 TX", "变化"], rows)
    lines += ["## 优化专项", "", "各项目逐轮核对校验值稳定，单位 ms。", ""]
    rows = []
    for group, data in special.items():
        if group == "random_long":
            continue
        for name, current in data["medians_ms"].items():
            samples = [sample[name][0] for sample in data["samples"]]
            old = old_special[group]["medians_ms"][name]
            rows.append([group, name, fmt(current), fmt(min(samples)), fmt(max(samples)), len(samples), *compare("专项/" + group, name, current, old)])
    lines += table(["专项", "项目", "中位数", "最小", "最大", "轮数", "上一轮 TX", "变化"], rows)
    random = {label: statistics.median(samples) for label, samples in special["random_long"]["samples_ms"].items()}
    lines += ["长随机数：" + "；".join(f"{label} {value:.3f} ms" for label, value in random.items()) + "；固定校验和 2500067985。", ""]
    compare("专项", "random_long", random["TX"], statistics.median(old_special["random_long"]["samples_ms"]["TX"]))
    gap("长随机数", "random_long", random["TX"], random["C++"], "C++")
    lines += ["## 新增契约对照", "", "动态模板交替两种宽度并包含函数传参，C++ 使用 `std::vformat`。图对照为 128 个节点、共享及循环边，复制使用身份映射，GC 扫描外部引用；100 轮均验证原图和副本的 256 个观察标记全部释放。图 TX 一栏直接链接当前 TX 运行时，包含通用运行时机制成本，不代表 .tx 前端生成代码或类析构/复活的全部语义。", ""]
    rows = []
    for group, tx_label in (("format_contract", "tx"), ("graph_contract", "tx_runtime")):
        data = read(out, group + ".json")
        old = read(completion, group + ".json")
        for name, current in data["medians_ms"][tx_label].items():
            reference = data["medians_ms"]["cpp"][name]
            rows.append([name, fmt(current), fmt(reference), f"{current/reference:.2f}×", *compare("契约/" + group, name, current, old["medians_ms"][tx_label][name]), data["checksums"][name]])
            gap("契约/" + group, name, current, reference, "C++")
    lines += table(["项目", "TX / TX 运行时", "C++", "TX/C++", "上一轮专项 TX", "变化", "校验值"], rows)
    lines += ["## 进程、启动、编译和内存", "", "内存每 2 ms 轮询进程树工作集，是近似采样峰值。墙钟时间包括启动和退出，不能与子项内部计时直接相加比较。", ""]
    rows = []
    for group, name, data in (("进程运行", "diverse", diverse["diverse"]["processes"]), ("空程序启动", "startup", diverse["startup"]), ("编译", "compilation", diverse["compilation"])):
        old_data = old_diverse["diverse"]["processes"] if name == "diverse" else old_diverse[name]
        for label, values in data.items():
            wall = values["wall_ms"]["median"]
            old = old_data[label]["wall_ms"]["median"]
            rows.append([group, label, fmt(wall), fmt(values["sampled_tree_peak_mib"]["median"]), fmt(old), percent(delta(old, wall))])
            if label == "TX" or name == "compilation" and label.startswith("TX"):
                compare(group, label, wall, old)
    lines += table(["类型", "语言 / 操作", "墙钟 ms", "峰值 MiB", "上一轮墙钟 ms", "变化"], rows)
    lines += ["## 差距及复现", "", "完整的 ≥3× 原始倍率见 [gaps.md](gaps.md) 和 [gaps.json](gaps.json)，含参考语义差异说明；不能把不同套件的同名项目累计为独立热点。所有上一轮 TX 变化见 [comparison.md](comparison.md) 和 [comparison.json](comparison.json)。", "",
              "- [主采样脚本](run.py)、[汇总脚本](summarize.py)、[输入及工具链指纹](manifest.json)、[完成记录](completed.json)。",
              "- [语言样本](language_features.json)、[语言日志](language_features.log)、[标准库/模块/网络/外部题样本](performance_retest.json)。",
              "- [综合/启动/编译/内存样本](diverse.json)、[校准样本](equivalence.json)、[专项样本](special.json)、[动态格式样本](format_contract.json)、[图对照样本](graph_contract.json)。",
              "- 复现命令：`python -X utf8 -B benchmarks/performance_retest_2026-09-30_after_completion/run.py`。脚本拒绝覆盖已有 manifest，重跑请使用新归档目录。",
              "- 沿用本机被 Git 忽略的模块审计、参考程序和 `E:/Project/problems` 正式数据；归档不是脱离这些本机数据即可独立执行的套件。",
              "- 本轮只新增复测及汇总记录，未修改编译器性能实现，也未运行无关功能测试。", ""]
    tx_wall = diverse["diverse"]["processes"]["TX"]["wall_ms"]["median"]
    cpp_wall = diverse["diverse"]["processes"]["C++"]["wall_ms"]["median"]
    highlights = ["## 结果与 ≥3× 差距", "",
                  f"综合进程墙钟 TX {tx_wall:.3f} ms、C++ {cpp_wall:.3f} ms，即 {tx_wall/cpp_wall:.2f}×；各个子项仍可能超过 3×。",
                  "完整清单包括 GCC / clang、Python / Java 以及带语义差异说明的原始对照条目，见 [gaps.md](gaps.md)。下面是需要优先关注的负载。", ""]
    selection = [("语言特性", "deinit"), ("综合", "serde_short_text"),
                 ("综合", "format_literal"), ("契约/format_contract", "format_parameter"),
                 ("契约/format_contract", "format_alternating"), ("契约/graph_contract", "graph_copy"),
                 ("综合", "serde_long_text"), ("audit/compute", "regex_search")]
    selected = [next(row for row in gaps if row["suite"] == group and row["name"] == name and row["reference"] == "C++") for group, name in selection]
    highlights += table(["项目", "TX ms", "C++ ms", "倍率", "说明"], [[row["name"], fmt(row["tx_ms"]), fmt(row["reference_ms"]), f"{row['ratio']:.2f}×", row["note"]] for row in selected])
    highlights += ["单节点自环 GC 17.60×、已知形状深拷贝 9.36×及可变实参展开 5.25×（C++）都有参考语义边界，不能换算成通用机制的倍率。通用图 GC 本轮为 1.46×，图复制为 4.96×。",
                   "接近阈值的编码项目随采样和参考编译器变化：字面量编码综合轮为 3.11×，GCC / clang 校准为 2.95× / 2.86×；不可变编码名综合轮为 3.25×，GCC / clang 校准为 3.32× / 2.98×。", "",
                   "### 本轮观察到的变慢项", "",
                   "下列为相对上一轮中位数增加至少 10% 的负载（校准采样单独记载）。绝对耗时很短或仅 3 轮采样的项目更易受调度影响；尚未做根因分析。", ""]
    slower = [row for row in comparisons if row["change_percent"] is not None and row["change_percent"] >= 10]
    highlights += table(["套件", "项目", "上一轮 ms", "本轮 ms", "变化"], [[row["suite"], row["name"], fmt(row["previous_ms"]), fmt(row["current_ms"]), percent(row["change_percent"])] for row in slower])
    highlights += ["`map_hit_128` 的校准轮变慢，但综合四语言轮为 2.362→1.892 ms；不能把单个采样增幅当作已经确认的实现回退。", ""]
    insertion = lines.index("## 覆盖与采样")
    lines[insertion:insertion] = highlights
    write("README.md", lines)
    save("comparison.json", comparisons)
    comparison_lines = ["# 本轮与上一轮的 TX 耗时变化", "", "单位 ms；负值表示变快。比较基准为紧邻上一轮全量记录；新增契约使用上一轮专项记录。短项和低轮数负载容易受调度扰动，本表不单独判断回退的根因。", ""]
    comparison_lines += table(["套件", "项目", "上一轮", "本轮", "变化"], [[row["suite"], row["name"], fmt(row["previous_ms"]), fmt(row["current_ms"]), percent(row["change_percent"])] for row in comparisons])
    write("comparison.md", comparison_lines)
    gaps.sort(key=lambda row: row["ratio"], reverse=True)
    save("gaps.json", gaps)
    gap_lines = ["# 本轮 TX 耗时达到参考语言三倍的项目", "", "倍率为同轮 TX 中位耗时除以同轮参考中位耗时，按未四舍五入值筛选 ≥3。这里列出每个套件的原始倍率，参考工作量不同的项目保留说明，不能视为通用语言性能结论。", "", "成功断言可被优化消除、旧简单求和参考及零耗时均不进入此表。HTTP 对 Java 的两个比例注明为端到端观察。没有参考实现的专项不计算跨语言倍率。", ""]
    gap_lines += table(["套件", "项目", "参考", "TX ms", "参考 ms", "倍率", "说明"], [[row["suite"], row["name"], row["reference"], f"{row['tx_ms']:.6f}", f"{row['reference_ms']:.6f}", f"{row['ratio']:.2f}×", row["note"]] for row in gaps])
    write("gaps.md", gap_lines)
    tx_process = diverse["diverse"]["processes"]["TX"]
    cpp_process = diverse["diverse"]["processes"]["C++"]
    save("summary.json", {"head": manifest["head"], "all_suites_completed": True,
                          "measure_minutes": (ended-started).total_seconds()/60,
                          "tx_process_wall_ms": tx_process["wall_ms"]["median"],
                          "cpp_process_wall_ms": cpp_process["wall_ms"]["median"],
                          "tx_cpp_process_ratio": tx_process["wall_ms"]["median"] / cpp_process["wall_ms"]["median"],
                          "tx_process_peak_mib": tx_process["sampled_tree_peak_mib"]["median"],
                          "comparison_rows": len(comparisons), "gap_rows": len(gaps)})
    print(f"REPORT COMPLETE: {out / 'README.md'}")


if __name__ == "__main__":
    main()
