import java.util.function.LongSupplier;

public class compare_java
{
    private static void report(String name, LongSupplier operation)
    {
        long started = System.nanoTime();
        long checksum = operation.getAsLong();
        double elapsed_ms = (System.nanoTime() - started) / 1_000_000.0;
        System.out.println(name);
        System.out.println(elapsed_ms);
        System.out.println(checksum);
    }

    public static void main(String[] args)
    {
        report("scalar_control", feature_core::scalar_control);
        report("updates", feature_core::updates);
        report("while_logic", feature_core::while_logic);
        report("float_arithmetic", feature_core::float_arithmetic);
        report("overloads", feature_core::overloads);
        report("named_arguments", feature_core::named_arguments);
        report("recursion", feature_core::recursion);
        report("variadic_unpack", feature_core::variadic_unpack);
        report("array_destructure", feature_containers::array_destructure);
        report("array_padded", feature_containers::array_padded);
        report("dict_iteration", feature_containers::dict_iteration);
        report("struct_operators", feature_objects::struct_operators);
        report("class_methods", feature_objects::class_methods);
        report("virtual_interface", feature_objects::virtual_interface);
        report("class_operator", feature_objects::class_operator);
        report("runtime_cast", feature_objects::runtime_cast);
        report("module_call", feature_objects::module_call);
        report("string_conversion", feature_core::string_conversion);
        report("deep_copy", feature_memory::deep_copy);
        report("copy_cycle", feature_memory::copy_cycle);
        report("deinit", feature_memory::deinit);
        report("cycle_gc", feature_memory::cycle_gc);
    }
}
