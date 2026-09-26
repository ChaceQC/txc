# 时间、数学、随机数与统计

本页记录标准库计划第 6 节的公开契约和逐项实施证据。既有
`time.unix_millis/monotonic_*`、`time.sleep_millis`、
`random.seed/random_int/random_float` 继续保留原有语义。
6.1～6.6 均已实现，以下分别记录契约、构建与定向验证边界。

## 6.1 时长与单调时间

`time.txh` 增加 `duration`（有符号微秒）和 `instant`（单调时钟微秒刻度）。
使用 `duration_from_micros/millis/seconds` 创建时长，
`duration_add/sub/negate` 计算时长，`duration_as_micros/millis` 读取数值；
毫秒读取向零截断。算术及单位换算超出有符号 64 位整数时报告
`runtime_error/out_of_range`。负时长可参与计算，但 `sleep` 和
`sleep_cancelled` 要求非负，违例报告 `runtime_error/invalid_argument`。

`instant_now()` 读取单调时钟；`instant_elapsed(start, finish)` 返回有符号时差，
`instant_after(start, delay)` 对单调刻度做有界加法。`instant` 的刻度没有
Unix 纪元含义，不能转换为 `unix_millis()` 墙上时间；系统调时不影响
计时结果。公开字段只用于诊断和跨模块传递，调用方不应手工构造或修改
`instant` 的刻度。

`sleep(duration)` 使用单调时间，长时长分段等待；`sleep_cancelled(duration,
token)` 通过取消状态条件变量被立即唤醒。主动取消报告
`cancelled_error/cancelled`，令牌截止时间到达报告
`cancelled_error/deadline_exceeded`。进入睡眠时先观察取消，再处理零时长；
返回时已提交的睡眠不能被之后的取消撤销。旧的 `sleep_millis(int)`
保留原有不可取消接口，迁移时可写
`time.sleep(time.duration_from_millis(100))`。

**实施记录：** 公开类型和函数在 `time.txh`，直接类型化调用在 LLVM 和
`src/stdlib/time_values.cpp`。`scripts/build.ps1` 完整构建 Windows x64
工具链和静态库成功；`tests/time/duration.tx` 编译运行通过，覆盖单位换算、
单调计时、溢出、负时长、主动取消和截止时间。未运行全量或非 Windows
平台验收。

## 6.2 日历与 IANA 时区

`local_date` 的年限为 0001～9999，`local_time` 精确到微秒。
`offset_datetime` 保存 Unix 微秒和显式 UTC 偏移秒数；
`zoned_datetime` 额外保存 IANA 区域名及该瞬间的偏移。
`parse_local_date/time`、`parse_offset_datetime` 接受严格 ISO 8601 扩展格式：
`YYYY-MM-DD`、`HH:MM:SS[.ffffff]` 和
`YYYY-MM-DDTHH:MM:SS[.ffffff]Z|±HH:MM[:SS]`；小数为 1～6 位，
格式化时删去末尾零。默认使用公历与 ASCII 数字，不读取进程 locale。
非法文本报告 `parse_error/invalid_syntax`，越界日期、时刻或偏移报告
`parse_error/out_of_range`。

`date_add_days/months` 做日历加减；月份加减将超出的月末日夹到目标月
最后一天，超出 0001～9999 报 `runtime_error/out_of_range`。
`date_days_between` 和 `datetime_difference` 都按结束减开始计算，后者
比较绝对时间。`offset_from_unix_millis` 是旧毫秒时间戳进入新时间类型的
显式桥，微秒部分为零；没有从单调 `instant` 到日历时间的转换。

`timezone_version()` 返回随 ICU4C 78.3 发行物交付的 IANA 数据版本；
当前 Windows x64 发行物返回 `2026a`。
`at_zone` 把绝对时间转换到指定 IANA 区域，`resolve_local` 将本地日期、
时刻和区域解析成绝对时间，`zoned_local_date/time` 取回该区域的日历部分。
未知区域报告 `parse_error/invalid_zone`。`resolve_local` 的 `policy` 必须是
`reject`、`earlier` 或 `later`：重复时刻按绝对时间选择前后一次；缺失
时刻在 `reject` 下报 `parse_error/nonexistent_time`，另外两种策略使用
变更前/后的偏移解释本地字段，因此返回的实际本地时刻会向前/向后平移
跳变长度。重复时刻在 `reject` 下报 `parse_error/ambiguous_time`。
`format_zoned_datetime` 输出偏移日期时间后接 `[区域名]`；这段区域标记
不作为 `parse_offset_datetime` 输入，须显式调用 `at_zone`。

**实施记录：** `time.txh` 的日期类型经 LLVM 直接调用日历与 ICU 入口；
`src/stdlib/time_calendar.cpp` 负责解析与公历换算，
`time_calendar_abi.cpp` 负责 ABI，`time_zone.cpp` 使用固定发行物中的
ICU 时区数据。Windows x64 完整构建成功；`tests/time/calendar.tx` 与
`examples/time_calendar.tx` 编译运行通过。用例覆盖闰日、月末、负 Unix
时间、偏移格式、未知时区及柏林夏令时缺失/重复时刻。未运行全量或非
Windows 平台验收。

## 6.3 独立随机生成器

`random.make_generator(seed)` 创建调用方持有的 `generator`，
`reseed(source, seed)` 复位同一个生成器；`algorithm_version()` 返回
`splitmix64-v1`。状态使用一个 64 位整数，`next_int` 对包含端点的整个
有符号 64 位区间做无偏拒绝采样，`next_float` 从高 53 位生成 `[0,1)`。
相同种子、算法版本及调用顺序的整数/浮点、抽样和洗牌结果可复现。
这套生成器适于模拟和测试，密钥必须使用 `crypto.random_bytes`。

`shuffle<T>` 返回独立重排的 `vector<T>`，`sample<T>` 无放回抽取指定
数量且保留抽取顺序，`choice<T>` 返回一个元素；都适用于具体元素类型，
不把静态已知的元素逐项装入 `any`。空向量选择、负样本数、样本数超出
长度或无效整数区间报告 `runtime_error/invalid_argument`，失败前不推进
生成器状态。分布入口 `bernoulli` 接受 `[0,1]` 概率，`normal` 要求非负
标准差，`exponential` 要求正速率；参数必须有限，结果溢出则报
`runtime_error/out_of_range`。正态/指数分布使用平台数学库，跨平台可能
在最后几位有浮点差异。

普通赋值共享同一个 `generator` 状态；`deep_copy` 复制当前状态并独立
推进。实例生成器不访问旧的线程局部 `random.seed/random_int/random_float`
状态。第 2.6 节 `Send/Sync` 尚未完成，生成器及其别名不得跨线程共享；
需要并发模拟时各线程用不同种子独立创建实例，静态线程规则落地后再
做跨线程验收。

**实施记录：** `random.txh` 公开实例 API，`random_generator.cpp` 固定
SplitMix64 的状态推进与分布，`random_sampling.cpp` 对具体向量元素
生成直接 ABI；语义分析检查泛型实参，LLVM 不逐元素动态装箱。
Windows x64 完整构建成功；`tests/random/generator.tx` 覆盖固定种子、
复合元素、失败不推进、别名/深复制、分布边界及旧接口隔离；
`generator_send.tx` 的 `assert_send` 在源码位置拒绝该类型。未运行全量、
非 Windows 平台或线程集成验收。

## 6.4 数学函数

`math.txh` 增加如下静态签名。角度单位
为弧度，浮点函数遵循 IEEE 754，结果不依赖进程默认 locale。

| 新增函数 | 定义域与特殊值 |
| --- | --- |
| `sin/cos/tan(value: float) -> float` | 有限输入正常计算；NaN 或正负无穷输入返回 NaN。`tan` 接近极点时遵循数学库的舍入结果。 |
| `asin/acos(value: float) -> float` | 有限输入限 `[-1,1]`；越界、NaN、正负无穷返回 NaN。 |
| `atan(value: float) -> float`、`atan2(y: float, x: float) -> float` | NaN 传播；正负无穷和带符号零遵循 IEEE 754 象限规则。 |
| `sinh/cosh/tanh(value: float) -> float` | NaN 传播；正负无穷分别产生有符号无穷、正无穷及正负 1；有限输入溢出可得无穷。 |
| `exp/log/log10(value: float) -> float` | NaN 传播；`exp(-∞)=0`、`exp(+∞)=+∞`，有限正向溢出为 `+∞`；对数在正数上定义，零返回 `-∞`，负数返回 NaN，`log(+∞)=+∞`。 |
| `is_finite/is_nan/is_infinite(value: float) -> bool` | 对所有浮点值给出分类，不报定义域错误。 |
| `gcd/lcm(left: int, right: int) -> int` | 非负结果，`gcd(0,0)=0`、`lcm(0,x)=0`；结果超过最大 int 报 `runtime_error/out_of_range`。 |
| `pow_int(base: int, exponent: int) -> int` | 指数必须非负，`0^0=1`；负指数报 `runtime_error/invalid_argument`，任一中间乘法溢出报 `runtime_error/out_of_range`。 |
| `round_to_int(value: float, mode: str) -> int` | 模式为 `toward_zero/floor/ceiling/half_even/half_away`；`half_away` 在恰好半值时远离零。非有限输入或舍入结果超出 int 报 `runtime_error/out_of_range`，未知模式报 `runtime_error/invalid_argument`。 |

旧的 `sqrt/pow/min/max` 等接口继续保留此前的有限值与错误规则。
**实施记录：** LLVM 按静态签名直接调用 `math_extended.cpp` 的类型化 ABI；
Windows x64 完整构建成功，`tests/math/extended.tx` 编译运行通过，覆盖
正常值、NaN/无穷大、最小 int 的 `gcd/lcm`、整数幂溢出、正负半值与无效
舍入模式，并复核旧 `sqrt/floor`。未运行全量或非 Windows 平台验收。

## 6.5 统计

`statistics.txh` 提供下表接口。凡带 `values` 的函数，各提供
`vector<float>` 与 `iterator<float>` 两个静态重载；迭代器重载消费当前
游标直到末尾，不修改原向量。所有输入元素必须有限，NaN 或无穷大报
`runtime_error/non_finite`。不提供 `vector<int>` 到 `float` 的隐式统计
重载，避免大整数求和时静默丢精度。

| 新增函数/类型 | 结果与规则 |
| --- | --- |
| `mean(values) -> float`、`median(values) -> float` | 均值使用补偿/在线累计；中位数对排序快照取中央值，偶数个时用避免中间溢出的中点。 |
| `variance_population(values) / variance_sample(values) -> float` | 分母分别为 `n` 与 `n-1`；在线使用 Welford 更新。 |
| `stdev_population(values) / stdev_sample(values) -> float` | 对对应方差取平方根，微小负舍入误差可夹至零，真正非有限中间值报错。 |
| `quantile(values, p: float) -> float` | `p` 须有限且位于 `[0,1]`；排序后按 R7 线性插值，`p=0/1` 分别取最小/最大值，结果必须有限。 |
| `frequencies(values) -> map<float,int>` | 统计每个有限浮点值的出现次数；`+0.0/-0.0` 归为同一键，计数溢出报错。 |
| `histogram(values, lower: float, upper: float, bins: int) -> histogram_result` | `lower < upper` 且边界有限，`bins` 为 1～1,000,000；区间 `[lower, upper]` 等宽分桶，`upper` 进入最后一桶；结果含 `counts: vector<int>`、`underflow: int`、`overflow: int`。 |
| `accumulator`、`new_accumulator()`、`add(accumulator, value: float)` | 可逐项输入而不用保存全部数据；私有字段持有 `count/mean/m2`，普通赋值共享状态，`deep_copy` 得到独立状态。 |
| `count/mean_of/variance_population_of/variance_sample_of/stdev_population_of/stdev_sample_of(accumulator)` | 与批量公式一致；拒绝的输入不得改变累计器状态，计数和数值中间结果须有界。 |

空输入的均值、中位数、方差、标准差和分位数报
`runtime_error/empty_sample`；样本方差/样本标准差在仅 1 项时报
`runtime_error/insufficient_sample`，总体方差/总体标准差在 1 项时为零。
非有限概率或区间参数报 `runtime_error/invalid_argument`，任何累计或
插值结果超出有限 float、计数溢出报 `runtime_error/out_of_range`。
中位数与分位数的迭代器重载需收集快照，最多 1,000,000 项，超过报
`runtime_error/size_limit`；向量重载也只对副本排序，不重排输入。
流式累计器更新采用事务式提交，错误后仍可继续使用。

**实施记录：** `statistics_core.cpp` 以 Welford 更新和 `long double` 中间值
计算统计量；`statistics_bucket.cpp` 负责频数与直方图。LLVM 在编译期按
`vector<float>` 或 `iterator<float>` 选择直接 ABI。Windows x64 完整构建
成功；`tests/statistics/basic.tx` 与 `examples/statistics_summary.tx` 编译运行
通过，覆盖空/单样本、极端有限均值、非有限元素、重复与带符号零、迭代器
消耗、别名与深复制、失败后状态以及所有迭代器重载。未运行全量或非
Windows 平台验收。

## 6.6 十进制定点数

`decimal.txh` 公开不可直接构造、字段私有的 `decimal` 类，对外是不变值。
内部用带符号十进制
系数与 0～18 位小数位表示值，公开精度上限为 38 位有效数字；运算使用
足以保存两个最大系数乘积的中间整数，再检查最终结果范围。普通赋值
与 `deep_copy` 都得到相同的不可变数值。

| 新增函数 | 结果与规则 |
| --- | --- |
| `parse(text: str) -> decimal`、`from_int(value: int) -> decimal` | 文本仅接受 ASCII `[-+]?[0-9]+(.[0-9]+)?`，不读取 locale；保留显式小数位及末尾零。超出 38 位有效数字或 18 位小数报 `parse_error/out_of_range`，其他非法文本报 `parse_error/invalid_syntax`。 |
| `to_text(value: decimal) -> str`、`scale(value: decimal) -> int` | 按保存的小数位输出固定十进制，不改用二进制浮点或科学记数法。 |
| `quantize(value, scale: int, mode: str) -> decimal` | 将结果调整到指定 0～18 位小数；不足补零，缩小时按模式舍入。 |
| `add/sub/mul/div(left: decimal, right: decimal, scale: int, mode: str) -> decimal` | 在指定目标小数位交付结果；加减、乘除均用十进制整数中间结果，最终统一舍入和范围检查。除数为零报 `runtime_error/division_by_zero`。 |
| `compare(left: decimal, right: decimal) -> int` | 忽略表示用小数位的差异，精确返回 -1、0、1。 |

舍入模式统一为 `toward_zero`、`floor`、`ceiling`、`half_even`、
`half_away`；前两者分别向零和负无穷，`ceiling` 向正无穷，两个半值
模式分别取偶数末位与远离零。目标小数位或模式无效报
`runtime_error/invalid_argument`，结果超出 38 位有效数字报
`runtime_error/out_of_range`。不提供从 `float` 的构造、隐式转换或
混合算术重载；金额必须从十进制文本或整数创建。文本中的负零规范化为
正零，同时保留显式小数位，例如 `-0.00` 输出 `0.00`。

**实施记录：** `decimal_big.cpp` 用十进制数位实现精确加减乘除，两个最大
系数的乘积保留完整中间值；`decimal.cpp` 处理目标小数位与舍入，
`decimal_abi.cpp` 负责私有类句柄和直接 ABI。Windows x64 完整构建成功；
`tests/decimal/arithmetic.tx` 与 `examples/decimal_money.tx` 编译运行通过，
覆盖 `0.1+0.2`、正负半值、跨精度四则运算、中间大数、除零、极限精度、
无效文本/模式和超范围输入。`float_rejected.tx` 与
`private_rejected.tx` 在源码位置分别拒绝二进制浮点赋值和私有字段访问。
未运行全量或非 Windows 平台验收。
