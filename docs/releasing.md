# GitHub CI/CD 与版本发布

公开仓库：<https://github.com/ChaceQC/txc>。
工作流位于 `.github/workflows/ci.yml`，构建、安装包验证与发布均在 GitHub 托管 runner 上执行。
当前交付平台为 Windows x64。

## CI

分支 push、pull request 和手动 `workflow_dispatch` 都会执行同一条构建链：

1. 在 Windows Server 2022 runner 上使用 Python 3.12 及预装的 CMake、Ninja、7-Zip。
2. 下载并核对 SHA-256，安装固定 GCC 13.1.0 POSIX/SEH/MSVCRT 和 LLVM 23.1.2。
3. 执行 `scripts/build.ps1`，生成编译器、普通与 ThinLTO 标准库、链接器、DLL、接口和兼容清单。
4. 验证正常工具包及 7 类兼容指纹错配；将整套产物打成 ZIP。
5. 将实际 ZIP 解压到包含中文与空格的临时路径，清除构建工具 PATH 后检查语法，在 ThinLTO 和普通链接模式下分别编译并运行数值样例与 ICU 示例。
6. 上传通过验证的 ZIP 和 SHA256SUMS.txt，保留 14 天。构建失败保留 CMake 诊断。

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

Tag push 会重新在 GitHub 上执行完整构建链。只有构建和 ZIP 安装验证均成功，
发布 job 才下载这次构建的 artifact、核对 SHA-256，并生成 Release 与更新记录。
工作流使用 GitHub 自动提供的 `GITHUB_TOKEN`，仅发布 job 授予 `contents: write`，
无需配置个人令牌。发布内容为：

- `txc-vX.Y.Z-windows-x64.zip`：编译器、clang、两套标准库、链接组件、运行时 DLL、
  第三方许可证、`stdlib/*.txh`、README、`docs/usage.md`、`docs/syntax.md`、全部文档与示例。
- `SHA256SUMS.txt`：上述 ZIP 的 SHA-256。

ZIP 内的 `build-info.json` 记录版本、源码提交、工具链版本和 Actions 运行地址。
请整体保留 `tx/`；禁止混用不同版本的编译器、库、接口或 DLL。
解压到任意路径后可在 ZIP 顶层执行 README 的快速使用命令。

失败时在 Actions 中查看相应步骤的日志。依赖下载等瞬时故障可重跑同一 workflow。
已有同名 Release 时拒绝覆盖资产；需要修改发布内容时使用新的版本 tag。
