"""Measure HTTP/3 server handles and memory while aborted request connections are reaped."""

import asyncio
import ctypes
from ctypes import wintypes
import pathlib
import ssl
import subprocess
import sys
import tempfile
import time

from aioquic.asyncio import connect
from aioquic.h3.connection import H3_ALPN
from aioquic.quic.configuration import QuicConfiguration
from aioquic.quic.events import ConnectionTerminated
from aioquic.quic.packet import QuicErrorCode

from check_http3_server import Http3Client, fixtures, request


ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.stdout.reconfigure(encoding="utf-8", line_buffering=True)
ABORT_BATCH = 100
BATCHES = 10
WARMUP_ABORTS = 10
IDLE_SECONDS = 60


class ProcessMemoryCountersEx(ctypes.Structure):
    _fields_ = [
        ("cb", wintypes.DWORD),
        ("page_fault_count", wintypes.DWORD),
        ("peak_working_set_size", ctypes.c_size_t),
        ("working_set_size", ctypes.c_size_t),
        ("quota_peak_paged_pool_usage", ctypes.c_size_t),
        ("quota_paged_pool_usage", ctypes.c_size_t),
        ("quota_peak_nonpaged_pool_usage", ctypes.c_size_t),
        ("quota_nonpaged_pool_usage", ctypes.c_size_t),
        ("pagefile_usage", ctypes.c_size_t),
        ("peak_pagefile_usage", ctypes.c_size_t),
        ("private_usage", ctypes.c_size_t),
    ]


kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
psapi = ctypes.WinDLL("psapi", use_last_error=True)
kernel32.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
kernel32.OpenProcess.restype = wintypes.HANDLE
kernel32.GetProcessHandleCount.argtypes = [
    wintypes.HANDLE, ctypes.POINTER(wintypes.DWORD)]
kernel32.GetProcessHandleCount.restype = wintypes.BOOL
kernel32.CloseHandle.argtypes = [wintypes.HANDLE]
kernel32.CloseHandle.restype = wintypes.BOOL
psapi.GetProcessMemoryInfo.argtypes = [
    wintypes.HANDLE, ctypes.POINTER(ProcessMemoryCountersEx), wintypes.DWORD]
psapi.GetProcessMemoryInfo.restype = wintypes.BOOL


def process_snapshot(process_id):
    query_information = 0x0400
    vm_read = 0x0010
    handle = kernel32.OpenProcess(query_information | vm_read, False,
                                  process_id)
    if not handle:
        raise ctypes.WinError(ctypes.get_last_error())
    try:
        handles = wintypes.DWORD()
        if not kernel32.GetProcessHandleCount(handle, ctypes.byref(handles)):
            raise ctypes.WinError(ctypes.get_last_error())
        counters = ProcessMemoryCountersEx()
        counters.cb = ctypes.sizeof(counters)
        if not psapi.GetProcessMemoryInfo(
                handle, ctypes.byref(counters), counters.cb):
            raise ctypes.WinError(ctypes.get_last_error())
        return {
            "handles": handles.value,
            "private_bytes": counters.private_usage,
            "working_set": counters.working_set_size,
        }
    finally:
        kernel32.CloseHandle(handle)


def print_snapshot(label, snapshot, baseline=None):
    row = (f"{label}: handles={snapshot['handles']} "
           f"private_mib={snapshot['private_bytes'] / 1048576:.2f} "
           f"working_set_mib={snapshot['working_set'] / 1048576:.2f}")
    if baseline is not None:
        row += (f" handle_delta={snapshot['handles'] - baseline['handles']}"
                f" private_delta_mib="
                f"{(snapshot['private_bytes'] - baseline['private_bytes']) / 1048576:.2f}"
                f" working_set_delta_mib="
                f"{(snapshot['working_set'] - baseline['working_set']) / 1048576:.2f}")
    print(row)


class resource_connection_refused(ConnectionError):
    pass


class resource_client(Http3Client):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.terminated = None

    def quic_event_received(self, event):
        if isinstance(event, ConnectionTerminated):
            self.terminated = event
        super().quic_event_received(event)


async def abort_partial_body(port, disconnect_delay=0.01):
    config = QuicConfiguration(is_client=True, alpn_protocols=H3_ALPN)
    config.verify_mode = ssl.CERT_NONE
    client = None

    def create_client(*args, **kwargs):
        nonlocal client
        client = resource_client(*args, **kwargs)
        return client

    try:
        async with connect("127.0.0.1", port, configuration=config,
                           create_protocol=create_client) as protocol:
            protocol.send_partial_body()
            await asyncio.sleep(disconnect_delay)
    except ConnectionError as error:
        if client is not None and client.terminated is not None:
            termination = client.terminated
            detail = (
                "HTTP/3 handshake terminated: "
                f"code={termination.error_code} "
                f"reason={termination.reason_phrase!r}")
            if termination.error_code == QuicErrorCode.CONNECTION_REFUSED:
                raise resource_connection_refused(detail) from error
            raise ConnectionError(detail) from error
        raise


async def abort_many(port, count):
    refusals = 0
    for index in range(count):
        retry_deadline = time.monotonic() + 5.0
        while True:
            try:
                await abort_partial_body(port)
                break
            except resource_connection_refused as error:
                # 16 条连接的服务端上限也包含待回收连接；只重试此明确的容量拒绝。
                refusals += 1
                if time.monotonic() >= retry_deadline:
                    raise AssertionError(
                        "HTTP/3 server kept refusing a resource-test "
                        f"connection at index={index + 1}") from error
                await asyncio.sleep(0.1)
        if (index + 1) % 8 == 0:
            await asyncio.sleep(0.15)
    return refusals


async def measure(directory):
    server = await asyncio.create_subprocess_exec(
        str(ROOT / "tx_build" / "http3_reap_server.exe"), str(directory),
        cwd=ROOT, stdout=asyncio.subprocess.PIPE,
        stderr=asyncio.subprocess.STDOUT,
        creationflags=subprocess.CREATE_NO_WINDOW)
    try:
        ready = await asyncio.wait_for(server.stdout.readline(), timeout=15)
        if ready.decode("utf-8", "replace").strip() != "HTTP3_RESOURCE_READY":
            rest, _ = await asyncio.wait_for(server.communicate(), timeout=5)
            raise AssertionError((ready + rest).decode("utf-8", "replace"))

        refusals = await abort_many(19758, WARMUP_ABORTS)
        await request(19758)
        await asyncio.sleep(0.5)
        baseline = process_snapshot(server.pid)
        print_snapshot("warmup baseline", baseline)
        samples = [baseline]
        total = 0
        for _ in range(BATCHES):
            batch_refusals = await abort_many(19758, ABORT_BATCH)
            refusals += batch_refusals
            total += ABORT_BATCH
            await request(19758)
            if batch_refusals:
                print(f"aborted_connections={total} "
                      f"transient_capacity_refusals={batch_refusals}")
            await asyncio.sleep(0.5)
            current = process_snapshot(server.pid)
            samples.append(current)
            print_snapshot(f"aborted_connections={total}", current, baseline)

        print(f"idle observation: {IDLE_SECONDS}s")
        for elapsed in range(10, IDLE_SECONDS + 1, 10):
            await asyncio.sleep(10)
            if elapsed == IDLE_SECONDS // 2:
                await request(19758)
                await asyncio.sleep(0.5)
            if server.returncode is not None:
                raise AssertionError(
                    f"resource server exited during idle observation: {server.returncode}")
            current = process_snapshot(server.pid)
            samples.append(current)
            print_snapshot(f"idle_seconds={elapsed}", current, baseline)

        await request(19758)
        output, _ = await asyncio.wait_for(server.communicate(), timeout=20)
        if server.returncode != 0:
            raise AssertionError(output.decode("utf-8", "replace"))

        max_handle_delta = max(item["handles"] for item in samples) - baseline["handles"]
        final = samples[-1]
        private_delta = final["private_bytes"] - baseline["private_bytes"]
        print(f"RESULT cycles={BATCHES * ABORT_BATCH} "
              f"max_handle_delta={max_handle_delta} "
              f"final_private_delta_mib={private_delta / 1048576:.2f} "
              f"capacity_refusals={refusals}")
    except Exception:
        if server.returncode is None:
            try:
                await asyncio.wait_for(server.wait(), timeout=0.2)
            except asyncio.TimeoutError:
                pass
        status_at_failure = ("running" if server.returncode is None
                             else f"exited ({server.returncode})")
        if server.returncode is None:
            server.kill()
            await server.wait()
        output = await server.stdout.read()
        print(f"resource server at failure={status_at_failure} "
              f"output={output.decode('utf-8', 'replace')!r}")
        raise
    finally:
        if server.returncode is None:
            server.kill()
            await server.wait()


async def main():
    with tempfile.TemporaryDirectory(prefix="tx-http3-resources-") as temporary:
        directory = pathlib.Path(temporary)
        fixtures(directory)
        source = ROOT / "tests/network/http3_reap_server.tx"
        subprocess.run([ROOT / "tx/txc.exe", source], cwd=ROOT, check=True)
        await measure(directory)


if __name__ == "__main__":
    asyncio.run(main())
