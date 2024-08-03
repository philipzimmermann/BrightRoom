#pragma once
#include <string>
#include "types.h"

namespace raw {

class RawLoader {
 public:
  RawLoader() = default;
  RawFile LoadRaw(const std::string& file_name);

 private:
  int cache;
};

}  // namespace raw