#pragma once
#include <libraw/libraw.h>
#include <memory>
#include <string>
#include "types.h"

namespace raw {

class RawLoader {
   public:
    std::unique_ptr<LibRaw> LoadRaw(const std::string& file_name);

   private:
    int _cache;
};

}  // namespace raw