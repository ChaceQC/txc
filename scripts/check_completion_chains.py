"""13.5 跨模块业务链和 13.1 原生子进程边界。"""

from pathlib import Path
import http.server
import json
import os
import subprocess
import tempfile
import threading
import sys

from check_tls_stream import compile_case, environment, run_pair
from check_tls import fixtures

ROOT = Path(__file__).resolve().parent.parent


def run(command, cwd, env):
    result = subprocess.run([str(item) for item in command], cwd=cwd, env=env,
                            capture_output=True, text=True, encoding="utf-8",
                            errors="replace", timeout=60)
    assert result.returncode == 0, (result.returncode, result.stdout, result.stderr)
    return result.stdout


class Handler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        body = json.dumps({"message": "本机并发请求"}, ensure_ascii=False).encode("utf-8")
        self.send_response(200)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, *_args):
        pass


def main():
    env = environment()
    with tempfile.TemporaryDirectory(prefix="tx-chains-") as location:
        directory = Path(location)
        if "native" in sys.argv[1:]:
            native = directory / "child_process.exe"
            run(["g++", "-std=c++23", "-finput-charset=UTF-8", "-fexec-charset=UTF-8", "-I", ROOT / "src",
                 ROOT / "tests/diagnostics/child_process.cpp", ROOT / "src/driver/child_process.cpp",
                 "-o", native], directory, env)
            print(run([native], directory, env).strip())
            return
        for name in ("file_data_db", "network_task_db_log", "password_tls_storage"):
            compile_case(ROOT / "tests/completion" / f"{name}.tx", directory / f"{name}.exe", env)
        run([directory / "file_data_db.exe"], directory, env)
        print("通过：文件 → JSON/CSV/XML → SQLite 事务")

        server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        worker = threading.Thread(target=server.serve_forever, daemon=True)
        worker.start()
        try:
            log = directory / "events.jsonl"
            run([directory / "network_task_db_log.exe", f"http://127.0.0.1:{server.server_port}/", log], directory, env)
            events = [json.loads(line) for line in log.read_text(encoding="utf-8").splitlines()]
            assert len(events) == 2 and {item["fields"]["id"] for item in events} == {1, 2}
            assert all(item["context"]["request_id"] == "pipeline-request" and
                       item["context"]["task_id"].startswith("task-") and
                       item["fields"]["authorization"] == "[REDACTED]" for item in events), events
            assert len({item["context"]["task_id"] for item in events}) == 2
        finally:
            server.shutdown()
            server.server_close()
            worker.join()
        print("通过：HTTP → 并发任务 → 数据库 → 遮蔽日志及独立任务上下文")

        fixtures(directory)
        tls_server = directory / "tls_server.exe"
        compile_case(ROOT / "tests/network/tls_stream_server.tx", tls_server, env)
        run_pair(directory, tls_server, directory / "password_tls_storage.exe", "good", env)
        print("通过：Argon2id → PKCS#12/证书 → 双向 TLS → 认证加密存储")

        native = directory / "child_process.exe"
        run(["g++", "-std=c++23", "-finput-charset=UTF-8", "-fexec-charset=UTF-8", "-I", ROOT / "src",
             ROOT / "tests/diagnostics/child_process.cpp", ROOT / "src/driver/child_process.cpp",
             "-o", native], directory, env)
        print(run([native], directory, env).strip())


if __name__ == "__main__":
    main()
