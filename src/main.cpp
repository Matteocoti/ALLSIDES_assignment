#include <chrono>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <vector>
#include "FakeCamera.hpp"
#include "HdrCombiner.hpp"
#include "Sensor.hpp"
#include "SyntheticScene.hpp"
#include "ToneMapper.hpp"

namespace
{
// Sensor resolution required by the assignment.
constexpr int RES = 4504;

// Read-noise standard deviation in ADU (see FakeCamera / README).
constexpr float READ_NOISE_ADU = 5.0f;

long timeDiff(std::chrono::steady_clock::time_point a, std::chrono::steady_clock::time_point b)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(b - a).count();
}

}  // namespace

int main()
{
    SyntheticScene<RES, RES> scene;

    scene.toPGM("scene.pgm", 1.0f);

    FakeCamera<RES, RES> camera(scene, READ_NOISE_ADU);

    // Acquire the three exposures, timing each grab.
    auto t0 = std::chrono::steady_clock::now();
    CameraFrame<RES, RES> fshort = camera.grab(ExposureTime::Short);
    auto t1 = std::chrono::steady_clock::now();
    CameraFrame<RES, RES> fmedium = camera.grab(ExposureTime::Medium);
    auto t2 = std::chrono::steady_clock::now();
    CameraFrame<RES, RES> flong = camera.grab(ExposureTime::Long);
    auto t3 = std::chrono::steady_clock::now();

    std::cout << "Short:  " << timeDiff(t0, t1) << " ms\n";
    std::cout << "Medium: " << timeDiff(t1, t2) << " ms\n";
    std::cout << "Long:   " << timeDiff(t2, t3) << " ms\n";

    fshort.toPGM("short.pgm", Sensor12::MAX_VAL);
    fmedium.toPGM("medium.pgm", Sensor12::MAX_VAL);
    flong.toPGM("long.pgm", Sensor12::MAX_VAL);

    // Merge the exposures into an HDR radiance map, timing the merge.
    HdrCombiner<RES, RES> combiner;
    auto t4 = std::chrono::steady_clock::now();
    HdrFrame<RES, RES> hdr = combiner.merge(std::vector<CameraFrame<RES, RES>>{fshort, fmedium, flong});
    auto t5 = std::chrono::steady_clock::now();

    std::cout << "Merge:  " << timeDiff(t4, t5) << " ms\n";

    // Tone-map the HDR result to a viewable 8-bit image.
    ReinhardToneMapper<RES, RES> mapper(255);
    ToneMapFrame<RES, RES> bmp = mapper.map(hdr);
    bmp.toPGM("hdr.pgm", 255);

    return 0;
}
