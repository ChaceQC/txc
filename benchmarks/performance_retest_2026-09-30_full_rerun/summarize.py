"""复用逐项汇总算法，替换上一轮报告中写死的描述与倍率。"""

from run import archive, load, previous


def main():
    report = load("previous_full_summary", previous / "summarize.py")
    report.out = archive
    report.previous = previous
    report.completion = previous
    original_write = report.write

    def write(name, lines):
        if name == "README.md":
            lines.insert(2, "本页记录原有套件；新增标准库已另行补齐。完整结论见 [含新增标准库的总报告](FULL_REPORT.md)、[52 模块覆盖映射](coverage.md)及[合并差距清单](all_gaps.md)。")
            # 汇总器保留了上一轮人工结论；本轮仅保留由原始样本计算的内容。
            lines = [line for line in lines if not line.startswith((
                "单节点自环 GC 17.60×", "接近阈值的编码项目随采样",
                "`map_hit_128` 的校准轮变慢",
            ))]
            for index, line in enumerate(lines):
                line = line.replace("# 2026-09-30 提交后全量性能复测",
                                    "# 2026-09-30 新一轮全量性能复测")
                line = line.replace("先提交优化及已有记录，再完成正式重建、封包和串行采样。",
                                    "基于当前工作树正式重建、封包并串行采样。")
                line = line.replace("../performance_retest_2026-09-30/README.md",
                                    "../performance_retest_2026-09-30_after_completion/README.md")
                line = line.replace("benchmarks/performance_retest_2026-09-30_after_completion/run.py",
                                    "benchmarks/performance_retest_2026-09-30_full_rerun/run.py")
                line = line.replace("上一轮专项 TX", "上一轮 TX")
                if line.startswith("“全量”为现有性能套件"):
                    line = "本页为沿用的 37 模块性能套件；本轮另补了 15 个模块及 test/log 扩展，共 29 个四语言负载，详见 [新增标准库报告](../stdlib_full_2026-09-30/README.md)。所有 52 个模块的代表负载已运行，不能外推为所有 API 或所有失败场景全覆盖。"
                lines[index] = line
        lines = [line.replace("新增契约使用上一轮专项记录", "新增契约同样使用上一轮全量记录")
                 for line in lines]
        original_write(name, lines)

    report.write = write
    report.main()


if __name__ == "__main__":
    main()
