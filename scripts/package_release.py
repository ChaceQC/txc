"""打包同一批构建的完整工具链、中文说明、语法文档和示例。"""

from pathlib import Path
import hashlib
import json
import os
import re
import zipfile


root = Path(__file__).resolve().parents[1]


def main():
    commit = os.environ["TXC_COMMIT"]
    if not re.fullmatch(r"[0-9a-f]{40}", commit):
        raise ValueError("无效的构建提交")
    if os.environ["TXC_REF_TYPE"] == "tag":
        version = os.environ["TXC_REF"]
        if not re.fullmatch(r"v\d+\.\d+\.\d+(?:-[0-9A-Za-z]+(?:[.-][0-9A-Za-z]+)*)?", version):
            raise ValueError("发布 tag 必须使用 v主版本.次版本.修订版本，可附加预发布后缀")
    else:
        version = "dev-" + commit[:12]
    name = f"txc-{version}-windows-x64"
    required = ["tx/txc.exe", "tx/clang.exe", "tx/libtxstdlib.a",
                "tx/libtxstdlib_lto.a", "tx/package.compat", "tx/link/ld.exe",
                "tx/link/ld.lld.exe", "docs/usage.md", "docs/syntax.md"]
    for relative in required:
        if not (root / relative).is_file() or (root / relative).stat().st_size == 0:
            raise FileNotFoundError(f"发布包缺少产物：{relative}")
    files = [root / "README.md", root / "example.tx"]
    for directory in ("tx", "docs", "examples", "modules"):
        files.extend(path for path in (root / directory).rglob("*") if path.is_file())
    destination = root / "dist"
    destination.mkdir(exist_ok=True)
    archive = destination / f"{name}.zip"
    with zipfile.ZipFile(archive, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=6) as package:
        for path in sorted(files):
            package.write(path, f"{name}/{path.relative_to(root).as_posix()}")
        metadata = {"version": version, "commit": commit, "platform": "windows-x64",
                    "gcc": "13.1.0-posix-seh-msvcrt", "llvm": "23.1.2",
                    "run_url": f"https://github.com/{os.environ.get('GITHUB_REPOSITORY', 'ChaceQC/txc')}"
                               f"/actions/runs/{os.environ.get('GITHUB_RUN_ID', '')}"}
        package.writestr(f"{name}/build-info.json", json.dumps(metadata, ensure_ascii=False, indent=2) + "\n")
    with archive.open("rb") as stream:
        digest = hashlib.file_digest(stream, "sha256").hexdigest()
    (destination / "SHA256SUMS.txt").write_text(f"{digest}  {archive.name}\n", encoding="utf-8")
    print(f"发布包：{archive.name} ({archive.stat().st_size / 1024 / 1024:.1f} MiB)")


if __name__ == "__main__":
    main()
