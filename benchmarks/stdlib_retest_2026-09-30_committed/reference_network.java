import java.io.*;
import java.net.*;
import java.nio.*;
import java.nio.file.*;
import java.util.*;
import java.security.*;
import java.security.cert.*;
import javax.net.ssl.*;

class reference_network
{
    static void run() throws Exception
    {
        long total = 0;
        long start = System.nanoTime();
        for (int i = 0; i < 100; i++)
        {
            total += InetAddress.getAllByName("localhost").length > 0 ? 1 : 0;
        }
        reference_java.report("dns_localhost", start, total);
        byte[] data = Files.readAllBytes(Path.of("message.bin"));
        try (DatagramSocket socket = new DatagramSocket())
        {
            socket.setSoTimeout(5000);
            InetAddress host = InetAddress.getByName("127.0.0.1");
            int port = Integer.parseInt(System.getenv("BENCH_UDP_PORT"));
            total = 0;
            start = System.nanoTime();
            for (int i = 0; i < 500; i++)
            {
                socket.send(new DatagramPacket(data, data.length, host, port));
                byte[] reply = new byte[2048];
                DatagramPacket packet = new DatagramPacket(reply, reply.length);
                socket.receive(packet);
                if (!Arrays.equals(data, Arrays.copyOf(reply, packet.getLength())))
                {
                    throw new IOException("UDP mismatch");
                }
                total += packet.getLength();
            }
            reference_java.report("udp_echo", start, total);
        }
        try (RandomAccessFile pipe = new RandomAccessFile("\\\\.\\pipe\\tx-ipc-" + System.getenv("BENCH_PIPE"), "rw"))
        {
            total = 0;
            start = System.nanoTime();
            for (int i = 0; i < 500; i++)
            {
                ByteBuffer frame = ByteBuffer.allocate(22).order(ByteOrder.LITTLE_ENDIAN);
                frame.put(new byte[]{'T', 'X', 'I', 'P'}).putInt(1).putLong(1).putInt(2).put((byte)0x18).put((byte)42);
                pipe.write(frame.array());
                byte[] reply = new byte[22];
                pipe.readFully(reply);
                if (!Arrays.equals(reply, frame.array()))
                {
                    throw new IOException("IPC mismatch");
                }
                total += reply[21];
            }
            reference_java.report("ipc_echo", start, total);
        }
        KeyStore store = KeyStore.getInstance(KeyStore.getDefaultType());
        store.load(null, null);
        CertificateFactory factory = CertificateFactory.getInstance("X.509");
        try (InputStream input = Files.newInputStream(Path.of("ca.der")))
        {
            store.setCertificateEntry("ca", factory.generateCertificate(input));
        }
        TrustManagerFactory manager = TrustManagerFactory.getInstance(TrustManagerFactory.getDefaultAlgorithm());
        manager.init(store);
        SSLContext context = SSLContext.getInstance("TLSv1.2");
        context.init(null, manager.getTrustManagers(), null);
        int port = Integer.parseInt(System.getenv("BENCH_TLS_PORT"));
        total = 0;
        start = System.nanoTime();
        for (int i = 0; i < 10; i++)
        {
            try (SSLSocket secure = (SSLSocket)context.getSocketFactory().createSocket("localhost", port))
            {
                secure.setSoTimeout(5000);
                SSLParameters parameters = secure.getSSLParameters();
                parameters.setEndpointIdentificationAlgorithm("HTTPS");
                parameters.setApplicationProtocols(new String[]{"bench"});
                secure.setSSLParameters(parameters);
                secure.startHandshake();
                secure.getOutputStream().write(42);
                if (secure.getInputStream().read() != 42)
                {
                    throw new IOException("TLS mismatch");
                }
                total++;
                secure.getSession().invalidate();
            }
        }
        reference_java.report("tls_handshake", start, total);
    }
}
