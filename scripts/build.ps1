param([switch]$Incremental)

$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)
$OutputEncoding = [System.Text.UTF8Encoding]::new($false)
$env:PYTHONUTF8 = '1'

$project_root = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$build_dir = [System.IO.Path]::GetFullPath((Join-Path $project_root 'build'))
$tool_dir = Join-Path $project_root 'tx'
$postgres_root = Join-Path $build_dir '_deps/postgres_binary-src'
$bundled_cmake = 'D:\CLion 2024.3.4\bin\cmake\win\x64\bin\cmake.exe'
$parallel_jobs = [Math]::Max(1, [Math]::Min([Environment]::ProcessorCount, 8))
if (-not $env:CCACHE_DIR)
{
    $env:CCACHE_DIR = Join-Path $env:LOCALAPPDATA 'TxCompiler\ccache'
}
if (-not $env:CCACHE_MAXSIZE)
{
    $env:CCACHE_MAXSIZE = '5G'
}

if (Test-Path -LiteralPath $bundled_cmake)
{
    $cmake_exe = $bundled_cmake
}
else
{
    $cmake_exe = (Get-Command cmake -ErrorAction Stop).Source
}

& $cmake_exe --log-level=WARNING -Wno-deprecated -S $project_root -B $build_dir -G Ninja `
    '-DCMAKE_C_COMPILER=gcc' '-DCMAKE_CXX_COMPILER=g++' '-DCMAKE_BUILD_TYPE=Release'
if ($LASTEXITCODE -ne 0)
{
    throw 'CMake 配置失败；build/ 已保留。'
}

& $cmake_exe --build $build_dir --parallel $parallel_jobs
if ($LASTEXITCODE -ne 0)
{
    throw '编译失败；build/ 已保留。'
}
if (Test-Path -LiteralPath (Join-Path $postgres_root 'pgsql/include/libpq-fe.h'))
{
    $postgres_root = Join-Path $postgres_root 'pgsql'
}

$clang_exe = $null
if ($env:TX_LLVM_BIN)
{
    $candidate = Join-Path $env:TX_LLVM_BIN 'clang.exe'
    if (Test-Path -LiteralPath $candidate -PathType Leaf)
    {
        $clang_exe = $candidate
    }
}
if (-not $clang_exe)
{
    $command = Get-Command clang.exe -ErrorAction SilentlyContinue
    if ($command)
    {
        $clang_exe = $command.Source
    }
}
if (-not $clang_exe)
{
    $candidate = 'C:\Program Files\LLVM\bin\clang.exe'
    if (Test-Path -LiteralPath $candidate -PathType Leaf)
    {
        $clang_exe = $candidate
    }
}
if (-not $clang_exe)
{
    $candidate = Join-Path $project_root '../.llvm-dev/extracted/LLVM/bin/clang.exe'
    if (Test-Path -LiteralPath $candidate -PathType Leaf)
    {
        $clang_exe = [System.IO.Path]::GetFullPath($candidate)
    }
}
if (-not $clang_exe)
{
    $candidate = Join-Path $tool_dir 'clang.exe'
    if (Test-Path -LiteralPath $candidate -PathType Leaf)
    {
        $clang_exe = $candidate
    }
}
if (-not $clang_exe)
{
    throw '构建时需要 LLVM clang.exe；请设置 TX_LLVM_BIN 或加入 PATH。build/ 已保留。'
}
$bundled_clang = Join-Path $tool_dir 'clang.exe'
if (-not [string]::Equals(
    [System.IO.Path]::GetFullPath($clang_exe),
    [System.IO.Path]::GetFullPath($bundled_clang),
    [System.StringComparison]::OrdinalIgnoreCase))
{
    Copy-Item -LiteralPath $clang_exe -Destination $bundled_clang -Force
}
Copy-Item -LiteralPath (Join-Path $project_root 'third_party/llvm/LICENSE.TXT') `
    -Destination (Join-Path $tool_dir 'LLVM-LICENSE.TXT') -Force

$gcc_exe = (Get-Command g++ -ErrorAction Stop).Source
$gcc_bin = Split-Path $gcc_exe
$ar_exe = Join-Path $gcc_bin 'ar.exe'
$link_dir = Join-Path $tool_dir 'link'
New-Item -ItemType Directory -Path $link_dir -Force | Out-Null
foreach ($name in @(
    'crt2.o', 'crtbegin.o', 'crtend.o',
    'libstdc++.dll.a', 'libmingw32.a', 'libgcc_s.a', 'libgcc.a',
    'libmoldname.a', 'libmingwex.a', 'libmsvcrt.a', 'libkernel32.a',
    'libpthread.a', 'libadvapi32.a', 'libbcrypt.a', 'libcrypt32.a', 'libncrypt.a',
    'libwinhttp.a', 'libws2_32.a', 'libdnsapi.a',
    'libshell32.a', 'libuser32.a',
    'libgdi32.a', 'libd2d1.a', 'libole32.a', 'libuuid.a', 'libcomctl32.a',
    'libiconv.a'))
{
    $source = (& $gcc_exe "-print-file-name=$name" | Select-Object -First 1).Trim()
    if (-not (Test-Path -LiteralPath $source -PathType Leaf))
    {
        throw "缺少链接依赖：$name；build/ 已保留。"
    }
    Copy-Item -LiteralPath $source -Destination (Join-Path $link_dir $name) -Force
}
$manifest_object = (& $gcc_exe '-print-file-name=default-manifest.o' | Select-Object -First 1).Trim()
if (Test-Path -LiteralPath $manifest_object -PathType Leaf)
{
    Copy-Item -LiteralPath $manifest_object -Destination (Join-Path $link_dir 'default-manifest.o') -Force
}
else
{
    # 上游 MinGW 没有 CLion 自带的默认 manifest；保留相同的权限和系统兼容声明。
    & (Join-Path $gcc_bin 'windres.exe') -I (Join-Path $project_root 'cmake') `
        -i (Join-Path $project_root 'cmake/application_manifest.rc') -O coff `
        -o (Join-Path $link_dir 'default-manifest.o')
    if ($LASTEXITCODE -ne 0)
    {
        throw '无法生成 Windows manifest；build/ 已保留。'
    }
}
Copy-Item -LiteralPath (Join-Path $gcc_bin 'ld.exe') `
    -Destination (Join-Path $link_dir 'ld.exe') -Force
& (Join-Path $gcc_bin 'windres.exe') -I (Join-Path $project_root 'cmake') `
    -i (Join-Path $project_root 'cmake/gui_manifest.rc') -O coff `
    -o (Join-Path $link_dir 'gui-manifest.o')
if ($LASTEXITCODE -ne 0)
{
    throw '无法生成 GUI DPI/Common Controls manifest；build/ 已保留。'
}
# CLion 的链接器附带 libssp；上游 MinGW 的链接器不依赖此 DLL。
$ssp_runtime = Join-Path $gcc_bin 'libssp-0.dll'
if (Test-Path -LiteralPath $ssp_runtime -PathType Leaf)
{
    Copy-Item -LiteralPath $ssp_runtime -Destination (Join-Path $link_dir 'libssp-0.dll') -Force
}
# libstdc++-6.dll 与 libwinpthread-1.dll 必须来自同一套 MinGW 运行时。
Copy-Item -LiteralPath (Join-Path $gcc_bin 'libwinpthread-1.dll') `
    -Destination (Join-Path $tool_dir 'libwinpthread-1.dll') -Force
Copy-Item -LiteralPath (Join-Path $build_dir '_deps/winpthread_runtime-src/mingw64/bin/libwinpthread-1.dll') `
    -Destination (Join-Path $tool_dir 'libwinpthread-u.dll') -Force
Copy-Item -LiteralPath (Join-Path $gcc_bin 'libstdc++-6.dll') `
    -Destination (Join-Path $tool_dir 'libstdc++-6.dll') -Force
Copy-Item -LiteralPath (Join-Path $build_dir '_deps/gcc_runtime-src/mingw64/bin/libgcc_s_seh-1.dll') `
    -Destination (Join-Path $tool_dir 'libgcc_s_seh-1.dll') -Force
Copy-Item -LiteralPath (Join-Path $build_dir '_deps/stdcpp_runtime-src/mingw64/bin/libstdc++-6.dll') `
    -Destination (Join-Path $tool_dir 'libstdc++-u.dll') -Force
Copy-Item -LiteralPath (Join-Path $build_dir '_deps/msquic_binary-src/build/native/bin/x64/msquic.dll') `
    -Destination (Join-Path $tool_dir 'msquic.dll') -Force

$compiler_path = Join-Path $tool_dir 'txc.exe'
$library_path = Join-Path $tool_dir 'libtxstdlib.a'
$interface_dir = Join-Path $tool_dir 'stdlib'
# 每轮从 CMake 的原始库开始；不能把上轮已合并依赖的发布库再次合并。
Copy-Item -LiteralPath (Join-Path $build_dir 'libtxstdlib.a') `
    -Destination $library_path -Force
foreach ($module in @('httpx_bridge', 'websocket_bridge', 'requests_bridge'))
{
    $source = Join-Path $project_root "src/stdlib/$module.tx"
    $ir = Join-Path $build_dir "$module.ll"
    $object = Join-Path $build_dir "$module.o"
    & $compiler_path emit-library-llvm $source -o $ir
    if ($LASTEXITCODE -ne 0)
    {
        throw "无法编译标准库 TX 模块：$module；build/ 已保留。"
    }
    & $bundled_clang -target x86_64-w64-windows-gnu -x ir -c -O3 -o $object $ir
    if ($LASTEXITCODE -ne 0)
    {
        throw "无法生成标准库 TX 对象文件：$module；build/ 已保留。"
    }
    & $ar_exe rcs $library_path $object
    if ($LASTEXITCODE -ne 0)
    {
        throw "无法归档标准库 TX 对象文件：$module；build/ 已保留。"
    }
}
$dependency_archives = @(
    (Join-Path $build_dir '_deps/nghttp2-build/lib/libnghttp2.a'),
    (Join-Path $build_dir '_deps/nghttp3-build/external/lib/libnghttp3.a'),
    (Join-Path $build_dir '_deps/mbedtls-build/library/libmbedtls.a'),
    (Join-Path $build_dir '_deps/mbedtls-build/library/libmbedx509.a'),
    (Join-Path $build_dir '_deps/mbedtls-build/library/libmbedcrypto.a'),
    (Join-Path $build_dir '_deps/mbedtls-build/3rdparty/everest/libeverest.a'),
    (Join-Path $build_dir '_deps/mbedtls-build/3rdparty/p256-m/libp256m.a'),
    (Join-Path $build_dir 'libargon2_reference_static.a'),
    (Join-Path $build_dir '_deps/libsodium_binary-src/mingw64/lib/libsodium.a'),
    (Join-Path $build_dir '_deps/icu_binary-src/mingw64/lib/libicuin.dll.a'),
    (Join-Path $build_dir '_deps/icu_binary-src/mingw64/lib/libicuuc.dll.a'),
    (Join-Path $build_dir '_deps/icu_binary-src/mingw64/lib/libicudt.dll.a'),
    (Join-Path $build_dir '_deps/pcre2_binary-src/mingw64/lib/libpcre2-8.a'),
    (Join-Path $build_dir '_deps/libxml2-build/libxml2.a'),
    (Join-Path $build_dir 'libtx_sqlite_static.a'),
    (Join-Path $build_dir 'libtx_libpq_import.a')
)
foreach ($archive in $dependency_archives)
{
    if (-not (Test-Path -LiteralPath $archive -PathType Leaf))
    {
        throw "缺少标准库静态依赖：$archive；build/ 已保留。"
    }
}
$merged_library = Join-Path $build_dir 'libtxstdlib-combined.a'
$mri_commands = @("CREATE `"$($merged_library.Replace('\', '/'))`"",
                  "ADDLIB `"$($library_path.Replace('\', '/'))`"")
foreach ($archive in $dependency_archives)
{
    $mri_commands += "ADDLIB `"$($archive.Replace('\', '/'))`""
}
$mri_commands += @('SAVE', 'END')
$mri_commands | & $ar_exe -M | Out-Null
if ($LASTEXITCODE -ne 0 -or
    -not (Test-Path -LiteralPath $merged_library -PathType Leaf))
{
    throw '无法归档 HTTP/2 静态依赖；build/ 已保留。'
}
Copy-Item -LiteralPath $merged_library -Destination $library_path -Force
$llvm_bin = Split-Path $clang_exe
foreach ($name in @('ld.lld.exe', 'llvm-ar.exe', 'llvm-nm.exe'))
{
    if (-not (Test-Path -LiteralPath (Join-Path $llvm_bin $name) -PathType Leaf))
    {
        throw "ThinLTO 需要与 clang 同版本的 $name；请设置 TX_LLVM_BIN。build/ 已保留。"
    }
}
$lto_arguments = @((Join-Path $project_root 'scripts/build_lto.py'),
    '--build', $build_dir, '--llvm-bin', $llvm_bin,
    '--mingw', (Split-Path $gcc_bin), '--output', (Join-Path $tool_dir 'libtxstdlib_lto.a'),
    '--jobs', $parallel_jobs)
foreach ($archive in $dependency_archives)
{
    $lto_arguments += @('--dependency', $archive)
}
& python @lto_arguments
if ($LASTEXITCODE -ne 0)
{
    throw 'ThinLTO 标准库构建失败；build/ 已保留。'
}
Copy-Item -LiteralPath (Join-Path $llvm_bin 'ld.lld.exe') `
    -Destination (Join-Path $link_dir 'ld.lld.exe') -Force
Copy-Item -LiteralPath (Join-Path $build_dir '_deps/nghttp2-src/COPYING') `
    -Destination (Join-Path $tool_dir 'NGHTTP2-LICENSE') -Force
Copy-Item -LiteralPath (Join-Path $build_dir '_deps/nghttp3-src/COPYING') `
    -Destination (Join-Path $tool_dir 'NGHTTP3-LICENSE') -Force
Copy-Item -LiteralPath (Join-Path $build_dir '_deps/msquic_binary-src/LICENSE') `
    -Destination (Join-Path $tool_dir 'MSQUIC-LICENSE') -Force
Copy-Item -LiteralPath (Join-Path $build_dir '_deps/mbedtls-src/LICENSE') `
    -Destination (Join-Path $tool_dir 'MBEDTLS-LICENSE') -Force
Copy-Item -LiteralPath (Join-Path $build_dir '_deps/argon2_reference-src/LICENSE') `
    -Destination (Join-Path $tool_dir 'ARGON2-LICENSE') -Force
Copy-Item -LiteralPath (Join-Path $build_dir '_deps/libsodium_binary-src/mingw64/share/licenses/libsodium/LICENSE') `
    -Destination (Join-Path $tool_dir 'LIBSODIUM-LICENSE') -Force
Copy-Item -LiteralPath (Join-Path $build_dir '_deps/icu_binary-src/mingw64/share/icu/78.3/LICENSE') `
    -Destination (Join-Path $tool_dir 'ICU-LICENSE') -Force
Copy-Item -LiteralPath (Join-Path $build_dir '_deps/pcre2_binary-src/mingw64/share/licenses/pcre2/LICENCE.md') `
    -Destination (Join-Path $tool_dir 'PCRE2-LICENCE.md') -Force
Copy-Item -LiteralPath (Join-Path $build_dir '_deps/libxml2-src/Copyright') `
    -Destination (Join-Path $tool_dir 'LIBXML2-LICENSE') -Force
Copy-Item -LiteralPath (Join-Path $project_root 'third_party/sqlite/LICENSE') `
    -Destination (Join-Path $tool_dir 'SQLITE-LICENSE') -Force
Copy-Item -LiteralPath (Join-Path $postgres_root 'server_license.txt') `
    -Destination (Join-Path $tool_dir 'POSTGRESQL-LICENSE') -Force
Copy-Item -LiteralPath (Join-Path $postgres_root 'commandlinetools_3rd_party_licenses.txt') `
    -Destination (Join-Path $tool_dir 'LIBPQ-THIRD-PARTY-LICENSES') -Force
foreach ($name in @('libpq.dll', 'libssl-3-x64.dll', 'libcrypto-3-x64.dll',
                    'libintl-9.dll', 'libiconv-2.dll'))
{
    Copy-Item -LiteralPath (Join-Path $postgres_root "bin/$name") `
        -Destination (Join-Path $tool_dir $name) -Force
}
Copy-Item -LiteralPath (Join-Path $postgres_root 'bin/libwinpthread-1.dll') `
    -Destination (Join-Path $tool_dir 'libwinpthread-p.dll') -Force
Copy-Item -LiteralPath (Join-Path $postgres_root 'pgAdmin 4/python/vcruntime140.dll') `
    -Destination (Join-Path $tool_dir 'vcruntime140.dll') -Force
Copy-Item -LiteralPath (Join-Path $build_dir '_deps/stdcpp_runtime-src/mingw64/share/licenses/libstdc++/COPYING.RUNTIME') `
    -Destination (Join-Path $tool_dir 'GCC-RUNTIME-LICENSE') -Force
Copy-Item -LiteralPath (Join-Path $build_dir '_deps/stdcpp_runtime-src/mingw64/share/licenses/libstdc++/COPYING3') `
    -Destination (Join-Path $tool_dir 'GCC-GPL-LICENSE') -Force
Copy-Item -LiteralPath (Join-Path $build_dir '_deps/winpthread_runtime-src/mingw64/share/licenses/libwinpthread/COPYING') `
    -Destination (Join-Path $tool_dir 'WINPTHREAD-LICENSE') -Force
foreach ($name in @('libicuin78.dll', 'libicuuc78.dll', 'libicudt78.dll'))
{
    Copy-Item -LiteralPath (Join-Path $build_dir "_deps/icu_binary-src/mingw64/bin/$name") `
        -Destination (Join-Path $tool_dir $name) -Force
}
# ICU 仅经 C 接口与 TX 运行时相连；私有导入名隔离另一套 C++ 和线程运行库。
$import_rewrites = @(
    @{ Name = 'libintl-9.dll'; From = 'libwinpthread-1.dll'; To = 'libwinpthread-p.dll' },
    @{ Name = 'libicuin78.dll'; From = 'libstdc++-6.dll'; To = 'libstdc++-u.dll' },
    @{ Name = 'libicuuc78.dll'; From = 'libstdc++-6.dll'; To = 'libstdc++-u.dll' },
    @{ Name = 'libicuuc78.dll'; From = 'libwinpthread-1.dll'; To = 'libwinpthread-u.dll' },
    @{ Name = 'libstdc++-u.dll'; From = 'libwinpthread-1.dll'; To = 'libwinpthread-u.dll' }
)
foreach ($rewrite in $import_rewrites)
{
    $old_import = [System.Text.Encoding]::ASCII.GetBytes($rewrite.From)
    $new_import = [System.Text.Encoding]::ASCII.GetBytes($rewrite.To)
    if ($old_import.Length -ne $new_import.Length)
    {
        throw "运行时导入名长度不一致：$($rewrite.Name)；build/ 已保留。"
    }
    $path = Join-Path $tool_dir $rewrite.Name
    $data = [System.IO.File]::ReadAllBytes($path)
    $matches = 0
    for ($index = 0; $index -le $data.Length - $old_import.Length; $index++)
    {
        if ($data[$index] -ne $old_import[0]) { continue }
        $equal = $true
        for ($offset = 1; $offset -lt $old_import.Length; $offset++)
        {
            if ($data[$index + $offset] -ne $old_import[$offset])
            {
                $equal = $false
                break
            }
        }
        if (-not $equal) { continue }
        for ($offset = 0; $offset -lt $new_import.Length; $offset++)
        {
            $data[$index + $offset] = $new_import[$offset]
        }
        $matches++
    }
    if ($matches -ne 1)
    {
        throw "运行时导入表与固定包不符：$($rewrite.Name)；build/ 已保留。"
    }
    [System.IO.File]::WriteAllBytes($path, $data)
}
if (-not (Test-Path -LiteralPath $compiler_path) -or
    -not (Test-Path -LiteralPath $library_path) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'string.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'format.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'io.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'file.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'bytes.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'crypto.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'public_key.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'tls.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'encoding.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'unicode.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'regex.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'file_stream.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'error.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'cancel.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'parse.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'json.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'csv.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'xml.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'math.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'algorithm.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'array.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'dictionary.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'fs.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'path.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'system.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'env.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'time.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'random.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'httpx.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'websocket.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'requests.txh')))
{
    throw '编译器产物或标准库接口不完整；build/ 已保留。'
}

$abi_fingerprint = (Get-Content -LiteralPath (Join-Path $build_dir 'compatibility_abi.txt') `
    -Encoding utf8 -Raw).Trim()
$compiler_hash = (Get-FileHash -LiteralPath $compiler_path -Algorithm SHA256).Hash.ToLowerInvariant()
$library_hash = (Get-FileHash -LiteralPath $library_path -Algorithm SHA256).Hash.ToLowerInvariant()
$lto_digest = (Get-FileHash -LiteralPath (Join-Path $tool_dir 'libtxstdlib_lto.a') -Algorithm SHA256).Hash.ToLowerInvariant()
$clang_digest = (Get-FileHash -LiteralPath $bundled_clang -Algorithm SHA256).Hash.ToLowerInvariant()
$lld_digest = (Get-FileHash -LiteralPath (Join-Path $link_dir 'ld.lld.exe') -Algorithm SHA256).Hash.ToLowerInvariant()
$compatibility_manifest = "tx-package-v3`nabi $abi_fingerprint`ntxc $compiler_hash`nstdlib $library_hash`nstdlib_lto $lto_digest`nclang $clang_digest`nlld $lld_digest`n"
foreach ($name in @('libgcc_s_seh-1.dll', 'libstdc++-6.dll',
                    'libwinpthread-1.dll', 'libstdc++-u.dll',
                    'libwinpthread-u.dll',
                    'libicuin78.dll', 'libicuuc78.dll', 'libicudt78.dll',
                    'msquic.dll', 'libpq.dll', 'libssl-3-x64.dll',
                    'libcrypto-3-x64.dll', 'libintl-9.dll', 'libiconv-2.dll',
                    'libwinpthread-p.dll', 'vcruntime140.dll'))
{
    $digest = (Get-FileHash -LiteralPath (Join-Path $tool_dir $name) -Algorithm SHA256).Hash.ToLowerInvariant()
    $compatibility_manifest += "$name $digest`n"
}
[System.IO.File]::WriteAllText((Join-Path $tool_dir 'package.compat'),
    $compatibility_manifest, [System.Text.UTF8Encoding]::new($false))

# 示例及中文文档随工具链交付，安装后无需访问 src/ 或 tests/。
Copy-Item -LiteralPath (Join-Path $project_root 'examples') -Destination $tool_dir -Recurse -Force
Copy-Item -LiteralPath (Join-Path $project_root 'docs') -Destination $tool_dir -Recurse -Force

$project_item = Get-Item -LiteralPath $project_root
$build_item = Get-Item -LiteralPath $build_dir
$resolved_root = [System.IO.Path]::GetFullPath($project_item.FullName)
$resolved_build = [System.IO.Path]::GetFullPath($build_item.FullName)
$expected_build = [System.IO.Path]::GetFullPath((Join-Path $resolved_root 'build'))
$root_prefix = $resolved_root.TrimEnd('\', '/') + [System.IO.Path]::DirectorySeparatorChar
if (($build_item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0 -or
    -not [string]::Equals($resolved_build, $expected_build,
        [System.StringComparison]::OrdinalIgnoreCase) -or
    -not $resolved_build.StartsWith($root_prefix,
        [System.StringComparison]::OrdinalIgnoreCase))
{
    throw '拒绝清理预期工作区以外的路径。'
}

if (-not $Incremental)
{
    Remove-Item -LiteralPath $resolved_build -Recurse -Force
}
Write-Output "编译器已生成：$compiler_path"
Write-Output "标准库已生成：$library_path"
Write-Output "标准库接口：$interface_dir"
if ($Incremental)
{
    Write-Output "增量构建目录已保留：$resolved_build"
}
