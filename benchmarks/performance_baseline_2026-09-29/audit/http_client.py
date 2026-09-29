import time
import sys

import httpx
import requests


def measure(name, operation):
    start = time.perf_counter_ns()
    checksum = operation()
    elapsed = (time.perf_counter_ns() - start) // 1000
    print(name)
    print(elapsed)
    print(checksum)


def httpx_get():
    url = "http://127.0.0.1:19790/data"
    checksum = 0
    for _ in range(100):
        response = httpx.get(url, timeout=5.0)
        checksum += int(response.status_code == 200 and response.text == "OK")
    return checksum


def requests_get():
    url = "http://127.0.0.1:19790/data"
    checksum = 0
    for _ in range(100):
        response = requests.get(url, timeout=5.0)
        checksum += int(response.status_code == 200 and response.text == "OK")
    return checksum


if __name__ == "__main__":
    if "--requests-only" not in sys.argv:
        measure("httpx_get", httpx_get)
    measure("requests_get", requests_get)
