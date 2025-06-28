#pragma once

#include <Halide.h>

namespace raw::pipeline {

inline auto LogSum(Halide::Func input, Halide::Var x, Halide::Var y, int width, int height) -> Halide::Func {
    Halide::Func luminance("luminance");
    luminance(x, y) = 0.2126f * input(x, y, 0) + 0.7152f * input(x, y, 1) + 0.0722f * input(x, y, 2);

    Halide::Func log_luminance("log_luminance");
    log_luminance(x, y) = Halide::log(Halide::max(luminance(x, y), 1e-6f));

    // Schedule
    luminance.compute_root().parallel(y).vectorize(x, 8);
    log_luminance.compute_root().parallel(y).vectorize(x, 8);
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

    luminance.compute_root().parallel(y).vectorize(x, 8);
    tone_mapped.compute_root().parallel(y).vectorize(x, 8);

    return tone_mapped;
}

}  // namespace raw::pipeline