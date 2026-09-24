# 编译与运行 .tx 文件

本文命令在 Windows PowerShell 中执行，工作目录为本仓库根目录。当前版本已在 GCC 13.1 下构建；需要支持 C++23 的 g++ 和 CMake 3.21 或更高版本。下文使用 Ninja 生成器，因此执行这些构建命令时也需要 Ninja。生成可执行文件时，g++ 必须位于 PATH。

## 构建编译器

使用构建脚本；当前机器的 CMake 随 CLion 安装，脚本会自动定位：

```powershell
.\scripts\build.ps1
```

编译器工具目录 `tx/` 包含 `txc.exe`、`libtxstdlib.a` 和 `stdlib/*.txh` 公开接口。脚本在 build/ 中进行 CMake 构建，确认成功后删除中间目录；构建失败时保留 build/ 供排查。txc 链接程序时会把静态库传给链接器一次。

## 编译源码

命令格式：

```text
txc <源码.tx> [-o <输出.exe>]
txc check <源码.tx>
```

编译仓库中的示例；默认输出到 tx_build/：

```powershell
.\tx\txc.exe .\example.tx
.\tx_build\example.exe
```

程序输出为 55，退出码为 0。省略 -o 时，输出到仓库根目录的 tx_build/，主文件名与源码相同；例如编译 example.tx 会生成 tx_build/example.exe。指定 -o 时可使用自选位置，txc 会创建其父目录。

编译器先解析并检查 .tx，再生成临时 C++23 源码，最后调用 PATH 中的 g++ 生成原生可执行文件。语法或类型错误会以“文件:行:列: 错误：原因”的形式报告；C++ 后端失败时会显示 g++ 的输出。

## 只检查源码

```powershell
.\tx\txc.exe check .\example.tx
```

check 会进行词法、语法、名称解析和静态类型检查，包括 main 入口与返回路径要求。检查通过时输出“语法和类型检查通过”，退出码为 0；检查失败时报告错误并返回非零退出码。这个命令不会生成可执行文件，也不会调用 g++。

语言写法见 [语法与类型规则](syntax.md)，可运行的完整示例见 [example.tx](../example.tx)。
