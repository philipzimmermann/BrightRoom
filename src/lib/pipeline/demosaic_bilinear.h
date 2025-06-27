#pragma once

#include <Halide.h>

namespace raw::pipeline {

inline auto DemosaicBilinear(Halide::Buffer<float> input, Halide::Var x, Halide::Var y, Halide::Var c,
                             Halide::Func fc) -> Halide::Func {
    Halide::Func demosaiced("demosaiced");
    Halide::Func clamped = Halide::BoundaryConditions::repeat_edge(input);

    Halide::Expr red, green, blue;

    auto color = fc(x, y);
    Halide::Expr at_red = color == 0;
    Halide::Expr at_green = (color == 1) || (color == 3);
    Halide::Expr at_blue = color == 2;

    // Red channel
    red = Halide::select(
        at_red, clamped(x, y),                                                             // At red pixel
        at_green,                                                                          // At green pixel
        Halide::select(fc(x + 1, y) == 0, (clamped(x - 1, y) + clamped(x + 1, y)) / 2.0f,  // In red-green row
                       (clamped(x, y - 1) + clamped(x, y + 1)) / 2.0f),                    // In blue-green row
        (clamped(x - 1, y - 1) + clamped(x + 1, y - 1) + clamped(x - 1, y + 1) + clamped(x + 1, y + 1)) /
            4.0f);  // At blue pixel

    // Green channel
    green = Halide::select(at_green, clamped(x, y),  // At green pixel
                           (clamped(x, y - 1) + clamped(x, y + 1) + clamped(x - 1, y) + clamped(x + 1, y)) /
                               4.0f);  // At red or blue pixel

    // Blue channel
    blue = Halide::select(
        at_blue, clamped(x, y),                                                            // At blue pixel
        at_green,                                                                          // At green pixel
        Halide::select(fc(x + 1, y) == 0, (clamped(x, y - 1) + clamped(x, y + 1)) / 2.0f,  // Green in red-green row
                       (clamped(x - 1, y) + clamped(x + 1, y)) / 2.0f),                    // Green in blue-green row
        (clamped(x - 1, y - 1) + clamped(x + 1, y - 1) + clamped(x - 1, y + 1) + clamped(x + 1, y + 1)) /
            4.0f);  // At red pixel

    // Combine channels into RGB output
    demosaiced(x, y, c) = Halide::select(c == 0, red, c == 1, green, blue);

    return demosaiced;
}

}  // namespace raw::pipeline