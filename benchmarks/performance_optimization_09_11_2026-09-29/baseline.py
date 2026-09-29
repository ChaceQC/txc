"""保存修改前程序与其配套 DLL；禁止覆盖基线。"""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

root = Path(__file__).resolve().parents[2]
archive = Path(__file__).resolve().parent
output = root / "tx_build/performance_09_11/baseline"
output.mkdir(parents=True, exist_ok=False)
program = output / "paths.exe"
subprocess.run([str(root / "tx/txc.exe"), str(archive / "paths.tx"),
                "-o", str(program)], cwd=root, check=True)
for dll in (root / "tx").glob("*.dll"):
    shutil.copy2(dll, output / dll.name)
paths = [root / "tx/txc.exe", root / "tx/libtxstdlib.a", archive / "paths.tx",
         root / "benchmarks/language_features/workload.tx",
         root / "benchmarks/language_features/workload.txh", *output.iterdir()]
record = {
    "head": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root,
                                    encoding="utf-8").strip(),
    "sha256": {str(p.relative_to(root)): hashlib.sha256(p.read_bytes()).hexdigest()
               for p in paths if p.is_file()},
}
(archive / "baseline.json").write_text(json.dumps(record, ensure_ascii=False, indent=2)
                                       + "\n", encoding="utf-8")
print(program)
