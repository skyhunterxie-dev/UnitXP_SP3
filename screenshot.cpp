#include "pch.h"

#include <cstdlib>
#include <cstring>

#include "screenshot.h"
#include "utf8_to_utf16.h"
#include "stb_image_write.h"


CTGAFILE_WRITE_0x5a4810 p_CTgaFile_Write_0x5a4810 = reinterpret_cast<CTGAFILE_WRITE_0x5a4810>(0x5a4810);
CTGAFILE_WRITE_0x5a4810 p_original_CTgaFile_Write_0x5a4810 = NULL;

SCREENSHOT_FILETYPE screenshot_filetype = SCREENSHOT_FILETYPE::jpg;

int __fastcall detoured_CTgaFile_Write_0x5a4810(uint32_t self, void* ignored, char* TGAfilename) {
    char filename[260];
    uint8_t additionalHeaderLength = *reinterpret_cast<uint8_t*>(self + 0x8);
    uint8_t colorMapType = *reinterpret_cast<uint8_t*>(self + 0x9);
    uint8_t imageType = *reinterpret_cast<uint8_t*>(self + 0xa);
    uint32_t tgaByteSize = *reinterpret_cast<uint32_t*>(self + 0x3c);
    uint32_t tgaHeaderAddr = self + 0x8;
    uint32_t tgaDataAddr = *reinterpret_cast<uint32_t*>(self + 0x4);
    uint32_t tgaFooterAddr = self + 0x20;

    if (tgaDataAddr == NULL || TGAfilename == NULL || imageType != 2 || additionalHeaderLength != 0 || colorMapType != 0) {
        return p_original_CTgaFile_Write_0x5a4810(self, TGAfilename);
    }

    if (screenshot_filetype == SCREENSHOT_FILETYPE::png) {
        if (sprintf_s(filename, sizeof(filename), "%s%s", TGAfilename, ".png") <= 0) {
            return p_original_CTgaFile_Write_0x5a4810(self, TGAfilename);
        }
    }
    else {
        if (sprintf_s(filename, sizeof(filename), "%s%s", TGAfilename, ".jpg") <= 0) {
            return p_original_CTgaFile_Write_0x5a4810(self, TGAfilename);
        }
    }

    const int bpp = 3;
    const int quality = 95;
    uint16_t width = *reinterpret_cast<uint16_t*>(tgaHeaderAddr + 0xc);
    uint16_t height = *reinterpret_cast<uint16_t*>(tgaHeaderAddr + 0xc + 2);

    // Swap color channel to fit stb_image_write
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            uint8_t* a = reinterpret_cast<uint8_t*>(tgaDataAddr + (x + y * width) * bpp);
            uint8_t* b = reinterpret_cast<uint8_t*>(tgaDataAddr + (x + y * width) * bpp + 2);

            uint8_t temp = *a;
            *a = *b;
            *b = temp;
        }
    }

    if (screenshot_filetype == SCREENSHOT_FILETYPE::png) {
        if (stbi_write_png(filename, width, height, bpp, reinterpret_cast<void*>(tgaDataAddr), width * bpp) != 0) {
            return 1;
        }
        else {
            return 0;
        }
    }
    else {
        if (stbi_write_jpg(filename, width, height, bpp, reinterpret_cast<void*>(tgaDataAddr), quality) != 0) {
            return 1;
        }
        else {
            return 0;
        }
    }
}
