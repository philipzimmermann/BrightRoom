#include "HalideRawPipeline.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include "libraw/libraw.h"
#include "preprocess_raw_generator.h"
#include "process_raw_generator.h"
#include "types.h"

namespace brightroom {
using Clock = std::chrono::steady_clock;
using Duration = std::chrono::milliseconds;

void HalideRawPipeline::Preprocess(LibRaw& raw_data) {
    auto total_start = Clock::now();
    auto step_start = Clock::now();

    // Create input buffers for the generator
    Halide::Runtime::Buffer<uint16_t> input_buffer(raw_data.imgdata.rawdata.raw_image, raw_data.imgdata.sizes.raw_width,
                                                   raw_data.imgdata.sizes.raw_height);

    // Create buffer for cblack values
    Halide::Runtime::Buffer<int> cblack_buffer(4);
    for (int i = 0; i < 4; i++) {
        cblack_buffer(i) = raw_data.imgdata.color.cblack[i];
    }

    // Create buffer for white balance factors
    Halide::Runtime::Buffer<float> wb_factors(3);
    float wb_r = raw_data.imgdata.color.cam_mul[0];
    float wb_g = raw_data.imgdata.color.cam_mul[1];
    float wb_b = raw_data.imgdata.color.cam_mul[2];
    float max_wb = std::max({wb_r, wb_g, wb_b});
    wb_factors(0) = wb_r / max_wb;
    wb_factors(1) = wb_g / max_wb;
    wb_factors(2) = wb_b / max_wb;

    // Create buffer for color space conversion matrix
    Halide::Runtime::Buffer<float> rgb_cam_buffer(3, 3);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            rgb_cam_buffer(i, j) = raw_data.imgdata.color.rgb_cam[i][j];
        }
    }

    auto demosaiced_buffer = Halide::Runtime::Buffer<float>::make_interleaved(raw_data.imgdata.sizes.raw_width,
                                                                              raw_data.imgdata.sizes.raw_height, 3);
    std::cout << "Running preprocess..." << "\n";
    step_start = Clock::now();

    // Call preprocess with all parameters
    auto error = preprocess_raw_generator(input_buffer.raw_buffer(),                         // Raw Bayer input
                                          static_cast<int>(raw_data.imgdata.idata.filters),  // Bayer pattern
                                          static_cast<int>(raw_data.imgdata.color.black),    // Global black level
                                          cblack_buffer.raw_buffer(),                        // Per-channel black levels
                                          static_cast<int>(raw_data.imgdata.color.maximum),  // White level
                                          demosaiced_buffer.raw_buffer());
    if (error != 0) {
        std::cout << "Preprocess error: " << error << "\n";
    }
    std::cout << "Preprocess time: " << std::chrono::duration_cast<Duration>(Clock::now() - step_start).count() << " ms"
              << "\n";

    _demosaiced_buffer = std::move(demosaiced_buffer);

    step_start = Clock::now();
    // Allocate vector for final image output
    _rgb8_vector.resize(raw_data.imgdata.sizes.raw_width * raw_data.imgdata.sizes.raw_height * 3);
    // Wrap the vector as a interleaved Halide buffer
    _rgb8_buffer = Halide::Runtime::Buffer<uint8_t>::make_interleaved(
        _rgb8_vector.data(), raw_data.imgdata.sizes.raw_width, raw_data.imgdata.sizes.raw_height, 3);
    std::cout << "RGB8 vector time: " << std::chrono::duration_cast<Duration>(Clock::now() - step_start).count()
              << " ms" << "\n";
    std::cout << "Total Preprocess time: " << std::chrono::duration_cast<Duration>(Clock::now() - total_start).count()
              << " ms" << "\n";
}

auto HalideRawPipeline::Process(LibRaw& raw_data, const Parameters& parameters) -> RgbImage {
    auto total_start = Clock::now();
    auto step_start = Clock::now();

    // Create buffer for white balance factors
    Halide::Runtime::Buffer<float> wb_factors(3);
    float wb_r = raw_data.imgdata.color.cam_mul[0];
    float wb_g = raw_data.imgdata.color.cam_mul[1];
    float wb_b = raw_data.imgdata.color.cam_mul[2];
    float max_wb = std::max({wb_r, wb_g, wb_b});
    wb_factors(0) = wb_r / max_wb;
    wb_factors(1) = wb_g / max_wb;
    wb_factors(2) = wb_b / max_wb;

    // Create buffer for color space conversion matrix
    Halide::Runtime::Buffer<float> rgb_cam_buffer(3, 3);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            rgb_cam_buffer(i, j) = raw_data.imgdata.color.rgb_cam[i][j];
        }
    }

    std::cout << "Running process..." << "\n";
    step_start = Clock::now();

    // Call the generator with all parameters
    auto error = process_raw_generator(_demosaiced_buffer.raw_buffer(),  // Raw Bayer input
                                       wb_factors.raw_buffer(),          // White balance factors
                                       parameters.exposure * 3.0f,       // Exposure compensation
                                       rgb_cam_buffer.raw_buffer(),      // Color space conversion matrix
                                       parameters.contrast * 1.5f,       // Contrast factor
                                       parameters.saturation * 1.0f,     // Saturation factor
                                       _rgb8_buffer.raw_buffer());
    if (error != 0) {
        std::cout << "Process error: " << error << "\n";
    }
    std::cout << "Process time: " << std::chrono::duration_cast<Duration>(Clock::now() - step_start).count() << " ms"
              << "\n";

    std::cout << "Total process time: " << std::chrono::duration_cast<Duration>(Clock::now() - total_start).count()
              << " ms" << "\n";

    return {_rgb8_vector, raw_data.imgdata.sizes.raw_width, raw_data.imgdata.sizes.raw_height};
}
}  // namespace brightroom