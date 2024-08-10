#include "pipeline.h"
#include <algorithm>
#include <cstdint>
#include "iostream"
#include "types.h"

namespace {
raw::RawFloatData Normalize(const raw::Raw16Data& rawdata) {
  auto minmax = std::minmax_element(rawdata.begin(), rawdata.end(),
                                    [](int a, int b) { return a < b; });
  std::cout << "min" << *minmax.first << " max:" << *minmax.second << std::endl;
  std::vector<float> output(rawdata.size());
  float difference = static_cast<float>(*minmax.second - *minmax.first);
  for (int i = 0; i < rawdata.size(); ++i) {
    output[i] = (rawdata[i] - *minmax.first) / difference;
  }
  return output;
}

raw::RawFloatData Demosaic(const raw::RawFloatData& rawdata) {
  return rawdata;
}

raw::RGB8_Data ToRgb8(const raw::RawFloatData& rawdata) {
  std::vector<uint8_t> rgb_data(rawdata.size());
  for (int i = 0; i < rawdata.size(); ++i) {
    rgb_data[i] = rawdata[i] * 255;
  }
  return rgb_data;
}

}  // namespace

namespace raw {

RgbImage Pipeline::Run(const RawFile& rawfile) const {
  auto rawdata = Normalize(rawfile.rawdata);
  rawdata = Demosaic(rawdata);
  auto rgb_data = ToRgb8(rawdata);
  return {rgb_data, rawfile.width, rawfile.height};
}
}  // namespace raw