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
$python_exe = (Get-Command python -ErrorAction Stop).Source
$java_exe = (Get-Command java -ErrorAction Stop).Source
$javac_exe = (Get-Command javac -ErrorAction Stop).Source
$python_source = Join-Path $PSScriptRoot 'compare_py.py'
$java_dir = Join-Path $project_root 'tx_build/language_features_java'
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
        [string[]]$ProgramArguments = @(),
        [double]$MillisecondsPerUnit = 1.0
    )

    [string[]]$lines = & $Executable @ProgramArguments
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

    New-Item -ItemType Directory -Path $java_dir -Force | Out-Null
    $java_sources = @('compare_java.java', 'feature_core.java',
        'feature_containers.java', 'feature_objects.java',
        'feature_memory.java', 'feature_workload.java') |
        ForEach-Object { Join-Path $PSScriptRoot $_ }
    & $javac_exe -encoding UTF-8 -d $java_dir @java_sources
    if ($LASTEXITCODE -ne 0)
    {
        throw 'Java 基准编译失败。'
    }

    $null = Read-BenchmarkResult -Executable $tx_exe -Label 'TX 预热' `
        -MillisecondsPerUnit 0.001
    $null = Read-BenchmarkResult -Executable $cpp_exe -Label 'C++ 预热'
    $null = Read-BenchmarkResult -Executable $python_exe -Label 'Python 预热' `
        -ProgramArguments @('-X', 'utf8', '-B', $python_source)
    $null = Read-BenchmarkResult -Executable $java_exe -Label 'Java 预热' `
        -ProgramArguments @('-cp', $java_dir, 'compare_java')

    $tx_samples = @{}
    $cpp_samples = @{}
    $python_samples = @{}
    $java_samples = @{}
    foreach ($name in $names)
    {
        $tx_samples[$name] = [System.Collections.Generic.List[double]]::new()
        $cpp_samples[$name] = [System.Collections.Generic.List[double]]::new()
        $python_samples[$name] = [System.Collections.Generic.List[double]]::new()
        $java_samples[$name] = [System.Collections.Generic.List[double]]::new()
    }

    for ($round = 0; $round -lt $Rounds; ++$round)
    {
        if ($round % 2 -eq 0)
        {
            $tx_result = Read-BenchmarkResult -Executable $tx_exe -Label 'TX' `
                -MillisecondsPerUnit 0.001
            $cpp_result = Read-BenchmarkResult -Executable $cpp_exe -Label 'C++'
            $python_result = Read-BenchmarkResult -Executable $python_exe `
                -Label 'Python' -ProgramArguments @('-X', 'utf8', '-B', $python_source)
            $java_result = Read-BenchmarkResult -Executable $java_exe `
                -Label 'Java' -ProgramArguments @('-cp', $java_dir, 'compare_java')
        }
        else
        {
            $java_result = Read-BenchmarkResult -Executable $java_exe `
                -Label 'Java' -ProgramArguments @('-cp', $java_dir, 'compare_java')
            $python_result = Read-BenchmarkResult -Executable $python_exe `
                -Label 'Python' -ProgramArguments @('-X', 'utf8', '-B', $python_source)
            $cpp_result = Read-BenchmarkResult -Executable $cpp_exe -Label 'C++'
            $tx_result = Read-BenchmarkResult -Executable $tx_exe -Label 'TX' `
                -MillisecondsPerUnit 0.001
        }
        foreach ($name in $names)
        {
            $tx_samples[$name].Add($tx_result[$name])
            $cpp_samples[$name].Add($cpp_result[$name])
            $python_samples[$name].Add($python_result[$name])
            $java_samples[$name].Add($java_result[$name])
        }
    }

    '{0,-22} {1,10} {2,12} {3,12} {4,12} {5,10} {6,10} {7,10}' -f `
        '项目', 'TX(ms)', 'C++(ms)', 'Python(ms)', 'Java(ms)',
        'TX/C++', 'TX/Py', 'TX/Java'
    foreach ($name in $names)
    {
        $tx_median = Get-Median $tx_samples[$name]
        $cpp_median = Get-Median $cpp_samples[$name]
        $python_median = Get-Median $python_samples[$name]
        $java_median = Get-Median $java_samples[$name]
        $cpp_ratio = if ($cpp_median -gt 0) { $tx_median / $cpp_median } else { [double]::NaN }
        $python_ratio = if ($python_median -gt 0) { $tx_median / $python_median } else { [double]::NaN }
        $java_ratio = if ($java_median -gt 0) { $tx_median / $java_median } else { [double]::NaN }
        '{0,-22} {1,10} {2,12} {3,12} {4,12} {5,10} {6,10} {7,10}' -f $name,
            $tx_median.ToString('F2', $culture),
            $cpp_median.ToString('F4', $culture),
            $python_median.ToString('F3', $culture),
            $java_median.ToString('F3', $culture),
            $cpp_ratio.ToString('F2', $culture),
            $python_ratio.ToString('F2', $culture),
            $java_ratio.ToString('F2', $culture)
    }
    '短项目仍受系统调度和 Java JIT 影响；四语言均已换算为毫秒。'
}
finally
{
    Pop-Location
}
