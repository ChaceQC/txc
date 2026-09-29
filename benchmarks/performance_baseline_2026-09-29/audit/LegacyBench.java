import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.stream.Stream;

public class LegacyBench
{
    interface Operation
    {
        long run() throws Exception;
    }

    private static void measure(String name, Operation operation) throws Exception
    {
        long start = System.nanoTime();
        long checksum = operation.run();
        double elapsed = (System.nanoTime() - start) / 1_000_000.0;
        System.out.println(name);
        System.out.println(elapsed);
        System.out.println(checksum);
    }

    private static long string_case()
    {
        long checksum = 0;
        for (int i = 0; i < 10000; ++i)
        {
            checksum += "alpha,beta,alpha".replace("alpha", "x")
                .split(",", -1).length;
        }
        return checksum;
    }

    private static long array_case()
    {
        List<Integer> values = new ArrayList<>();
        for (int i = 0; i < 64; ++i)
        {
            values.add(i);
        }
        long checksum = 0;
        for (int i = 0; i < 3000; ++i)
        {
            List<Integer> combined = new ArrayList<>(values);
            combined.addAll(values);
            Collections.reverse(combined);
            List<Integer> middle = new ArrayList<>(combined.subList(16, 112));
            checksum += middle.get(0) + middle.size();
        }
        return checksum;
    }

    private static long dict_case()
    {
        Map<Integer, Integer> values = new HashMap<>();
        long checksum = 0;
        for (int repetition = 1; repetition <= 100; ++repetition)
        {
            for (int i = 0; i < 128; ++i)
            {
                values.put(i, i + repetition);
                checksum += values.get(i);
            }
        }
        return checksum;
    }

    private static long path_case()
    {
        long checksum = 0;
        for (int i = 0; i < 10000; ++i)
        {
            Path joined = Path.of("tx_build").resolve("sub/data.txt");
            String name = joined.getFileName().toString();
            checksum += name.length() - name.lastIndexOf('.');
        }
        return checksum;
    }

    private static long fs_case() throws Exception
    {
        long checksum = 0;
        for (int i = 0; i < 200; ++i)
        {
            try (Stream<Path> entries = Files.list(Path.of("tx/stdlib")))
            {
                checksum += entries.sorted().count();
            }
            if (Files.isRegularFile(Path.of("tx/stdlib/math.txh")))
            {
                ++checksum;
            }
        }
        return checksum;
    }

    private static long file_case() throws Exception
    {
        String content = "0123456789abcdef".repeat(4);
        Path path = Path.of("tx_build/perf_audit_20260927/legacy_java.txt");
        long checksum = 0;
        for (int i = 0; i < 300; ++i)
        {
            Files.writeString(path, content, StandardCharsets.UTF_8);
            checksum += Files.readString(path, StandardCharsets.UTF_8).length();
        }
        return checksum;
    }

    private static long io_case()
    {
        for (int i = 0; i < 5000; ++i)
        {
            System.err.print("x");
        }
        return 5000;
    }

    private static long time_case()
    {
        long checksum = 0;
        for (int i = 0; i < 100000; ++i)
        {
            checksum += System.currentTimeMillis() > 0 ? 1 : 0;
        }
        return checksum;
    }

    public static void main(String[] args) throws Exception
    {
        measure("string", LegacyBench::string_case);
        measure("array", LegacyBench::array_case);
        measure("dict_hash", LegacyBench::dict_case);
        measure("path", LegacyBench::path_case);
        measure("fs", LegacyBench::fs_case);
        measure("file", LegacyBench::file_case);
        measure("io", LegacyBench::io_case);
        measure("time", LegacyBench::time_case);
    }
}
