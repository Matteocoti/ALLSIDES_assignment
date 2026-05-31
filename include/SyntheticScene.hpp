#pragma once
#include <cmath>
#include "BaseFrame.hpp"

/**
 * @brief Synthetic, exposure-independent scene radiance map in [0, 1].
 *
 * The radiance is the ground truth that every exposure shares: only the camera
 * gain changes between exposures, never the scene content.
 *
 * The default scene is a horizontal logarithmic radiance ramp spanning a
 * dynamic range far wider than any single exposure can capture (from a deep
 * shadow that only the long exposure lifts above the noise floor, to a highlight
 * that saturates the long/medium exposures and survives only in the short one).
 * This forces the merge to combine all three exposures and makes it easy to
 * validate: the recovered radiance should be linear in the known ground truth
 * (up to a global scale), with seamless exposure hand-offs.
 *
 * @tparam W Image width in pixels.
 * @tparam H Image height in pixels.
 */
template <int W, int H>
class SyntheticScene : public BaseFrame<float, W, H>
{
   public:
    /**
     * @brief Builds the default logarithmic radiance ramp.
     */
    SyntheticScene();
};

template <int W, int H>
SyntheticScene<W, H>::SyntheticScene()
{
    // Radiance varies log-linearly along x (constant along y), from R_MIN to
    // R_MAX. The log spacing gives equal relative steps per column, matching how
    // exposures and noise behave multiplicatively.
    constexpr float R_MIN = 1e-4f;  // deep shadow (near the noise floor)
    constexpr float R_MAX = 1.0f;   // highlight (saturates the long exposure)

    for(int x = 0; x < W; x++) {
        const float t = static_cast<float>(x) / (W - 1);          // 0..1 across the width
        const float radiance = R_MIN * std::pow(R_MAX / R_MIN, t);  // R_MIN -> R_MAX
        for(int y = 0; y < H; y++) {
            (*this)(x, y) = radiance;
        }
    }
}
