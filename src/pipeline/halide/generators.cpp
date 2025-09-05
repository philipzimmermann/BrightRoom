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
    Input<Buffer<float, 1>> wb_factors{"wb_factors"};

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

        // White balance
        Func white_balanced = brightroom::WhiteBalance(demosaiced, x, y, c, wb_factors);

        RDom rdom(input.dim(0).min(), input.dim(0).extent(), input.dim(1).min(), input.dim(1).extent());

        auto max_red = Halide::maximum(white_balanced(rdom.x, rdom.y, 0));
        auto max_green = Halide::maximum(white_balanced(rdom.x, rdom.y, 1));
        auto max_blue = Halide::maximum(white_balanced(rdom.x, rdom.y, 2));
        auto min_channels = Halide::min(max_red, max_green, max_blue);

        Func clipped("clipped");
        clipped(x, y, c) = Halide::min(white_balanced(x, y, c), min_channels);

        output = clipped;

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
            wb_factors.set_estimates({{0, 3}});
            output.set_estimates({{0, 4000}, {0, 6000}, {0, 3}});
        }
    }
};

HALIDE_REGISTER_GENERATOR(PreprocessRawGenerator, preprocess_raw_generator)

class ProcessRawGenerator : public Halide::Generator<ProcessRawGenerator> {
   public:
    // Inputs
    Input<Buffer<float, 3>> input{"input"};

    Input<float> exposure{"exposure"};
    Input<Buffer<float, 2>> rgb_cam{"rgb_cam"};
    Input<float> contrast_factor{"contrast_factor"};
    Input<float> saturation_factor{"saturation_factor"};

    // Output
    Output<Buffer<uint8_t, 3>> output{"output"};  // Final RGB8 output

    // Intermediate stages
    Var x{"x"}, y{"y"}, c{"c"};
    Var yo{"yo"}, yi{"yi"};

    void generate() {
        // Exposure compensation
        Func exposure_adjusted = brightroom::Exposure(input, x, y, c, exposure);

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

        // Add saturation adjustment
        Func saturation_adjusted = brightroom::SaturationAdjustment(contrast_adjusted, x, y, c, saturation_factor);

        // Convert to RGB8 (modify to use saturation_adjusted instead of contrast_adjusted)
        output = brightroom::ToRgb8(saturation_adjusted, x, y, c);

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
            rgb_cam.set_estimates({{0, 3}, {0, 3}});
            exposure.set_estimate(3.0f);
            contrast_factor.set_estimate(1.5f);
            saturation_factor.set_estimate(1.0f);
            output.set_estimates({{0, 4000}, {0, 6000}, {0, 3}});
        } else {
            // Manual schedule similar to your original pipeline
            output.split(y, yo, yi, 32).parallel(yo).vectorize(x, 16);
            saturation_adjusted.store_at(output, yo).compute_at(output, yi).vectorize(x, 8);
            contrast_adjusted.store_at(output, yo).compute_at(output, yi).vectorize(x, 8);
            gamma_corrected.store_at(output, yo).compute_at(output, yi).vectorize(x, 8);
            srgb.store_at(output, yo).compute_at(output, yi).vectorize(x, 8);
            // tone_mapped.store_at(output, yo).compute_at(output, yi).vectorize(x, 8);
        }
    }
};

HALIDE_REGISTER_GENERATOR(ProcessRawGenerator, process_raw_generator)

class HistogramGenerator : public Halide::Generator<HistogramGenerator> {
   public:
    // Inputs
    Input<Buffer<uint8_t, 3>> input{"input"};

    // Output
    Output<Buffer<uint32_t, 1>> output{"output"};  // Histogram output

    // Intermediate stages
    Var x{"x"}, y{"y"}, c{"c"};
    Var yo{"yo"}, yi{"yi"};

    void generate() {
        // See tutorial https://halide-lang.org/tutorials/tutorial_lesson_09_update_definitions.html

        Func histogram("histogram");
        // Histogram buckets start as zero.
        histogram(x) = Halide::cast<uint32_t>(0);
        RDom r(0, input.width(), 0, input.height());
        Expr intensity =
            Halide::cast<uint8_t>(Halide::round(Halide::clamp(brightroom::Luminance(input, r.x, r.y), 0.0f, 255.0f)));
        histogram(intensity) += Halide::cast<uint32_t>(1);

        output = histogram;

        // For interleaved input
        input.dim(0).set_stride(3);
        input.dim(2).set_stride(1);
        input.dim(2).set_bounds(0, 3);  // Dimension 2 (c) starts at 0 and has extent 3.

        // Schedule
        constexpr bool kAutoSchedule = true;
        if (kAutoSchedule) {
            // Let the autoscheduler handle it
            input.set_estimates({{0, 4000}, {0, 6000}, {0, 3}});
            output.set_estimates({{0, 256}});
        }
    }
};

HALIDE_REGISTER_GENERATOR(HistogramGenerator, histogram_generator)
