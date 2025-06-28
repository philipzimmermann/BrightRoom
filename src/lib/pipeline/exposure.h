#pragma once

#include <Halide.h>

namespace raw::pipeline {

inline auto Exposure(Halide::Func input, Halide::Var x, Halide::Var y, Halide::Var c, float exposure) -> Halide::Func {
    Halide::Func exposure_adjusted("exposure_adjusted");
    exposure_adjusted(x, y, c) = input(x, y, c) * exposure;
    return exposure_adjusted;
}

}  // namespace raw::pipeline