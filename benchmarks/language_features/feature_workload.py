"""Separate module for cross-file function calls."""


class pair:
    __slots__ = ("left", "right")

    def __init__(self, left, right):
        self.left = left
        self.right = right


def bump(value):
    return value + 7


def measure(value):
    return value.left + value.right
