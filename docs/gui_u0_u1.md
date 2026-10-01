# Windows GUI U0–U1

状态：U0–U1 已实现，定向自动验证通过；真实桌面人工验收待执行（2026-10-02）。完整规划见 [GUI 设计](windows_gui_design.md)。

## 阶段契约

U0 登记设计中的全部具体不透明资源类型。U1 创建 `gui_container`、`gui_label`、`gui_button`、`gui_text_box`、`gui_check_box`，其他类型留给对应阶段。资源沿用图形共享值根和静态类型检查，不能伪造，不能 Send/Sync，直接或嵌套 deep_copy 失败。父节点持有子节点；局部变量释放不销毁挂树控件。显式 close 递归销毁并使所有别名失效，重复 close 安全。

公开签名以 `tx/stdlib/gui.txh` 为准。`root(window)` 重复调用返回同一根；根关闭后允许重新创建。`create_container` 创建嵌套容器，默认 column。`create_check_box(parent, text, three_state)` 固定二态或三态；`set_checked/checked` 仅接受二态，三态用 `set_check_state/check_state` 的 `off/on/mixed`。文本框创建时固定单行/多行，提供只读、密码、Unicode 标量选择范围和文本长度上限；多行密码模式拒绝。默认按钮和取消按钮限于同一窗口。

五种已实现控件具有显式重载的 id、is_open、close、set_enabled、set_visible、set_reserved_space、set_width、set_height、set_constraints、set_margin、set_alignment、set_cell。四种文本控件具有 set_text/text；button/text/check 支持 focus。公共操作在 LLVM 中静态选择具体类型 ABI，不经过 any 或运行时函数名分派。

`length(mode, value)` 的 mode 为 fixed/auto/stretch；`fixed`、`auto_length`、`stretch` 提供工厂。fixed 非负，stretch 权重有限且在 (0,16384]；auto 的 value 必须为 0。默认 column 宽 stretch、高 auto；row 中未显式设置宽度的子项按内容宽度。width/height 默认策略仍可显式覆盖。`set_constraints` 的四个参数依次为 min_width/min_height/max_width/max_height，默认 0/0/16384/16384；margin 与容器 padding 为四边统一非负 DIP。alignment 的横纵值均为 start/center/end/stretch，默认 stretch。

row/column 用 `set_row/set_column(parent, padding, gap)`；grid 用 `set_grid(parent, rows, columns, padding, gap)`，默认行 auto、列等权 stretch。用 `set_grid_row/set_grid_column` 修改单条轨道，用 `set_cell(control, row, column, row_span, column_span)` 定位。网格默认位置为 (0,0,1,1)，允许显式重叠；范围和跨度在 setter 时检查，改变网格大小也先检查已有子项，失败保持原布局。隐藏项退出布局，reserved_space 可保留位置；禁用项仍占空间。overlay、scroll、absolute 及手工 bounds 不属于 U1。

布局先测量原生系统字体与内容，再分配 fixed/auto 和间距，余量按 stretch 权重分配；遇到 max 后重新分配。不足时收缩 auto/stretch 至 min，fixed 保持固定；仍不足则由父 HWND 裁剪。网格跨度先满足可伸缩轨道，纯固定轨道不扩张。DIP 使用累计边界取整为像素，避免 125% 下的相邻间隙。属性与尺寸改变只标脏，下一事件交付/绘图前刷新；`flush_layout(window)` 在活动帧或通知重入期间拒绝。

事件统一为 graphics.event.control：source_id/action/text/number/state/item_id/command_id/revision。替换 G1 尚未产生过事件的旧 control_id 字段。产生 activated、text_changed、text_committed、check_changed、focus_gained、focus_lost；离散通知隔断连续文本合并。setter 更新当前状态及 revision，但不发送用户事件。密码事件没有文本。事件为发生时快照，关闭后历史事件仍可交付。回调只更新 C++ 状态和队列，错误保存至下次 next_event，不执行 TX。

Tab/Shift+Tab 按树创建顺序遍历可见、启用的 button/text/check；关闭、隐藏或禁用焦点节点时转移到下一个节点，没有候选时回到顶层窗口。Enter 在单行输入框产生 text_committed，多行保留换行，按钮可键盘激活；其他位置交给默认按钮。Escape 交给取消按钮，否则产生 close_requested。IME 组合期间不截获 Enter/Escape/Tab。完整访问键、快捷键与 UIA 留待 U2/U5。

## 固定限额

每窗口 4096 个节点（含根），树深度 64，grid 每轴 128 条轨道，文本最多 65536 UTF-8 字节，尺寸/边距/间距最多 16384 DIP，字体跟随系统消息字体并按窗口 DPI 更新。文本框默认最大 16384 个 Unicode 标量，显式上限为 1–16384；程序 setter 超长直接失败，用户输入越界恢复上一次有效文本。事件队列及主线程退出清理沿用 G1。GUI 误用采用 graphics_error，布局错误为 invalid_layout。

## 验证与交付

定向验证覆盖静态拒绝、资源别名/深复制、类型化容器、事件快照与 setter 静默、原生生命周期、布局边界、模拟 125%/200% DPI、键盘消息及普通/ThinLTO 直接 ABI。问候和设置表单放在 `examples/gui/`。真实鼠标键盘、微软拼音、多显示器 DPI、视觉与屏幕阅读器验收单独记录，自动消息检查不等于人工验收。

2026-10-02，当前 Windows x64 环境完成以下验证：

| 项目 | 结果 |
| --- | --- |
| 普通工具链与 ThinLTO 标准库 | 构建成功，LLVM 23.1.2；编译器、两种静态库与兼容性摘要已更新 |
| 四类静态拒绝 | 错控件类型、整数伪造、跨线程闭包捕获、嵌套非 Send 在源码位置拒绝 |
| LLVM IR | set_text/set_height 等调用具体控件 ABI，布局记录展开为标量，无通用 external_call |
| TX 行为，普通/ThinLTO | length 工厂、布局属性、UTF 标量选择、二/三态、类型化 vector、any 恢复、深复制拒绝、setter 静默及父子别名失效通过 |
| 原生生命周期与消息 | 同树控件的最后一个 TX 引用释放后仍存活；父关闭递归销毁 HWND；跨线程和帧内刷新拒绝；焦点转移、Tab、Enter/Escape 与 IME 组合标记保护通过 |
| 原生布局 | stretch 达 max 后重新分配、空间不足的 min 约束、网格跨度与失败原子性、隐藏/保留位置、模拟 120/192 DPI 字体与相邻像素边界通过 |
| 控件事件 | 文本快照、相邻合并、提交屏障、revision、密码不携带文本、复选框状态更新、关闭后历史 ID 与队列溢出交付通过 |
| 两个完整示例 | 自动启动，发送中文/emoji 输入与按钮点击，TX 事件循环更新标签，关闭请求正常退出 |
| 图形 G0–G1 回归 | 原有静态拒绝、普通/ThinLTO 行为、异常清理、原生生命周期及双窗口示例构建通过 |

复现入口为 `python scripts/check_gui.py` 和 `python scripts/check_graphics.py`。可直接运行 `tx_build/gui_u0_u1/greeting.exe`、`tx_build/gui_u0_u1/settings.exe`；源码再次编译可用 `tx/txc.exe examples/gui/greeting.tx`，默认产物在 `tx_build/`。

生成的图形/GUI 程序同时链接 Common Controls 与 `tx/link/gui-manifest.o`，清单启用 Common Controls v6 和 Per-Monitor V2。直接从 C++ 链接图形原生实现也必须使用该清单及 comctl32，两个原生验证入口已按此设置。GUI 子系统、无控制台发行体验仍属于后续发行阶段。

本文记录 U0–U1 的原始交付。真实 125%/200% 混合显示器、微软拼音、视觉、实际键盘和最低系统版本仍待人工验收，清单见 [tests/gui/README.md](../tests/gui/README.md)；后续已实现的命令和数据控件见 [U2–U3](gui_u2_u3.md)，U4–U6 尚未实现。
