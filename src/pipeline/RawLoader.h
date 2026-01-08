#pragma once
#include <libraw/libraw.h>
#include <memory>
#include <string>

namespace brightroom {

class RawLoader {
   public:
    auto LoadRaw(const std::string& file_name) -> std::unique_ptr<LibRaw>;

   private:
    int _cache;
};

}  // namespace brightroom