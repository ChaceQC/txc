"""08：serde 相关行为、直接路径/失败清理和静态描述检查。"""

import json
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] /
                       "benchmarks/performance_optimization_08_2026-09-29"))
from measure import root, archive, run


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    out = root / "tx_build/serde_direct_checks"
    out.mkdir(parents=True, exist_ok=True)
    cases = ["serde/behavior", "serde/module", "formats/serde_migration", "serde/direct"]
    for case in cases:
        executable = out / (case.replace("/", "_") + ".exe")
        run([root / "tx/txc.exe", root / "tests" / (case + ".tx"), "-o", executable])
        print(run([executable]).strip())
    llvm = out / "direct.ll"
    run([root / "tx/txc.exe", "emit-llvm", root / "tests/serde/direct.tx", "-o", llvm])
    ir = llvm.read_text(encoding="utf-8")
    expected = ["vector_integer", "vector_floating", "vector_boolean", "vector_bytes",
                "integer", "floating", "boolean", "text", "bytes"]
    for codec in expected:
        assert "ptr @tx_serde_" + codec in ir, codec
    assert ".json_order = private constant" in ir
    assert ".cbor_order = private constant" in ir
    assert "{ i64, ptr, ptr, ptr, ptr }" in ir
    native = out / "direct_native.exe"
    run(["g++", "-std=c++23", "-O2", "-finput-charset=UTF-8", "-fexec-charset=UTF-8",
         "-Isrc", root / "tests/serde/direct_native.cpp", root / "tx/libtxstdlib.a",
         "-Ltx/link", "-lwinhttp", "-lws2_32", "-ladvapi32", "-lbcrypt", "-lshell32",
         "-luser32", "-liconv", "-o", native])
    evidence = run([native]).strip()
    print(evidence)
    (archive / "structure.json").write_text(json.dumps(
        {"cases": cases, "bound_codecs": expected, "native": json.loads(evidence)},
        ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
