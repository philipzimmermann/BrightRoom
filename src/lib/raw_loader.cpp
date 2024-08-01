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

raw::Thumbnail GetThumbnail(const LibRaw& iProcessor) {
  const auto& thumbnail = iProcessor.imgdata.thumbnail;
  jpegErrorManager jerr;
  struct jpeg_decompress_struct cinfo;
  cinfo.err = jpeg_std_error(&jerr.pub);

  jpeg_create_decompress(&cinfo);
  jpeg_mem_src(&cinfo, (unsigned char*)thumbnail.thumb, thumbnail.tlength);
  int rc = jpeg_read_header(&cinfo, TRUE);
  jpeg_start_decompress(&cinfo);

  unsigned char* jdata =
      (unsigned char*)malloc(thumbnail.twidth * thumbnail.theight * 3);
  unsigned char* rowptr[1];
  while (cinfo.output_scanline < cinfo.output_height)  // loop
  {
    rowptr[0] = (unsigned char*)jdata +  // secret to method
                3 * cinfo.output_width * cinfo.output_scanline;

    jpeg_read_scanlines(&cinfo, rowptr, 1);
  }
  jpeg_finish_decompress(&cinfo);  //finish decompressing
  return raw::Thumbnail{jdata, thumbnail.twidth, thumbnail.theight};
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

  // Let us unpack the image
  iProcessor.unpack();

  if (iProcessor.unpack_thumb() != LibRaw_errors::LIBRAW_SUCCESS) {
    std::cout << "error:" << iProcessor.unpack_thumb() << std::endl;
  };

  // Convert from imgdata.rawdata to imgdata.image:
  iProcessor.raw2image();
  std::cout << " Finished!" << std::endl;

  RawFile rawfile{GetThumbnail(iProcessor), iProcessor.imgdata.image};
  iProcessor.recycle();
  return rawfile;
  // -------------------------------------

  // And let us print its dump; the data are accessible through data fields of
  // the class
  //   for (int i = 0; i < iProcessor.imgdata.thumbnail.twidth *
  //                           iProcessor.imgdata.thumbnail.theight*3;
  //        i++) {
  //     // printf("i=%d R=%d G=%d B=%d G2=%d\n",
  //     //        i,
  //     //        iProcessor.imgdata.image[i][0] >> 8,
  //     //        iProcessor.imgdata.image[i][1] >> 8,
  //     //        iProcessor.imgdata.image[i][2] >> 8,
  //     //        iProcessor.imgdata.image[i][3]);
  //     q_image.setPixelColor(
  //         i % iProcessor.imgdata.sizes.iwidth,
  //         i / iProcessor.imgdata.sizes.iwidth,
  //         QColor{std::min(iProcessor.imgdata.image[i][0] >> 3, 255),
  //                std::min(iProcessor.imgdata.image[i][1] >> 3, 255),
  //                std::min(iProcessor.imgdata.image[i][2] >> 3, 255)});
  //   }

  // Finally, let us free the image processor for work with the next image
  // iProcessor.recycle();
}

}  // namespace raw