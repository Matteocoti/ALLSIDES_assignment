#include <cmath>
#include "BaseFrame.hpp"

template <int W, int H>
class SyntheticScene : public BaseFrame<float, W, H>
{
   public:
    SyntheticScene();
};

template <int W, int H>
SyntheticScene<W, H>::SyntheticScene()
{
    for(int y = 0; y < H; y++) {
        for(int x = 0; x < W; x++) {
            float dx = x - W / 2.f;
            float dy = y - H / 2.f;
            float r = sqrt(dx * dx + dy * dy);
            (*this)(x, y) = 0.5f + 0.5f * sinf(r * 0.05f);
        }
    }
}
