#pragma once
#include <cstdint>

/**
 * @brief Supported camera exposure times.
 *
 * @note The enumerator value IS the exposure duration in milliseconds, so that
 * exposure_ms() reduces to a plain cast.
 */
enum class ExposureTime : uint16_t {
    Short = 5,    ///< 5 ms exposure.
    Medium = 40,  ///< 40 ms exposure.
    Long = 320,   ///< 320 ms exposure.
};

constexpr float exposure_ms(ExposureTime e)
/**
 * @brief Returns the exposure duration in milliseconds.
 *
 * @param e The exposure setting.
 *
 * @return Exposure duration in ms, as a float.
 */
{
    return static_cast<float>(static_cast<uint16_t>(e));
}
