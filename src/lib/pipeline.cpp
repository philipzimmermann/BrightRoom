#include "pipeline.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include "iostream"
#include "types.h"

namespace {

std::vector<uint8_t> Demosaic(std::vector<float>& normalized_bayer, LibRaw& rawProcessor, int width, int height) {

    auto get_pixel = [&](int y, int x) -> float {
        x = std::clamp(x, 0, width - 1);
        y = std::clamp(y, 0, height - 1);
        return normalized_bayer[y * width + x];
    };

    std::vector<uint8_t> rgb8(width * height * 3, 0);

    for (int row = 1; row < height - 1; row++) {
        for (int col = 1; col < width - 1; col++) {
            int color = rawProcessor.FC(row, col);
            float R, G, B;

            if (color == 0) {  // Red
                R = get_pixel(row, col);
                G = (get_pixel(row - 1, col) + get_pixel(row + 1, col) + get_pixel(row, col - 1) +
                     get_pixel(row, col + 1)) /
                    4;
                B = (get_pixel(row - 1, col - 1) + get_pixel(row - 1, col + 1) + get_pixel(row + 1, col - 1) +
                     get_pixel(row + 1, col + 1)) /
                    4;
            } else if (color == 1 || color == 3) {  // Green
                G = get_pixel(row, col);
                if (rawProcessor.FC(row, col - 1) == 0) {  // Horizontal green line between R and B
                    R = (get_pixel(row, col - 1) + get_pixel(row, col + 1)) / 2;
                    B = (get_pixel(row - 1, col) + get_pixel(row + 1, col)) / 2;
                } else {
                    R = (get_pixel(row - 1, col) + get_pixel(row + 1, col)) / 2;
                    B = (get_pixel(row, col - 1) + get_pixel(row, col + 1)) / 2;
                }
            } else {  // Blue
                B = get_pixel(row, col);
                G = (get_pixel(row - 1, col) + get_pixel(row + 1, col) + get_pixel(row, col - 1) +
                     get_pixel(row, col + 1)) /
                    4;
                R = (get_pixel(row - 1, col - 1) + get_pixel(row - 1, col + 1) + get_pixel(row + 1, col - 1) +
                     get_pixel(row + 1, col + 1)) /
                    4;
            }

            int idx = (row * width + col) * 3;
            rgb8[idx + 0] = static_cast<uint8_t>(R * 255);
            rgb8[idx + 1] = static_cast<uint8_t>(G * 255);
            rgb8[idx + 2] = static_cast<uint8_t>(B * 255);
        }
    }
    return rgb8;
}

raw::RGB8_Data ToRgb8(const raw::RawFloatData& rawdata) {
    raw::RGB8_Data rgb_data(rawdata.size());
    for (int i = 0; i < rawdata.size(); ++i) {
        rgb_data[i] = rawdata[i] * 255;
    }
    return rgb_data;
}

}  // namespace

namespace raw {

RgbImage Pipeline::Run(LibRaw& rawProcessor) const {

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    // Black level substraction
    for (int row = 0; row < rawProcessor.imgdata.sizes.raw_height; row++) {
        for (int col = 0; col < rawProcessor.imgdata.sizes.raw_width; col++) {
            int color = rawProcessor.FC(row, col);  // Bayer color at this pixel
            ushort* pixel =
                &rawProcessor.imgdata.rawdata.raw_image[row * rawProcessor.imgdata.sizes.raw_pitch / 2 + col];
            auto black = rawProcessor.imgdata.color.black + rawProcessor.imgdata.color.cblack[color];
            *pixel = std::max(0, *pixel - static_cast<uint16_t>(black));
        }
    }

    // Normalize
    auto white_level = static_cast<float>(rawProcessor.imgdata.color.maximum);
    std::vector<float> normalized_bayer(rawProcessor.imgdata.sizes.raw_width * rawProcessor.imgdata.sizes.raw_height,
                                        0);
    std::cout << "white_level: " << white_level << std::endl;
    for (int row = 0; row < rawProcessor.imgdata.sizes.raw_height; row++) {
        for (int col = 0; col < rawProcessor.imgdata.sizes.raw_width; col++) {
            int color = rawProcessor.FC(row, col);
            ushort* pixel =
                &rawProcessor.imgdata.rawdata.raw_image[row * rawProcessor.imgdata.sizes.raw_pitch / 2 + col];
            float norm = static_cast<float>(*pixel) / white_level;
            norm = std::clamp(norm, 0.0F, 1.0F);
            normalized_bayer[row * rawProcessor.imgdata.sizes.raw_width + col] = norm;
        }
    }

    // Demosaic
    auto rgb8 = Demosaic(normalized_bayer, rawProcessor, rawProcessor.imgdata.sizes.raw_width,
                         rawProcessor.imgdata.sizes.raw_height);

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << "Pipeline time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms" << std::endl;

    return {rgb8, rawProcessor.imgdata.sizes.raw_width, rawProcessor.imgdata.sizes.raw_height};
}
}  // namespace raw