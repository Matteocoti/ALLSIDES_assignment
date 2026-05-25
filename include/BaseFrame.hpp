#include <vector>
template <typename type, int W, int H>
class BaseFrame
{
   protected:
    std::vector<type> data_;

   public:
    static constexpr int WIDTH = W;
    static constexpr int HEIGHT = H;
    static constexpr int SIZE = W * H;

    BaseFrame() : data_(SIZE, type{0}) {}

    // TODO: bound check

    type &operator()(int x, int y)
    {
        return data_[x + y * WIDTH];
    }

    const type &operator()(int x, int y) const
    {
        return data_[x + y * WIDTH];
    }

    type &operator[](int i)
    {
        return data_[i];
    }
    const type &operator[](int i) const
    {
        return data_[i];
    }
};
