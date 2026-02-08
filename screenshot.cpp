#include "pch.h"

#include <cstdlib>
#include <cstring>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <new>

#include "screenshot.h"
#include "utf8_to_utf16.h"
#include "stb_image_write.h"


CTGAFILE_WRITE_0x5a4810 p_CTgaFile_Write_0x5a4810 = reinterpret_cast<CTGAFILE_WRITE_0x5a4810>(0x5a4810);
CTGAFILE_WRITE_0x5a4810 p_original_CTgaFile_Write_0x5a4810 = NULL;

SCREENSHOT_FILETYPE screenshot_filetype = SCREENSHOT_FILETYPE::png;
bool screenshot_enabled = true;

struct PendingScreenshot {
    uint8_t* buffer;
    uint16_t width;
    uint16_t height;
    SCREENSHOT_FILETYPE filetype;
};

static std::atomic<int> screenshots_pending{0};
static std::mutex screenshot_mutex;
static std::queue<PendingScreenshot> screenshot_queue;
static bool worker_running = false;
static char screenshot_dir[260] = {0};
static DWORD last_write_second = 0;
static int same_second_count = 0;

static void screenshot_worker() {
    const int bpp = 3;
    const int quality = 95;

    while (true) {
        PendingScreenshot shot;

        {
            std::lock_guard<std::mutex> lock(screenshot_mutex);
            if (screenshot_queue.empty()) {
                worker_running = false;
                screenshots_pending.fetch_sub(1);
                return;
            }
            shot = screenshot_queue.front();
            screenshot_queue.pop();
        }

        SYSTEMTIME st;
        GetLocalTime(&st);
        DWORD current_second = st.wDay * 86400 + st.wHour * 3600 + st.wMinute * 60 + st.wSecond;

        if (current_second == last_write_second) {
            same_second_count++;
        }
        else {
            last_write_second = current_second;
            same_second_count = 0;
        }

        char filename[260];
        const char* ext = (shot.filetype == SCREENSHOT_FILETYPE::png) ? ".png" : ".jpg";
        sprintf_s(filename, sizeof(filename),
                  "%sWoWScrnShot_%02d%02d%02d_%02d%02d%02d_%d%s",
                  screenshot_dir,
                  st.wMonth, st.wDay, st.wYear % 100,
                  st.wHour, st.wMinute, st.wSecond,
                  same_second_count, ext);

        // Swap BGR to RGB on our copy
        for (int y = 0; y < shot.height; ++y) {
            for (int x = 0; x < shot.width; ++x) {
                uint8_t* a = shot.buffer + (x + y * shot.width) * bpp;
                uint8_t* b = shot.buffer + (x + y * shot.width) * bpp + 2;

                uint8_t temp = *a;
                *a = *b;
                *b = temp;
            }
        }

        if (shot.filetype == SCREENSHOT_FILETYPE::png) {
            stbi_write_png(filename, shot.width, shot.height, bpp, shot.buffer, shot.width * bpp);
        }
        else {
            stbi_write_jpg(filename, shot.width, shot.height, bpp, shot.buffer, quality);
        }

        delete[] shot.buffer;
    }
}

int __fastcall detoured_CTgaFile_Write_0x5a4810(uint32_t self, void* ignored, char* TGAfilename) {
    if (!screenshot_enabled) {
        return p_original_CTgaFile_Write_0x5a4810(self, TGAfilename);
    }

    uint8_t additionalHeaderLength = *reinterpret_cast<uint8_t*>(self + 0x8);
    uint8_t colorMapType = *reinterpret_cast<uint8_t*>(self + 0x9);
    uint8_t imageType = *reinterpret_cast<uint8_t*>(self + 0xa);
    uint32_t tgaHeaderAddr = self + 0x8;
    uint32_t tgaDataAddr = *reinterpret_cast<uint32_t*>(self + 0x4);

    if (tgaDataAddr == NULL || TGAfilename == NULL || imageType != 2 || additionalHeaderLength != 0 || colorMapType != 0) {
        return p_original_CTgaFile_Write_0x5a4810(self, TGAfilename);
    }

    // Extract directory from game's filename on first call
    if (screenshot_dir[0] == '\0') {
        strncpy(screenshot_dir, TGAfilename, sizeof(screenshot_dir) - 1);
        char* last_sep = strrchr(screenshot_dir, '\\');
        if (!last_sep) last_sep = strrchr(screenshot_dir, '/');
        if (last_sep) *(last_sep + 1) = '\0';
        else screenshot_dir[0] = '\0';
    }

    const int bpp = 3;
    uint16_t width = *reinterpret_cast<uint16_t*>(tgaHeaderAddr + 0xc);
    uint16_t height = *reinterpret_cast<uint16_t*>(tgaHeaderAddr + 0xc + 2);

    size_t dataSize = width * height * bpp;
    uint8_t* buffer;
    try {
        buffer = new uint8_t[dataSize];
    }
    catch (const std::bad_alloc&) {
        return p_original_CTgaFile_Write_0x5a4810(self, TGAfilename);
    }
    memcpy(buffer, reinterpret_cast<void*>(tgaDataAddr), dataSize);

    {
        std::lock_guard<std::mutex> lock(screenshot_mutex);
        screenshot_queue.push({buffer, width, height, screenshot_filetype});

        if (!worker_running) {
            try {
                worker_running = true;
                screenshots_pending.fetch_add(1);
                std::thread(screenshot_worker).detach();
            }
            catch (...) {
                worker_running = false;
                screenshots_pending.fetch_sub(1);
                while (!screenshot_queue.empty()) {
                    delete[] screenshot_queue.front().buffer;
                    screenshot_queue.pop();
                }
                return p_original_CTgaFile_Write_0x5a4810(self, TGAfilename);
            }
        }
    }

    return 1;
}
