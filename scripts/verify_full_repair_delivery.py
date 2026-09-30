"""对比测量产物与重新封包后产物的 PE 代码/数据段，不执行或重复测量程序。"""
import hashlib
import json
from pathlib import Path
import struct
import subprocess

root = Path(__file__).resolve().parents[1]
archive = root / "benchmarks/performance_full_repair_2026-09-30"
work = root / "tx_build/performance_full_repair"
latest = json.loads((archive / "calibration.json").read_text(encoding="utf-8"))
http_delivery = json.loads((archive / "delivery_http.json").read_text(encoding="utf-8"))


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def sections(path):
    data = path.read_bytes()
    pe = struct.unpack_from("<I", data, 0x3c)[0]
    assert data[pe:pe + 4] == b"PE\0\0"
    count = struct.unpack_from("<H", data, pe + 6)[0]
    optional_size = struct.unpack_from("<H", data, pe + 20)[0]
    start = pe + 24 + optional_size
    result = {}
    for index in range(count):
        offset = start + index * 40
        name = data[offset:offset + 8].rstrip(b"\0").decode("ascii")
        size, address = struct.unpack_from("<II", data, offset + 16)
        # .buildid 和 COFF/PE 文件头为链接身份元数据，不能用于判定机器码改变。
        if name != ".buildid":
            result[name] = hashlib.sha256(data[address:address + size]).hexdigest()
    return result


def main_check():
    delivered = {"compiler_sha256": digest(root / "tx/txc.exe"),
                 "stdlib_sha256": digest(root / "tx/libtxstdlib.a")}
    assert delivered["compiler_sha256"] == latest["manifest"]["compiler_sha256"]
    results = {}
    folder = work / "packaged"
    folder.mkdir(exist_ok=True)
    for name, sample in (("network", latest["network"]), ("diverse", latest["diverse"]), ("http", http_delivery["http"])):
        previous = work / (name + ".exe")
        assert digest(previous) == sample["program_sha256"]["new"]
        current = folder / previous.name
        built = subprocess.run([str(root / "tx/txc.exe"), str(root / sample["source"]), "-o", str(current)],
                               cwd=root, capture_output=True, encoding="utf-8", timeout=180)
        if built.returncode:
            raise RuntimeError(built.stdout + built.stderr)
        before, after = sections(previous), sections(current)
        changed = [key for key in before.keys() | after.keys() if before.get(key) != after.get(key)]
        results[name] = {"measured_sha256": digest(previous), "packaged_sha256": digest(current),
                         "changed_sections": changed, "sections_sha256": after}
        print(name + ": " + ("identical sections" if not changed else ", ".join(changed)), flush=True)
    assert all(not value["changed_sections"] for value in results.values()), "delivery executable sections changed"
    result = {**delivered, "measured_stdlib_sha256": latest["manifest"]["stdlib_sha256"],
              "raw_stdlib_unchanged": delivered["stdlib_sha256"] == latest["manifest"]["stdlib_sha256"],
              "http_measured_stdlib_sha256": http_delivery["manifest"]["stdlib_sha256"],
              "build_directory_removed": not (root / "build").exists(),
              "covered_unique_gaps": 51, "executable_section_comparisons": results}
    (archive / "delivery.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main_check()
