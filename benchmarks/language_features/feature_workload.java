final class feature_workload
{
    record pair(long left, long right)
    {
    }

    private feature_workload()
    {
    }

    static long bump(long value)
    {
        return value + 7;
    }

    static long measure(pair value)
    {
        return value.left() + value.right();
    }
}
