"""打包同一批构建的完整工具链、中文说明、语法文档和示例。"""

from pathlib import Path
import hashlib
import json
import os
import re
import zipfile
import argparse
import tarfile
import platform


root = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--tool-dir", type=Path, default=root / "tx")
    options = parser.parse_args()
    tool_dir = options.tool_dir.resolve()
    linux = platform.system() == "Linux"
    target = "linux-x64" if linux else "windows-x64"
    commit = os.environ["TXC_COMMIT"]
    if not re.fullmatch(r"[0-9a-f]{40}", commit):
        raise ValueError("无效的构建提交")
    if os.environ["TXC_REF_TYPE"] == "tag":
        version = os.environ["TXC_REF"]
        if not re.fullmatch(r"v\d+\.\d+\.\d+(?:-[0-9A-Za-z]+(?:[.-][0-9A-Za-z]+)*)?", version):
            raise ValueError("发布 tag 必须使用 v主版本.次版本.修订版本，可附加预发布后缀")
    else:
        version = "dev-" + commit[:12]
    name = f"txc-{version}-{target}"
    binaries = ["txc", "clang", "libtxstdlib.a", "libtxstdlib_lto.a", "package.compat"] if linux else [
        "txc.exe", "clang.exe", "libtxstdlib.a", "libtxstdlib_lto.a", "package.compat"]
    for relative in [*binaries, "link/ld.lld" + ("" if linux else ".exe")]:
        if not (tool_dir / relative).is_file() or (tool_dir / relative).stat().st_size == 0:
            raise FileNotFoundError(f"发布包缺少产物：{relative}")
    files = [(root / "README.md", "README.md"), (root / "example.tx", "example.tx")]
    for directory in ("docs", "examples", "modules"):
        files.extend((path, path.relative_to(root).as_posix())
                     for path in (root / directory).rglob("*") if path.is_file())
    files.extend((tool_dir / item, "tx/" + item) for item in binaries)
    for directory in (("stdlib", "link", "lib", "licenses") if linux else ("stdlib", "link")):
        files.extend((path, "tx/" + path.relative_to(tool_dir).as_posix())
                     for path in (tool_dir / directory).rglob("*") if path.is_file())
    if linux:
        files.append((tool_dir / "toolchain-info.json", "tx/toolchain-info.json"))
    else:
        files.extend((path, "tx/" + path.name) for path in tool_dir.iterdir()
                     if path.is_file() and (path.suffix == ".dll" or "LICENSE" in path.name or "LICENCE" in path.name))
    destination = root / "dist"
    destination.mkdir(exist_ok=True)
    archive = destination / (name + (".tar.gz" if linux else ".zip"))
    metadata = {"version": version, "commit": commit, "platform": target,
                "run_url": f"https://github.com/{os.environ.get('GITHUB_REPOSITORY', 'ChaceQC/txc')}"
                           f"/actions/runs/{os.environ.get('GITHUB_RUN_ID', '')}"}
    if linux:
        metadata.update(json.loads((tool_dir / "toolchain-info.json").read_text(encoding="utf-8")))
    else:
        metadata.update({"gcc": "13.1.0-posix-seh-msvcrt", "llvm": "23.1.2"})
    information = json.dumps(metadata, ensure_ascii=False, indent=2) + "\n"
    if linux:
        import io
        with tarfile.open(archive, "w:gz") as package:
            for path, relative in sorted(files):
                package.add(path, arcname=f"{name}/{relative}", recursive=False)
            entry = tarfile.TarInfo(f"{name}/build-info.json")
            encoded = information.encode("utf-8")
            entry.size = len(encoded)
            package.addfile(entry, io.BytesIO(encoded))
    else:
        with zipfile.ZipFile(archive, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=6) as package:
            for path, relative in sorted(files):
                package.write(path, f"{name}/{relative}")
            package.writestr(f"{name}/build-info.json", information)
    with archive.open("rb") as stream:
        digest = hashlib.file_digest(stream, "sha256").hexdigest()
    (destination / "SHA256SUMS.txt").write_text(f"{digest}  {archive.name}\n", encoding="utf-8")
    print(f"发布包：{archive.name} ({archive.stat().st_size / 1024 / 1024:.1f} MiB)")


if __name__ == "__main__":
    main()
