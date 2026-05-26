#include "SyntheticScene.hpp"
#include "FakeCamera.hpp"

int main(int argc, char *argv[])
{
    SyntheticScene<4504, 4504> scene;
    FakeCamera<4504, 4504> camera(scene);

    CameraFrame<4504, 4504> fshort = camera.grab(ExposureTime::Short);
    CameraFrame<4504, 4504> fmedium = camera.grab(ExposureTime::Medium);
    CameraFrame<4504, 4504> flong = camera.grab(ExposureTime::Long);


    fshort.toPGM("short.pgm", 4095);
    fmedium.toPGM("medium.pgm", 4095);
    flong.toPGM("long.pgm", 4095);



    flong.toPGM("long.pgm", 4096);
    return 0;
}
