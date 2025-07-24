#include "pipeline.h"
#include <Halide.h>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include "classic_pipeline.h"
#include "raw_pipeline_generator.h"
#include "types.h"

namespace raw {

namespace {

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

auto HalidePipelineGen(LibRaw& rawProcessor, const Parameters& parameters) -> RgbImage {
    using Clock = std::chrono::steady_clock;
    using Duration = std::chrono::milliseconds;
    auto total_start = Clock::now();
    auto step_start = Clock::now();

    // Create input buffers for the generator
    Halide::Buffer<uint16_t> input_buffer(rawProcessor.imgdata.rawdata.raw_image, rawProcessor.imgdata.sizes.raw_width,
                                          rawProcessor.imgdata.sizes.raw_height);

    // Create buffer for cblack values
    Halide::Buffer<int> cblack_buffer(4);
    for (int i = 0; i < 4; i++) {
        cblack_buffer(i) = rawProcessor.imgdata.color.cblack[i];
    }

    // Create buffer for white balance factors
    Halide::Buffer<float> wb_factors(3);
    float wb_r = rawProcessor.imgdata.color.cam_mul[0];
    float wb_g = rawProcessor.imgdata.color.cam_mul[1];
    float wb_b = rawProcessor.imgdata.color.cam_mul[2];
    float max_wb = std::max({wb_r, wb_g, wb_b});
    wb_factors(0) = wb_r / max_wb;
    wb_factors(1) = wb_g / max_wb;
    wb_factors(2) = wb_b / max_wb;

    // Create buffer for color space conversion matrix
    Halide::Buffer<float> rgb_cam_buffer(3, 3);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            rgb_cam_buffer(i, j) = rawProcessor.imgdata.color.rgb_cam[i][j];
        }
    }

    Halide::Buffer<uint8_t> rgb8_buffer(rawProcessor.imgdata.sizes.raw_width, rawProcessor.imgdata.sizes.raw_height, 3);
    // auto rgb8_buffer = Halide::Buffer<uint8_t>::make_interleaved(rawProcessor.imgdata.sizes.raw_width,
    //                                                              rawProcessor.imgdata.sizes.raw_height, 3);
    std::cout << "Running generator..." << std::endl;

    // Call the generator with all parameters
    auto error = raw_pipeline_generator(input_buffer.raw_buffer(),                             // Raw Bayer input
                                        static_cast<int>(rawProcessor.imgdata.idata.filters),  // Bayer pattern
                                        static_cast<int>(rawProcessor.imgdata.color.black),    // Global black level
                                        cblack_buffer.raw_buffer(),  // Per-channel black levels
                                        static_cast<int>(rawProcessor.imgdata.color.maximum),  // White level
                                        wb_factors.raw_buffer(),                               // White balance factors
                                        parameters.exposure,                                   // Exposure compensation
                                        rgb_cam_buffer.raw_buffer(),  // Color space conversion matrix
                                        1.5f,                         // Contrast factor
                                        rgb8_buffer.raw_buffer());
    std::cout << "Generator error: " << error << std::endl;
    std::cout << "Generator time: " << std::chrono::duration_cast<Duration>(Clock::now() - step_start).count() << " ms"
              << std::endl;

    // Copy the result to a vector
    step_start = Clock::now();
    std::vector<uint8_t> rgb8_vector(rawProcessor.imgdata.sizes.raw_width * rawProcessor.imgdata.sizes.raw_height * 3);

    for (int y = 0; y < rawProcessor.imgdata.sizes.raw_height; y++) {
        for (int x = 0; x < rawProcessor.imgdata.sizes.raw_width; x++) {
            for (int c = 0; c < 3; c++) {
                rgb8_vector[y * rawProcessor.imgdata.sizes.raw_width * 3 + x * 3 + c] = rgb8_buffer(x, y, c);
            }
        }
    }
    // memcpy(rgb8_vector.data(), rgb8_buffer.data(), rgb8_vector.size() * sizeof(uint8_t));
    std::cout << "Copy time: " << std::chrono::duration_cast<Duration>(Clock::now() - step_start).count() << " ms"
              << std::endl;

    std::cout << "Total pipeline time: " << std::chrono::duration_cast<Duration>(Clock::now() - total_start).count()
              << " ms" << std::endl;

    return {rgb8_vector, rawProcessor.imgdata.sizes.raw_width, rawProcessor.imgdata.sizes.raw_height};
}
}  // namespace

auto Pipeline::Run(LibRaw& rawProcessor, const Parameters& parameters) const -> RgbImage {
    return HalidePipelineGen(rawProcessor, parameters);
}
}  // namespace raw