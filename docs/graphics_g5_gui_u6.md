# 图形 G5 与 GUI U6 发行

接口冻结及验证日期：2026-10-02。发行接入和定向自动验证已完成；最低系统基线和真实桌面验收待执行。

## 可执行入口

`txc 源码.tx --subsystem console|windows [-o 程序.exe] [--no-lto]`。
默认 console；选项只接受一次，只用于编译可执行程序。windows 仅支持 Windows，
仍由 CRT 初始化并调用 TX main，使用 Windows PE 子系统及 mainCRTStartup 入口。
两种链接模式都嵌入 PerMonitorV2、Common Controls v6、asInvoker 和 Windows 10 兼容清单。
console 图形程序仍保留现有图形清单行为。

## 错误与分发

windows 程序保留继承的标准流。未捕获错误有可用 stderr 时写入 stderr；没有可用 stderr 时，
写 UTF-8 日志到 `%LOCALAPPDATA%/TX/diagnostics/`，失败则尝试 `%TEMP%/TX/diagnostics/`。
文件名含进程 ID 和时间，包含错误及源码调用栈；错误框显示精简信息及日志路径。
两个目录均无法写入时，错误框明确提示日志保存失败，并向调试输出发送错误。
错误退出码为 1，原有资源清理仍执行。不创建或附加控制台。

复制生成的 exe 时，应一并复制输出目录中编译器放置的运行时 DLL；Windows 系统图形 DLL
由操作系统提供。工具链包则整体保留 tx/，包含普通和 ThinLTO 静态库、链接组件、
GUI 清单、全部公开 .txh、文档与示例。无 Python/.NET/浏览器运行时要求。

## 示例与接口

公开签名以 `tx/stdlib/graphics.txh`、`graphics_text.txh`、`gui.txh`、`gui_data.txh`、
`gui_dialog.txh` 为准。接口语义和限额见 [G0–G1](graphics_g0_g1.md)、
[G2–G3](graphics_g2_g3.md)、[U0–U1](gui_u0_u1.md)、[U2–U3](gui_u2_u3.md)、[U4–U5](gui_u4_u5.md)。

```powershell
.\tx\txc.exe examples\graphics\two_windows.tx --subsystem windows -o tx_build\two_windows.exe
.\tx\txc.exe examples\gui\task_workspace.tx --subsystem windows -o tx_build\task_workspace.exe
.\tx\txc.exe examples\gui\task_workspace.tx --subsystem windows --no-lto -o tx_build\task_workspace_native.exe
```

目标基线为 Windows 10 1703+ / Windows 11 x64。清空开发工具 PATH 的本机安装验证
不能替代干净目标机验证。Windows 10 1703、Windows 11 干净机、混合 DPI、微软拼音、
Narrator、高对比度及真实 GPU 恢复在实际执行前均为待验收。

## 自动验证记录

环境：Windows 11 x64，系统 build 26200，GCC 13.1.0、LLVM 23.1.2。

| 项目 | 结果 |
| --- | --- |
| 构建 | 编译器、普通/ThinLTO 标准库及兼容摘要已生成 |
| 双链接 GUI 入口 | PE subsystem=2，CRT 调用 TX main，DPI V2/Common Controls/asInvoker 清单存在；原生控件和 Direct2D 帧可创建、提交并退出 |
| 重定向及错误退出 | 两种链接模式保留 stdout/stderr，错误退出码 1，错误文本包含源码位置和调用栈 |
| 无 stderr | 两种模式分别验证 LocalAppData 日志、主目录不可写时 TEMP 回退、两目录均不可写时明确错误框；验证脚本只操作自身进程 |
| console 兼容 | 显式 console 的 PE subsystem=3；原有默认 console 的两种链接和数值/ICU 程序通过安装验证 |
| 参数拒绝 | 缺失值、非法值、重复 --subsystem 返回非零，不进入编译 |
| 实际 ZIP | 解压到包含中文和空格的临时目录，PATH 只保留 Windows/System32 与 Windows；普通/ThinLTO 数值、ICU、GUI 自检编译运行通过 |
| 完整示例 | task_workspace 的 windows 普通/ThinLTO、two_windows 的 windows ThinLTO 编译通过；本阶段未重新执行这些交互示例的人工验收 |
| Linux | 已提供 Windows 子系统明确拒绝路径；本次未运行 Linux 构建或测试 |

复现入口：`python scripts/check_gui_release.py` 和 `python scripts/check_ci_package.py`。
后者检查 `dist/` 中实际 Windows ZIP；CI 已复用这个入口。本次未运行全仓测试。
本地程序位于 `tx_build/gui_release/`，工具链位于 `tx/`。
本地 ZIP 含未提交的工作区代码，`build-info.json` 的 `working_tree_dirty=true`；未推送或发布。
产物确认后尝试清理临时 build/，自动审批返回 `blocked by policy`，因此该目录仍保留。
