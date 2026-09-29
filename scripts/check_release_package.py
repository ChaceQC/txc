"""13.4：公开接口/符号/许可证清单以及脱离源码树的安装包验证。"""

from pathlib import Path
import hashlib
import json
import os
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent


def run(args, cwd):
    result = subprocess.run([str(value) for value in args], cwd=cwd, capture_output=True,
                            text=True, encoding="utf-8", errors="replace", timeout=120)
    assert result.returncode == 0, (args, result.stdout, result.stderr)
    return result.stdout


def main():
    modules = sorted((ROOT / "tx/stdlib").glob("*.txh"))
    docs = {path: path.read_text(encoding="utf-8") for path in (ROOT / "docs").glob("*.md")}
    examples = {path: path.read_text(encoding="utf-8") for path in (ROOT / "examples").rglob("*.tx")}
    inventory = []
    for module in modules:
        text = module.read_text(encoding="utf-8")
        references = [path.name for path, content in docs.items()
                      if module.name in content and "plan" not in path.name]
        users = [path.relative_to(ROOT).as_posix() for path, content in examples.items()
                 if f'"{module.name}"' in content]
        assert references, f"没有中文文档入口：{module.name}"
        assert users, f"没有公开示例：{module.name}"
        inventory.append({"module": module.name, "declarations": len(re.findall(r"^def ", text, re.M)),
                          "sha256": hashlib.sha256(module.read_bytes()).hexdigest(),
                          "docs": references, "examples": users})
    licenses = sorted(path for path in (ROOT / "tx").iterdir()
                      if re.search(r"LICENSE|COPYING|NOTICE", path.name))
    assert all(path.stat().st_size > 0 for path in licenses)
    required = {"MBEDTLS-LICENSE", "NGHTTP2-LICENSE", "NGHTTP3-LICENSE", "MSQUIC-LICENSE",
                "LLVM-LICENSE.TXT", "LIBSODIUM-LICENSE", "ICU-LICENSE", "LIBXML2-LICENSE",
                "SQLITE-LICENSE", "POSTGRESQL-LICENSE", "ARGON2-LICENSE"}
    assert required <= {path.name for path in licenses}

    with tempfile.TemporaryDirectory(prefix="tx-installed-") as location:
        installed = Path(location)
        package = installed / "tx"
        shutil.copytree(ROOT / "tx", package)
        shutil.copytree(package / "examples", installed / "examples")
        shutil.copytree(package / "docs", installed / "docs")
        txc = package / "txc.exe"
        source = installed / "all_modules.tx"
        source.write_text("\n".join(f'import "{module.name}" as module_{index}'
                                    for index, module in enumerate(modules)) +
                          "\n\ndef main() -> int\n{\n    return 0\n}\n", encoding="utf-8")
        ir = installed / "interfaces.ll"
        run([txc, "emit-llvm", source, "-o", ir], installed)
        declarations = set(re.findall(r"^declare [^\n]*?@(txrt_\w+)\(",
                                      ir.read_text(encoding="utf-8"), re.M))
        nm = shutil.which("nm")
        assert nm, "需要构建工具链的 nm 执行静态符号核对"
        symbols = run([nm, "-g", "--defined-only", package / "libtxstdlib.a"], installed)
        defined = set(re.findall(r"\b(txrt_\w+)\s*$", symbols, re.M))
        missing = sorted(declarations - defined)
        assert not missing, f"静态库缺少公开 ABI：{missing}"
        selected = ["algorithm_extended.tx", "unicode.tx", "bytes_file_stream.tx", "decimal_money.tx",
                    "json_stream.tx", "sqlite.tx", "db_async.tx", "password.tx", "public_key.tx",
                    "profile_workload.tx", "toolchain_test.tx", "async_file.tx", "cbor.tx",
                    "concurrency.tx", "network_endpoints.tx", "profile_memory.tx", "ipc.tx"]
        for name in selected:
            program = installed / (Path(name).stem + ".exe")
            run([txc, installed / "examples" / name, "-o", program], installed)
            run([program], installed)
        assert not (installed / "src").exists()
        print(f"安装包验证通过：{len(modules)} 个模块，{len(declarations)} 个 ABI 符号，{len(selected)} 个运行示例")

    output = ROOT / "tx_build" / "release_inventory.json"
    output.parent.mkdir(exist_ok=True)
    output.write_text(json.dumps({"modules": inventory, "licenses": [p.name for p in licenses],
                                  "abi_symbols": len(declarations), "installed_examples": selected},
                                 ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print("清单：tx_build/release_inventory.json")


if __name__ == "__main__":
    main()
