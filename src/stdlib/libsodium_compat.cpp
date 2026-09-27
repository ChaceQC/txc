#include <cstddef>

#ifdef _WIN32
// 固定的 MinGW libsodium 静态包使用 C23 memset_explicit，当前链接 CRT
// 未导出该符号。经 volatile 写入确保擦除不会被优化掉。
extern "C" void* memset_explicit(void* destination, int value,
                                  std::size_t count) noexcept
{
    auto* bytes = static_cast<volatile unsigned char*>(destination);
    for (std::size_t index = 0; index < count; ++index)
    {
        bytes[index] = static_cast<unsigned char>(value);
    }
    return destination;
}
#endif
