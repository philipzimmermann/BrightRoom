#pragma once

#include <Halide.h>

namespace raw::pipeline {

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

}  // namespace raw::pipeline