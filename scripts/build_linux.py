"""构建 Linux x86_64 原生工具包和 ThinLTO 库，成功验证后清理临时目录。"""

from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import argparse
import json
import os
import platform
import shlex
import shutil
import subprocess

from linux_bundle import bundle_tools, merge_archive, run, write_manifest


root = Path(__file__).resolve().parents[1]


def compile_lto(entry, destination, clang, cache):
    original = shlex.split(entry["command"])
    flags = []
    index = 1
    while index < len(original):
        item = original[index]
        if item in ("-o", "-MF", "-MT", "-MQ"):
            index += 2
            continue
        if item not in ("-c", "-MD", "-MMD", "-MP") and not item.endswith((".c", ".cpp")):
            flags.append(item)
        index += 1
    source = Path(entry["file"])
    output = destination / (source.relative_to(root).as_posix().replace("/", "_") + ".bc")
    command = [clang, *flags, "-flto=thin", "-c", source, "-o", output]
    if cache:
        command.insert(0, cache)
    run(*command)
    return output


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--incremental", action="store_true")
    parser.add_argument("--output", type=Path, default=root / "tx")
    parser.add_argument("--jobs", type=int, default=min(os.cpu_count() or 2, 8))
    options = parser.parse_args()
    if platform.system() != "Linux" or platform.machine() != "x86_64":
        raise SystemExit("此脚本需要 Linux x86_64")
    build = root / "build/linux"
    tool_dir = options.output.resolve()
    if build.resolve() in tool_dir.parents or tool_dir == build.resolve():
        raise SystemExit("工具包输出不能位于待清理的 build/linux 内；可使用 --output tx/linux")
    jobs = max(1, min(options.jobs, 8))
    llvm = Path(os.environ.get("TX_LLVM_BIN", "/usr/lib/llvm-18/bin"))
    clang = llvm / "clang++"
    tool_dir.mkdir(parents=True, exist_ok=True)
    configure = ["cmake", "-S", str(root), "-B", str(build), "-G", "Ninja",
        "-DCMAKE_BUILD_TYPE=Release", "-DCMAKE_C_COMPILER=" + str(llvm / "clang"),
        "-DCMAKE_CXX_COMPILER=" + str(clang), "-DTX_TOOL_OUTPUT_DIR=" + str(tool_dir)]
    cache_path = build / "CMakeCache.txt"
    cache = cache_path.read_text(encoding="utf-8") if cache_path.exists() else ""
    if f"TX_TOOL_OUTPUT_DIR:PATH={tool_dir}" not in cache or str(clang) not in cache:
        subprocess.run(configure, check=True)
    # 更换编译器时 CMake 可能清空缓存并丢失同次传入的输出路径。
    if f"TX_TOOL_OUTPUT_DIR:PATH={tool_dir}" not in (build / "CMakeCache.txt").read_text(encoding="utf-8"):
        subprocess.run(configure, check=True)
    subprocess.run(["cmake", "--build", str(build), "--parallel", str(jobs)], check=True)
    interfaces = tool_dir / "stdlib"
    if interfaces.resolve() != (root / "tx/stdlib").resolve():
        shutil.copytree(root / "tx/stdlib", interfaces, dirs_exist_ok=True)
    bundle_tools(tool_dir, llvm / "clang", llvm / "ld.lld")
    dependencies = [build / "libtx_sqlite_static.a",
        build / "_deps/nghttp2-build/lib/libnghttp2.a",
        build / "_deps/nghttp3-build/external/lib/libnghttp3.a"]
    dependencies += sorted((build / "_deps/mbedtls-build").rglob("*.a"))
    bridge_objects = []
    bridge_bitcode = []
    lto_dir = build / "lto"
    lto_dir.mkdir(exist_ok=True)
    for module in ("httpx_bridge", "websocket_bridge", "requests_bridge"):
        ir = build / (module + ".ll")
        run(tool_dir / "txc", "emit-library-llvm", root / f"src/stdlib/{module}.tx", "-o", ir)
        for lto in (False, True):
            output = build / (module + (".bc" if lto else ".o"))
            run(llvm / "clang", "-target", "x86_64-unknown-linux-gnu", "-x", "ir", "-O3",
                *(["-flto=thin"] if lto else []), "-c", ir, "-o", output)
            (bridge_bitcode if lto else bridge_objects).append(output)
    merge_archive(tool_dir / "libtxstdlib.a", [build / "libtxstdlib.a", *dependencies],
                  bridge_objects, llvm / "llvm-ar")
    entries = json.loads((build / "compile_commands.json").read_text(encoding="utf-8"))
    entries = [entry for entry in entries if "CMakeFiles/txstdlib.dir/" in entry["command"]]
    if not entries:
        raise RuntimeError("编译数据库缺少标准库源文件")
    with ThreadPoolExecutor(max_workers=jobs) as executor:
        objects = list(executor.map(lambda entry: compile_lto(entry, lto_dir, clang,
            shutil.which("ccache")), entries))
    merge_archive(tool_dir / "libtxstdlib_lto.a", dependencies, [*objects, *bridge_bitcode], llvm / "llvm-ar")
    for source, name in ((build / "_deps/mbedtls-src/LICENSE", "MBEDTLS-LICENSE"),
                         (build / "_deps/nghttp2-src/COPYING", "NGHTTP2-LICENSE"),
                         (build / "_deps/nghttp3-src/COPYING", "NGHTTP3-LICENSE"),
                         (build / "_deps/msquic_headers-src/LICENSE", "MSQUIC-LICENSE"),
                         (root / "third_party/sqlite/LICENSE", "SQLITE-LICENSE")):
        shutil.copy2(source, tool_dir / "licenses" / name)
    write_manifest(tool_dir, (build / "compatibility_abi.txt").read_text(encoding="utf-8").strip())
    info = {"platform": "linux-x64", "minimum_system": "Ubuntu 24.04 / glibc 2.39",
            "llvm": run(llvm / "clang", "--version").splitlines()[0]}
    (tool_dir / "toolchain-info.json").write_text(json.dumps(info, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    run(tool_dir / "txc", "check", root / "example.tx")
    if not options.incremental:
        resolved = build.resolve()
        if resolved != root / "build/linux" or build.is_symlink():
            raise RuntimeError("拒绝清理预期目录以外的路径")
        shutil.rmtree(resolved)
    print(f"Linux 工具包已生成：{tool_dir}", flush=True)


if __name__ == "__main__":
    main()
