final class feature_objects
{
    private interface readable
    {
        long read();
    }

    private static final class point
    {
        final long x;
        final long y;

        point(long x, long y)
        {
            this.x = x;
            this.y = y;
        }

        point plus(point other)
        {
            return new point(x + other.x, y + other.y);
        }

        boolean same(point other)
        {
            return x == other.x && y == other.y;
        }
    }

    private static class counter
    {
        protected long value;

        counter(long value)
        {
            this.value = value;
        }

        long increase(long delta)
        {
            value += delta;
            return value;
        }

        long increase(double delta)
        {
            value += (long) delta;
            return value;
        }

        long read()
        {
            return value;
        }

        long plus(counter other)
        {
            return value + other.read();
        }
    }

    private static final class adjusted_counter extends counter implements readable
    {
        adjusted_counter(long value)
        {
            super(value);
        }

        @Override
        public long read()
        {
            return super.read() + 1;
        }

        @Override
        long plus(counter other)
        {
            return super.plus(other) + 1;
        }
    }

    private static final class alternate_counter extends counter implements readable
    {
        alternate_counter(long value)
        {
            super(value);
        }

        @Override
        public long read()
        {
            return value + 1;
        }

        @Override
        long plus(counter other)
        {
            return super.plus(other) + 1;
        }
    }

    private feature_objects()
    {
    }

    static long struct_operators()
    {
        long checksum = 0;
        for (long i = 1; i <= 100000; ++i)
        {
            point current = new point(i, 2);
            current = current.plus(new point(3, 4));
            if (current.same(new point(i + 3, 6)))
            {
                checksum += current.x;
            }
        }
        return checksum;
    }

    static long class_methods()
    {
        counter value = new counter(0);
        long checksum = 0;
        for (long i = 1; i <= 200000; ++i)
        {
            checksum += value.increase(i / 1000 + 1);
            checksum += value.increase(1.0);
        }
        return checksum;
    }

    static long virtual_interface()
    {
        readable interface_view = new adjusted_counter(7);
        if (System.currentTimeMillis() <= 0)
        {
            interface_view = new alternate_counter(7);
        }
        counter parent_view = (counter) interface_view;
        long checksum = 0;
        for (int i = 0; i < 200000; ++i)
        {
            checksum += interface_view.read() + parent_view.read();
        }
        return checksum;
    }

    static long class_operator()
    {
        counter left = new adjusted_counter(7);
        if (System.currentTimeMillis() <= 0)
        {
            left = new alternate_counter(7);
        }
        counter right = new counter(2);
        long checksum = 0;
        for (int i = 0; i < 100000; ++i)
        {
            checksum += left.plus(right);
        }
        return checksum;
    }

    static long runtime_cast()
    {
        readable interface_view = new adjusted_counter(7);
        long checksum = 0;
        for (int i = 0; i < 100000; ++i)
        {
            counter parent_view = (counter) interface_view;
            checksum += parent_view.read();
        }
        return checksum;
    }

    static long module_call()
    {
        long checksum = 0;
        for (long i = 1; i <= 100000; ++i)
        {
            feature_workload.pair item = new feature_workload.pair(i, i + 1);
            checksum += feature_workload.bump(i) + feature_workload.measure(item);
        }
        return checksum;
    }
}
