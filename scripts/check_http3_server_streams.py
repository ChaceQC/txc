"""Verify HTTP/3 stream reaping on one long-lived QUIC connection."""

import asyncio
import ctypes
from ctypes import wintypes
import pathlib
import ssl
import subprocess
import sys
import tempfile

from aioquic.asyncio import QuicConnectionProtocol, connect
from aioquic.h3.connection import H3_ALPN, H3Connection
from aioquic.h3.events import DataReceived, HeadersReceived
from aioquic.quic.configuration import QuicConfiguration

from check_http3_server import ROOT, fixtures


sys.stdout.reconfigure(encoding="utf-8", line_buffering=True)
WARMUP_REQUESTS = 16
MEASURED_REQUESTS = 240
SAMPLE_INTERVAL = 16


kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
kernel32.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
kernel32.OpenProcess.restype = wintypes.HANDLE
kernel32.GetProcessHandleCount.argtypes = [
    wintypes.HANDLE, ctypes.POINTER(wintypes.DWORD)]
kernel32.GetProcessHandleCount.restype = wintypes.BOOL
kernel32.CloseHandle.argtypes = [wintypes.HANDLE]
kernel32.CloseHandle.restype = wintypes.BOOL


def process_handle_count(process_id):
    handle = kernel32.OpenProcess(0x0400, False, process_id)
    if not handle:
        raise ctypes.WinError(ctypes.get_last_error())
    try:
        count = wintypes.DWORD()
        if not kernel32.GetProcessHandleCount(handle, ctypes.byref(count)):
            raise ctypes.WinError(ctypes.get_last_error())
        return count.value
    finally:
        kernel32.CloseHandle(handle)


class StreamClient(QuicConnectionProtocol):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.http = H3Connection(self._quic)
        self.pending = {}

    def start_get(self, path, wait_for_response=True):
        stream_id = self._quic.get_next_available_stream_id()
        future = asyncio.get_running_loop().create_future()
        if wait_for_response:
            self.pending[stream_id] = {
                "future": future,
                "status": None,
                "body": bytearray(),
            }
        self.http.send_headers(stream_id, [
            (b":method", b"GET"), (b":scheme", b"https"),
            (b":authority", b"service.example"),
            (b":path", path.encode("ascii"))], end_stream=True)
        self.transmit()
        return stream_id, future

    async def get(self, path):
        _, future = self.start_get(path)
        return await asyncio.wait_for(future, timeout=8)

    def abort_partial_request(self):
        stream_id = self._quic.get_next_available_stream_id()
        self.http.send_headers(stream_id, [
            (b":method", b"POST"), (b":scheme", b"https"),
            (b":authority", b"service.example"), (b":path", b"/peer-abort"),
            (b"content-length", b"16")], end_stream=False)
        self.http.send_data(stream_id, b"partial", end_stream=False)
        self.transmit()
        self._quic.reset_stream(stream_id, error_code=0x10C)
        self.transmit()

    def quic_event_received(self, event):
        for message in self.http.handle_event(event):
            state = self.pending.get(message.stream_id)
            if state is None:
                continue
            if isinstance(message, HeadersReceived):
                state["status"] = dict(message.headers).get(b":status")
                if message.stream_ended:
                    state["future"].set_result(
                        (state["status"], bytes(state["body"])))
            elif isinstance(message, DataReceived):
                state["body"].extend(message.data)
                if message.stream_ended and not state["future"].done():
                    state["future"].set_result(
                        (state["status"], bytes(state["body"])))
                    self.pending.pop(message.stream_id, None)


async def exercise(port, process_id):
    config = QuicConfiguration(is_client=True, alpn_protocols=H3_ALPN)
    config.verify_mode = ssl.CERT_NONE  # 本机独立客户端只检查协议互操作。
    async with connect("127.0.0.1", port, configuration=config,
                       create_protocol=StreamClient) as protocol:
        protocol.abort_partial_request()
        await asyncio.sleep(0.1)

        protocol.start_get("/local-close", wait_for_response=False)
        await asyncio.sleep(0.1)

        for index in range(WARMUP_REQUESTS):
            result = await protocol.get(f"/warmup/{index}")
            if result != (b"200", f"/warmup/{index}".encode("ascii")):
                raise AssertionError((index, result))
        await asyncio.sleep(0.2)
        baseline = process_handle_count(process_id)
        print(f"same-connection warmup={WARMUP_REQUESTS} handles={baseline}")

        samples = []
        for start in range(0, MEASURED_REQUESTS, SAMPLE_INTERVAL):
            for index in range(start, start + SAMPLE_INTERVAL):
                try:
                    result = await protocol.get(f"/measured/{index}")
                except BaseException:
                    print(f"measured request timed out index={index}")
                    raise
                if result != (b"200", f"/measured/{index}".encode("ascii")):
                    raise AssertionError((index, result))
            await asyncio.sleep(0.1)
            count = process_handle_count(process_id)
            samples.append(count)
            print(f"same-connection completed={start + SAMPLE_INTERVAL} "
                  f"handles={count} delta={count - baseline}")

        if max(samples, default=baseline) - baseline > 8:
            raise AssertionError(f"stream handles did not return near baseline: {samples}")
        await asyncio.sleep(0.5)
        final_count = process_handle_count(process_id)
        print(f"same-connection settled handles={final_count} "
              f"delta={final_count - baseline}")
        if final_count == 0 or final_count - baseline > 8:
            raise AssertionError(
                f"stream handle count did not settle while serving: {final_count - baseline}")
        if await protocol.get("/finish") != (b"200", b"/finish"):
            raise AssertionError("final request failed before listener close")


async def main():
    with tempfile.TemporaryDirectory(prefix="tx-http3-streams-") as temporary:
        directory = pathlib.Path(temporary)
        fixtures(directory)
        source = ROOT / "tests/network/http3_stream_reap_server.tx"
        subprocess.run([ROOT / "tx/txc.exe", source], cwd=ROOT, check=True)

        server = subprocess.Popen(
            [ROOT / "tx_build/http3_stream_reap_server.exe", directory],
            cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            creationflags=subprocess.CREATE_NO_WINDOW)
        try:
            ready = server.stdout.readline().decode("utf-8", "replace").strip()
            if ready != "HTTP3_STREAM_REAP_READY":
                output, _ = server.communicate(timeout=5)
                raise AssertionError((ready + output.decode("utf-8", "replace")))
            try:
                await exercise(19759, server.pid)
            except BaseException:
                if server.poll() is None:
                    server.kill()
                    server.wait()
                output, _ = server.communicate()
                print(output.decode("utf-8", "replace"), file=sys.stderr)
                raise
            output, _ = server.communicate(timeout=15)
            if server.returncode != 0:
                raise AssertionError(
                    f"HTTP/3 stream server exit={server.returncode}: "
                    + output.decode("utf-8", "replace"))
        finally:
            if server.poll() is None:
                server.kill()
                server.wait()
        print("HTTP/3 同连接正常完成、对端中止和本端关闭流回收通过")


if __name__ == "__main__":
    asyncio.run(main())
