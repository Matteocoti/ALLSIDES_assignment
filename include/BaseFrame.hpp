#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

/**
 * @brief Fixed-size, contiguous 2D image buffer in row-major order.
 *
 * @tparam type Pixel value type.
 * @tparam W    Image width in pixels.
 * @tparam H    Image height in pixels.
 */
template <typename type, int W, int H>
class BaseFrame
{
   protected:
    std::vector<type> data_;  ///< Pixel storage, row-major, size WIDTH*HEIGHT.

   public:
    static constexpr int WIDTH = W;     ///< Image width in pixels.
    static constexpr int HEIGHT = H;    ///< Image height in pixels.
    static constexpr int SIZE = W * H;  ///< Total number of pixels.

    /**
     * @brief Constructs a frame with every pixel zero-initialised.
     *
     * The frame data are allocated on the heap using a std::vector.
     */
    BaseFrame() : data_(SIZE, type{0}) {}

    /**
     * @brief Unchecked mutable access by 2D coordinates (row-major).
     *
     * @param x Column index, expected in [0, WIDTH).
     * @param y Row index, expected in [0, HEIGHT).
     *
     * @return Reference to the pixel.
     *
     * @warning No bounds checking. Use at() for checked access.
     */
    type &operator()(int x, int y)
    {
        return data_[x + y * WIDTH];
    }

    /**
     * @brief Unchecked unmutable access by 2D coordinates (row-major).
     *
     * @param x Column index, expected in [0, WIDTH).
     * @param y Row index, expected in [0, HEIGHT).
     *
     * @return Const reference to the pixel.
     *
     * @warning No bounds checking. Use at() for checked access.
     */
    const type &operator()(int x, int y) const
    {
        return data_[x + y * WIDTH];
    }

    /**
     * @brief Checked mutable access by 2D coordinates.
     *
     * @param x Column index.
     * @param y Row index.
     *
     * @return Reference to the pixel.
     *
     * @throws std::out_of_range if (x, y) lies outside the image.
     */
    type &at(int x, int y)
    {
        if(x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) {
            throw std::out_of_range("BaseFrame::at: coordinates out of range");
        }
        return data_[x + y * WIDTH];
    }

    /**
     * @brief Checked unmutable access by 2D coordinates.
     *
     * @param x Column index.
     * @param y Row index.
     *
     * @return Const reference to the pixel.
     *
     * @throws std::out_of_range if (x, y) lies outside the image.
     */
    const type &at(int x, int y) const
    {
        if(x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) {
            throw std::out_of_range("BaseFrame::at: coordinates out of range");
        }
        return data_[x + y * WIDTH];
    }

    /**
     *  @brief Returns a pointer to the contiguous pixel buffer.
     *
     *  @return Mutable pointer to the underlying data array.
     */
    type *data() noexcept
    {
        return data_.data();
    }

    /**
     *  @brief Returns a const pointer to the contiguous pixel buffer.
     *
     *  @return Const pointer to the underlying data array.
     */
    const type *data() const noexcept
    {
        return data_.data();
    }

    /**
     *  @brief Unchecked mutable access by linear buffer index.
     *
     *  @param i Flat index, expected in [0, SIZE).
     *
     *  @return Reference to the pixel.
     *
     *  @warning No bounds checking. Intended for tight loops over contiguous data.
     */
    type &operator[](int i)
    {
        return data_[i];
    }

    /**
     *  @brief Unchecked unmutable access by linear buffer index.
     *
     *  @param i Flat index, expected in [0, SIZE).
     *
     *  @return Const reference to the pixel.
     *
     *  @warning No bounds checking. Intended for tight loops over contiguous data.
     */
    const type &operator[](int i) const
    {
        return data_[i];
    }

    /**
     * @brief Checked mutable access by linear buffer index.
     *
     * @param i Flat index.
     *
     * @return Reference to the pixel.
     *
     * @throws std::out_of_range if i is outside [0, SIZE).
     */
    type &at(int i)
    {
        if(i >= SIZE || i < 0) {
            throw std::out_of_range("BaseFrame::at: index out of range");
        }
        return data_[i];
    }

    /**
     * @brief Checked unmutable access by linear buffer index.
     *
     * @param i Flat index.
     *
     * @return Const reference to the pixel.
     *
     * @throws std::out_of_range if i is outside [0, SIZE).
     */
    const type &at(int i) const
    {
        if(i >= SIZE || i < 0) {
            throw std::out_of_range("BaseFrame::at: index out of range");
        }
        return data_[i];
    }

    /**
     * @brief Writes the frame as a binary PGM (P5) file.
     *
     * Pixels are normalised against @p max_value and written at 8-bit depth when
     * @p max_value <= 255, otherwise at 16-bit big-endian depth.
     *
     * @param filePath  Destination path.
     * @param max_value Value mapped to full white; must be > 0.
     *
     * @return true on success, false if the file could not be opened or a write failed.
     *
     * @throws std::invalid_argument if @p max_value <= 0.
     */
    bool toPGM(const std::string &filePath, const type max_value) const;
};

template <typename type, int W, int H>
bool BaseFrame<type, W, H>::toPGM(const std::string &filePath, const type max_value) const
{
    if(max_value <= 0) {
        throw std::invalid_argument("BaseFrame::toPGM: max value must be > 0");
    }

    std::ofstream f(filePath, std::ios::binary);

    // Checking if the file was opened correctly
    if(!f) {
        return false;
    }

    // On the basis of the max value, the function defines if the data must be
    // represented with one or two bytes.
    const bool eightBit = (max_value <= static_cast<type>(255));
    const int outMax = eightBit ? 255 : 65535;

    f << "P5\n" << WIDTH << " " << HEIGHT << "\n" << outMax << "\n";

    for(const auto &pxl : data_) {
        const float norm = static_cast<float>(std::clamp(pxl, static_cast<type>(0), max_value)) / static_cast<float>(max_value);

        if(eightBit) {
            const auto val = static_cast<uint8_t>(std::lround(norm * 255.0f));
            f.put(static_cast<char>(val));
        } else {
            const auto val = static_cast<uint16_t>(std::lround(norm * 65535.0f));
            f.put(static_cast<char>(val >> 8));
            f.put(static_cast<char>(val & 0xFF));
        }
    }

    return f.good();
}
