#pragma once

#include "ExposureTime.hpp"
#include "FakeCamera.hpp"
#include "Sensor.hpp"

template <int W, int H>
using HdrFrame = BaseFrame<float, W, H>;

template <int W, int H>
class HdrCombiner
{
   private:
    std::vector<float> weight_;

   public:
    HdrCombiner();

    HdrFrame<W, H> merge(const std::vector<CameraFrame<W, H>> &frames,
                         const size_t max_iter = 20,
                         const float delta = 1e-3) const;
};
template <int W, int H>
HdrCombiner<W, H>::HdrCombiner() : weight_(Sensor12::NUM_LEVELS, 0.0f)
{
    // Gaussian reliability weight centred on the mid-range; the extremes
    // (0 and MAX_VAL) stay at 0 to discard saturated and black pixels.
    for(int i = 1; i < Sensor12::MAX_VAL; i++) {
        weight_[i] = std::exp(-4.0f * (i - Sensor12::MID) * (i - Sensor12::MID) / (Sensor12::MID * Sensor12::MID));
    }
}

template <int W, int H>
HdrFrame<W, H> HdrCombiner<W, H>::merge(const std::vector<CameraFrame<W, H>> &frames,
                                        const size_t max_iter,
                                        const float delta) const
{
    HdrFrame<W, H> frame;

    size_t num_frames = frames.size();

    size_t num_pixels = frame.SIZE;

    // Initialization of the camera response function as linear
    std::vector<float> f(Sensor12::NUM_LEVELS);

    for(size_t j = 0; j < Sensor12::NUM_LEVELS; j++) {
        f[j] = j / static_cast<float>(Sensor12::NUM_LEVELS);
    }

    std::vector<float> x(num_pixels);
    std::vector<uint32_t> card(Sensor12::NUM_LEVELS);

    std::vector<float> num_f(Sensor12::NUM_LEVELS, 0.0f);
    std::vector<float> exp_times(num_frames);
    for(size_t i = 0; i < num_frames; i++) {
        exp_times[i] = exposure_ms(frames[i].exposure);
    }

    // Convergence loop
    for(size_t iter = 0; iter < max_iter; iter++) {
        std::fill(card.begin(), card.end(), 0);
        std::fill(num_f.begin(), num_f.end(), 0.0f);
        // Step A: finding x[j] for each pixel looping over the frames
        for(size_t j = 0; j < num_pixels; j++) {
            float num = 0, den = 0;
            for(size_t i = 0; i < num_frames; i++) {
                uint16_t z = frames[i][j];
                num += weight_[z] * f[z] * exp_times[i];
                den += weight_[z] * (exp_times[i] * exp_times[i]);
            }
            x[j] = (den) ? num / den : 0;
        }

        // Step B: once determined the current optimal x, we try to determine the
        // camera response function
        for(size_t j = 0; j < num_pixels; j++) {
            for(size_t i = 0; i < num_frames; i++) {
                uint16_t m = frames[i][j];
                if(m == 0 || m == Sensor12::MAX_VAL) {
                    continue;
                }
                card[m] += 1;
                num_f[m] += exp_times[i] * x[j];
            }
        }

        std::vector<float> f_new(Sensor12::NUM_LEVELS, 0.0f);
        for(size_t m = 0; m < Sensor12::NUM_LEVELS; m++) {
            f_new[m] = card[m] > 0 ? num_f[m] / card[m] : (m > 0 ? f[m] : 0.0f);
        }
        float pivot = f_new[Sensor12::NUM_LEVELS / 2];
        for(size_t m = 0; m < Sensor12::NUM_LEVELS; m++) {
            f_new[m] /= pivot;
        }

        float diff = 0;
        for(size_t m = 0; m < Sensor12::NUM_LEVELS; m++) {
            diff += std::abs(f_new[m] - f[m]);
        }

        // Convergence check
        if(diff / static_cast<float>(Sensor12::NUM_LEVELS) < delta) {
            break;
        }
        f = f_new;
    }
    for(size_t j = 0; j < num_pixels; j++) {
        frame[j] = x[j];
    }
    return frame;
}
