import java.sql.*;
import java.util.*;
import java.util.concurrent.*;
import java.security.*;
import java.nio.charset.StandardCharsets;

class reference_database
{
    static Connection sqlite() throws Exception
    {
        return DriverManager.getConnection("jdbc:sqlite:bench.sqlite");
    }

    static void sql(Connection connection, String text) throws Exception
    {
        try (Statement statement = connection.createStatement())
        {
            statement.execute(text);
        }
    }

    static long scalar(Connection connection, String text) throws Exception
    {
        try (Statement statement = connection.createStatement(); ResultSet rows = statement.executeQuery(text))
        {
            rows.next();
            return rows.getLong(1);
        }
    }

    static long query(boolean transaction) throws Exception
    {
        try (Connection connection = sqlite())
        {
            if (transaction)
            {
                sql(connection, "BEGIN");
            }
            long result = scalar(connection, "SELECT 42");
            if (transaction)
            {
                sql(connection, "COMMIT");
            }
            return result;
        }
    }

    static int history(Connection connection, String expected) throws Exception
    {
        sql(connection, "BEGIN IMMEDIATE");
        sql(connection, "CREATE TABLE IF NOT EXISTS tx_schema_migrations(version BIGINT PRIMARY KEY, checksum TEXT NOT NULL)");
        int count = 0;
        try (Statement statement = connection.createStatement(); ResultSet rows = statement.executeQuery(
            "SELECT version, checksum FROM tx_schema_migrations ORDER BY version"))
        {
            while (rows.next())
            {
                count++;
                if (rows.getInt(1) != count || !rows.getString(2).equals(expected))
                {
                    throw new SQLException("migration mismatch");
                }
            }
        }
        sql(connection, "COMMIT");
        return count;
    }

    static void run(boolean postgres) throws Exception
    {
        Class.forName("org.sqlite.JDBC");
        Connection connection;
        if (postgres)
        {
            Properties properties = new Properties();
            properties.setProperty("user", "tx_test");
            properties.setProperty("password", System.getenv("TX_DB_PASSWORD"));
            properties.setProperty("sslmode", "verify-full");
            properties.setProperty("sslrootcert", System.getenv("TX_DB_CA"));
            connection = DriverManager.getConnection("jdbc:postgresql://localhost:" + System.getenv("TX_DB_PORT") + "/postgres", properties);
        }
        else
        {
            connection = DriverManager.getConnection("jdbc:sqlite::memory:");
        }
        String prefix = postgres ? "postgres" : "sqlite";
        sql(connection, "CREATE TEMP TABLE bench_items(id BIGINT, label TEXT)");
        sql(connection, "BEGIN");
        try (PreparedStatement insert = connection.prepareStatement("INSERT INTO bench_items VALUES(?, 'payload')"))
        {
            long start = System.nanoTime();
            long total = 0;
            for (int i = 1; i <= 2000; i++)
            {
                insert.setLong(1, i);
                total += insert.executeUpdate();
            }
            sql(connection, "COMMIT");
            reference_java.report(prefix + "_insert", start, total);
        }
        try (PreparedStatement query = connection.prepareStatement("SELECT id, label FROM bench_items ORDER BY id"))
        {
            long start = System.nanoTime();
            long total = 0;
            for (int i = 0; i < 10; i++)
            {
                try (ResultSet rows = query.executeQuery())
                {
                    while (rows.next())
                    {
                        total += rows.getLong(1) + rows.getString(2).length();
                    }
                }
            }
            reference_java.report(prefix + "_read", start, total);
        }
        long start = System.nanoTime();
        for (int i = 0; i < 100; i++)
        {
            sql(connection, "BEGIN");
            sql(connection, "SAVEPOINT point");
            sql(connection, "INSERT INTO bench_items VALUES(9999, 'rollback')");
            sql(connection, "ROLLBACK TO point");
            sql(connection, "RELEASE point");
            sql(connection, "COMMIT");
        }
        reference_java.report(prefix + "_savepoint", start, 100);
        if (scalar(connection, "SELECT count(*) FROM bench_items") != 2000)
        {
            throw new SQLException("row count mismatch");
        }
        connection.close();
        if (postgres)
        {
            return;
        }
        start = System.nanoTime();
        long total = 0;
        for (int i = 0; i < 100; i++)
        {
            total += query(false);
        }
        reference_java.report("sqlite_pool", start, total);
        try (ExecutorService executor = Executors.newSingleThreadExecutor())
        {
            start = System.nanoTime();
            total = 0;
            for (int i = 0; i < 100; i++)
            {
                total += executor.submit(() -> query(true)).get();
            }
            reference_java.report("sqlite_async", start, total);
        }
        try (Connection memory = DriverManager.getConnection("jdbc:sqlite::memory:"))
        {
            String text = "CREATE TABLE migration_items(id BIGINT)";
            byte[] framed = ("tx-migration-v1\n" + text.length() + ":" + text).getBytes(StandardCharsets.UTF_8);
            String expected = HexFormat.of().formatHex(MessageDigest.getInstance("SHA-256").digest(framed));
            sql(memory, "CREATE TABLE tx_schema_migrations(version BIGINT PRIMARY KEY, checksum TEXT NOT NULL)");
            sql(memory, "INSERT INTO tx_schema_migrations VALUES(1, '" + expected + "')");
            sql(memory, text);
            start = System.nanoTime();
            total = 0;
            for (int i = 0; i < 100; i++)
            {
                String actual = HexFormat.of().formatHex(MessageDigest.getInstance("SHA-256").digest(framed));
                if (!actual.equals(expected))
                {
                    throw new SQLException("checksum mismatch");
                }
                history(memory, expected);
                total += history(memory, expected);
            }
            reference_java.report("migration_recheck", start, total);
        }
    }
}
