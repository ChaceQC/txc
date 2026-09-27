"""Python 3.12 counterparts for the 22 TX language feature workloads."""

from time import perf_counter_ns

import feature_containers
import feature_core
import feature_memory
import feature_objects


CASES = (
    ("scalar_control", feature_core.scalar_control),
    ("updates", feature_core.updates),
    ("while_logic", feature_core.while_logic),
    ("float_arithmetic", feature_core.float_arithmetic),
    ("overloads", feature_core.overloads),
    ("named_arguments", feature_core.named_arguments),
    ("recursion", feature_core.recursion),
    ("variadic_unpack", feature_core.variadic_unpack),
    ("array_destructure", feature_containers.array_destructure),
    ("array_padded", feature_containers.array_padded),
    ("dict_iteration", feature_containers.dict_iteration),
    ("struct_operators", feature_objects.struct_operators),
    ("class_methods", feature_objects.class_methods),
    ("virtual_interface", feature_objects.virtual_interface),
    ("class_operator", feature_objects.class_operator),
    ("runtime_cast", feature_objects.runtime_cast),
    ("module_call", feature_objects.module_call),
    ("string_conversion", feature_core.string_conversion),
    ("deep_copy", feature_memory.deep_copy),
    ("copy_cycle", feature_memory.copy_cycle),
    ("deinit", feature_memory.deinit),
    ("cycle_gc", feature_memory.cycle_gc),
)


def main():
    for name, operation in CASES:
        started = perf_counter_ns()
        checksum = operation()
        print(name)
        print((perf_counter_ns() - started) / 1_000_000)
        print(checksum)


if __name__ == "__main__":
    main()
