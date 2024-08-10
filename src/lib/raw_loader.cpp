#include "raw_loader.h"

#include <libraw/libraw.h>
#include <opencv2/core/hal/interface.h>
#include <qbytearrayview.h>
#include <qpixmap.h>
#include <qwindowdefs.h>
#include <stddef.h>
#include <QApplication>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QImageReader>
#include <cstdint>
#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include "Tracy.hpp"
#include "jpeglib.h"
#include "libraw/libraw_const.h"
#include "setjmp.h"

namespace {

struct jpegErrorManager {
  struct jpeg_error_mgr pub;
  jmp_buf setjmp_buffer;
};

raw::RgbImage CreateThumbnail(const LibRaw& iProcessor) {
  ZoneScoped;
  const auto& thumbnail = iProcessor.imgdata.thumbnail;
  jpegErrorManager jerr;
  struct jpeg_decompress_struct cinfo;
  cinfo.err = jpeg_std_error(&jerr.pub);

  jpeg_create_decompress(&cinfo);
  jpeg_mem_src(&cinfo, (unsigned char*)thumbnail.thumb, thumbnail.tlength);
  int rc = jpeg_read_header(&cinfo, TRUE);
  jpeg_start_decompress(&cinfo);

  std::vector<unsigned char> jdata(thumbnail.twidth * thumbnail.theight * 3);
  unsigned char* rowptr[1];
  while (cinfo.output_scanline < cinfo.output_height)  // loop
  {
    rowptr[0] = jdata.data() +  // secret to method
                3 * cinfo.output_width * cinfo.output_scanline;

    jpeg_read_scanlines(&cinfo, rowptr, 1);
  }
  jpeg_finish_decompress(&cinfo);  //finish decompressing
  return raw::RgbImage{jdata, thumbnail.twidth, thumbnail.theight};
}

raw::RawFile CreateRawFile(LibRaw& iProcessor) {
  ZoneScoped;
  auto thumbnail = CreateThumbnail(iProcessor);
  std::vector<uint16_t> rawdata(iProcessor.imgdata.sizes.raw_width *
                                    iProcessor.imgdata.sizes.raw_height * 3,
                                0);
  for (int row = 0; row < iProcessor.imgdata.sizes.raw_height; ++row) {
    for (int col = 0; col < iProcessor.imgdata.sizes.raw_width; ++col) {
      auto index = row * iProcessor.imgdata.sizes.raw_width + col;
      if (row % 2 == 0) {
        if (col % 2 == 0) {
          rawdata[index * 3 + 0] = iProcessor.imgdata.rawdata.raw_image[index];
        } else {
          rawdata[index * 3 + 1] = iProcessor.imgdata.rawdata.raw_image[index];
        }
      } else {
        if (col % 2 == 0) {
          rawdata[index * 3 + 1] = iProcessor.imgdata.rawdata.raw_image[index];
        } else {
          rawdata[index * 3 + 2] = iProcessor.imgdata.rawdata.raw_image[index];
        }
      }
    }
  }
  return {std::move(thumbnail), std::move(rawdata),
          iProcessor.imgdata.sizes.raw_width,
          iProcessor.imgdata.sizes.raw_height};
}
}  // namespace

namespace raw {

RawFile RawLoader::LoadRaw(const std::string& file_name) {
  ZoneScoped;
  std::cout << "Loading RAW" << std::endl;

  // Let us create an image processor
  LibRaw iProcessor;

  // Open the file and read the metadata
  iProcessor.open_file(file_name.c_str());

  // The metadata are accessible through data fields of the class
  printf("Image size: %d x %d\n", iProcessor.imgdata.sizes.width,
         iProcessor.imgdata.sizes.height);

  // Fills iProcessor.rawdata.raw_image
  iProcessor.unpack();

  if (iProcessor.unpack_thumb() != LibRaw_errors::LIBRAW_SUCCESS) {
    std::cout << "error:" << iProcessor.unpack_thumb() << std::endl;
  };

  // Convert from imgdata.rawdata to imgdata.image:
  // iProcessor.raw2image();
  std::cout << "Read Raw Image" << std::endl;

  RawFile rawfile = CreateRawFile(iProcessor);
  std::cout << "Created Raw Object" << std::endl;
  iProcessor.recycle();
  std::cout << "Recycled iProcessor" << std::endl;

  return rawfile;
}

}  // namespace raw