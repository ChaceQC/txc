# 完整版续接实施记录

## 2026-10-03 本轮：P6/P7 命令与系统集成

开始实现前，按用户要求将此前续接提交为 `37370b6`
（`feat(native-gui): add shaping, containers, data views and interaction`）。
本轮在此基线上增加共享命令、自绘菜单/上下文菜单、工具栏/状态栏、模态 owner、
自绘消息框、Windows Shell/Linux Portal 文件选择、system/high_contrast、字号和应用缩放，
以及 Windows UIA / Linux AT-SPI 的公共语义与操作桥接。具体接口见 [系统集成契约](native_gui_system.md)。

实现使用直接静态 C ABI；普通控件和菜单继续自绘。命令共用状态，快捷键冲突明确拒绝，
菜单取消不会穿透点击，打开菜单时暂停编辑器输入法。模态恢复 owner 和逻辑焦点。
辅助访问通过 UI 线程处理，密码原文不公开，关闭节点与替换后的 combo 项旧引用失效。
没有辅助客户端访问时不持续构建完整语义快照；协议模块按属性、组件、选择和文本职责拆分。

| 最终定向验证 | Windows x64 | Ubuntu 24.04 x86_64 |
| --- | --- | --- |
| 普通静态库和 ThinLTO 工具包构建 | 通过 | 通过 |
| 命令共享/冲突/禁用、菜单键盘操作/取消、模态阻塞与恢复 | 通过 | 通过 |
| 高对比主题、字号/缩放、语义值/范围/选区/密码隔离与旧引用失效 | 通过 | 通过 |
| `.tx` 直接调用 IR、普通和 ThinLTO ABI | 通过 | 通过 |
| 系统辅助客户端遍历与实际操作 | UIA Invoke/Value/Text/Range 通过 | AT-SPI Registry/Action/EditableText/Text/Value 通过 |
| 自绘消息框默认动作、系统文件取消和 owner 恢复 | 通过 | 消息/owner 公共逻辑已覆盖，未执行真人桌面操作 |
| Portal 选择/取消/错误与中文 URI 解码 | 不适用 | 独立 D-Bus 对端通过 |
| 同源 system_workbench 可执行文件 | 已生成 | 已生成 |
| 真人读屏、真实 Linux Portal backend、真实中文输入法 | 未执行 | 未执行 |

已实际查看高对比度渲染图 `tx_build/native_gui/system_windows.bmp`。
未重跑 Unicode 官方用例、字体整形全量检查或无关标准库测试。

同源示例产物 SHA-256：

- `system_workbench.exe`：`0f5e36f162d0c14aa17d001e77903720bc705dd87358ec2142059fe1b4c08915`
- `system_workbench_linux`：`6d2412ff35b97137ce5bfedafbb004f895192f32745ae8fc04750d6877543ea5`

本节记录本地构建与协议验证。源码提交后的 GitHub CI 结果以对应提交的 Actions 运行记录为准。
构建与协议验证不等同于完整 GUI、人工验收或正式发行完成；
其余字体、渲染、平台与发行工作见 [剩余工作清单](native_gui_remaining.md)。

确认双平台产物后，尝试使用原生 PowerShell 清理精确限定的 `build/` 路径，
已先核对工作区绝对路径、目录不是重解析点及交付产物非空；自动审批仍返回
`blocked by policy`，没有提供更具体原因。本轮未绕过限制，临时构建目录保留。

## 上轮续接：OpenType 整形

本轮按尚缺的 R1/R2 继续，保留此前容器、数据和双向编辑实现。已实现字体表驱动的
GSUB 替换、GPOS 定位与 GDEF 分类，并接入水平文字布局及阿拉伯连接状态。
测量与绘制必须使用相同整形结果；连字仍保留原始字素的编辑位置，软换行后重新整形。
字体偏移、递归查找和字形膨胀均需有边界。实现依据为 OpenType GSUB/GPOS/GDEF
及 Common Table Formats，Unicode 连接属性固定 16.0。

验证以小型人工构造的字体表、真实 TrueType 字体和现有布局定向检查为准。
此段是实施范围，完成证据在文末记录；不将本轮整形推进等同于整个完整版完成。

本次需求包括双向文本、复杂脚本整形、字体回退、滚动与数据控件、表格、页签、对话框、
辅助功能和真实中文输入法验收。范围保持 `native_gui_full_plan.md` 的自研、双平台约束。

实施顺序与验收边界：

1. R2：Unicode 16.0 UAX #9 的段落级别、显式嵌入/隔离、弱类型、括号、中性类型和行重排。
   保留逻辑标量索引，视觉顺序只用于布局；官方 BidiTest/BidiCharacterTest 与布局检查分开。
2. R1/R2：按字素覆盖选择字体，测量与绘制共用同一字体；OpenType GSUB/GPOS/GDEF
   与脚本特有处理分开实现，不能将 cmap 映射或阿拉伯表现形式替换称作完整整形。
3. P4/P5：滚动容器、页签、分隔器与稳定行 ID 的 list/table/tree；裁剪、虚拟可见行、
   键盘焦点、选择和模型修订共同验证。
4. P6：自绘对话框、owner 模态与焦点恢复、默认/取消动作；文件选择接平台协议。
5. P7：共享语义树连接 Windows UIA / Linux AT-SPI，验证实际遍历与操作。
6. P0/P3/P8：在真实中文输入法服务上验证组合、候选定位、取消、切换焦点和提交；
   协议模拟、自动检查与用户人工验收分别记录，未执行的不标记通过。

研究补充（2026-10-02）：通过 SciSpace 阅读 MtScript（10.1023/A:1000611103672）与
When Fonts Do Not Know Everything（10.1002/spe.819）的摘要，借鉴逻辑文本与视觉布局分离、
字体规则与脚本知识分离的设计。本次没有取得这两篇论文全文，不能声称复现全文算法。
双向算法直接核对 [UAX #9 revision 50](https://www.unicode.org/reports/tr9/tr9-50.html)
的规则，固定 Unicode 16.0 数据；不能以字符串倒序替代双向算法。

## 已实际落地

- 自研双向核心、Unicode 16.0 属性/括号/镜像数据生成器及官方测试文件。
- 按段落解析、按软换行重排；RTL 命中/光标逻辑索引与非连续视觉选区。
- 字素级字体回退；自动候选使用常见平台字体路径，布局保留字体所有权，测量和绘制一致。
- `examples/native_gui/bidi_text.tx` 及 Windows 普通/ThinLTO 可执行示例。

| 验证 | Windows | Ubuntu 24.04 |
| --- | --- | --- |
| UAX #9 BidiTest（方向展开）770,241 实例 | 通过 | 通过 |
| BidiCharacterTest 91,707 条 | 通过 | 通过 |
| 断行、RTL 光标/命中、分段选区、字素回退及实际绘制 | 通过 | 通过 |
| 普通静态库与 ThinLTO 工具包构建 | 通过 | 通过 |
| `.tx` 静态 ABI、保留控件、布局与生命周期 | 通过 | 通过 |

图像 `tx_build/native_gui/bidi_text.bmp` 已查看，SHA-256 为
`df22779259a2ead79ae1bb13bc7a74788dcf38607c7717bb3f537651893a76bb`。
该图像验证显示内容，不代替真实输入法或读屏操作验收。

## 尚未交付

当前仍未完成全部需求：其他复杂脚本特有重排与完整规范化、完整字体发现与字体格式/hinting、
combo、访问键、默认/取消动作、共享命令、菜单/对话框和 UIA/AT-SPI 仍需实现。
系统/高对比主题、Wayland、完整键盘布局以及图像采样/脏区提交也尚未完成。
水平 GSUB/GPOS/GDEF 和阿拉伯连接已在本轮接入，具体边界见 [整形契约](native_gui_shaping.md)。
当前 WSL 会话未发现运行中的 IBus/Fcitx 服务；真实中文输入法与人工桌面验收未执行。

Linux 打包首次误用默认 `tx/` 路径，在查找该路径下的 `txc` 时失败；随后已用
`--output tx/linux` 完成正确构建，并重新构建 Windows 工具包恢复同名链接文件。
尝试删除误放的新文件被自动审批以 `blocked by policy` 拒绝，未给出更详细原因，未绕过。
待清理的新增路径为 `tx/lib/`、`tx/licenses/`、`tx/clang`、`tx/link/crt1.o`、
`tx/link/crti.o`、`tx/link/crtn.o`、`tx/link/ld.lld`。这些是额外 Linux 文件，
Windows 使用恢复后的 `.exe` 链接器及 Windows 对象；本次不是干净发行目录验收。
增量构建目录 `build/` 也保留。不要将当前工具包状态标记为正式发行完成。

## 2026-10-02 后续实现：视觉编辑、容器与数据视图

本节继续以上工作区，不撤回上一轮双向算法和字体回退改动。

1. `text_caret.cpp` 保存带 upstream/downstream 亲和性的视觉停靠点。编辑器、鼠标命中、
   Home/End、上下移动、选区折叠、辅助光标与 IME 候选位置使用同一位置；密码导航保持原始字素边界。
2. `containers*.cpp` 与 `container_*.cpp` 实现自绘滚动条、嵌套滚轮传递、焦点滚入、页签、
   分隔器与画布。页 ID 使用面板 ID，关闭分隔器首面板不改变另一个面板的身份。
3. `data_model.cpp/data_columns.cpp` 与数据视图模块实现整批校验后提交、生命周期稳定 ID、
   树父关系检查、独立列顺序、选择、排序请求、展开和分页修订隔离。十万行数据不创建行控件。
4. 新能力通过直接 C ABI 导出，新增资源有独立静态类型。事件追加整数 item_id/column_id、
   x/y/modifiers 字段；现有字段含义保留。没有引入外部 GUI、字体、排版或渲染实现。
5. `scripts/build_linux.py` 的默认输出改为 `tx/linux`，防止重复覆盖 Windows 工具包目录。

验证入口：

```text
python scripts/check_native_gui_text.py --only layout
python scripts/check_native_gui_extended.py --font <TrueType 字体路径>
```

第二个脚本可用 `--only behavior` 或 `--only abi` 定向复核，避免改了示例后重复运行核心检查。
Windows 与 Ubuntu 24.04 已完成普通/ThinLTO 构建及首轮核心/静态 ABI 检查；Linux 交互检查
先映射系统窗口再申请焦点，符合 X11 要求。此处的输入事件由检查程序发出，不是人工操作验收。
后续边界修正与最终产物检查另在本节末记录。

新增源码示例：`examples/native_gui/data_workbench.tx`。Windows 与 Linux 同源可执行文件分别为
`tx_build/native_gui/data_workbench.exe`、`tx_build/native_gui/data_workbench_linux`。
这是一轮容器与数据工作台交付，不能把尚未实现的整形、对话框、辅助技术或真实输入法验收标为完成。

### 本轮最终验证与交付记录

| 检查 | Windows x64 | Ubuntu 24.04 x86_64 |
| --- | --- | --- |
| 视觉停靠点、亲和性、软换行、字素及基础字体回退布局 | 通过 | 通过 |
| 滚动条、嵌套滚轮传递、焦点滚入、页签隐藏和分隔捕获取消 | 通过 | 通过 |
| 批量失败原子性、ID 复用拒绝、分页过期拒绝、树关系 | 通过 | 通过 |
| 十万行表格、祖先裁剪、可见行缓存、Shift 选择与排序请求 | 通过 | 通过 |
| 树展开/收起、键盘父子导航和选区身份保留 | 通过 | 通过 |
| 新增容器/数据/画布的普通静态 ABI 与 ThinLTO ABI | 通过 | 通过 |
| 同源 data_workbench 可执行文件 | 已生成 | 已生成 |
| 真实中文输入法、DPI/布局人工操作及读屏验收 | 未执行 | 未执行 |

最终示例 SHA-256：

- Windows：`0b1d192c4f4ba72680ceca647a51e051df6e9b033b298275482c13940b6a6cf3`
- Linux：`bf073c7ec5beddcb9d76d1fbd032298280c27b51cdfbca914962a93dafe5dc45`

`extended_windows.bmp`、`extended_linux.bmp` 和 `extended_abi.bmp` 已实际查看，
表格、选区、滚动条、页签、分隔器、透明色合成和画布内容可见。两平台表格图像哈希不同，
本轮没有将它们标为确定性像素一致性验收；窗口焦点与字体集合也未统一为像素对照夹具。

清理状态：再次使用原生 PowerShell 对上述明确列出的额外 Linux 路径执行清理，
命令先验证仓库绝对路径、目标不是重解析点以及对应双平台产物存在，但仍被自动审批以
`blocked by policy` 拒绝，未提供进一步原因。没有换工具绕过；七个额外路径仍存在，
`build/` 的后续清理未继续。源码和本轮可执行文件已交付，当前目录不能标为干净正式发行。

## 2026-10-02 本轮最终记录：OpenType 水平整形

本轮实际接入自研 GSUB/GPOS/GDEF，覆盖字形替换、连字、上下文查找、字距、连写附着、
附标定位、分类/过滤和连字光标设计坐标。Unicode 16.0 脚本/连接数据经过固定 SHA-256 校验，
阿拉伯初始/中间/结束形态来自字体 GSUB，ZWJ/ZWNJ 参与连接控制。
Unicode Hiragana/Katakana 均映射到 OpenType kana，避免按 Unicode 别名直接生成错误脚本标签。

公共布局按字体/脚本/方向分段整形，测量与绘制共用结果；软换行后重新整形，连字内部仍能
逐原始字素移动光标与选择。没有改动 UAX #9/#14/#29 核心算法，也没有运行它们的全量用例。
现有容器、数据和视觉编辑实现保留。具体表格式与限制见 [整形契约](native_gui_shaping.md)。

| 定向检查 | Windows x64 | Ubuntu 24.04 x86_64 |
| --- | --- | --- |
| 人工字体表：GSUB/GPOS、GDEF 分类/过滤/光标 | 通过 | 通过 |
| 截断表、输出字形越界、上下文递归和 2048 字形连接链 | 通过 | 通过 |
| 实际字体：阿拉伯连字/连接/附标、ZWNJ、换行重整形 | 通过 | 通过 |
| 既有断行/双向/光标/选区/字素回退布局检查 | 通过 | 通过 |
| 普通静态库和 ThinLTO 工具包构建 | 通过 | 通过 |
| 同源 .tx 普通/ThinLTO ABI 渲染，两种模式图像相同 | 通过 | 通过 |
| 同源 shaping_workbench 交互程序构建 | 已生成 | 已生成 |
| 真实输入法、读屏和人工桌面验收 | 未执行 | 未执行 |

最终整形核心使用同一份 Arial 字体，shaping_windows.bmp 与 shaping_linux.bmp 的 SHA-256 均为：

519940efec7fcb61a210fed154ec0a2df662dbfbbe1c059327752a62307327b3

实际查看了核心渲染、.tx 标签/编辑器输出和 PNG 预览。像素一致性只适用于这份固定字体夹具，
不能推广为所有系统字体、不同桌面主题或真实输入法的验收。

源码：examples/native_gui/shaping_workbench.tx。可执行文件及 SHA-256：

- tx_build/native_gui/shaping_workbench.exe：dc5d56bf31ad7443a6db733e942c3f834c50417a124640322e1c3d6f787417b3
- tx_build/native_gui/shaping_workbench_linux：68e4eda676255531d33ed3caf83c01ac45a5fe71095172eb439356d4a768a569

定向入口：scripts/check_native_gui_text.py 的 --only shaping/--only layout，及
scripts/check_native_gui_shaping.py 的普通/ThinLTO ABI 与示例构建检查。Linux 整形检查需提供
--shaping-font；ABI 检查使用 --font。

**完整版仍未全部完成。** 尚缺其他脚本专门重排/规范化、完整字体发现和字体格式/hinting、
combo/访问键/默认取消动作、共享命令/菜单/对话框、system/high contrast、UIA/AT-SPI、
Wayland/完整键盘布局、图像采样/脏区提交，以及真实输入法和人工桌面验收。
非默认可变字体、竖排和完整语言/特性选择也未交付。此前记录的额外 Linux 打包文件及
增量 build 目录仍保留；本轮没有执行清理，也没有提交、推送或标记正式发行。

## 2026-10-03 续接：P2 基础交互

本轮沿用前述 OpenType、双向编辑、容器和数据视图改动，补齐 P2 的三个剩余实现项：

- 自绘只读 combo：原子替换选项、程序选择、候选导航、弹层滚动与可见行绘制、窗口边界内向上/向下展开。
- 访问键：显式 Alt+ASCII 字母/数字，重复键循环焦点，隐藏/禁用节点过滤，不解析原文字中的 `&`。
- 窗口默认/取消按钮：弱引用绑定，保留单行 `text_committed`、多行换行与输入法组合隔离，重复键不重复激活。

弹层取消不提交候选；外部点击不穿透，失焦/捕获丢失/隐藏/关闭清理弹层。
鼠标按住候选时按 Escape 也必须释放捕获，随后释放鼠标不能重新展开。
Windows/X11 补充 F4 和数字键映射。无新 GUI、字体、渲染库依赖。

接口与行为边界：[基础交互契约](native_gui_interaction.md)。
源码示例：`examples/native_gui/interaction_workbench.tx`。
定向入口：`scripts/check_native_gui_interaction.py --font <字体>`，支持 `--only behavior|abi`。
本段实现不等于完整版完成；最终平台结果在本节验证表记录。

尚余：其他复杂脚本特有整形/规范化、完整字体发现/格式/hinting、共享命令/菜单/模态及文件对话框、
system/high contrast、UIA/AT-SPI、原生 Wayland/完整键盘布局、图像采样/脏区提交、
真实输入法与人工桌面验收，以及干净正式发行。此前记录的额外 Linux 打包路径尚未清理。

### 本轮最终验证

| 检查 | Windows x64 | Ubuntu 24.04 x86_64 |
| --- | --- | --- |
| combo 候选/提交/取消/滚动命中、点击不穿透、捕获清理 | 通过 | 通过 |
| 访问键重复抑制/冲突循环、默认/取消动作、组合状态隔离 | 通过 | 通过 |
| 普通静态库与 ThinLTO 工具包构建 | 通过 | 通过 |
| `.tx` 普通/ThinLTO ABI、越界错误与直接调用 IR 检查 | 通过 | 通过 |
| 最终同源 interaction_workbench 可执行文件 | 已生成 | 已生成 |
| 既有 extended_behavior 容器/数据/视觉编辑定向回归 | 通过 | 本轮未重跑 |
| 显式中文 TrueType 字体的实际绘制检查 | 通过 | 通过 |
| 真实输入法、读屏与人工桌面验收 | 未执行 | 未执行 |

Arial 夹具的 Windows/Linux 图像不同；Linux 自动回退集合缺少可用中文字体，初次结果出现缺字符号。
随后指定 `msyh.ttc`，运行最终行为程序并查看 `interaction_cjk_linux.bmp`，中文正常。
此结果只确认指定字体路径有效，不将完整字体发现或 CFF/hinting 标记为完成。

最终工作台 SHA-256：

- Windows：`92bd73511b7a3c5487efc2dc71785da57351dd8813ce71942317f3e850994560`
- Linux：`a5317dec0bc4d1f070d566343765e1cc82a1639dd836daa408445d648c6525d0`

已执行 `git diff --check`；本轮没有提交或推送。增量 build 目录与先前额外打包路径保留，
当前仍是开发工具包交付，不是完整版完成或干净正式发行。
