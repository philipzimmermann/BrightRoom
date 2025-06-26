#pragma once

#include <Halide.h>

namespace raw::pipeline {

inline auto WhiteLevel(Halide::Buffer<uint16_t> input, Halide::Var x, Halide::Var y, int white) -> Halide::Func {
    Halide::Func output("white_level");

    // Convert to float, divide by white level, and clamp between 0 and 1
    output(x, y) = Halide::clamp(Halide::cast<float>(input(x, y)) / static_cast<float>(white), 0.0f, 1.0f);

    return output;
}

}  // namespace raw::pipeline