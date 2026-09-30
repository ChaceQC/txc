"""定向验证全程序 SSA/别名/逃逸/移动和真正的 ThinLTO 优化。"""

from pathlib import Path
import json
import os
import re
import shutil
import subprocess


root = Path(__file__).resolve().parents[1]
output = root / "tx_build/program_analysis_checks"


def run(arguments, timeout=180, environment=None, cwd=root):
    if environment is None:
        environment = os.environ.copy()
        environment["PATH"] = str(root / "tx") + os.pathsep + environment["PATH"]
    result = subprocess.run([str(argument) for argument in arguments], cwd=cwd,
                            env=environment, capture_output=True, encoding="utf-8",
                            errors="replace", timeout=timeout)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    return result.stdout


def function_ir(ir, name):
    result = re.search(rf"^define [^\n]* @tx_fn_m0_{name}_0\([^\n]*\)[^\n]*\{{\n(.*?)^\}}",
                       ir, re.MULTILINE | re.DOTALL)
    assert result, name
    return result[1]


def check_analysis():
    source = root / "tests/program_analysis.tx"
    analysis_path = output / "program.analysis.json"
    run([root / "tx/txc.exe", "emit-analysis", source, "-o", analysis_path])
    analysis = json.loads(analysis_path.read_text(encoding="utf-8"))
    functions = {function["name"]: function for function in analysis["functions"]}

    def summary(name):
        return next(value for symbol, value in functions.items() if symbol.endswith("_" + name))

    assert summary("recursive_alias")["parameters"][0]["returned_alias"]
    assert summary("other_alias")["parameters"][0]["returned_alias"]
    assert summary("store")["parameters"][0]["mutated"]
    assert summary("store")["parameters"][1]["captured"]
    assert summary("return_container")["parameters"][0]["returned_content"]
    assert summary("loop_alias")["parameters"][0]["returned_alias"]
    assert summary("owned_text")["parameters"][0]["rebound"]
    assert summary("store_then_read")["parameters"][1]["returned_alias"]
    assert any(instruction["operation"] == "cleanup" for block in summary("witness_scope")["blocks"]
               for instruction in block["instructions"])
    for name in ("branch_text", "loop_alias"):
        assert any(instruction["operation"] == "phi" for block in summary(name)["blocks"]
                   for instruction in block["instructions"])
    assert any(edge["exceptional"] for block in summary("exception_text")["blocks"]
               for edge in block["successors"])
    llvm = output / "program.ll"
    run([root / "tx/txc.exe", "emit-llvm", source, "-o", llvm])
    ir = llvm.read_text(encoding="utf-8")
    assert '"target-cpu"="x86-64"' in ir
    assert '"target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87"' in ir
    assert "@txrt_str_clone(" not in function_ir(ir, "move_text")
    assert "@txrt_str_clone(" in function_ir(ir, "exception_text")
    assert "@txrt_str_clone(" in function_ir(ir, "loop_text")
    assert "@txrt_record_struct_new(" not in function_ir(ir, "native_recursive")
    assert "@tx_fn_m0_recursive_score_0_native(" in function_ir(ir, "native_recursive")
    for lto in (True, False):
        executable = output / ("program_lto.exe" if lto else "program_native.exe")
        arguments = [root / "tx/txc.exe", source, "-o", executable]
        if not lto:
            arguments.append("--no-lto")
        run(arguments)
        assert "PROGRAM_ANALYSIS_OK" in run([executable])
    print("PASS SSA/CFG, recursive aliases, heap escape, move lifetime, LTO/native parity")


def check_lto():
    probe = output / "lto_constant.o"
    run([root / "tx/clang.exe", "-target", "x86_64-w64-windows-gnu", "-flto=thin",
         "-O3", "-x", "ir", "-c", root / "tests/lto_constant.ll", "-o", probe])
    link = root / "tx/link"
    executable = output / "lto_constant.exe"
    library = output / "libtxstdlib_lto.a"
    shutil.copy2(root / "tx/libtxstdlib_lto.a", library)
    libraries = ["stdc++", "mingw32", "gcc_s", "gcc", "moldname", "mingwex", "msvcrt",
                 "kernel32", "pthread", "advapi32", "bcrypt", "crypt32", "ncrypt", "winhttp",
                 "ws2_32", "dnsapi", "shell32", "user32", "iconv"]
    library_paths = [link / ("lib" + name + (".dll.a" if name == "stdc++" else ".a"))
                     for name in libraries]
    run([link / "ld.lld.exe", "-flavor", "link", "/lldmingw", "/lldsavetemps",
         "/opt:lldlto=3", "/entry:mainCRTStartup", "/subsystem:console", "/machine:x64",
         "/alternatename:__image_base__=__ImageBase",
         "/out:" + str(executable), link / "crt2.o", link / "crtbegin.o",
         probe, library, *library_paths,
         link / "default-manifest.o", link / "crtend.o"])
    run([executable])
    optimized = list(output.glob("*lto_constant*opt.bc"))
    assert optimized, "链接器未输出 ThinLTO 优化记录"
    proved = False
    for path in optimized:
        llvm = path.with_suffix(".ll")
        run([root / "tx/clang.exe", "-target", "x86_64-w64-windows-gnu", "-x", "ir",
             "-S", "-emit-llvm", path, "-o", llvm])
        ir = llvm.read_text(encoding="utf-8")
        match = re.search(r"define [^\n]* @main\([^\n]*\)[^\n]*\{(.*?)^\}",
                          ir, re.MULTILINE | re.DOTALL)
        if match and "@txrt_require_success" not in match[1] and "ret i32 0" in match[1]:
            proved = True
    assert proved, "未证明程序与运行时之间的调用被跨模块消除"
    print("PASS ThinLTO: runtime success check imported and folded to main -> 0")


def check_installed():
    installed = output / "中文 安装目录"
    package = installed / "tx"
    shutil.copytree(root / "tx", package, dirs_exist_ok=True)
    shutil.copytree(root / "tests/program_analysis_module", installed / "program_analysis_module",
                    dirs_exist_ok=True)
    source = installed / "program.tx"
    shutil.copy2(root / "tests/program_analysis.tx", source)
    executable = installed / "中文 输出/program.exe"
    environment = os.environ.copy()
    environment["PATH"] = str(Path(os.environ["SystemRoot"]) / "System32")
    run([package / "txc.exe", source, "-o", executable], environment=environment, cwd=installed)
    assert "PROGRAM_ANALYSIS_OK" in run([executable], environment=environment, cwd=installed)
    print("PASS installed ThinLTO package: Chinese paths, system-only PATH, no C++ source tree")


def main():
    output.mkdir(parents=True, exist_ok=True)
    check_analysis()
    check_lto()
    check_installed()


if __name__ == "__main__":
    main()
