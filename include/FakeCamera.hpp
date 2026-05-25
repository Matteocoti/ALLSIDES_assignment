#pragma once
#include <algorithm>
#include <cstdint>
#include <future>
#include <thread>
#include "SyntheticScene.hpp"

enum class ExposureTime : uint16_t {
    Short = 5,
    Medium = 40,
    Long = 320,
};

constexpr float exposure_ms(ExposureTime e)
{
    return static_cast<float>(static_cast<uint16_t>(e));
}

template <int W, int H>
class FakeCamera
{
   private:
    const SyntheticScene<W, H> &scene_;

   public:
    FakeCamera(const SyntheticScene<W, H> &scene);

    BaseFrame<uint16_t, W, H> grab(const ExposureTime exposure) const;
};

template <int W, int H>
FakeCamera<W, H>::FakeCamera(const SyntheticScene<W, H> &scene) : scene_(scene)
{
}

// TODO: handle std::async exception to mark as noexcept
template <int W, int H>
BaseFrame<uint16_t, W, H> FakeCamera<W, H>::grab(const ExposureTime exposure) const
{
    BaseFrame<uint16_t, W, H> frame;

    // Define gain using the exposure time: the gain is defined as 1
    // with the medium exposure time and the others are scaled as consequence
    const float exposure_gain = exposure_ms(exposure) / exposure_ms(ExposureTime::Medium);

    // Then, the value is scaled to 12 bit
    const float gain = exposure_gain * 4095.0f;

    // Each pixel can be processed indipendently, so we can parallelize the computation.
    // Since no constraint was defined, we use all the availables threads
    const unsigned n_threads = std::thread::hardware_concurrency();
    // Each thread can process a chunk indipendently
    const unsigned chunks = scene_.SIZE / n_threads;

    std::vector<std::future<void>> futures;
    futures.reserve(n_threads);

    for(unsigned idx = 0; idx < n_threads; idx++) {
        // Defining the chunk limits: we look also at the case in which the size is not
        // a multiple of the number of threds. In that case, the last thread will compute
        // the remaining pixels.
        int start = chunks * idx;
        int end = (idx == (n_threads - 1)) ? scene_.SIZE : (chunks * (idx + 1));

        futures.push_back(std::async(std::launch::async, [&s = this->scene_, &frame, gain, start, end]() {
            for(int i = start; i < end; ++i) {
                // We compute the value of the ideal pixel as the value from the synthetic scene
                // multiplied by the gain
                const float pxl_idl = s[i] * gain;

                // We clamp the data
                frame[i] = static_cast<uint16_t>(std::clamp(pxl_idl, 0.0f, 4095.0f));
            }
        }));
    }

    // We wait for all the threads
    for(auto &f : futures)
        f.get();

    return frame;
}
