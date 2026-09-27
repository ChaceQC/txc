import java.util.HashMap;
import java.util.Map;

final class feature_core
{
    private feature_core()
    {
    }

    private static long runtime_seed()
    {
        return System.currentTimeMillis() > 0 ? 1 : 2;
    }

    private static long add(long left, long right)
    {
        return left + right;
    }

    private static double add(double left, double right)
    {
        return left + right;
    }

    private static long recursive_sum(long value)
    {
        if (value == 0)
        {
            return 0;
        }
        return value + recursive_sum(value - 1);
    }

    private static long collect(long first, Object[] args,
                                Map<String, Object> kwargs)
    {
        return first + args.length + kwargs.size() +
            (long) args[0] + (long) kwargs.get("bonus");
    }

    static long scalar_control()
    {
        long checksum = 0;
        for (long i = 1; i <= 500000; ++i)
        {
            if (i <= 250000 && i > 0)
            {
                checksum += i;
            }
            else
            {
                checksum -= i;
            }
        }
        return checksum;
    }

    static long updates()
    {
        long checksum = 0;
        long value = runtime_seed();
        for (long i = 1; i <= 500000; ++i)
        {
            value += i;
            checksum += ++value;
            value -= value / 7;
            --value;
            if (value > 1000000)
            {
                value -= 1000000;
            }
        }
        return checksum;
    }

    static long while_logic()
    {
        long checksum = 0;
        long index = 0;
        while (index < 500000)
        {
            ++index;
            if (index <= 250000 || (index > 500000 && index < 0))
            {
                checksum += index;
            }
            else
            {
                checksum -= index;
            }
        }
        return checksum;
    }

    static long float_arithmetic()
    {
        double checksum = 0.0;
        for (long i = 1; i <= 500000; ++i)
        {
            checksum += (double) i / 2.0;
        }
        return (long) checksum;
    }

    static long overloads()
    {
        long checksum = runtime_seed();
        for (long i = 1; i <= 100000; ++i)
        {
            checksum += add(i, 3L) + (long) add(1.0, 2.0);
            checksum -= checksum / 1000000;
        }
        return checksum;
    }

    static long named_arguments()
    {
        // Java 没有命名实参，和 C++ 对照一样使用已绑定顺序的调用。
        long checksum = runtime_seed();
        for (long i = 1; i <= 200000; ++i)
        {
            checksum += add(i, 3L);
            checksum -= checksum / 1000000;
        }
        return checksum;
    }

    static long recursion()
    {
        long offset = runtime_seed();
        long checksum = 0;
        for (long i = 1; i <= 10000; ++i)
        {
            long depth = 20 + offset + i / 1000 - (i / 10000) * 10;
            checksum += recursive_sum(depth);
        }
        return checksum;
    }

    static long variadic_unpack()
    {
        Object[] extra = {2L, 3L};
        Map<String, Object> named = new HashMap<>();
        named.put("bonus", 4L);
        long checksum = 0;
        for (long i = 1; i <= 10000; ++i)
        {
            checksum += collect(i, extra, named);
        }
        return checksum;
    }

    static long string_conversion()
    {
        long checksum = 0;
        for (long i = 1; i <= 50000; ++i)
        {
            String encoded = Long.toString(i);
            checksum += Long.parseLong(encoded);
            checksum += ("id:" + encoded).length();
        }
        return checksum;
    }
}
