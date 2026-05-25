#include "SyntheticScene.hpp"
#include "FakeCamera.hpp"

int main(int argc, char *argv[])
{
    SyntheticScene<4504, 4504> scene;

    FakeCamera<4504, 4504> fake_camera(scene);

    BaseFrame fshort = fake_camera.grab(ExposureTime::Short);
    BaseFrame fmedium = fake_camera.grab(ExposureTime::Medium);
    BaseFrame flong = fake_camera.grab(ExposureTime::Long);

    return 0;
}
