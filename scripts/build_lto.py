"""以同一 MinGW ABI 构建 ThinLTO 标准库，第三方依赖保留本机对象。"""

from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path
import argparse
import ctypes
import json
import os
import re
import shutil
import subprocess


def run(arguments, *, cwd=None, input_text=None):
    result = subprocess.run([str(argument) for argument in arguments], cwd=cwd,
                            input=input_text, capture_output=True, encoding="utf-8",
                            errors="replace", timeout=600)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    return result.stdout


def split_command(command):
    shell = ctypes.WinDLL("shell32", use_last_error=True)
    shell.CommandLineToArgvW.argtypes = [ctypes.c_wchar_p, ctypes.POINTER(ctypes.c_int)]
    shell.CommandLineToArgvW.restype = ctypes.POINTER(ctypes.c_wchar_p)
    count = ctypes.c_int()
    values = shell.CommandLineToArgvW(command, ctypes.byref(count))
    if not values:
        raise OSError(ctypes.get_last_error(), "无法拆分 CMake 编译命令")
    try:
        return [values[index] for index in range(count.value)]
    finally:
        kernel = ctypes.WinDLL("kernel32")
        kernel.LocalFree.argtypes = [ctypes.c_void_p]
        kernel.LocalFree(values)


def llvm_identity(output):
    version = re.search(r"(?:clang version|LLD|LLVM version)\s+(\d+\.\d+\.\d+)", output)
    commit = re.search(r"\b[0-9a-f]{40}\b", output)
    if not version:
        raise RuntimeError("无法确定 LLVM 工具版本：" + output)
    return version[1], commit[0] if commit else None


def compile_entry(entry, clang, mingw, destination, cache):
    arguments = split_command(entry["command"])
    compiler = next(index for index, argument in enumerate(arguments)
                    if Path(argument).name.lower() in ("g++.exe", "g++", "c++.exe"))
    original = arguments[compiler + 1:]
    flags = []
    index = 0
    while index < len(original):
        argument = original[index]
        if argument in ("-o", "-MF", "-MT", "-MQ"):
            index += 2
            continue
        if argument not in ("-c", "-MD", "-MMD", "-MP") and not argument.endswith((".cpp", ".c")):
            if not argument.startswith("-finline-limit="):
                flags.append(argument)
        index += 1
    source = Path(entry["file"])
    object_path = destination / (source.relative_to(destination.parents[1]).as_posix()
                                 .replace("/", "_") + ".bc")
    command = [clang, "-target", "x86_64-w64-windows-gnu", "--sysroot=" + str(mingw),
               # 随包 GCC 的 thread_local 使用 emutls，Clang 必须采用相同 ABI。
               *flags, "-femulated-tls", "-flto=thin", "-c", source, "-o", object_path]
    if cache:
        command.insert(0, cache)
    run(command, cwd=entry["directory"])
    undefined = run([Path(clang).with_name("llvm-nm.exe"), "--undefined-only", object_path])
    if re.search(r"\b_ZSt(?:15__once_callable|11__once_call)\b", undefined):
        # ld.lld 在 emutls 降低前解析 bitcode 符号，无法匹配 MinGW 的导入名称。
        # 标准库头文件也可能引入 call_once；保留同次 Release 构建的原生对象。
        native_object = Path(entry["directory"]) / original[original.index("-o") + 1]
        object_path = object_path.with_suffix(".o")
        shutil.copy2(native_object, object_path)
        print(f"ThinLTO ABI 原生对象：{source.name}", flush=True)
    return object_path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--llvm-bin", type=Path, required=True)
    parser.add_argument("--mingw", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--dependency", type=Path, action="append", default=[])
    parser.add_argument("--jobs", type=int, default=4)
    options = parser.parse_args()
    build = options.build.resolve()
    llvm = options.llvm_bin.resolve()
    clang = llvm / "clang.exe"
    identity = llvm_identity(run([clang, "--version"]))
    for name in ("ld.lld.exe", "llvm-ar.exe", "llvm-nm.exe"):
        other = llvm_identity(run([llvm / name, "--version"]))
        if other[0] != identity[0] or (other[1] and identity[1] and other[1] != identity[1]):
            raise RuntimeError(f"clang 与 {name} 的版本或构建提交不一致")
    destination = build / "lto"
    destination.mkdir(exist_ok=True)
    commands = json.loads((build / "compile_commands.json").read_text(encoding="utf-8"))
    entries = [entry for entry in commands if "CMakeFiles/txstdlib.dir/" in
               entry["command"].replace("\\", "/")]
    if not entries:
        raise RuntimeError("CMake 编译数据库未包含标准库源文件")
    cache = next(build.glob("_deps/ccache_binary-src/ccache.exe"), None)
    objects = []
    failures = []
    with ThreadPoolExecutor(max_workers=max(1, min(options.jobs, 8))) as executor:
        futures = [executor.submit(compile_entry, entry, clang, options.mingw,
                                   destination, cache) for entry in entries]
        for future in as_completed(futures):
            try:
                objects.append(future.result())
            except Exception as failure:
                failures.append(str(failure))
            if len(objects) % 50 == 0:
                print(f"ThinLTO 标准库：{len(objects)}/{len(entries)}", flush=True)
    if failures:
        raise RuntimeError("\n".join(failures))
    for module in ("httpx_bridge", "websocket_bridge", "requests_bridge"):
        bitcode = destination / (module + ".bc")
        run([clang, "-target", "x86_64-w64-windows-gnu", "-x", "ir", "-O3",
             "-flto=thin", "-c", build / (module + ".ll"), "-o", bitcode])
        objects.append(bitcode)
    staged = destination / "libtxstdlib_lto.a"
    commands = [f'CREATE "{staged.as_posix()}"']
    commands.extend(f'ADDMOD "{path.as_posix()}"' for path in sorted(objects))
    commands.extend(f'ADDLIB "{path.resolve().as_posix()}"' for path in options.dependency)
    run([llvm / "llvm-ar.exe", "-M"], input_text="\n".join(commands + ["SAVE", "END", ""]))
    temporary = options.output.with_suffix(".tmp")
    shutil.copy2(staged, temporary)
    os.replace(temporary, options.output)
    native_count = sum(path.suffix == ".o" for path in objects)
    print(f"ThinLTO 标准库完成：{len(objects) - native_count} 个 bitcode 模块，"
          f"{native_count} 个 ABI 原生对象，LLVM {identity[0]}")


if __name__ == "__main__":
    main()
