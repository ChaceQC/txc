"""同一 QUIC 连接上两条 HTTP/3 流由两个 TX 工作线程并行处理。"""

import asyncio
import pathlib
import ssl
import subprocess
import sys
import tempfile
import time

from aioquic.asyncio import QuicConnectionProtocol, connect
from aioquic.h3.connection import H3_ALPN, H3Connection
from aioquic.h3.events import DataReceived, HeadersReceived
from aioquic.quic.configuration import QuicConfiguration

from check_http3_server import fixtures


ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.stdout.reconfigure(encoding="utf-8")


class ParallelClient(QuicConnectionProtocol):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.http = H3Connection(self._quic)
        self.responses = {}

    async def get_two(self):
        loop = asyncio.get_running_loop()
        started = time.perf_counter()
        for name in ("first", "second"):
            stream_id = self._quic.get_next_available_stream_id()
            self.responses[stream_id] = {
                "expected": name.encode(), "status": None,
                "body": bytearray(), "done": loop.create_future()}
            self.http.send_headers(stream_id, [
                (b":method", b"GET"), (b":scheme", b"https"),
                (b":authority", b"127.0.0.1:19754"),
                (b":path", f"/items/{name}".encode())], end_stream=True)
        self.transmit()
        await asyncio.wait_for(asyncio.gather(*(
            item["done"] for item in self.responses.values())), 8)
        for item in self.responses.values():
            if item["status"] != b"200" or bytes(item["body"]) != item["expected"]:
                raise AssertionError(item)
        return time.perf_counter() - started

    def quic_event_received(self, event):
        for message in self.http.handle_event(event):
            if isinstance(message, HeadersReceived):
                self.responses[message.stream_id]["status"] = dict(
                    message.headers).get(b":status")
            elif isinstance(message, DataReceived):
                item = self.responses[message.stream_id]
                item["body"].extend(message.data)
                if message.stream_ended and not item["done"].done():
                    item["done"].set_result(None)


async def run_client():
    config = QuicConfiguration(is_client=True, alpn_protocols=H3_ALPN)
    config.verify_mode = ssl.CERT_NONE
    async with connect("127.0.0.1", 19754, configuration=config,
                       create_protocol=ParallelClient) as protocol:
        return await protocol.get_two()


def main():
    with tempfile.TemporaryDirectory(prefix="tx-http3-parallel-") as temporary:
        directory = pathlib.Path(temporary)
        fixtures(directory)
        source = ROOT / "tests/network/http3_parallel_server.tx"
        subprocess.run([ROOT / "tx/txc.exe", source], cwd=ROOT, check=True)
        server = subprocess.Popen(
            [ROOT / "tx_build/http3_parallel_server.exe", directory],
            cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            creationflags=subprocess.CREATE_NO_WINDOW)
        try:
            time.sleep(0.5)
            if server.poll() is not None:
                output, _ = server.communicate()
                raise AssertionError(output.decode("utf-8", "replace"))
            elapsed = asyncio.run(run_client())
            output, _ = server.communicate(timeout=12)
            if server.returncode != 0 or elapsed >= 1.0:
                raise AssertionError(f"server={server.returncode}, "
                    f"elapsed={elapsed:.3f}s, " + output.decode("utf-8", "replace"))
            print(f"HTTP/3 同连接双流并行通过：{elapsed:.3f}s")
        finally:
            if server.poll() is None:
                server.kill()
                server.wait()


if __name__ == "__main__":
    main()
