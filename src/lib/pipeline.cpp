#include "pipeline.h"
#include <Halide.h>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include "classic_pipeline.h"
#include "halide_pipeline.h"
#include "types.h"

namespace raw {

auto ClassicPipeline(LibRaw& rawProcessor) -> RgbImage {
    using Clock = std::chrono::steady_clock;
    using Duration = std::chrono::milliseconds;

    auto total_start = Clock::now();
    auto step_start = Clock::now();

    // Black level subtraction
    for (int row = 0; row < rawProcessor.imgdata.sizes.raw_height; row++) {
        for (int col = 0; col < rawProcessor.imgdata.sizes.raw_width; col++) {
            int color = rawProcessor.FC(row, col);
            uint16_t* pixel =
                &rawProcessor.imgdata.rawdata.raw_image[row * rawProcessor.imgdata.sizes.raw_pitch / 2 + col];
            auto black = rawProcessor.imgdata.color.black + rawProcessor.imgdata.color.cblack[color];
            *pixel = std::max(0, *pixel - static_cast<uint16_t>(black));
        }
    }
    auto step_end = Clock::now();
    std::cout << "Black level subtraction: " << std::chrono::duration_cast<Duration>(step_end - step_start).count()
              << " ms" << std::endl;

    // White Level
    step_start = Clock::now();
    auto white_level = static_cast<float>(rawProcessor.imgdata.color.maximum);
    std::vector<float> normalized_bayer(rawProcessor.imgdata.sizes.raw_width * rawProcessor.imgdata.sizes.raw_height,
                                        0);
    for (int row = 0; row < rawProcessor.imgdata.sizes.raw_height; row++) {
        for (int col = 0; col < rawProcessor.imgdata.sizes.raw_width; col++) {
            uint16_t* pixel =
                &rawProcessor.imgdata.rawdata.raw_image[row * rawProcessor.imgdata.sizes.raw_pitch / 2 + col];
            float normalized = static_cast<float>(*pixel) / white_level;
            normalized = std::clamp(normalized, 0.0F, 1.0F);
            normalized_bayer[row * rawProcessor.imgdata.sizes.raw_width + col] = normalized;
        }
    }
    step_end = Clock::now();
    std::cout << "White level: " << std::chrono::duration_cast<Duration>(step_end - step_start).count() << " ms"
              << std::endl;

    // Demosaic
    step_start = Clock::now();
    auto rgb = classic::Demosaic(normalized_bayer, rawProcessor, rawProcessor.imgdata.sizes.raw_width,
                                 rawProcessor.imgdata.sizes.raw_height);
    step_end = Clock::now();
    std::cout << "Demosaicing: " << std::chrono::duration_cast<Duration>(step_end - step_start).count() << " ms"
              << std::endl;

    // White balance
    step_start = Clock::now();
    rgb = classic::WhiteBalance(rgb, rawProcessor);
    step_end = Clock::now();
    std::cout << "White balance: " << std::chrono::duration_cast<Duration>(step_end - step_start).count() << " ms"
              << std::endl;

    // Exposure compensation
    step_start = Clock::now();
    float exposure = 3.0f;
    rgb = classic::ExposureCompensation(rgb, exposure);
    step_end = Clock::now();
    std::cout << "Exposure compensation: " << std::chrono::duration_cast<Duration>(step_end - step_start).count()
              << " ms" << std::endl;

    // Tone mapping
    step_start = Clock::now();
    rgb = classic::ToneMapping(rgb);
    step_end = Clock::now();
    std::cout << "Tone mapping: " << std::chrono::duration_cast<Duration>(step_end - step_start).count() << " ms"
              << std::endl;

    // sRGB
    step_start = Clock::now();
    rgb = classic::ColorSpaceConversion(rgb, rawProcessor);
    step_end = Clock::now();
    std::cout << "Color space conversion: " << std::chrono::duration_cast<Duration>(step_end - step_start).count()
              << " ms" << std::endl;

    // Gamma Correction
    step_start = Clock::now();
    rgb = classic::GammaCorrection(rgb);
    step_end = Clock::now();
    std::cout << "Gamma correction: " << std::chrono::duration_cast<Duration>(step_end - step_start).count() << " ms"
              << std::endl;

    // Contrast Adjustment
    step_start = Clock::now();
    float contrast_factor = 1.5f;
    rgb = classic::ContrastAdjustment(rgb, contrast_factor);
    step_end = Clock::now();
    std::cout << "Contrast adjustment: " << std::chrono::duration_cast<Duration>(step_end - step_start).count() << " ms"
              << std::endl;

    // ToRgb8
    step_start = Clock::now();
    auto rgb8 = classic::ToRgb8(rgb);
    step_end = Clock::now();
    std::cout << "RGB8 conversion: " << std::chrono::duration_cast<Duration>(step_end - step_start).count() << " ms"
              << std::endl;

    auto total_end = Clock::now();
    std::cout << "Total pipeline time: " << std::chrono::duration_cast<Duration>(total_end - total_start).count()
              << " ms" << std::endl;

    return {rgb8, rawProcessor.imgdata.sizes.raw_width, rawProcessor.imgdata.sizes.raw_height};
}

auto HalidePipeline(LibRaw& rawProcessor) -> RgbImage {
    using Clock = std::chrono::steady_clock;
    using Duration = std::chrono::milliseconds;

    auto total_start = Clock::now();
    try {

        Halide::Var x("x"), y("y");

        int filters = rawProcessor.imgdata.idata.filters;
        Halide::Buffer<uint16_t> input_buffer(rawProcessor.imgdata.rawdata.raw_image,
                                              rawProcessor.imgdata.sizes.raw_width,
                                              rawProcessor.imgdata.sizes.raw_height);
        Halide::Func input = Halide::BoundaryConditions::repeat_edge(input_buffer);

        auto fc = halide::FC(x, y, filters);

        // Black level
        std::array<int, 4> cblack = {
            static_cast<int>(rawProcessor.imgdata.color.cblack[0]),
            static_cast<int>(rawProcessor.imgdata.color.cblack[1]),
            static_cast<int>(rawProcessor.imgdata.color.cblack[2]),
            static_cast<int>(rawProcessor.imgdata.color.cblack[3]),
        };

        auto black_adjusted =
            halide::BlackLevel(input, x, y, fc, static_cast<int>(rawProcessor.imgdata.color.black), cblack);

        // White Level
        auto white_adjusted =
            halide::WhiteLevel(black_adjusted, x, y, static_cast<int>(rawProcessor.imgdata.color.maximum));

        // Demosaic
        Halide::Var c("c");

        auto demosaiced = halide::DemosaicBilinear(white_adjusted, x, y, c, fc);

        // White balance
        float wb_r = rawProcessor.imgdata.color.cam_mul[0];
        float wb_g = rawProcessor.imgdata.color.cam_mul[1];
        float wb_b = rawProcessor.imgdata.color.cam_mul[2];
        auto max = std::max({wb_r, wb_g, wb_b});
        wb_r /= max;
        wb_g /= max;
        wb_b /= max;
        auto white_balanced = halide::WhiteBalance(demosaiced, x, y, c, {wb_r, wb_g, wb_b});

        // Exposure compensation
        float exposure = 3.0f;
        auto exposure_adjusted = halide::Exposure(white_balanced, x, y, c, exposure);

        // Tone mapping
        auto log_sum = halide::LogSum(exposure_adjusted, x, y, rawProcessor.imgdata.sizes.raw_width,
                                      rawProcessor.imgdata.sizes.raw_height);

        log_sum.compile_jit();
        std::cout << "\nLog sum: " << std::endl;
        auto step_start = Clock::now();
        Halide::Buffer<float> log_sum_buf = log_sum.realize();
        std::cout << "Log sum realize took: " << std::chrono::duration_cast<Duration>(Clock::now() - step_start).count()
                  << " ms" << std::endl;
        log_sum.print_loop_nest();
        std::cout << std::endl;

        float log_avg = std::exp(log_sum_buf(0) / static_cast<float>(rawProcessor.imgdata.sizes.raw_width *
                                                                     rawProcessor.imgdata.sizes.raw_height));

        auto tone_mapped = halide::ToneMapping(exposure_adjusted, x, y, c, log_avg, 0.18f);

        // sRGB
        const std::array<std::array<float, 3>, 3> rgb_cam = {
            {{rawProcessor.imgdata.color.rgb_cam[0][0], rawProcessor.imgdata.color.rgb_cam[0][1],
              rawProcessor.imgdata.color.rgb_cam[0][2]},
             {rawProcessor.imgdata.color.rgb_cam[1][0], rawProcessor.imgdata.color.rgb_cam[1][1],
              rawProcessor.imgdata.color.rgb_cam[1][2]},
             {rawProcessor.imgdata.color.rgb_cam[2][0], rawProcessor.imgdata.color.rgb_cam[2][1],
              rawProcessor.imgdata.color.rgb_cam[2][2]}}};
        auto srgb = halide::ColorSpaceConversion(tone_mapped, x, y, c, rgb_cam);

        // Gamma Correction
        auto gamma_corrected = halide::GammaCorrection(srgb, x, y, c);

        // Contrast Adjustment
        float contrast_factor = 1.5f;
        auto contrast_adjusted = halide::ContrastAdjustment(gamma_corrected, x, y, c, contrast_factor);

        // ToRgb8
        auto rgb8 = halide::ToRgb8(contrast_adjusted, x, y, c);

        // fc.compute_root().parallel(y);
        // black_adjusted.compute_root().parallel(y);
        // white_adjusted.compute_root().parallel(y);
        // demosaiced.compute_root().parallel(y).vectorize(x, 8);
        // white_balanced.compute_root().parallel(y);
        // exposure_adjusted.compute_root().parallel(y).vectorize(x, 8);
        // tone_mapped.compute_root().parallel(y).vectorize(x, 8);
        // srgb.compute_root().parallel(y).vectorize(x, 8);
        // gamma_corrected.compute_root().parallel(y).vectorize(x, 8);
        // contrast_adjusted.compute_root().parallel(y).vectorize(x, 8);
        // rgb8.compute_root().parallel(y).vectorize(x, 8);

        rgb8.compile_jit();

        step_start = Clock::now();

        Halide::Buffer<uint8_t> rgb8_image =
            rgb8.realize({rawProcessor.imgdata.sizes.raw_width, rawProcessor.imgdata.sizes.raw_height, 3});
        std::vector<uint8_t> rgb8_vector(rawProcessor.imgdata.sizes.raw_width * rawProcessor.imgdata.sizes.raw_height *
                                         3);
        // memcpy(rgb8_vector.data(), rgb8_image.data(), rgb8_vector.size() * sizeof(uint8_t));
        for (int x = 0; x < rawProcessor.imgdata.sizes.raw_width; x++) {
            for (int y = 0; y < rawProcessor.imgdata.sizes.raw_height; y++) {
                for (int c = 0; c < 3; c++) {
                    rgb8_vector[y * rawProcessor.imgdata.sizes.raw_width * 3 + x * 3 + c] = rgb8_image(x, y, c);
                }
            }
        }
        auto step_end = Clock::now();
        std::cout << "Rgb8: \n";
        std::cout << "Realize time: " << std::chrono::duration_cast<Duration>(step_end - step_start).count() << " ms"
                  << std::endl;

        std::cout << "Pseudo-code for the schedule:\n";
        rgb8.print_loop_nest();
        std::cout << std::endl;

        std::cout << "Total pipeline time: " << std::chrono::duration_cast<Duration>(Clock::now() - total_start).count()
                  << " ms" << std::endl;

        return {rgb8_vector, rawProcessor.imgdata.sizes.raw_width, rawProcessor.imgdata.sizes.raw_height};
    } catch (const Halide::Error& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
    return {raw::RGB8_Data(0), 0, 0};
}

auto Pipeline::Run(LibRaw& rawProcessor) const -> RgbImage {
    return HalidePipeline(rawProcessor);
}
}  // namespace raw