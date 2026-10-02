# 自研数据视图契约

list/table/tree 在同一自绘控件树中运行。每个视图拥有一个平台无关的数据快照，
普通数据行不创建控件节点或系统窗口；只为视口内行建立文字布局。

- 行和列的 ID 为正整数，行 ID 在视图生命周期内不能删除后复用。批量 replace/append/update、
  删除及重新排序先验证完整快照，再一次提交并递增 `revision`；失败时保留原数据、选区和修订。
- 单次最多 100000 行、256 列，总文本不超过 64 MiB；树深度最多 64，拒绝缺失父项和依赖环。
  树的 `parent_id=0` 表示根项，删除父项会一并删除后代。`has_children` 可声明尚未载入的子项。
- 选择以 ID 保存，数据排序不改变选择身份。单选/多选、Ctrl 切换、Shift 连续选区、
  Ctrl+A、方向/PageUp/PageDown/Home/End 和 Enter 激活均由公共输入层处理。
  程序更新静默修剪已删除的选择；用户操作产生 `data_selection_changed/data_activated`。
- 表格保存固定的单元格列身份，列宽、隐藏及展示顺序可以独立调整，不改变行单元格的归属。
  点击列头或在聚焦列上按 Ctrl+Space 产生 `sort_requested`，携带整数 `column_id`、
  `state`（升序）和发生时 `revision`。应用通过 `set_order` 应用排序结果，库不猜测数据类型。
- 树 Left 收起或移到父项，Right 展开或进入首个子项；用户展开事件为 `tree_expanded`。
  声明有子项但尚未载入时另发 `expand_requested`。收起不删除子项或已有选择。
- `begin_page` 产生递增请求号，`apply_page` 同时核对最新请求号与预期修订。
  新请求或任何成功数据变更使旧请求过期，拒绝后台旧结果覆盖新模型。

控件修订、ID、列 ID 通过整数 ABI 传递。事件交付后模型可能继续变化，使用异步结果的应用必须
核对事件修订。滚动条、嵌套裁剪和焦点自动滚入复用容器实现；验证不依赖系统 ListView/TreeView。
