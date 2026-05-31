#pragma once
#include <cmath>
#include <cstdint>
#include "HdrCombiner.hpp"

/**
 * @brief Tone-mapped output frame: an 8-bit, displayable image.
 */
template <int W, int H>
using ToneMapFrame = BaseFrame<uint8_t, W, H>;

/**
 * @brief Global Reinhard operator: compresses an HDR radiance map to 8-bit.
 *
 * Uses the log-average luminance to set the exposure key, then applies the
 * Reinhard curve L / (1 + L). Global (single curve for the whole image), so it
 * is cheap and order-independent, at the cost of less local contrast than the
 * full local Reinhard operator.
 *
 * @tparam W Image width in pixels.
 * @tparam H Image height in pixels.
 */
template <int W, int H>
class ReinhardToneMapper
{
   private:
    uint16_t scale_;  ///< Output white level (e.g. 255 for an 8-bit image).

   public:
    /**
     * @brief Constructs the mapper.
     *
     * @param scale Value mapped to full white in the output.
     */
    ReinhardToneMapper(uint16_t scale);

    /**
     * @brief Tone-maps an HDR frame to an 8-bit frame.
     *
     * @param frame The HDR radiance map.
     *
     * @return The tone-mapped 8-bit frame.
     */
    ToneMapFrame<W, H> map(const HdrFrame<W, H> &frame) const;
};

template <int W, int H>
ReinhardToneMapper<W, H>::ReinhardToneMapper(uint16_t scale) : scale_(scale)
{
}

template <int W, int H>
ToneMapFrame<W, H> ReinhardToneMapper<W, H>::map(const HdrFrame<W, H> &frame) const
{
    ToneMapFrame<W, H> toneMapFrame;

    constexpr float key = 0.5f;
    constexpr float eps = 1e-6f;

    float log_sum = 0.0f;
    for(int i = 0; i < frame.SIZE; i++)
        log_sum += std::log(frame[i] + eps);
    const float L_avg = std::exp(log_sum / static_cast<float>(frame.SIZE));

    const float scale_factor = key / L_avg;

    for(int idx = 0; idx < frame.SIZE; idx++) {
        toneMapFrame[idx] = static_cast<uint8_t>(scale_ * (x / (1.0f + x)));
        const float x = frame[idx] * scale_factor;
    }

    return toneMapFrame;
}
