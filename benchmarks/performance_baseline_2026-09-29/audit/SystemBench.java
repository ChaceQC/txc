import java.io.File;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public class SystemBench
{
    interface Operation
    {
        long run() throws Exception;
    }

    private static void measure(String name, Operation operation) throws Exception
    {
        long start = System.nanoTime();
        long checksum = operation.run();
        long elapsed = (System.nanoTime() - start) / 1000;
        System.out.println(name);
        System.out.println(elapsed);
        System.out.println(checksum);
    }

    private static long file_stream_rw() throws Exception
    {
        Path path = Path.of("tx_build/perf_audit_20260927/stream_java.bin");
        byte[] data = "0123456789abcdef".repeat(4).getBytes(StandardCharsets.UTF_8);
        long checksum = 0;
        for (int i = 0; i < 200; ++i)
        {
            Files.write(path, data);
            checksum += Files.readAllBytes(path).length;
        }
        return checksum;
    }

    private static long process_spawn() throws Exception
    {
        String path = new File("tx_build/perf_audit_20260927/child.exe")
            .getAbsolutePath();
        long checksum = 0;
        for (int i = 0; i < 50; ++i)
        {
            Process process = new ProcessBuilder(path).redirectOutput(
                ProcessBuilder.Redirect.DISCARD).redirectError(
                ProcessBuilder.Redirect.DISCARD).start();
            checksum += process.waitFor() == 0 ? 1 : 0;
        }
        return checksum;
    }

    public static void main(String[] args) throws Exception
    {
        measure("file_stream_rw", SystemBench::file_stream_rw);
        measure("process_spawn", SystemBench::process_spawn);
    }
}
