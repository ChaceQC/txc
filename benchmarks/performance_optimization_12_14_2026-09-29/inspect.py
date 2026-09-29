"""冻结候选的机器码和源码摘要，供复核结构变化。"""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
archive = Path(__file__).resolve().parent
output = root / "tx_build/performance_12_14"


def run(*arguments):
    result = subprocess.run([str(value) for value in arguments], cwd=root,
                            capture_output=True, encoding="utf-8", errors="strict", timeout=60)
    assert result.returncode == 0, result.stdout + result.stderr
    return result.stdout


def main():
    calls = {}
    for label, path in (("old", output / "baseline/paths.exe"),
                        ("new", output / "candidate/paths.exe")):
        parts = []
        calls[label] = {}
        for name in ("snapshot", "scan", "arithmetic", "recursive", "copies"):
            symbol = "tx_fn_m0_" + name + "_0"
            assembly = "\n".join(line.rstrip() for line in
                run("objdump", "-d", "--disassemble=" + symbol, path).splitlines()) + "\n"
            assert symbol in assembly
            parts.append(assembly)
            calls[label][name] = re.findall(r"call\s+[^\n]*<([^>]+)>", assembly)
        (archive / (label + "_hotpaths.asm")).write_text("\n".join(parts), encoding="utf-8")
    assert any("txrt_iterator_next_scalar_i64" in call for call in calls["old"]["snapshot"])
    assert not any("txrt_iterator_next" in call for call in calls["new"]["snapshot"])
    assert not any("txrt_value_clone" in call for call in calls["new"]["scan"])
    assert not any("txrt_div_i64" in call for call in calls["new"]["arithmetic"])
    assert not any("txrt_call_bind" in call for call in calls["new"]["arithmetic"])
    assert not any("txrt_sub_i64" in call for call in calls["new"]["recursive"])
    (archive / "machine_calls.json").write_text(json.dumps(calls, ensure_ascii=False, indent=2)
                                                + "\n", encoding="utf-8")
    changed = run("git", "diff", "--name-only").splitlines()
    changed += run("git", "ls-files", "--others", "--exclude-standard").splitlines()
    selected = [root / path for path in changed if path.startswith(("src/", "tests/performance_12_14/"))
                or path in ("CMakeLists.txt", "scripts/check_performance_12_14.py")]
    manifest = {str(path.relative_to(root)): hashlib.sha256(path.read_bytes()).hexdigest()
                for path in sorted(set(selected)) if path.is_file()}
    (archive / "source_manifest.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2)
                                                  + "\n", encoding="utf-8")
    members = run("ar", "t", root / "tx/libtxstdlib.a").splitlines()
    bridges = {}
    for name in ("httpx_bridge.o", "websocket_bridge.o", "requests_bridge.o"):
        assert members.count(name) == 1, (name, members.count(name))
        # Windows ar p 的 stdout 会转换换行；用二进制成员提取校验真实字节。
        with tempfile.TemporaryDirectory(prefix="bridges_", dir=output) as temporary:
            subprocess.run(["ar", "x", str(root / "tx/libtxstdlib.a"), name], cwd=temporary, check=True)
            data = (Path(temporary) / name).read_bytes()
        built = root / "build" / name
        if built.exists():
            assert data == built.read_bytes(), name
        bridges[name] = hashlib.sha256(data).hexdigest()
    (archive / "bridges.json").write_text(json.dumps(bridges, indent=2) + "\n", encoding="utf-8")
    print("PASS optimized machine calls; recorded", len(manifest), "source hashes")


if __name__ == "__main__":
    main()
