# Unicode、正则与增量编码

第五部分的 Unicode 文本以有效 UTF-8 `str` 为输入。公开位置明确区分 UTF-8 字节、Unicode 标量和字素簇；现有 `len(str)`、`string.find/slice` 的单位仍是 Unicode 标量，`len(bytes)` 仍是字节。所有新增接口拒绝无效 UTF-8，不按宿主机默认 locale 改变结果。

## Unicode 接口

`unicode.txh` 使用随 Windows x64 工具链固定交付的 ICU4C 78.3 DLL 和数据。`version()` 返回数据中的 Unicode 版本，`icu_version()` 返回 ICU 版本。`string.lower/upper` 继续只转换 ASCII；完整映射需显式调用下面的新函数。

| 接口 | 语义 |
| --- | --- |
| `normalize(text: str, form: str) -> str` | `form` 只接受 `NFC`、`NFD`、`NFKC`、`NFKD`；返回规范化的新文本 |
| `case_fold(text: str) -> str` | Unicode 默认完整 case fold，不依赖 locale |
| `to_lower(text: str, locale: str) -> str`、`to_upper(text: str, locale: str) -> str` | 完整大小写映射；locale 使用显式 BCP 47 标签，`und` 表示根区域设置 |
| `code_points(text: str) -> vector<int>` | 逐个 Unicode 标量返回码点，顺序与 `string.slice` 的标量位置相同 |
| `graphemes(text: str) -> vector<str>`、`grapheme_count(text: str) -> int` | ICU 字符边界规则切分与计数扩展字素簇；组合音标和 ZWJ 序列可占一个字素簇 |
| `category(code_point: int) -> str` | 返回 Unicode 通用类别的二字母简称，例如 `Lu`、`Mn`；代理项和超出 U+10FFFF 的值无效 |
| `display_width(text: str) -> int` | 单行终端显示列数：组合/格式字符 0 列，东亚宽/全宽字符及 emoji 呈现簇 2 列，其余 1 列；控制字符与换行报错；东亚模糊宽度按 1 列 |
| `compare(left: str, right: str, locale: str, strength: str) -> int` | 显式 ICU locale 比较器，结果为 -1、0、1；`strength` 为 `primary`、`secondary`、`tertiary` 或 `identical`，开启规范化；不会改变语言内置 `<` 或容器默认排序 |

未知规范化形式、locale、比较强度及码点分别返回 `parse_error` 的 `invalid_form`、`invalid_locale`、`invalid_strength`、`invalid_code_point`；无效文本返回 `invalid_unicode`。输入或输出超出 ICU 32 位长度及标准库分配上限返回 `runtime_error` / `size_limit`。字素切分和比较器依据固定数据版本；不同 Unicode 版本的边界或排序结果可能不同。

## 正则接口

`regex.txh` 基于固定 PCRE2 10.48 的 8 位模式。所有模式强制启用 UTF 与 Unicode 字符属性；`flags` 可由 `i`（不区分大小写）、`m`（多行锚点）、`s`（点号包含换行）、`x`（忽略模式空白）组成，每个字母最多一次。模式最多 1 MiB，语法错误返回 `parse_error` / `invalid_regex`，中文信息包含**模式 UTF-8 字节偏移**。`compile` 返回可共享的不可变 `regex_pattern`；`compile_with_cancel` 额外绑定 `cancel_token`，执行时持续检查该令牌。

| 接口 | 语义 |
| --- | --- |
| `compile(pattern, flags, max_input_bytes, match_limit, depth_limit) -> regex_pattern` | 编译带限额的模式 |
| `compile_with_cancel(pattern, flags, max_input_bytes, match_limit, depth_limit, token) -> regex_pattern` | 编译并绑定取消令牌 |
| `search(pattern, text, start_byte) -> regex_match` | 从 `start_byte` 起找第一个匹配；位置须位于 UTF-8 标量边界 |
| `match(pattern, text) -> regex_match`、`full_match(pattern, text) -> regex_match` | 分别要求从文本开头匹配、整段文本匹配 |
| `find_all(pattern, text) -> vector<str>` | 按顺序返回所有完整匹配文本；需要逐次捕获时可用 `search` 从上次字节终点继续搜索，若上次为零长度匹配，调用方须先推进一个 Unicode 标量 |
| `replace(pattern, text, replacement) -> str` | 全局替换；模板支持 `$$`、`$0`～`$99`、`${name}`，其他 `$` 用法报 `parse_error` / `invalid_replacement` |
| `split(pattern, text) -> vector<str>` | 按匹配边界拆分；保留首尾空片段，不额外插入捕获组 |

`regex_match.found=false` 表示未命中，此时文本和捕获向量为空，四个位置为 -1。命中时 `text` 是完整匹配；`start_byte/end_byte` 是相对原文本的 UTF-8 字节半开区间，`start_scalar/end_scalar` 是 Unicode 标量半开区间，与 `string.slice` 的位置单位一致。`groups` 包含第 0 组完整匹配及所有捕获组；`group_names` 与 `groups` 对齐，未命名组用空串；`group_start_bytes/group_end_bytes` 同样对齐，未参与匹配的捕获组以 -1 标记，其 `groups` 文本为空。字素簇不作为正则位置单位；需要字素匹配可使用 PCRE2 的 `\X`。

查找、拆分和替换遇到零长度匹配时，先在同一字节位置尝试非空锚定匹配；若没有，则推进一个完整 UTF-8 标量。这样不会停在同一位置，也不会切开多字节字符。`replace` 在匹配区间之外保留原文本，捕获组替换采用当前匹配的快照；未参与匹配的组展开为空串。

## 正则资源与取消限制

编译时必须指定 `max_input_bytes`（1～32 MiB）、`match_limit`（1～10,000,000 个 PCRE2 匹配步骤）和 `depth_limit`（1～100,000）；捕获组最多 256 个。执行前检查输入 UTF-8 字节数；执行时设置 PCRE2 匹配与深度限额及 32 MiB 堆限额。`find_all/split` 最多产生 100,000 个匹配/片段，`replace` 的输出最多 32 MiB。批量查找和拆分不重复计算未公开的标量位置或构造捕获组。超出输入或输出大小返回 `runtime_error` / `size_limit`，超出捕获、匹配、深度、堆或结果数返回 `runtime_error` / `regex_limit`；不可信模式不会无限回溯。取消令牌在执行前及 PCRE2 自动 callout 中检查；主动取消和截止时间返回 `cancelled_error` / `cancelled`，部分生成的替换或拆分结果不会返回。
