# 自研 Unicode 属性数据

版本固定为 Unicode 16.0.0。属性数据来自 Unicode 官方发布；边界算法由本项目实现。
按 Unicode 数据文件许可分发，见 https://www.unicode.org/license.txt 。
许可副本位于 `src/stdlib/native_gui/core/unicode_license.txt`。

| 文件 | SHA-256 |
| --- | --- |
| [auxiliary/GraphemeBreakProperty.txt](https://www.unicode.org/Public/16.0.0/ucd/auxiliary/GraphemeBreakProperty.txt) | `c29360bd6f7132811d701d29069541e827eb44bfc4c8fbde8c370d6982689dc1` |
| [emoji/emoji-data.txt](https://www.unicode.org/Public/16.0.0/ucd/emoji/emoji-data.txt) | `f1365a5173eee18e1f98b240cdc492e84a25f1ce7e0c9d1094eb29c41a22696a` |
| [DerivedCoreProperties.txt](https://www.unicode.org/Public/16.0.0/ucd/DerivedCoreProperties.txt) | `39d35161f2954497f69e08bdb9e701493f476a3d30222de20028feda36c1dabd` |

`unicode_tables.inc` 由上述文件的属性区间排序并合并相邻同属性区间得到。
未列出的字素属性使用 Other，InCB 使用 None，Extended_Pictographic 使用 No。
这只是属性表与边界算法的基础，不代表整形、双向排版或编辑器已经完成。

官方 `GraphemeBreakTest.txt` 保存在 `tests/native_gui/data/grapheme_break_test.txt`，
SHA-256 为 `ee2b9354d270ac061b29f09662cafea06341d77e704b8cc6bd72aaeeda363cb5`。

## 换行实现契约

续接工作按 Unicode 16.0 的 [UAX #14 revision 53](https://www.unicode.org/reports/tr14/tr14-53.html)
实现默认断行机会。输入和结果都使用 Unicode 标量索引；分别表示禁止、允许和强制断行。
LB1 使用默认属性解析，SA 中 Mn/Mc 解析为 CM，其余 SA 解析为 AL；不声称实现泰语词典分词。
LB9 跳过附着的 CM/ZWJ 时仍保留原始索引，后续上下文规则按有效字符匹配。

布局采用贪心选择行宽以内最后一个合法机会；无机会且单词本身超宽时才按字素紧急折行。
普通软换行不切开字素，保留全部文本索引及选区；CRLF 视为一次强制换行，BK/CR/LF/NL
全部遵循强制换行。此策略不是 Knuth–Plass 段落全局优化，也不代表双向或复杂脚本整形已完成。

编辑缓冲将所有强制换行归一化为 LF，单行控件过滤这些换行与 Tab；布局本身仍支持原始 CRLF。
行尾普通空格和 Tab 保留在原行，避免自动换行产生只含这些空格的新行。

属性生成器为 `scripts/generate_gui_line_break.mjs`，固定 SHA-256 后生成 `line_break_tables.inc`；
官方用例保存在 `tests/native_gui/data/line_break_test.txt`。数据许可随 Windows/Linux 工具包分发。

| Unicode 16.0 源文件 | SHA-256 |
| --- | --- |
| LineBreak.txt | `e97e4259d0d20fab150b9c7b4b28abfae5cd78ca97e7f4ac6ed20d685d5f4a7c` |
| EastAsianWidth.txt | `43adc76c0686a42cb370764eb8cfe2b2a45b10b855e5572a2db4a0eecce15d5b` |
| extracted/DerivedGeneralCategory.txt | `7676ab755a41ef82108460238569e60ad65c191ddafe61b36c6765ec1353f293` |
| auxiliary/LineBreakTest.txt | `910759a611a479f37df4f535d2e64d7589be3c5ac5f7491cf1fb4fa4cdb211e9` |

`emoji/emoji-data.txt` 与字素模块使用同一份已列出的固定数据。SA 按默认规则处理，不含词典分词。
Windows 与 Ubuntu 24.04 均通过全部 16,672 条官方断行用例；另有英文单词、中文标点、
CRLF/段落分隔符、字素紧急折行、光标命中和跨行选区的布局检查。
