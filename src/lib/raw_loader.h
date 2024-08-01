#pragma once
#include <libraw/libraw.h>
#include <string>

namespace raw {

class Thumbnail {
 public:
  Thumbnail(unsigned char* pixels, int width, int height)
      : pixels(pixels), width(width), height(height){};

  unsigned char* pixels;
  int width;
  int height;
};

class RawFile {
 public:
  RawFile(Thumbnail thumbnail) : thumbnail(thumbnail){};

  Thumbnail thumbnail;
};

class RawLoader {
 public:
  RawLoader() = default;
  RawFile LoadRaw(const std::string& file_name);

 private:
  int cache;
};

}  // namespace raw