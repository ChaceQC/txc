from collections import deque
from functools import partial
import heapq
import time


def measure(name, operation):
    start = time.perf_counter_ns()
    checksum = operation()
    print(name)
    print((time.perf_counter_ns() - start) // 1000)
    print(checksum)


def vector_push():
    values = []
    for i in range(100000):
        values.append(i)
    return values[99999] + len(values)


def vector_index():
    seed = 7 if time.time_ns() > 0 else 8
    values = [seed] * 1024
    checksum = 0
    for i in range(500000):
        checksum += values[i % 1024]
    return checksum


def map_lookup():
    values = {i: i for i in range(128)}
    checksum = 0
    for i in range(100000):
        checksum += values[i % 128]
    return checksum


def set_contains():
    values = set(range(128))
    checksum = 0
    for i in range(100000):
        checksum += int(i % 128 in values)
    return checksum


def heap_push_pop():
    values = []
    for i in range(50000):
        heapq.heappush(values, i % 1000)
    checksum = 0
    for _ in range(50000):
        checksum += heapq.heappop(values)
    return checksum


def queue_push_pop():
    values = deque()
    for i in range(100000):
        values.append(i)
    checksum = 0
    for _ in range(100000):
        checksum += values.popleft()
    return checksum


def iterator_snapshot():
    values = list(range(1000))
    checksum = 0
    for _ in range(100):
        snapshot = list(values)
        cursor = iter(snapshot)
        for _ in range(1000):
            checksum += next(cursor)
    return checksum


class Option:
    def __init__(self, value):
        self.item = value

    def value(self):
        if self.item is None:
            raise ValueError("missing value")
        return self.item


def option_value():
    checksum = 0
    for i in range(100000):
        value = Option(i % 8)
        checksum += value.value()
    return checksum


def double_value(value):
    return value * 2


def function_value():
    operation = double_value
    checksum = 0
    for i in range(100000):
        checksum += operation(i)
    return checksum


def add(base, value):
    return base + value


def closure_bind():
    operation = partial(add, 5)
    checksum = 0
    for i in range(100000):
        checksum += operation(i)
    return checksum


if __name__ == "__main__":
    for name in (
        "vector_push", "vector_index", "map_lookup", "set_contains",
        "heap_push_pop", "queue_push_pop", "iterator_snapshot",
        "option_value", "function_value", "closure_bind",
    ):
        measure(name, globals()[name])
