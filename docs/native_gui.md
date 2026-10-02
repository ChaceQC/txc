# 跨平台自研 GUI 库

目标为 Windows + Linux 完整版，完整范围见 [实施契约](native_gui_full_plan.md)。
当前处于开发中，不能把基础控件或单个演示视为完整版。算法依据见 [研究记录](native_gui_research.md)。

## 实现边界

GUI 核心不依赖 Qt、GTK、SDL、ImGui、Direct2D、DirectWrite、FreeType、HarfBuzz、Pango、
Cairo、Skia 或 ICU。字体文件和 Unicode 官方属性数据作为输入，由本项目自己解析与处理。

- `core/`：软件像素缓冲、alpha 合成、路径细分与扫描转换、字体解析、字素边界、文本模型。
- `platform/`：Windows 系统窗口/输入/像素提交，以及直接 X11/XWayland 协议连接。
- 控件树、布局、交互、绘制和应用事件循环由双平台共用代码负责。
- Windows 的 GDI 只提交已经生成的像素，不绘制文字、路径或系统控件。
- Linux 当前支持本地 X11/XWayland 的 24-bit TrueColor、32-bit 像素存储，以及自行编码的 XIM 输入法协议；
  原生 Wayland、完整键盘布局与辅助功能尚未完成，真实中文输入法服务验收仍待执行。

现有 `gui` 与 `graphics` 保持兼容；新库使用独立应用与窗口资源，不导入 Windows 专用接口。

## 当前公开接口

`native_gui.txh` 提供应用、窗口、面板、标签、按钮、复选框、单选按钮、文本框、滑块和进度条的不透明类型。

`open_app()` 读取默认 TrueType 字体；`open_app(font_path)` 显式指定 TTF/TTC 文件。
也可以通过 `TX_GUI_FONT` 指定路径。Windows 默认微软雅黑；Linux 查找文泉驿、DejaVu Sans
或 Liberation Sans。默认字体缺字会显示该字体的 missing glyph；自动多字体回退仍待实现。

`create_window(app, title, width, height)` 创建窗口，`show` 显示，`root` 取得唯一根面板。
当前面板支持 `set_column/set_row`，尺寸由 `set_size` 设置，0 表示自动测量。
列方向的自动宽度填充父项，行方向按内容测量；空间不足时裁剪，隐藏项退出布局。

布局扩展按完整契约实现：`fixed/auto_length/stretch` 表达尺寸策略，
`set_width/set_height/set_constraints/set_margin/set_alignment` 设置节点策略；
`set_grid/set_grid_row/set_grid_column/set_cell` 配置网格与跨度，`set_overlay` 配置叠放。
固定轨道不参与空间不足时的收缩；auto/stretch 遵守最小与最大尺寸，剩余空间按权重分配。
非法轨道、跨度、负边距或非有限数必须在修改树之前拒绝。

窗口持有根，面板持有子项。句柄释放不会移除仍在树内的控件；`close` 递归使子项失效且幂等。
GUI 资源不可伪造、不可 Send/Sync。节点数量上限 4096，嵌套上限 64。

`next_event(app, timeout_ms)` 返回 `result<option<event>>`，-1 阻塞、0 立即返回。
库在事件交付前处理脏布局与重绘；不需要应用启动重绘定时器或每帧重建控件。
事件记录包含 `kind/window_id/source_id/text/number/state/revision`：

- `close_requested`：应用决定是否关闭窗口。
- `resized`：客户区已经更新。
- `activated`：按钮激活；`source_id` 标识按钮。
- `check_changed`：复选框状态更新；`state` 为发生时快照。

程序 setter 不产生用户操作通知。鼠标按下与释放必须落在同一按钮才激活；
Tab/Shift+Tab 遍历可用控件，Space 按下/释放激活，Enter 忽略按键重复。
`set_theme` 当前支持 light/dark。`paint` 可同步提交一帧，`save_bitmap` 保存自己生成的 BMP。

文本控件提供单行/多行、只读、密码、长度上限和选区。端点以 Unicode 标量计数，
落在字素内部时向前对齐；编辑导航按字素或 Unicode 词边界移动。
Ctrl+A/C/X/V/Z/Y、Ctrl+方向键、Home/End/PageUp/PageDown 由本项目的编辑模型处理。
多行 Enter 换行，单行 Enter 或 Ctrl+Enter 产生 `text_committed`；用户编辑产生 `text_changed`。
密码控件的用户事件不带正文，复制/剪切不导出密码。

进度条支持确定值和不确定动画，滑块提供 `value_changed/value_committed`，单选按钮按同一父面板下
的 group ID 互斥。程序 setter 不发送这些用户事件。输入和范围控件示例见
[自研编辑与交互](../examples/native_gui/editor.tx)。

Windows 输入法使用系统 IMM 消息，只接收预编辑/提交文本及设置候选位置；预编辑的绘制仍由自研核心完成。
Linux 的 X11 剪贴板自行实现 Selection、UTF8_STRING/STRING 和 INCR 传输。
Linux XIM 已实现发现与连接、六种 X 传输组合、UTF-8 协商、自绘预编辑、提交、候选位置、
触发键、同步与取消。支持具有 UTF-8 编码和 PreeditCallbacks 样式的输入法服务；不支持的
编码/样式、协议错误与超时会报告 `input_method_error`，随后恢复直接键盘输入。
焦点切换时重置旧组合，排队输入在应用处理焦点/光标后继续发送。密码框不启用 IME。
测试使用独立协议对端和真实 X11 连接，真实中文输入法服务及候选窗口桌面验收仍待执行。

## 编译和运行

在双平台编译器构建完成后，使用同一份 [工作台源码](../examples/native_gui/workbench.tx)。

Windows：

```powershell
.\tx\txc.exe .\examples\native_gui\workbench.tx --subsystem windows -o .\tx_build\native_gui\workbench.exe
.\tx_build\native_gui\workbench.exe
```

Linux（根据工具包位置调整 txc 路径）：

```sh
./tx/linux/txc examples/native_gui/workbench.tx -o tx_build/native_gui/workbench_linux
TX_GUI_FONT=/path/to/font.ttf ./tx_build/native_gui/workbench_linux
```

## 当前验证与未完成项

自研渲染/字体核心在 Windows 与 Ubuntu 24.04 上生成了 SHA-256 相同的 BMP：
`b7749cb52f1d2e34f7b229eff28565c7e2a67795d2c68b25e3901dc48f813024`。
两边独立系统窗口的像素显示与关闭检查通过。
Unicode 16.0 的 1093 条官方字素边界用例在双平台通过。

文字布局使用自研 Unicode 16.0 UAX #14 默认断行，双平台通过 16,672 条官方用例；
布局优先在合法机会换行，超宽词采用字素级紧急折行，并保持光标/选区与逻辑索引一致。
仍未实现 UAX #9、GSUB/GPOS、复杂脚本整形、字体回退、hinting 或全部字体格式，不能称为完整文字引擎。
文本缓冲的字素移动、词导航、选区、删除、撤销重做与换行归一化已接入输入控件。

[宽窄文本布局](../examples/native_gui/text_wrapping.tx) 可编译运行并导出
`tx_build/native_gui/text_wrapping.bmp`，用于检查实际 `.tx` 静态调用链生成的文本图像。
模块完成状态、验证入口与后续工作见 [续接记录](native_gui_progress.md)。

完整控件集合、滚动容器、数据模型、命令/菜单/对话框、UIA/AT-SPI、
双平台发行与桌面验收均仍属于实施契约中的待完成工作。
