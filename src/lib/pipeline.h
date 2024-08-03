#pragma once

#include "types.h"
namespace raw {

class Pipeline {
  RgbImage Run(const RawFile& input);
};
}  // namespace raw