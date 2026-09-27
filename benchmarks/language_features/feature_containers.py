"""Mixed arrays and a typed-key dictionary counterpart."""


def array_destructure():
    checksum = 0
    for i in range(1, 20001):
        pair = [i, i + 1]
        left, right = pair
        pair[1] = right + 1
        for item in pair:
            checksum += int(item)
    return checksum


def array_padded():
    checksum = 0
    for i in range(1, 50001):
        values = [i, i + 1, None, None]
        if values[3] is None:
            checksum += len(values)
        values[2] = i
        checksum += int(values[2])
    return checksum


def dict_iteration():
    # Python 的 True 与 1 是同一个键；标签保留 TX 的类型化键区别。
    values = {("int", 1): 1, ("str", "two"): 2, ("bool", True): 3}
    text_key = ("str", "two")
    checksum = 0
    for i in range(1, 100001):
        values[text_key] = i
        for _ in values:
            checksum += 1
        checksum += int(values[text_key])
    return checksum
