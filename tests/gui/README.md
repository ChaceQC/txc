# GUI 定向验证

U4–U5 使用 `python scripts/check_gui_u4_u5.py`，可指定 `tx`、`native`、`example` 分组，只验证发生变化的部分。覆盖普通/ThinLTO 具体 ABI、错误类型与跨线程拒绝、分页焦点、滚动定位、分栏、原生辅助名称、自绘 UIA 跨线程查询和受控动作、模拟 DPI/主题/失焦、画布重建/关闭，以及后台取消和完成。产物在 `tx_build/gui_u4_u5/`，契约与验收记录见 [U4–U5](../../docs/gui_u4_u5.md)。

任务工作台可用 `tx/txc.exe examples/gui/task_workspace.tx` 编译；运行后人工核对 Ctrl+Tab、分隔条方向键、画布 Enter/Space、滚动与编辑、取消/关闭，以及 Narrator、高对比度和真实混合 DPI。自动验证不更改系统主题。

U2–U3 扩展验证使用 `python scripts/check_graphics_gui_extensions.py gui native`，具体结果见 [U2–U3 记录](../../docs/gui_u2_u3.md)。运行 `data_browser.tx` 人工核对菜单/工具栏/按钮多入口、Ctrl+O、输入法优先级、文件框取消/多选、剪贴板忙及多显示器 DPI。自动验证不读取或覆盖用户剪贴板内容。

构建后运行 `python scripts/check_gui.py`，产物位于 `tx_build/gui_u0_u1/`。
脚本覆盖具体控件错误、伪造资源、闭包捕获和嵌套 Send 拒绝，普通与 ThinLTO 的直接 ABI、属性及生命周期，原生布局边界、事件快照/合并/密码、UTF 选择、焦点转移、消息队列溢出、模拟 125%/200% DPI；最后启动两个示例验证原生输入/点击经 TX 事件循环更新标签并正常退出。

人工验收待执行：

1. 运行问候示例，用鼠标和 Tab/Shift+Tab 在输入框与按钮间操作，Enter 提交姓名，Escape 关闭。
2. 运行设置表单，缩放窗口，检查 grid 两列、备注多行、按钮 row，空格切换复选框、保存与取消。
3. 在真实 125% 和 200% 显示器间移动窗口，检查系统字体、输入框高度、相邻边界和客户区填充。
4. 用微软拼音编辑中文，组合期间 Enter/Escape 不触发提交或关闭；检查 emoji/组合字符选择。
5. 多窗口关闭与异常退出后没有遗留 HWND。U5 的 Narrator、访问键和真实高对比度检查仍须人工验收。
