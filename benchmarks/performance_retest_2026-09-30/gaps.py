"""从本轮原始中位数提取跨语言差距，不把不同专项负载混作对照。"""
import json
import statistics
from pathlib import Path

archive = Path(__file__).resolve().parent


def read(name):
    return json.loads((archive / name).read_text(encoding="utf-8"))


def main():
    rows = []

    def add(group, name, tx, ref, language, note=""):
        if ref > 0 and tx / ref >= 3:
            rows.append(dict(group=group, name=name, tx_ms=tx, reference_ms=ref,
                             reference=language, ratio=tx / ref, note=note))

    lang = read("language_features.json")
    for name in lang["checksums"]:
        for label in ("CPP", "Python", "Java"):
            add("语言特性", name, statistics.median(lang["TX"][name]),
                statistics.median(lang[label][name]), label)
    audit = read("performance_retest.json")
    for section in ("library", "audit", "network"):
        groups = {"组合": audit[section]["results"]} if section == "library" else audit[section]["results"]
        for group, data in groups.items():
            med = data["medians_ms"]
            for name, tx in med.get("TX", {}).items():
                for label, cases in med.items():
                    if label == "TX" or name not in cases:
                        continue
                    note = ""
                    if section == "library":
                        note = "整数毫秒短项，倍率精度有限"
                    if name == "statistics_mean":
                        note = "旧参考为普通求和；优先看校准补偿统计"
                    if name == "test_assert":
                        note = "参考循环可被优化消除，不作为有效热点排名"
                    if name == "random_int" and label != "C++":
                        note = "随机序列不同，不作为等价性能结论"
                    add(section + "/" + group, name, tx, cases[name], label, note)
    for name, cases in read("diverse.json")["diverse"]["cases"].items():
        for label, data in cases.items():
            if label != "TX":
                add("综合", name, cases["TX"]["median_ms"], data["median_ms"], label)
    for group, data in read("equivalence.json")["results"].items():
        if "cases" not in data:
            continue
        for name, case in data["cases"].items():
            add("校准/" + group, name, case["TX_median_ms"], case["CPP_median_ms"],
                "clang C++" if group.endswith("_clang") else "GCC C++")
    for task, cases in audit["external"]["results"]["tasks"].items():
        for name, data in cases.items():
            med = data["median_ms"]
            for label, value in med.items():
                if label != "TX":
                    add("外部题/" + task, name, med["TX"], value, label)
    med = {k: statistics.median(v) for k, v in read("special.json")["random_long"]["samples_ms"].items()}
    add("长随机数", "random_long", med["TX"], med["C++"], "C++")
    rows.sort(key=lambda row: row["ratio"], reverse=True)
    (archive / "gaps.json").write_text(json.dumps(rows, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    lines = ["# 本轮 TX 耗时达到参考语言 3 倍的项目", "",
             "倍率 = TX 中位耗时 / 同轮参考中位耗时，使用未四舍五入值筛选 ≥3。单位 ms。不同套件的同名项目保留各自结果，不累计为独立热点。", "",
             "无参考实现的 TX 专项无法判定跨语言倍率。零耗时不计算倍率；test_assert 的近零参考可以被优化消除，不能据此排名。标准库 dict 与 dict_hash/dict_dynamic 名称及实现不同，不强行配对。HTTP 客户端名称和实现不同，不强行配对。", "",
             "| 套件 | 项目 | 参考 | TX ms | 参考 ms | 倍率 | 说明 |",
             "| --- | --- | --- | ---: | ---: | ---: | --- |"]
    lines += [f"| {r['group']} | {r['name']} | {r['reference']} | {r['tx_ms']:.6f} | {r['reference_ms']:.6f} | {r['ratio']:.2f}× | {r['note']} |" for r in rows]
    (archive / "gaps.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(json.dumps(rows, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
