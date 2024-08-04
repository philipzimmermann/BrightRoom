#include "pipeline.h"
#include <cstdint>
#include "types.h"

namespace {

raw::RgbImage Demosaic(const raw::RawFile& rawfile) {
  std::vector<uint8_t> rgb_8bit(rawfile.rawdata.size());
  for (int i = 0; i < rawfile.rawdata.size(); ++i) {
    rgb_8bit[i] = std::min(255, rawfile.rawdata[i] / 2);
  }
  return {rgb_8bit, rawfile.width, rawfile.height};
}

}  // namespace

namespace raw {

RgbImage Pipeline::Run(const RawFile& rawfile) const {
  return Demosaic(rawfile);
}
}  // namespace raw