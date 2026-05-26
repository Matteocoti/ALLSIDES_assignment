#pragma once
#include <cstdint>

enum class ExposureTime : uint16_t {
    Short = 5,
    Medium = 40,
    Long = 320,
};

constexpr float exposure_ms(ExposureTime e)
{
    return static_cast<float>(static_cast<uint16_t>(e));
}
