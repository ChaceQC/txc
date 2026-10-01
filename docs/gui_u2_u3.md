# Windows GUI U2–U3

状态：实现及定向自动验证通过；真实桌面人工验收待执行。接口冻结日期：2026-10-02。以 `gui.txh` 与 `gui_data.txh` 为公开签名来源，复用 graphics 的单会话、错误和事件队列。

## 命令与系统交互

命令属于窗口，ID 会话内唯一；菜单、按钮和原生 toolbar 共享命令文本、enabled、checked、revision。快捷键是固定 key_chord 记录，窗口作用域内拒绝重复；组合输入期间及编辑控件标准编辑快捷键优先交给原生控件。命令事件包含 command_id，source_id 标识入口。

文件选择采用 Common Item Dialog：打开单文件、打开多文件、保存路径、目录选择；取消为空 option。filter 是名称和扩展名向量；默认文件系统项，禁止虚拟路径。保存仅选择路径。消息框提供 ok/ok_cancel/yes_no/yes_no_cancel。剪贴板无文本为空，忙返回 clipboard_busy，Unicode 严格转换。

自定义 `show_modal(child, owner)` 立即返回；禁止环及重复模态关系，关闭子窗口恢复 owner 原 enabled 状态，关闭 owner 同时关闭模态后代。系统对话框和剪贴板调用拒绝活动帧；系统循环只处理原生消息，不回调 TX。

## 数据控件

list/table/tree 使用类型化模型和原生 ListView/TreeView。模型最多 100000 项、64 MiB 文本、表格最多 256 列、树深度最多 64；树实例化最多 10000 项。ID 为非负 int，模型生命周期内删除后不复用；replace 可保留仍存在的 ID。批量 replace/append/update/remove/set_order 先验证完整快照再提交，失败保留旧 revision、数据和选择。

列表和表格采用 owner-data 虚拟显示，同步取数仅访问 C++ 快照；模型可绑定同窗口的多个视图。选择用 ID 表达，排序后重新映射到索引，删除选中项后每视图至多一个 selection_changed。setter 不产生用户事件，模型删除导致选择变化例外。

U3 的列宽显式用 DIP float，列在模型创建时固定；树视图采用原生单选，`multiple` 必须为 false。列表/表格可多选。树删除父项时连同后代删除；顺序向量必须包含所有现存 ID，兄弟顺序遵循向量顺序。关闭仍被视图引用的模型返回 model_in_use；窗口关闭会一并使模型和视图失效。无需 TX 同步取数回调。

分页由 `begin_page(model)` 返回请求 ID，`apply_page(model, request_id, expected_revision, items)` 原子提交；新请求及任何模型修改使旧回复 stale_revision，不能覆盖新数据。树节点 parent_id 使用 option<int>，load_state 为 unloaded/loading/loaded/failed；展开 unloaded/failed 产生 expand_requested（item_id + revision），状态切换 loading；应用随后使用模型操作提交子项。树环、缺父节点、重复 ID、列数不匹配立即报错。

## 验证记录

2026-10-02，当前 Windows x64 环境：

| 项目 | 结果 |
| --- | --- |
| 普通/ThinLTO TX 用例 | 命令文本/enabled/checked 同步、快捷键冲突、模型快照隔离、排序保持 ID 选择、重复/复用 ID 拒绝、批量失败不改变 revision、过期分页拒绝、三个视图及窗口关闭失效通过 |
| 原生 Win32 用例 | 菜单状态与按钮命令通知、模态窗口恢复 owner 原 enabled 状态、ListView 同步取数、排序后选中索引、树展开 loading/request 与树环拒绝通过 |
| 基础回归 | 原有 U0–U1 属性/布局/生命周期用例和静态跨线程拒绝通过 |
| 示例 | `examples/gui/data_browser.tx` 编译成功，演示文件框、共享菜单/工具栏/按钮命令、排序及剪贴板 |

复现：`python scripts/check_graphics_gui_extensions.py`；产物位于 `tx_build/graphics_gui_g2_u3/`。示例运行：`tx/txc.exe examples/gui/data_browser.tx` 后启动 `tx_build/data_browser.exe`。

真实系统文件框的确认/取消、多选和过滤器、剪贴板忙/跨进程文本、微软拼音期间快捷键、实际键盘访问键、多显示器 DPI 仍待人工验收；自动验证没有操作用户剪贴板。本文记录 U2–U3 的原始交付，后续复杂容器、自绘 UIA、主题恢复见 [U4–U5](gui_u4_u5.md)；U6 及最低系统版本发行验收仍待执行。
