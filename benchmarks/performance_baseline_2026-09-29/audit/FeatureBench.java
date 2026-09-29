import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Map;
import java.util.OptionalInt;
import java.util.PriorityQueue;
import java.util.Set;
import java.util.function.IntUnaryOperator;

public class FeatureBench
{
    interface Operation
    {
        long run();
    }

    private static void measure(String name, Operation operation)
    {
        operation.run();
        long start = System.nanoTime();
        long checksum = operation.run();
        System.out.println(name);
        System.out.println((System.nanoTime() - start) / 1000);
        System.out.println(checksum);
    }

    private static long vector_push()
    {
        ArrayList<Integer> values = new ArrayList<>();
        for (int i = 0; i < 100000; ++i)
        {
            values.add(i);
        }
        return values.get(99999) + values.size();
    }

    private static long vector_index()
    {
        int seed = System.currentTimeMillis() > 0 ? 7 : 8;
        ArrayList<Integer> values = new ArrayList<>();
        for (int i = 0; i < 1024; ++i)
        {
            values.add(seed);
        }
        long checksum = 0;
        for (int i = 0; i < 500000; ++i)
        {
            checksum += values.get(i % 1024);
        }
        return checksum;
    }

    private static long map_lookup()
    {
        Map<Integer, Integer> values = new HashMap<>();
        for (int i = 0; i < 128; ++i)
        {
            values.put(i, i);
        }
        long checksum = 0;
        for (int i = 0; i < 100000; ++i)
        {
            checksum += values.get(i % 128);
        }
        return checksum;
    }

    private static long set_contains()
    {
        Set<Integer> values = new HashSet<>();
        for (int i = 0; i < 128; ++i)
        {
            values.add(i);
        }
        long checksum = 0;
        for (int i = 0; i < 100000; ++i)
        {
            checksum += values.contains(i % 128) ? 1 : 0;
        }
        return checksum;
    }

    private static long heap_push_pop()
    {
        PriorityQueue<Integer> values = new PriorityQueue<>();
        for (int i = 0; i < 50000; ++i)
        {
            values.add(i % 1000);
        }
        long checksum = 0;
        for (int i = 0; i < 50000; ++i)
        {
            checksum += values.element();
            values.remove();
        }
        return checksum;
    }

    private static long queue_push_pop()
    {
        ArrayDeque<Integer> values = new ArrayDeque<>();
        for (int i = 0; i < 100000; ++i)
        {
            values.addLast(i);
        }
        long checksum = 0;
        for (int i = 0; i < 100000; ++i)
        {
            checksum += values.removeFirst();
        }
        return checksum;
    }

    private static long iterator_snapshot()
    {
        ArrayList<Integer> values = new ArrayList<>();
        for (int i = 0; i < 1000; ++i)
        {
            values.add(i);
        }
        long checksum = 0;
        for (int cycle = 0; cycle < 100; ++cycle)
        {
            ArrayList<Integer> snapshot = new ArrayList<>(values);
            Iterator<Integer> cursor = snapshot.iterator();
            for (int i = 0; i < 1000; ++i)
            {
                checksum += cursor.next();
            }
        }
        return checksum;
    }

    private static long option_value()
    {
        long checksum = 0;
        for (int i = 0; i < 100000; ++i)
        {
            OptionalInt value = OptionalInt.of(i % 8);
            checksum += value.getAsInt();
        }
        return checksum;
    }

    private static long function_value()
    {
        IntUnaryOperator operation = value -> value * 2;
        long checksum = 0;
        for (int i = 0; i < 100000; ++i)
        {
            checksum += operation.applyAsInt(i);
        }
        return checksum;
    }

    private static long closure_bind()
    {
        int base = 5;
        IntUnaryOperator operation = value -> base + value;
        long checksum = 0;
        for (int i = 0; i < 100000; ++i)
        {
            checksum += operation.applyAsInt(i);
        }
        return checksum;
    }

    public static void main(String[] args)
    {
        measure("vector_push", FeatureBench::vector_push);
        measure("vector_index", FeatureBench::vector_index);
        measure("map_lookup", FeatureBench::map_lookup);
        measure("set_contains", FeatureBench::set_contains);
        measure("heap_push_pop", FeatureBench::heap_push_pop);
        measure("queue_push_pop", FeatureBench::queue_push_pop);
        measure("iterator_snapshot", FeatureBench::iterator_snapshot);
        measure("option_value", FeatureBench::option_value);
        measure("function_value", FeatureBench::function_value);
        measure("closure_bind", FeatureBench::closure_bind);
    }
}
