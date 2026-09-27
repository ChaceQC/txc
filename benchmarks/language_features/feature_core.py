"""Scalar operations and call binding workloads."""

from time import time_ns


def runtime_seed():
    return 1 if time_ns() > 0 else 2


def add_int(left, right):
    return left + right


def add_float(left, right):
    return left + right


def recursive_sum(value):
    if value == 0:
        return 0
    return value + recursive_sum(value - 1)


def collect(first, *args, **kwargs):
    return first + len(args) + len(kwargs) + args[0] + kwargs["bonus"]


def scalar_control():
    checksum = 0
    for i in range(1, 500001):
        if i <= 250000 and i > 0:
            checksum += i
        else:
            checksum -= i
    return checksum


def updates():
    checksum = 0
    value = runtime_seed()
    for i in range(1, 500001):
        value += i
        value += 1
        checksum += value
        value -= value // 7
        value -= 1
        if value > 1000000:
            value -= 1000000
    return checksum


def while_logic():
    checksum = 0
    index = 0
    while index < 500000:
        index += 1
        if index <= 250000 or (index > 500000 and index < 0):
            checksum += index
        else:
            checksum -= index
    return checksum


def float_arithmetic():
    checksum = 0.0
    for i in range(1, 500001):
        checksum += float(i) / 2.0
    return int(checksum)


def overloads():
    # Python 无基于参数类型的函数重载，使用已解析目标的两个函数。
    checksum = runtime_seed()
    for i in range(1, 100001):
        checksum += add_int(i, 3) + int(add_float(1.0, 2.0))
        checksum -= checksum // 1000000
    return checksum


def named_arguments():
    checksum = runtime_seed()
    for i in range(1, 200001):
        checksum += add_int(right=3, left=i)
        checksum -= checksum // 1000000
    return checksum


def recursion():
    offset = runtime_seed()
    checksum = 0
    for i in range(1, 10001):
        depth = 20 + offset + i // 1000 - (i // 10000) * 10
        checksum += recursive_sum(depth)
    return checksum


def variadic_unpack():
    extra = [2, 3]
    named = {"bonus": 4}
    checksum = 0
    for i in range(1, 10001):
        checksum += collect(i, *extra, **named)
    return checksum


def string_conversion():
    checksum = 0
    for i in range(1, 50001):
        encoded = str(i)
        checksum += int(encoded)
        checksum += len("id:" + encoded)
    return checksum
