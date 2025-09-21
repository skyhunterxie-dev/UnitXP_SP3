#include "pch.h"

#include <Windows.h>
#include <d3d9.h>

#include <string>
#include <sstream>
#include <unordered_map>
#include <list>

#include "sceneEnd.h"
#include "utf8_to_utf16.h"
#include "Vanilla1121_functions.h"
#include "worldText.h"

static ID3DXFont* sceneEnd_font = NULL;
static ID3DXFont* sceneEnd_fontBIG = NULL;
static LPDIRECT3DDEVICE9 lastDXdevice = NULL;
static std::list<xp3::FloatingUpText> floatingTexts{};
static std::unordered_map<uint64_t, xp3::CritText> critTexts{};
static int floatingDistance = 300;
static int startOffset = 50;

bool sceneEnd_isEnabled = false;

LPD3DXCREATEFFONTW p_D3DCreateFontW = NULL;
void sceneEnd_init() {
    HMODULE hDLL = NULL;
    for (int ver = 43; ver >= 24; --ver) {
        std::stringstream ss{};
        ss << "d3dx9_" << ver << ".dll";

        hDLL = LoadLibraryA(ss.str().c_str());
        if (hDLL != NULL) {
            break;
        }
    }
    if (hDLL == NULL) {
        return;
    }

    p_D3DCreateFontW = reinterpret_cast<LPD3DXCREATEFFONTW>(GetProcAddress(hDLL, "D3DXCreateFontW"));

    if (p_D3DCreateFontW == NULL) {
        FreeLibrary(hDLL);
        return;
    }

    sceneEnd_isEnabled = true;
}
void sceneEnd_end() {
    if (sceneEnd_font != NULL) {
        sceneEnd_font->Release();
        sceneEnd_font = NULL;
    }
    if (sceneEnd_fontBIG != NULL) {
        sceneEnd_fontBIG->Release();
        sceneEnd_fontBIG = NULL;
    }
    sceneEnd_isEnabled = false;
}

ISCENEEND p_sceneEnd = reinterpret_cast<ISCENEEND>(0x5a17a0);
ISCENEEND p_original_sceneEnd = NULL;
void __fastcall detoured_sceneEnd(uint32_t CGxDevice) {
    if (*reinterpret_cast<uint32_t*>(CGxDevice + 0x3a38) != NULL && sceneEnd_isEnabled) {
        LPDIRECT3DDEVICE9 dxDevice = reinterpret_cast<LPDIRECT3DDEVICE9>(*reinterpret_cast<uint32_t*>(CGxDevice + 0x38a8));

        if (dxDevice != lastDXdevice) {
            lastDXdevice = dxDevice;
            RECT gameWindowRect = {};
            if (GetClientRect(vanilla1121_gameWindow(), &gameWindowRect)) {
                floatingDistance = std::abs(gameWindowRect.bottom - gameWindowRect.top) / 4;
                startOffset = std::abs(gameWindowRect.bottom - gameWindowRect.top) / 5 / 5;
            }
            if (sceneEnd_font != NULL) {
                sceneEnd_font->Release();
                sceneEnd_font = NULL;
            }
            if (sceneEnd_fontBIG != NULL) {
                sceneEnd_fontBIG->Release();
                sceneEnd_fontBIG = NULL;
            }
        }

        if (sceneEnd_font == NULL) {
            // To find OS default font face name
            NONCLIENTMETRICSW ncm = {};
            ZeroMemory(&ncm, sizeof(ncm));
            ncm.cbSize = sizeof(ncm);
            if (false == SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0))
            {
                sceneEnd_isEnabled = false;
                p_original_sceneEnd(CGxDevice);
                return;
            }

            if (false == SUCCEEDED(p_D3DCreateFontW(dxDevice, 60, 0, FW_NORMAL, 1, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, ncm.lfMessageFont.lfFaceName, &sceneEnd_font))) {
                sceneEnd_font = NULL;
                sceneEnd_isEnabled = false;
                p_original_sceneEnd(CGxDevice);
                return;
            }
            sceneEnd_font->PreloadCharacters(0x20, 0x7e);
        }

        if (sceneEnd_fontBIG == NULL) {
            // To find OS default font face name
            NONCLIENTMETRICSW ncm = {};
            ZeroMemory(&ncm, sizeof(ncm));
            ncm.cbSize = sizeof(ncm);
            if (false == SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0))
            {
                sceneEnd_isEnabled = false;
                p_original_sceneEnd(CGxDevice);
                return;
            }

            if (false == SUCCEEDED(p_D3DCreateFontW(dxDevice, 70, 0, FW_MEDIUM, 1, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, ncm.lfMessageFont.lfFaceName, &sceneEnd_fontBIG))) {
                sceneEnd_fontBIG = NULL;
                sceneEnd_isEnabled = false;
                p_original_sceneEnd(CGxDevice);
                return;
            }
            sceneEnd_fontBIG->PreloadCharacters(0x20, 0x7e);
        }

        if (false == floatingTexts.empty()) {
            for (auto it = floatingTexts.begin(); it != floatingTexts.end();) {
                int update = it->update(sceneEnd_font, lastDXdevice);
                if (update == -1) {
                    it = floatingTexts.erase(it);
                    continue;
                }
                if (update > 0) {
                    it->draw();
                }
                it++;
            }
        }
        if (false == critTexts.empty()) {
            for (auto it = critTexts.begin(); it != critTexts.end();) {
                int update = it->second.update(sceneEnd_font, sceneEnd_fontBIG, lastDXdevice);
                if (update == -1) {
                    it = critTexts.erase(it);
                    continue;
                }
                if (update > 0) {
                    it->second.draw();
                }
                it++;
            }
        }
    }

    p_original_sceneEnd(CGxDevice);
}

static std::unordered_map<int, std::string> worldTextHistory{};
CREATEWORLDTEXT p_createWorldText = reinterpret_cast<CREATEWORLDTEXT>(0x6c73f0);
CREATEWORLDTEXT p_original_createWorldText = NULL;
void __fastcall detoured_createWorldText(uint32_t self, void* ignored, int type, char const* text, uint32_t color, uint32_t unknown) {
    uint64_t stickToGUID = *reinterpret_cast<uint64_t*>(self + 0x10);
    if (stickToGUID == 0) {
        stickToGUID = UnitGUID("player");
    }
    float time = 3.0f;
    D3DCOLOR recolor = D3DCOLOR_XRGB(255, 255, 255);
    ID3DXFont* font = sceneEnd_font;

    // type:
    // 0 attack/heal
    // 1 absorb
    // 2 crit
    // 3 dodge
    // 4 exp
    // 5 honor
    switch (type) {
    case 0:
    case 1:
    case 3:
    {
        if (color != 0 && (color & 1) == 0) {
            recolor = D3DCOLOR_XRGB((color & 0xff00) >> 8, (color & 0xff0000) >> 16, (color & 0xff000000) >> 24);
        }
        xp3::FloatingUpText newText(text, stickToGUID, recolor, time, startOffset, floatingDistance, font, lastDXdevice);
        floatingTexts.push_back(newText);
        return;
    }
    case 2:
    {
        time = 3.0f;
        if (color != 0 && (color & 1) == 0) {
            recolor = D3DCOLOR_XRGB((color & 0xff00) >> 8, (color & 0xff0000) >> 16, (color & 0xff000000) >> 24);
        }
        xp3::CritText newCrit(text, stickToGUID, recolor, time, startOffset, sceneEnd_font, sceneEnd_fontBIG, lastDXdevice);
        critTexts.insert({ stickToGUID, newCrit });
        return;
    }
    case 4:
    {
        time = 5.0f;
        font = sceneEnd_fontBIG;
        recolor = D3DCOLOR_XRGB(0xff, 0x33, 0xcc);
        if (color != 0 && (color & 1) == 0) {
            recolor = D3DCOLOR_XRGB((color & 0xff00) >> 8, (color & 0xff0000) >> 16, (color & 0xff000000) >> 24);
        }
        xp3::FloatingUpText newText(text, stickToGUID, recolor, time, startOffset, floatingDistance, font, lastDXdevice);
        floatingTexts.push_back(newText);
        return;
    }
    case 5:
    {
        time = 5.0f;
        font = sceneEnd_fontBIG;
        recolor = D3DCOLOR_XRGB(239, 191, 4);
        if (color != 0 && (color & 1) == 0) {
            recolor = D3DCOLOR_XRGB((color & 0xff00) >> 8, (color & 0xff0000) >> 16, (color & 0xff000000) >> 24);
        }
        xp3::FloatingUpText newText(text, stickToGUID, recolor, time, startOffset, floatingDistance, font, lastDXdevice);
        floatingTexts.push_back(newText);
        return;
    }
    default:
    {
        std::stringstream ss{};
        ss << type << " " << std::string(text);

        if (color != 0 && (color & 1) == 0) {
            uint32_t c = *reinterpret_cast<uint32_t*>(color);
            ss << " (" << c << ")";
        }
        uint64_t testGUID = *reinterpret_cast<uint64_t*>(self + 0x10);
        ss << "0x" << testGUID << " ";
        worldTextHistory.insert({ type, ss.str() });
        p_original_createWorldText(self, type, text, color, unknown);
        return;
    }
    }
}

std::string sceneEnd_debugText() {
    std::stringstream ss{};
    ss << "Unimplemented world text history:" << std::endl;
    for (auto &i : worldTextHistory) {
        ss << i.second << std::endl;
    }
    return ss.str();
}
