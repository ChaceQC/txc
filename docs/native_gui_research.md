# 自研 GUI 算法与规范依据

检索日期：2026-10-02。检索使用 SciSpace 学术索引，并核对 Crossref 书目信息和官方规范页面。
这里只借鉴论文算法、数学定义和公开格式/协议，不引入参考项目的实现代码或运行库。

## 渲染

| 来源 | 用途 | 实现与验证要求 |
| --- | --- | --- |
| Porter, Duff, *Compositing Digital Images*, 1984，[DOI](https://doi.org/10.1145/800031.808606) | 预乘 alpha 与合成算子 | 像素使用预乘 BGRA；source-over 为 `C = Cs + Cd × (1 − As)`。检验透明边缘、覆盖率与 alpha，避免重复预乘 |
| Blinn, *Fun with Premultiplied Alpha*, 1996，[DOI](https://doi.org/10.1109/38.536279) | 预乘颜色在处理过程中的一致性 | 图像采样与合成保持同一表示；接口明确颜色空间，不能把 sRGB 数值平均称为线性光合成 |
| Hersch, *Efficient Rendering of Outline Characters*, 1991，[EPFL](https://infoscience.epfl.ch/record/99753) | 字形轮廓细分、扫描转换、跨度填充 | 曲线停止条件必须处理退化与回折，不能只用控制点到无限直线的距离 |
| Hain, *Fast Termination Criterion for Recursive Subdivision of Bézier Curves*, 1999，[DOI](https://doi.org/10.1145/306363.306407) | 自适应曲线展平的停止条件 | 精度以设备像素定义，限制递归与点数；细分误差与光栅采样误差分别验证 |
| Lemoine, Boyer, *Rasterization by Multiresolution Integration*, 2010，[DOI](https://doi.org/10.1109/SITIS.2010.31) | 点采样与面积积分的取舍 | 当前规则子像素采样只能作为基础，不能宣称精确覆盖积分；后续比较质量、内存及耗时 |

当前 CPU 光栅化按有向边交点生成扫描跨度，支持 non-zero 和 even-odd；初始抗锯齿使用
4×4 子像素覆盖率。它不是论文中精确面积积分算法的复现，也不能保证任意细小图元不消失。
在性能优化前保留确定性参考结果，避免优化后只凭截图判断正确。

## 字体与排版

论文用于设计思想，二进制格式和 Unicode 行为以规范为准：

- [OpenType glyf](https://learn.microsoft.com/en-us/typography/opentype/spec/glyf)：简单与复合字形、
  on/off-curve 点、二次 Bézier 轮廓、变换、点对齐与递归。不能只解析简单 ASCII 字形。
- [OpenType cmap](https://learn.microsoft.com/en-us/typography/opentype/spec/cmap)：format 4 用于 BMP，
  format 12 支持补充平面；字符映射与字形轮廓是独立步骤。
- [OpenType 规范](https://learn.microsoft.com/en-us/typography/opentype/spec/)：进一步按 head/maxp/
  hhea/hmtx/loca、GSUB/GPOS/GDEF 的具体表契约实现，所有偏移与长度均按不可信输入检查。
- [UAX #29](https://www.unicode.org/reports/tr29/)：字素与词边界；光标移动和删除不能直接按
  UTF-8 字节或 UTF-16 单元递增。
- [UAX #14](https://www.unicode.org/reports/tr14/)：合法换行机会；CJK 禁则、组合字符与硬换行。
- [UAX #9](https://www.unicode.org/reports/tr9/)：双向文本处理；保留逻辑索引与视觉字形映射，
  不用反转字符串冒充双向排版。
- Knuth, Plass, *Breaking Paragraphs into Lines*, 1981，[DOI](https://doi.org/10.1002/spe.4380111102)：
  段落整体断行优化。编辑器默认断行需优先考虑增量更新与响应时间，不能无条件套用
  段落全局优化；高质量文本显示可单独提供此策略。

实际实现应固定 Unicode 数据版本，并使用相应官方边界/双向/换行测试数据。
读取最新版规范页面仅用于研究，不等于已经通过相应版本的一致性测试。

## 已读证据的范围

平台协议还核对了 X.Org 的 [X11 协议](https://www.x.org/releases/X11R7.7/doc/xproto/x11protocol.html)
与 [Input Method Protocol](https://www.x.org/releases/current/doc/libX11/XIM/xim.html)。
X11 窗口和像素提交已自行编码；XIM 的连接、属性协商、输入上下文、提交与预编辑消息已自行实现。
协议对端验证与真实中文输入法服务验收分开记录，后者仍待完成。

本轮读取了学术索引中的论文摘要、Crossref 的两篇原始论文书目，以及上述官方规范页面。
尚未逐篇阅读全文，不能把摘要中的性能或正确性结论直接套到本项目。
每个模块的验收记录必须标明采用的具体算法、支持范围、未完成项及实际运行证据。

## 2026-10-02 续接核对

- 重新读取 X.Org Input Method Protocol 的传输表 D.3 与 D.4–D.11：实现 0.0、0.1、0.2、
  1.0、2.0、2.1 六种组合，以及分片 ClientMessage、属性追加和两种通知方式。
- 对照 `XIM_SET_EVENT_MASK` 的 IC=0 默认值语义，避免覆盖独立 IC 掩码；对照
  `XIM_FORWARD_EVENT`、`XIM_SYNC_REPLY`、`XIM_RESET_IC` 的同步规则，处理 Tab/鼠标
  交付后才允许后续输入进入新的编辑上下文。取消时丢弃旧提交和 reset 返回的旧预编辑文本。
- Linux 完整窗口检查暴露了旧 `GetKeyboardMapping` 请求的字段错位；重新对照 X11 协议的
  请求编码，修正为第二字节保留、偏移 4/5 分别为 first-keycode/count。协议对端检查与
  实际窗口初始化检查都保留，避免用前者替代后者。
- 重新读取 Unicode 16.0 [UAX #14 revision 53](https://www.unicode.org/reports/tr14/tr14-53.html)
  第 6 节 LB1–LB31 和第 8 节：默认断行使用本版的引号规则、数值规则和 Brahmic 音节规则，
  不混用新版数据。实现已通过本版全部 16,672 条官方用例。
- 文本布局采用合法机会上的贪心断行和字素级紧急折行；没有采用 Knuth–Plass 全局最优断行。
  这次核对的是协议与 Unicode 规范全文相关章节，没有将先前读到的论文摘要冒充全文研究。
