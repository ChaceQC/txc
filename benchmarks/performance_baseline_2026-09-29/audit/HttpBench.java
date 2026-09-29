import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;

public class HttpBench
{
    public static void main(String[] args) throws Exception
    {
        HttpClient client = HttpClient.newBuilder().version(
            HttpClient.Version.HTTP_1_1).build();
        HttpRequest request = HttpRequest.newBuilder(
            URI.create("http://127.0.0.1:19790/data")).GET().build();
        long start = System.nanoTime();
        long checksum = 0;
        for (int i = 0; i < 100; ++i)
        {
            HttpResponse<String> response = client.send(request,
                HttpResponse.BodyHandlers.ofString());
            checksum += response.statusCode() == 200 &&
                response.body().equals("OK") ? 1 : 0;
        }
        System.out.println("http_get");
        System.out.println((System.nanoTime() - start) / 1000);
        System.out.println(checksum);
    }
}
