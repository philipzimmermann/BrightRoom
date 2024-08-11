#pragma once

#include <cstdint>
#include <vector>
namespace raw {
using RGB8_Data = std::vector<uint8_t>;

// Bayer data assumed format:
// R G R G R
// G B G B G
// R G R G R
// G B G B G
using Raw16Data = std::vector<uint16_t>;
using RawFloatData = std::vector<float>;

class RgbImage {
 public:
  RgbImage(std::vector<uint8_t> pixels, int width, int height)
      : pixels(pixels), width(width), height(height){};

  RGB8_Data pixels;  // Format_RGB888
  int width;
  int height;
};

class RawFile {
 public:
  RawFile(RgbImage thumbnail, Raw16Data rawdata, int width, int height)
      : thumbnail(thumbnail), rawdata(rawdata), width(width), height(height){};

  RgbImage thumbnail;
  Raw16Data rawdata;
  int width;
  int height;
};
}  // namespace raw