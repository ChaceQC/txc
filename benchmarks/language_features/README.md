# TX 语言特性性能基准

本目录测量当前语法和运行时机制在生成程序中的开销。`compare.tx` 将负载分开计时，每项输出三行：名称、程序内部耗时（毫秒）、校验值。`workload.txh` 与配对的 `workload.tx` 用于测量别名导入后的函数调用和结构体传参。

从仓库根目录运行。编译器源码有改动时先执行 `scripts/build.ps1`，确保 `tx/txc.exe` 与运行时库对应当前源码。

```powershell
.\tx\txc.exe check .\benchmarks\language_features\compare.tx
.\tx\txc.exe .\benchmarks\language_features\compare.tx -o .\tx_build\language_features_tx.exe
.\tx_build\language_features_tx.exe
```

`check` 只测静态检查；若要观察编译器自身的耗时，可分别对 `check` 和完整编译命令使用 PowerShell 的 `Measure-Command`。下表的计时只来自生成程序，不含编译、启动和输出。每项在计时前准备固定输入，在计时后打印校验值；`deep_copy` 的原件未被修改检查和 `cycle_gc` 的补充安全点属于各自的计时范围。

| 项目 | 主要覆盖 | 循环次数 | 预期校验值 |
| --- | --- | ---: | ---: |
| `scalar_control` | `for`、整数算术、`if/else`、`&&`、复合赋值 | 500000 | -62500000000 |
| `updates` | 后置 `++/--`、表达式中的更新结果 | 500000 | 500000 |
| `while_logic` | `while`、`&&` 与 `||` 短路、条件分支 | 500000 | -62500000000 |
| `float_arithmetic` | `int as float`、浮点除法与加法 | 500000 | 62500125000 |
| `overloads` | 整数与浮点函数重载 | 100000 | 5000650000 |
| `named_arguments` | 命名实参绑定 | 200000 | 20000700000 |
| `recursion` | 深度 24 的直接递归 | 10000 | 3000000 |
| `variadic_unpack` | `*args`、`**kwargs` 与 `*`、`**` 实参展开 | 10000 | 50095000 |
| `array_destructure` | 数组字面量、解包、索引写入、元素遍历与 `any as int` | 20000 | 400060000 |
| `array_padded` | 定长数组、部分初始化、`none`、`is_none` 与索引 | 50000 | 1250225000 |
| `dict_iteration` | 混合类型键、索引更新与键遍历 | 100000 | 5000350000 |
| `struct_operators` | 命名构造、字段读取、`+`、`==` 与 `+=` 运算符成员 | 100000 | 5000350000 |
| `class_methods` | 类字段、普通方法重载及方法内复合赋值 | 200000 | 80000200000 |
| `virtual_interface` | 多继承、接口与父类引用的虚方法分派、`super` | 200000 | 3200000 |
| `class_operator` | 类虚运算符覆盖和父类运算符调用 | 100000 | 1000000 |
| `runtime_cast` | 接口引用到类的运行时转换与虚调用 | 100000 | 800000 |
| `module_call` | `.txh`/`.tx` 配对、别名导入、跨模块函数和结构体 | 100000 | 15000950000 |
| `string_conversion` | 数字与字符串双向 `as`、字符串拼接和 `len` | 50000 | 1250413894 |
| `deep_copy` | 嵌套数组、字典、结构体的递归复制与写入隔离 | 10000 | 50005001 |
| `copy_cycle` | 自引用数组的深拷贝、环内别名关系与原件隔离 | 10000 | 50005007 |
| `deinit` | 无环类对象释放与析构回调 | 20000 | 20000 |
| `cycle_gc` | 自引用类对象、循环回收及析构回调 | 1000 | 1000 |

`cycle_gc` 在循环后最多增加 4096 次分配安全点，直到本轮创建的 1000 个环都完成析构；若校验值不足 1000，表示回收没有在该上限内完成。这个项目测量完整回收前的工作，不把进程退出时的清理算进计时。

建议先预热，再在相同编译器和机器上运行多轮并比较各项中位数。计时精度为 1 ms，短项目的单轮结果波动较大；运行顺序、GC 阈值和其他负载也会影响数值。这里的项目用于观察同一语言版本前后的变化。标准库与 C++ Release 的同负载对照仍在 [library_compare](../library_compare/README.md)，两组负载不能直接混合计算语言间倍率。
