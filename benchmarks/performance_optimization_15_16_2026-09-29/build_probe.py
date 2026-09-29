"""单次拆分记录普通构建成本，检查当前工具链的 LLVM LTO 可用性。"""
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import time

from measure import archive, digest, out, root, run, save, sources


def split_build(package, label):
    destination = out / "build_probe" / label
    destination.mkdir(parents=True, exist_ok=True)
    ir = destination / "paths.ll"
    object_path = destination / "paths.o"
    executable = destination / "paths.exe"
    costs = {}
    _, costs["frontend_ms"] = run(package / "txc.exe", "emit-llvm", sources["paths"], "-o", ir)
    _, costs["llvm_codegen_ms"] = run(package / "clang.exe", "-target", "x86_64-w64-windows-gnu",
                                      "-x", "ir", "-c", "-O3", "-o", object_path, ir)
    link = package / "link"
    args = [link / "ld.exe", "-m", "i386pep", "-Bdynamic", "-o", executable,
            link / "crt2.o", link / "crtbegin.o", "-L" + str(link), object_path, package / "libtxstdlib.a"]
    args += ["-l" + name for name in (
        "stdc++", "mingw32", "gcc_s", "gcc", "moldname", "mingwex", "msvcrt", "kernel32",
        "pthread", "advapi32", "bcrypt", "crypt32", "ncrypt", "winhttp", "ws2_32", "dnsapi",
        "shell32", "user32", "kernel32", "iconv", "mingw32", "gcc_s", "gcc", "moldname",
        "mingwex", "msvcrt", "kernel32")]
    args += [link / "default-manifest.o", link / "crtend.o"]
    _, costs["link_ms"] = run(*args)
    for dll in package.glob("*.dll"):
        shutil.copy2(dll, destination / dll.name)
    costs["executable_bytes"] = executable.stat().st_size
    costs["executable_sha256"] = digest(executable)
    costs["library_bytes"] = (package / "libtxstdlib.a").stat().st_size
    costs["compiler_bytes"] = (package / "txc.exe").stat().st_size
    costs["ir_sha256"] = digest(ir)
    return costs


def lto_probe():
    # 复用已经用于等价对照的 MinGW 头文件和链接路径；不引入新的运行库。
    spec = importlib.util.spec_from_file_location("equivalence", root / "scripts/run_performance_equivalence.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    flags = module.clang_flags()
    destination = out / "build_probe/lto"
    destination.mkdir(parents=True, exist_ok=True)
    bitcode = destination / "core.bc"
    _, compile_ms = run(*flags, "-Isrc", "-emit-llvm", "-c", archive / "core.cpp", "-o", bitcode)
    command = [*flags, "-flto", "-fuse-ld=lld", str(bitcode), str(root / "tx/libtxstdlib.a"),
               "-Ltx/link", "-lstdc++", "-lwinhttp", "-lws2_32", "-ladvapi32", "-lbcrypt",
               "-lshell32", "-luser32", "-liconv", "-o", str(destination / "probe.exe")]
    started = time.perf_counter()
    result = subprocess.run(command, cwd=root, capture_output=True, encoding="utf-8", errors="strict", timeout=60)
    return {"command": command, "compile_bitcode_ms": compile_ms,
            "link_ms": (time.perf_counter() - started) * 1000, "returncode": result.returncode,
            "diagnostic": result.stdout + result.stderr,
            "scope": "Native C++ probe only; GCC archives remain native objects; no package migration."}


def main():
    report = {"cost_samples": 1,
              "old": split_build(out / "baseline/tx", "old"),
              "new": split_build(root / "tx", "new"),
              "lto_availability": lto_probe()}
    save("build_costs.json", report)
    print(json.dumps(report, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
