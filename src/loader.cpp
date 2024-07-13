#include "loader.h"

#include <iostream>
#include <libraw/libraw.h>
#include <filesystem>

void load_raw()
{
    std::cout << "Loading RAW" << std::endl;

    // Let us create an image processor
    LibRaw iProcessor;

    // Open the file and read the metadata
    std::string path = "/home/philip/workspace/cpp/BrightRoom/examples/P1190134.ORF";
    std::cout << "Path exists:" << std::filesystem::exists(path) << std::endl;
    iProcessor.open_file(path.c_str());

    // The metadata are accessible through data fields of the class
    printf("Image size: %d x %d\n", iProcessor.imgdata.sizes.width, iProcessor.imgdata.sizes.height);

    // Let us unpack the image
    iProcessor.unpack();

    // Convert from imgdata.rawdata to imgdata.image:
    iProcessor.raw2image();

    // And let us print its dump; the data are accessible through data fields of the class
    for (int i = 0; i < 100; i++)
        printf("i=%d R=%d G=%d B=%d G2=%d\n",
               i,
               iProcessor.imgdata.image[i][0],
               iProcessor.imgdata.image[i][1],
               iProcessor.imgdata.image[i][2],
               iProcessor.imgdata.image[i][3]);

    // Finally, let us free the image processor for work with the next image
    iProcessor.recycle();
    std::cout << " Finished!" << std::endl;
}