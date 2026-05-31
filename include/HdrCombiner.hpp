#pragma once

#include <algorithm>
#include <cmath>
#include <future>
#include <thread>
#include <vector>
#include "ExposureTime.hpp"
#include "FakeCamera.hpp"
#include "Sensor.hpp"

/**
 * @brief HDR output frame: a floating-point radiance map.
 */
template <int W, int H>
using HdrFrame = BaseFrame<float, W, H>;

/**
 * @brief Per-thread partial histograms used to re-estimate the response (Step B).
 *
 * Each worker accumulates into its own Hist over a disjoint range of pixels;
 * the partials are then reduced into the global histograms, avoiding data races.
 */
struct Hist {
    std::vector<uint32_t> card;  ///< Per-level pixel counts.
    std::vector<float> numf;     ///< Per-level accumulated t_i * x_j.
};

/**
 * @brief Merges multiple exposures into an HDR radiance map (Robertson's method).
 *
 * Implements the iterative maximum-likelihood estimation of Robertson et al.:
 * it alternates between estimating the per-pixel irradiance (Step A) and the
 * camera response function (Step B), weighting each measurement by its
 * reliability (a Gaussian centred on the mid-range, zero for saturated and black
 * pixels). Saturated and underexposed samples are therefore excluded from the
 * estimate.
 *
 * @tparam W Image width in pixels.
 * @tparam H Image height in pixels.
 */
template <int W, int H>
class HdrCombiner
{
   private:
    std::vector<float> weight_;  ///< Reliability weight per pixel value (LUT, size NUM_LEVELS).

   public:
    /**
     * @brief Builds the reliability weight lookup table.
     */
    HdrCombiner();

    /**
     * @brief Merges the given exposures into a single HDR radiance map.
     *
     * @param frames   The exposures to merge (same scene, different exposure times).
     *
     * @param max_iter Maximum number of refinement iterations.
     * @param delta    Convergence threshold on the mean per-level response change.
     *
     * @return The merged HDR frame (radiance, defined up to a global scale factor).
     */
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

    // Pixels are independent in Step A, so the work is split across all available
    // hardware threads. hardware_concurrency() may report 0, hence the guard.
    const unsigned n_threads = std::max(1u, std::thread::hardware_concurrency());
    const size_t chunk = num_pixels / n_threads;

    // Convergence loop
    for(size_t iter = 0; iter < max_iter; iter++) {
        std::fill(card.begin(), card.end(), 0);
        std::fill(num_f.begin(), num_f.end(), 0.0f);
        // Step A: estimate x[j] for each pixel. Pixels are independent, so the
        // range is split across threads (each writes its own disjoint x[j]).
        std::vector<std::future<void>> futures;
        futures.reserve(n_threads);
        for(unsigned idx = 0; idx < n_threads; ++idx) {
            const size_t start = chunk * idx;
            const size_t stop = (idx == n_threads - 1) ? num_pixels : chunk * (idx + 1);
            futures.push_back(std::async(std::launch::async, [&, start, stop]() {
                for(size_t j = start; j < stop; ++j) {
                    float num = 0, den = 0;
                    for(size_t i = 0; i < num_frames; ++i) {
                        const uint16_t z = frames[i][j];
                        num += weight_[z] * f[z] * exp_times[i];
                        den += weight_[z] * (exp_times[i] * exp_times[i]);
                    }
                    x[j] = (den) ? num / den : 0.0f;
                }
            }));
        }
        for(auto &fut : futures) {
            fut.get();
        }

        // Step B: re-estimate the response from x. card[m] / num_f[m] are
        // histograms over the pixel value m, and different pixels hit the same
        // bin, so each thread accumulates into a private local histogram and the
        // partials are reduced into the globals.
        auto stepB = [&](size_t start, size_t stop) -> Hist {
            Hist h{std::vector<uint32_t>(Sensor12::NUM_LEVELS, 0), std::vector<float>(Sensor12::NUM_LEVELS, 0.0f)};
            for(size_t j = start; j < stop; ++j) {
                for(size_t i = 0; i < num_frames; ++i) {
                    const uint16_t m = frames[i][j];
                    if(m == 0 || m == Sensor12::MAX_VAL) {
                        continue;
                    }
                    h.card[m] += 1;
                    h.numf[m] += exp_times[i] * x[j];
                }
            }
            return h;
        };

        std::vector<std::future<Hist>> bFutures;
        bFutures.reserve(n_threads);
        for(unsigned idx = 0; idx < n_threads; ++idx) {
            const size_t start = chunk * idx;
            const size_t stop = (idx == n_threads - 1) ? num_pixels : chunk * (idx + 1);
            bFutures.push_back(std::async(std::launch::async, stepB, start, stop));
        }
        // Reduction of the per-thread partials into the global histograms.
        for(auto &fut : bFutures) {
            const Hist h = fut.get();
            for(int m = 0; m < Sensor12::NUM_LEVELS; ++m) {
                card[m] += h.card[m];
                num_f[m] += h.numf[m];
            }
        }

        std::vector<float> f_new(Sensor12::NUM_LEVELS, 0.0f);
        for(size_t m = 0; m < Sensor12::NUM_LEVELS; m++) {
            f_new[m] = card[m] > 0 ? num_f[m] / card[m] : (m > 0 ? f[m] : 0.0f);
        }
        // Fix the scale ambiguity by normalising the response at the mid-range.
        const float pivot = f_new[Sensor12::NUM_LEVELS / 2];
        if(pivot > 0.0f) {
            for(size_t m = 0; m < Sensor12::NUM_LEVELS; m++) {
                f_new[m] /= pivot;
            }
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
