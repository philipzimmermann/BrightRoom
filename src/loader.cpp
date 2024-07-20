#include "loader.h"

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
#include <filesystem>
#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include "jpeglib.h"
#include "libraw/libraw_const.h"
#include "setjmp.h"

struct jpegErrorManager {
  struct jpeg_error_mgr pub;
  jmp_buf setjmp_buffer;
};

void load_raw() {
  std::cout << "Loading RAW" << std::endl;

  // Let us create an image processor
  LibRaw iProcessor;

  // Open the file and read the metadata
  std::string path =
      "/home/philip/workspace/cpp/BrightRoom/examples/P1190134.ORF";
  std::cout << "Path exists:" << std::filesystem::exists(path) << std::endl;
  iProcessor.open_file(path.c_str());

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

  QImage q_image{iProcessor.imgdata.thumbnail.twidth,
                 iProcessor.imgdata.thumbnail.theight, QImage::Format_RGB16};
  std::cout << "format:" << iProcessor.imgdata.thumbnail.tformat << "  "
            << LibRaw_thumbnail_formats::LIBRAW_THUMBNAIL_JPEG << std::endl;

  const auto& thumbnail = iProcessor.imgdata.thumbnail;

  //------------------------------

  // JPEG Decompression :O
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

  cv::Mat image = cv::Mat(cv::Size(thumbnail.twidth, thumbnail.theight),
                          CV_8UC3, jdata, cv::Mat::AUTO_STEP);
  cv::Mat converted;
  cvtColor(image, converted, cv::COLOR_BGR2RGB);
  cv::imshow("Image", converted);
  cv::waitKey(0);

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
  iProcessor.recycle();

  char* argv[] = {(char*)"BrightRoom"};
  int argc = 1;
  QApplication a(argc, argv);
  QGraphicsScene scene;
  QGraphicsView view(&scene);
  auto path2 = QApplication::applicationDirPath().toStdString() +
               "/../../../examples/groundhog.png";
  std::cout << path2 << std::endl;
  QGraphicsPixmapItem item{QPixmap{path2.c_str()}};

  scene.addPixmap(QPixmap::fromImage(q_image.scaledToWidth(1500)));

  view.show();
  a.exec();

  std::cout << " Finished!" << std::endl;
}