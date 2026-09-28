"""用临时证书验证 11.3 的本机 TLS 双向安全流。"""

from pathlib import Path
import os
import socket
import subprocess
import sys
import tempfile
import time

from check_tls import fixtures


ROOT = Path(__file__).resolve().parents[1]


def environment():
    values = os.environ.copy()
    values["PATH"] = str(ROOT / "tx") + os.pathsep + values["PATH"]
    return values


def compile_case(source, target, env):
    result = subprocess.run(
        [str(ROOT / "tx/txc.exe"), str(source), "-o", str(target)],
        cwd=ROOT, env=env, capture_output=True, timeout=60)
    if result.returncode:
        output = (result.stdout + result.stderr).decode("utf-8", "replace")
        raise AssertionError(f"TLS 定向程序编译失败：{output}")


def free_port():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as selected:
        selected.bind(("127.0.0.1", 0))
        return selected.getsockname()[1]


def run_pair(directory, server_program, client_program, mode, env):
    port = str(free_port())
    server = subprocess.Popen(
        [str(server_program), str(directory), port, mode],
        cwd=ROOT, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    try:
        time.sleep(0.4)
        client = subprocess.run(
            [str(client_program), str(directory), port, mode],
            cwd=ROOT, env=env, capture_output=True, timeout=15)
        server_output, server_error = server.communicate(timeout=15)
    finally:
        if server.poll() is None:
            server.kill()
            server.communicate()
    if client.returncode or server.returncode:
        details = (client.stdout + client.stderr + server_output +
                   server_error).decode("utf-8", "replace")
        raise AssertionError(
            f"TLS {mode} 失败：client={client.returncode}, "
            f"server={server.returncode}\n{details}")


def main():
    env = environment()
    with tempfile.TemporaryDirectory(prefix="tx-tls-stream-") as location:
        directory = Path(location)
        fixtures(directory)
        server_program = directory / "server.exe"
        client_program = directory / "client.exe"
        concurrent_client_program = directory / "random_tls_concurrency.exe"
        compile_case(ROOT / "tests/network/tls_stream_server.tx",
                     server_program, env)
        compile_case(ROOT / "tests/network/tls_stream_client.tx",
                     client_program, env)
        compile_case(ROOT / "tests/crypto/random_tls_concurrency.tx",
                     concurrent_client_program, env)
        for mode in ("good", "bad_host", "bad_trust", "no_client",
                     "alpn_mismatch"):
            run_pair(directory, server_program, client_program, mode, env)
            print(f"TLS_STREAM_{mode.upper()}_OK")
        run_pair(directory, server_program, concurrent_client_program,
                 "good", env)
        print("TLS_RANDOM_CONCURRENCY_OK")


if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    main()
