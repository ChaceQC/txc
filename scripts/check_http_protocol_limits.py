"""HTTP/1.1 和 HTTP/2 的非法输入及头/正文限额专项检查。"""

import pathlib
import socket
import subprocess
import sys
import time

import h2.config
import h2.connection
import h2.events


ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.stdout.reconfigure(encoding="utf-8")


def compile_server(name):
    subprocess.run([ROOT / "tx/txc.exe", ROOT / "tests/network" / f"{name}.tx"],
                   cwd=ROOT, check=True)


def run_case(name, port, payload, expected_status=None):
    process = subprocess.Popen([ROOT / "tx_build" / f"{name}.exe"], cwd=ROOT,
                               stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                               creationflags=subprocess.CREATE_NO_WINDOW)
    try:
        time.sleep(0.3)
        with socket.create_connection(("127.0.0.1", port), timeout=4) as peer:
            peer.settimeout(4)
            peer.sendall(payload)
            peer.shutdown(socket.SHUT_WR)
            if expected_status is not None:
                response = peer.recv(4096)
                if not response.startswith(
                        f"HTTP/1.1 {expected_status} ".encode("ascii")):
                    raise AssertionError(response)
            else:
                try:
                    while peer.recv(4096):
                        pass
                except socket.timeout:
                    pass
        output, _ = process.communicate(timeout=8)
        if process.returncode != 0:
            raise AssertionError(f"{name}: {process.returncode} "
                                 f"{output.decode('utf-8', errors='replace')}")
    finally:
        if process.poll() is None:
            process.kill()
            process.wait()


def h2_request(header=None):
    connection = h2.connection.H2Connection(config=h2.config.H2Configuration(
        client_side=True, header_encoding="utf-8"))
    connection.initiate_connection()
    headers = [(":method", "GET"), (":scheme", "http"),
               (":authority", "127.0.0.1:19750"), (":path", "/")]
    if header:
        headers.append(header)
    connection.send_headers(1, headers, end_stream=True)
    return connection.data_to_send()


def h2_oversized_request(peer):
    connection = h2.connection.H2Connection(config=h2.config.H2Configuration(
        client_side=True, header_encoding="utf-8"))
    connection.initiate_connection()
    connection.send_headers(1, [
        (":method", "GET"), (":scheme", "http"),
        (":authority", "127.0.0.1:19751"), (":path", "/"),
        ("content-length", "8388609")], end_stream=True)
    peer.sendall(connection.data_to_send())
    peer.shutdown(socket.SHUT_WR)
    try:
        while peer.recv(4096):
            pass
    except ConnectionResetError:
        pass


def check_http2_terminal_sessions():
    compile_server("http2_terminal_sessions_server")
    process = subprocess.Popen(
        [ROOT / "tx_build/http2_terminal_sessions_server.exe"], cwd=ROOT,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        creationflags=subprocess.CREATE_NO_WINDOW)
    try:
        time.sleep(0.3)
        for _ in range(17):
            with socket.create_connection(("127.0.0.1", 19751), timeout=4) as peer:
                peer.settimeout(4)
                h2_oversized_request(peer)
        connection = h2.connection.H2Connection(config=h2.config.H2Configuration(
            client_side=True, header_encoding="utf-8"))
        with socket.create_connection(("127.0.0.1", 19751), timeout=4) as peer:
            peer.settimeout(4)
            connection.initiate_connection()
            connection.send_headers(1, [
                (":method", "GET"), (":scheme", "http"),
                (":authority", "127.0.0.1:19751"), (":path", "/")],
                end_stream=True)
            peer.sendall(connection.data_to_send())
            status = None
            body = bytearray()
            ended = False
            while not ended:
                data = peer.recv(4096)
                if not data:
                    raise AssertionError("健康 HTTP/2 请求前连接提前关闭")
                for event in connection.receive_data(data):
                    if isinstance(event, h2.events.ResponseReceived):
                        status = dict(event.headers)[":status"]
                    elif isinstance(event, h2.events.DataReceived):
                        body.extend(event.data)
                        connection.acknowledge_received_data(
                            event.flow_controlled_length, event.stream_id)
                    elif isinstance(event, h2.events.StreamEnded):
                        ended = event.stream_id == 1
                pending = connection.data_to_send()
                if pending:
                    peer.sendall(pending)
        output, _ = process.communicate(timeout=8)
        if process.returncode != 0:
            raise AssertionError("终止会话验收服务退出失败：" +
                                 output.decode("utf-8", errors="replace"))
        if status != "200" or bytes(body) != b"healthy":
            raise AssertionError((status, bytes(body)))
        print("HTTP/2 连续淘汰 17 个超限会话后仍可处理正常请求")
    finally:
        if process.poll() is None:
            process.kill()
            process.wait()


def main():
    compile_server("http_invalid_server")
    compile_server("http2_invalid_server")
    run_case("http_invalid_server", 19749,
             b"GET / HTTP/1.1\r\n\r\n", 400)
    prefix = b"GET / HTTP/1.1\r\nHost: local\r\nX-Large: "
    run_case("http_invalid_server", 19749,
             prefix + b"x" * (65536 - len(prefix)), 431)
    run_case("http_invalid_server", 19749,
             b"POST / HTTP/1.1\r\nHost: local\r\nContent-Length: 8388609\r\n"
             b"\r\n", 413)
    run_case("http2_invalid_server", 19750,
             b"INVALID HTTP2 PREFACE DATA")
    run_case("http2_invalid_server", 19750,
             b"PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n" +
             b"\x00\x00\x00\x04\x00\x00\x00\x00\x00" +
             b"\x00\x00\x00\x01\x05\x00\x00\x00\x00")
    run_case("http2_invalid_server", 19750,
             h2_request(("x-large", "x" * 65536)))
    run_case("http2_invalid_server", 19750,
             h2_request(("content-length", "8388609")))
    check_http2_terminal_sessions()
    print("HTTP/1.1 与 HTTP/2 非法输入和限额检查通过")


if __name__ == "__main__":
    main()
