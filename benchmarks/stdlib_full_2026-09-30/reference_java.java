import java.io.*;
import java.nio.*;
import java.nio.channels.*;
import java.nio.file.*;
import java.util.*;
import java.util.concurrent.*;
import java.util.concurrent.atomic.*;
import java.util.concurrent.locks.*;
import java.util.function.*;

public class reference_java
{
    static void report(String name, long start, long total)
    {
        System.out.println(name);
        System.out.println((System.nanoTime() - start) / 1000.0);
        System.out.println(total);
    }

    static void concurrency() throws Exception
    {
        long[] total = {0};
        long start = System.nanoTime();
        for (int i = 0; i < 100; i++)
        {
            Thread thread = new Thread(() ->
            {
                total[0]++;
            });
            thread.start();
            thread.join();
        }
        report("thread_spawn_join", start, total[0]);
        ReentrantLock mutex = new ReentrantLock();
        total[0] = 0;
        start = System.nanoTime();
        for (int i = 0; i < 100000; i++)
        {
            mutex.lock();
            total[0]++;
            mutex.unlock();
        }
        report("mutex_uncontended", start, total[0]);
        AtomicLong atomic = new AtomicLong();
        start = System.nanoTime();
        for (int i = 0; i < 100000; i++)
        {
            atomic.getAndIncrement();
        }
        report("atomic_add", start, atomic.get());
        ArrayBlockingQueue<Long> queue = new ArrayBlockingQueue<>(1);
        total[0] = 0;
        start = System.nanoTime();
        for (int i = 1; i <= 20000; i++)
        {
            if (!queue.offer((long)i, 1, TimeUnit.SECONDS))
            {
                throw new IOException("queue full");
            }
            total[0] += queue.poll(1, TimeUnit.SECONDS);
        }
        report("channel_send_recv", start, total[0]);
        start = System.nanoTime();
        total[0] = 0;
        try (ExecutorService executor = Executors.newSingleThreadExecutor())
        {
            for (int i = 0; i < 500; i++)
            {
                total[0] += executor.submit(() -> 1).get();
            }
        }
        report("task_spawn_wait", start, total[0]);
    }

    static void async_file() throws Exception
    {
        byte[] payload = Files.readAllBytes(Path.of("block.bin"));
        long total = 0;
        long start = System.nanoTime();
        for (int i = 0; i < 32; i++)
        {
            try (AsynchronousFileChannel file = AsynchronousFileChannel.open(Path.of("async.bin"),
                StandardOpenOption.CREATE, StandardOpenOption.WRITE))
            {
                ByteBuffer buffer = ByteBuffer.wrap(payload);
                long offset = (long)i * 65536;
                while (buffer.hasRemaining())
                {
                    int count = file.write(buffer, offset).get();
                    offset += count;
                    total += count;
                }
            }
            try (AsynchronousFileChannel file = AsynchronousFileChannel.open(Path.of("async.bin"), StandardOpenOption.READ))
            {
                ByteBuffer buffer = ByteBuffer.allocate(65536);
                long offset = (long)i * 65536;
                while (buffer.hasRemaining())
                {
                    int count = file.read(buffer, offset).get();
                    if (count < 0)
                    {
                        throw new EOFException();
                    }
                    offset += count;
                }
                if (!Arrays.equals(buffer.array(), payload))
                {
                    throw new IOException("file mismatch");
                }
                total += buffer.position();
            }
        }
        report("async_file_rw", start, total);
    }

    static void diagnostics() throws Exception
    {
        long[] total = {0};
        IntConsumer parameter = index ->
        {
            total[0] += index;
        };
        long start = System.nanoTime();
        for (int i = 0; i < 20000; i++)
        {
            parameter.accept(i);
        }
        report("test_parameterized", start, total[0]);
        total[0] = 0;
        IntBinaryOperator generate = (seed, index) -> seed + index;
        IntPredicate predicate = value ->
        {
            total[0]++;
            return value >= 1000;
        };
        start = System.nanoTime();
        for (int i = 0; i < 20000; i++)
        {
            if (!predicate.test(generate.applyAsInt(1000, i)))
            {
                throw new IOException("property failed");
            }
        }
        report("test_property", start, total[0]);
        java.util.logging.Logger logger = java.util.logging.Logger.getLogger("benchmark");
        logger.setLevel(java.util.logging.Level.INFO);
        start = System.nanoTime();
        for (int i = 0; i < 100000; i++)
        {
            logger.fine(() -> "filtered " + Map.of("index", 42));
        }
        report("log_filtered", start, 100000);
        Object lock = new Object();
        try (BufferedWriter writer = Files.newBufferedWriter(Path.of("events.jsonl")))
        {
            start = System.nanoTime();
            for (int i = 1; i <= 2000; i++)
            {
                String record = "{\"timestamp_ms\":" + System.currentTimeMillis()
                    + ",\"context\":{\"task_id\":\"\",\"thread_id\":\"" + Thread.currentThread().threadId()
                    + "\",\"request_id\":\"bench\"},\"level\":\"info\",\"message\":\"entry\",\"fields\":{\"index\":"
                    + i + ",\"password\":\"[REDACTED]\"}}\n";
                synchronized (lock)
                {
                    writer.write(record);
                    writer.flush();
                }
            }
            writer.flush();
            report("log_file", start, 2000);
        }
    }

    static void profile()
    {
        record span(int id, String name, long duration) {}
        List<span> spans = new ArrayList<>();
        long start = System.nanoTime();
        for (int i = 0; i < 1000; i++)
        {
            long begin = System.nanoTime();
            spans.add(new span(i, "bench", System.nanoTime() - begin));
        }
        report("profile_spans", start, spans.size());
    }

    public static void main(String[] args) throws Exception
    {
        switch (args[0])
        {
            case "concurrency" -> concurrency();
            case "async_file" -> async_file();
            case "diagnostics" -> diagnostics();
            case "profile" -> profile();
            case "sqlite", "postgres" -> reference_database.run(args[0].equals("postgres"));
            case "security" -> reference_security.run();
            case "network" -> reference_network.run();
            default -> throw new IllegalArgumentException("group");
        }
    }
}
