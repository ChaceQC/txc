# GitHub runner 使用固定 MinGW ABI；不依赖预装的 GCC/LLVM 版本。
$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)
$OutputEncoding = [Console]::OutputEncoding
if (-not $env:GITHUB_ENV -or -not $env:RUNNER_TEMP)
{
    throw '此脚本只用于 GitHub Actions。'
}
$tools_dir = Join-Path $env:RUNNER_TEMP 'txc-toolchains'
New-Item -ItemType Directory -Path $tools_dir -Force | Out-Null

function get_verified_archive
{
    param([string]$url, [string]$name, [string]$sha256)
    $archive = Join-Path $env:RUNNER_TEMP $name
    & curl.exe --fail --location --retry 3 --silent --show-error $url --output $archive
    if ($LASTEXITCODE -ne 0)
    {
        throw "下载失败：$name"
    }
    if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash -ne $sha256)
    {
        throw "下载校验失败：$name"
    }
    return $archive
}

$mingw_bin = Join-Path $tools_dir 'mingw64/bin'
if (-not (Test-Path -LiteralPath (Join-Path $mingw_bin 'g++.exe')))
{
    $archive = get_verified_archive `
        'https://github.com/niXman/mingw-builds-binaries/releases/download/13.1.0-rt_v11-rev1/x86_64-13.1.0-release-posix-seh-msvcrt-rt_v11-rev1.7z' `
        'txc-mingw.7z' '6b41fdf246756c04d2ac413d8835e347ffc18fdeda265f5310ac6aeb3c1dbbcf'
    & 7z x $archive "-o$tools_dir" -y -bso0 -bsp0
    if ($LASTEXITCODE -ne 0)
    {
        throw '无法解压 MinGW。'
    }
}

$llvm_dir = Join-Path $tools_dir 'clang+llvm-23.1.2-x86_64-pc-windows-msvc'
$llvm_bin = Join-Path $llvm_dir 'bin'
if (-not (Test-Path -LiteralPath (Join-Path $llvm_bin 'llvm-nm.exe')))
{
    $archive = get_verified_archive `
        'https://github.com/llvm/llvm-project/releases/download/llvmorg-23.1.2/clang%2Bllvm-23.1.2-x86_64-pc-windows-msvc.tar.xz' `
        'txc-llvm.tar.xz' '8fb91cdc44fcbbdcf6b3ffd0a1f9859abd14a3c3aae4423c2b6d4a4f90bf0095'
    Push-Location $tools_dir
    try
    {
        & cmake -E tar xf $archive
        if ($LASTEXITCODE -ne 0)
        {
            throw '无法解压 LLVM。'
        }
    }
    finally
    {
        Pop-Location
    }
}

foreach ($name in @('clang.exe', 'ld.lld.exe', 'llvm-ar.exe', 'llvm-nm.exe'))
{
    if (-not (Test-Path -LiteralPath (Join-Path $llvm_bin $name)))
    {
        throw "固定 LLVM 归档缺少 $name。"
    }
}
& (Join-Path $mingw_bin 'g++.exe') --version
if ($LASTEXITCODE -ne 0)
{
    throw 'GCC 无法运行。'
}
& (Join-Path $llvm_bin 'clang.exe') --version
if ($LASTEXITCODE -ne 0)
{
    throw 'Clang 无法运行。'
}
[System.IO.File]::AppendAllText($env:GITHUB_PATH, "$mingw_bin`n", [System.Text.UTF8Encoding]::new($false))
[System.IO.File]::AppendAllText($env:GITHUB_ENV, "TX_LLVM_BIN=$llvm_bin`n", [System.Text.UTF8Encoding]::new($false))
