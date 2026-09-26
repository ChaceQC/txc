#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tx_generated::decimal_math
{

class natural
{
public:
    natural();
    explicit natural(std::string_view text);

    [[nodiscard]] bool is_zero() const noexcept;
    [[nodiscard]] bool is_odd() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] std::string text() const;
    void multiply_pow10(std::size_t places);

    [[nodiscard]] static int compare(const natural& left,
                                     const natural& right) noexcept;
    [[nodiscard]] static natural add(const natural& left, const natural& right);
    [[nodiscard]] static natural subtract(const natural& left,
                                          const natural& right);
    [[nodiscard]] static natural multiply(const natural& left,
                                          const natural& right);
    [[nodiscard]] static std::pair<natural, natural> divide(
        const natural& numerator, const natural& denominator);

private:
    void normalize() noexcept;
    void shift_digit(std::uint8_t digit);

    std::vector<std::uint8_t> digits_;
};

} // namespace tx_generated::decimal_math
