#include <Halide.h>
#include "functions.h"

class PreprocessRawGenerator : public Halide::Generator<PreprocessRawGenerator> {
   public:
    // Inputs
    Input<Buffer<uint16_t, 2>> input{"input"};  // Raw Bayer input
    Input<int> filters{"filters"};              // Bayer pattern
    Input<int> black_level{"black_level"};      // Global black level
    Input<Buffer<int, 1>> cblack{"cblack"};     // Per-channel black levels
    Input<int> white_input{"white_input"};      // White level

    // Output
    Output<Buffer<float, 3>> output{"output"};  // Intermediate output

    // Intermediate stages
    Var x{"x"}, y{"y"}, c{"c"};
    Var yo{"yo"}, yi{"yi"};

    void generate() {
        // Create the Bayer pattern function
        Halide::Func input_boundary = Halide::BoundaryConditions::repeat_edge(input);
        Func fc = brightroom::FC(x, y, filters);

        // Black level subtraction
        Func black_adjusted = brightroom::BlackLevel(input_boundary, x, y, fc, black_level, cblack);

        // White level normalization
        Func white_adjusted = brightroom::WhiteLevel(black_adjusted, x, y, white_input);

        // Demosaic
        Func demosaiced = brightroom::DemosaicBilinear(white_adjusted, x, y, c, fc);

        output = demosaiced;

        // For interleaved output
        output.dim(0).set_stride(3);
        output.dim(2).set_stride(1);

        constexpr bool kAutoSchedule = true;
        if (kAutoSchedule) {
            // Let the autoscheduler handle it
            input.set_estimates({{0, 4000}, {0, 6000}});
            filters.set_estimate(0);
            black_level.set_estimate(0);
            cblack.set_estimates({{0, 4}});
            white_input.set_estimate(0);
            output.set_estimates({{0, 4000}, {0, 6000}, {0, 3}});
        }
    }
};

HALIDE_REGISTER_GENERATOR(PreprocessRawGenerator, preprocess_raw_generator)

class ProcessRawGenerator : public Halide::Generator<ProcessRawGenerator> {
   public:
    // Inputs
    Input<Buffer<float, 3>> input{"input"};            // Already demosaiced input
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

        // White balance
        Func white_balanced = brightroom::WhiteBalance(input, x, y, c, wb_factors);

        // Exposure compensation
        Func exposure_adjusted = brightroom::Exposure(white_balanced, x, y, c, exposure);

        // Tone mapping
        // Func log_sum = brightroom::LogSum(exposure_adjusted, x, y, input.width(), input.height());
        // log_sum.compute_root();

        // Expr log_avg = Halide::exp(log_sum() / Halide::cast<float>(input.width() * input.height()));

        // auto tone_mapped = brightroom::ToneMapping(exposure_adjusted, x, y, c, log_avg, 0.18f);

        // Color space conversion
        Func srgb = brightroom::ColorSpaceConversion(exposure_adjusted, x, y, c, rgb_cam);

        // Gamma correction
        Func gamma_corrected = brightroom::GammaCorrection(srgb, x, y, c);

        // Contrast adjustment
        Func contrast_adjusted = brightroom::ContrastAdjustment(gamma_corrected, x, y, c, contrast_factor);

        // Convert to RGB8
        output = brightroom::ToRgb8(contrast_adjusted, x, y, c);

        // For interleaved output
        input.dim(0).set_stride(3);
        input.dim(2).set_stride(1);

        output.dim(0).set_stride(3);
        output.dim(2).set_stride(1);

        input.dim(2).set_bounds(0, 3);  // Dimension 2 (c) starts at 0 and has extent 3.
        output.dim(2).set_bounds(0, 3);

        output.reorder(c, x, y).unroll(c);

        // Schedule
        constexpr bool kAutoSchedule = true;
        if (kAutoSchedule) {
            // Let the autoscheduler handle it
            input.set_estimates({{0, 4000}, {0, 6000}, {0, 3}});
            wb_factors.set_estimates({{0, 3}});
            rgb_cam.set_estimates({{0, 3}, {0, 3}});
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
        }
    }
};

HALIDE_REGISTER_GENERATOR(ProcessRawGenerator, process_raw_generator)
