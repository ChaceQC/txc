# Windows 图形 G0–G1

状态：G0–G1 已实现，定向自动验证通过；真实桌面人工验收待执行（2026-10-01）。完整终态见 [设计](windows_graphics_design.md)。

## 接口与阶段边界

公开接口以 `tx/stdlib/graphics.txh` 为准。G0 登记设计中的全部 11 个不透明资源名，G1 只创建 `graphics_app`、`graphics_window`、`graphics_canvas`。不能用无参构造、整数或其他资源类型伪造；显式 `any` 恢复检查类型。所有图形资源均非 Send/Sync；容器、结构体、闭包捕获和并发容器遵循递归检查。直接或嵌套 `deep_copy` 在实际遇到资源时失败。

G1 提供 `open_app`、两个 `close` 重载、`create_window`、`is_open`、`show`、`set_title`、`window_id`、`invalidate`、`next_event`、`begin_frame`、`end_frame`、`cancel_frame`，以及颜色参数的 `clear`、`draw_line`、`draw_rect`、`fill_rect`、`draw_ellipse`、`fill_ellipse`、`draw_rounded_rect`、`fill_rounded_rect`。`backend(window) -> str` 返回 `hardware`、`software` 或尚未创建目标时的 `uninitialized`。

`window_options(title, width, height, resizable)` 无隐含字段或默认参数；尺寸是客户区 DIP。G1 冻结 `point(x,y)`、`size(width,height)`、`rect(x,y,width,height)`、`color(red,green,blue,alpha)`、`matrix(m11,m12,m21,m22,dx,dy)`。几何与颜色均为 `float`，颜色通道在 `[0,1]`，线宽必须正，负尺寸非法，零面积不画；圆角半径非负并截断到对应半边长。输入记录在调用时快照读取。

事件公共字段固定为 `kind: str`、`window_id: int`、`timestamp_ms: int` 和六个可选载荷：`pointer`、`key`、`text`、`resize`、`timer_id`、`control`。时间从会话开始按单调毫秒计算。

| 载荷 | 固定字段 |
| --- | --- |
| `pointer_event` | `x,y,wheel_x,wheel_y: float`；`button: str`；`shift,ctrl,alt,meta: bool` |
| `key_event` | `key: str`；`scan_code: int`；`shift,ctrl,alt,meta,repeat: bool` |
| `text_event` | `text: str`；`selection_start,selection_length: int`（Unicode 标量索引） |
| `resize_event` | `width,height: float`（DIP）；`pixel_width,pixel_height,dpi: int` |
| `control_event` | U0 扩展为 `source_id: int`、`action: str`、`text: option<str>`、`number: option<float>`、`state: option<bool>`、`item_id/command_id: option<int>`、`revision: int`；详见 [GUI U0–U1](gui_u0_u1.md) |

G1 产生 `paint`、`close_requested`、`closed`、`resized`、`dpi_changed`。`paint` 同窗口合并；其余事件保持入队顺序，关闭通知只发一次。`next_event` 每次至多处理 64 条系统消息，返回一个事件；无事件时消息感知等待，超时为 `-1`、`0` 或正整数。无存活窗口且队列已空立即返回空 option。队列满时先移除可合并的 paint/resized/dpi_changed；仍无空间则返回 `resource_limit`，不静默丢弃离散事件。

## 生命周期与限额

最多一个活动会话，必须在 TX 主线程初始化，COM 使用 STA。窗口默认隐藏，`show` 安排重绘。`close` 幂等，同一资源的所有别名一起失效。只允许一个活动帧，帧内不能取事件、创建窗口或切换绘制目标。窗口关闭、取消帧和提交都使画布失效；最小化、隐藏或客户区为零时开始帧返回空 option。窗口关闭先取消其帧，再释放目标和 HWND；会话关闭逐个关闭窗口、释放 D2D 工厂和 COM。

会话持有窗口控制块，主线程退出钩子兜底关闭。图形值的最终释放只标记待清理控制块，下一次 UI 调用或主线程退出执行原生销毁；GC/工作线程不调用 Win32/COM。显式关闭后存活别名只保留失效状态。失去最后一个画布引用会在下次 UI 调用取消悬空帧。

| 限额 | G0–G1 固定值 |
| --- | --- |
| 同时存活窗口 | 64 |
| 待交付事件 | 4096 |
| 每次消息批次 | 64 |
| 客户区每边物理像素 | 16384（DPI 换算后检查） |
| 窗口标题 UTF-8 字节数 | 65536，拒绝 NUL 和非法 UTF-8 |

设备目标优先硬件，失败回退软件，可通过 `backend` 查询。`EndDraw` 遇到目标丢失返回 `device_lost` 并安排重绘，下次帧重建。G1 不承诺完整设备恢复验收。DPI 在首个 HWND 前声明/校验 Per-Monitor V2，接收 `WM_DPICHANGED` 建议矩形，渲染和尺寸事件使用新 DPI。GUI 子系统仍在后续发行阶段；U0–U1 已为图形/GUI 程序加入 DPI 与 Common Controls v6 清单，控制台示例保留运行时 DPI 校验。

## 错误与 ABI

新增 `error.graphics_error`，公共错误布局仍为 `kind/code/message`，异常类别数值追加为 8，不改变既有值。平台错误的 `code` 为稳定类别；`message` 包含操作和原生数值码，另提供 `last_native_error() -> int` 查询当前线程最近一次 Win32/HRESULT 错误，未发生过为 0。它不要求会话已成功初始化，初始化失败也能直接取得数值码，不需解析中文信息。创建、显示、标题修改、取事件和帧开始/提交返回 `result`；关闭、失效、几何命令的误用抛图形异常。

普通资源值沿用运行时共享值根，缓存原生控制块视图；LLVM 静态选定 `txrt_graphics_*` 函数，记录按已知字段展开为 `double/i64/i1/ptr`。绘图命令直接接收原生画布指针和标量，不做名称查找、动态字典或逐命令装箱。option/result 只在返回边界使用现有状态结构。C ABI 用状态码和 out 参数，异常不穿越 WndProc。

标准图形模块按规范化的标准库路径识别，Linux 在 import 源码处拒绝；本地同名模块仍按普通模块加载。Windows 专用源由 CMake 条件加入，ThinLTO 从相同编译数据库取源；兼容性摘要包含图形公共头和接口。仅链接调用所需系统 import libraries，不分发 Windows 系统图形 DLL。

## 验证记录

2026-10-01 在当前 Windows x64 环境完成：

| 项目 | 结果 |
| --- | --- |
| `scripts/build.ps1 -Incremental` | 普通标准库、编译器、ThinLTO 标准库与兼容性清单构建成功；ThinLTO 使用 LLVM 23.1.2 |
| `python scripts/check_graphics.py` | 全部通过；四类静态拒绝、几何标量 ABI、普通/ThinLTO 两次 TX 行为运行、异常退出、原生生命周期和双窗口示例构建 |
| 原生生命周期 | COM MTA/STA 冲突的数值错误码、跨线程拒绝、客户区 DIP 换算、最小化/恢复、缩放、模拟 192 DPI 消息、队列合并/溢出、80 ms 超时等待、悬空帧取消、退出后 HWND 销毁通过 |
| 非图形程序 | `examples/overload.tx --no-lto` 构建运行通过；PE 导入表未引入 D2D、OLE32、USER32 图形依赖 |
| Linux | 已加入按真实标准库路径识别的 import 平台诊断；本轮未执行 Linux 构建/运行 |

验证程序与 IR 位于 `tx_build/graphics_g0_g1/`，已构建的交互示例为 `two_windows.exe`。复现命令：

```powershell
.\tx\txc.exe .\examples\graphics\two_windows.tx
.\tx_build\two_windows.exe
```

人工清单见 [tests/graphics/README.md](../tests/graphics/README.md)。多显示器混合 DPI、实际拖动与遮挡恢复、远程桌面/软件渲染、最低 Windows 版本仍待验收。自动发送尺寸/DPI 消息及最小化恢复验证，不等同于真实桌面视觉验收。
