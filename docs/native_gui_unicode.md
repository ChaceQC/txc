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

## 双向文本与字体回退（2026-10-02）

自研 `bidi.cpp` / `bidi_sequence.cpp` 实现 Unicode 16.0 UAX #9 revision 50：
P2/P3、显式嵌入/覆盖与隔离、溢出深度、X9/X10、W1–W7、BD16/N0–N2、I1/I2、L1/L2。
属性、括号及镜像映射由 `scripts/generate_gui_bidi.mjs` 从固定哈希数据生成。
Windows 与 Ubuntu 24.04 各通过 770,241 个 BidiTest 方向展开实例，以及
91,707 条 BidiCharacterTest 用例。测试使用官方期望级别和视觉索引，不以内置库作为算法实现。

| Unicode 16.0 文件 | SHA-256 |
| --- | --- |
| extracted/DerivedBidiClass.txt | `71ed943a49c58568d8d92e80ecc2ba2f06e62aee9c8ebb0e6e8bd2c3ed8b180e` |
| BidiBrackets.txt | `b8f32554c6f658821fb0ee742d21c5b1f2086b9bf13071fed04894b022f93d67` |
| BidiMirroring.txt | `d7afdadd1bbd66f5a663ac0e8f7958f18fd9491fc0bc59ec5877cb82db71db7d` |
| BidiTest.txt | `93e5eb9d88ca89dcf895f5576486a3363762ad2aa8f2db2fa56fe60cb82b9520` |
| BidiCharacterTest.txt | `d04a51a90052dcd71c4e91ee5b3a9d973ee35c12406b5a99875ac8163c8f2804` |

布局按段落分析方向，在贪心断行之后逐行执行 L1/L2，并按完整字素移动组合字符（L3）。
奇数级别使用镜像字符映射（L4），测量和绘制选取相同的镜像字符。方向控制符保留逻辑索引但
没有字形和宽度。`hit` 返回逻辑标量位置；RTL 字素的两侧对应相反的逻辑端点。
一个逻辑选区可以产生同一行的多个矩形，仅合并相邻片段，不覆盖中间未选文字。

`font_family` 按候选次序选择覆盖整个字素的字体，布局持有自动加载字体的共享所有权。
没有完整覆盖时保留主字体 `.notdef`；不拆开字素使用不同字体拼接。行高同时容纳集合中
最大上升部和基线以下高度。应用自动尝试平台常见 TrueType/TTC 路径，跳过不存在、损坏
或不支持的候选；显式主字体失败仍报错。相同字体集合才能用于跨平台像素比较。
字体发现目前是有限的常见路径集合，不是完整系统字体目录枚举；不支持 CFF/彩色 emoji。

本轮已接入 GSUB/GPOS/GDEF 和阿拉伯连接，支持范围见 [整形契约](native_gui_shaping.md)。
无适用 GPOS 附标查找时仍使用基础定位；其他复杂脚本处理不能据此标记完成。
视觉左右键与双重光标亲和性已经接入；Ctrl+左右仍采用逻辑词导航。具体策略见下节。
真实输入法和读屏验收仍需单独推进。

定向入口：`python scripts/check_native_gui_text.py --only bidi`；布局与回退使用
`--only layout --font <主字体> --fallback-font <回退字体>`。

## 视觉编辑续接契约

编辑缓冲继续保存逻辑标量索引，显示位置额外保存 upstream（附着前一字素）或 downstream
（附着后一字素）亲和性。同一索引在双向边界或软换行处可以对应两个位置；绘制、鼠标命中、
上下移动和输入法候选定位必须使用同一亲和性。主光标显示当前位置，同一行存在另一个位置时
显示较短的辅助光标；软换行的另一个位置不绘制辅助光标。

未按 Ctrl 的左右键按行内视觉顺序逐字素移动；行边缘进入前/后一行。Home/End 到视觉行首/尾，
Ctrl+Home/End 到文档逻辑首/尾，Ctrl+左右继续按逻辑词边界导航。Shift 保留逻辑选区锚点，
取消选区时落到所选字素的视觉左/右边缘。Backspace/Delete 仍按逻辑字素删除。
零宽方向控制符不额外产生停在同一像素的方向键步骤，逻辑删除和程序选区仍能访问其索引。
密码显示的掩码不得引入原始字素内部的编辑停靠点。以上是新增行为契约，验证结果另行记录。

## 整形属性数据

scripts/generate_gui_shaping.mjs 校验以下官方 Unicode 16.0 文件后生成
shaping_tables.inc。Joining_Type 的默认值是 Non_Joining；透明类型已由官方派生数据确定。
Script=Common/Inherited 由布局段继承相邻脚本。数据沿用 Unicode 许可证。

| 文件 | SHA-256 |
| --- | --- |
| extracted/DerivedJoiningType.txt | 6bd08b97da66b70ccfdab105a352de2984e02625239ec5695422c99b33d854f0 |
| Scripts.txt | 9e88f0a677df47311106340be8ede2ecdacd9c1c931831218d2be6d5508e0039 |
| PropertyValueAliases.txt | 440fd3e5460b9bfe31da67b6f923992e1989d31fe2ed91e091c4b8f8e2620bf9 |
