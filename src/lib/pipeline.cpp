#include "pipeline.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include "iostream"
#include "types.h"

namespace {

std::vector<float> Demosaic(std::vector<float>& normalized_bayer, LibRaw& rawProcessor, int width, int height) {

    auto get_pixel = [&](int y, int x) -> float {
        x = std::clamp(x, 0, width - 1);
        y = std::clamp(y, 0, height - 1);
        return normalized_bayer[y * width + x];
    };

    std::vector<float> rgb(width * height * 3, 0);

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
            rgb[idx + 0] = R;
            rgb[idx + 1] = G;
            rgb[idx + 2] = B;
        }
    }
    return rgb;
}

std::vector<float> WhiteBalance(std::vector<float>& rgb, LibRaw& rawProcessor) {
    float wb_r = rawProcessor.imgdata.color.cam_mul[0];
    float wb_g = rawProcessor.imgdata.color.cam_mul[1];
    float wb_b = rawProcessor.imgdata.color.cam_mul[2];

    auto max = std::max({wb_r, wb_g, wb_b});

    std::cout << "wb_r: " << wb_r << ", wb_g: " << wb_g << ", wb_b: " << wb_b << std::endl;

    for (int i = 0; i < rgb.size(); i++) {
        if (i % 3 == 0) {
            rgb[i] *= wb_r / max;
        } else if (i % 3 == 1) {
            rgb[i] *= wb_g / max;
        } else {
            rgb[i] *= wb_b / max;
        }
    }
    return rgb;
}

std::vector<float> ColorSpaceConversion(std::vector<float>& rgb, LibRaw& rawProcessor) {
    for (int i = 0; i < rgb.size(); i += 3) {
        float R = rgb[i];
        float G = rgb[i + 1];
        float B = rgb[i + 2];

        float r_srgb = rawProcessor.imgdata.color.rgb_cam[0][0] * R + rawProcessor.imgdata.color.rgb_cam[0][1] * G +
                       rawProcessor.imgdata.color.rgb_cam[0][2] * B;
        float g_srgb = rawProcessor.imgdata.color.rgb_cam[1][0] * R + rawProcessor.imgdata.color.rgb_cam[1][1] * G +
                       rawProcessor.imgdata.color.rgb_cam[1][2] * B;
        float b_srgb = rawProcessor.imgdata.color.rgb_cam[2][0] * R + rawProcessor.imgdata.color.rgb_cam[2][1] * G +
                       rawProcessor.imgdata.color.rgb_cam[2][2] * B;

        rgb[i] = r_srgb;
        rgb[i + 1] = g_srgb;
        rgb[i + 2] = b_srgb;
    }
    return rgb;
}

std::vector<float> GammaCorrection(std::vector<float>& rgb) {
    for (int i = 0; i < rgb.size(); i++) {
        rgb[i] = std::pow(std::clamp(rgb[i], 0.0f, 1.0f), 1.0f / 2.2f);  // sRGB gamma
    }
    return rgb;
}

raw::RGB8_Data ToRgb8(const std::vector<float>& rgb) {
    raw::RGB8_Data rgb_data(rgb.size());
    for (int i = 0; i < rgb.size(); ++i) {
        rgb_data[i] = std::clamp(rgb[i] * 255, 0.0F, 255.0F);
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
    auto rgb = Demosaic(normalized_bayer, rawProcessor, rawProcessor.imgdata.sizes.raw_width,
                        rawProcessor.imgdata.sizes.raw_height);

    // White balance
    rgb = WhiteBalance(rgb, rawProcessor);

    std::cout << "rgb max: " << *std::max_element(rgb.begin(), rgb.end()) << std::endl;
    std::cout << "rgb min: " << *std::min_element(rgb.begin(), rgb.end()) << std::endl;

    // sRGB
    rgb = ColorSpaceConversion(rgb, rawProcessor);

    // Gamma Correction
    rgb = GammaCorrection(rgb);

    // ToRgb8
    auto rgb8 = ToRgb8(rgb);

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << "Pipeline time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms" << std::endl;

    return {rgb8, rawProcessor.imgdata.sizes.raw_width, rawProcessor.imgdata.sizes.raw_height};
}
}  // namespace raw