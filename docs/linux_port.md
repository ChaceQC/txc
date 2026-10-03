# Linux 原生适配与双平台 CI/CD

## 目标和边界

目标为 Linux x86_64 原生编译器及完整标准库，与 Windows x64 保持同一套 `.tx` / `.txh` 语言接口。第一条 Linux 交付基线为 Ubuntu 24.04（glibc 2.39）；其他发行版和 ARM64 不能据此宣称已验证。

编译期名称解析、静态类型、直接调用和 LLVM 代码生成模型保持不变。平台差异由构建选源和小范围系统接口封装处理。不得把缺失功能改为静默成功，也不得把 `unsupported_platform` 占位实现当作完成。

## 实施顺序

1. 构建依赖：区分 Windows 固定二进制包和 Linux 原生依赖，确定 LLVM / C++23 工具链，保留普通与 ThinLTO 标准库。
2. 编译器驱动：UTF-8 参数、可执行文件定位、子进程、包指纹、ELF 目标与链接、测试和性能分析入口。
3. 基础系统：编码、文件和路径、原子替换与锁、文件监控、环境、进程、IPC、资源统计。
4. 网络与安全：POSIX socket / DNS、HTTP / WebSocket、TLS / X.509 / PKCS#12、HTTP/3。
5. 异步运行时：定时器、文件读写、网络等待、取消及资源回收；维护既有 task scope 和错误语义。
6. 交付：Linux 构建脚本、tar.gz、兼容清单、移出源码树的安装验证，Windows/Linux Actions 和共同发布门禁。

## 平台规则

- Linux 路径及环境变量按平台语义区分大小写，文件名通过 UTF-8 接口传递；不套用 Windows 盘符或命令行引号规则。
- Linux 可执行文件无 `.exe` 后缀，编译目标为 `x86_64-unknown-linux-gnu`；Windows 继续使用 `x86_64-w64-windows-gnu`。
- Linux 子进程直接传递参数数组，不隐式经 shell；信号、退出状态、超时和进程组行为必须明确映射。
- 文件权限读取/设置 POSIX 位，文件系统不提供创建时间时 `created_millis` 返回 0。符号链接按 `stat` / `lstat` 区分跟随与不跟随。
- Linux 异步文件使用有界工作线程执行 `pread` / `pwrite`，取消在块边界生效；定时器独立等待。性能分析采线程 CPU 时间和 TX 源码帧，不提供 Windows 原生 PC 采样。
- 普通库和 ThinLTO 库必须分别验证；工具包需要记录目标平台和工具链，禁止跨平台或跨构建混装。
- 发布包必须在离开源码和构建目录后进行编译、链接和运行验证；分支产物留在 Actions，只有版本 tag 才触发 Release。

## 依赖与验证入口

`python3 scripts/build_linux.py` 构建普通与 ThinLTO 静态库。`scripts/setup_linux.py` 提供 Ubuntu 24.04 安装入口，具体系统语义见 [进程](process.md)、[IPC](ipc.md)、[网络](linux_network.md)、[TLS](tls.md) 与 [HTTP/3](http3.md)。

开发和 CI 默认输出均为 `tx/linux`，可通过 `--output` 显式指定。运行时验证设置
`TXC_TOOL_DIR="$PWD/tx/linux"`，打包使用 `python3 scripts/package_release.py --tool-dir tx/linux`。
打包后的 tar.gz 内仍统一以 `tx/` 为工具包目录，不改变用户解压后的命令。
自绘 GUI 的 X11/XWayland、AT-SPI 和 Portal 依赖及验收边界见 [系统集成契约](native_gui_system.md)。

定向验证使用 `TXC_TOOL_DIR` 指定工具目录。安装包、进程与异步、网络、证书、TLS、HTTP/3 分别由 `check_linux_package.py`、`check_linux_runtime.py`、`check_linux_network.py`、`check_x509.py`、`check_tls_stream.py` 和 `check_http3_server.py` 覆盖。安全验证的 Python 依赖列在 `scripts/requirements_ci.txt`，不随工具包运行时分发。

## 完成证据

2026-10-01 已完成 Linux 原生实现及 WSL Ubuntu 24.04 x86_64 定向验证，使用 Clang/LLD 18。实际选入 Linux 标准库的 310 个翻译单元已编译，未选入 `unsupported_platform` / `iocp_unavailable` 占位实现。

| 验证 | 结果 |
| --- | --- |
| 编译器、普通标准库、ThinLTO 标准库、三组 TX 桥接模块 | 原生构建通过 |
| 实际 tar.gz 解压安装 | 清空工具 PATH 和动态库搜索环境后，通过中文空格路径、数值、Unicode、普通/ThinLTO、系统和文件验证 |
| 进程、IPC、监控、定时器、profile、异步文件 | 定向行为验证通过 |
| DNS、TCP/UDP、HTTP 会话、WebSocket、显式代理、WSS 服务端 | 本机回环验证通过 |
| X.509 / PKCS#12 / PKCS#8 与 TLS | 证书用途、信任、资源回收、双向握手、ALPN 和并发随机源验证通过 |
| HTTP/3 | aioquic 中断请求后 GET、TX 信任配置、二进制正文、未知根拒绝和监听器关闭后回复全部通过；进程正常退出 |

运行验证修复了 Linux LLD 缺少 `--eh-frame-hdr` 导致可恢复异常终止进程的问题，以及 MsQuic 卸载后 OpenSSL 退出回调访问已卸载代码的问题。Linux MsQuic 使用 `RTLD_NODELETE` 保持回调代码映射，连接、注册和 API 资源仍正常释放。

实际下载 CI 包后的补充检查发现，仅清空工具 PATH / `LD_LIBRARY_PATH` 仍可能由宿主已安装的库掩盖打包缺漏。Linux 打包现同时保存链接名和 ELF `DT_SONAME` 文件名，补齐 ICU i18n、PCRE2、libsodium、Argon2、libpq、libcurl、c-ares 的运行时名称；安装门禁用 `ldd` 核实编译器、LLVM 工具和生成程序的非 glibc 依赖实际来自包内目录，并支持中文和空格路径。

双平台 GitHub 工作流已在 `codex/linux-support` 分支通过：[运行 36788749124](https://github.com/ChaceQC/txc/actions/runs/36788749124)，对应实现提交 `dac5ad9`。Windows 与 Linux 的构建、实际压缩包安装以及 Linux 运行时/网络/安全门禁均成功。后续提交状态及可下载产物见 [GitHub Actions](https://github.com/ChaceQC/txc/actions/workflows/ci.yml)。源码分支产物为 Actions artifact；版本 tag 通过双平台门禁后发布 Release。Linux 原生支持从 `v0.2.0` 开始交付。
