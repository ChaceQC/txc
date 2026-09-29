import json
import statistics
from pathlib import Path

root = Path(__file__).resolve().parents[2]
out = root / 'benchmarks/performance_retest_2026-09-30'
baseline = root / 'benchmarks/performance_baseline_2026-09-29/samples'


def read(path):
    return json.loads(path.read_text(encoding='utf-8'))


def table(headers, rows):
    return ['| ' + ' | '.join(headers) + ' |', '| ' + ' | '.join(['---'] * len(headers)) + ' |',
            *['| ' + ' | '.join(str(item) for item in row) + ' |' for row in rows], '']


def fmt(value):
    return f'{value:.3f}' if isinstance(value, (int, float)) else str(value)


def change(old, new):
    return f'{(new / old - 1) * 100:+.1f}%' if old else '—'


def main():
    manifest = read(out / 'manifest.json')
    complete = read(out / 'completed.json')
    language = read(out / 'language_features.json')
    audit = read(out / 'performance_retest.json')
    old_audit = read(baseline / 'performance_retest_20260929.json')
    diverse = read(out / 'diverse.json')
    old_diverse = read(baseline / 'diverse_performance_results_20260929.json')
    special = read(out / 'special.json')
    equivalent = read(out / 'equivalence.json')['results']
    lines = ['# 2026-09-30 全量性能复测', '',
             '本轮重建当前工作树，串行执行现有基准及新增优化专项。'
             '所有程序正常退出；固定校验值、逐轮稳定性及外部题答案核对通过。'
             '下文保留发现的变慢项，不把全套运行完成等同于全部性能目标达标。', '',
             f"- HEAD：`{manifest['head']}`；工作树状态及源码/工具链 SHA-256 见 [manifest.json](manifest.json)。",
             f"- 测量开始 UTC：{manifest['started_utc']}；完成 UTC：{complete['completed_utc']}。",
             '- 先执行 `scripts/build.ps1`；成功打包后脚本已清理临时 `build/`。',
             '- 测量前后核对源码、标准库接口及工具链哈希一致。所有负载串行执行，未与本轮构建并行。',
             '- 耗时单位为 ms；除进程/启动/编译外，表中为程序内部计时中位数。百分比为本轮相对 9 月 29 日优化前复测，负值表示变快。跨轮机器状态与代码同时变化，不能仅凭这些百分比归因。', '',
             '## 覆盖与采样', '']
    lines += table(['基准', '范围', '预热 / 正式轮数'], [
        ['语言特性', '22 项，TX/C++/Python/Java', '1 / 7，交替顺序'],
        ['标准库组合', '10 项 TX；C++ 字典提供两种参考，共 11 行', '1 / 7，交替顺序'],
        ['模块与特性审计', '21 项计算、10 项特性、6 项系统；另含 Python/Java 历史组合', '1 / 3，交替顺序'],
        ['本机网络', 'HTTP、Requests、WebSocket 共 3 项 TX', '1 / 3，交替顺序'],
        ['外部题', '2 题各 5 组，5 种语言；两题 TX 普通程序全套正式数据另核对', '2 / 17，轮换顺序'],
        ['综合负载', '18 项，4 种语言', '1 / 5，轮换顺序'],
        ['启动 / 编译', '空程序 4 种语言 / 5 类编译操作，含采样内存', '1 / 7；1 / 3'],
        ['专项', '调用借用、静态运行时、长随机数、解析、格式化、serde、容器/调用、迭代/算术/拷贝、编码/统计、文件流', '1 / 7；静态运行时 1 / 3'],
        ['校准对照', '18 项综合、堆、补偿统计；GCC 与随包 clang 对照', '1 / 5，交替顺序'],
    ])
    lines += ['“全量”指现有性能基准及本轮列出的优化专项。沿用的模块审计按上一轮口径覆盖 37 个公开模块，'
              '不能等同于全部 50 个模块都有独立性能基线；原来缺少代表负载的并发、DNS、TLS 等模块未在本轮新建基准。', '',
              '## 语言特性', '', '[7 轮原始样本](language_features.json)；每项四语言均核对固定校验值。', '']
    rows = []
    old_language = {}
    for line in (root / 'benchmarks/performance_retest_2026-09-29.md').read_text(encoding='utf-8').splitlines():
        fields = [part.strip() for part in line.split('|')]
        if len(fields) == 7 and fields[1] in language['checksums']:
            old_language[fields[1]] = float(fields[2])
    for name in language['checksums']:
        old = old_language[name]
        rows.append([name, *[fmt(statistics.median(language[label][name])) for label in ['TX', 'CPP', 'Python', 'Java']],
                     fmt(old), change(old, statistics.median(language['TX'][name]))])
    lines += table(['项目', 'TX', 'C++', 'Python', 'Java', '优化前 TX', 'TX 变化'], rows)
    for section in ['library', 'audit', 'network']:
        lines += [f'## {dict(library="标准库组合", audit="模块与新增特性", network="本机网络")[section]}', '']
        groups = {'组合': audit[section]['results']} if section == 'library' else audit[section]['results']
        old_groups = {'组合': old_audit[section]['results']} if section == 'library' else old_audit[section]['results']
        for group, data in groups.items():
            lines += [f'### {group}', '']
            med = data['medians_ms']
            labels = list(med)
            names = list(dict.fromkeys(name for label in labels for name in med[label]))
            old_tx = old_groups[group]['medians_ms'].get('TX', {})
            rows = []
            for name in names:
                new = med.get('TX', {}).get(name)
                old = old_tx.get(name)
                rows.append([name, *[fmt(med[label].get(name, '—')) for label in labels],
                             fmt(old) if old is not None else '—', change(old, new) if old is not None and new is not None else '—'])
            lines += table(['项目', *labels, '优化前 TX', 'TX 变化'], rows)
            if data['cross_language_checksum_differences']:
                lines += ['跨语言校验值差异：`' + json.dumps(data['cross_language_checksum_differences'], ensure_ascii=False) + '`。', '']
    lines += ['标准库组合使用整数毫秒计时，短项比值不稳定。模块审计的 `random_int` 在 TX/C++ 使用相同随机序列，'
              'Python/Java 序列不同，不能据此比较同随机序列开销。HTTP 不同语言的输出项目名不完全相同，保留原名称与各自稳定性检查。', '',
              '## 综合负载与完整契约对照', '',
              'C++ 采用目前仓库中已校准的 64 位容器、完整解析结果及 payload schema；优化前归档的 C++ 部分参考较简化，'
              '因此下表只比较 TX 历史变化，不把 C++ 历史变化作为优化收益。', '']
    rows = []
    for name, values in diverse['diverse']['cases'].items():
        old = old_diverse['diverse']['cases'][name]['TX']['median_ms']
        rows.append([name, *[fmt(values[label]['median_ms']) for label in ['TX', 'C++', 'Python', 'Java']], fmt(old), change(old, values['TX']['median_ms'])])
    lines += table(['项目', 'TX', 'C++', 'Python', 'Java', '优化前 TX', 'TX 变化'], rows)
    lines += ['### 同轮 GCC / clang 校准倍率', '', '每格为 TX/C++，倍率越小越好；此处与四语言综合采样是独立的 5 轮校准采样。', '']
    rows = []
    for group in ['diverse', 'heap', 'statistics']:
        for name, values in equivalent[group]['cases'].items():
            clang = equivalent[group + '_clang']['cases'][name]
            rows.append([name, fmt(values['TX_median_ms']), fmt(values['CPP_median_ms']), fmt(clang['CPP_median_ms']),
                         f"{values['TX_median_ms']/values['CPP_median_ms']:.2f}×", f"{clang['TX_median_ms']/clang['CPP_median_ms']:.2f}×"])
    lines += table(['项目', 'TX（GCC 配对轮）', 'GCC', 'clang', 'TX/GCC', 'TX/clang'], rows)
    lines += ['## 外部题', '', '两题均用本轮工具链重建 TX 普通程序与计时程序。普通程序通过各自全部正式数据，计时程序每次输出与答案核对。', '']
    for task, cases in audit['external']['results']['tasks'].items():
        lines += [f'### {task}', '']
        rows = []
        for name, values in cases.items():
            old = old_audit['external']['results']['tasks'][task][name]['median_ms']['TX']
            med = values['median_ms']
            rows.append([name, *[fmt(med[label]) for label in ['TX', 'C++', 'Python', 'Java', 'JavaScript']], fmt(old), change(old, med['TX'])])
        lines += table(['输入', 'TX', 'C++', 'Python', 'Java', 'JavaScript', '优化前 TX', 'TX 变化'], rows)
    lines += ['## 专项', '', '专项逐轮核对校验值稳定；这里报告本轮绝对耗时。', '']
    rows = []
    for group, data in special.items():
        if group == 'random_long':
            continue
        for name, median in data['medians_ms'].items():
            samples = [sample[name][0] for sample in data['samples']]
            rows.append([group, name, fmt(median), fmt(min(samples)), fmt(max(samples)), len(samples)])
    lines += table(['专项', '项目', '中位数', '最小', '最大', '轮数'], rows)
    lines += ['长随机数（固定校验和 2500067985）：' + '；'.join(f'{label} {statistics.median(values):.3f} ms' for label, values in special['random_long']['samples_ms'].items()) + '。', '',
              '## 进程、启动、编译及内存', '', '内存每 2 ms 轮询进程树工作集，只代表近似采样峰值。', '']
    rows = []
    for label, data in diverse['diverse']['processes'].items():
        rows.append(['综合运行', label, fmt(data['wall_ms']['median']), fmt(data['sampled_tree_peak_mib']['median'])])
    for label, data in diverse['startup'].items():
        rows.append(['空程序启动', label, fmt(data['wall_ms']['median']), fmt(data['sampled_tree_peak_mib']['median'])])
    for label, data in diverse['compilation'].items():
        rows.append(['编译', label, fmt(data['wall_ms']['median']), fmt(data['sampled_tree_peak_mib']['median'])])
    lines += table(['类型', '语言 / 操作', '墙钟 ms', '采样峰值 MiB'], rows)
    lines += ['## 证据与复现边界', '',
              '- [主采样脚本](run.py)、[工具链与源码快照](manifest.json)、[完成记录](completed.json)。',
              '- [语言特性原始样本](language_features.json)、[语言特性日志](language_features.log)、[标准库/模块/网络/外部题样本](performance_retest.json)。',
              '- [综合与进程样本](diverse.json)、[校准对照样本](equivalence.json)、[专项样本](special.json)。',
              '- 复测入口为 `python -X utf8 -B benchmarks/performance_retest_2026-09-30/run.py`。再次执行前应复制到新的归档目录，避免覆盖本轮样本。',
              '- 主脚本沿用本机 `tx_build/performance_retest_20260927.py`、模块审计目录和外部题比较脚本；外部题源文件/正式数据位于 `E:/Project/problems`。它不是脱离本机外部数据即可独立复现的套件。',
              '- 本轮没有修改编译器性能实现，也没有运行与性能任务无关的大量功能测试。', '']
    (out / 'README.md').write_text('\n'.join(lines), encoding='utf-8')


if __name__ == '__main__':
    main()
