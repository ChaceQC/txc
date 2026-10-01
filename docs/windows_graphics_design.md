# Windows 图形库设计

状态：G0–G1 已实现并通过定向自动验证，人工桌面验收待执行；G2–G5 仍为设计提案。本文定义 TX 的 Windows 图形库终态、接口草案和分阶段交付要求；当前交付范围和验证记录见 [G0–G1 接口](graphics_g0_g1.md)。日期：2026-10-01。

## 1. 目标与范围

提供可用于教学绘图、数据可视化、二维交互程序以及 Windows 桌面工具的图形库。用户通过 `.txh` 导入，编译后直接运行原生程序，不要求安装 Python、浏览器、.NET 或额外 GUI 框架。

基础能力包括多窗口、二维几何、路径、颜色和渐变、图片、中文文字排版、鼠标键盘与输入法、定时刷新、离屏绘制和图片导出。桌面工具通过独立的原生控件模块获得按钮、输入框、列表、菜单和系统对话框。

首个支持目标为 Windows 10 1703 及以上和 Windows 11，x64；选择该基线是为了统一使用 Per-Monitor DPI Awareness V2。实际最低系统版本仍需结合现有发行依赖在对应机器验收，不把 API 可用等同于整包兼容。ARM64、Linux 图形后端、3D、音视频和完整游戏引擎不在本设计交付范围。

不增加 `enum`、`match`、属性宏或新的闭包语法。需要增加的是图形资源的内置不透明类型、标准库符号及平台能力检查，语言既有语法保持不变。

## 2. 当前仓库依据

| 已有机制 | 本设计如何使用 |
| --- | --- |
| [模块导入](modules.md)：接口查找、别名、规范路径与内部唯一符号 | 保留 `import "graphics.txh" as gfx`，不通过导入文本或别名猜测标准库身份 |
| [option/result](option_result.md)：类型化状态与可恢复错误 | 等待事件使用 `option`；创建、解码、提交等使用 `result` |
| [Send/Sync](send_sync.md)：不透明句柄默认不可跨线程 | 窗口和画布绑定 UI 线程；后台只交付满足 Send 的数据 |
| `tx/stdlib/file_stream.txh`：编译器识别资源类型，接口只声明函数 | 图形句柄沿用资源类型接入方式，不公开整数指针 |
| `src/backend/llvm/codegen_external_direct.cpp`：静态外部调用选择 | 直接生成图形专用 C ABI 调用 |
| `src/driver/native_toolchain_windows.cpp`：随包链接器和标准库链接 | 增补图形依赖、GUI 子系统和清单资源的交付 |

G0 已为 `error.txh` 增加 `graphics_error`，登记图形资源内置类型，并接入专用 C ABI、平台检查和两种标准库产物；完整实现与验证范围见 G0–G1 接口文档。

## 3. 技术选型

| 职责 | 选择 | 原因与边界 |
| --- | --- | --- |
| 窗口与消息 | Win32 Unicode API | 原生多窗口、输入、DPI、菜单与系统对话框 |
| 二维绘制 | Direct2D，基础实现使用 HWND render target | 支持抗锯齿、路径、裁剪和离屏目标；暂不引入 D3D 交换链复杂度 |
| 文字 | DirectWrite | 中文、字体回退、双向文本、测量与命中测试；不自行拼接字符宽度 |
| 图片 | Windows Imaging Component（WIC） | PNG/JPEG/BMP 解码及 PNG 导出，无需打包额外图片解码库 |
| 标准控件 | Win32 + Common Controls v6 | 优先复用键盘导航、辅助功能和 IME 行为 |
| 原生资源管理 | C++23 RAII | HWND、COM、绘图对象均有明确销毁责任 |

Direct2D 创建目标时优先硬件，失败时允许软件 render target；软件回退成功应提供可查询的后端信息。两条路径均失败才返回错误。基础版本不承诺垂直同步、HDR 或逐像素跨设备一致。

GDI 只用于必要的系统互操作。SDL/SFML 更适合跨平台游戏窗口；Qt 对当前原生二维标准库目标引入较大构建与分发成本；WebView2 适合网页 UI，但不作为本库底层。将来可以另立这些集成模块，不预先建立通用插件后端。

## 4. 模块与目录

| 拟新增接口 | 职责 |
| --- | --- |
| `graphics.txh` | 会话、窗口、事件、画布、形状、路径、画刷、图像与离屏绘制 |
| `graphics_text.txh` | 字体、布局、测量、命中测试、文本选择范围几何 |
| `gui.txh` | 标准控件、布局、菜单、剪贴板和系统文件对话框 |

`graphics_text` 和 `gui` 导入 `graphics`；基础模块不反向依赖它们。共享记录类型只定义一次。首期不设一个返回 `any` 的通用 `get/set/call` API。

建议实现目录：

| 路径 | 职责 |
| --- | --- |
| `src/stdlib/graphics/` | 公共值与资源定义、参数规则、绘图和事件领域逻辑 |
| `src/stdlib/graphics/windows/` | Win32、Direct2D、DirectWrite、WIC 具体实现 |
| `src/stdlib/gui/windows/` | 控件、对话框和辅助功能互操作 |
| `src/backend/cpp/graphics_*_abi.cpp` | 图形专用 C ABI、状态与错误桥接 |
| `src/backend/llvm/` | 类型映射、函数绑定和直接调用生成 |
| `tx/stdlib/` | 公开 `.txh`，不放 C++ 实现 |
| `examples/graphics/` | 后续实现时交付可运行示例 |
| `tests/graphics/` | 后续定向验证和手动验收说明 |

标准库 C++ 实现仍进入现有 `libtxstdlib.a` 及 ThinLTO 产物。按对象文件拆分图形模块，未使用图形的程序不拉入图形实现，不自动创建窗口或初始化 COM。

## 5. 类型、所有权与错误

### 5.1 类型模型

新增不透明资源类型：`graphics_app`、`graphics_window`、`graphics_canvas`、`graphics_image`、`graphics_surface`、`graphics_path`、`graphics_brush`、`graphics_font`、`graphics_text_layout`、`gui_control`、`gui_menu`。这些名称由编译器登记，用户不能调用无参构造器伪造资源。

`point`、`size`、`rect`、`color`、`matrix` 等使用公开结构体。TX 现有结构体具有共享语义，因此 API 必须在调用时快照读取字段，不能在后台持有调用者可修改的结构体。坐标使用 `float`；颜色通道使用 `[0, 1]` 的 `float`，顺序为 RGBA。

```tx
# graphics.txh 中拟公开的记录；不是新增语法。
struct point
{
    x: float
    y: float
}

struct rect
{
    x: float
    y: float
    width: float
    height: float
}

struct color
{
    red: float
    green: float
    blue: float
    alpha: float
}

struct window_options
{
    title: str
    width: float
    height: float
    resizable: bool
}
```

所有图形资源首期均不声明 Send/Sync。普通赋值共享同一资源，关闭后所有别名失效；`close` 幂等。`deep_copy` 遇到图形资源，包括嵌套在容器内的资源，明确报错；图片独立副本通过专用快照 API 产生。

窗口属于会话，窗口画布属于窗口，离屏画布属于 surface；画布持有帧状态和资源代际。关闭窗口会废弃当前帧、销毁子控件、释放目标，并使既存画布失效。保存下来的画布不能用于下一帧。不同会话的资源混用报 `wrong_owner`。

首次初始化必须在 TX 主线程，进程最多一个活动图形会话。应用显式关闭时按“帧 → 子控件/窗口 → 设备资源 → COM”释放；同时注册主线程上下文退出清理。GC 的最终释放只能转交 UI 线程待销毁队列，不能在 GC 或工作线程调用 DestroyWindow。关闭整个会话后遗留别名只保留无效控制块。

### 5.2 错误契约

增加 `error.graphics_error`，沿用 `kind/code/message`，并接入异常匹配、`result.error()` 和源码位置诊断。OS 错误记录原生错误码及操作上下文，不能要求用户解析中文 message。

| code | 触发场景 |
| --- | --- |
| `unsupported_platform` | 当前目标平台无图形实现 |
| `invalid_argument` | 非有限数、负尺寸、非法颜色、无效 UTF-8 或不支持的参数值 |
| `wrong_thread` / `wrong_owner` | 调用线程或所属会话不匹配 |
| `closed_resource` / `invalid_frame` | 关闭后使用、帧外绘图、重复提交 |
| `device_lost` / `backend_unavailable` | 目标丢失或硬件和软件目标都不可用 |
| `decode_failed` / `resource_limit` | 图片损坏、解码或队列超出限额 |
| `platform_error` | 其他 Win32/COM 操作失败 |

创建、文件解码、帧开始/提交和系统交互等可失败操作返回 `result<T>`；绘图命令的非法参数、过期资源和线程错误按现有异常机制抛出。Direct2D 延迟报告的绘制错误在 `end_frame` 返回。析构不抛异常。无事件、用户取消、窗口暂不能绘制分别用空 `option` 表示，不伪装成失败。

## 6. 窗口与事件循环

核心采用由 TX 主线程主动取事件的模型。Win32 WndProc 只处理必要系统协议、更新内部状态和排队，不直接执行 TX 函数，C++ 异常不得穿越 WndProc。

### 6.1 核心接口草案

以下声明省略所属模块前缀；窗口、事件和绘制接口位于 `graphics.txh`。公开资源类型需先完成编译器登记。

```tx
def open_app() -> result<graphics_app>
def close(app: graphics_app) -> void
def create_window(app: graphics_app, options: window_options) -> result<graphics_window>
def close(window: graphics_window) -> void
def is_open(window: graphics_window) -> bool
def show(window: graphics_window) -> result<void>
def set_title(window: graphics_window, title: str) -> result<void>
def window_id(window: graphics_window) -> int
def invalidate(window: graphics_window) -> void
def next_event(app: graphics_app, timeout_ms: int) -> result<option<event>>
def begin_frame(window: graphics_window) -> result<option<graphics_canvas>>
def end_frame(canvas: graphics_canvas) -> result<void>
def cancel_frame(canvas: graphics_canvas) -> void
```

窗口默认隐藏，便于配置后一次展示；`show` 后安排初始重绘。宽高是客户区 DIP，不包含标题栏；初始尺寸必须为正。`next_event` 的 `timeout_ms` 为 `-1` 无限等待、`0` 非阻塞或正整数超时，其余值非法。无存活窗口且队列清空时立即返回空值，应用自行结束或创建新窗口。

一次调用先处理有限批次系统消息，再交付一个事件，避免输入洪泛饿死应用逻辑。等待使用消息感知等待，不使用忙轮询。一次仅允许一个活动帧，活动帧内禁止再次取事件、显示模态对话框或切换目标。

### 6.2 事件数据与顺序

`event` 是具有固定字段的普通记录：`kind: str`、`window_id: int`、`timestamp_ms: int`，以及 `pointer: option<pointer_event>`、`key: option<key_event>`、`text: option<text_event>`、`resize: option<resize_event>`、`timer_id: option<int>`、`control: option<control_event>`。非对应载荷为空；不使用动态字典。时间戳来自单调时钟，以会话起点为零。

这里的字符串表示外部事件种类，不用于查找或执行函数。调用目标、字段访问和载荷类型仍在编译期确定。当前没有枚举和判别联合语法，暂用这种显式记录；后续语言增加枚举时再单独设计迁移。

| kind | 载荷与语义 |
| --- | --- |
| `paint` | 重绘请求；同窗口合并，不携带失效的 OS 绘图句柄 |
| `close_requested` | 用户请求关闭；程序调用 `close` 接受，忽略即拒绝 |
| `closed` | 窗口已销毁后的单次通知；ID 不再对应活动窗口 |
| `resized` / `dpi_changed` | 客户区 DIP、物理像素尺寸和新 DPI；更新状态后入队 |
| `pointer_moved` / `pointer_down` / `pointer_up` / `wheel` | 客户区 DIP 坐标、按钮、修饰键、滚轮增量 |
| `key_down` / `key_up` | 稳定键名、原生扫描码、修饰键及重复标记；不充当文本输入 |
| `text_input` / `composition` | 已提交 UTF-8 文本或输入法预编辑快照 |
| `focus_gained` / `focus_lost` | 焦点状态；失焦时清空内部按键及捕获状态 |
| `timer` | 定时器 ID；重复到期合并，不无限积压 |
| `control` | 控件 ID、动作和类型化可选文本/数值载荷 |

窗口 ID 和控件 ID 在会话内不复用，不公开 HWND；定时器也使用会话唯一 ID。未发生合并的事件保持入队顺序；移动事件只合并连续同窗口移动，不能跨越按下/释放边界。单次解码、窗口数量、图片内存和事件队列均必须设置限额；默认事件上限建议 4096，可合并事件优先腾出容量。若离散输入仍不能入队，清空按键/捕获状态并让下一次取事件返回 `resource_limit`，不得静默漏掉 key_up 后留下永久按下状态。

`WM_PAINT` 内及时完成 BeginPaint/EndPaint 验证失效区域，同时入队 `paint`。实际 D2D 绘制由之后的 TX 帧进行；保留上一帧显示直到新帧提交。最小化或客户区零尺寸时 `begin_frame` 成功返回空 option，恢复后自动请求重绘。

## 7. 绘图契约

### 7.1 坐标与颜色

原点在客户区左上，x 向右、y 向下；几何与字体大小均为 DIP，`1 DIP = 1/96 英寸`。屏幕像素尺寸为 `DIP × DPI / 96`，输入事件反向转换一次。DPI 改变时采用系统建议窗口矩形、调整渲染目标和资源，再交付新 DPI 事件。

矩形使用 x/y/width/height，负宽高非法，零面积不绘制；线宽必须大于零。变换采用六分量仿射矩阵，接口须固定乘法顺序为“当前矩阵 × 新矩阵”，避免调用者猜测平移缩放先后。

公开颜色为 sRGB 通道和直通 alpha；上传及内部混合转换成预乘 alpha，默认 source-over。导出 PNG 转回直通 alpha，alpha 为零时 RGB 归零。基础窗口不支持透明桌面合成；透明内容绘制到离屏 surface。

### 7.2 接口组

| 接口组 | 终态能力及规则 |
| --- | --- |
| `clear(canvas, color)` | 清空整个目标，不受当前变换和裁剪影响 |
| `draw_line`、`draw_rect`、`fill_rect` | 类型化点、矩形、画刷和线宽 |
| `draw_ellipse`、`fill_ellipse`、圆角矩形 | 椭圆使用包围矩形，圆角半径非负且限制于半边长 |
| `create_path`、`move_to`、`line_to`、`bezier_to`、`close_path`、`finish_path` | 路径先构建后冻结，绘制要求已冻结；填充规则显式指定 |
| `solid_brush`、`linear_gradient`、`radial_gradient` | 渐变位置范围为 `[0,1]`，至少两个有序色标 |
| `save`、`restore`、`set_transform`、`clip_rect`、`clip_path` | 保存/恢复变换与裁剪；栈下溢立即报错，帧结束栈不平衡报错 |
| `draw_image` | 源矩形为图片像素，目标矩形为 DIP；最近邻/线性插值显式选择 |
| `create_surface`、`begin_surface_frame`、`snapshot` | 离屏目标以像素尺寸和 DPI 创建；快照为独立不可变图片 |
| `save_png` | 保存图片或 surface 快照；默认拒绝覆盖，显式参数允许覆盖 |

绘图主路径使用命令即时调用与后端批处理，不建立由 TX 字符串命令组成的解释器。图片、路径、画刷和文字布局可缓存复用；每帧不重新解码图片或创建字体。

### 7.3 设备丢失

`end_frame` 遇到 `D2DERR_RECREATE_TARGET` 时废弃当前帧，返回 `device_lost`，释放设备相关缓存并请求下次重绘。应用收到可恢复错误后重新绘制完整场景，不假定上一帧已呈现。

字体描述、已冻结路径、原始解码像素等 CPU 资源保留；目标相关画刷/位图按新代际重建。普通窗口不自动保存用户绘图命令。离屏 surface 内容丢失时增加 `surface_lost` 状态：必须由应用重画；只有已完成的独立图片快照保证内容保留。不能把清零后的 surface 当作原内容导出。

## 8. 文字、图片与输入

`graphics_text.txh` 设计 `create_font(app, family, size)`、`create_layout(font, text, width, height)`、`draw_layout(canvas, layout, origin, color)`、`measure(layout)`、`hit_test(layout, point)` 和 `selection_rects(layout, start, end)`。其中 font/layout 返回 `result`，measure 返回固定字段记录，selection_rects 返回 `vector<graphics.rect>`。

字体名称和文本严格按 UTF-8 接收，内部转为 UTF-16。对外文本索引采用与现有 TX 字符串一致的 Unicode 标量位置，内部维护 UTF-8、UTF-16 和排版 cluster 映射；禁止返回一半代理对的位置。光标移动和删除按字素边界，排版命中点吸附到有效 cluster；双向文字选择允许多个矩形。截断、换行、对齐、行距和 locale 后续通过类型化 options 记录设置。

自绘文字输入需提供启用/停用输入、更新插入点矩形、预编辑区间和提交事件的接口。仅处理 `WM_CHAR` 不算完成 IME；必须处理组合开始/更新/结束、取消、候选窗定位及 DPI 变化。普通桌面表单优先使用原生输入控件；自绘编辑器的完整辅助功能需要专门实现，不能凭文字绘制接口宣称已支持。

图片接口分为文件解码、内存 bytes 解码和像素构造。首轮固定支持 PNG/JPEG/BMP，动画图片不自动播放。建议单边上限 16384 像素、单图解码内存上限 256 MiB，乘法先检查溢出；检查尺寸后再分配，不能只检查压缩文件大小。默认忽略 EXIF 旋转时必须在文档说明；终态采用应用 EXIF 朝向后返回尺寸的统一行为。

图片色彩终态统一转换到 sRGB；不支持的色彩配置返回明确错误，不悄悄按其他格式解释。WIC 编码器写入临时同目录文件，成功后替换目标；失败删除临时文件并保留旧文件。

后台任务仅传递路径、bytes 或类型化计算结果；UI 线程创建目标图片资源。后台解码扩展需要独立 COM 初始化和可跨线程的只读像素载荷审计，不直接把 UI 图片句柄标为 Send。

## 9. 原生 GUI 模块

完整控件、布局、事件、数据模型、辅助功能与接口示例见 [Windows GUI 库设计](windows_gui_design.md)。本文保留图形与 GUI 的边界概要；具体 GUI 类型、依赖顺序和 U0–U6 交付要求以该专项设计为准。

`gui.txh` 在图形核心稳定后实施，提供按钮、标签、单/多行输入、复选框、下拉框、列表和表格、滚动区域、进度条、菜单/快捷键、消息框、文件打开/保存框及剪贴板文本。

第一套布局采用行、列、网格，具有 margin、padding、gap、固定/自适应/剩余空间三种尺寸分配。使用明确记录和函数配置，布局变更批量应用；不引入 CSS 解释器。控件保留稳定 ID、父子关系和独立 HWND；parent 关闭时递归销毁，跨窗口移动禁止。

控件事件进入同一事件队列，单个控件通过 ID 路由。原生控件覆盖在父窗口绘图区之上，不支持与任意 D2D 路径交错透明混合；需要复杂混合的区域使用独立自绘控件。默认使用系统主题与 Common Controls v6，不承诺所有原生控件统一暗色。

终态验收必须包含 Tab 焦点顺序、访问键、键盘操作、屏幕阅读器可读名称/状态、系统高对比度和 200% 缩放。标准控件复用系统辅助功能，自绘组件补充 UI Automation provider。

文件对话框使用 `result<option<str>>` 表示成功路径、用户取消和失败，调用只允许在 UI 线程且无活动帧。系统模态循环可以处理 OS 消息，但期间不回调 TX；关闭和重绘等事件继续排队，并遵守有界队列策略。文件框仅选择路径，不负责写入或覆盖文件。

## 10. 使用示例草案

以下程序展示 G1 的最小体验。`event` 的字段见第 6 节，`clear`、`fill_rect` 为第 7 节接口；为突出主流程，失败结果使用 `.value()` 交给统一异常机制，退出清理由会话保护机制兜底。包含双窗口和 `device_lost` 处理的正式示例见 `examples/graphics/two_windows.tx`。

```tx
import "graphics.txh" as gfx

def main() -> int
{
    auto app = gfx.open_app().value()
    auto options = gfx.window_options("TX 图形窗口", 800.0, 600.0, true)
    auto window = gfx.create_window(app, options).value()
    gfx.show(window).value()

    while gfx.is_open(window)
    {
        auto pending = gfx.next_event(app, -1).value()
        if pending.is_some()
        {
            auto event = pending.value()
            if event.kind == "close_requested"
            {
                gfx.close(window)
            }
            else if event.kind == "paint"
            {
                auto frame = gfx.begin_frame(window).value()
                if frame.is_some()
                {
                    auto canvas = frame.value()
                    gfx.clear(canvas, gfx.color(0.96, 0.97, 1.0, 1.0))
                    gfx.fill_rect(canvas, gfx.rect(40.0, 40.0, 240.0, 120.0),
                        gfx.color(0.15, 0.45, 0.90, 1.0))
                    gfx.end_frame(canvas).value()
                }
            }
        }
    }
    gfx.close(app)
    return 0
}
```

`fill_rect` 终态设计 `color` 和 `graphics_brush` 两个重载，编译期选择入口；G1 已提供颜色重载。正式双窗口示例已检查提交结果的 `device_lost` code，等待重绘后重建整帧，其他错误照常传播。

动画通过定时器请求 `invalidate`，使用单调时间计算真实帧间隔；不在死循环里持续画图。默认建议 60 Hz 上限，最小化暂停刷新，恢复时重置动画时间基准；慢帧合并通知，不积压历史画面。定时器只表达调度意图，不保证固定 FPS。

## 11. 编译器与发行接入

1. 在 `src/common` 登记资源类型、错误种类；模块解析器和语义层识别标准图形接口身份。更新容器嵌套、类型比较、转换、Send/Sync 和 deep_copy 规则，不能仅在“直接调用 thread.spawn”一处拦截。
2. LLVM 后端生成 `txrt_graphics_*` 等固定符号调用。标量走明确 ABI 类型，记录使用固定布局或明确字段参数；绘图热路径不通过 `any`、字符串查找或逐调用类型分派。
3. C ABI 使用状态码和 out 参数承接现有错误边界；native resource 指针只在内部传递，外部不能转为 `int`。在动态 `any` 显式恢复资源时检查类型一次，不让普通静态绘图重复检查类型。
4. 在图形实现内部检查真正动态的资源存活、线程、所有者、帧代际和系统错误；这些检查不因静态调用而省略。
5. Windows 构建按需补齐 `user32`、`gdi32`、`d2d1`、`dwrite`、`windowscodecs`、`ole32`、`uuid`、`comctl32`、`shell32`、`comdlg32`、`imm32`，由最终调用符号核实，避免盲目复制系统 DLL。对 MinGW 头文件、COM 接口和 import library 做小型构建验证。
6. 普通构建与 ThinLTO 必须同时纳入新源文件、依赖和兼容性摘要；`tx/link/` 包含所需 import libraries，目标机使用 Windows 自带图形 DLL。现有 C++ runtime DLL 分发策略不变。
7. 新增计划中的 `--subsystem console|windows`，默认 console。windows 模式仍调用 TX `main`，明确配置匹配的 CRT 入口和 PE subsystem，嵌入 DPI V2 与 Common Controls v6 清单。不得仅去掉控制台却遗漏启动入口与清单资源。
8. GUI 模式保留可用的重定向标准流；没有标准流时，未捕获错误写入诊断日志并显示精简错误框。默认不隐藏错误或静默退出。日志位置和失败回退须写入使用文档。
9. Linux 构建不引用 Windows 头文件和库。首期对标准图形模块给出“当前目标平台不支持 Windows 图形库”的源码位置诊断；现有非图形 Linux 程序仍正常编译。

COM 会话初始化为 STA；已有不兼容 apartment 时返回明确错误，不在隐藏线程偷偷创建窗口。每次成功的 COM 初始化与释放配对。DPI 优先由生成程序清单声明；运行时初始化仅作受控校验/回退，不能在 HWND 创建后强行改变全进程 DPI 模式。

## 12. 实施顺序与验收

各阶段均交付公开接口、实现、示例、错误说明和对应验证记录。下表为阶段完成条件；G0–G1 的当前自动验证结果与人工待验收项见 [验证记录](graphics_g0_g1.md#验证记录)。

| 阶段 | 交付 | 完成条件 |
| --- | --- | --- |
| G0：接入与资源 | 内置资源类型、错误桥、平台门禁、普通/ThinLTO 链接 | 错类型调用与跨线程捕获在源码处拒绝；关闭后别名报错；非图形构建不受影响 |
| G1：窗口与二维基础 | 会话、多窗口、队列、帧、基础几何、DPI | 两窗口独立关闭；重绘/缩放/最小化正确；空闲等待不忙轮询；异常退出无遗留窗口 |
| G2：文字和图片 | DirectWrite、WIC、路径/渐变、离屏/PNG | 中文/emoji/双向文本、中文路径、alpha、限额和导出失败保持原文件均符合契约 |
| G3：交互与恢复 | 键鼠、IME、捕获、定时器、设备重建 | 组合输入不重复；失焦无卡键；拖动与 DPI 切换正确；设备丢失可重画 |
| G4：桌面工具 | 控件、布局、菜单、对话框、辅助功能 | 表单与绘图区共存，键盘可用，取消正常，200% DPI 与屏幕阅读器可验收 |
| G5：发行 | GUI 子系统、资源清单、示例与最终文档 | 无开发环境的 Windows 目标机可运行；两种链接产物齐全；最低系统基线得到实测 |

定向自动验证优先覆盖：几何/像素换算、UTF 索引映射、事件顺序、句柄失效、资源限额、编译期静态类型与 IR 直接调用。对像素结果采用容差检查，不把不同 GPU 的抗锯齿差异当作语义失败。

必须人工验收的项目包括多显示器混合 DPI、微软拼音输入法、拖动缩放、窗口遮挡恢复、远程桌面/软件渲染、无控制台错误提示、高对比度和辅助功能。未实际执行前一律标为待验收。

性能只选代表场景记录：空闲窗口、1000 个矩形、缓存文字布局、图片缩放和连续调整窗口大小；报告硬件、尺寸、DPI、后端、帧时间与资源峰值。没有实测前不写“稳定 60 FPS”。

## 13. 实施前需冻结的细节

本提案已经确定平台技术、模块边界、主要语义和交付顺序。G0 开始时进一步冻结每个 options/事件载荷的完整字段、默认限额、API 签名表、错误种类迁移点与资源类型 ABI；G2 冻结颜色管理、字体回退和排版选项；G4 冻结控件集合与布局记录。每次冻结先改本文及接口文档，再写实现。

G0–G1 的冻结结果见 [接口、限额与 ABI](graphics_g0_g1.md)。本阶段只产生窗口生命周期、重绘、尺寸和 DPI 事件；其余事件的记录字段先固定，实际输入与控件行为分别由 G3/G4 交付。基础几何提供颜色重载，画刷重载随 G2 交付。

优先交付 G0–G3，形成独立可用的 Windows 二维图形库；G4–G5 完成桌面工具能力和发行验收。不能用“能显示一个窗口”代替整个图形库完成，也不能把设计示例计为已通过的程序。
