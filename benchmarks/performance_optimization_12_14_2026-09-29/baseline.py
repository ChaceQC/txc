"""保存改动前程序、IR 与配套 DLL，拒绝覆盖。"""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

root = Path(__file__).resolve().parents[2]
archive = Path(__file__).resolve().parent
output = root / "tx_build/performance_12_14/baseline"
output.mkdir(parents=True, exist_ok=False)
shutil.copy2(root / "tx/libtxstdlib.a", output / "libtxstdlib.a")
subprocess.run([str(root / "tx/txc.exe"), str(archive / "paths.tx"),
                "-o", str(output / "paths.exe")], cwd=root, check=True)
for dll in (root / "tx").glob("*.dll"):
    shutil.copy2(dll, output / dll.name)
paths = [root / "tx/txc.exe", root / "tx/libtxstdlib.a", archive / "paths.tx",
         *output.iterdir()]
record = {
    "head": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root,
                                    encoding="utf-8").strip(),
    "sha256": {str(p.relative_to(root)): hashlib.sha256(p.read_bytes()).hexdigest()
               for p in paths if p.is_file()},
}
(archive / "baseline.json").write_text(json.dumps(record, ensure_ascii=False, indent=2)
                                       + "\n", encoding="utf-8")
print(output)
