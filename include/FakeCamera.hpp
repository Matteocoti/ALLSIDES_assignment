#pragma once
#include <algorithm>
#include <cstdint>
#include <future>
#include <random>
#include <thread>
#include "ExposureTime.hpp"
#include "SyntheticScene.hpp"
#include "Sensor.hpp"

template <int W, int H>
class CameraFrame : public BaseFrame<uint16_t, W, H>
{
   public:
    const ExposureTime exposure;

    CameraFrame(ExposureTime exp) : exposure(exp) {}
};

template <int W, int H>
class FakeCamera
{
   private:
    const SyntheticScene<W, H> &scene_;
    const float stdDev_;

    void processPixels(CameraFrame<W, H> &frame, const int start, const int stop, const float gain) const;

   public:
    FakeCamera(const SyntheticScene<W, H> &scene, const float noiseStdDev);

    CameraFrame<W, H> grab(const ExposureTime exposure) const;
};

template <int W, int H>
FakeCamera<W, H>::FakeCamera(const SyntheticScene<W, H> &scene, const float noiseStdDev) : scene_(scene), stdDev_(noiseStdDev)
{
}

template <int W, int H>
void FakeCamera<W, H>::processPixels(CameraFrame<W, H> &frame, const int start, const int stop, const float gain) const
{
    std::random_device rd;
    std::mt19937 gen(rd());

    std::normal_distribution<float> gaussian_noise{0.0f, stdDev_};
    for(int i = start; i < stop; ++i) {
        // We compute the value of the ideal pixel as the value from the synthetic scene
        // multiplied by the gain
        const float pxl_idl = scene_[i] * gain;
        // Noisy value data is the sum of the ideal one and the gaussian noise
        const float pxl_noise = pxl_idl + gaussian_noise(gen);
        // We clamp the data
        frame[i] = static_cast<uint16_t>(std::clamp(pxl_noise, 0.0f, Sensor12::MAX_VAL_F));
    }
}

template <int W, int H>
CameraFrame<W, H> FakeCamera<W, H>::grab(const ExposureTime exposure) const
{
    CameraFrame<W, H> frame(exposure);

    // Define gain using the exposure time: the gain is defined as 1
    // with the medium exposure time and the others are scaled as consequence
    const float exposure_gain = exposure_ms(exposure) / exposure_ms(ExposureTime::Medium);

    // Then, the value is scaled to the sensor full range
    const float gain = exposure_gain * Sensor12::MAX_VAL_F;

    // Each pixel can be processed indipendently, so we can parallelize the computation.
    // Since no constraint was defined, we use all the availables threads
    // std::thread::hardware_concurrency can return zero, so in that case
    // we suppose only one thread available
    const unsigned n_threads = std::max(static_cast<unsigned>(1), std::thread::hardware_concurrency());

    // In case of a multithreaded environment, we use futures to parallelize
    // the pixel computation
    if(n_threads > 1) {
        // Each thread can process a chunk indipendently
        const unsigned chunks = scene_.SIZE / n_threads;

        std::vector<std::future<void>> futures;
        futures.reserve(n_threads);

        for(unsigned idx = 0; idx < n_threads; idx++) {
            // Defining the chunk limits: we look also at the case in which the size is not
            // a multiple of the number of threds. In that case, the last thread will compute
            // the remaining pixels.
            int start = chunks * idx;
            int stop = (idx == (n_threads - 1)) ? scene_.SIZE : (chunks * (idx + 1));

            futures.push_back(std::async(std::launch::async,
                                         [this, &frame, start, stop, gain]() { processPixels(frame, start, stop, gain); }));
        }

        // We wait for all the threads
        for(auto &f : futures)
            f.get();
    } else {
        processPixels(frame, 0, scene_.SIZE, gain);
    }

    return frame;
}
