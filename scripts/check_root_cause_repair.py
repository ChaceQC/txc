"""根因报告的定向语义、绑定身份、效果摘要与快速路径检查。"""

import json
from pathlib import Path

from check_static_execution import function, run


root = Path(__file__).resolve().parents[1]
output = root / "tx_build/root_cause_repair_checks"


def main():
    output.mkdir(parents=True, exist_ok=True)
    source = root / "tests/root_cause_repair.tx"
    executable = output / "repair.exe"
    llvm = output / "repair.ll"
    analysis = output / "repair.analysis.json"
    run([root / "tx/txc.exe", source, "-o", executable])
    assert "ROOT_CAUSE_REPAIR_OK" in run([executable])
    run([root / "tx/txc.exe", "emit-llvm", source, "-o", llvm])
    run([root / "tx/txc.exe", "emit-analysis", source, "-o", analysis])
    ir = llvm.read_text(encoding="utf-8")
    guards = function(ir, "repeated_guard_names")
    assert guards.count("call i32 @txrt_sync_local_lock_i64(") == 2
    assert "call i32 @txrt_sync_lock_i64(" not in guards
    shadowed = function(ir, "shadowed_guard_fallback")
    assert "call i32 @txrt_sync_local_lock_i64(" in shadowed
    assert "call i32 @txrt_sync_lock_i64(" in shadowed
    heap = function(ir, "default_heap")
    assert "call i32 @txrt_value_clone(" not in heap
    assert heap.count("call i32 @txrt_gc_safepoint_context(") == 1
    for name in ("custom_heap", "heap_rebound", "escaped_heap"):
        assert "call i32 @txrt_value_clone(" in function(ir, name), name
    cursor = function(ir, "snapshot_cursor")
    assert "call ptr @txrt_iterator_snapshot_cursor(" in cursor
    assert "getelementptr inbounds %tx_iterator_cursor" in cursor
    assert "call i32 @txrt_iterator_next_scalar_i64(" not in cursor
    assert "call ptr @txrt_iterator_snapshot_cursor(" not in function(ir, "shared_cursor")
    assert "ptr @tx_serde_pair_i64_str, ptr null, ptr null" in ir
    functions = {item["name"]: item for item in json.loads(analysis.read_text(encoding="utf-8"))["functions"]}
    for name in ("default_heap", "snapshot_cursor"):
        item = functions["m0_" + name]
        locals_ = {variable["id"] for variable in item["variables"]
                   if variable["name"] in ("values", "cursor", "item")}
        roots = {alias for block in item["blocks"] for instruction in block["instructions"]
                 if instruction["operation"] == "bind_local" and instruction["variable"] in locals_
                 for alias in instruction["aliases"]}
        assert roots.isdisjoint(item["captured_roots"]), name
    saved = functions["m0_saved_element"]["parameters"]
    assert saved[0]["mutated"] and not saved[0]["captured"]
    assert saved[1]["captured"] and 1 in saved[0]["contains_parameters"]
    assert functions["m0_returned_option"]["parameters"][0]["returned_content"]
    assert functions["m0_option_alias"]["parameters"][0]["returned_alias"]
    print("PASS root cause repair: scope identity, heap, iterator, serde, escape and error boundaries")
    source = root / "tests/result_length.tx"
    executable = output / "length.exe"
    llvm = output / "length.ll"
    run([root / "tx/txc.exe", source, "-o", executable])
    assert "RESULT_LENGTH_OK" in run([executable])
    run([root / "tx/txc.exe", "emit-llvm", source, "-o", llvm])
    ir = llvm.read_text(encoding="utf-8")
    formatted = function(ir, "format_length")
    assert "@txrt_format_integer_length(" in formatted
    assert "@txrt_format_specialized(" not in formatted
    assert "@txrt_encoding_encode_known_length(" in function(ir, "encoded_length")
    assert "@txrt_encoding_decode_known_length(" in function(ir, "decoded_length")
    fallback = function(ir, "check_fallback")
    assert "@txrt_format_specialized(" in fallback
    assert "@txrt_encoding_encode_literal(" in fallback
    print("PASS length projection: Unicode, BOM, boundaries, side effects, failures and materialization fallback")


if __name__ == "__main__":
    main()
