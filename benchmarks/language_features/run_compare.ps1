param(
    [ValidateRange(1, 31)]
    [int]$Rounds = 7
)

$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)
$OutputEncoding = [System.Text.UTF8Encoding]::new($false)
$culture = [System.Globalization.CultureInfo]::InvariantCulture

$project_root = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$cmake_dir = Join-Path $project_root 'tx_build/language_features_cmake'
$tx_exe = Join-Path $project_root 'tx_build/language_features_tx.exe'
$cpp_exe = Join-Path $cmake_dir 'language_features_cpp.exe'
$txc_exe = Join-Path $project_root 'tx/txc.exe'
$bundled_cmake = 'D:\CLion 2024.3.4\bin\cmake\win\x64\bin\cmake.exe'

if (Test-Path -LiteralPath $bundled_cmake -PathType Leaf)
{
    $cmake_exe = $bundled_cmake
}
else
{
    $cmake_exe = (Get-Command cmake -ErrorAction Stop).Source
}

$expected = [ordered]@{
    scalar_control = -62500000000L
    updates = 391096443253L
    while_logic = -62500000000L
    float_arithmetic = 62500125000L
    overloads = 4838081397L
    named_arguments = 18731467920L
    recursion = 3420000L
    variadic_unpack = 50095000L
    array_destructure = 400060000L
    array_padded = 1250225000L
    dict_iteration = 5000350000L
    struct_operators = 5000350000L
    class_methods = 2726759900400L
    virtual_interface = 3200000L
    class_operator = 1000000L
    runtime_cast = 800000L
    module_call = 15000950000L
    string_conversion = 1250413894L
    deep_copy = 50005001L
    copy_cycle = 50005007L
    deinit = 20000L
    cycle_gc = 1000L
}
$names = @($expected.Keys)

function Read-BenchmarkResult
{
    param(
        [string]$Executable,
        [string]$Label,
        [double]$MillisecondsPerUnit = 1.0
    )

    [string[]]$lines = & $Executable
    if ($LASTEXITCODE -ne 0)
    {
        throw "$Label 运行失败，退出码 $LASTEXITCODE。"
    }
    if ($lines.Count -ne $names.Count * 3)
    {
        throw "$Label 输出行数不正确：$($lines.Count)。"
    }

    $result = [ordered]@{}
    for ($index = 0; $index -lt $names.Count; ++$index)
    {
        $line = $index * 3
        $name = $lines[$line].Trim()
        if ($name -ne $names[$index])
        {
            throw "$Label 第 $($index + 1) 项名称不匹配：$name。"
        }
        $elapsed = [double]::Parse($lines[$line + 1], $culture) * $MillisecondsPerUnit
        $checksum = [long]::Parse($lines[$line + 2], $culture)
        if ($checksum -ne $expected[$name])
        {
            throw "$Label 的 $name 校验失败：得到 $checksum，预期 $($expected[$name])。"
        }
        $result[$name] = $elapsed
    }
    return $result
}

function Get-Median
{
    param([System.Collections.Generic.List[double]]$Values)

    [double[]]$sorted = @($Values | Sort-Object)
    $middle = [int][Math]::Floor($sorted.Count / 2)
    if ($sorted.Count % 2 -eq 1)
    {
        return $sorted[$middle]
    }
    return ($sorted[$middle - 1] + $sorted[$middle]) / 2.0
}

Push-Location $project_root
try
{
    & $cmake_exe -S benchmarks/language_features -B $cmake_dir -G Ninja `
        '-DCMAKE_CXX_COMPILER=g++' '-DCMAKE_BUILD_TYPE=Release'
    if ($LASTEXITCODE -ne 0)
    {
        throw 'C++ Release 配置失败。'
    }
    & $cmake_exe --build $cmake_dir --config Release
    if ($LASTEXITCODE -ne 0)
    {
        throw 'C++ Release 构建失败。'
    }
    & $txc_exe benchmarks/language_features/compare.tx -o $tx_exe
    if ($LASTEXITCODE -ne 0)
    {
        throw 'TX 基准编译失败。'
    }

    $null = Read-BenchmarkResult -Executable $tx_exe -Label 'TX 预热' `
        -MillisecondsPerUnit 0.001
    $null = Read-BenchmarkResult -Executable $cpp_exe -Label 'C++ 预热'

    $tx_samples = @{}
    $cpp_samples = @{}
    foreach ($name in $names)
    {
        $tx_samples[$name] = [System.Collections.Generic.List[double]]::new()
        $cpp_samples[$name] = [System.Collections.Generic.List[double]]::new()
    }

    for ($round = 0; $round -lt $Rounds; ++$round)
    {
        if ($round % 2 -eq 0)
        {
            $tx_result = Read-BenchmarkResult -Executable $tx_exe -Label 'TX' `
                -MillisecondsPerUnit 0.001
            $cpp_result = Read-BenchmarkResult -Executable $cpp_exe -Label 'C++'
        }
        else
        {
            $cpp_result = Read-BenchmarkResult -Executable $cpp_exe -Label 'C++'
            $tx_result = Read-BenchmarkResult -Executable $tx_exe -Label 'TX' `
                -MillisecondsPerUnit 0.001
        }
        foreach ($name in $names)
        {
            $tx_samples[$name].Add($tx_result[$name])
            $cpp_samples[$name].Add($cpp_result[$name])
        }
    }

    '{0,-22} {1,10} {2,12} {3,10}' -f '项目', 'TX(ms)', 'C++(ms)', 'TX/C++'
    foreach ($name in $names)
    {
        $tx_median = Get-Median $tx_samples[$name]
        $cpp_median = Get-Median $cpp_samples[$name]
        $ratio = if ($cpp_median -gt 0) { $tx_median / $cpp_median } else { [double]::NaN }
        '{0,-22} {1,10} {2,12} {3,10}' -f $name,
            $tx_median.ToString('F2', $culture),
            $cpp_median.ToString('F4', $culture),
            $ratio.ToString('F2', $culture)
    }
    '短项目仍受系统调度影响；TX 和 C++ 均已换算为毫秒。'
}
finally
{
    Pop-Location
}
