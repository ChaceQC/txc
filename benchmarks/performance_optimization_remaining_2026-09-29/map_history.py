"""只复核 02 之前留存的原程序与最终候选，不重跑其他历史套件。"""
import json
import statistics

from measure import root, work, digest, read_program, save


def main():
    previous = root / "tx_build/performance_15_16/map_history/before_02/call_borrowing_20260929.exe"
    if not previous.is_file():
        raise SystemExit("历史二进制未随 Git 分发；本机不存在时只阅读归档样本。")
    assert digest(previous) == "4b1fbef79a4ebf4d6b92a7bb615dad0f4279823b96193c8bd03872c8719db577"
    programs = {"before_02": previous, "current": work / "candidate/borrowing.exe"}
    expected = None
    for program in programs.values():
        observed = {name: value["checksum"] for name, value in read_program(program)["cases"].items()}
        assert expected is None or observed == expected
        expected = observed
    samples = {name: [] for name in programs}
    orders = []
    for index in range(5):
        order = list(programs) if index % 2 == 0 else list(reversed(programs))
        orders.append(order)
        for label in order:
            measured = read_program(programs[label])
            assert {name: value["checksum"] for name, value in measured["cases"].items()} == expected
            samples[label].append(measured)
    medians = {label: {name: statistics.median(sample["cases"][name]["ms"] for sample in values)
                       for name in expected} for label, values in samples.items()}
    files = [file for program in programs.values() for file in [program, *program.parent.glob("*.dll")]]
    result = {"warmups": 1, "rounds": 5, "orders": orders, "samples": samples,
              "checksums": expected, "medians_ms": medians,
              "sha256": {str(path.relative_to(root)): digest(path) for path in files},
              "limit": "旧程序使用 01 留存的 DLL；初始历史运行的加载路径仍不可追溯。"}
    save("map_history.json", result)
    print(json.dumps(medians, indent=2))


if __name__ == "__main__":
    main()
