#include <Halide.h>
#include <array>
#include "halide_pipeline_gen.h"

class RawPipelineGenerator : public Halide::Generator<RawPipelineGenerator> {
   public:
    // Inputs
    Input<Buffer<uint16_t, 2>> input{"input"};         // Raw Bayer input
    Input<int> filters{"filters"};                     // Bayer pattern
    Input<int> black_level{"black_level"};             // Global black level
    Input<Buffer<int, 1>> cblack{"cblack"};            // Per-channel black levels
    Input<int> white_input{"white_input"};             // White level
    Input<Buffer<float, 1>> wb_factors{"wb_factors"};  // White balance factors
    Input<float> exposure{"exposure"};                 // Exposure compensation
    Input<Buffer<float, 2>> rgb_cam{"rgb_cam"};        // Color space conversion matrix
    Input<float> contrast_factor{"contrast_factor"};   // Contrast adjustment factor

    // Output
    Output<Buffer<uint8_t, 3>> output{"output"};  // Final RGB8 output

    // Intermediate stages
    Var x{"x"}, y{"y"}, c{"c"};
    Var yo{"yo"}, yi{"yi"};

    void generate() {
        // Create the Bayer pattern function
        Halide::Func input_boundary = Halide::BoundaryConditions::repeat_edge(input);
        Func fc = halide_gen::FC(x, y, filters);

        // Black level subtraction
        Func black_adjusted = halide_gen::BlackLevel(input_boundary, x, y, fc, black_level, cblack);

        // White level normalization
        Func white_adjusted = halide_gen::WhiteLevel(black_adjusted, x, y, white_input);

        // Demosaic
        Func demosaiced = halide_gen::DemosaicBilinear(white_adjusted, x, y, c, fc);

        // White balance
        Func white_balanced = halide_gen::WhiteBalance(demosaiced, x, y, c, wb_factors);

        // Exposure compensation
        Func exposure_adjusted = halide_gen::Exposure(white_balanced, x, y, c, exposure);

        // Tone mapping
        // Func log_sum = halide_gen::LogSum(exposure_adjusted, x, y, input.width(), input.height());
        // log_sum.compute_root();

        // Expr log_avg = Halide::exp(log_sum() / Halide::cast<float>(input.width() * input.height()));

        // auto tone_mapped = halide_gen::ToneMapping(exposure_adjusted, x, y, c, log_avg, 0.18f);

        // Color space conversion
        Func srgb = halide_gen::ColorSpaceConversion(exposure_adjusted, x, y, c, rgb_cam);

        // Gamma correction
        Func gamma_corrected = halide_gen::GammaCorrection(srgb, x, y, c);

        // Contrast adjustment
        Func contrast_adjusted = halide_gen::ContrastAdjustment(gamma_corrected, x, y, c, contrast_factor);

        // Convert to RGB8
        output = halide_gen::ToRgb8(contrast_adjusted, x, y, c);

        // Schedule
        bool auto_schedule = false;
        if (auto_schedule) {
            // Let the autoscheduler handle it
            input.set_estimates({{0, 4000}, {0, 6000}});
            filters.set_estimate(0);
            black_level.set_estimate(0);
            white_input.set_estimate(0);
            exposure.set_estimate(3.0f);
            contrast_factor.set_estimate(1.5f);
            output.set_estimates({{0, 4000}, {0, 6000}, {0, 3}});
        } else {
            // Manual schedule similar to your original pipeline
            output.split(y, yo, yi, 32).parallel(yo).vectorize(x, 16);
            contrast_adjusted.store_at(output, yo).compute_at(output, yi).vectorize(x, 8);
            gamma_corrected.store_at(output, yo).compute_at(output, yi).vectorize(x, 8);
            srgb.store_at(output, yo).compute_at(output, yi).vectorize(x, 8);
            // tone_mapped.store_at(output, yo).compute_at(output, yi).vectorize(x, 8);
            demosaiced.compute_root().vectorize(x, 8);
            white_adjusted.compute_root().split(y, yo, yi, 32).parallel(yo).vectorize(x, 16);
            black_adjusted.compute_root().split(y, yo, yi, 32).parallel(yo).vectorize(x, 16);
        }
    }
};

HALIDE_REGISTER_GENERATOR(RawPipelineGenerator, raw_pipeline_generator)
