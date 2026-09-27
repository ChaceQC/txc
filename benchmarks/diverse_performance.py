"""与 diverse_performance.tx 使用相同输入、次数和校验值的 Python 负载。"""

import json
import time


def report(name, operation):
    started = time.perf_counter_ns()
    checksum = operation()
    elapsed_us = (time.perf_counter_ns() - started) // 1000
    print(name)
    print(elapsed_us)
    print(checksum)


def bench_vector_scale():
    small = [3] * 1000
    large = [3] * 100000

    def scan(values, repetitions):
        checksum = 0
        for _ in range(repetitions):
            for value in values:
                checksum += value
        return checksum

    report("vector_scan_1k", lambda: scan(small, 1000))
    report("vector_scan_100k", lambda: scan(large, 100))


def bench_vector_access():
    values = [i % 97 for i in range(8192)]

    def access(stride):
        checksum = 0
        for i in range(1000000):
            checksum += values[(i * stride) % 8192]
        return checksum

    report("vector_index_sequential", lambda: access(1))
    report("vector_index_strided", lambda: access(127))


def bench_map_distribution():
    small = {i: i for i in range(128)}
    large = {i: i for i in range(8192)}

    def hits(values, modulus, stride):
        checksum = 0
        for i in range(500000):
            checksum += values[(i * stride) % modulus]
        return checksum

    def mixed():
        checksum = 0
        for i in range(500000):
            key = 100000 + i % 8192
            if i % 10 == 0:
                key = i % 8192
            if key in large:
                checksum += 1
        return checksum

    report("map_hit_128", lambda: hits(small, 128, 1))
    report("map_hit_8192", lambda: hits(large, 8192, 127))
    report("map_hit_10_percent", mixed)


def bench_dictionary_keys():
    # Python 的 bool 与 int 共用键空间，类型标签保留 TX 的键类型区分。
    values = {("int", 7): 1, ("text", "seven"): 2, ("bool", True): 3}

    def lookup(kind):
        checksum = 0
        for i in range(200000):
            key = ("int", 7) if kind == "int" else ("text", "seven")
            if kind == "miss":
                key = ("int", i)
            if key in values:
                checksum += 1
        return checksum

    report("dictionary_int_hit", lambda: lookup("int"))
    report("dictionary_text_hit", lambda: lookup("text"))
    report("dictionary_mostly_miss", lambda: lookup("miss"))


def bench_format_paths():
    template = "{}:{}"

    def literal():
        checksum = 0
        for i in range(50000):
            checksum += len(f"item:{i}")
        return checksum

    def dynamic():
        checksum = 0
        for i in range(50000):
            checksum += len(template.format("item", i))
        return checksum

    report("format_literal", literal)
    report("format_dynamic", dynamic)


def bench_encoding_paths():
    value = "Hello, 世界 🌍"
    codec = "utf-8"

    def encode(name):
        checksum = 0
        for _ in range(50000):
            checksum += len(value.encode(name))
        return checksum

    report("encoding_literal", lambda: encode("utf-8"))
    report("encoding_dynamic", lambda: encode(codec))


def bench_serde_size():
    short_value = {"id": 7, "name": "x"}
    long_value = {"id": 7, "name": "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"}

    def roundtrip(value):
        checksum = 0
        for _ in range(5000):
            encoded = json.dumps(value, ensure_ascii=False, separators=(",", ":"))
            decoded = json.loads(encoded)
            checksum += decoded["id"] + len(decoded["name"])
        return checksum

    report("serde_short_text", lambda: roundtrip(short_value))
    report("serde_long_text", lambda: roundtrip(long_value))


def bench_parse_paths():
    valid_inputs = [str(i) for i in range(1000)]
    invalid_inputs = [text + "x" for text in valid_inputs]

    def valid():
        checksum = 0
        for i in range(100000):
            try:
                int(valid_inputs[i % 1000], 10)
                checksum += 1
            except ValueError:
                pass
        return checksum

    def invalid():
        checksum = 0
        for i in range(100000):
            try:
                int(invalid_inputs[i % 1000], 10)
            except ValueError:
                checksum += 1
        return checksum

    report("parse_valid", valid)
    report("parse_invalid", invalid)


def main():
    bench_vector_scale()
    bench_vector_access()
    bench_map_distribution()
    bench_dictionary_keys()
    bench_format_paths()
    bench_encoding_paths()
    bench_serde_size()
    bench_parse_paths()


if __name__ == "__main__":
    main()
