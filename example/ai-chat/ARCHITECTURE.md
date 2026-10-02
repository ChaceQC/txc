# TX Chat 代码分层

本次重构保留 Responses 协议、配置与会话文件格式、加密方式及用户操作语义。

## 职责

- `main.tx`：创建 `application` 并运行，不处理控件、网络或文件。
- `app/application`：应用生命周期、事件分发及异常收束。
- `app/workspace_controller`：启动恢复、保存、历史切换和数据目录切换。
- `app/chat_controller`：发送、流式显示和成功提交；生成中的状态只属于该控制器。
- `ui/chat_view`：控件私有化，按侧栏、标题、聊天、连接和数据页面构建；向应用提供界面操作与语义动作。
- `domain/conversation`：封装消息、元信息及未保存状态，负责有效会话的序列化和恢复，不访问文件或 GUI。
- `storage/session_repository`：原子保存、加载、历史索引和稳定 ID；条目信息用完整记录组织，不再维护平行数组。
- `storage/connection_settings`：本地连接配置及旧版兼容。
- `storage/credential_cipher`：crypto 加解密和独立密钥文件。
- `storage/data_paths`：默认位置、自定义目录及定位文件。
- `network/chat_worker`：拥有线程、通道和取消资源，统一提交、轮询与关闭。
- `network/responses_client`：HTTP、认证、错误脱敏、流式接收及响应资源关闭。
- `network/response_stream`：增量 SSE 解帧和 Responses 事件解析，不依赖 GUI、线程或文件。

## 约束

界面控件不进入后台线程；线程只传递请求和回复数据。纯数据消息及历史条目保留为 struct，具有状态与行为的组件使用 class。

只有完整成功回答进入 conversation。失败或断流的半截文本由 chat_controller 展示，不写入会话上下文。空闲事件循环阻塞等待，流式进行时有限轮询。

消息集合与持久化元信息由 conversation 方法维护；控制器不直接改写其内部字段。历史条目 ID 不复用已经删除的 ID。保存失败与保存后刷新失败分别报告。

界面绘制前读取布局尺寸，绘制时不刷新布局；常规错误不触发重绘循环。密钥不进入日志、异常展示或会话文件。重构只编译，不启动验证脚本。
