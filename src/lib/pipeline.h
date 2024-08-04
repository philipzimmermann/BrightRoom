#pragma once

#include "types.h"
namespace raw {

class Pipeline {
 public:
  Pipeline(){};
  RgbImage Run(const RawFile& input) const;

 private:
};
}  // namespace raw