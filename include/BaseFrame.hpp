#pragma once
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <string>
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

    bool toPGM(const std::string &filePath, const type max_value) const;
};

template <typename type, int W, int H>
bool BaseFrame<type, W, H>::toPGM(const std::string &filePath, const type max_value) const
{
    std::ofstream f(filePath, std::ios::binary);

    f << "P5" << std::endl;
    f << WIDTH << " " << HEIGHT << std::endl;
    f << 65535 << std::endl;

    for(const auto &pxl : data_) {
        float norm = static_cast<float>(std::clamp(pxl, static_cast<type>(0), max_value)) / static_cast<float>(max_value);

        uint16_t val = static_cast<uint16_t>(norm * 65535.0f);

        f.put(val >> 8);
        f.put(val & 0xFF);
    }

    return true;
}
