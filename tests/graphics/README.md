# G0–G1 定向验证与人工验收

在 Windows 构建完成后执行 `python scripts/check_graphics.py`。脚本只覆盖本阶段接口，并在 `tx_build/graphics_g0_g1/` 保存程序和 IR。

自动项：

- 错类型、伪造资源、跨线程捕获、嵌套非 Send 类型在源码位置拒绝。
- 普通静态库与默认 ThinLTO 分别编译运行 `behavior.tx`；检查 result/option、别名、容器、动态恢复、深拷贝限制、窗口关闭和帧错误。
- IR 必须直接调用 `txrt_graphics_*`，几何按 `double` 参数传入。
- `windows_lifecycle.cpp` 检查 UI 线程、COM apartment 冲突的数值错误码、窗口客户区 DIP 换算、最小化/恢复、尺寸改变、模拟 DPI 消息、消息合并/溢出、超时等待、最后一个资源引用释放与显式退出清理。
- `exception_cleanup.tx` 检查未捕获图形错误的源码位置和进程退出；原生测试额外检查退出清理后 HWND 已销毁。

人工项（当前均待验收）：

1. 编译运行 `examples/graphics/two_windows.tx`，分别关闭两个窗口，另一窗口应继续响应。
2. 持续拖动缩放，最小化后恢复，遮挡后露出，检查图形完整、没有闪烁或旧画布残留。
3. 在 100%/150%/200% 混合 DPI 显示器间移动窗口，检查 DIP 大小、客户区尺寸和新帧清晰度。模拟消息验证不代替真实多显示器验收。
4. 在远程桌面或无法创建硬件目标的环境验证软件回退，并通过 `gfx.backend(window)` 核对后端。
5. 在目标最低 Windows 版本上核对整包运行。GUI 子系统、无控制台错误提示、输入法和辅助功能留在 G3–G5 的对应验收中。
