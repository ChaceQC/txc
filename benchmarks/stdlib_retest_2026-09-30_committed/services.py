"""四语言共用的回环 UDP、TLS 和 TXIP 命名管道回声服务。"""
from contextlib import contextmanager
import ctypes
from ctypes import wintypes
import socket
import ssl
import threading
import uuid


@contextmanager
def servers(fixtures):
    stopping = threading.Event()
    udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    udp.bind(("127.0.0.1", 0))
    udp.settimeout(0.2)
    listener = socket.socket()
    listener.bind(("127.0.0.1", 0))
    listener.listen(32)
    listener.settimeout(0.2)
    context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    context.minimum_version = ssl.TLSVersion.TLSv1_2
    context.maximum_version = ssl.TLSVersion.TLSv1_2
    context.options |= ssl.OP_NO_TICKET
    context.load_cert_chain(fixtures / "server.crt", fixtures / "server.key")
    context.set_alpn_protocols(["bench"])
    pipe_name = "bench_" + uuid.uuid4().hex
    full_name = "\\\\.\\pipe\\tx-ipc-" + pipe_name
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel.CreateNamedPipeW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD, wintypes.DWORD,
        wintypes.DWORD, wintypes.DWORD, wintypes.DWORD, wintypes.DWORD, ctypes.c_void_p]
    kernel.CreateNamedPipeW.restype = wintypes.HANDLE
    kernel.ConnectNamedPipe.argtypes = [wintypes.HANDLE, ctypes.c_void_p]
    kernel.ReadFile.argtypes = [wintypes.HANDLE, ctypes.c_void_p, wintypes.DWORD,
        ctypes.POINTER(wintypes.DWORD), ctypes.c_void_p]
    kernel.WriteFile.argtypes = kernel.ReadFile.argtypes
    kernel.FlushFileBuffers.argtypes = [wintypes.HANDLE]
    kernel.DisconnectNamedPipe.argtypes = [wintypes.HANDLE]
    kernel.CloseHandle.argtypes = [wintypes.HANDLE]
    handle = kernel.CreateNamedPipeW(full_name, 3, 0, 1, 65536, 65536, 5000, None)
    if handle == ctypes.c_void_p(-1).value:
        raise OSError("benchmark pipe creation failed")
    failures = []

    def udp_echo():
        while not stopping.is_set():
            try:
                data, peer = udp.recvfrom(8192)
                udp.sendto(data, peer)
            except socket.timeout:
                pass
            except OSError:
                break

    def tls_echo():
        while not stopping.is_set():
            try:
                peer, _ = listener.accept()
            except socket.timeout:
                continue
            except OSError:
                break
            try:
                peer.settimeout(5)
                peer.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
                with context.wrap_socket(peer, server_side=True) as secure:
                    data = secure.recv(1)
                    if data:
                        secure.sendall(data)
            except (OSError, ssl.SSLError) as failure:
                if not stopping.is_set():
                    failures.append(type(failure).__name__)
            finally:
                peer.close()

    def pipe_echo():
        buffer = ctypes.create_string_buffer(65536)
        while not stopping.is_set():
            connected = kernel.ConnectNamedPipe(handle, None)
            if not connected and ctypes.get_last_error() != 535:
                break
            while not stopping.is_set():
                count = wintypes.DWORD()
                if not kernel.ReadFile(handle, buffer, len(buffer), ctypes.byref(count), None) or not count.value:
                    break
                written = wintypes.DWORD()
                if not kernel.WriteFile(handle, buffer, count.value, ctypes.byref(written), None):
                    break
            kernel.DisconnectNamedPipe(handle)

    threads = [threading.Thread(target=action, daemon=True) for action in (udp_echo, tls_echo, pipe_echo)]
    for thread in threads:
        thread.start()
    try:
        yield {"BENCH_UDP_PORT": str(udp.getsockname()[1]),
               "BENCH_TLS_PORT": str(listener.getsockname()[1]), "BENCH_PIPE": pipe_name}
        if failures:
            raise RuntimeError("TLS echo server failures: " + ", ".join(failures))
    finally:
        stopping.set()
        udp.close()
        listener.close()
        try:
            with open(full_name, "r+b", buffering=0):
                pass
        except OSError:
            pass
        for thread in threads:
            thread.join(timeout=6)
        kernel.CloseHandle(handle)
