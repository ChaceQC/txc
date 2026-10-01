# Windows 图形 G2–G3

状态：实现及定向自动验证通过；真实桌面人工验收待执行。接口冻结日期：2026-10-02。

## 公开契约

保持现有语法、资源共享语义和 UI 线程约束。`graphics.txh` 扩展路径、画刷、状态栈、图片、离屏和输入；`graphics_text.txh` 提供 DirectWrite 字体与布局。全部调用由编译器静态选择 C ABI。

- 路径 `create_path(app, fill_rule)` 接受 `alternate/winding`；move/line/bezier/close 构建，finish 冻结；冻结后不能修改。每路径最多 65536 段。
- 画刷包括纯色、线性和径向渐变。色标是 `vector<gradient_stop>`，2–256 个，位置有序且在 [0,1]。颜色为 sRGB 直通 alpha，绘制使用预乘 source-over。
- `save/restore` 保存变换和裁剪，最多 64 层；裁剪最多 64 层。`transform` 按当前矩阵乘新矩阵；`clear` 忽略裁剪和变换；提交要求 save/restore 平衡。
- 图片支持 PNG/JPEG/BMP 的文件、bytes 解码和 RGBA8 像素构造。每边 1–16384，单图最多 256 MiB，会话图像预算 512 MiB。解码先检查尺寸，应用 EXIF 朝向，将有配置的图片转换到 sRGB；无配置按 sRGB 解释，不支持的配置返回 decode_failed。
- surface 使用像素宽高及正 DPI；帧先写临时 WIC 位图，提交成功才替换原内容。snapshot 是独立不可变图片。PNG 以直通 alpha 导出，透明像素 RGB 归零；临时同目录文件提交，默认拒绝覆盖，失败保持旧文件。
- 字体 family、大小，布局文本、宽高均显式传参。默认系统字体回退、自动换行、左对齐、顶对齐、自然行距；`set_layout_options` 可设置对齐、换行和 locale。测量及命中测试返回固定记录；文本索引为 Unicode 标量半开区间，命中吸附 DirectWrite cluster。选择返回 `vector<gfx.rect>`，双向文字可有多个矩形。文本上限 1 MiB UTF-8。
- `enable_text_input`、`set_ime_rect` 控制自绘输入和候选窗位置。composition 载荷为当前预编辑快照，提交只产生一次 text_input；停用、失焦清空组合和代理对状态。键名使用 a–z、0–9、f1–f24、left/right/up/down、enter/escape/tab/backspace/delete/home/end/page_up/page_down/space、shift/control/alt/meta，其他键为 unknown。
- 鼠标位置为 DIP，滚轮以一格为 1。移动仅连续合并；捕获显式启停，失焦释放捕获并清空按键。定时器 ID 会话内不复用，间隔 10–2147483647 ms，单窗口最多 256；相同 timer 合并，最小化时不交付，恢复后重新计时。
- 路径、画刷描述、字体、布局、图片像素和 surface 不依赖窗口 render target。窗口设备丢失使当前帧失效并请求重画；下一帧重建目标，重新上传图片和画刷。资源由会话持有，GC 只标记，UI 线程释放 COM。

## 验证记录

2026-10-02，当前 Windows x64 环境：

| 项目 | 结果 |
| --- | --- |
| 普通/ThinLTO 构建 | 编译器、两个标准库产物、系统 import library 与兼容性清单生成成功，LLVM 23.1.2 |
| TX G2–G3 两种链接模式 | 中文路径 PNG、预乘/直通 alpha 往返、拒绝覆盖保全、图片限额、DirectWrite 中文/emoji/双向布局与标量命中/选择、路径/渐变、帧取消/栈不平衡、定时器、关闭失效通过 |
| 原生验证 | clear 忽略裁剪、透明离屏像素、移动合并/按下屏障、代理对组装、失焦/队列溢出清空按键、销毁并重建 render target 通过 |
| 基础回归 | G0–G1 和 U0–U1 原有 TX 行为及跨线程静态拒绝通过 |
| 视觉产物 | 已检查生成的绘图 PNG，含渐变、中文、emoji 与双向文字 |

复现：`python scripts/check_graphics_gui_extensions.py`。可传入 `graphics gui baseline native` 中的分组名称，只运行指定部分。程序、IR 和 PNG 位于 `tx_build/graphics_gui_g2_u3/`。

交互示例：`tx/txc.exe examples/graphics/interactive_scene.tx`，再运行 `tx_build/interactive_scene.exe`。已验证示例编译；真实交互仍按人工清单执行。

微软拼音的候选窗与组合取消、真实多显示器混合 DPI、实际拖动/捕获、远程桌面/软件回退和 GPU 实际移除仍待人工验收。模拟消息、后端目标主动释放及重建不等同于这些验收；本轮未做性能基准或 Linux 构建。
