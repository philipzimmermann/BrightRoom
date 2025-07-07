#include "classic_pipeline.h"
#include <algorithm>

namespace classic {
auto Demosaic(std::vector<float>& normalized_bayer, LibRaw& rawProcessor, int width, int height) -> std::vector<float> {

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

auto WhiteBalance(std::vector<float>& rgb, LibRaw& rawProcessor) -> std::vector<float> {
    float wb_r = rawProcessor.imgdata.color.cam_mul[0];
    float wb_g = rawProcessor.imgdata.color.cam_mul[1];
    float wb_b = rawProcessor.imgdata.color.cam_mul[2];

    auto max = std::max({wb_r, wb_g, wb_b});

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

auto ToneMapping(std::vector<float>& rgb, float key_value) -> std::vector<float> {
    // First calculate the log average luminance
    float log_sum = 0.0f;
    std::vector<float> luminance(rgb.size() / 3);

    for (size_t i = 0; i < rgb.size(); i += 3) {
        // Calculate luminance using the Rec. 709 weights
        float lum = 0.2126f * rgb[i] + 0.7152f * rgb[i + 1] + 0.0722f * rgb[i + 2];
        luminance[i / 3] = lum;
        log_sum += std::log(std::max(1e-6f, lum));
    }

    float log_average = std::exp(log_sum / (rgb.size() / 3));

    // Scale factor based on key value (key_value controls the overall brightness)
    float scale_factor = key_value / log_average;

    // Apply Reinhard tone mapping to RGB channels while preserving colors
    for (size_t i = 0; i < rgb.size(); i += 3) {
        float lum = luminance[i / 3];
        float mapped_lum = (lum * scale_factor) / (1.0f + lum * scale_factor);
        float scale = mapped_lum / (lum + 1e-6f);  // Avoid division by zero

        // Apply the scaling to preserve colors
        rgb[i] = std::clamp(rgb[i] * scale, 0.0f, 1.0f);
        rgb[i + 1] = std::clamp(rgb[i + 1] * scale, 0.0f, 1.0f);
        rgb[i + 2] = std::clamp(rgb[i + 2] * scale, 0.0f, 1.0f);
    }

    return rgb;
}

auto ColorSpaceConversion(std::vector<float>& rgb, LibRaw& rawProcessor) -> std::vector<float> {
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

auto ExposureCompensation(std::vector<float>& rgb, float exposure) -> std::vector<float> {
    for (int i = 0; i < rgb.size(); i++) {
        rgb[i] *= exposure;
    }
    return rgb;
}

auto GammaCorrection(std::vector<float>& rgb) -> std::vector<float> {
    for (int i = 0; i < rgb.size(); i++) {
        rgb[i] = std::pow(std::clamp(rgb[i], 0.0f, 1.0f), 1.0f / 2.2f);  // sRGB gamma
    }
    return rgb;
}

auto ContrastAdjustment(std::vector<float>& rgb, float contrast, float midpoint) -> std::vector<float> {
    for (int i = 0; i < rgb.size(); i++) {
        rgb[i] = std::clamp((rgb[i] - midpoint) * contrast + midpoint, 0.0f, 1.0f);
    }
    return rgb;
}

auto ToRgb8(const std::vector<float>& rgb) -> raw::RGB8_Data {
    raw::RGB8_Data rgb_data(rgb.size());
    for (int i = 0; i < rgb.size(); ++i) {
        rgb_data[i] = std::clamp(rgb[i] * 255, 0.0F, 255.0F);
    }
    return rgb_data;
}
}  // namespace classic