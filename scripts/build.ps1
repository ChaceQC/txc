$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)
$OutputEncoding = [System.Text.UTF8Encoding]::new($false)

$project_root = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$build_dir = [System.IO.Path]::GetFullPath((Join-Path $project_root 'build'))
$tool_dir = Join-Path $project_root 'tx'
$bundled_cmake = 'D:\CLion 2024.3.4\bin\cmake\win\x64\bin\cmake.exe'

if (Test-Path -LiteralPath $bundled_cmake)
{
    $cmake_exe = $bundled_cmake
}
else
{
    $cmake_exe = (Get-Command cmake -ErrorAction Stop).Source
}

& $cmake_exe --log-level=WARNING -Wno-deprecated -S $project_root -B $build_dir -G Ninja `
    '-DCMAKE_CXX_COMPILER=g++' '-DCMAKE_BUILD_TYPE=Release'
if ($LASTEXITCODE -ne 0)
{
    throw 'CMake 配置失败；build/ 已保留。'
}

& $cmake_exe --build $build_dir
if ($LASTEXITCODE -ne 0)
{
    throw '编译失败；build/ 已保留。'
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
    'crt2.o', 'crtbegin.o', 'crtend.o', 'default-manifest.o',
    'libstdc++.dll.a', 'libmingw32.a', 'libgcc_s.a', 'libgcc.a',
    'libmoldname.a', 'libmingwex.a', 'libmsvcrt.a', 'libkernel32.a',
    'libpthread.a', 'libadvapi32.a', 'libbcrypt.a', 'libwinhttp.a', 'libws2_32.a',
    'libshell32.a', 'libuser32.a',
    'libiconv.a'))
{
    $source = (& $gcc_exe "-print-file-name=$name" | Select-Object -First 1).Trim()
    if (-not (Test-Path -LiteralPath $source -PathType Leaf))
    {
        throw "缺少链接依赖：$name；build/ 已保留。"
    }
    Copy-Item -LiteralPath $source -Destination (Join-Path $link_dir $name) -Force
}
foreach ($name in @('ld.exe', 'libssp-0.dll'))
{
    Copy-Item -LiteralPath (Join-Path $gcc_bin $name) `
        -Destination (Join-Path $link_dir $name) -Force
}
foreach ($name in @('libgcc_s_seh-1.dll', 'libstdc++-6.dll',
                    'libwinpthread-1.dll'))
{
    Copy-Item -LiteralPath (Join-Path $gcc_bin $name) `
        -Destination (Join-Path $tool_dir $name) -Force
}

$compiler_path = Join-Path $tool_dir 'txc.exe'
$library_path = Join-Path $tool_dir 'libtxstdlib.a'
$interface_dir = Join-Path $tool_dir 'stdlib'
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
    (Join-Path $build_dir '_deps/mbedtls-build/library/libmbedtls.a'),
    (Join-Path $build_dir '_deps/mbedtls-build/library/libmbedx509.a'),
    (Join-Path $build_dir '_deps/mbedtls-build/library/libmbedcrypto.a'),
    (Join-Path $build_dir '_deps/mbedtls-build/3rdparty/everest/libeverest.a'),
    (Join-Path $build_dir '_deps/mbedtls-build/3rdparty/p256-m/libp256m.a')
)
foreach ($archive in $dependency_archives)
{
    if (-not (Test-Path -LiteralPath $archive -PathType Leaf))
    {
        throw "缺少 HTTP/2 静态依赖：$archive；build/ 已保留。"
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
Copy-Item -LiteralPath (Join-Path $build_dir '_deps/nghttp2-src/COPYING') `
    -Destination (Join-Path $tool_dir 'NGHTTP2-LICENSE') -Force
Copy-Item -LiteralPath (Join-Path $build_dir '_deps/mbedtls-src/LICENSE') `
    -Destination (Join-Path $tool_dir 'MBEDTLS-LICENSE') -Force
if (-not (Test-Path -LiteralPath $compiler_path) -or
    -not (Test-Path -LiteralPath $library_path) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'string.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'format.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'io.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'file.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'bytes.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'crypto.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'encoding.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'file_stream.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'error.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'parse.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'json.txh')) -or
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

Remove-Item -LiteralPath $resolved_build -Recurse -Force
Write-Output "编译器已生成：$compiler_path"
Write-Output "标准库已生成：$library_path"
Write-Output "标准库接口：$interface_dir"
