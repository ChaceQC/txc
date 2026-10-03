# 自研 GUI 续接记录

更新：2026-10-03。本轮开始前已按用户要求将此前 GUI 续接提交为 `37370b6`，
提交内容包括 OpenType 整形、容器、数据视图与 P2 基础交互。本轮 P6/P7 改动位于该提交之后。
完整范围以 [实施契约](native_gui_full_plan.md) 为准。本记录不把局部工作包完成等同于完整版验收。
当前尚缺实现、人工验收与发行收尾统一见 [剩余工作清单](native_gui_remaining.md)。

最新工作为 P6/P7：共享命令、自绘菜单和消息对话框、模态 owner、系统文件选择器、系统/高对比主题、
UIA 与 AT-SPI。接口、系统服务依赖和定向验证见 [系统集成契约](native_gui_system.md)。

最新续接已增加 GSUB/GPOS/GDEF 水平整形和阿拉伯连接，见 [整形契约](native_gui_shaping.md)。
此前已增加视觉光标/亲和性、自绘滚动/页签/分隔器/画布，以及 list/table/tree 的模型、
选择、列管理、排序请求、树展开和虚拟可见行。详细内容、验证与尚未交付项见
[完整版续接实施记录](native_gui_completion.md)，接口分别见 [容器](native_gui_containers.md)
和 [数据视图](native_gui_data.md)。下文保留此前 XIM/UAX #14 工作记录，不代表最新全部状态。

## 本次落地

1. 重新对照 X.Org XIM 协议，补齐六种 X 传输组合，修正上下文掩码、焦点交付边界、
   动态触发键的窗口事件掩码。旧输入在 reset 结束前被隔离；协议失败恢复直接键盘输入。
   完整窗口检查另发现并修复 `GetKeyboardMapping` 的字段错位，保证实际窗口可以初始化键盘。
2. 自行实现 Unicode 16.0 UAX #14 默认断行，固定输入数据版本与 SHA-256，保留官方测试源文件。
3. 文字布局改用合法机会上的贪心断行，中文标点与英文单词参与规则；超宽词按字素紧急折行。
   更新换行、命中、光标和选区的一致性检查，以及单行框的全部强制换行过滤。
4. 更新编辑器示例、研究记录、Unicode 文档和双平台 Unicode 数据许可证打包。

## 验证记录

| 检查 | Windows | Ubuntu 24.04 |
| --- | --- | --- |
| UAX #14 官方 16,672 条用例 | 通过 | 通过 |
| 单词/中文标点/字素紧急折行/光标/选区 | 通过 | 通过 |
| 编辑缓冲/撤销/换行归一化 | 通过 | 通过 |
| XIM 六种传输/预编辑/提交/焦点隔离/动态触发键/错误恢复 | 不适用 | 通过 |
| 普通静态库、ThinLTO 与 `.tx` GUI 定向检查 | 通过 | 通过，已包含键盘初始化修正 |
| `.tx` 宽窄文本导出示例 | 通过 | 通过，与 Windows 像素文件一致 |

两边使用同一份微软雅黑 TTC，生成的 `text_wrapping_windows.bmp` / `text_wrapping_linux.bmp`
SHA-256 均为 `5c3636a52a69189dfe330f529282bf098eb7a06da547f9bfbdaef82b955763a6`。
实际图像已查看，预览为 `tx_build/native_gui/text_wrapping.png`。图像中的组合附标缺字也保留了
当前字体回退/整形尚缺的真实状态，不把断行通过当作完整文字引擎通过。

工具包位于 `tx/` 与 `tx/linux/`；交互示例为 `tx_build/native_gui/editor.exe` 和
`tx_build/native_gui/editor_linux`。源码、测试数据、工具包、示例可执行文件与图像产物均已保留。
产物确认后的 `build/` 清理被自动审批以 `blocked by policy` 阻止；没有给出更具体原因，
因此临时目录仍保留，尚未完成清理。没有通过其他工具绕过该限制。

源码与协议测试不依赖外部 GUI、字体或排版实现。XIM 测试在真实 X11 连接上使用自己编写的
独立协议对端，不修改系统输入法选择和用户剪贴板。实际中文输入法服务、候选窗口、辅助技术
与真人桌面验收尚无本轮证据。

复核入口（文字检查独立编译自研源码；`.tx` GUI 检查需要先构建对应平台工具包）：

```powershell
python scripts/check_native_gui_text.py
python scripts/check_native_gui.py
```

```sh
python3 scripts/check_native_gui_text.py --font /path/to/font.ttf
python3 scripts/check_native_gui.py --font /path/to/font.ttf
```

文字检查支持 `--only line|layout|buffer|xim`，用于只重跑发生变化的部分；XIM 检查仅在 Linux 运行。
比较双平台时应指定同一份字体。桌面示例为 `examples/native_gui/editor.tx`。

## 完整范围的剩余工作

| 工作包 | 当前边界与尚缺部分 |
| --- | --- |
| R0 渲染 | 现有路径/扫描转换/4×4 覆盖采样；图像采样、脏区提交及完整边界验收未全部完成 |
| R1 字体 | TrueType/TTC、轮廓、复合字形、缓存及有限候选路径的字素级回退已有；完整字体发现、hinting 和其他字体格式仍缺 |
| R2 排版 | 字素、词边界、默认断行、UAX #9、视觉亲和性及水平 GSUB/GPOS/GDEF/阿拉伯连接已有；其他脚本特有重排和完整规范化仍缺 |
| P0 平台 | Windows 与 X11/XWayland；原生 Wayland、完整键盘布局和真实输入法服务验收仍缺 |
| P1 布局 | row/column/grid/overlay、尺寸策略和约束已有；最终嵌套/DPI 桌面验收未完成 |
| P2 交互 | 基础按钮/勾选/单选/范围控件、combo、访问键、默认/取消动作已有；定向证据见基础交互契约，人工桌面验收仍待执行 |
| P3 文本 | 单/多行、选区、撤销、双向视觉编辑与平台输入已接入；真实中文输入法验收仍缺 |
| P4 容器 | tabs/scroll/split、滚动条、嵌套滚动与独立画布已有；人工桌面验收仍待执行 |
| P5 数据 | list/table/tree、稳定 ID、虚拟行、树导航与批量模型更新已有；完整应用及人工验收仍待执行 |
| P6 命令 | 共享命令、快捷键、自绘菜单、工具栏/状态栏、模态/消息和系统文件/目录对话框已接入 |
| P7 主题/辅助 | system/high_contrast、字号/缩放和 UIA/AT-SPI 已接入；真实读屏体验仍待人工验收 |
| P8 分发 | 双平台开发工具包已有；完整示例集、发行和真人桌面验收仍缺 |

本次续接已推进 R2 双向与 R1 字体回退，详见 [完整版续接实施记录](native_gui_completion.md)
和 [Unicode 契约](native_gui_unicode.md)。后续仍需其他脚本特有整形、
字体、渲染、平台完善；真实输入法、读屏使用与人工桌面验收作为独立门槛。

本次新增双平台证据：UAX #9 的 770,241 个属性测试实例与 91,707 条字符用例通过；
双向布局/回退检查、普通与 ThinLTO 工具包构建及 `.tx` GUI 定向检查通过。
示例为 `examples/native_gui/bidi_text.tx`，Windows 图像已查看。打包目录清理的审批限制与
剩余文件见上面的续接实施记录；不能将构建通过标记为完整版或干净发行验收通过。
