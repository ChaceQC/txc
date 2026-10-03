# 命令、对话框、主题与辅助技术

本契约对应完整版 P6/P7，沿用自绘控件和直接静态 ABI。普通菜单、工具栏、状态栏和消息
对话框使用公共控件与软件绘制；平台适配负责窗口所有权、系统偏好和辅助技术协议。

## 命令与菜单

- 命令归属一个窗口，具有稳定 ID、文字、快捷键、enabled、checked 和 checkable 状态。
  按钮、工具栏和菜单引用同一命令；一次用户触发只产生一个 `command` 事件。
  程序更新状态不触发执行事件。关闭命令后，引用它的入口失效。
- 快捷键使用 `ctrl+shift+alt+meta+键名`，修饰键顺序不限，键名为小写 ASCII 字母、数字、
  `f1`–`f12` 或具名导航键。同一窗口不允许重复绑定；输入法组合期间不执行快捷键。
- 菜单支持分隔项和子菜单；禁止跨窗口连接和循环。菜单栏与上下文菜单共用模型。
  方向键导航，Enter/Space 执行，Escape 逐层返回；隐藏、禁用、关闭项不可执行。
  关闭弹层恢复原焦点；外部点击取消且不穿透。
- 工具栏是带语义角色的 row 面板，状态栏是带语义角色的标签，沿用已有布局接口。

## 对话框

- `show_modal(dialog, owner)` 非阻塞，继续使用应用事件循环；禁止自身、跨应用和所有权循环。
  owner 在模态期间不能接收用户操作；嵌套模态按所有权链恢复焦点。
- `end_dialog(dialog, result)` 保存结果并关闭；关闭请求由业务处理，也可直接 close 取消。
  `dialog_result` 的 none 表示未确认或取消，整数表示显式完成。
- 消息框返回 `ok/yes/no/cancel`。文件操作返回 `result<option<str>>`：取消为 none，
  平台或文件系统失败为 error。保存选择不会写入文件。

## 主题和字体

- 支持 `light/dark/system/high_contrast`。默认 system；系统高对比度优先于常规配色。
  系统偏好变更触发重绘和 `theme_changed`，不重建控件树。
- Windows 读取应用明暗偏好和系统高对比度颜色；Linux 使用桌面设置服务。
  无桌面偏好时采用 light；显式主题仍然有效。
- 字号与应用缩放独立于窗口 DPI；变更使文字缓存和布局失效，命中与绘制使用同一尺度。

## 辅助技术

- 公共语义树提供稳定身份、父子关系、名称、帮助、角色、屏幕边界、可见/禁用/焦点状态。
  Windows UIA 与 Linux AT-SPI 从同一树读取，操作回到 UI 线程及现有控件行为。
- 按控件实际能力公开 Invoke、Value、Range、Selection、Text；只读和禁用状态限制写入。
  密码控件不公开原文、选择内容和可恢复原文的文本范围。
- 隐藏页、关闭控件、模态 owner、数据行与当前选择必须反映真实状态；旧引用不能操作新对象。
  状态、焦点、文本和选择变更发送协议通知。

## 验证边界

定向验证覆盖命令共享/冲突、菜单取消、模态恢复、主题更新、语义查询与操作、静态 ABI，
并从系统辅助技术客户端遍历和操作窗口。真人读屏使用和桌面观感验收单独记录，
不能由构建或内部语义树测试替代。

## 平台依赖与能力边界

Windows 使用系统 UIAutomationCore 和 Shell COM，不创建普通系统子控件。
Linux 窗口仍使用自研 X11/XWayland；D-Bus 传输动态加载系统 `libsystemd.so.0`，
AT-SPI 连接 `org.a11y.Bus` 和 Registry，文件选择与外观设置使用桌面 Portal。
Linux 桌面需要 `at-spi2-core`、`xdg-desktop-portal` 及对应桌面的 Portal backend。
这些是系统服务；控件、布局、绘制、文字和辅助语义不调用外部 GUI 实现。

无 AT-SPI 服务时产生 `accessibility_error`，普通 GUI 继续运行；无 Portal 文件服务时返回错误，
不把服务不可用当作取消。Linux 无外观服务时回退浅色；显式主题和显式高对比度仍可使用。
系统偏好在事件循环中每秒检查，窗口 DPI 与应用缩放共同参与绘制和输入换算。

语义公开现有控件树、菜单项、数据行、combo 项和页签；数据行不产生系统子窗口。
辅助协议通过 UI 线程读取/操作现有状态。普通 GUI 无辅助客户端访问时不持续构建完整语义快照。
当前提供普通文本与控件能力，不宣称实现 UIA/AT-SPI 的所有文档、富文本、注释、表格单元格等扩展接口。
完整读屏交互、不同桌面 Portal backend、真实输入法和人工桌面验收仍需用户环境的独立证据。

定向入口：

```powershell
python scripts/check_native_gui_system.py
python scripts/check_native_gui_system.py --only protocol
```

```sh
python3 scripts/check_native_gui_system.py --font /path/to/font.ttf
python3 scripts/check_native_gui_system.py --font /path/to/font.ttf --only protocol
```

`system_behavior.cpp` 检查命令共享与冲突、菜单、模态恢复、主题/缩放、语义操作与旧引用失效。
`.tx` 检查普通和 ThinLTO 的直接 ABI；`uia_behavior.cpp` 与 `atspi_behavior.cpp`
通过系统客户端/注册表读取和操作真实控件。`windows_dialog_behavior.cpp` 检查自绘消息框默认动作、
系统 Shell 取消与 owner 恢复。`portal_dialog_behavior.cpp` 使用独立 D-Bus 对端验证协议，
需要在 `dbus-run-session` 隔离会话执行；它不替代真实桌面文件选择器的人工验收。

GitHub Actions 使用 `scripts/check_native_gui_ci.py` 汇总本轮门禁，保留逐项日志与 JSON 结果。
`examples/native_gui/release_smoke.tx` 在真实解包目录验证普通/ThinLTO 链接、自绘、命令和模态恢复。
CI 环境、系统服务与安装包检查见 [CI/CD 说明](releasing.md)。

同源示例：`examples/native_gui/system_workbench.tx`，产物位于
`tx_build/native_gui/system_workbench.exe` 与 `system_workbench_linux`。

2026-10-03 最终双平台普通/ThinLTO 构建、核心行为与 `.tx` ABI 检查通过；
Windows UIA 与 Linux AT-SPI 系统客户端遍历、实际控件读写通过。
Windows 消息框与 Shell 取消、Linux Portal 独立协议对端检查通过。
完整验证记录和产物摘要见 [本轮实施记录](native_gui_completion.md)。
