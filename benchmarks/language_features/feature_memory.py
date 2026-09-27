"""Deep copy, object release, and cycle collection workloads."""

from copy import deepcopy
import gc


class record:
    __slots__ = ("items",)

    def __init__(self, items):
        self.items = items


class tracked:
    __slots__ = ("totals",)

    def __init__(self, totals):
        self.totals = totals

    def __del__(self):
        self.totals[0] += 1


class cycle_node:
    __slots__ = ("totals", "next")

    def __init__(self, totals):
        self.totals = totals
        self.next = None

    def __del__(self):
        self.totals[0] += 1


def make_cycle(totals):
    node = cycle_node(totals)
    node.next = node


def deep_copy():
    source = record({"numbers": [1, 2, 3]})
    checksum = 0
    for i in range(1, 10001):
        copied = deepcopy(source)
        copied.items["numbers"][0] = i
        checksum += copied.items["numbers"][0]
    checksum += source.items["numbers"][0]
    return checksum


def copy_cycle():
    source = [None, 7]
    source[0] = source
    checksum = 0
    for i in range(1, 10001):
        copied = deepcopy(source)
        copied[0][1] = i
        checksum += copied[1]
        copied[0] = None
    checksum += source[1]
    source[0] = None
    return checksum


def deinit():
    totals = [0]
    for _ in range(20000):
        item = tracked(totals)
        item = None
    return totals[0]


def cycle_gc():
    totals = [0]
    for _ in range(1000):
        make_cycle(totals)
    # Python 通过真实循环 GC 回收；TX 的安全点策略与其不同。
    attempts = 0
    while totals[0] < 1000 and attempts < 4096:
        padding = []
        gc.collect()
        attempts += 1
    return totals[0]
