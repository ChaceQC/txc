"""Structure, method, interface, and cross-module workloads."""

from time import time_ns

import feature_workload


class point:
    __slots__ = ("x", "y")

    def __init__(self, x, y):
        self.x = x
        self.y = y

    def __add__(self, other):
        return point(self.x + other.x, self.y + other.y)

    def __iadd__(self, other):
        return self + other

    def __eq__(self, other):
        return self.x == other.x and self.y == other.y


class readable:
    def read(self):
        raise NotImplementedError


class counter:
    __slots__ = ("value",)

    def __init__(self, value):
        self.value = value

    def increase(self, delta):
        self.value += int(delta)
        return self.value

    def read(self):
        return self.value

    def __add__(self, other):
        return self.value + other.read()


class adjusted_counter(counter, readable):
    def read(self):
        return super().read() + 1

    def __add__(self, other):
        return super().__add__(other) + 1


class alternate_counter(counter, readable):
    def read(self):
        return self.value + 1

    def __add__(self, other):
        return super().__add__(other) + 1


def struct_operators():
    checksum = 0
    for i in range(1, 100001):
        current = point(y=2, x=i)
        current += point(3, 4)
        if current == point(i + 3, 6):
            checksum += current.x
    return checksum


def class_methods():
    value = counter(0)
    checksum = 0
    for i in range(1, 200001):
        checksum += value.increase(i // 1000 + 1)
        checksum += value.increase(1.0)
    return checksum


def virtual_interface():
    interface_view = adjusted_counter(7)
    if time_ns() <= 0:
        interface_view = alternate_counter(7)
    parent_view = interface_view
    checksum = 0
    for _ in range(200000):
        checksum += interface_view.read() + parent_view.read()
    return checksum


def class_operator():
    left = adjusted_counter(7)
    if time_ns() <= 0:
        left = alternate_counter(7)
    right = counter(2)
    checksum = 0
    for _ in range(100000):
        checksum += left + right
    return checksum


def runtime_cast():
    interface_view = adjusted_counter(7)
    checksum = 0
    for _ in range(100000):
        if not isinstance(interface_view, counter):
            raise TypeError("counter cast failed")
        parent_view = interface_view
        checksum += parent_view.read()
    return checksum


def module_call():
    checksum = 0
    for i in range(1, 100001):
        item = feature_workload.pair(i, i + 1)
        checksum += feature_workload.bump(i) + feature_workload.measure(item)
    return checksum
