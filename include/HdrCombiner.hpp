#pragma once

#include "ExposureTime.hpp"
#include "FakeCamera.hpp"

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
HdrCombiner<W, H>::HdrCombiner() : weight_(4096, 0)
{
    for(int i = 1; i < 4095; i++) {
        weight_[i] = std::exp(-4.0f * (i - 2047.5f) * (i - 2047.5f) / (2047.5f * 2047.5f));
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

    // Initializazion of the function response camera as liner
    std::vector<float> f(4096);

    for(size_t j = 0; j < 4096; j++) {
        f[j] = j / 4096.0f;
    }

    std::vector<float> x(num_pixels);
    std::vector<uint32_t> card(4096);

    std::vector<float> num_f(4096, 0.0f);

    // Convergence loop
    for(size_t iter = 0; iter < max_iter; iter++) {
        std::fill(card.begin(), card.end(), 0);
        std::fill(num_f.begin(), num_f.end(), 0.0f);
        // Step A: findinf x[j] for each pixel looping over the frames
        for(size_t j = 0; j < num_pixels; j++) {
            float num = 0, den = 0;
            for(size_t i = 0; i < num_frames; i++) {
                float ti = exposure_ms(frames.at(i).exposure);
                uint16_t z = frames[i][j];
                num += weight_[z] * f[z] * ti;
                den += weight_[z] * (ti * ti);
            }
            x[j] = (den) ? num / den : 0;
        }

        // Step B: once determined the current optimal x, we try to determine the
        // camera response function
        for(size_t j = 0; j < num_pixels; j++) {
            for(size_t i = 0; i < num_frames; i++) {
                uint16_t m = frames[i][j];
                if(m == 0 || m == 4095)
                    continue;
                float ti = exposure_ms(frames.at(i).exposure);
                card[m] += 1;
                num_f[m] += ti * x[j];
            }
        }

        std::vector<float> f_new(4096, 0.0f);
        for(size_t m = 0; m < 4096; m++) {
            f_new[m] = card[m] > 0 ? num_f[m] / card[m] : (m > 0 ? f[m] : 0.0f);
        }
        float pivot = f_new[2047];
        for(size_t m = 0; m < 4096; m++) {
            f_new[m] /= pivot;
        }

        float diff = 0;
        for(size_t m = 0; m < 4096; m++) {
            diff += std::abs(f_new[m] - f[m]);
        }

        // Convergence check
        if(diff / 4096.0f < delta) {
            break;
        }
        f = f_new;
    }
    for(size_t j = 0; j < num_pixels; j++) {
        frame[j] = x[j];
    }
    return frame;
}
