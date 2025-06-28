#pragma once

#include <Halide.h>

namespace raw::pipeline {

// Apply normalized white balance factors to the input image
inline auto WhiteBalance(Halide::Func input, Halide::Var x, Halide::Var y, Halide::Var c,
                         std::array<float, 3> wb_factors) -> Halide::Func {
    Halide::Func white_balanced("white_balanced");
    white_balanced(x, y, c) = input(x, y, c) * Halide::select(c == 0, wb_factors[0],  // Red channel
                                                              c == 1, wb_factors[1],  // Green channel
                                                              wb_factors[2]           // Blue channel
                                               );
    return white_balanced;
}

}  // namespace raw::pipeline