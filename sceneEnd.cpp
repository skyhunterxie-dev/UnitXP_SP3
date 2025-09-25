#include "pch.h"

// This file requires to be saved in UTF-8 BOM, as it includes Chinese strings

#include <Windows.h>
#include <d3d9.h>

#include <string>
#include <sstream>

#include "sceneEnd.h"
#include "utf8_to_utf16.h"
#include "Vanilla1121_functions.h"

static ID3DXFont* sceneEnd_font = NULL;
static ID3DXFont* sceneEnd_fontBIG = NULL;
static ID3DXFont* sceneEnd_fontSmall = NULL;
static LPDIRECT3DDEVICE9 lastDXdevice = NULL;
static std::list<xp3::FloatingUpText> floatingTexts{};
static std::unordered_map<uint64_t, xp3::CritText> critTexts{};
static std::list<xp3::FloatingUpText> smallFloatingTexts{};
static int floatingDistance = 300;
static int startOffsetX = 0;
static int startOffsetY = 50;
static int widthOfW = -1;
static int heightOfW = -1;
static const float whiteFloatingTime = 2.5f;
static const float yellowFloatingTime = 2.5f;
static const float critTime = 2.5f;
static const float xpTime = 5.0f;
static int sceneEnd_fontSize = 36;

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

static void sceneEnd_fontPreload(ID3DXFont* font) {
    // Visiable ASCII by ChatGPT
    font->PreloadCharacters(0x20, 0x7e);

    // Chinese
    font->PreloadTextW(utf8_to_utf16(u8"经验值：").c_str(), 4);
    font->PreloadTextW(utf8_to_utf16(u8"脱离进入战斗").c_str(), 6);
    font->PreloadTextW(utf8_to_utf16(u8"非荣誉击杀").c_str(), 5);
    font->PreloadTextW(utf8_to_utf16(u8"未命中").c_str(), 3);
    font->PreloadTextW(utf8_to_utf16(u8"闪避").c_str(), 2);
    font->PreloadTextW(utf8_to_utf16(u8"招架").c_str(), 2);
    font->PreloadTextW(utf8_to_utf16(u8"格挡").c_str(), 2);
    font->PreloadTextW(utf8_to_utf16(u8"吸收").c_str(), 2);
    font->PreloadTextW(utf8_to_utf16(u8"免疫").c_str(), 2);
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
    if (sceneEnd_fontSmall != NULL) {
        sceneEnd_fontSmall->Release();
        sceneEnd_fontSmall = NULL;
    }
    sceneEnd_isEnabled = false;
}

bool sceneEnd_reloadFont(int fontSize) {
    if (sceneEnd_isEnabled == false) {
        return false;
    }
    else {
        sceneEnd_end();
        sceneEnd_isEnabled = true;
    }

    sceneEnd_fontSize = fontSize;

    if (sceneEnd_font == NULL) {
        // ChatGPT: Microsoft YaHei is Unicode font and it exists even on English Windows
        if (false == SUCCEEDED(p_D3DCreateFontW(lastDXdevice, fontSize, 0, FW_NORMAL, 1, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei", &sceneEnd_font))) {
            sceneEnd_font = NULL;
            return false;
        }
        sceneEnd_fontPreload(sceneEnd_font);
        RECT r = {};
        SetRect(&r, 0, 0, 1, 1);
        heightOfW = sceneEnd_font->DrawTextW(NULL, utf8_to_utf16(u8"W").c_str(), -1, &r, DT_LEFT | DT_CALCRECT, D3DCOLOR_XRGB(0, 0, 0));
        widthOfW = r.right;
    }

    if (sceneEnd_fontBIG == NULL) {
        // ChatGPT: Microsoft YaHei is Unicode font and it exists even on English Windows
        if (false == SUCCEEDED(p_D3DCreateFontW(lastDXdevice, fontSize + 6, 0, FW_SEMIBOLD, 1, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei", &sceneEnd_fontBIG))) {
            sceneEnd_fontBIG = NULL;
            return false;
        }
        sceneEnd_fontPreload(sceneEnd_fontBIG);
    }

    if (sceneEnd_fontSmall == NULL) {
        // ChatGPT: Microsoft YaHei is Unicode font and it exists even on English Windows
        if (false == SUCCEEDED(p_D3DCreateFontW(lastDXdevice, fontSize - 6, 0, FW_NORMAL, 1, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei", &sceneEnd_fontSmall))) {
            sceneEnd_fontSmall = NULL;
            return false;
        }
        sceneEnd_fontPreload(sceneEnd_fontSmall);
    }

    return true;
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
                startOffsetY = std::abs(gameWindowRect.bottom - gameWindowRect.top) / 5 / 5;
            }

            if (false == sceneEnd_reloadFont(sceneEnd_fontSize)) {
                sceneEnd_isEnabled = false;
                lastDXdevice = NULL;
                p_original_sceneEnd(CGxDevice);
                return;
            }
        }

        if (sceneEnd_font == NULL || sceneEnd_fontBIG == NULL || sceneEnd_fontSmall == NULL) {
            if (false == sceneEnd_reloadFont(sceneEnd_fontSize)) {
                sceneEnd_isEnabled = false;
                lastDXdevice = NULL;
                p_original_sceneEnd(CGxDevice);
                return;
            }
        }

        if (false == smallFloatingTexts.empty()) {
            for (auto it = smallFloatingTexts.begin(); it != smallFloatingTexts.end();) {
                int update = it->update(sceneEnd_fontSmall, lastDXdevice);
                if (update == -1) {
                    it = smallFloatingTexts.erase(it);
                    continue;
                }
                if (update > 0) {
                    it->draw();
                }
                it++;
            }
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

static void sortAddNewFloatingText(xp3::FloatingUpText& newText, std::list<xp3::FloatingUpText>& list) {
    int maxOverlapHeight = -1;
    for (auto it = list.begin(); it != list.end(); ++it) {
        RECT r = {};
        if (IntersectRect(&r, &newText.m_rect, &it->m_rect)) {
            int intersectHeight = r.bottom - r.top;
            if (intersectHeight > maxOverlapHeight) {
                maxOverlapHeight = intersectHeight;
            }
        }
    }

    if (maxOverlapHeight > 0) {
        for (auto it = list.begin(); it != list.end(); ++it) {
            it->fastForward(maxOverlapHeight);
        }
    }

    list.push_back(newText);
}

static std::unordered_map<int, std::string> worldTextHistory{};
CREATEWORLDTEXT p_createWorldText = reinterpret_cast<CREATEWORLDTEXT>(0x6c73f0);
CREATEWORLDTEXT p_original_createWorldText = NULL;
bool sceneEnd_useXP3combatText = false;
void __fastcall detoured_createWorldText(uint32_t self, void* ignored, int type, char const* text, uint32_t color, uint32_t unknown) {
    if (false == sceneEnd_useXP3combatText) {
        p_original_createWorldText(self, type, text, color, unknown);
        return;
    }

    uint64_t stickToGUID = *reinterpret_cast<uint64_t*>(self + 0x10);
    if (stickToGUID == 0) {
        stickToGUID = UnitGUID("player");
    }
    float time = whiteFloatingTime;
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
            time = yellowFloatingTime;
            recolor = D3DCOLOR_XRGB((color & 0xff00) >> 8, (color & 0xff0000) >> 16, (color & 0xff000000) >> 24);
        }
        xp3::FloatingUpText newText(text, stickToGUID, recolor, time, startOffsetX, startOffsetY, floatingDistance, font, lastDXdevice);
        sortAddNewFloatingText(newText, floatingTexts);
        return;
    }
    case 2:
    {
        time = critTime;
        if (color != 0 && (color & 1) == 0) {
            recolor = D3DCOLOR_XRGB((color & 0xff00) >> 8, (color & 0xff0000) >> 16, (color & 0xff000000) >> 24);
        }
        xp3::CritText newCrit(text, stickToGUID, recolor, time, startOffsetY, sceneEnd_font, sceneEnd_fontBIG, lastDXdevice);

        auto it = critTexts.find(stickToGUID);
        if (it != critTexts.end()) {
            critTexts.erase(it);
        }

        critTexts.insert({ stickToGUID, newCrit });
        return;
    }
    case 4:
    {
        time = xpTime;
        font = sceneEnd_fontBIG;
        recolor = D3DCOLOR_XRGB(0xff, 0x33, 0xcc);
        if (color != 0 && (color & 1) == 0) {
            recolor = D3DCOLOR_XRGB((color & 0xff00) >> 8, (color & 0xff0000) >> 16, (color & 0xff000000) >> 24);
        }
        xp3::FloatingUpText newText(text, stickToGUID, recolor, time, startOffsetX, startOffsetY, floatingDistance, font, lastDXdevice);
        sortAddNewFloatingText(newText, floatingTexts);
        return;
    }
    case 5:
    {
        time = xpTime;
        font = sceneEnd_fontBIG;
        recolor = D3DCOLOR_XRGB(239, 191, 4);
        if (color != 0 && (color & 1) == 0) {
            recolor = D3DCOLOR_XRGB((color & 0xff00) >> 8, (color & 0xff0000) >> 16, (color & 0xff000000) >> 24);
        }
        xp3::FloatingUpText newText(text, stickToGUID, recolor, time, startOffsetX, startOffsetY, floatingDistance, font, lastDXdevice);
        sortAddNewFloatingText(newText, floatingTexts);
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

void sceneEnd_addSmallFloatingText(std::string text, D3DCOLOR color) {
    xp3::FloatingUpText newText(text, UnitGUID("player"), color, whiteFloatingTime, 0, 0, floatingDistance, sceneEnd_fontSmall, lastDXdevice);
    sortAddNewFloatingText(newText, smallFloatingTexts);
}

void sceneEnd_addCritText(std::string text, D3DCOLOR color) {
    uint64_t player = UnitGUID("player");

    xp3::CritText newCrit(text, player, color, critTime, startOffsetY / 2, sceneEnd_font, sceneEnd_fontBIG, lastDXdevice);
    
    auto it = critTexts.find(player);
    if (it != critTexts.end()) {
        critTexts.erase(it);
    }

    critTexts.insert({ player, newCrit });
}

std::string sceneEnd_debugText() {
    std::stringstream ss{};
    ss << "Unimplemented world text history:" << std::endl;
    for (auto &i : worldTextHistory) {
        ss << i.second << std::endl;
    }
    return ss.str();
}
