# GitHub CI/CD 与版本发布

公开仓库：<https://github.com/ChaceQC/txc>。
工作流位于 `.github/workflows/ci.yml`，构建、安装包验证与发布均在 GitHub 托管 runner 上执行。
工作流分别构建 Windows x64 和 Linux x86_64（Ubuntu 24.04 / glibc 2.39）工具包。

## CI

分支 push、pull request 和手动 `workflow_dispatch` 都会执行两条构建链。Windows 链：

1. 在 Windows Server 2022 runner 上使用 Python 3.12 及预装的 CMake、Ninja、7-Zip。
2. 下载并核对 SHA-256，安装固定 GCC 13.1.0 POSIX/SEH/MSVCRT 和 LLVM 23.1.2。
3. 执行 `scripts/build.ps1`，生成编译器、普通与 ThinLTO 标准库、链接器、DLL、接口和兼容清单。
4. 验证正常工具包及 7 类兼容指纹错配；执行自绘 GUI 的保留控件、容器/数据、基础交互、整形 ABI、命令/主题/模态与 UIA 客户端检查，再将整套产物打成 ZIP。
5. 将实际 ZIP 解压到包含中文与空格的临时路径，清除构建工具 PATH 后检查语法，在 ThinLTO 和普通链接模式下分别编译并运行数值样例、ICU 示例、原有 GUI 和自绘 GUI 发行自检；检查 Windows PE 子系统、DPI/Common Controls 清单及控件/绘图启动。
6. 上传通过验证的 ZIP 和 SHA256SUMS.txt，保留 14 天。构建失败保留 CMake 诊断。

Linux 链在 `ubuntu-24.04` 上执行 `scripts/setup_linux.py` 安装原生依赖，以 LLVM 18 构建编译器和两套标准库。
构建输出显式固定为 `tx/linux`，既有运行时检查通过 `TXC_TOOL_DIR` 使用同一目录，打包显式传入 `--tool-dir tx/linux`。
`scripts/package_release.py` 生成保留可执行权限的 tar.gz，`scripts/check_linux_package.py` 把实际包解压到中文与空格路径，清空工具 PATH 和动态库搜索环境后运行普通/ThinLTO、Unicode、系统文件和自绘 GUI 接口验证。
Linux 产物以 `txc-linux-x64` Actions artifact 上传，缓存和失败诊断与 Windows 独立。

GUI CI 入口为 `scripts/check_native_gui_ci.py`。Windows 显式使用 Arial，Linux 使用文泉驿正黑，
整形 ABI 使用 DejaVu Sans。Linux GUI 检查与解包检查在 Xvfb 和独立 D-Bus 会话运行；
AT-SPI 通过真实 Registry 遍历与操作，Portal 对端在额外隔离会话验证成功/取消/错误。
不启动真人输入法或读屏验收，也不把 Xvfb/协议检查标为真实桌面体验验收。

每个平台上传 `*-native-gui-results`，包含 `ci/results.json`、逐项日志、位图和 LLVM IR，
失败时也保留；不会因协议测试失败而跳过失败或把错误计为通过。
全量 Unicode 官方用例、性能测试和其他未修改标准库套件不由此 GUI 门禁触发。

CI 缓存固定工具链及 ccache，构建失败时也保存已经完成的编译缓存；没有运行性能测试或全量标准库测试。
上游 MinGW 缺少默认 manifest 对象时，构建脚本通过 windres 生成相同权限和系统兼容声明的对象。
分支包版本为 `dev-<commit>`，只产生 Actions artifact。

## 推送 tag 发布

正式版本 tag 使用 `v主版本.次版本.修订版本`，例如 `v0.1.0`。
预发布可以使用 `v0.2.0-rc.1`；包含后缀的版本自动标记为 GitHub Pre-release。
从要发布的已提交版本执行：

```powershell
git push origin master
git tag -a v0.1.0 -m "TX Compiler v0.1.0"
git push origin v0.1.0
```

Tag push 会重新在 GitHub 上执行两端构建链。只有两端构建和安装验证均成功，
发布 job 才下载这次构建的 artifact、核对 SHA-256，并生成 Release 与更新记录。
工作流使用 GitHub 自动提供的 `GITHUB_TOKEN`，仅发布 job 授予 `contents: write`，
无需配置个人令牌。发布内容为：

- `txc-vX.Y.Z-windows-x64.zip`：编译器、clang、两套标准库、链接组件、运行时 DLL、
  第三方许可证、`stdlib/*.txh`、README、`docs/usage.md`、`docs/syntax.md`、全部文档与示例。
- `txc-vX.Y.Z-linux-x64.tar.gz`：Linux ELF 编译器、clang/LLD、普通/ThinLTO 标准库、原生共享库、接口、文档、示例和许可证。宿主提供 glibc 与系统加载器。
- `SHA256SUMS.txt`：两份安装包的 SHA-256，发布前分别验证再合并。

安装包内的 `build-info.json` 记录版本、源码提交、工具链版本和 Actions 运行地址；Linux 的 `tx/licenses/packages.txt` 另记录实际系统依赖版本。
Windows 包还记录图形最低目标、子系统/链接模式和工作区是否含未提交改动；最低目标是支持契约，不能视为已完成基线实测。G5/U6 定向错误提示验证入口为 `python scripts/check_gui_release.py`，只关闭脚本自身创建的错误框，具体记录见 [G5/U6](graphics_g5_gui_u6.md)。
请整体保留 `tx/`；禁止混用不同版本的编译器、库、接口或 DLL。
解压到任意路径后可在 ZIP 顶层执行 README 的快速使用命令。

失败时在 Actions 中查看相应步骤的日志。依赖下载等瞬时故障可重跑同一 workflow。
已有同名 Release 时拒绝覆盖资产；需要修改发布内容时使用新的版本 tag。
