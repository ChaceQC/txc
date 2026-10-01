"""归档 Linux 原生依赖、链接组件和许可证，保留宿主 glibc 基线。"""

from pathlib import Path
import hashlib
import os
import re
import shutil
import subprocess


def run(*arguments, input_text=None):
    result = subprocess.run([str(value) for value in arguments], input=input_text,
                            capture_output=True, encoding="utf-8", errors="replace", timeout=600)
    if result.returncode:
        raise RuntimeError(" ".join(map(str, arguments)) + "\n" + result.stdout + result.stderr)
    return result.stdout


def digest(path):
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def merge_archive(output, archives, objects, archiver):
    commands = [f'CREATE "{output}"']
    commands += [f'ADDLIB "{path}"' for path in archives]
    commands += [f'ADDMOD "{path}"' for path in objects]
    run(archiver, "-M", input_text="\n".join(commands + ["SAVE", "END", ""]))


def library_path(name):
    path = Path(run("g++", "-print-file-name=" + name).strip())
    if path.is_file():
        return path
    for line in run("ldconfig", "-p").splitlines():
        fields = line.split()
        if fields and fields[0] == name and "x86-64" in line:
            return Path(fields[-1])
    raise FileNotFoundError("缺少 Linux 依赖：" + name)


system_library = re.compile(r"^(?:ld-linux.*|lib(?:c|m|pthread|dl|rt|resolv|util|anl)\.so(?:\..*)?)$")


def elf_dependencies(path):
    environment = os.environ.copy()
    environment.pop("LD_LIBRARY_PATH", None)
    environment.pop("LD_PRELOAD", None)
    environment["LC_ALL"] = "C"
    result = subprocess.run(["ldd", str(path)], env=environment, capture_output=True,
                            encoding="utf-8", errors="replace", timeout=60, check=True)
    if "=> not found" in result.stdout:
        raise RuntimeError("共享库依赖未解析：" + result.stdout)
    # 安装路径可能带空格，文件名后的地址字段才是路径边界。
    return [(name, Path(location)) for name, location in re.findall(
        r"^\s*(\S+)\s+=>\s+(/.*?)\s+\(0x[0-9a-f]+\)\s*$", result.stdout, re.MULTILINE)]


def require_bundled_dependencies(executable, library_directory):
    root = library_directory.resolve()
    for name, path in elf_dependencies(executable):
        if not system_library.fullmatch(name) and not path.resolve().is_relative_to(root):
            raise RuntimeError(f"{executable.name} 从工具包外加载 {name}：{path}")


def bundle_tools(tool_dir, clang, linker):
    libraries = tool_dir / "lib"
    link = tool_dir / "link"
    licenses = tool_dir / "licenses"
    for directory in (libraries, link, licenses):
        directory.mkdir(parents=True, exist_ok=True)
    shutil.copy2(clang, tool_dir / "clang")
    shutil.copy2(linker, link / "ld.lld")
    for name in ("crt1.o", "crti.o", "crtbegin.o", "crtend.o", "crtn.o", "libgcc.a"):
        shutil.copy2(library_path(name), link / name)
    pending = [tool_dir / "txc", tool_dir / "clang", link / "ld.lld"]
    for name in ("libstdc++.so.6", "libgcc_s.so.1", "libicui18n.so", "libicuuc.so",
                 "libicudata.so", "libpcre2-8.so", "libsodium.so", "libargon2.so",
                 "libxml2.so", "libpq.so", "libnghttp2.so", "libcurl.so",
                 "libssl.so", "libcrypto.so", "libcares.so", "libz.so", "libmsquic.so.2"):
        path = library_path(name)
        shutil.copy2(path.resolve(), libraries / name)
        # 链接使用 libfoo.so，ELF 的 DT_NEEDED 则使用 SONAME；两种名称都必须存在。
        soname = run("patchelf", "--print-soname", path.resolve()).strip()
        if soname and Path(soname).name != soname:
            raise RuntimeError("无效的共享库 SONAME：" + soname)
        if soname and soname != name:
            shutil.copy2(path.resolve(), libraries / soname)
        pending.append(path)
    seen = set()
    origins = set()
    # glibc 及其加载器由最低支持系统提供，避免把它们与其他系统组件混装。
    while pending:
        path = pending.pop()
        resolved = path.resolve()
        if resolved in seen:
            continue
        seen.add(resolved)
        if str(resolved).startswith(("/usr/", "/lib/")):
            origins.add(resolved)
        for name, dependency in elf_dependencies(path):
            if system_library.fullmatch(name):
                continue
            destination = libraries / name
            if dependency.resolve() != destination.resolve():
                shutil.copy2(dependency.resolve(), destination)
            pending.append(dependency)
    for path in libraries.iterdir():
        run("patchelf", "--set-rpath", "$ORIGIN", path)
    run("patchelf", "--set-rpath", "$ORIGIN/lib", tool_dir / "txc")
    run("patchelf", "--set-rpath", "$ORIGIN/lib", tool_dir / "clang")
    run("patchelf", "--set-rpath", "$ORIGIN/../lib", link / "ld.lld")
    packages = set()
    for path in origins:
        query = subprocess.run(["dpkg-query", "-S", str(path)], capture_output=True,
                               encoding="utf-8", errors="replace")
        for line in query.stdout.splitlines():
            package = line.split(": ", 1)[0].split(":", 1)[0]
            notice = Path("/usr/share/doc") / package / "copyright"
            if notice.is_file():
                shutil.copy2(notice, licenses / (package + "-copyright"))
                packages.add(package)
    (licenses / "packages.txt").write_text(run("dpkg-query", "-W", "-f=${Package} ${Version}\n",
        *sorted(packages)), encoding="utf-8")


def write_manifest(tool_dir, abi):
    fields = [("abi", abi), ("txc", digest(tool_dir / "txc")),
              ("stdlib", digest(tool_dir / "libtxstdlib.a")),
              ("stdlib_lto", digest(tool_dir / "libtxstdlib_lto.a")),
              ("clang", digest(tool_dir / "clang")), ("lld", digest(tool_dir / "link/ld.lld"))]
    fields += [("lib/" + path.name, digest(path)) for path in sorted((tool_dir / "lib").iterdir()) if path.is_file()]
    (tool_dir / "package.compat").write_text("tx-package-v3\n" +
        "".join(f"{name} {value}\n" for name, value in fields), encoding="utf-8")
