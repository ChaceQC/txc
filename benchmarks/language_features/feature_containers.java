import java.util.HashMap;
import java.util.Map;

final class feature_containers
{
    private record typed_key(String type, Object value)
    {
    }

    private feature_containers()
    {
    }

    static long array_destructure()
    {
        long checksum = 0;
        for (long i = 1; i <= 20000; ++i)
        {
            Object[] pair = {i, i + 1};
            Object left = pair[0];
            Object right = pair[1];
            pair[1] = (long) right + 1;
            for (Object item : pair)
            {
                checksum += (long) item;
            }
        }
        return checksum;
    }

    static long array_padded()
    {
        long checksum = 0;
        for (long i = 1; i <= 50000; ++i)
        {
            Object[] values = {i, i + 1, null, null};
            if (values[3] == null)
            {
                checksum += values.length;
            }
            values[2] = i;
            checksum += (long) values[2];
        }
        return checksum;
    }

    static long dict_iteration()
    {
        Map<typed_key, Object> values = new HashMap<>();
        values.put(new typed_key("int", 1L), 1L);
        values.put(new typed_key("str", "two"), 2L);
        values.put(new typed_key("bool", true), 3L);
        long checksum = 0;
        for (long i = 1; i <= 100000; ++i)
        {
            values.put(new typed_key("str", "two"), i);
            for (typed_key key : values.keySet())
            {
                checksum += 1;
            }
            checksum += (long) values.get(new typed_key("str", "two"));
        }
        return checksum;
    }
}
