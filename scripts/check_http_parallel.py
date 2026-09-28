"""验证 HTTP/1.1 并发连接与同一 HTTP/2 会话上的两条并行流。"""

import concurrent.futures
import pathlib
import socket
import subprocess
import sys
import time
import urllib.request

import h2.config
import h2.connection
import h2.events


ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.stdout.reconfigure(encoding="utf-8")


def run_server(name, request):
    source = ROOT / "tests/network" / f"{name}.tx"
    subprocess.run([ROOT / "tx/txc.exe", source], cwd=ROOT, check=True)
    process = subprocess.Popen([ROOT / "tx_build" / f"{name}.exe"], cwd=ROOT,
                               stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                               creationflags=subprocess.CREATE_NO_WINDOW)
    try:
        time.sleep(0.4)
        elapsed = request()
        output, _ = process.communicate(timeout=12)
        if process.returncode != 0:
            raise AssertionError(f"{name} 退出码 {process.returncode}: "
                                 f"{output.decode('utf-8', errors='replace')}")
        if elapsed >= 1.0:
            raise AssertionError(f"{name} 两个处理函数未并行完成：{elapsed:.3f}s")
        print(f"{name}: 两个请求完成于 {elapsed:.3f}s")
    finally:
        if process.poll() is None:
            process.kill()
            process.wait()


def http1_requests():
    def fetch(item):
        with urllib.request.urlopen(
                f"http://127.0.0.1:19747/items/{item}", timeout=5) as response:
            return response.status, response.read()

    started = time.perf_counter()
    with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
        values = list(pool.map(fetch, ("first", "second")))
    if values != [(200, b"first"), (200, b"second")]:
        raise AssertionError(values)
    return time.perf_counter() - started


def http2_streams():
    connection = h2.connection.H2Connection(config=h2.config.H2Configuration(
        client_side=True, header_encoding="utf-8"))
    with socket.create_connection(("127.0.0.1", 19748), timeout=5) as peer:
        peer.settimeout(5)
        connection.initiate_connection()
        peer.sendall(connection.data_to_send())
        expected = {1: b"first", 3: b"second"}
        bodies = {1: bytearray(), 3: bytearray()}
        statuses = {}
        for stream_id, item in ((1, "first"), (3, "second")):
            connection.send_headers(stream_id, [
                (":method", "GET"), (":scheme", "http"),
                (":authority", "127.0.0.1:19748"),
                (":path", f"/items/{item}")], end_stream=True)
        started = time.perf_counter()
        peer.sendall(connection.data_to_send())
        ended = set()
        while ended != set(expected):
            data = peer.recv(65536)
            if not data:
                raise AssertionError("HTTP/2 会话提前关闭")
            for event in connection.receive_data(data):
                if isinstance(event, h2.events.ResponseReceived):
                    statuses[event.stream_id] = dict(event.headers)[":status"]
                elif isinstance(event, h2.events.DataReceived):
                    bodies[event.stream_id].extend(event.data)
                    connection.acknowledge_received_data(
                        event.flow_controlled_length, event.stream_id)
                elif isinstance(event, h2.events.StreamEnded):
                    ended.add(event.stream_id)
            pending = connection.data_to_send()
            if pending:
                peer.sendall(pending)
        if any(statuses.get(key) != "200" or bytes(bodies[key]) != value
               for key, value in expected.items()):
            raise AssertionError((statuses, bodies))
        return time.perf_counter() - started


def main():
    run_server("http_routes_parallel", http1_requests)
    run_server("http2_parallel_server", http2_streams)


if __name__ == "__main__":
    main()
