# 自研 GUI 续接记录

更新：2026-10-02。开发基线为 `4b3fb6e`，此前与本次自研 GUI 工作仍保留在当前工作区。
完整范围以 [实施契约](native_gui_full_plan.md) 为准。本记录不把局部工作包完成等同于完整版验收。

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
| R1 字体 | TrueType/TTC、轮廓、复合字形与缓存；自动字体回退、hinting 和其他字体格式仍缺 |
| R2 排版 | 字素、词边界、默认断行与逻辑索引布局已有；UAX #9 双向、GSUB/GPOS、复杂脚本整形仍缺 |
| P0 平台 | Windows 与 X11/XWayland；原生 Wayland、完整键盘布局和真实输入法服务验收仍缺 |
| P1 布局 | row/column/grid/overlay、尺寸策略和约束已有；最终嵌套/DPI 桌面验收未完成 |
| P2 交互 | 基础按钮/勾选/单选/范围控件已有；combo、访问键、默认/取消动作等仍缺 |
| P3 文本 | 单/多行、选区、撤销与平台输入已接入；双向编辑及真实中文输入法验收仍缺 |
| P4 容器 | tabs/scroll/split、滚动条与独立画布仍缺 |
| P5 数据 | list/table/tree、稳定 ID、虚拟行与模型更新仍缺 |
| P6 命令 | 共享命令、菜单、快捷键系统及模态/文件对话框仍缺 |
| P7 主题/辅助 | light/dark 已有；system/high contrast、UIA、AT-SPI 仍缺 |
| P8 分发 | 双平台开发工具包已有；完整示例集、发行和真人桌面验收仍缺 |

后续先补 R2 的双向文本与整形/字体回退契约，重新核对 Unicode 和 OpenType 规范，
再推进 P4/P5 容器与数据控件；真实输入法服务验收作为独立门槛补齐。
