#include "pch.h"

#include <Windows.h>
#include <d3d9.h>

#include <string>
#include <sstream>

#include "FPScap.h"
#include "performanceProfiling.h"
#include "Vanilla1121_functions.h"
#include "sceneBegin_sceneEnd.h"
#include "utf8_to_utf16.h"

GXSCENEPRESENT_0x58a960 p_GxScenePresent_0x58a960 = reinterpret_cast<GXSCENEPRESENT_0x58a960>(0x58a960);
GXSCENEPRESENT_0x58a960 p_original_GxScenePresent_0x58a960 = NULL;

typedef UINT(WINAPI* NTDELAYEXECUTION)(BOOL, LARGE_INTEGER*);
static NTDELAYEXECUTION NtDelayExecution;

LARGE_INTEGER targetFrameInterval = {};
LARGE_INTEGER backgroundFrameInterval = {};

static LARGE_INTEGER nextFrameTime = {};
static int fpsMethod = 0; // 1 for NtDelayExecution()
static LARGE_INTEGER fpsSpinning = {};
static LARGE_INTEGER fpsResolutionUnit = {}; // Unit is 100 nanosecond intervals

void __fastcall detoured_GxScenePresent_0x58a960(uint32_t unknown) {
    if (vanilla1121_gameInForeground()) {
        if (targetFrameInterval.QuadPart < 1) {
            p_original_GxScenePresent_0x58a960(unknown);
            if (scene_needReloadFont) {
                LPDIRECT3DDEVICE9 gDevice = reinterpret_cast<LPDIRECT3DDEVICE9>(vanilla1121_d3dDevice(vanilla1121_gxDevice()));
                if (scene_lastDXdevice != NULL
                    && scene_lastDXdevice == gDevice
                    && scene_lastDXdevice->TestCooperativeLevel() == D3D_OK
                    && scene_fontsOnLost == false) {
                    scene_reloadFont();
                }
            }
            return;
        }
    }
    else {
        if (backgroundFrameInterval.QuadPart < 1) {
            p_original_GxScenePresent_0x58a960(unknown);
            if (scene_needReloadFont) {
                LPDIRECT3DDEVICE9 gDevice = reinterpret_cast<LPDIRECT3DDEVICE9>(vanilla1121_d3dDevice(vanilla1121_gxDevice()));
                if (scene_lastDXdevice != NULL
                    && scene_lastDXdevice == gDevice
                    && scene_lastDXdevice->TestCooperativeLevel() == D3D_OK
                    && scene_fontsOnLost == false) {
                    scene_reloadFont();
                }
            }
            return;
        }
    }

    LARGE_INTEGER now = {};
    QueryPerformanceCounter(&now);

    LARGE_INTEGER sleep = {};
    sleep.QuadPart = nextFrameTime.QuadPart - now.QuadPart;
    if (sleep.QuadPart > fpsSpinning.QuadPart) {
        // Negative values indicate relative time
        // NtDelayExecution https://ntdoc.m417z.com/ntdelayexecution
        sleep.QuadPart -= fpsSpinning.QuadPart;
        sleep.QuadPart /= fpsResolutionUnit.QuadPart;
        sleep.QuadPart = -sleep.QuadPart;

        if (fpsMethod == 1) {
            NtDelayExecution(FALSE, &sleep);
        }
    }

    // Spinning for the tail
    QueryPerformanceCounter(&now);
    while (now.QuadPart < nextFrameTime.QuadPart) {
        YieldProcessor();
        QueryPerformanceCounter(&now);
    }

    p_original_GxScenePresent_0x58a960(unknown);

    // From https://github.com/doitsujin/dxvk/blob/4799558d322f67d1ff8f2c3958ff03e776b65bc6/src/util/util_fps_limiter.cpp#L51
    if (vanilla1121_gameInForeground()) {
        nextFrameTime.QuadPart = (now.QuadPart < nextFrameTime.QuadPart + targetFrameInterval.QuadPart)
            ? nextFrameTime.QuadPart + targetFrameInterval.QuadPart
            : now.QuadPart + targetFrameInterval.QuadPart;
    }
    else {
        nextFrameTime.QuadPart = (now.QuadPart < nextFrameTime.QuadPart + backgroundFrameInterval.QuadPart)
            ? nextFrameTime.QuadPart + backgroundFrameInterval.QuadPart
            : now.QuadPart + backgroundFrameInterval.QuadPart;
    }

    if (scene_needReloadFont) {
        LPDIRECT3DDEVICE9 gDevice = reinterpret_cast<LPDIRECT3DDEVICE9>(vanilla1121_d3dDevice(vanilla1121_gxDevice()));
        if (scene_lastDXdevice != NULL
            && scene_lastDXdevice == gDevice
            && scene_lastDXdevice->TestCooperativeLevel() == D3D_OK
            && scene_fontsOnLost == false) {
            scene_reloadFont();
        }
    }
}

void initFPScap() {
    targetFrameInterval.QuadPart = 0;
    backgroundFrameInterval.QuadPart = 0;
    nextFrameTime.QuadPart = 0;
    fpsSpinning.QuadPart = getPerformanceCounterFrequency().QuadPart / 250; // Some Linux default to 250 for CONFIG_HZ
    fpsResolutionUnit.QuadPart = getPerformanceCounterFrequency().QuadPart / 10000000;

    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (ntdll) {
        NtDelayExecution = reinterpret_cast<NTDELAYEXECUTION>(GetProcAddress(ntdll, "NtDelayExecution"));
        if (NtDelayExecution == NULL) {
            // Fallback to spinning
            fpsMethod = 0;
            return;
        }
    }
    else {
        // Fallback to spinning
        fpsMethod = 0;
        return;
    }

    fpsMethod = 1;
    return;
}

std::string debugFPScap() {
    std::stringstream ss{};
    ss << "FPS cap timing method: ";
    if (fpsMethod == 1) {
        ss << "NtDelayExecution() with tail spinning";
    }
    else {
        ss << "spinning";
    }

    return ss.str();
}
