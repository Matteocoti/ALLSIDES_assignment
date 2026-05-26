#pragma once
#include <cmath>
#include <cstdint>
#include "HdrCombiner.hpp"

template <int W, int H>
using ToneMapFrame = BaseFrame<uint8_t, W, H>;

template <int W, int H>
class ReinhardToneMapper
{
   private:
    uint16_t scale_;

   public:
    ReinhardToneMapper(uint16_t scale);
    ToneMapFrame<W, H> map(const HdrFrame<W, H> &frame) const noexcept;
};

template <int W, int H>
ReinhardToneMapper<W, H>::ReinhardToneMapper(uint16_t scale) : scale_(scale) {}

template <int W, int H>
ToneMapFrame<W, H> ReinhardToneMapper<W, H>::map(const HdrFrame<W, H> &frame) const noexcept
{
    ToneMapFrame<W, H> toneMapFrame;

    constexpr float key = 0.5f;
    constexpr float eps = 1e-6f;

    float log_sum = 0.0f;
    for(int i = 0; i < frame.SIZE; i++)
        log_sum += std::log(frame[i] + eps);
    float L_avg = std::exp(log_sum / static_cast<float>(frame.SIZE));

    float scale_factor = key / L_avg;

    for(int idx = 0; idx < frame.SIZE; idx++) {
        float x = frame[idx] * scale_factor;
        toneMapFrame[idx] = static_cast<uint8_t>(scale_ * (x / (1.0f + x)));
    }

    return toneMapFrame;
}
