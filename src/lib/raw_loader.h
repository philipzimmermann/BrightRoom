#pragma once
#include <libraw/libraw.h>
#include <string>
#include "types.h"

namespace raw {

class RawLoader {
   public:
    LibRaw LoadRaw(const std::string& file_name);

   private:
    int _cache;
};

}  // namespace raw