# Windows GUI 库设计

状态：U0–U1 已实现并通过定向自动验证，真实桌面人工验收待执行；具体接口与记录见 [GUI U0–U1](gui_u0_u1.md)。U2–U6 仍为设计。日期：2026-10-02。本文是 [Windows 图形库设计](windows_graphics_design.md)中 GUI 部分的详细契约，目标是使 TX 能编写完整的 Windows 桌面工具，而不仅是显示绘图窗口。

## 1. 目标与技术边界

首个目标为 Windows x64 原生桌面程序，平台基线与图形库一致。覆盖表单、设置页、文件管理、表格浏览、后台任务进度和自绘可视化区域。使用 Win32、Common Controls v6、系统对话框及 DirectWrite/Direct2D，随标准库静态链接，复用现有运行时分发。

采用保留控件树：程序创建控件、更新属性，由库负责布局、系统消息和重绘。普通按钮与输入框使用系统控件，自绘仅用于画布和系统未提供的明确能力。默认跟随系统字体、颜色和高对比度设置；统一暗色皮肤不作为原生控件的默认承诺。

不要求新增 TX 语法、反射、运行时类名注册、HTML/CSS 或 JavaScript。完整富文本编辑器、浏览器、跨平台后端、可视化界面设计器和插件市场不在本次范围。布局与模型层保留清晰职责，但不为尚未实现的平台引入抽象后端体系。

## 2. 与图形库的关系

| 层 | 责任 |
| --- | --- |
| `graphics.txh` | 图形会话、顶层窗口、消息等待、绘图资源、帧与 DPI |
| `gui.txh` | 控件树、布局、属性、焦点、快捷键和控件事件 |
| `gui_data.txh` | 列表、表格、树的类型化模型与批量更新 |
| `gui_dialog.txh` | 文件选择、消息框、自定义模态窗口和剪贴板文本 |
| `graphics_text.txh` | 自绘区域的文字排版和命中测试 |

`gui_data` 和 `gui_dialog` 依赖 `gui`，`gui` 依赖 `graphics`；`graphics` 不导入 GUI。控件事件公共记录只在图形基础接口中定义一次，避免接口循环。应用使用同一个 `graphics_app` 和 `graphics.next_event`，不再创建第二个 UI 消息循环。

顶层窗口沿用 `graphics_window`，无需重复创建 `gui_window`。每个窗口最多一个 GUI 根容器，根容器占满客户区；独立绘图区使用 `gui_canvas` 子控件。在有 GUI 根容器的窗口上绘制顶层背景只能影响控件后面的区域。

图形文档中的“G4 GUI”由本文 U0–U6 细化。GUI 基础开发依赖图形 G0–G1，文本输入增强和画布整合再依赖 G2–G3，不必等所有高级路径与图片功能完成后才开始写按钮。

## 3. 资源类型与生命周期

### 3.1 公开类型

编译器新增不透明类型：`gui_container`、`gui_label`、`gui_button`、`gui_text_box`、`gui_check_box`、`gui_combo_box`、`gui_list_view`、`gui_table_view`、`gui_tree_view`、`gui_progress_bar`、`gui_slider`、`gui_tabs`、`gui_canvas`、`gui_menu`、`gui_command`、`gui_list_model`、`gui_table_model`、`gui_tree_model`。

这些类型不是公开字段结构体，不允许伪造构造、整数转换或动态创建。使用具体类型使 `set_checked(text_box, true)` 等调用在编译期报错。

共用操作以有限、显式的重载提供，例如 `set_enabled`、`set_visible`、`set_bounds`、`set_accessibility`、`id` 和 `close`。内部可以共享 C++ 实现，但不能将所有类型擦除为 `any` 再逐次判断。图形概要中的泛称 `gui_control` 不作为万能属性入口；确有集合需求时使用稳定 ID 查询状态，不通过 ID 调用任意方法。

### 3.2 所有权

- 容器拥有其子控件，窗口拥有根容器；TX 局部变量释放不会把仍挂在树上的控件销毁。
- `close(control)` 从树中移除并销毁原生资源，幂等；父节点关闭递归关闭子节点，所有别名随即失效。
- 模型由视图和 TX 引用共同持有；关闭一个视图不关闭其他视图共用的模型。显式关闭模型若仍绑定视图则报 `model_in_use`，先解绑或关闭视图。
- 所有 GUI 资源均非 Send/Sync，默认复制共享底层对象，`deep_copy` 包含它们时失败。
- ID 是会话内不复用的 `int`，不是 HWND。关闭后已排队事件可保留为历史快照，程序必须允许忽略失效 ID。
- 设置父节点只允许在创建时完成。动态重排可改变同一容器内顺序；跨窗口迁移需重建，避免焦点、原生父子关系和 DPI 状态失配。

显式销毁和异常退出清理都在 UI 线程执行，遵循图形会话的主线程退出保护。原生父窗口销毁带来的子 HWND 通知与资源关闭必须幂等处理，避免二次 DestroyWindow。

## 4. 控件终态清单

| 控件 | 主要能力 | 原生实现与限制 |
| --- | --- | --- |
| label | 文本、自动换行、标签关联 | Static；可关联输入框的访问键和辅助名称 |
| button | 文本、默认按钮、取消按钮 | Button；激活同时支持鼠标和键盘 |
| check_box | 二态/三态 | Button；状态为 `off/on/mixed`，二态拒绝 mixed |
| radio group | 单选组 | Button；由容器中的组 ID 保证互斥 |
| text_box | 单/多行、只读、密码、长度限制、选择 | Edit；单行和多行模式在创建后不切换 |
| combo_box | 单选、可编辑模式、选项 ID | ComboBox；选择与编辑文本为不同状态 |
| list_view | 单/多选、图标/文本、稳定项 ID | ListView；大量数据使用模型快照 |
| table_view | 列、行、排序请求、选择、虚拟显示 | Report ListView；基础单元格显示文本，不承诺嵌套任意控件 |
| tree_view | 展开、选中、延迟载入 | TreeView；展开请求不能同步调用 TX 获取数据 |
| progress_bar | 有界进度、不确定进度 | Progress；范围检查，不把负值表示特殊状态 |
| slider | 范围、步长、当前值 | Trackbar；连续拖动与提交分开通知 |
| tabs | 页面、选择、键盘切换 | Tab + 页面容器；隐藏页退出布局和 Tab 导航 |
| scroll panel | 单轴/双轴滚动 | 容器 + 滚动条；逻辑滚动量以 DIP 保存 |
| split panel | 分栏、分隔条、最小尺寸 | 自有容器；必须提供键盘调整和辅助功能 |
| canvas | 自绘区域、输入与绘图帧 | 独立子 HWND + Direct2D target |
| menu / status bar / tool bar | 命令、状态与工具入口 | Win32 菜单与 Common Controls |

可变长度数据使用 `vector<T>`，不以异构 `array` 或 `dict` 作为默认 API。可选数值、选择和取消使用 `option`，不用 `-1`、空字符串或空控件伪装缺失。

## 5. 布局系统

### 5.1 布局对象

容器提供 `column`、`row`、`grid`、`overlay`、`scroll` 五种布局。布局是控件树的一部分，不依赖每帧重建。手工定位作为容器的显式 `absolute` 模式提供，不能在自动布局容器里偷偷覆盖某个子控件位置。

尺寸策略用记录 `length` 表达：`mode: str` 和 `value: float`，由 `fixed(value)`、`auto_length()`、`stretch(weight)` 工厂构造。mode 只接受 `fixed/auto/stretch`；工厂不引入新语法。固定值非负，权重为有限正数。

每个子项有 width/height 策略、min/max 尺寸、margin、水平/垂直对齐；容器有 padding 和 gap。网格另外提供行列轨道与子项行列位置、跨度，越界或非正跨度立即报错。overlay 按显式子项顺序确定层叠，仅适用于符合原生 HWND 层叠限制的内容。

### 5.2 测量与分配

1. 以可用 DIP 尺寸、控件内容与字体测量子项。自动尺寸结合文本测量、控件边框和系统主题指标计算。
2. 先分配固定项、auto 项和间距，再按权重分配剩余空间，并应用 min/max。达到上限的项退出权重分配，重新分配剩余空间。
3. 空间不足时优先收缩允许收缩的项至最小值；仍不足则裁剪或由 scroll 容器滚动，不生成负尺寸。
4. 网格跨度项的最小需求分配到覆盖的可伸缩轨道；全部为固定轨道时不擅自改轨道尺寸，内容按裁剪规则处理。
5. 最后统一 DIP 转像素。按累计边界取整分配余数，使相邻边界一致，避免 125% DPI 下逐项取整造成缝隙。

可滚动方向对子项提供无界测量时，`stretch` 在该方向按 auto 处理，避免“内容高度取决于剩余高度”的循环。嵌套布局必须有递归深度限额和明确失败诊断。

`set_visible(false)` 隐藏并退出布局；`set_enabled(false)` 仍占空间但不参与用户操作。需要隐藏但保留位置时使用独立的 `set_reserved_space` 选项，不把两种行为混在同一个布尔值里。

属性修改标记脏节点，在交付下一次事件或开始绘制前集中重新布局。`flush_layout(window)` 可显式要求同步计算；活动绘图帧或原生通知重入期间调用则失败。布局变更通过批量窗口定位提交，减少闪烁。

## 6. 事件、命令与状态同步

### 6.1 事件模型

控件事件进入图形库 `event.control`，使用固定记录：`source_id: int`、`action: str`、`text: option<str>`、`number: option<float>`、`state: option<bool>`、`item_id: option<int>`、`command_id: option<int>`、`revision: int`。额外的复杂选择通过视图查询获取类型化快照。

| action | 契约 |
| --- | --- |
| `activated` | 按钮、菜单或快捷键激活；具有命令时同时带 command_id |
| `text_changed` | 用户编辑后的完整 UTF-8 快照；同控件连续编辑通知可合并 |
| `text_committed` | 单行 Enter 或显式提交；IME 尚在组合时不提交 |
| `selection_changed` | 选择已经更新；稳定项 ID 或由查询取得的 ID 向量 |
| `check_changed` | 状态已更新；三态通过专门状态查询，bool 载荷只用于二态 |
| `value_changed` / `value_committed` | 滑块持续更新/结束操作 |
| `sort_requested` | 用户点列头；提出排序意图，不隐式改变应用业务数据 |
| `expand_requested` | 请求载入树节点，调用方可稍后提供孩子 |
| `canvas_paint` | 自绘区域需要重画，source_id 标识画布 |

setter 默认不发送用户操作事件，避免“程序设置 → 事件 → 再设置”的循环；需要发布业务变更时由应用显式处理。一个 setter 成功后 getter 立即可见，布局和重绘延后；失败不得留下半更新的公共属性。

事件载荷是发生时快照，getter 是当前状态，两者不能保证一致。每次状态变化递增 revision；异步验证或查询返回时先比较 revision，丢弃过期结果。合并事件保留最后快照和 revision，不能跨越提交、失焦、关闭等离散事件。

### 6.2 命令

`gui_command` 保存稳定 ID、显示文本、enabled、checked 和快捷键。菜单、按钮、工具栏可绑定同一个命令；更新命令状态同步所有入口。

快捷键以 `key_chord` 记录表达，字段为键名和 ctrl/alt/shift/win 布尔值，不解析任意脚本。按模态窗口、焦点控件、顶层窗口顺序解析；同一作用域重复绑定在注册时拒绝。编辑控件优先消费输入法、复制粘贴和文本导航快捷键，应用只能显式覆盖允许覆盖的组合；Windows 保留组合不保证可绑定。

首期不引入事件冒泡/捕获树和同步可取消 TX 回调。需要取消的操作采用请求事件加显式执行，例如 `close_requested` 后调用 close。原生控件必须同步决定的返回值由已经设置好的原生状态处理，不能阻塞等待 TX 事件循环。

### 6.3 数据绑定

采用显式单向渲染与事件更新：应用状态更新后调用 setter；收到控件事件后更新应用状态。提供类型化 setter 和批量模型更新即可，不在初版实现属性路径字符串、自动反射或通用双向绑定引擎。

表单验证由应用在提交时执行；库提供错误文本、关联标签、辅助说明和焦点定位。密码控件的掩码仅防屏幕直观显示，`str` 不具备安全擦除保证；事件不携带密码文本，应用需显式读取，日志和诊断不记录其内容。

## 7. 列表、表格与树模型

`gui_data.txh` 定义 `list_item`、`table_column`、`table_row`、`tree_item` 等固定记录。项 ID 在模型生命周期内唯一，删除后不复用；显示顺序和 ID 分离。选择以 ID 表示，排序不改变选中身份，删除后移除对应选择并产生一次合并通知。

表格行基础格式为 `id: int` 和 `cells: vector<str>`，列定义包含 ID、标题、宽度策略与对齐。日期、货币等业务格式由应用转换后交付；排序使用应用维护的真实值，不默认对格式化字符串做数值排序。

模型支持 `replace_items`、`append_items`、`remove_items`、`update_items` 和 `set_order`，每次成功操作形成新 revision。重复 ID、列数不匹配、非法父节点或树环在提交前拒绝；一批数据全成功或全失败。

原生虚拟列表收到同步取数通知时，从 C++ 已持有的不可变显示快照读取，绝不反向调用 TX。大数据通过分页/窗口化更新快照；后台计算新数据，UI 线程提交。页加载事件携带请求 ID 和 revision，迟到结果不得覆盖新查询。

树节点使用“未加载/正在加载/已加载/加载失败”状态，展开未加载节点产生请求并显示占位；收起不阻塞后台任务。原生 TreeView 仍有每节点成本，不能宣传为无限虚拟树。默认应设可配置的已实例化节点和缓存字节上限，达到上限返回 `resource_limit`。

## 8. 焦点、文本与辅助功能

Tab 顺序默认按布局树稳定顺序，可显式设置；隐藏、禁用和非交互控件跳过。焦点节点销毁时转移到同作用域下一个可用节点，再退回父窗口；不能留下指向销毁 HWND 的焦点缓存。

单行 Enter 优先完成 IME，再产生文本提交或默认按钮命令；多行 Enter 默认插入换行。Escape 优先取消当前 IME 组合，再交给取消按钮或窗口。单选组支持方向键，菜单支持标准访问键，自定义 split/canvas 提供明确键盘替代操作。

原生 Edit 内部 UTF-16 与 TX UTF-8 严格转换。公开选择范围采用 Unicode 标量索引的半开区间，与图形文字接口一致；拒绝非法边界，不能返回代理对中点。字素移动交由控件与输入法，正式验收覆盖组合字符和 emoji。

每个交互控件支持 `accessible_name`、`help_text` 和标签关联；原生控件复用 Windows 已有辅助功能。自绘组件实现 UI Automation 的名称、角色、状态、边界及所需 pattern，例如 Invoke/Value/Selection；provider 只读取线程安全的原生快照或派发受控 UI 操作，不在 COM 回调线程执行 TX。

系统字体、主题、高对比度或 DPI 改变时使测量缓存失效并重新布局。库提供遵循系统主题的语义颜色，不强迫用户用固定浅色背景。跨显示器移动同时更新根容器、子 HWND、文字度量和画布目标。

## 9. 对话框、菜单与剪贴板

| 接口 | 返回与行为 |
| --- | --- |
| 打开单文件 | `result<option<str>>`；取消为空，失败为 error |
| 打开多文件 | `result<option<vector<str>>>`；取消为空，成功至少一个路径 |
| 保存路径 | `result<option<str>>`；可配置扩展名和覆盖提示，返回路径不代表文件已写入 |
| 选目录 | `result<option<str>>`；返回实际文件系统路径，不把虚拟 Shell 项目当路径 |
| 消息框 | `result<str>`；结果限定于 options 指定的按钮及关闭策略 |
| 剪贴板读文本 | `result<option<str>>`；无文本为空，剪贴板忙返回错误 |
| 剪贴板写文本 | `result<void>`；严格转换 UTF-16，成功移交系统内存所有权 |

系统文件框使用 Common Item Dialog；过滤器为名称与扩展名向量，不接受直接拼装的过滤字符串。路径统一 UTF-8；文件保存依然由现有 file/fs 模块完成。

自定义模态窗口采用非阻塞模式：`show_modal(child, owner)` 禁用 owner 并立即返回，事件仍由唯一外层循环处理；关闭 child 时恢复 owner。嵌套关系必须无环，父窗口销毁时关闭其模态子窗口，失败时恢复原 enabled 状态。

系统文件框允许系统自己的嵌套消息循环，但不能在活动绘图帧内打开，不能从 WndProc 调用 TX。后台完成通知和关闭请求排队，返回后再交付；队列上限和溢出处理沿用图形库。

## 10. 后台任务与画布整合

计算、文件和网络工作通过现有 thread/task/channel 执行，闭包不捕获任何 GUI 句柄。结果使用满足 Send 的标量、文本、bytes 或唯一移动的类型化结构体交付。

基础集成用有限超时的 `graphics.next_event` 定期检查通道，推荐 16–50 ms，并限制每轮消费量。空闲时没有后台任务则无限等待。后续可增加绑定通道就绪通知的等待接口，但必须先实现不丢唤醒的原生集成，不能仅调用 PostMessage 就宣称与所有 channel 原子协同。

后台工作携带应用自建的 generation/request_id；窗口关闭或查询切换后丢弃过期结果。取消任务是请求，UI 线程不执行无界 join；关闭流程先请求取消，继续泵送消息，任务收束后再关闭会话。

`gui_canvas` 的 `begin_frame` 返回 `result<option<graphics_canvas>>`，提交和取消复用图形库；画布与顶层窗口共用“同线程一次一个活动帧”规则。原生控件之间不能穿插任意透明图层；canvas 不可覆盖输入框实现半透明玻璃效果。裁剪、滚动和 DPI 变更由其独立客户区负责。

## 11. 核心 API 与示例草案

以下核心接口已在 U0–U1 实现，完整签名以 `tx/stdlib/gui.txh` 为准。这里以明确实参呈现最小稳定入口。

```tx
# gui.txh 的部分声明。
def root(window: graphics_window) -> result<gui_container>
def set_column(parent: gui_container, padding: float, gap: float) -> void
def create_label(parent: gui_container, text: str) -> result<gui_label>
def create_text_box(parent: gui_container, multiline: bool) -> result<gui_text_box>
def create_button(parent: gui_container, text: str) -> result<gui_button>
def set_text(control: gui_label, text: str) -> void
def set_text(control: gui_text_box, text: str) -> void
def text(control: gui_text_box) -> str
def id(control: gui_button) -> int
def focus(control: gui_text_box) -> result<void>
```

创建控件默认可见并按创建顺序加入父布局；根容器跟随客户区。默认 column 子项宽度填满可用空间，高度自动测量。后续 options 记录扩展 min/max、对齐和辅助信息，不破坏上述简单用法。

```tx
import "graphics.txh" as gfx
import "gui.txh" as gui

def main() -> int
{
    auto app = gfx.open_app().value()
    auto window = gfx.create_window(app,
        gfx.window_options("TX 问候程序", 480.0, 260.0, true)).value()
    auto root = gui.root(window).value()
    gui.set_column(root, 16.0, 12.0)
    auto label = gui.create_label(root, "请输入姓名").value()
    auto input = gui.create_text_box(root, false).value()
    auto button = gui.create_button(root, "问候").value()
    gui.set_default_button(button)
    gfx.show(window).value()
    gui.focus(input).value()
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
            else if event.kind == "control" && event.control.is_some()
            {
                auto action = event.control.value()
                if (action.source_id == gui.id(button) && action.action == "activated") || (action.source_id == gui.id(input) && action.action == "text_committed")
                {
                    gui.set_text(label, "你好，" + gui.text(input))
                }
            }
        }
    }
    gfx.close(app)
    return 0
}
```

示例是单窗口流程，所以关闭请求可直接关闭唯一窗口。正式多窗口示例必须按 window_id 路由，并在关闭后忽略其剩余控件事件。普通原生控件由库自动绘制，不要求应用收到 paint 后重画按钮。

## 12. 错误与实现组织

GUI 使用图形库新增的 `graphics_error` 类别，避免每个控件引入异常种类。补充稳定 code：`invalid_layout`、`duplicate_id`、`model_in_use`、`stale_revision`、`modal_conflict`、`clipboard_busy`。通用线程、关闭、限额和系统失败沿用图形错误码。

创建、系统交互、焦点切换等实际可能失败的操作返回 `result`；布局和属性 setter 对非法输入、无效资源或原生调用失败抛可捕获异常。UI 内部回调必须捕获失败并保存待交付错误，不能向 Windows 调用栈抛出。

建议目录进一步按职责划分：

| 目录 | 内容 |
| --- | --- |
| `src/stdlib/gui/` | 控件生命周期、记录、模型、布局算法和公共契约 |
| `src/stdlib/gui/windows/` | 控件创建/通知、焦点、菜单、主题、IME、对话框和 UIA |
| `src/backend/cpp/gui_*_abi.cpp` | 具体控件、模型与布局的静态 ABI |
| `src/backend/llvm/` | 资源类型、重载与 ABI 固定符号生成 |
| `tx/stdlib/gui*.txh` | 公开接口 |
| `examples/gui/` | 表单、列表、文件选择、任务进度、自绘混合示例 |

新增控件资源类型需要完整接入模块解析、sema、类型化容器、资源关闭、深复制拒绝及 Send/Sync；不能只在 `gui` 函数入口验证。Win32 通知到内部控件对象的映射可以运行时完成，但从 TX 调用 `set_text` 到具体 ABI 的选择必须静态完成。

发行复用图形库的 DPI 清单、Common Controls v6 清单和 GUI 子系统。UI Automation 相关 SDK/import library 需根据实际接口核实追加，普通和 ThinLTO 构建均保持一致；不把 Windows 自带 DLL 复制到安装包。

## 13. 实施阶段与完成条件

| 阶段 | 交付 | 验收门槛 |
| --- | --- | --- |
| U0：公共骨架 | 控件类型、树、错误、事件记录、资源 ABI | 错控件类型/跨线程在编译期拒绝；父关闭使子别名失效；事件无需 TX 回调 |
| U1：表单与布局 | label/button/text/check、row/column/grid、焦点 | 上述问候程序和设置表单可运行；调整大小、125%/200% DPI 和键盘操作正确 |
| U2：命令与系统交互 | 菜单/工具栏、命令、快捷键、文件框/剪贴板 | 多入口状态一致；IME 不被快捷键截断；取消、剪贴板忙和模态 owner 恢复正确 |
| U3：数据控件 | list/table/tree 模型、分页、稳定 ID | 排序后选择不漂移；过期页不覆盖；同步系统取数不回调 TX；限额可诊断 |
| U4：复杂界面 | tabs/scroll/split、任务进度、canvas | 后台期间 UI 可交互；任务取消与关闭可收束；自绘与原生控件滚动/DPI 正确 |
| U5：辅助功能与恢复 | UIA、高对比度、字体主题变化、异常清理 | 屏幕阅读器可操作；失焦无卡键；无泄漏窗口；设备丢失不损坏控件树 |
| U6：发行与文档 | 完整接口、示例、GUI 可执行入口、兼容记录 | 干净 Windows 环境可运行，目标基线实测，普通/ThinLTO 包均完整 |

只在实现对应功能时做定向验证：布局算法边界、UTF 索引、事件合并、模型原子更新、生命周期与静态 ABI 生成。人工验收重点是微软拼音、多显示器混合 DPI、Tab/Enter/Escape、Narrator、高对比度和系统模态框。阶段表描述各阶段目标；U0–U1 的实际自动验证与人工待验收项见专项交付记录，U2–U6 尚未实施。

每阶段先冻结具体公开签名、options 默认值及限额，再实现；同步更新文档和可运行示例。完整终态以 U0–U6 全部交付并完成对应验收为准，基础表单可用只代表 U1 完成。
