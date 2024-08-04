#pragma once

#include <vector>
namespace raw {
using RawData = std::vector<unsigned short>;

class RgbImage {
 public:
  RgbImage(std::vector<unsigned char> pixels, int width, int height)
      : pixels(pixels), width(width), height(height){};

  std::vector<unsigned char> pixels;
  int width;
  int height;
};

class RawFile {
 public:
  RawFile(RgbImage thumbnail, RawData rawdata, int width, int height)
      : thumbnail(thumbnail), rawdata(rawdata), width(width), height(height){};

  RgbImage thumbnail;
  RawData rawdata;  // Bayer data assumed format G B G B \ R G R G \ G B G B
  int width;
  int height;
};
}  // namespace raw