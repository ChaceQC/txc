import java.util.ArrayList;
import java.util.HashMap;
import java.util.IdentityHashMap;
import java.util.List;
import java.util.Map;

final class feature_memory
{
    private record record_value(Map<String, List<Object>> items)
    {
    }

    private static final class tracked
    {
        final long[] totals;

        tracked(long[] totals)
        {
            this.totals = totals;
        }

        void close()
        {
            totals[0] += 1;
        }
    }

    private static final class cycle_array
    {
        final Object[] items = new Object[2];
    }

    private static final class cycle_node
    {
        final long[] totals;
        cycle_node next;

        cycle_node(long[] totals)
        {
            this.totals = totals;
        }

        void close()
        {
            totals[0] += 1;
        }
    }

    private static final class cycle_registry
    {
        private final List<cycle_node> nodes = new ArrayList<>();
        private int allocations;

        void add(cycle_node node)
        {
            nodes.add(node);
            allocations += 1;
        }

        void note_allocation()
        {
            allocations += 1;
        }

        void safepoint()
        {
            int threshold = Math.max(64, nodes.size() / 2);
            if (!nodes.isEmpty() && allocations >= threshold)
            {
                collect();
            }
        }

        private void collect()
        {
            // Java 无确定时机的析构回调；这里只回收本用例的单节点自环。
            for (cycle_node node : nodes)
            {
                if (node.next == node)
                {
                    node.next = null;
                    node.close();
                }
            }
            nodes.clear();
            allocations = 0;
        }
    }

    private feature_memory()
    {
    }

    private static record_value copy_record(record_value source)
    {
        Map<String, List<Object>> copied = new HashMap<>();
        for (Map.Entry<String, List<Object>> entry : source.items().entrySet())
        {
            copied.put(entry.getKey(), new ArrayList<>(entry.getValue()));
        }
        return new record_value(copied);
    }

    private static cycle_array copy_cycle_graph(cycle_array source,
                                                IdentityHashMap<cycle_array, cycle_array> seen)
    {
        cycle_array found = seen.get(source);
        if (found != null)
        {
            return found;
        }
        cycle_array copied = new cycle_array();
        seen.put(source, copied);
        for (int index = 0; index < source.items.length; ++index)
        {
            Object item = source.items[index];
            copied.items[index] = item instanceof cycle_array
                ? copy_cycle_graph((cycle_array) item, seen) : item;
        }
        return copied;
    }

    static long deep_copy()
    {
        Map<String, List<Object>> items = new HashMap<>();
        items.put("numbers", new ArrayList<>(List.of(1L, 2L, 3L)));
        record_value source = new record_value(items);
        long checksum = 0;
        for (long i = 1; i <= 10000; ++i)
        {
            record_value copied = copy_record(source);
            copied.items().get("numbers").set(0, i);
            checksum += (long) copied.items().get("numbers").get(0);
        }
        checksum += (long) source.items().get("numbers").get(0);
        return checksum;
    }

    static long copy_cycle()
    {
        cycle_array source = new cycle_array();
        source.items[0] = source;
        source.items[1] = 7L;
        long checksum = 0;
        for (long i = 1; i <= 10000; ++i)
        {
            cycle_array copied = copy_cycle_graph(source, new IdentityHashMap<>());
            cycle_array self = (cycle_array) copied.items[0];
            self.items[1] = i;
            checksum += (long) copied.items[1];
            copied.items[0] = null;
        }
        checksum += (long) source.items[1];
        source.items[0] = null;
        return checksum;
    }

    static long deinit()
    {
        long[] totals = {0};
        for (int i = 0; i < 20000; ++i)
        {
            tracked item = new tracked(totals);
            item.close();
        }
        return totals[0];
    }

    static long cycle_gc()
    {
        long[] totals = {0};
        cycle_registry registry = new cycle_registry();
        for (int i = 0; i < 1000; ++i)
        {
            cycle_node node = new cycle_node(totals);
            node.next = node;
            registry.add(node);
            registry.safepoint();
        }
        int attempts = 0;
        while (totals[0] < 1000 && attempts < 4096)
        {
            Object[] padding = new Object[0];
            registry.note_allocation();
            registry.safepoint();
            attempts += 1;
        }
        return totals[0];
    }
}
