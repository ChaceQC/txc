# TX 与 C++ 语言特性性能对照

`compare.tx` 与 C++23 Release 对照使用相同的输入、循环次数和校验值，分别测量 22 项语言特性及运行时负载。每项输出三行：名称、程序内部耗时（毫秒）、校验值。TX 的 `workload.txh`/`workload.tx` 和 C++ 的 `workload.hpp`/`workload.cpp` 都让 `module_call` 跨源文件调用。

从仓库根目录运行。编译器源码有改动时先执行 `scripts/build.ps1`，确保 `tx/txc.exe` 与运行时库对应当前源码。比较脚本会用 CMake 的 Release 配置构建 C++ 对照、编译 TX，分别预热一次，再交替顺序测量 7 轮；每轮逐项检查两边的校验值，最后输出耗时中位数和 TX/C++ 倍率。CMake 优先使用本机 CLion 附带的版本，找不到时使用 PATH 中的 `cmake`。

```powershell
.\benchmarks\language_features\run_compare.ps1
```

可用 `-Rounds 3` 指定较少轮数做快速检查。两边的生成程序分别位于 `tx_build/language_features_tx.exe` 和 `tx_build/language_features_cmake/language_features_cpp.exe`，也可以从仓库根目录单独运行。若要观察编译器自身的耗时，可分别对 `txc check` 和完整编译命令使用 PowerShell 的 `Measure-Command`；下表的程序内部计时不含编译、启动和输出。

C++ 对照按同一运算顺序实现。C++ 没有命名实参和 `**kwargs` 语法，因此对应项目使用已经绑定好顺序的函数调用和按值传入的动态容器。数组使用 `std::vector<std::any>`，字典使用按键类型区分的 `std::unordered_map`，结构体和类引用使用 `std::shared_ptr`。`deep_copy` 的 C++ 实现复制此用例已知的嵌套形状；`copy_cycle` 使用对象身份表保留自环；`cycle_gc` 使用只识别本用例单节点自环的专用回收器。后面三项以及字典的容器表示并不提供与 TX 运行时完全相同的通用语义，倍率只表示这里写出的两份实现的耗时差。C++ Release 使用优化编译，TX 目标程序由随 TX 分发的 clang 以 `-O2` 生成。

| 项目 | 主要覆盖 | 循环次数 | 预期校验值 |
| --- | --- | ---: | ---: |
| `scalar_control` | `for`、整数算术、`if/else`、`&&`、复合赋值 | 500000 | -62500000000 |
| `updates` | 后置 `++/--`、表达式中的更新结果 | 500000 | 391096443253 |
| `while_logic` | `while`、`&&` 与 `||` 短路、条件分支 | 500000 | -62500000000 |
| `float_arithmetic` | `int as float`、浮点除法与加法 | 500000 | 62500125000 |
| `overloads` | 整数与浮点函数重载 | 100000 | 4838081397 |
| `named_arguments` | 命名实参绑定 | 200000 | 18731467920 |
| `recursion` | 深度 21～30 的直接递归 | 10000 | 3420000 |
| `variadic_unpack` | `*args`、`**kwargs` 与 `*`、`**` 实参展开 | 10000 | 50095000 |
| `array_destructure` | 数组字面量、解包、索引写入、元素遍历与 `any as int` | 20000 | 400060000 |
| `array_padded` | 定长数组、部分初始化、`none`、`is_none` 与索引 | 50000 | 1250225000 |
| `dict_iteration` | 混合类型键、索引更新与键遍历 | 100000 | 5000350000 |
| `struct_operators` | 命名构造、字段读取、`+`、`==` 与 `+=` 运算符成员 | 100000 | 5000350000 |
| `class_methods` | 类字段、普通方法重载及方法内复合赋值 | 200000 | 2726759900400 |
| `virtual_interface` | 多继承、接口与父类引用的虚方法分派、`super` | 200000 | 3200000 |
| `class_operator` | 类虚运算符覆盖和父类运算符调用 | 100000 | 1000000 |
| `runtime_cast` | 接口引用到类的运行时转换与虚调用 | 100000 | 800000 |
| `module_call` | `.txh`/`.tx` 配对、别名导入、跨模块函数和结构体 | 100000 | 15000950000 |
| `string_conversion` | 数字与字符串双向 `as`、字符串拼接和 `len` | 50000 | 1250413894 |
| `deep_copy` | 嵌套数组、字典、结构体的递归复制与写入隔离 | 10000 | 50005001 |
| `copy_cycle` | 自引用数组的深拷贝、环内别名关系与原件隔离 | 10000 | 50005007 |
| `deinit` | 无环类对象释放与析构回调 | 20000 | 20000 |
| `cycle_gc` | 自引用类对象、循环回收及析构回调 | 1000 | 1000 |

`updates`、`overloads`、`named_arguments` 和 `recursion` 使用程序启动后取得的正 Unix 时间选择固定种子 1，避免 C++ Release 将整个循环预先折叠为常量。`virtual_interface` 与 `class_operator` 也在计时前根据运行时时间选择对象类型，使虚分派保留动态类型。上表的预期值适用于正常的正 Unix 时间环境。

`cycle_gc` 在循环后最多增加 4096 次分配安全点，直到本轮创建的 1000 个环都完成析构；若校验值不足 1000，脚本会报错。`deep_copy` 的原件未被修改检查和 `cycle_gc` 的补充安全点属于各自的计时范围，进程退出时的清理不算进计时。

TX 计时精度为 1 ms，C++ 使用更细的计时；短项目的单轮倍率只宜粗略看待。运行顺序、GC 阈值和其他系统负载也会影响数值。标准库与 C++ Release 的对照仍在 [library_compare](../library_compare/README.md)，两组负载和倍率分别解读。
