# 基础交互补全契约

本轮补全 P2 的下拉选择、访问键、窗口默认与取消动作。复用公共布局、文字整形与像素绘制，
不创建系统子控件。普通静态 ABI 与 ThinLTO 必须使用相同接口。

- `create_combo_box(parent)` 创建只读选择框；`set_combo_items(control, vector<str>)` 原子替换选项，
  最多 10000 项，每项最多 65536 UTF-8 字节，总计最多 16 MiB。替换后清空选择。
- `selected_index` 返回零基索引，`-1` 表示无选择；`set_selected_index` 接受 `-1` 或有效索引。
  程序设置不发送用户事件。用户提交产生 `selection_changed`，`number` 为索引、`item_id` 为索引加一，
  `text` 为选项文字，`revision` 为控件修订号；同一选择不重复发送。
- 点击、Space、Enter、F4 或 Alt+Down 展开。展开后方向键、Home/End、PageUp/PageDown 改变候选，
  Enter/Space 或鼠标释放提交，Escape/F4/Alt+Up 取消。Tab 取消后继续焦点导航。
  关闭状态的方向键/Home/End 直接选择。弹层限制在当前窗口内，空间不足时向上展开，可滚动。
  点击外部关闭并吞掉该次按下，防止穿透激活底层按钮；失焦、捕获丢失、隐藏、禁用或关闭取消候选。
- `set_access_key(control, key)` 接受单个 ASCII 字母/数字（字母大小写等价），空串清除。
  Alt+键激活按钮/复选/单选，或聚焦编辑器、滑块与 combo；combo 同时展开。
  同键多控件时按树顺序循环聚焦，不自动激活。不可用与隐藏页控件不参与。
  文本不隐式解析 `&`，标签可显式写出 `(Alt+X)`，避免破坏已有文字。
- `set_default_button(button, enabled)` / `set_cancel_button(button, enabled)` 每个窗口各绑定一个按钮。
  置 true 替换旧绑定；false 只清除自身绑定。绑定使用弱引用，不延长控件生命周期。
  无修饰 Enter 优先处理展开 combo、多行编辑器和已聚焦按钮/复选/单选；单行编辑器保留 `text_committed`
  并激活可用默认按钮。Escape 优先关闭弹层，随后才触发取消按钮。重复按键不重复激活。
  输入法组合期间不触发访问键和默认/取消动作。隐藏、禁用、关闭的绑定不触发。

这不替代 P6 的共享命令、菜单与模态对话框，也不代表真实输入法或人工桌面验收通过。

## 2026-10-03 验证与运行

Windows x64 与 Ubuntu 24.04 均已通过定向行为检查及普通/ThinLTO 静态 ABI，生成了
`tx_build/native_gui/interaction_workbench.exe` 与 `interaction_workbench_linux`。
定向行为由检查程序发送输入事件，不是人工桌面操作或真实输入法验收。

Windows 使用系统字体即可运行；Linux 需要当前自研解析器支持的 TrueType/TTC 字体。
可以通过 `TX_GUI_FONT` 显式指定。当前 WSL 的 Arial 检查夹具没有可用的中文回退字体，
首次 Linux 图像中的中文显示为缺字符号；改用同一份 `msyh.ttc` 后已确认中文显示。
这没有修复完整字体发现、CFF 字体解析或系统字体集合差异，也不宣称双平台图像一致。

```sh
TX_GUI_FONT=/path/to/chinese-truetype-font.ttc ./tx_build/native_gui/interaction_workbench_linux
```
