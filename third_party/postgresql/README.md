# PostgreSQL libpq 发行依赖

- 固定包：EDB PostgreSQL Windows x64 binaries 18.4-1。
- URL：https://get.enterprisedb.com/postgresql/postgresql-18.4-1-windows-x64-binaries.zip
- SHA-256：`7effe34c0bf89027b3f171447d351cbc460f4566c8d0f643daec67f140787858`。
- `cmake/postgres.cmake` 校验并提取依赖。仅使用 libpq C ABI；`libpq.def` 列出当前需要的导出，由工具链 `dlltool` 生成 GNU 导入库并合并进 `libtxstdlib.a`。
- 发行包含 `libpq.dll`、`libssl-3-x64.dll`、`libcrypto-3-x64.dll`、`libintl-9.dll`、`libiconv-2.dll`、`libwinpthread-p.dll` 和 `vcruntime140.dll`，全部来自上述固定归档并纳入 `package.compat`。libintl 的 pthread 导入名改为 `libwinpthread-p.dll`，避免与编译器的另一套运行库混用。
- PostgreSQL License 从包中 `server_license.txt` 复制为 `tx/POSTGRESQL-LICENSE`；OpenSSL、gettext、iconv 等随包声明从 `commandlinetools_3rd_party_licenses.txt` 复制为 `tx/LIBPQ-THIRD-PARTY-LICENSES`。libpq 及相关项目的许可证各自适用；系统 Windows/UCRT 组件遵循系统平台许可。
- 数据库服务器、pgAdmin 和服务管理程序仅供构建依赖/本机临时验证使用，不随 TX 发行。`scripts/check_db_postgres.py` 在 `tx_build` 下初始化临时实例，验证后关闭并删除实例。
