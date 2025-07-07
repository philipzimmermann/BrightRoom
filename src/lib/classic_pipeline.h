#pragma once

#include <vector>
#include "libraw/libraw.h"
#include "types.h"

namespace classic {
auto Demosaic(std::vector<float>& normalized_bayer, LibRaw& rawProcessor, int width, int height) -> std::vector<float>;

auto WhiteBalance(std::vector<float>& rgb, LibRaw& rawProcessor) -> std::vector<float>;

auto ToneMapping(std::vector<float>& rgb, float key_value = 0.18f) -> std::vector<float>;

auto ColorSpaceConversion(std::vector<float>& rgb, LibRaw& rawProcessor) -> std::vector<float>;

auto ExposureCompensation(std::vector<float>& rgb, float exposure) -> std::vector<float>;

auto GammaCorrection(std::vector<float>& rgb) -> std::vector<float>;

auto ContrastAdjustment(std::vector<float>& rgb, float contrast, float midpoint = 0.5f) -> std::vector<float>;

auto ToRgb8(const std::vector<float>& rgb) -> raw::RGB8_Data;
}  // namespace classic