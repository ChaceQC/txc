# 编译与运行 .tx 文件

本文命令在 Windows PowerShell 中执行，工作目录为本仓库根目录。构建 txc 和运行时库需要支持 C++23 的 g++、CMake 3.21 或更高版本以及 Ninja；构建脚本还需要 LLVM 的 `clang.exe`，可通过 `TX_LLVM_BIN` 指定其 bin 目录或加入 PATH。当前已用 LLVM 23.1.2 验证。运行构建好的 txc 编译 `.tx` 时，不需要在 PATH 中安装 g++、LLVM 或链接器。

## 构建编译器

使用构建脚本；当前机器的 CMake 随 CLion 安装，脚本会自动定位：

```powershell
.\scripts\build.ps1
```

编译器工具目录 `tx/` 包含 `txc.exe`、`clang.exe`、`libtxstdlib.a`、`link/` 内的链接组件、运行时 DLL 和 `stdlib/*.txh` 公开接口。交付时保留整个 `tx/` 目录。脚本在 build/ 中进行 CMake 构建，确认成功后删除中间目录；构建失败时保留 build/ 供排查。构建时使用 g++；生成的 txc 不会调用它。

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

程序展示当前语言的各类语法并以退出码 0 结束。省略 -o 时，输出到仓库根目录的 tx_build/，主文件名与源码相同；例如编译 example.tx 会生成 tx_build/example.exe。指定 -o 时可使用自选位置，txc 会创建其父目录。

编译器先解析并检查 .tx，再生成 LLVM IR，由同目录的 `clang.exe` 将 IR 编为 Windows 目标文件，最后用 `tx/link/ld.exe` 与运行时库链接为原生可执行文件。生成程序需要的 MinGW 运行时 DLL 会复制到输出目录。语法或类型错误会以“文件:行:列: 错误：原因”的形式报告；后端失败时会显示目标文件生成器或链接器的输出。

## 只检查源码

```powershell
.\tx\txc.exe check .\example.tx
```

check 会进行词法、语法、名称解析和静态类型检查，包括 main 入口与返回路径要求。检查通过时输出“语法和类型检查通过”，退出码为 0；检查失败时报告错误并返回非零退出码。这个命令不会生成目标文件或调用链接器。

语言写法见 [语法与类型规则](syntax.md)，可运行的完整示例见 [example.tx](../example.tx)。
