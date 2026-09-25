# TX 标准库与 C++ Release 对照

`compare.tx` 覆盖 `tx/stdlib/` 中公开的 9 个模块，并额外测量核心字典操作。`compare.cpp` 使用同样的输入和循环次数；数组采用 `std::vector<std::any>`，字典同时提供动态线性表和常见的 `std::unordered_map` 两种 C++ 对照。每项输出三行：名称、程序内部耗时（毫秒）、校验值。随机数使用同一种 `mt19937_64` 引擎和种子，结果可直接核对。

从仓库根目录运行。TX 编译器和运行时库先由 `scripts/build.ps1` 构建；C++ 对照通过 CMake 的 `Release` 配置构建。GCC 13.1.0 下该配置使用 `-O3 -DNDEBUG`，当前 TX 目标程序由 `clang -O3` 生成。下表是此前 `clang -O2` 下记录的历史结果，尚未按当前构建重新测量。

```powershell
cmake -S benchmarks/library_compare -B tx_build/library_compare_cmake -G Ninja '-DCMAKE_CXX_COMPILER=g++' '-DCMAKE_BUILD_TYPE=Release'
cmake --build tx_build/library_compare_cmake --config Release
.\tx\txc.exe .\benchmarks\library_compare\compare.tx -o .\tx_build\library_compare_tx.exe
.\tx_build\library_compare_tx.exe 2>$null
.\tx_build\library_compare_cmake\library_compare_cpp.exe 2>$null
```

两程序须从仓库根目录运行，以访问 `tx/stdlib/` 并在 `tx_build/` 写入各自的临时文本文件。`io` 项向标准错误写入 5000 个 `x`，上面的命令将其丢弃。运行后可删除 `tx_build/library_compare_tx.txt`、`tx_build/library_compare_cpp.txt` 和生成的程序及 CMake 构建目录。

| 项目 | 工作负载 |
| --- | --- |
| math | 100 万次平方根求和 |
| random | 同种子生成 20 万个区间整数并求和 |
| string | 1 万次替换并分割文本 |
| array | 3000 次拼接、反转、切片并读取元素 |
| dict | 128 个整数键，重复 100 轮更新和读取 |
| path | 1 万次路径拼接与扩展名提取 |
| fs | 200 次列目录并检查文件类型 |
| file | 300 次写入和读取 64 字节 UTF-8 文本 |
| io | 5000 次单字符标准错误输出 |
| time | 10 万次获取 Unix 毫秒时间 |

2026-09-25 在本机预热后测 7 轮，各项使用程序内部计时的中位数。“优化前”是在整数内联检查完成后、标准库直接调用和字典索引优化前测得；“当前”还包括随机数状态复用与错误路径改动。C++ 对照始终以 Release 构建，每轮先后顺序交替。

| 项目 | TX 优化前 (ms) | TX 当前 (ms) | C++ Release (ms) | 当前 TX / C++ |
| --- | ---: | ---: | ---: | ---: |
| math | 120 | 3 | 2.141 | 约 1.4 倍 |
| random | 37 | 1 | 0.338 | 约 3 倍 |
| string | 18 | 8 | 2.533 | 约 3.2 倍 |
| array | 40 | 23 | 10.780 | 约 2.1 倍 |
| dict，对照动态线性表 | 38 | 3 | 14.933 | 0.20 倍 |
| dict，对照哈希表 | 38 | 3 | 0.0663 | 约 45 倍 |
| path | 23 | 16 | 8.926 | 约 1.8 倍 |
| fs | 27 | 29 | 25.820 | 约 1.1 倍 |
| file | 1231 | 1248 | 1242.280 | 约 1 倍 |
| io | 8 | 7 | 6.707 | 约 1 倍 |
| time | 9 | 4 | 3.302 | 约 1.2 倍 |

所有相应校验值一致；浮点求和按误差小于 0.001 核对。TX 计时精度为 1 ms，短项目的比例只宜粗略看待。此前只完成直接 ABI 时，math 与 random 两项分别为 12 ms 和 6 ms；去掉成功路径的线程局部错误缓冲区访问并复用随机数状态后，分别为 3 ms 和 1 ms。文件和目录项目受 Windows 文件系统缓存、杀毒扫描及输出重定向影响，不能代表纯计算开销。当前 TX 字典为保留插入顺序的动态值容器并维护哈希索引；两种 C++ 字典分别是动态线性表和仅有整数键值的 `std::unordered_map`，数据表示与语义不同，因此这两行展示算法与表示的影响，不能直接当作语言开销。

可单独复现较长的随机数负载，两个程序应输出相同的校验和 `2500067985`：

```powershell
.\tx\txc.exe .\benchmarks\library_compare\random_profile.tx -o .\tx_build\random_profile_tx.exe
.\tx_build\random_profile_tx.exe
.\tx_build\library_compare_cmake\library_compare_cpp.exe random-long
```

瓶颈定位时，500 万次调用的 C++ Release 中位数依次为：循环内直接使用生成器 8.46 ms；把生成器引用传给不内联函数 12.72 ms；不内联函数每次自行获取线程局部状态 121.76 ms；直接调用旧运行时函数 120.31 ms；经旧直接 C ABI 调用 127.06 ms。TX 旧路径同为 127 ms。当前 MinGW 构建中 `random.cpp.obj` 引用 `__emutls_get_address`。因此运行时每次获取线程局部状态是主因；现在生成函数只在首次实际使用时获取一次，并在该次函数调用内复用。

上面的独立长负载在修复后预热并交替测 7 轮，TX 为 38 ms，C++ Release 为 9.03 ms，当前约差 4.2 倍；校验和同为 `2500067985`。此前的 127 ms 与修复后 43 ms 来自另一份同时测平方根与随机数的组合程序，二者使用同一份源码，可用于观察改动前后；独立长负载提供当前的可复现对照。

## 2026-09-25 数组优化后单次热态复测

重新构建 TX 编译器及运行时库，并编译 TX 与 GCC 13.1.0 Release 对照程序。两程序先各运行一次预热，然后按 C++、TX 顺序各测一次；下表取这一次程序内部计时，不是上节的 7 轮中位数。相应整数校验值一致，浮点求和结果在误差范围内。TX 使用 `clang -O3`，其计时精度为 1 ms。

| 项目 | TX (ms) | C++ Release (ms) | TX / C++ |
| --- | ---: | ---: | ---: |
| math | 34 | 2.1020 | 16.18 |
| random | 8 | 0.3312 | 24.15 |
| string | 13 | 2.4615 | 5.28 |
| array | 17 | 11.8875 | 1.43 |
| dict，对照动态线性表 | 5 | 16.9263 | 0.30 |
| dict，对照哈希表 | 5 | 0.0638 | 78.37 |
| path | 22 | 8.2984 | 2.65 |
| fs | 28 | 24.3313 | 1.15 |
| file | 1253 | 1245.4210 | 1.01 |
| io | 8 | 6.1687 | 1.30 |
| time | 5 | 3.2571 | 1.54 |

数组组合用例本轮为 TX 17 ms、C++ 11.8875 ms，约 1.43 倍；旧记录为 TX 23 ms、C++ 10.780 ms。由于本轮只取一次热态样本，这只能说明当前观察值，不能代替稳定的前后对照。`math` 与 `random` 的 TX 时间分别为 34 ms、8 ms，高于上节历史记录的 3 ms、1 ms；当前生成的 `math` 循环每轮含数学 C ABI 调用与 GC 安全点调用，具体差异仍需单独定位。
