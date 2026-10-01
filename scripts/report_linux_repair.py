"""汇总已完成的热点配对采样、验证与交付指纹。"""
from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json
import shutil
import sys

root = Path(__file__).resolve().parents[1]
archive = root / "benchmarks/linux_repair_2026-10-01"


def read(name):
    return json.loads((archive / (name + ".json")).read_text(encoding="utf-8"))


def main():
    checks = read("checks")
    if not checks["passed"]:
        raise RuntimeError("验证尚未通过")
    stages = {name: read(name) for name in ("platform", "platform_mount", "runtime", "queue", "serde_final")}
    paths = [root / "src/stdlib" / name for name in (
        "ws_frames.cpp", "x509_parse_posix.cpp", "filesystem_listing.cpp", "format_stream.cpp",
        "format_stream.hpp", "json_parser.cpp", "json_parser_string.cpp", "json_output.hpp")]
    paths += [root / "src/backend/cpp/queue_abi.cpp"]
    paths += [root / "tx/linux" / name for name in ("txc", "libtxstdlib.a", "libtxstdlib_lto.a")]
    manifest = {"completed_utc": datetime.now(timezone.utc).isoformat(), "checks_passed": True,
                "sha256": {str(path.relative_to(root)): hashlib.sha256(path.read_bytes()).hexdigest() for path in paths}}
    (archive / "delivery.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    lines = ["# Linux 热点修复与定向复测", "",
        "按 L/W → TX/C++ 顺序修改并测量。所有下表来自同机旧/新可执行文件交替采样：一次预热、五次正式采样、逐轮核对校验值。没有重跑全量 181 项，也没有重测 Windows。", "",
        "## 实际修改", "",
        "- WebSocket：不超过 16 KiB 的帧将帧头、掩码与正文合并发送，避免短帧拆包触发 Nagle/延迟确认；大帧仍分块。",
        "- X.509：线程内弱引用记住最近成功解析的不可变 bytes 对象；不延长证书生命周期，不缓存信任、时间或吊销验证。",
        "- Linux 非递归目录名称枚举直接使用 readdir，保持排序、符号链接可见性与错误处理。",
        "- JSON/serde：空白判断、标点消费和字符串分支减少重复 peek，位置、限额和流式读取规则不变。",
        "- 流式 JSON：修复已有 ASCII 批量输出超出 4096 字节块上限的问题，在 UTF-8 标量边界切块；原生边界断言保留。",
        "- 标量/文本队列的小型 ABI 操作向 Clang 提供 always_inline 提示，使 ThinLTO 有机会合并循环中的解包及调用；保留检查和异常边界。", "",
        "## 主要结果", "", "| 位置/阶段 | 负载 | 旧 ms | 新 ms | 耗时变化 | 新 TX/C++ |", "| --- | --- | ---: | ---: | ---: | ---: |"]
    highlights = [("platform", "ws", "websocket_echo"), ("platform_mount", "ws", "websocket_echo"),
        ("platform", "security", "x509_parse_der"), ("platform_mount", "security", "x509_parse_der"),
        ("platform", "library", "fs"), ("platform_mount", "library", "fs"),
        ("serde_final", "diverse", "serde_short_text"), ("serde_final", "diverse", "serde_long_text"),
        ("queue", "features", "queue_push_pop")]
    for stage, suite, case in highlights:
        value = stages[stage][suite]["medians_ms"][case]
        ratio = f"{value['new']/value['cpp']:.2f}×" if value.get("cpp", 0) else "—"
        lines.append(f"| {stage} | {case} | {value['old']:.3f} | {value['new']:.3f} | {(value['new']/value['old']-1)*100:+.1f}% | {ratio} |")
    lines += ["", "platform/runtime/queue/serde_final 使用 Linux /tmp 本地文件系统；platform_mount 使用 /mnt/e 挂载盘。阶段分别构建，不同阶段的绝对数字不用于声称因果。runtime 之后的队列调整只单独复测 features，JSON/serde 结果取最终 serde_final 阶段。", "",
        "## 文件系统环境差距", "", "日志的逐条 flush 与即时错误语义没有修改。下面均为同一个旧程序，区别是测试文件/程序所在文件系统。", "",
        "| 旧程序负载 | /mnt/e ms | Linux 本地 ms |", "| --- | ---: | ---: |"]
    for suite, case in (("diagnostics", "log_file"), ("async_file", "async_file_rw"), ("library", "fs"), ("library", "file")):
        local = stages["platform"][suite]["medians_ms"][case]["old"]
        mount = stages["platform_mount"][suite]["medians_ms"][case]["old"]
        lines.append(f"| {case} | {mount:.3f} | {local:.3f} |")
    lines += ["", "因此本次不把 /mnt/e 的日志、文件和目录高倍率标为已消除。真实 Linux 部署应在本地文件系统重测，不能通过取消 flush 或隐藏 I/O 错误换取分数。", "",
        "## 验证与剩余问题", "",
        "已通过 JSON 行为/流式原生边界、serde TX/原生直接路径、队列调用效果和容器行为、目录名称/中文/悬空链接/错误路径、X.509 原有安全验证以及 Linux WebSocket 分片/控制帧验证。完整输出见 checks.json。",
        "队列机器码直接调用数：`" + json.dumps(stages["queue"]["features"].get("queue_abi_calls", {})) + "`。普通库与 ThinLTO 库均已构建，验证同时使用二者。",
        "任务提交等待的 L/W 差距尚未解决；本轮旧/新约同一水平。对象析构、深拷贝、循环 GC、小向量扫描仍存在 TX/C++ 差距，本次没有修改其算法或生成代码，不能把采样波动认作修复。X.509 加速适用于同一不可变对象的重复解析，不代表首次或大量不同证书解析的同等收益。",
        "Windows 工具包本次未重建，公共 C++ 文件的 Windows 原生验证尚未执行。Linux 交付位于 tx/linux，源码和二进制指纹见 delivery.json。", "",
        "## 全部定向样本的中位数", "", "| 阶段 | 套件 | 项目 | 旧 ms | 新 ms | C++ ms |", "| --- | --- | --- | ---: | ---: | ---: |"]
    for stage, suites in stages.items():
        for suite, data in suites.items():
            for case, value in data["medians_ms"].items():
                cpp = f"{value['cpp']:.3f}" if "cpp" in value else "—"
                lines.append(f"| {stage} | {suite} | {case} | {value['old']:.3f} | {value['new']:.3f} | {cpp} |")
    (archive / "README.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
    if "--clean" in sys.argv:
        build = root / "build/linux"
        if build.exists():
            if build.is_symlink() or build.resolve() != root.resolve() / "build/linux":
                raise RuntimeError("构建目录超出预期路径")
            shutil.rmtree(build)
    print("REPAIR_REPORT_READY")


if __name__ == "__main__":
    main()
