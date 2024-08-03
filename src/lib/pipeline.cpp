#include "pipeline.h"
#include "types.h"

namespace {

raw::RgbImage Demosaic(const raw::RawFile& rawfile) {}

}  // namespace

namespace raw {

RgbImage Pipeline::Run(const RawFile& rawfile) {
  return Demosaic(rawfile);
}
}  // namespace raw