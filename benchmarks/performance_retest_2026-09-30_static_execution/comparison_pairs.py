"""从本轮不同格式的原始结果提取全部有效跨语言对照。"""

import statistics


def collect(archive, stdlib_archive, read, details):
    rows = []
    gap_notes = {(row["suite"], row["name"], row["reference"]): row["note"]
                 for row in read(archive / "all_gaps.json")}

    def add(suite, name, tx, reference, label, note=""):
        rows.append({"suite": suite, "name": name, "reference": label,
            "tx_ms": tx, "reference_ms": reference,
            "ratio": tx / reference if reference > 0 else None,
            "note": gap_notes.get((suite, name, label), note) or note})

    language = read(archive / "language_features.json")
    for name in language["checksums"]:
        tx = statistics.median(language["TX"][name])
        for label in ("CPP", "Python", "Java"):
            note = ""
            if label in ("CPP", "Java"):
                note = {"deep_copy": "参考只复制已知形状；通用图复制见契约组",
                        "cycle_gc": "参考只识别单节点自环；通用图 GC 见契约组",
                        "variadic_unpack": "参考不执行 TX 动态命名实参绑定"}.get(name, "")
            add("语言特性", name, tx, statistics.median(language[label][name]),
                "C++" if label == "CPP" else label, note)
    audit = read(archive / "performance_retest.json")
    for section in ("library", "audit", "network"):
        groups = {"组合": audit[section]["results"]} if section == "library" else audit[section]["results"]
        for group, data in groups.items():
            medians = data["medians_ms"]
            for name, tx in medians.get("TX", {}).items():
                for label, cases in medians.items():
                    if label == "TX" or name not in cases or name == "test_assert":
                        continue
                    if name == "statistics_mean" and label in ("C++", "Java"):
                        continue
                    note = "整数毫秒短项，倍率精度有限" if section == "library" else ""
                    if name == "random_int" and label != "C++":
                        note = "随机序列不同，仅作原始负载观察"
                    add(section + "/" + group, name, tx, cases[name], label, note)
    http = audit["network"]["results"]["http"]["medians_ms"]
    for name in ("httpx_get", "requests_get"):
        add("网络观察", name + " / http_get", http["TX"][name], http["Java"]["http_get"],
            "Java", "客户端实现与生命周期不同，仅作 HTTP 端到端观察")
    diverse = read(archive / "diverse.json")["diverse"]["cases"]
    for name, cases in diverse.items():
        for label in ("C++", "Python", "Java"):
            note = "局部不可变模板，不能代表真正动态格式化" if name == "format_dynamic" else ""
            add("综合", name, cases["TX"]["median_ms"], cases[label]["median_ms"], label, note)
    equivalent = read(archive / "equivalence.json")["results"]
    for group in ("diverse", "heap", "statistics"):
        for suffix, label in (("", "GCC C++"), ("_clang", "clang C++")):
            for name, cases in equivalent[group + suffix]["cases"].items():
                add("校准/" + group + suffix, name, cases["TX_median_ms"], cases["CPP_median_ms"], label)
    for task, cases in audit["external"]["results"]["tasks"].items():
        for name, data in cases.items():
            medians = data["median_ms"]
            for label, value in medians.items():
                if label != "TX":
                    add("外部题/" + task, name, medians["TX"], value, label)
    random = read(archive / "special.json")["random_long"]["samples_ms"]
    add("长随机数", "random_long", statistics.median(random["TX"]),
        statistics.median(random["C++"]), "C++")
    for group, tx_label in (("format_contract", "tx"), ("graph_contract", "tx_runtime")):
        data = read(archive / (group + ".json"))["medians_ms"]
        for name, tx in data[tx_label].items():
            add("契约/" + group, name, tx, data["cpp"][name], "C++",
                "直接链接 TX 运行时；不代表前端生成代码的全部语义" if tx_label == "tx_runtime" else "")
    for group, data in read(stdlib_archive / "results.json").items():
        for name, cases in data["cases"].items():
            for label in ("C++", "Java", "Python"):
                add("新增/" + group, name, cases["TX"]["median_ms"], cases[label]["median_ms"], label,
                    details[name][2].replace("采用校准样本：", ""))
    return rows
