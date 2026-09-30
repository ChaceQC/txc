# 文件系统与路径扩展（7.1、7.2）

本模块以 UTF-8 传入和返回路径。`fs` 的新增严格接口失败时抛出 `io_error`；`path` 中需要访问磁盘的接口也如此。旧接口的签名、排序和错误行为保持原样。当前发行目标是 Windows x64，使用宽字符系统 API。

## 7.1 文件系统元信息、筛选与监视

`fs.stat(path)` 跟随最终符号链接，`fs.lstat(path)` 观察链接本身。路径不存在（包括断链的 `stat`）为 `io_error/not_found`；权限不足为 `io_error/permission_denied`。返回的 `file_info` 字段为：

| 字段 | 规则 |
| --- | --- |
| `kind` | `file`、`directory`、`symlink` 或 `other`；只有 `lstat` 能把最终链接报告为 `symlink`。 |
| `size` | 普通文件的字节数；其他类型为 0。 |
| `permissions` | Windows 映射的可读/可写位：普通文件为 `0444` 或 `0666`，目录为 `0555` 或 `0777`；只读属性不代表 ACL 授权。 |
| `created_millis`、`accessed_millis`、`modified_millis` | Unix 纪元毫秒；访问时间可能受文件系统延迟更新策略影响。`lstat` 读取最终链接自身的时间。 |

`fs.set_permissions(path, permissions)` 在 Windows 接受 `0444`、`0666`、`0555`、`0777`，只修改最终目标的只读属性；其他位或不适合目标类型的组合报 `io_error/invalid_argument`。这不是 ACL 管理接口。`fs.create_symlink(target, link_path, directory)` 保留相对目标文本，Windows 由 `directory` 指定链接类型；目标可暂不存在，失败时不留下成功句柄。`fs.read_symlink(path)` 只接受最终组件为符号链接或目录联接的路径，返回存储的目标路径。创建可能因系统策略报 `permission_denied`。

`fs.list_directory_filtered(path, kind, extension, recursive)` 返回相对于 `path` 的 UTF-8 路径，按无符号 UTF-8 字节排序；`kind` 为 `any/file/directory/symlink`，`extension` 为空或带点后缀（如 `.tx`），精确且区分大小写。筛选按不跟随链接的类型进行，递归遍历也不进入目录链接；目录自身不作为结果。旧 `list_directory*` 和 `walk_directory` 不改变行为。

`fs.watch(path, recursive)` 返回不透明 `fs_watcher`，只接受目录。`fs.watch_next(watcher, timeout_millis)` 返回 `watch_event { kind, path }`：`created/deleted/modified/renamed_old/renamed_new` 的 `path` 是相对路径；`timeout` 和 `overflow` 的 `path` 为空。超时 0 表示立即轮询，-1 表示无限等待，其他负数报 `io_error/invalid_argument`。监视事件可合并；缓冲溢出时返回 `overflow`，调用方必须重新扫描。等待超时保留尚未完成的系统请求，不丢弃后续事件。`fs.close_watch` 可重复调用；关闭后等待报 `io_error/closed_handle`。最后一个 TX 引用释放时会自动关闭原生句柄。监视器不支持跨线程转移或共享。Windows 监视队列和路径变动存在竞争，事件不是权限边界或可靠审计日志。

Linux 目录监视使用非阻塞 `inotify`，关闭等待通过 `eventfd` 唤醒；内核队列保留两次调用之间的事件。递归监视为每个真实子目录登记监视，不跟随子目录符号链接。目录移出树时撤销对应监视，移入或新建时登记新子树，并额外返回 `overflow` 提醒调用方重新扫描安装监视期间的变化。内核队列溢出或根目录本身被移动、删除同样报告 `overflow`；此时调用方应检查路径并按需重建监视器。事件名与上文保持一致。

## 7.2 路径解析、分解和比较

旧 `normalize/absolute/relative` 继续按词法处理，不读取磁盘。新增 `path.canonical(path)` 跟随链接并要求整个路径存在；`path.weakly_canonical(path)` 解析已存在的前缀，保留可能不存在的尾部。两者都返回绝对路径，发生访问失败时抛 `io_error`。它们反映调用时的文件系统状态，不能自动证明之后打开该路径是安全的；安全约束应使用目录句柄相对打开与不跟随链接选项。

`path.root_name/root_directory/root_path/stem` 补齐分解，与原有 `parent/file_name/extension` 一起遵循 `std::filesystem::path` 的平台语法。`path.compare(left, right)` 返回 -1、0 或 1，仅做词法比较：先规范 `.`、`..` 和分隔符，Windows 用不区分大小写的序数比较；不访问磁盘，也不判断两个路径是否指向同一对象。`path.equivalent(left, right)` 访问磁盘比较对象身份，两条路径都必须存在。Windows 可存在区分大小写的目录，`compare` 因此也不是访问控制依据。

## 实施与验收边界

本节使用标准库和 Win32 系统 API，不新增第三方依赖。定向验证覆盖中文/空格路径、缺失路径及现有目录联接的 `stat/lstat`、权限映射、筛选与排序、监视事件/超时/关闭、真实路径和平台比较。第 7.7 项补充的链接竞争与文件监视压力/溢出证据见下方记录；第 7.3～7.6 项按各自接口与证据验收。

### 7.1 实施记录（2026-09-27）

- **代码：** `fs.txh` 新增 `file_info`、`stat/lstat`、Windows 只读位映射、符号链接创建/读取、按类型与扩展名筛选，以及不透明 `fs_watcher`。元信息与目录联接目标经 Win32 API 读取；监视器使用重叠 I/O，事件交付前重新挂起读取，关闭和最后引用释放时取消请求并清理句柄。
- **构建：** `scripts/build.ps1` 成功生成 Windows x64 的 `txc.exe`、标准库静态库与兼容指纹，并清理临时 `build/`。
- **定向验证：** `scripts/check_filesystem_path.py` 的元信息、权限合法/非法值、缺失路径、中文空格目录筛选和排序、普通/递归监视、超时与重复关闭通过。当前机器缺少创建符号链接权限，创建操作按契约返回 `io_error/permission_denied`；通过系统现有目录联接验证了 `lstat` 观察链接、`stat` 跟随链接及 `read_symlink` 读取目标。
- **平台与终态边界：** 权限位是 Windows 只读属性映射，不含 ACL；事件可合并或溢出。创建链接成功路径因本机系统策略未实测，断链、链接竞争和监视溢出的后续证据见第 7.7 记录；跨平台终态独立验收。

### 7.2 实施记录（2026-09-27）

- **代码：** `path.txh` 新增 `canonical/weakly_canonical`、根与词干分解、Windows 平台词法比较及访问磁盘的对象身份比较；已有 `normalize/absolute/relative` 的词法语义保持不变。新错误沿用 `io_error` 的稳定代码。
- **构建：** 与 7.1 同次完整构建，兼容指纹随 `.txh` 和 LLVM ABI 更新，临时 `build/` 已清理。
- **定向验证：** `scripts/check_filesystem_path.py` 通过中文空格路径、已有前缀加缺失尾部、根/文件名/词干/扩展名分解、大小写与分隔符比较、真实对象同一性，以及 `canonical` 缺失路径的 `io_error/not_found`。
- **平台与终态边界：** `compare` 只作 Windows 不区分大小写的词法比较，不读取磁盘；`canonical` 和 `equivalent` 访问文件系统，结果受调用时状态影响。路径字符串解析不能充当安全访问边界；跨平台行为仍待专项验收，链接竞争的后续证据见第 7.7 记录。

### 7.7 边界验证（2026-09-27）

- **权限与短读：** `scripts/check_process.py filesystem` 在中文空格目录中写入二进制短文件，确认大请求返回实际短块、下一次读取才返回 EOF；只读权限拒绝写入后恢复权限。短写与缓冲满超时由同脚本的 `pipes` 组覆盖。
- **链接竞争：** 原生用例使用当前用户可创建的目录联接，后台持续切换两个目标，前台执行 200 轮 `stat/lstat/read_symlink` 与目标文件大小检查，均得到合法目标状态；再把目标移走，确认 `lstat` 保留链接身份而 `stat` 报 `not_found`。本机 `CreateSymbolicLinkW` 仍受权限限制，返回 `permission_denied` 的路径已验证；不把目录联接测试记为该受限成功路径的实测。
- **溢出与恢复：** 在未消费通知时创建 1600 个长文件名，真实触发系统监视缓冲溢出并取得 `overflow`；排空后新建文件仍取得 `created`，重复关闭无副作用。
- **等待中关闭修复：** 原生复现发现静态 pthread 互斥锁与动态 pthread 条件等待的布局混用会崩溃；监视器改用成对的 Win32 SRW 锁/条件变量。取消系统请求、唤醒等待者、等待引用退出再关闭句柄的流程经定向用例验证，等待方得到 `closed_handle`。这不放开 TX `fs_watcher` 的 `Send/Sync` 限制。
- **构建与边界：** 修正已进入重新构建的 Windows x64 标准库，最终产物上文件系统组复核通过；`build/` 已清理。跨卷/断电保证、ACL 语义及非 Windows 平台不扩大到本次证据之外。
