import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;

import java.io.ByteArrayInputStream;
import java.math.BigDecimal;
import java.math.RoundingMode;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.text.Normalizer;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HexFormat;
import java.util.Map;
import java.util.Random;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;

public class ComputeBench
{
    private static volatile String number_text = "12345";
    private static final Object cancel_lock = new Object();
    private static volatile boolean cancelled = false;

    interface Operation
    {
        long run() throws Exception;
    }

    private static void measure(String name, Operation operation) throws Exception
    {
        operation.run();
        long start = System.nanoTime();
        long checksum = operation.run();
        long elapsed = (System.nanoTime() - start) / 1000;
        System.out.println(name);
        System.out.println(elapsed);
        System.out.println(checksum);
    }

    private static long algorithm_sort()
    {
        int[] values = new int[256];
        for (int i = 0; i < 256; ++i)
        {
            values[i] = 255 - i;
        }
        long checksum = 0;
        for (int i = 0; i < 5000; ++i)
        {
            int[] ordered = values.clone();
            Arrays.sort(ordered);
            checksum += ordered[0] + ordered[255];
        }
        return checksum;
    }

    private static long bytes_hex()
    {
        byte[] data = "Hello, 世界!".getBytes(StandardCharsets.UTF_8);
        HexFormat hex = HexFormat.of();
        long checksum = 0;
        for (int i = 0; i < 20000; ++i)
        {
            checksum += hex.parseHex(hex.formatHex(data)).length;
        }
        return checksum;
    }

    private static long cancel_status()
    {
        long checksum = 0;
        for (int i = 0; i < 100000; ++i)
        {
            synchronized (cancel_lock)
            {
                checksum += cancelled ? 1 : 0;
            }
        }
        return checksum;
    }

    private static long crypto_sha256() throws Exception
    {
        byte[] data = "0123456789abcdef".repeat(4).getBytes(StandardCharsets.UTF_8);
        long checksum = 0;
        for (int i = 0; i < 10000; ++i)
        {
            checksum += MessageDigest.getInstance("SHA-256").digest(data).length;
        }
        return checksum;
    }

    private static long decimal_add()
    {
        BigDecimal left = new BigDecimal("12345.67");
        BigDecimal right = new BigDecimal("89.01");
        long checksum = 0;
        for (int i = 0; i < 10000; ++i)
        {
            BigDecimal value = left.add(right).setScale(2, RoundingMode.HALF_EVEN);
            checksum += value.toPlainString().length();
        }
        return checksum;
    }

    private static long dictionary_contains()
    {
        Map<Integer, Integer> values = new HashMap<>();
        for (int i = 0; i < 128; ++i)
        {
            values.put(i, i);
        }
        long checksum = 0;
        for (int i = 0; i < 100000; ++i)
        {
            checksum += values.containsKey(i % 128) ? 1 : 0;
        }
        return checksum;
    }

    private static long encoding_utf8()
    {
        long checksum = 0;
        for (int i = 0; i < 20000; ++i)
        {
            checksum += "Hello, 世界!".getBytes(StandardCharsets.UTF_8).length;
        }
        return checksum;
    }

    private static long env_get()
    {
        long checksum = 0;
        for (int i = 0; i < 20000; ++i)
        {
            checksum += System.getenv("TX_PERF_AUDIT_VALUE").length();
        }
        return checksum;
    }

    private static long format_text()
    {
        long checksum = 0;
        for (int i = 0; i < 10000; ++i)
        {
            checksum += String.format("%s:%d", "alpha", 42).length();
        }
        return checksum;
    }

    private static long json_parse(ObjectMapper mapper) throws Exception
    {
        String source = "{\"a\":123,\"b\":\"hello\",\"c\":[1,2,3]}";
        long checksum = 0;
        for (int i = 0; i < 10000; ++i)
        {
            checksum += mapper.readTree(source).get("a").asInt();
        }
        return checksum;
    }

    private static long math_sqrt()
    {
        double checksum = 0;
        for (int i = 1; i <= 1000000; ++i)
        {
            checksum += Math.sqrt(i);
        }
        return (long)checksum;
    }

    private static long parse_int()
    {
        long checksum = 0;
        for (int i = 0; i < 100000; ++i)
        {
            checksum += Long.parseLong(number_text);
        }
        return checksum;
    }

    private static long random_int()
    {
        Random source = new Random(12345);
        long checksum = 0;
        for (int i = 0; i < 200000; ++i)
        {
            checksum += source.nextInt(1001);
        }
        return checksum;
    }

    private static long regex_search()
    {
        Pattern pattern = Pattern.compile("[a-z]+[0-9]+");
        long checksum = 0;
        for (int i = 0; i < 20000; ++i)
        {
            Matcher result = pattern.matcher("prefix abc123 suffix");
            if (result.find())
            {
                checksum += result.group().length();
            }
        }
        return checksum;
    }

    private static long serde_json(ObjectMapper mapper) throws Exception
    {
        Map<String, Object> value = new HashMap<>();
        value.put("$schema", 1);
        value.put("number", 42);
        value.put("name", "alpha");
        long checksum = 0;
        for (int i = 0; i < 10000; ++i)
        {
            String encoded = mapper.writeValueAsString(value);
            JsonNode decoded = mapper.readTree(encoded);
            checksum += decoded.get("number").asInt();
        }
        return checksum;
    }

    private static long statistics_mean()
    {
        double[] values = new double[128];
        for (int i = 0; i < 128; ++i)
        {
            values[i] = i;
        }
        long checksum = 0;
        for (int i = 0; i < 10000; ++i)
        {
            double sum = 0;
            for (double value : values)
            {
                sum += value;
            }
            checksum += (long)(sum / values.length);
        }
        return checksum;
    }

    private static long test_assert()
    {
        long checksum = 0;
        for (int i = 0; i < 100000; ++i)
        {
            if (i < 0)
            {
                throw new AssertionError("negative index");
            }
            ++checksum;
        }
        return checksum;
    }

    private static long unicode_nfc()
    {
        long checksum = 0;
        for (int i = 0; i < 10000; ++i)
        {
            checksum += Normalizer.normalize("Café", Normalizer.Form.NFC).length();
        }
        return checksum;
    }

    private static long xml_parse(DocumentBuilder builder) throws Exception
    {
        byte[] source = "<r><a>1</a><a>2</a></r>".getBytes(StandardCharsets.UTF_8);
        long checksum = 0;
        for (int i = 0; i < 10000; ++i)
        {
            checksum += builder.parse(new ByteArrayInputStream(source))
                .getDocumentElement().getTagName().length();
        }
        return checksum;
    }

    public static void main(String[] args) throws Exception
    {
        ObjectMapper mapper = new ObjectMapper();
        DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
        DocumentBuilder builder = factory.newDocumentBuilder();
        measure("algorithm_sort", ComputeBench::algorithm_sort);
        measure("bytes_hex", ComputeBench::bytes_hex);
        measure("cancel_status", ComputeBench::cancel_status);
        measure("crypto_sha256", ComputeBench::crypto_sha256);
        measure("decimal_add", ComputeBench::decimal_add);
        measure("dictionary_contains", ComputeBench::dictionary_contains);
        measure("encoding_utf8", ComputeBench::encoding_utf8);
        measure("env_get", ComputeBench::env_get);
        measure("format_text", ComputeBench::format_text);
        measure("json_parse", () -> json_parse(mapper));
        measure("math_sqrt", ComputeBench::math_sqrt);
        measure("parse_int", ComputeBench::parse_int);
        measure("random_int", ComputeBench::random_int);
        measure("regex_search", ComputeBench::regex_search);
        measure("serde_json", () -> serde_json(mapper));
        measure("statistics_mean", ComputeBench::statistics_mean);
        measure("test_assert", ComputeBench::test_assert);
        measure("unicode_nfc", ComputeBench::unicode_nfc);
        measure("xml_parse", () -> xml_parse(builder));
    }
}
