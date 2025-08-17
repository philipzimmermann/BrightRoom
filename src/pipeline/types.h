#pragma once

#include <cstdint>
#include <vector>
namespace brightroom {
using RGB8_Data = std::vector<uint8_t>;

// Bayer data assumed format:
// R G R G R
// G B G B G
// R G R G R
// G B G B G
using Raw16Data = std::vector<uint16_t>;
using RawFloatData = std::vector<float>;

struct RgbImage {
    RGB8_Data pixels;  // Format_RGB888
    int width;
    int height;
};

struct RawFile {
    RgbImage thumbnail;
    Raw16Data rawdata;
    int width;
    int height;
};
}  // namespace brightroom