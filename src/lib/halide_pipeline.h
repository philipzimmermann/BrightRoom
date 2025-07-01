#pragma once

#include <Halide.h>

namespace raw::pipeline {

inline auto FC(Halide::Var x, Halide::Var y, int filters) -> Halide::Func {
    Halide::Func fc("fc");
    fc(x, y) = Halide::cast<uint16_t>(filters >> (((y << 1 & 14) | (x & 1)) << 1) & 3);
    return fc;
}

inline auto BlackLevel(Halide::Func input, Halide::Var x, Halide::Var y, Halide::Func fc, int black,
                       const std::array<int, 4>& cblack) -> Halide::Func {
    auto black_value = Halide::cast<uint16_t>(
        static_cast<int>(black) +
        Halide::select(fc(x, y) == 0, cblack[0],
                       Halide::select(fc(x, y) == 1, cblack[1], Halide::select(fc(x, y) == 2, cblack[2], cblack[3]))));
    Halide::Func black_adjusted;
    black_adjusted(x, y) = Halide::cast<uint16_t>(Halide::max(0, Halide::cast<int32_t>(input(x, y)) - black_value));
    return black_adjusted;
}

inline auto WhiteLevel(Halide::Func input, Halide::Var x, Halide::Var y, int white) -> Halide::Func {
    Halide::Func output("white_level");

    // Convert to float, divide by white level, and clamp between 0 and 1
    output(x, y) = Halide::clamp(Halide::cast<float>(input(x, y)) / static_cast<float>(white), 0.0f, 1.0f);

    return output;
}

inline auto WhiteBalance(Halide::Func input, Halide::Var x, Halide::Var y, Halide::Var c,
                         std::array<float, 3> wb_factors) -> Halide::Func {
    Halide::Func white_balanced("white_balanced");
    white_balanced(x, y, c) = input(x, y, c) * Halide::select(c == 0, wb_factors[0],  // Red channel
                                                              c == 1, wb_factors[1],  // Green channel
                                                              wb_factors[2]           // Blue channel
                                               );
    return white_balanced;
}

inline auto DemosaicBilinear(Halide::Func input, Halide::Var x, Halide::Var y, Halide::Var c,
                             Halide::Func fc) -> Halide::Func {
    Halide::Func demosaiced("demosaiced");
    Halide::Expr red, green, blue;

    auto color = fc(x, y);
    Halide::Expr at_red = color == 0;
    Halide::Expr at_green = (color == 1) || (color == 3);
    Halide::Expr at_blue = color == 2;

    // Red channel
    red = Halide::select(
        at_red, input(x, y),                                                           // At red pixel
        at_green,                                                                      // At green pixel
        Halide::select(fc(x + 1, y) == 0, (input(x - 1, y) + input(x + 1, y)) / 2.0f,  // In red-green row
                       (input(x, y - 1) + input(x, y + 1)) / 2.0f),                    // In blue-green row
        (input(x - 1, y - 1) + input(x + 1, y - 1) + input(x - 1, y + 1) + input(x + 1, y + 1)) /
            4.0f);  // At blue pixel

    // Green channel
    green = Halide::select(
        at_green, input(x, y),                                                            // At green pixel
        (input(x, y - 1) + input(x, y + 1) + input(x - 1, y) + input(x + 1, y)) / 4.0f);  // At red or blue pixel

    // Blue channel
    blue = Halide::select(
        at_blue, input(x, y),                                                          // At blue pixel
        at_green,                                                                      // At green pixel
        Halide::select(fc(x + 1, y) == 0, (input(x, y - 1) + input(x, y + 1)) / 2.0f,  // Green in red-green row
                       (input(x - 1, y) + input(x + 1, y)) / 2.0f),                    // Green in blue-green row
        (input(x - 1, y - 1) + input(x + 1, y - 1) + input(x - 1, y + 1) + input(x + 1, y + 1)) /
            4.0f);  // At red pixel

    // Combine channels into RGB output
    demosaiced(x, y, c) = Halide::select(c == 0, red, c == 1, green, blue);

    return demosaiced;
}

inline auto LogSum(Halide::Func input, Halide::Var x, Halide::Var y, int width, int height) -> Halide::Func {
    Halide::Func luminance("luminance");
    luminance(x, y) = 0.2126f * input(x, y, 0) + 0.7152f * input(x, y, 1) + 0.0722f * input(x, y, 2);

    Halide::Func log_luminance("log_luminance");
    log_luminance(x, y) = Halide::log(Halide::max(luminance(x, y), 1e-6f));

    // luminance.compute_root().parallel(y).vectorize(x, 8);
    //     log_luminance.compute_root().parallel(y).vectorize(x, 8);
    Halide::RDom r(0, width, 0, height);
    Halide::Func log_sum("log_sum");
    log_sum() = Halide::sum(log_luminance(r.x, r.y));
    return log_sum;
}

inline auto ToneMapping(Halide::Func input, Halide::Var x, Halide::Var y, Halide::Var c, float log_avg,
                        float key_value = 0.18f) -> Halide::Func {
    Halide::Func tone_mapped("tone_mapped");
    Halide::Func luminance("luminance");

    luminance(x, y) = 0.2126f * input(x, y, 0) + 0.7152f * input(x, y, 1) + 0.0722f * input(x, y, 2);

    Halide::Expr scale_factor = key_value / log_avg;

    Halide::Expr lum = luminance(x, y);
    Halide::Expr mapped_lum = (lum * scale_factor) / (1.0f + lum * scale_factor);
    Halide::Expr scale = mapped_lum / (lum + 1e-6f);

    tone_mapped(x, y, c) = Halide::clamp(input(x, y, c) * scale, 0.0f, 1.0f);

    // luminance.compute_root().parallel(y).vectorize(x, 8);
    // tone_mapped.compute_root().parallel(y).vectorize(x, 8);

    return tone_mapped;
}

inline auto Exposure(Halide::Func input, Halide::Var x, Halide::Var y, Halide::Var c, float exposure) -> Halide::Func {
    Halide::Func exposure_adjusted("exposure_adjusted");
    exposure_adjusted(x, y, c) = input(x, y, c) * exposure;
    return exposure_adjusted;
}

inline auto ColorSpaceConversion(Halide::Func input, Halide::Var x, Halide::Var y, Halide::Var c,
                                 std::array<std::array<float, 3>, 3> rgb_cam) -> Halide::Func {
    Halide::Func color_space_converted("color_space_converted");
    color_space_converted(x, y, c) = Halide::select(
        c == 0, rgb_cam[0][0] * input(x, y, 0) + rgb_cam[0][1] * input(x, y, 1) + rgb_cam[0][2] * input(x, y, 2),
        c == 1, rgb_cam[1][0] * input(x, y, 0) + rgb_cam[1][1] * input(x, y, 1) + rgb_cam[1][2] * input(x, y, 2),
        rgb_cam[2][0] * input(x, y, 0) + rgb_cam[2][1] * input(x, y, 1) + rgb_cam[2][2] * input(x, y, 2));
    return color_space_converted;
}

inline auto GammaCorrection(Halide::Func input, Halide::Var x, Halide::Var y, Halide::Var c) -> Halide::Func {
    Halide::Func gamma_corrected("gamma_corrected");
    gamma_corrected(x, y, c) = Halide::pow(Halide::clamp(input(x, y, c), 0.0f, 1.0f), 1.0f / 2.2f);
    return gamma_corrected;
}

inline auto ContrastAdjustment(Halide::Func input, Halide::Var x, Halide::Var y, Halide::Var c, float contrast_factor,
                               float midpoint = 0.5f) -> Halide::Func {
    Halide::Func contrast_adjusted("contrast_adjusted");
    contrast_adjusted(x, y, c) = Halide::clamp((input(x, y, c) - midpoint) * contrast_factor + midpoint, 0.0f, 1.0f);
    return contrast_adjusted;
}

inline auto ToRgb8(Halide::Func input, Halide::Var x, Halide::Var y, Halide::Var c) -> Halide::Func {
    Halide::Func rgb8("rgb8");
    rgb8(x, y, c) = Halide::cast<uint8_t>(Halide::clamp(Halide::round(input(x, y, c) * 255.0f), 0.0f, 255.0f));
    return rgb8;
}

}  // namespace raw::pipeline