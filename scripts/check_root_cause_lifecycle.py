"""局部类、图生命周期、SQLite 只读复用和 TLS 身份租约的定向检查。"""

import os
from pathlib import Path
import subprocess
import tempfile

from check_static_execution import function, run
from check_tls import fixtures


root = Path(__file__).resolve().parents[1]
output = root / "tx_build/root_cause_repair_checks"


def native_case(source, directory, *arguments):
    target = directory / (Path(source).stem + ".exe")
    run(["g++", "-std=c++23", "-O2", "-finput-charset=UTF-8", "-fexec-charset=UTF-8", "-Isrc",
         '-DMBEDTLS_USER_CONFIG_FILE="' + str(root / 'cmake/mbedtls_user_config.h') + '"',
         "-Ibuild/_deps/mbedtls-src/include", "-Ibuild/_deps/sqlite_amalgamation-src",
         "-Ibuild/_deps/mbedtls-src/3rdparty/everest/include",
         "-Ibuild/_deps/postgres_binary-src/include", root / source, root / "tx/libtxstdlib.a",
         "-Ltx/link", "-lwinhttp", "-lws2_32", "-ldnsapi", "-ladvapi32", "-lbcrypt",
         "-lcrypt32", "-lncrypt", "-lshell32", "-luser32", "-liconv", "-o", target])
    environment = os.environ.copy()
    environment["PATH"] = str(root / "tx") + os.pathsep + environment["PATH"]
    result = subprocess.run([str(target), *map(str, arguments)], cwd=directory, env=environment,
                            capture_output=True, encoding="utf-8", errors="strict", timeout=60)
    assert result.returncode == 0, result.stdout + result.stderr
    print(result.stdout.strip(), flush=True)


def main():
    output.mkdir(parents=True, exist_ok=True)
    for case, marker in (("root_cause_lifecycle", "ROOT_CAUSE_LIFECYCLE_OK"),
                         ("performance_completion_gc", "GC_GRAPH_RESURRECTION_OK"),
                         ("performance_12_14/graphs", "graphs ok")):
        executable = output / (case.replace("/", "_") + ".exe")
        run([root / "tx/txc.exe", root / "tests" / (case + ".tx"), "-o", executable])
        result = run([executable])
        assert marker in result, result
        print("PASS", case, flush=True)
    llvm = output / "lifecycle.ll"
    run([root / "tx/txc.exe", "emit-llvm", root / "tests/root_cause_lifecycle.tx", "-o", llvm])
    ir = llvm.read_text(encoding="utf-8")
    for name in ("local_loop", "local_failure"):
        body = function(ir, name)
        assert "@txrt_class_local_new(" in body
        assert "@tx_class_local_cleanup_" in body
        assert "@txrt_record_class_new(" not in body
    assert "@txrt_record_class_new(" in function(ir, "escaping")
    assert "@txrt_str_clone(" not in function(ir, "text_conversion")
    assert "@txrt_dict_ref_contains_i64(" in function(ir, "scalar_dictionary")
    assert "@txrt_dict_ref_get_i64_key_i64(" in function(ir, "scalar_dictionary")
    print("PASS local class and dictionary/conversion IR", flush=True)
    with tempfile.TemporaryDirectory(prefix="root_cause_native_", dir=output) as temporary:
        directory = Path(temporary)
        native_case("tests/db/pool_read_reuse.cpp", directory)
        fixtures(directory)
        native_case("tests/crypto/tls_material_cache.cpp", directory, directory / "server.p12")


if __name__ == "__main__":
    main()
