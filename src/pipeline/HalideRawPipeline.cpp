#include "HalideRawPipeline.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include "downscale_generator.h"
#include "histogram_generator.h"
#include "libraw/libraw.h"
#include "preprocess_raw_generator.h"
#include "process_raw_generator.h"
#include "types.h"

namespace brightroom {
using Clock = std::chrono::steady_clock;
using Duration = std::chrono::milliseconds;

namespace {
auto ScaleParameters(const Parameters& parameters) -> Parameters {
    // Frontend has range -3 to 3 for all parameters where 0 means no change.
    // We need to scale them for the halide pipeline.

    Parameters scaled_parameters;
    scaled_parameters.exposure = std::pow(2.0F, parameters.exposure) * 3.0F;
    scaled_parameters.contrast = std::pow(1.5F, parameters.contrast) * 1.5F;
    scaled_parameters.saturation = std::pow(2.0F, parameters.saturation);
    return scaled_parameters;
}
}  // namespace

void HalideRawPipeline::Preprocess(LibRaw& raw_data) {
    auto total_start = Clock::now();
    auto step_start = Clock::now();

    // Create input buffers for the generator
    Halide::Runtime::Buffer<uint16_t> input_buffer(raw_data.imgdata.rawdata.raw_image, raw_data.imgdata.sizes.raw_width,
                                                   raw_data.imgdata.sizes.raw_height);

    _width = raw_data.imgdata.sizes.raw_width;
    _height = raw_data.imgdata.sizes.raw_height;

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
    std::cout << "WB factors: " << wb_factors(0) << ", " << wb_factors(1) << ", " << wb_factors(2) << "\n";

    // Create buffer for color space conversion matrix
    Halide::Runtime::Buffer<float> rgb_cam_buffer(3, 3);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            rgb_cam_buffer(i, j) = raw_data.imgdata.color.rgb_cam[i][j];
        }
    }

    auto preprocessed_buffer = Halide::Runtime::Buffer<float>::make_interleaved(_width, _height, 3);
    std::cout << "Running preprocess..." << "\n";
    step_start = Clock::now();

    // Call preprocess with all parameters
    auto error = preprocess_raw_generator(input_buffer.raw_buffer(),                         // Raw Bayer input
                                          static_cast<int>(raw_data.imgdata.idata.filters),  // Bayer pattern
                                          static_cast<int>(raw_data.imgdata.color.black),    // Global black level
                                          cblack_buffer.raw_buffer(),                        // Per-channel black levels
                                          static_cast<int>(raw_data.imgdata.color.maximum),  // White level
                                          wb_factors.raw_buffer(),                           // White balance factors
                                          preprocessed_buffer.raw_buffer());
    if (error != 0) {
        std::cout << "Preprocess error: " << error << "\n";
    }
    std::cout << "Preprocess time: " << std::chrono::duration_cast<Duration>(Clock::now() - step_start).count() << " ms"
              << "\n";

    _preprocessed_buffer = std::move(preprocessed_buffer);

    step_start = Clock::now();
    // Allocate vector and Halide buffer for the final image output
    _rgb8_vector.resize(_width * _height * 3);
    _rgb8_buffer = Halide::Runtime::Buffer<uint8_t>::make_interleaved(_rgb8_vector.data(), _width, _height, 3);

    // Prepare second vector and Halide buffer for the downscaled image
    _downscaled_vector.resize(_width / _downscale_factor * _height / _downscale_factor * 3);
    _downscaled_buffer = Halide::Runtime::Buffer<uint8_t>::make_interleaved(
        _downscaled_vector.data(), _width / _downscale_factor, _height / _downscale_factor, 3);

    std::cout << "RGB8 vector time: " << std::chrono::duration_cast<Duration>(Clock::now() - step_start).count()
              << " ms" << "\n";
    std::cout << "Total Preprocess time: " << std::chrono::duration_cast<Duration>(Clock::now() - total_start).count()
              << " ms" << "\n";
}

auto HalideRawPipeline::Process(LibRaw& raw_data, const Parameters& parameters) -> RgbImage {
    auto total_start = Clock::now();
    auto step_start = Clock::now();

    // Create buffer for color space conversion matrix
    Halide::Runtime::Buffer<float> rgb_cam_buffer(3, 3);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            rgb_cam_buffer(i, j) = raw_data.imgdata.color.rgb_cam[i][j];
        }
    }

    std::cout << "Running process..." << "\n";
    step_start = Clock::now();

    Parameters scaled_parameters = ScaleParameters(parameters);

    // Call the generator with all parameters
    auto error = process_raw_generator(_preprocessed_buffer.raw_buffer(), scaled_parameters.exposure,
                                       rgb_cam_buffer.raw_buffer(), scaled_parameters.contrast,
                                       scaled_parameters.saturation, _rgb8_buffer.raw_buffer());
    if (error != 0) {
        std::cout << "Process error: " << error << "\n";
    }
    std::cout << "Process time: " << std::chrono::duration_cast<Duration>(Clock::now() - step_start).count() << " ms"
              << "\n";
    step_start = Clock::now();
    error = downscale_generator(_rgb8_buffer.raw_buffer(), _downscaled_buffer.raw_buffer());
    if (error != 0) {
        std::cout << "Downscale error: " << error << "\n";
    }
    std::cout << "Downscale time: " << std::chrono::duration_cast<Duration>(Clock::now() - step_start).count() << " ms"
              << "\n";

    std::cout << "Total process time: " << std::chrono::duration_cast<Duration>(Clock::now() - total_start).count()
              << " ms" << "\n";

    return {_rgb8_vector, _width, _height};
}

auto HalideRawPipeline::DownscaleImage() -> RgbImage {

    // TODO: Should not be computed on full image, but on a downsampled image
    auto total_start = Clock::now();

    auto error = downscale_generator(_rgb8_buffer.raw_buffer(), _downscaled_buffer.raw_buffer());
    if (error != 0) {
        std::cout << "Downscale error: " << error << "\n";
    }

    std::cout << "Total downscale time: " << std::chrono::duration_cast<Duration>(Clock::now() - total_start).count()
              << " ms" << "\n";

    return {_downscaled_vector, _width / _downscale_factor, _height / _downscale_factor};
}

auto HalideRawPipeline::GetHistogram() -> Histogram {

    // TODO: Should not be computed on full image, but on a downsampled image
    auto total_start = Clock::now();

    auto error = histogram_generator(_downscaled_buffer.raw_buffer(), _histogram_buffer.raw_buffer());
    if (error != 0) {
        std::cout << "Histogram error: " << error << "\n";
    }
    std::copy(_histogram_buffer.data(), _histogram_buffer.data() + kHistogramBins, _histogram.begin());

    std::cout << "Total histogram time: " << std::chrono::duration_cast<Duration>(Clock::now() - total_start).count()
              << " ms" << "\n";
    return _histogram;
}
}  // namespace brightroom