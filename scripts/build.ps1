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

& $cmake_exe -S $project_root -B $build_dir -G Ninja '-DCMAKE_CXX_COMPILER=g++'
if ($LASTEXITCODE -ne 0)
{
    throw 'CMake 配置失败；build/ 已保留。'
}

& $cmake_exe --build $build_dir
if ($LASTEXITCODE -ne 0)
{
    throw '编译失败；build/ 已保留。'
}

$compiler_path = Join-Path $tool_dir 'txc.exe'
$library_path = Join-Path $tool_dir 'libtxstdlib.a'
$interface_dir = Join-Path $tool_dir 'stdlib'
if (-not (Test-Path -LiteralPath $compiler_path) -or
    -not (Test-Path -LiteralPath $library_path) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'string.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'io.txh')) -or
    -not (Test-Path -LiteralPath (Join-Path $interface_dir 'file.txh')))
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
