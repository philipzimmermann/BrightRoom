#include "pipeline.h"
#include <algorithm>
#include <cstdint>
#include "iostream"
#include "types.h"

namespace {
float& At(raw::RawFloatData& rawdata, int x, int y, int color, int width) {
  return rawdata[y * width * 3 + x * 3 + color];
}

raw::RawFloatData Normalize(raw::Raw16Data& rawdata) {

  // TODO get minmax per channel and normalize per channel!
  auto minmax = std::minmax_element(rawdata.begin(), rawdata.end(),
                                    [](int a, int b) { return a < b; });
  std::cout << "min" << *minmax.first << " max:" << *minmax.second << std::endl;
  raw::RawFloatData output(rawdata.size());
  float difference = static_cast<float>(*minmax.second - *minmax.first);
  for (int i = 0; i < output.size(); ++i) {
    output[i] = (rawdata[i] - *minmax.first) / difference;
  }
  return output;
}

void Demosaic(raw::RawFloatData& rawdata, int width, int height) {

  // R G R G R
  // G B G B G
  // R G R G R
  // G B G B G

  // interpolate horizontal
  for (int row = 0; row < height; ++row) {
    if (row % 2 == 0) {
      // type 1
      // get red
      for (int col = 1; col < width - 1; col += 2) {
        At(rawdata, col, row, 0, width) =
            (At(rawdata, col - 1, row, 0, width) +
             At(rawdata, col + 1, row, 0, width)) /
            2;
      }
      // get green
      for (int col = 2; col < width - 1; col += 2) {
        At(rawdata, col, row, 1, width) =
            (At(rawdata, col - 1, row, 1, width) +
             At(rawdata, col + 1, row, 1, width)) /
            2;
      }
    } else {
      // type 2
      // get green
      for (int col = 1; col < width - 1; col += 2) {
        At(rawdata, col, row, 1, width) =
            (At(rawdata, col - 1, row, 1, width) +
             At(rawdata, col + 1, row, 1, width)) /
            2;
      }
      // get blue
      for (int col = 2; col < width - 1; col += 2) {
        At(rawdata, col, row, 2, width) =
            (At(rawdata, col - 1, row, 2, width) +
             At(rawdata, col + 1, row, 2, width)) /
            2;
      }
    }
  }

  // interpolate vertical
  for (int col = 0; col < width; ++col) {
    if (col % 2 == 0) {
      // type 1
      // get red
      for (int row = 1; row < height - 1; row += 2) {
        At(rawdata, col, row, 0, width) =
            (At(rawdata, col, row - 1, 0, width) +
             At(rawdata, col, row + 1, 0, width)) /
            2;
      }
      // get blue
      for (int row = 2; row < height - 1; row += 2) {
        At(rawdata, col, row, 2, width) =
            (At(rawdata, col, row - 1, 2, width) +
             At(rawdata, col, row + 1, 2, width)) /
            2;
      }
    } else {
      // type 2
      // get green + red
      for (int row = 1; row < height - 1; row += 2) {
        At(rawdata, col, row, 1, width) =
            (At(rawdata, col, row - 1, 1, width) +
             At(rawdata, col, row + 1, 1, width)) /
            2;
        At(rawdata, col, row, 0, width) =
            (At(rawdata, col, row - 1, 0, width) +
             At(rawdata, col, row + 1, 0, width)) /
            2;
      }
      // get  blue
      for (int row = 2; row < height - 1; row += 2) {
        At(rawdata, col, row, 2, width) =
            (At(rawdata, col, row - 1, 2, width) +
             At(rawdata, col, row + 1, 2, width)) /
            2;
      }
    }
  }

  for (int i = width * 3; i < 20 + width * 3; ++i) {
    std::cout << "row2:" << rawdata[i] << std::endl;
  }
  for (int i = width * 3 * 2; i < 20 + width * 3 * 2; ++i) {
    std::cout << "row3:" << rawdata[i] << std::endl;
  }
}

raw::RGB8_Data ToRgb8(const raw::RawFloatData& rawdata) {
  raw::RGB8_Data rgb_data(rawdata.size());
  for (int i = 0; i < rawdata.size(); ++i) {
    rgb_data[i] = rawdata[i] * 255;
  }
  return rgb_data;
}

}  // namespace

namespace raw {

RgbImage Pipeline::Run(const RawFile& rawfile) const {
  auto rawdata = rawfile.rawdata;
  auto float_data = Normalize(rawdata);
  Demosaic(float_data, rawfile.width, rawfile.height);
  auto rgb_data = ToRgb8(float_data);
  return {rgb_data, rawfile.width, rawfile.height};
}
}  // namespace raw