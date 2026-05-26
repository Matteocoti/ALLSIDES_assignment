#include "SyntheticScene.hpp"
#include "FakeCamera.hpp"

int main(int argc, char *argv[])
{
    SyntheticScene<4504, 4504> scene;

    FakeCamera<4504, 4504> fake_camera(scene);

    BaseFrame fshort = fake_camera.grab(ExposureTime::Short);

    fshort.toPGM("short.pgm", 4096);

    BaseFrame fmedium = fake_camera.grab(ExposureTime::Medium);

    fmedium.toPGM("medium.pgm", 4096);
    BaseFrame flong = fake_camera.grab(ExposureTime::Long);

    flong.toPGM("long.pgm", 4096);
    return 0;
}
