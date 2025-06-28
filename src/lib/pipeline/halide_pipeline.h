#pragma once

#include <Halide.h>

namespace raw::pipeline {

auto ColorSpaceConversion(Halide::Func input, Halide::Var x, Halide::Var y, Halide::Var c,
                          std::array<std::array<float, 3>, 3> rgb_cam) -> Halide::Func {
    Halide::Func color_space_converted("color_space_converted");
    color_space_converted(x, y, c) = Halide::select(
        c == 0, rgb_cam[0][0] * input(x, y, 0) + rgb_cam[0][1] * input(x, y, 1) + rgb_cam[0][2] * input(x, y, 2),
        c == 1, rgb_cam[1][0] * input(x, y, 0) + rgb_cam[1][1] * input(x, y, 1) + rgb_cam[1][2] * input(x, y, 2),
        rgb_cam[2][0] * input(x, y, 0) + rgb_cam[2][1] * input(x, y, 1) + rgb_cam[2][2] * input(x, y, 2));
    return color_space_converted;
}

auto GammaCorrection(Halide::Func input, Halide::Var x, Halide::Var y, Halide::Var c) -> Halide::Func {
    Halide::Func gamma_corrected("gamma_corrected");
    gamma_corrected(x, y, c) = Halide::pow(Halide::clamp(input(x, y, c), 0.0f, 1.0f), 1.0f / 2.2f);
    return gamma_corrected;
}

auto ContrastAdjustment(Halide::Func input, Halide::Var x, Halide::Var y, Halide::Var c, float contrast_factor,
                        float midpoint = 0.5f) -> Halide::Func {
    Halide::Func contrast_adjusted("contrast_adjusted");
    contrast_adjusted(x, y, c) = Halide::clamp((input(x, y, c) - midpoint) * contrast_factor + midpoint, 0.0f, 1.0f);
    return contrast_adjusted;
}

auto ToRgb8(Halide::Func input, Halide::Var x, Halide::Var y, Halide::Var c) -> Halide::Func {
    Halide::Func rgb8("rgb8");
    rgb8(x, y, c) = Halide::cast<uint8_t>(Halide::clamp(Halide::round(input(x, y, c) * 255.0f), 0.0f, 255.0f));
    return rgb8;
}

}  // namespace raw::pipeline