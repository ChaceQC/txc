# TX 编译器

使用 C++23 实现的静态强类型 .tx 语言编译器。当前版本解析、检查并编译仓库中的 example.tx，生成原生可执行文件。

## 快速使用

运行 `scripts/build.ps1` 构建 txc 后，在仓库根目录执行：

```powershell
.\tx\txc.exe check .\example.tx
.\tx\txc.exe .\example.tx
.\tx_build\example.exe
```

check 会检查语法和静态类型；示例程序运行后输出 55。完整构建步骤和命令参数见 [编译与运行](docs/usage.md)，当前语言支持的语法和类型规则见 [语法说明](docs/syntax.md)。

最终编译器产物 `txc.exe` 和 `libtxstdlib.a` 位于 `tx/`；构建成功后清理 `build/` 中间文件。txc 默认将 .tx 程序放在 `tx_build/`。

字符串、浮点数、混合类型数组和结构体的用法见 [数据类型示例](examples/data_types.tx)。

模块导入、终端输入输出、可指定字符集的文本文件读写与字符串库见 [模块说明](docs/modules.md)、[标准库说明](docs/standard_library.md)和[可运行示例](examples/import_io.tx)。
终端与文件操作的单独示例见 [终端 I/O](examples/terminal_io.tx)、[文件 I/O](examples/file_io.tx)和[指定字符集读写](examples/file_encodings.tx)。
两个模块拥有同名函数时的调用见 [别名导入示例](examples/import_alias.tx)。
`import "io.txh"` 和 `import "xx/xx.txh"` 会先查找源码同目录的接口，再查找 `tx/stdlib/` 下的对应路径；见 [本地接口优先示例](examples/local_priority/main.tx)。
函数的直接递归、相互递归和无返回值递归见 [递归示例](examples/recursion.tx)。
同名函数按参数类型列表重载的用法见 [函数重载示例](examples/overload.tx)。
`int` 和 `float` 的范围见 [语法与类型规则](docs/syntax.md)，最小 `int` 的直接写法见 [数值边界示例](examples/numeric_bounds.tx)。

## 代码布局

- `src/common`：共用类型和错误位置。
- `src/frontend`：词法分析、语法树、语法分析和语义检查。
- `src/backend/cpp`：C++ 源码生成与运行时。
- `src/stdlib`：标准库二进制实现；公开接口位于 `tx/stdlib/*.txh`。
- `src/driver`：`txc` 命令行入口。
