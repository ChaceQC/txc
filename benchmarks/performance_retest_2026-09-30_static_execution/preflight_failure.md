# 正式采样前的构建预检记录

最初按要求提交当前改动，提交为 `1399b1f123a7494d284ffb644487cec804db60cf`，
再执行 `scripts/build.ps1` 重建和封包。准备标准库参考时，默认 ThinLTO 路径
编译 `database.tx` 失败，链接器报告：

```text
ld.lld: error: undefined symbol: std::__once_callable
ld.lld: error: undefined symbol: std::__once_call
referenced by src/stdlib/crypto_random.cpp
libtxstdlib_lto.a(src_stdlib_crypto_random.cpp.bc)
```

此时尚未建立正式采样 manifest，也未开始任何正式采样。

对照随包 MinGW `libstdc++-6.dll` 及其导入库，实际导出的符号为
`__emutls_v._ZSt15__once_callable` 和 `__emutls_v._ZSt11__once_call`。
Clang 构建 C++ ThinLTO 模块时默认采用原生 TLS，与这些 emutls 导出不一致。

提交 `3792fd4143f528f0605ca89e89bb876f6f456019` 在
`scripts/build_lto.py` 的 C++ 编译参数中加入 `-femulated-tls`。重建后仍然
报告上述符号缺失；单独向链接器传递 emutls 选项也未解决。

继续处理时，在链接阶段明确传递 `--plugin-opt=-emulated-tls`，并将
`crypto_random.cpp` 与 `public_key.cpp` 的一次性初始化改为线程安全的
局部静态初始化，避开 `std::call_once` 对外部模拟 TLS 的导入依赖。
首次初始化结果仍被缓存，失败状态及对外错误语义保持一致。

该修复提交为 `62b5892`。数据库、安全等七个标准库负载均成功链接，
语言特性完成首轮采样；随后模块审计中的 WebSocket 负载暴露了
`ws_client.cpp` 经由 C++ 标准库头文件引入的同类 TLS 依赖。
这一轮未完成的 manifest 和语言样本移至 `interrupted_before_ws_fix/`，
不计入最终结果。

最终在构建入口扫描每个 bitcode 模块的未定义符号，对需要
`std::__once_callable` 或 `std::__once_call` 的编译单元保留同次 GCC
Release 构建的原生对象。其余模块继续使用 ThinLTO；混合归档方式与
原有第三方依赖一致。最终修复提交为
`f1cc9a5368d01d8a9b413facf2495af8f63e5c57`，正式封包成功，构建结果为
316 个 bitcode 模块与 1 个 ABI 原生对象（`ws_client.cpp`）。默认
WebSocket 编译预检通过，随后执行完整采样，两套 completed 记录均已生成。

完整复测使用默认 ThinLTO 模式；测试提交、实际工具链 SHA-256
及最终完成情况见两套归档的 manifest 和 completed 记录。
