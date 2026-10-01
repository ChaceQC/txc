# Windows GUI U4–U5

状态：实现及定向自动验证通过，真实桌面人工验收待执行。接口冻结：2026-10-02；验证日期：2026-10-02。

## 复杂容器与进度

公开签名以 `tx/stdlib/gui.txh` 为准，不新增语法。所有资源沿用 UI 线程、共享别名、树拥有权、4096 节点/64 层限制及具体类型静态 ABI。

- `create_tabs(parent)` 创建原生分页控件；`add_tab(tabs, title)` 返回页面容器，最多 128 页。`select_tab(tabs, page)` 只接受其直属页面；`selected_tab(tabs)` 返回当前页面的稳定 ID（无页面为空 option）。未选页面不参与布局和焦点导航；关闭当前页选择相邻页。用户切换产生 `selection_changed`，item_id 为页面 ID，setter 不发送用户事件。
- `create_scroll(parent, horizontal, vertical)` 返回滚动容器，至少启用一轴，内容使用普通 row/column/grid 布局。`set_scroll_position(container, x, y)` 与 `scroll_x/scroll_y` 使用 DIP，非滚动轴必须为零，超出内容范围钳制。滚动轴按内容测量，stretch 按 auto 处理；内容每轴最多 16384 DIP。滚轮、原生滚动条和键盘方向/PageUp/PageDown/Home/End 可操作，子 HWND 与画布一起裁剪、移动。
- `create_split(parent, vertical, position, minimum_first, minimum_second)` 返回分栏容器，vertical=true 表示上下分栏，否则左右分栏。内建两个页面通过 `split_pane(container, index)`（0/1）取得；不允许直接增加第三个子项或覆盖分栏布局。分隔条 6 DIP；position 为 [0,1] 的比例，最小尺寸为非负 DIP。空间不足按两侧最小尺寸比例裁剪；鼠标拖动或方向键改变位置，Home/End 到边界，键盘步长 0.02，产生 value_changed/value_committed。失焦、取消模式或失去捕获终止拖动。
- `create_progress_bar(parent)` 初始范围 0–100、值 0；`set_progress(control, minimum, maximum, value)` 原子更新，范围和值是 int，0 <= minimum < maximum <= 2147483647。`set_indeterminate(control, bool)` 显式切换不确定进度，退出后恢复保存值。
- `create_slider(parent, minimum, maximum, value, step)` 使用同样整数范围，step 为正且不超过范围；`set_slider_value` 校验范围，`slider_value` 返回当前值。原生拖动/键盘产生 value_changed，结束产生 value_committed。

## 画布与后台工作

`create_canvas(parent)` 返回独立子 HWND。`begin_canvas(canvas)` 返回 `result<option<graphics_canvas>>`；隐藏、最小化或零尺寸时为空。与窗口帧共用单会话活动帧约束，绘图、提交和取消复用 graphics 接口。`invalidate_canvas` 请求完整重画，`canvas_width/height` 返回客户区 DIP。`canvas_paint` 经 control.source_id 路由；鼠标/键盘事件同时保留 graphics 的类型化 pointer/key 载荷并用 control.source_id 标识画布，坐标相对画布客户区。Enter/Space 或 UIA Invoke 产生 activated，应用定义具体业务动作。

画布目标按客户区像素和 DPI 更新。设备丢失只释放绘图目标并请求重画，不销毁控件树；关闭画布/父窗口或异常退出取消活动帧、释放资源。系统主题变更重画原生控件及画布；`theme_color(role)` 提供 background/text/window/window_text/highlight/highlight_text 语义颜色，`high_contrast()` 读取系统状态。

示例 `examples/gui/task_workspace.tx` 以有界 channel + cancel_token 传输进度（包含 generation），后台不捕获 GUI 资源。工作期间 next_event 最多等待 25 ms，每轮最多消费 16 条进度；完成后无限等待。使用 `channel.select(..., 0, token)` 轮询，避免把 recv 的 timeout 错当成 EOF。取消及关闭先请求取消，继续处理事件，收到完成标记后可关闭窗口；排空至 worker 的 channel.close/EOF 后 join，最后关闭会话。UI 异常也执行这条收束路径；旧 generation 的回复丢弃。

## 辅助功能与恢复

具体控件支持 `set_accessibility(control, name, help_text)` 和 `set_label(control, label)`。标签必须属于同一窗口，关联名称随标签文字更新；显式 name 优先。系统控件复用原生 MSAA/UIA bridge 与既有 pattern，不替换其选择、文本或密码行为。自绘 split/canvas 通过 WM_GETOBJECT 提供 UIA Simple provider，报告名称、帮助、角色、启用/焦点/边界状态；split 提供 RangeValue，canvas 提供 Invoke。provider 从加锁快照读取，动作投递受控原生消息，绝不在 COM 回调调用 TX。关闭使现有 provider 失效。

Tab 导航跳过隐藏页面与禁用祖先；Ctrl+Tab 切页。失焦/窗口失活释放画布按键、鼠标捕获与分栏拖动状态。字体、主题、系统颜色和 DPI 更新触发重新测量、子控件重绘和 UIA 快照更新。窗口模态禁用/恢复会刷新整棵树的 UIA enabled 状态，已排队动作也重新检查 owner；完全被滚动祖先裁剪的画布标为 offscreen。布局/绘图帧重入限制、原生回调异常转存和会话退出清理继续适用。

普通及 ThinLTO 程序均链接 UIAutomationCore、oleacc、oleaut32。构建脚本在 MinGW 缺少 UIA 导入库时根据 `cmake/uiautomationcore.def` 生成所需系统导出的导入库；不复制 Windows 系统 DLL。

## 验证记录

当前 Windows x64 / LLVM 23.1.2 环境：

| 项目 | 结果 |
| --- | --- |
| 构建 | 编译器、普通与 ThinLTO 标准库、UIA 导入库、工具包兼容摘要均已生成 |
| TX / 静态 ABI | 普通与 ThinLTO 的分页选择/关闭、分栏、滚动位置、进度原子范围检查、滑块、画布/主题与父关闭失效通过；错误控件类型及画布跨线程捕获在编译期拒绝，IR 调用具体 ABI |
| 原生布局与输入 | 相邻分页选择、隐藏页焦点拒绝、双轴滚动、分栏键盘/拖动/失焦、不同画布/窗口事件不混合，以及模拟 120/192 DPI 通过 |
| UIA | 原生辅助名称及关联标签更新、自绘 RangeValue 跨线程查询/动作、Invoke、禁用祖先和模态 owner 恢复、滚动裁剪后的 offscreen、关闭后的 provider 失效通过 |
| 恢复与清理 | 主题变化触发字体重测量；画布隐藏时无帧，释放目标后可重建且控件树保留；父关闭及原生资源句柄异常展开会取消帧并清理 HWND。真实 GPU 设备故障未注入 |
| 任务示例 | ThinLTO 编译运行，工作期间关闭/取消收束、正常完成时更新状态和关闭通过；只操作验证脚本创建的进程 |
| 受影响回归 | GUI U2–U3 普通/ThinLTO、图形 G0–G1 与 GUI U0–U1 行为、原生输入/绘图/命令/模型/重建通过 |

复现入口：`python scripts/check_gui_u4_u5.py`；支持 `tx`、`native`、`example` 分组。回归入口：`python scripts/check_graphics_gui_extensions.py gui baseline native`。没有运行全仓测试。

产物在 `tx_build/gui_u4_u5/`，可直接运行 `task_workspace.exe`；源码为 `examples/gui/task_workspace.tx`。编译器和两种标准库在 `tx/`，产物与兼容摘要已确认。本次自动审批以“blocked by policy”拒绝删除已经核验的 `E:\Project\other\Compilation\build`，因此临时构建目录仍保留。

人工验收待执行：Narrator 实际导航与操作、微软拼音、真实高对比度切换、多显示器混合 DPI、自绘和原生控件滚动观感。U6 发行及最低系统基线实测不属于本次范围。
