#include <iostream>
#include "HdrCombiner.hpp"
#include "SyntheticScene.hpp"
#include "FakeCamera.hpp"
#include "ToneMapper.hpp"

auto timeDiff(std::chrono::steady_clock::time_point a, std::chrono::steady_clock::time_point b)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(b - a).count();
}

int main(int argc, char *argv[])
{
    SyntheticScene<4504, 4504> scene;
    FakeCamera<4504, 4504> camera(scene);

    auto t0 = std::chrono::steady_clock::now();
    CameraFrame<4504, 4504> fshort = camera.grab(ExposureTime::Short);
    auto t1 = std::chrono::steady_clock::now();
    CameraFrame<4504, 4504> fmedium = camera.grab(ExposureTime::Medium);
    auto t2 = std::chrono::steady_clock::now();
    CameraFrame<4504, 4504> flong = camera.grab(ExposureTime::Long);
    auto t3 = std::chrono::steady_clock::now();

    std::cout << "Short:  " << timeDiff(t0, t1) << " ms\n";
    std::cout << "Medium: " << timeDiff(t1, t2) << " ms\n";
    std::cout << "Long:   " << timeDiff(t2, t3) << " ms\n";

    fshort.toPGM("short.pgm", 4095);
    fmedium.toPGM("medium.pgm", 4095);
    flong.toPGM("long.pgm", 4095);

    HdrCombiner<4504, 4504> combiner;
    auto t4 = std::chrono::steady_clock::now();
    HdrFrame<4504, 4504> hdr = combiner.merge(std::vector<CameraFrame<4504, 4504>>{fshort, fmedium, flong});
    auto t5 = std::chrono::steady_clock::now();

    std::cout << "Merge:  " << timeDiff(t4, t5) << " ms\n";
    ReinhardToneMapper<4504, 4504> mapper(255);
    ToneMapFrame<4504, 4504> bmp = mapper.map(hdr);

    bmp.toPGM("hdr.pgm", 255);

    return 0;
}
