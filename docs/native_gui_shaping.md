# 自研 OpenType 水平整形

本模块接入公共文字布局，普通标签、按钮、编辑器及数据单元格均使用同一条整形路径。
代码位于 src/stdlib/native_gui/core/ 下的 opentype_*.cpp、shaping*.cpp 与 text_shaping.cpp。
不调用操作系统排版服务，也不链接外部字体或整形实现。

## 已实现的表行为

| 数据 | 支持内容 |
| --- | --- |
| 公共表 | ScriptList/默认或指定 LangSys、required feature、FeatureList、LookupList；Coverage 1/2、ClassDef 1/2 |
| GSUB | Single 1/2、Multiple、Alternate（默认第一个）、Ligature、Context 1/2/3、ChainedContext 1/2/3、Extension、ReverseChainSingle |
| GPOS | Single 1/2、Pair 1/2、Cursive、MarkToBase、MarkToLigature、MarkToMark、Context/ChainedContext 1/2/3、Extension |
| GDEF | 基字形/连字/附标分类、MarkAttachClassDef、MarkGlyphSetsDef；连字光标设计坐标 |
| LookupFlag | IgnoreBase/IgnoreLigatures/IgnoreMarks、MarkAttachmentType、MarkFilteringSet；Cursive 的 RightToLeft |
| 定位值 | x/y placement、水平 advance、Anchor 1/2/3 设计坐标、整像素字号下 Device 1/2/3 |

GSUB 特性阶段为 ccmp/locl、阿拉伯 isol/fina/medi/init、rlig/calt/liga/clig。
GPOS 使用 curs/kern/dist/abvm/blwm/mark/mkmk。每个特性内部按 LookupList 索引排序，
从对应语言系统选择已注册的特性；嵌套查找直接使用同一表的 Lookup 索引。
Alternate 默认选择第一个候选，目前没有公开用户选择 alternate/语言/特性开关的 .tx 接口。

上下文动作的 sequenceIndex 相对于前面动作已经修改的输入序列。替换输出保留逻辑标量
区间；跨附标连字保留组件来源，供 MarkToLigature 确定附着组件。反向链式替换从末尾扫描。
PairSet 内 Device 偏移以 PairSet 为基准，其余 ValueRecord 按规范使用自己的直接父表。
Cursive 先记录附着关系，再在一次查找结束后线性解析，拒绝附着循环。

## 脚本与布局

Unicode 16.0 的 Script 与 Joining_Type 由固定哈希的官方数据生成。阿拉伯连接状态区分
初始、中间、结束和独立形态，透明附标不阻断连接，ZWJ 引发连接，ZWNJ 阻断连接和连字。
字形选择来自真实 GSUB，未使用 Arabic Presentation Forms 的字符替换。

按字素选择字体，再按字体、脚本和双向级别划分连续整形段。先用整形 advance 估计行宽，
软换行后重新执行该行的整形，必要时收缩行尾重新断行，避免跨行保留连字或错误连接形态。
测量和绘制共用结果；原始逻辑文本、字素删除与选区接口保持不变。

连字覆盖的原始字素仍各有光标位置。可用的 GDEF 连字光标设计坐标优先；缺失、组件数量
不匹配、坐标不适合当前 advance 或轮廓点型光标使用均分 advance。RTL 按视觉坐标映射回
逻辑字素。无附标查找时保留基础定位，不能把这种降级效果当作正确附标定位。

## 明确边界

- 已接入基础水平 OpenType 和阿拉伯连接。印度系音节重排、缅甸文、高棉文、藏文、
  Hangul Jamo 等脚本的专门处理与完整规范化仍待实现；仅有字体查找引擎不等于全部脚本完成。
- 非默认可变字体实例、FeatureVariations、竖排、用户选择 alternate、完整语言选择接口未交付。
  VariationIndex 在默认实例使用零变化；本轮不支持非默认坐标。
- 字体仍使用 TrueType/TTC glyf 轮廓；CFF/CFF2、彩色字形及 hinting 未实现。
  Anchor format 2 使用未拟合的设计坐标；GDEF 光标不应用像素网格微调。
- 输入和输出字形数量上限一百万；上下文嵌套最多 16 层，操作预算最多一千万。
  所有被访问的表偏移/长度、Lookup/Feature/覆盖索引和输出字形均检查边界。
  这不是对整个字体每个未使用表的完整验证器。
- 本轮没有更改 UAX #9/#14/#29 算法，也没有重跑其全部官方用例。
  真实输入法、读屏与人工桌面验收仍是独立工作。

## 定向验证

~~~powershell
python scripts/check_native_gui_text.py --only shaping --shaping-font C:/Windows/Fonts/arial.ttf
python scripts/check_native_gui_text.py --only layout
~~~

~~~sh
python3 scripts/check_native_gui_text.py --only shaping --shaping-font /path/to/arial.ttf
python3 scripts/check_native_gui_text.py --only layout --font /path/to/msyh.ttc --fallback-font /path/to/segoeui.ttf
~~~

二进制表夹具自行构造，检查替换、上下文序列修改、分类/过滤、定位、光标、截断数据、
递归限制与长 Cursive 链。真实字体检查 Arabic 连字/连接/附标、ZWNJ、整形行宽、
连字内部视觉光标和软换行重整形。输出 shaping_windows.bmp / shaping_linux.bmp。
同源交互示例为 examples/native_gui/shaping_workbench.tx。

具体平台与构建结果以 [续接记录](native_gui_completion.md) 中的本轮最终记录为准。
