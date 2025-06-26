#pragma once

#include <Halide.h>
#include <cstdint>

namespace raw::pipeline {

inline auto FC(Halide::Var x, Halide::Var y, int filters) -> Halide::Func {
    Halide::Func fc("fc");
    fc(x, y) = Halide::cast<uint16_t>(filters >> (((y << 1 & 14) | (x & 1)) << 1) & 3);
    return fc;
}

}  // namespace raw::pipeline