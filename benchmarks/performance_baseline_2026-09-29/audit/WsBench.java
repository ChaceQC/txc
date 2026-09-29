import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.WebSocket;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.TimeUnit;

public class WsBench
{
    private static class Listener implements WebSocket.Listener
    {
        final CompletableFuture<String> response = new CompletableFuture<>();
        final StringBuilder text = new StringBuilder();

        @Override
        public void onOpen(WebSocket socket)
        {
            socket.request(1);
        }

        @Override
        public java.util.concurrent.CompletionStage<?> onText(
            WebSocket socket, CharSequence data, boolean last)
        {
            text.append(data);
            if (last)
            {
                response.complete(text.toString());
            }
            socket.request(1);
            return null;
        }
    }

    public static void main(String[] args) throws Exception
    {
        HttpClient client = HttpClient.newHttpClient();
        long checksum = 0;
        long start = System.nanoTime();
        for (int i = 0; i < 100; ++i)
        {
            Listener listener = new Listener();
            WebSocket socket = client.newWebSocketBuilder().buildAsync(
                URI.create("ws://127.0.0.1:19791/"), listener).get(5,
                TimeUnit.SECONDS);
            socket.sendText("ok", true).get(5, TimeUnit.SECONDS);
            checksum += listener.response.get(5, TimeUnit.SECONDS).equals("ok") ? 1 : 0;
        }
        System.out.println("websocket_echo");
        System.out.println((System.nanoTime() - start) / 1000);
        System.out.println(checksum);
    }
}
