# 类型化迭代器

`iterator<T>` 无需 import，类型参数与 `vector<T>` 一致。`snapshot_iter()` 固定创建时的元素序列，`live_iter()` 每次按当前下标读取；两者均用 `next() -> option<T>` 取值。普通赋值共享迭代器游标，`deep_copy` 复制游标状态和容器快照；`close()` 使所有共享别名失效。

实时迭代器在 `next()` 首次返回空 option 后永久结束。插入、删除、重排、清空和追加都不会令它崩溃；尚未结束时它按下标观察修改，因此下标移动可能导致重读或跳过。快照仅固定元素槽位，复合元素仍可能共享内部对象。旧 `for value in vector` 保持原有“初始长度、实时下标读取”语义。

若调用方在取得元素后执行回调且回调失败，迭代器已经前进。错误沿原类别传播；捕获后可继续读取。`close()` 后调用 `next()` 报 `runtime_error/invalid_state`，在已经到达终点后调用仍返回空 option。迭代器持有源向量共享引用，原变量重新绑定不会提前销毁旧向量。

示例见[迭代器示例](../examples/iterators.tx)。

## 稳定 snapshot 游标优化

性能计划第 12 项对整个函数中不逃逸、不重绑定的局部 `iterator<int/float/bool>` snapshot 缓存稳定视图。局部标量 option 的 next 直接读取元素并写回 index，不逐元素跨 ABI；原句柄保持拥有关系。closed 仍优先检查，首次读空仍写入 exhausted，后续表达式报错不会撤销已提交的推进。LLVM 只有在循环没有可能改变状态的操作时才能提升检查。

普通赋值别名、传参、返回、deep_copy、实时游标或复合元素继续执行完整入口。单个局部 snapshot 内混合直接 next、普通 next 和 close 时共享同一游标状态。统计库读取相同状态；固定为无对象边的标量/文本迭代器不登记循环节点，实际创建仍计入分配次数。定向用例见 [iterators.tx](../tests/performance_12_14/iterators.tx)。

## 2.4 实施记录

- **代码：** `iterator<T>` 接入类型解析、静态方法签名、LLVM 调用和六种元素 ABI；迭代器持有源向量或快照，普通赋值共享游标，`deep_copy` 复制游标与容器图。标量元素的 `next()` 可直接写入局部 `option<T>` 的栈上状态和值，不逐项创建运行时 `option` 对象；需要传递或保存该值时才生成句柄。局部迭代器调用借用句柄；复合迭代器接入 `any` 类型恢复、打印及循环回收对象图。现有 `for value in vector` 的代码路径未更改。
- **构建：** `scripts/build.ps1` 在 Windows x64 成功生成编译器、标准库静态库和兼容清单。
- **定向验证：** `examples/iterators.tx` 编译运行，覆盖快照与实时读取、值修改、删除导致的下标移动、结束后的追加、共享别名、`deep_copy`、`any` 恢复、字符串和结构体元素；`tests/stdlib/iterator_module/main.tx` 跨 `.txh` 返回游标并输出 `17`。关闭后读取报运行错误，`iterator<any>` 在源码位置报中文类型错误。未运行全量套件。
- **边界与验收：** 快照固定元素槽位但复合元素可共享内部对象；实时游标按位置观察修改，已结束游标不重新开启。当前证据限 Windows x64；异步/并发迭代器由后续运行时与并发小项定义。
