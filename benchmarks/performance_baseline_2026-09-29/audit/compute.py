import csv
import decimal
import hashlib
import io
import json
import math
import os
import random
import re
import statistics
import threading
import time
import unicodedata
import xml.etree.ElementTree as element_tree


def measure(name, operation):
    start = time.perf_counter_ns()
    checksum = operation()
    elapsed = (time.perf_counter_ns() - start) // 1000
    print(name)
    print(elapsed)
    print(checksum)


def algorithm_sort():
    values = list(range(255, -1, -1))
    checksum = 0
    for _ in range(5000):
        ordered = sorted(values)
        checksum += ordered[0] + ordered[255]
    return checksum


def bytes_hex():
    data = "Hello, 世界!".encode("utf-8")
    checksum = 0
    for _ in range(20000):
        checksum += len(bytes.fromhex(data.hex()))
    return checksum


def cancel_status():
    lock = threading.Lock()
    cancelled = False
    checksum = 0
    for _ in range(100000):
        with lock:
            checksum += int(cancelled)
    return checksum


def crypto_sha256():
    data = b"0123456789abcdef" * 4
    checksum = 0
    for _ in range(10000):
        checksum += len(hashlib.sha256(data).digest())
    return checksum


def csv_parse():
    content = "a,b\r\n1,2\r\n3,4\r\n"
    checksum = 0
    for _ in range(10000):
        checksum += len(list(csv.reader(io.StringIO(content), strict=True)))
    return checksum


def decimal_add():
    left = decimal.Decimal("12345.67")
    right = decimal.Decimal("89.01")
    unit = decimal.Decimal("0.01")
    checksum = 0
    for _ in range(10000):
        value = (left + right).quantize(unit, rounding=decimal.ROUND_HALF_EVEN)
        checksum += len(str(value))
    return checksum


def dictionary_contains():
    values = {i: i for i in range(128)}
    checksum = 0
    for i in range(100000):
        checksum += int(i % 128 in values)
    return checksum


def encoding_utf8():
    checksum = 0
    for _ in range(20000):
        checksum += len("Hello, 世界!".encode("utf-8"))
    return checksum


def env_get():
    os.environ["TX_PERF_AUDIT_VALUE"] = "sample"
    checksum = 0
    for _ in range(20000):
        checksum += len(os.getenv("TX_PERF_AUDIT_VALUE"))
    del os.environ["TX_PERF_AUDIT_VALUE"]
    return checksum


def format_text():
    checksum = 0
    for _ in range(10000):
        checksum += len("{}:{}".format("alpha", 42))
    return checksum


def json_parse():
    source = '{"a":123,"b":"hello","c":[1,2,3]}'
    checksum = 0
    for _ in range(10000):
        checksum += json.loads(source)["a"]
    return checksum


def math_sqrt():
    checksum = 0.0
    for i in range(1, 1000001):
        checksum += math.sqrt(i)
    return int(checksum)


def parse_int():
    checksum = 0
    for _ in range(100000):
        checksum += int("12345")
    return checksum


def random_int():
    source = random.Random(12345)
    checksum = 0
    for _ in range(200000):
        checksum += source.randint(0, 1000)
    return checksum


def regex_search():
    pattern = re.compile("[a-z]+[0-9]+")
    checksum = 0
    for _ in range(20000):
        result = pattern.search("prefix abc123 suffix")
        checksum += len(result.group(0))
    return checksum


def serde_json():
    value = {"$schema": 1, "number": 42, "name": "alpha"}
    checksum = 0
    for _ in range(10000):
        encoded = json.dumps(value, separators=(",", ":"))
        checksum += json.loads(encoded)["number"]
    return checksum


def statistics_mean():
    values = [float(i) for i in range(128)]
    checksum = 0
    for _ in range(10000):
        checksum += int(statistics.mean(values))
    return checksum


def test_assert():
    checksum = 0
    for i in range(100000):
        assert i >= 0
        checksum += 1
    return checksum


def unicode_nfc():
    checksum = 0
    for _ in range(10000):
        checksum += len(unicodedata.normalize("NFC", "Café"))
    return checksum


def xml_parse():
    checksum = 0
    for _ in range(10000):
        checksum += len(element_tree.fromstring("<r><a>1</a><a>2</a></r>").tag)
    return checksum


def main():
    for name in (
        "algorithm_sort", "bytes_hex", "cancel_status", "crypto_sha256",
        "csv_parse", "decimal_add", "dictionary_contains", "encoding_utf8",
        "env_get", "format_text", "json_parse", "math_sqrt", "parse_int",
        "random_int", "regex_search", "serde_json", "statistics_mean",
        "test_assert", "unicode_nfc", "xml_parse",
    ):
        measure(name, globals()[name])


if __name__ == "__main__":
    main()
