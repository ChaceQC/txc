import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.Map;
import java.util.function.LongSupplier;

public class diverse_performance_java
{
    private static void report(String name, LongSupplier operation)
    {
        long started = System.nanoTime();
        long checksum = operation.getAsLong();
        long micros = (System.nanoTime() - started) / 1000;
        System.out.println(name);
        System.out.println(micros);
        System.out.println(checksum);
    }

    private static void bench_vector_scale()
    {
        int[] small = new int[1000];
        int[] large = new int[100000];
        java.util.Arrays.fill(small, 3);
        java.util.Arrays.fill(large, 3);
        report("vector_scan_1k", () ->
        {
            long checksum = 0;
            for (int repetition = 0; repetition < 1000; ++repetition)
            {
                for (int value : small)
                {
                    checksum += value;
                }
            }
            return checksum;
        });
        report("vector_scan_100k", () ->
        {
            long checksum = 0;
            for (int repetition = 0; repetition < 100; ++repetition)
            {
                for (int value : large)
                {
                    checksum += value;
                }
            }
            return checksum;
        });
    }

    private static void bench_vector_access()
    {
        int[] values = new int[8192];
        for (int i = 0; i < values.length; ++i)
        {
            values[i] = i % 97;
        }
        report("vector_index_sequential", () ->
        {
            long checksum = 0;
            for (int i = 0; i < 1000000; ++i)
            {
                checksum += values[i % 8192];
            }
            return checksum;
        });
        report("vector_index_strided", () ->
        {
            long checksum = 0;
            for (int i = 0; i < 1000000; ++i)
            {
                checksum += values[(i * 127) % 8192];
            }
            return checksum;
        });
    }

    private static void bench_map_distribution()
    {
        Map<Integer, Integer> small = new HashMap<>();
        Map<Integer, Integer> large = new HashMap<>();
        for (int i = 0; i < 8192; ++i)
        {
            large.put(i, i);
            if (i < 128)
            {
                small.put(i, i);
            }
        }
        report("map_hit_128", () ->
        {
            long checksum = 0;
            for (int i = 0; i < 500000; ++i)
            {
                checksum += small.get(i % 128);
            }
            return checksum;
        });
        report("map_hit_8192", () ->
        {
            long checksum = 0;
            for (int i = 0; i < 500000; ++i)
            {
                checksum += large.get((i * 127) % 8192);
            }
            return checksum;
        });
        report("map_hit_10_percent", () ->
        {
            long checksum = 0;
            for (int i = 0; i < 500000; ++i)
            {
                int key = 100000 + i % 8192;
                if (i % 10 == 0)
                {
                    key = i % 8192;
                }
                if (large.containsKey(key))
                {
                    checksum += 1;
                }
            }
            return checksum;
        });
    }

    private static void bench_dictionary_keys()
    {
        Map<Object, Integer> values = new HashMap<>();
        values.put(7, 1);
        values.put("seven", 2);
        values.put(true, 3);
        report("dictionary_int_hit", () ->
        {
            long checksum = 0;
            for (int i = 0; i < 200000; ++i)
            {
                if (values.containsKey(7))
                {
                    checksum += 1;
                }
            }
            return checksum;
        });
        report("dictionary_text_hit", () ->
        {
            long checksum = 0;
            for (int i = 0; i < 200000; ++i)
            {
                if (values.containsKey("seven"))
                {
                    checksum += 1;
                }
            }
            return checksum;
        });
        report("dictionary_mostly_miss", () ->
        {
            long checksum = 0;
            for (int i = 0; i < 200000; ++i)
            {
                if (values.containsKey(i))
                {
                    checksum += 1;
                }
            }
            return checksum;
        });
    }

    private static String format_dynamic(String pattern, int value)
    {
        int first = pattern.indexOf("{}");
        int second = pattern.indexOf("{}", first + 2);
        if (first < 0 || second < 0)
        {
            throw new IllegalArgumentException("unexpected format pattern");
        }
        return pattern.substring(0, first) + "item"
            + pattern.substring(first + 2, second) + value
            + pattern.substring(second + 2);
    }

    private static void bench_format_paths()
    {
        String pattern = "{}:{}";
        report("format_literal", () ->
        {
            long checksum = 0;
            for (int i = 0; i < 50000; ++i)
            {
                checksum += ("item:" + i).length();
            }
            return checksum;
        });
        report("format_dynamic", () ->
        {
            long checksum = 0;
            for (int i = 0; i < 50000; ++i)
            {
                checksum += format_dynamic(pattern, i).length();
            }
            return checksum;
        });
    }

    private static void bench_encoding_paths()
    {
        String value = "Hello, 世界 🌍";
        String codec = "utf-8";
        report("encoding_literal", () ->
        {
            long checksum = 0;
            for (int i = 0; i < 50000; ++i)
            {
                checksum += value.getBytes(StandardCharsets.UTF_8).length;
            }
            return checksum;
        });
        report("encoding_dynamic", () ->
        {
            long checksum = 0;
            for (int i = 0; i < 50000; ++i)
            {
                checksum += value.getBytes(Charset.forName(codec)).length;
            }
            return checksum;
        });
    }

    private static long json_roundtrip(ObjectMapper mapper, Map<String, Object> value)
    {
        try
        {
            String encoded = mapper.writeValueAsString(value);
            JsonNode decoded = mapper.readTree(encoded);
            return decoded.get("id").asInt() + decoded.get("name").asText().length();
        }
        catch (JsonProcessingException exception)
        {
            throw new IllegalStateException(exception);
        }
    }

    private static void bench_serde_size()
    {
        ObjectMapper mapper = new ObjectMapper();
        Map<String, Object> short_value = Map.of("id", 7, "name", "x");
        Map<String, Object> long_value = Map.of("id", 7, "name", "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
        // Jackson 的首次类加载和 JIT 会只落在短文本行；先分别预热两种长度。
        for (int i = 0; i < 5000; ++i)
        {
            json_roundtrip(mapper, short_value);
            json_roundtrip(mapper, long_value);
        }
        report("serde_short_text", () ->
        {
            long checksum = 0;
            for (int i = 0; i < 5000; ++i)
            {
                checksum += json_roundtrip(mapper, short_value);
            }
            return checksum;
        });
        report("serde_long_text", () ->
        {
            long checksum = 0;
            for (int i = 0; i < 5000; ++i)
            {
                checksum += json_roundtrip(mapper, long_value);
            }
            return checksum;
        });
    }

    private static void bench_parse_paths()
    {
        String[] valid_inputs = new String[1000];
        String[] invalid_inputs = new String[1000];
        for (int i = 0; i < 1000; ++i)
        {
            valid_inputs[i] = Integer.toString(i);
            invalid_inputs[i] = valid_inputs[i] + "x";
        }
        report("parse_valid", () ->
        {
            long checksum = 0;
            for (int i = 0; i < 100000; ++i)
            {
                try
                {
                    Integer.parseInt(valid_inputs[i % 1000], 10);
                    checksum += 1;
                }
                catch (NumberFormatException exception)
                {
                    // 合法输入若走到这里，校验值会失败。
                }
            }
            return checksum;
        });
        report("parse_invalid", () ->
        {
            long checksum = 0;
            for (int i = 0; i < 100000; ++i)
            {
                try
                {
                    Integer.parseInt(invalid_inputs[i % 1000], 10);
                }
                catch (NumberFormatException exception)
                {
                    checksum += 1;
                }
            }
            return checksum;
        });
    }

    public static void main(String[] arguments)
    {
        bench_vector_scale();
        bench_vector_access();
        bench_map_distribution();
        bench_dictionary_keys();
        bench_format_paths();
        bench_encoding_paths();
        bench_serde_size();
        bench_parse_paths();
    }
}
