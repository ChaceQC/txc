# TX Chat

使用当前 .tx 语言编写的 Windows 桌面应用。支持 OpenAI Responses 流式文本对话、保存会话、历史列表恢复和继续对话、本地连接配置及自定义数据目录。

## 编译运行

在仓库根目录执行：

```powershell
.\tx\txc.exe .\example\ai-chat\main.tx --subsystem windows -o .\tx_build\ai-chat\ai_chat.exe
.\tx_build\ai-chat\ai_chat.exe
```

分发时携带可执行文件所在目录的运行时 DLL。

## 使用

- 在“连接设置”填写 Base URL、API key 和模型名称，点击“保存连接设置”。Base URL 可填 `https://api.openai.com/v1` 或完整 `/responses` 地址；旧 `/chat/completions` 后缀会转换为 `/responses`。
- 在“对话”页输入消息，点击“发送消息”或按 Ctrl+Enter；Enter 换行。回答逐步显示。
- “保存会话”保存完整的成功对话及连接设置。左侧点击已保存会话可恢复并继续；继续后再次保存会更新原会话。切换或关闭前会提示保存未保存的对话。
- “本地数据”页的“数据保存路径”显示当前目录，可输入绝对路径或选择文件夹，再点击“应用目录”。切换会加载目标目录的历史，原目录文件不移动、不删除；目标目录没有连接配置时沿用当前配置。

## 本地文件

默认数据目录为 `%LOCALAPPDATA%\TX\ai-chat\`：

- `settings.json`：URL、模型和 crypto 加密后的 API key；启动时自动恢复。
- `settings.key`：由 `crypto.generate_key` 生成的 32 字节随机加密密钥。
- `sessions/*.json`：完整对话、标题、接口地址和模型，不含 API key。
- 默认目录中的 `location.json`：自定义数据目录的位置，即使改换数据目录，此定位文件仍保留在默认目录。

API key 通过标准库 `crypto.encrypt/decrypt` 使用 AES-256-GCM 加密和认证，不再调用 PowerShell 或 DPAPI。加密密钥不显示在日志中。配置和 `settings.key` 需要一起备份；同时取得这两个文件即可解密，因此本地文件加密不等于 Windows 账户隔离。旧版配置只恢复 URL 和模型，须重新填写 API key；首次保存新版时保留 `settings.json.v1.bak`，不调用 Windows 解密旧密文。历史会话恢复其 URL 和模型，密钥使用当前连接配置。

## 协议及限制

请求为 `POST /responses`，使用 Bearer 认证及 `model`、`input`、`stream: true`、`store: false`。解析 SSE 的 `response.output_text.delta`、`response.refusal.delta` 和 `response.completed`；`error`、`response.failed`、`response.incomplete` 或提前断流视为失败。完整成功回答才加入后续上下文，不自动重试。参考：[官方流式事件](https://developers.openai.com/api/reference/resources/responses/streaming-events)。

支持增量 UTF-8 解码、跨块 SSE 行和 CR/LF 换行。HTTP 读取上限 4 MiB，网络阶段超时 60 秒，并非整段生成的总时限。生成期间暂不能切换会话或保存，关闭操作提示等待完成。当前为文本客户端，不执行工具调用或处理图片。

聊天区仅显示最近约 15000 个字符，完整历史仍保存在内存及会话文件中。输入最多 16000 个字符。HTTP 错误显示状态码和响应正文，自动遮蔽当前密钥并限制错误正文长度。

## 源码

- `main.tx`：仅创建并运行应用。
- `app/`：应用生命周期、工作区控制器和流式对话控制器。
- `domain/`：私有化消息和元信息的 `conversation`，不访问 GUI 或文件。
- `storage/`：`session_repository`、`connection_settings`、`credential_cipher` 和 `data_paths`，分别负责会话存储、连接配置、加密和目录定位。
- `network/`：`chat_worker`、`responses_client` 和 `response_stream`，分别管理后台线程、HTTP 请求和 SSE 解析。
- `ui/chat_view`：控件引用私有化，按侧栏、标题、聊天页、连接页、数据页分别构建，对外提供语义操作。
- `check_behavior.tx`：可选的定向检查，覆盖分块文本、错误事件、会话往返和加密配置。

完整职责和依赖约束见 [ARCHITECTURE.md](ARCHITECTURE.md)。本次重构仅编译，未运行测试或验证脚本。

流式读取还修复了标准库 `src/stdlib/httpx_client_session.cpp`：WinHTTP 按当前可用字节读取，避免等待缓冲区填满后才显示回答。最新界面及数据目录设置仍需用户实际验收。

空闲界面使用阻塞事件等待，仅生成回答时每 25 毫秒轮询后台结果。运行时 GC 登记表会独立清理过期弱引用，避免常驻工作线程延后全图扫描时积累已销毁对象的登记记录；不改变活对象图的并发回收规则。此项已修复源码并重新构建，未运行长时间内存采样。
