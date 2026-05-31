#pragma once
#include <cstdint>

/**
 * @brief Compile-time description of a linear sensor's pixel format.
 *
 * All members are static and constexpr: Sensor is a pure compile-time trait,
 * not an object meant to be instantiated.
 *
 * @tparam BitDepth Number of bits per pixel (e.g. 12 for a 12-bit sensor).
 */
template <int BitDepth>
class Sensor
{
   public:
    /**
     * @brief Bits per pixel.
     */
    static constexpr int NUM_BITS = BitDepth;

    /**
     * @brief Number of discrete levels a pixel can assume
     */
    static constexpr int NUM_LEVELS = 1 << BitDepth;

    /**
     * @brief Saturation value
     */
    static constexpr uint16_t MAX_VAL = static_cast<uint16_t>(NUM_LEVELS - 1);

    /**
     * @brief Saturation value as float, for floating-point scaling/clamping.
     */
    static constexpr float MAX_VAL_F = static_cast<float>(NUM_LEVELS - 1);

    /**
     * @brief Mid-range value (e.g. centre of the HDR weighting curve).
     */
    static constexpr float MID = (NUM_LEVELS - 1) / 2.0f;

    Sensor() = delete;
};

/**
 * @brief Alias to a 12bit sensor
 */
using Sensor12 = Sensor<12>;
