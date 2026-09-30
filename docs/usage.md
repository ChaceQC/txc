# 编译与运行 .tx 文件

本文命令在 Windows 的 PowerShell 7 中执行，工作目录为本仓库根目录。构建 txc 和运行时库需要支持 C++23 的 g++、CMake 3.21 或更高版本以及 Ninja；构建脚本还需要同版本 LLVM 的 `clang.exe`、`llvm-ar.exe` 和 `ld.lld.exe`，通过 `TX_LLVM_BIN` 指定完整 LLVM 的 bin 目录。当前已用 LLVM 23.1.2 验证。运行构建好的 txc 编译 `.tx` 时，不需要在 PATH 中安装 g++、LLVM 或链接器。

## 构建编译器

使用构建脚本；当前机器的 CMake 随 CLion 安装，脚本会自动定位：

```powershell
.\scripts\build.ps1
.\scripts\build.ps1 -Incremental
```

默认构建成功后清理 `build/`；反复开发时使用 `-Incremental` 保留构建目录，让 Ninja 只重编译变化的目标。构建脚本显式以最多 8 个并行任务运行 CMake，C/C++ 编译由 ccache 缓存；未设置 `CCACHE_DIR` 时缓存位于 `%LOCALAPPDATA%\TxCompiler\ccache`，即使清理 `build/` 也保留。ccache `4.14` 的 Windows x86_64 构建工具由 CMake 校验 SHA-256 `2568347a697e103ca1b073981c704ad76fb2507d066c38dba038dd73399d968f` 后下载，许可证为 GPL-3.0-or-later，仅用于构建，不进入 `tx/` 发行目录。其他平台若已安装 ccache 也会启用编译缓存。

编译器工具目录 `tx/` 包含 `txc.exe`、`clang.exe`、`libtxstdlib.a`、`package.compat`、`link/` 内的链接组件、运行时 DLL 和 `stdlib/*.txh` 公开接口。交付时保留整个 `tx/` 目录。脚本在 build/ 中以 Release 配置构建编译器、运行时库和标准库，还会把 `src/stdlib/httpx_bridge.tx`、`websocket_bridge.tx`、`requests_bridge.tx` 编成对象文件，并将固定版本的 nghttp2、Mbed TLS、Argon2 参考实现、PCRE2、libxml2 静态库及 ICU 导入库归档进 `libtxstdlib.a`；固定 ICU DLL、配套私有运行库和许可证（含 `ARGON2-LICENSE`）随 `tx/` 交付，txc 编译程序时会把所需 DLL 放在输出目录。`package.compat` 同时校验编译器、标准库、接口、ABI 指纹和运行时 DLL 的 SHA-256，混用不同批次产物时会在编译阶段报错。首次构建需要下载已固定 SHA-256 的源码与二进制归档，以及固定 Git 提交的 Argon2 源码。普通构建成功后删除中间目录，失败或启用 `-Incremental` 时保留。`tx/stdlib/` 不包含 `.tx` 实现源码。构建时使用 g++；生成的 txc 不会调用它。直接用单配置 CMake 构建时默认也采用 Release；可显式设置 `CMAKE_BUILD_TYPE` 覆盖。

`txc` 在检查、测试、生成 LLVM IR 或编译前验证公开 `.txh`、编译器、最终标准库归档和运行时 ABI 的兼容指纹。缺文件或混用不同构建的产物会给出中文诊断；更新其中任一项后须重新运行构建脚本并整体交付 `tx/`。构建标准库桥接对象时的 `emit-library-llvm` 只验证接口，最终归档完成后再生成兼容清单。完整规则见[标准库公共契约](standard_library_foundation.md#5-包兼容指纹与依赖登记)。

CMake 在 `build/libtxstdlib.a` 保存未合并的原生库；构建脚本每次从它生成 `tx/libtxstdlib.a`，再加入桥接对象和依赖。重复增量构建不会重复合并上次的第三方对象。单独执行 CMake 构建不完成发布包，须运行上述脚本生成兼容清单和完整工具目录。

## 编译源码

命令格式：

```text
txc <源码.tx> [-o <输出.exe>] [--no-lto]
txc check <源码.tx>
txc test <目录或源码.tx> [--case <相对路径>] [--format json]
txc emit-library-llvm <库源码.tx> -o <输出.ll>
txc emit-analysis <源码.tx> [-o <输出.analysis.json>]
```

编译仓库中的示例；默认输出到 tx_build/：

```powershell
.\tx\txc.exe .\example.tx
.\tx_build\example.exe
```

程序展示当前语言的各类语法并以退出码 0 结束。省略 -o 时，输出到仓库根目录的 tx_build/，主文件名与源码相同；例如编译 example.tx 会生成 tx_build/example.exe。指定 -o 时可使用自选位置，txc 会创建其父目录。

路径含空格时，在 PowerShell 中用双引号包裹；执行带引号的程序路径时使用 `&`：

```powershell
& ".\tx\txc.exe" ".\examples\system_env.tx" -o ".\tx_build\中文 输出\中文 程序.exe"
& ".\tx_build\中文 输出\中文 程序.exe" "输入 文件.txt"
```

txc 启动 clang 和链接器时会为各个参数添加必要的引号并处理反斜杠转义，保留完整的工具路径、临时文件路径和输出路径。

`emit-library-llvm` 用于构建标准库：允许无 `main` 的 TX 模块，输出不含程序入口的 LLVM IR。`scripts/build.ps1` 再把 IR 编成目标文件并归档到 `libtxstdlib.a`。预编译标准库接口使用稳定内部类型符号，使库对象与不同导入顺序的用户程序共享相同的结构体类型身份。

编译器先解析并检查 .tx，为所有函数建立 SSA/CFG 与别名、逃逸、调用效果及移动分析，再生成 LLVM IR。默认由同目录 `clang.exe` 生成 ThinLTO bitcode，以 `tx/link/ld.lld.exe` 与 `libtxstdlib_lto.a` 共同优化并链接为原生可执行文件。`--no-lto` 显式使用普通目标文件、`libtxstdlib.a` 和 GNU ld；两种路径均使用同一套 MinGW ABI。生成程序需要的运行时 DLL 会复制到输出目录。语法或类型错误会以“文件:行:列: 错误：原因”的形式报告；后端失败时会显示编译器或链接器的输出。

`emit-analysis` 只输出 JSON 分析记录，不调用链接器。记录包含每个函数的控制流块、异常边、支配关系、SSA phi 与输入、别名集合、参数修改/保存/返回关系和移动候选；默认保存到 `tx_build/<源码名>.analysis.json`。分析覆盖本次加载的所有模块，未知动态行为保守处理，细节见[静态执行体系](static_execution_architecture.md)。工具包兼容清单 v3 还校验 ThinLTO 静态库、clang 和 ld.lld 的指纹，交付时需要整体替换 `tx/`。

程序参数传给生成的可执行文件，通过 `system.args()` 读取，不写在 txc 编译命令后：

```powershell
.\tx\txc.exe .\examples\system_env.tx
.\tx_build\system_env.exe "输入 文件.txt" --output "输出目录"
```

接口语义见[系统与环境变量](system_env.md)，main 仍声明为无参数的 `def main() -> int`。

## 只检查源码

```powershell
.\tx\txc.exe check .\example.tx
```

check 会进行词法、语法、名称解析和静态类型检查，包括 main 入口与返回路径要求。检查通过时输出“语法和类型检查通过”，退出码为 0；检查失败时报告错误并返回非零退出码。这个命令不会生成目标文件或调用链接器。

## 运行 TX 测试

```powershell
.\tx\txc.exe test .\tests\diagnostics
.\tx\txc.exe test .\tests\diagnostics --case basic_test.tx --format json
.\tx\txc.exe test .\tests\diagnostics\basic_test.tx
```

目录模式发现名称以 `_test.tx` 结尾的文件；文件模式可显式运行任意 `.tx` 测试源码。每个文件单独编译、执行并汇总，通过、失败、崩溃和编译错误分别计数。`--case` 选择目录中的一个相对文件路径，`--format json` 输出机器可读报告。可在测试 `main()` 中用 `test.run_case` 显式注册命名回调。完整接口与当前边界见[测试、日志与诊断](test_log_debug.md)。

语言写法见 [语法与类型规则](syntax.md)，可运行的完整示例见 [example.tx](../example.tx)。
