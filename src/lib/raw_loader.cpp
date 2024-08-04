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
#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include "jpeglib.h"
#include "libraw/libraw_const.h"
#include "setjmp.h"

namespace {

struct jpegErrorManager {
  struct jpeg_error_mgr pub;
  jmp_buf setjmp_buffer;
};

raw::RgbImage CreateThumbnail(const LibRaw& iProcessor) {
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

raw::RawFile CreateRawFile(const LibRaw& iProcessor) {
  auto thumbnail = CreateThumbnail(iProcessor);
  std::vector<unsigned short> rawdata;
  rawdata.insert(rawdata.end(), &iProcessor.imgdata.rawdata.raw_image[0],
                 &iProcessor.imgdata.rawdata
                      .raw_image[iProcessor.imgdata.sizes.raw_width *
                                 iProcessor.imgdata.sizes.raw_height]);
  return {std::move(thumbnail), std::move(rawdata),
          iProcessor.imgdata.sizes.raw_width,
          iProcessor.imgdata.sizes.raw_height};
}
}  // namespace

namespace raw {

RawFile RawLoader::LoadRaw(const std::string& file_name) {
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