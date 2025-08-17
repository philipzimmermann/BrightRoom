#include "RawLoader.h"

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
#include "types.h"

namespace {

struct jpegErrorManager {
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

brightroom::RgbImage CreateThumbnail(const LibRaw& _iProcessor) {
    ZoneScoped;
    const auto& thumbnail = _iProcessor.imgdata.thumbnail;
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
    return brightroom::RgbImage{jdata, thumbnail.twidth, thumbnail.theight};
}

brightroom::RawFile CreateRawFile(LibRaw& _iProcessor) {
    ZoneScoped;
    auto thumbnail = CreateThumbnail(_iProcessor);
    std::vector<uint16_t> rawdata(_iProcessor.imgdata.sizes.raw_width * _iProcessor.imgdata.sizes.raw_height * 3, 0);
    std::cout << "col" << _iProcessor.COLOR(0, 0) << _iProcessor.imgdata.idata.cdesc << std::endl;
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    for (int row = 0; row < _iProcessor.imgdata.sizes.raw_height; ++row) {
        for (int col = 0; col < _iProcessor.imgdata.sizes.raw_width; ++col) {
            auto index = row * _iProcessor.imgdata.sizes.raw_width + col;
            if (row % 2 == 0) {
                if (col % 2 == 0) {
                    rawdata[index * 3 + 0] = _iProcessor.imgdata.rawdata.raw_image[index];
                } else {
                    rawdata[index * 3 + 1] = _iProcessor.imgdata.rawdata.raw_image[index];
                }
            } else {
                if (col % 2 == 0) {
                    rawdata[index * 3 + 1] = _iProcessor.imgdata.rawdata.raw_image[index];
                } else {
                    rawdata[index * 3 + 2] = _iProcessor.imgdata.rawdata.raw_image[index];
                }
            }
        }
    }
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << "Time taken: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms"
              << std::endl;
    brightroom::RawFile raw_file;
    raw_file.thumbnail = std::move(thumbnail);
    raw_file.rawdata = std::move(rawdata);
    raw_file.width = _iProcessor.imgdata.sizes.raw_width;
    raw_file.height = _iProcessor.imgdata.sizes.raw_height;

    return raw_file;
}
}  // namespace

namespace brightroom {

std::unique_ptr<LibRaw> RawLoader::LoadRaw(const std::string& file_name) {
    ZoneScoped;
    std::cout << "Loading RAW" << std::endl;
    auto i_processor = std::make_unique<LibRaw>();

    // Let us create an image processor

    // Open the file and read the metadata
    i_processor->open_file(file_name.c_str());

    // The metadata are accessible through data fields of the class
    printf("Image size: %d x %d\n", i_processor->imgdata.sizes.width, i_processor->imgdata.sizes.height);

    // Fills _iProcessor.rawdata.raw_image
    i_processor->unpack();

    if (i_processor->unpack_thumb() != LibRaw_errors::LIBRAW_SUCCESS) {
        std::cout << "error:" << i_processor->unpack_thumb() << std::endl;
    };
    std::cout << "Read Raw Image" << std::endl;
    return i_processor;
}

}  // namespace brightroom