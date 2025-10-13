#include "pch.h"

// This file requires to be saved in UTF-8 BOM, as it includes Chinese strings

#pragma comment(lib, "libMinHook.x86.lib")

#include <Windows.h>
#include "MinHook.h"

#include <string>
#include <sstream>

#include "sceneEnd.h"
#include "utf8_to_utf16.h"
#include "Vanilla1121_functions.h"

static ID3DXFont* sceneEnd_font = NULL;
static bool fontOnLost = false;
static ID3DXFont* sceneEnd_fontBIG = NULL;
static bool fontBIGOnLost = false;
static ID3DXFont* sceneEnd_fontSmall = NULL;
static bool fontSmallOnLost = false;
static ID3DXFont* sceneEnd_fontHUGE = NULL;
static bool fontHugeOnLost = false;
static LPDIRECT3DDEVICE9 lastDXdevice = NULL;
static std::list<xp3::FloatingUpText> floatingTexts{};
static std::unordered_map<uint64_t, xp3::CritText> critTexts{};
static std::list<xp3::FloatingUpText> smallFloatingTexts{};
static int sceneEnd_fontSize = 36;

bool sceneEnd_isEnabled = false;
bool sceneEnd_hideEXPtext = false;

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

typedef HRESULT(__stdcall* D3DDEVICERESET)(LPDIRECT3DDEVICE9 pSelf, D3DPRESENT_PARAMETERS* pPresentationParameters);
static D3DDEVICERESET p_D3D_deviceReset = NULL;
static D3DDEVICERESET p_original_D3D_deviceReset = NULL;
HRESULT __stdcall detoured_D3D_deviceReset(LPDIRECT3DDEVICE9 pSelf, D3DPRESENT_PARAMETERS* pPresentationParameters) {

    // The strange thing is that this function never get called
    // I don't understand how the game handles Alt+Tab in Fullscreen mode

    if (sceneEnd_font != NULL && fontOnLost == false) {
        sceneEnd_font->OnLostDevice();
        fontOnLost = true;
    }
    if (sceneEnd_fontBIG != NULL && fontBIGOnLost == false) {
        sceneEnd_fontBIG->OnLostDevice();
        fontBIGOnLost = true;
    }
    if (sceneEnd_fontSmall != NULL && fontSmallOnLost == false) {
        sceneEnd_fontSmall->OnLostDevice();
        fontSmallOnLost = true;
    }
    if (sceneEnd_fontHUGE != NULL && fontHugeOnLost == false) {
        sceneEnd_fontHUGE->OnLostDevice();
        fontHugeOnLost = true;
    }

    HRESULT ret = p_original_D3D_deviceReset(pSelf, pPresentationParameters);

    if (SUCCEEDED(ret)) {
        HRESULT rFont = sceneEnd_font->OnResetDevice();
        HRESULT rFontBIG = sceneEnd_fontBIG->OnResetDevice();
        HRESULT rFontSmall = sceneEnd_fontSmall->OnResetDevice();
        HRESULT rFontHUGE = sceneEnd_fontHUGE->OnResetDevice();
        if (rFont == S_OK && rFontBIG == S_OK && rFontSmall == S_OK && rFontHUGE == S_OK) {
            fontOnLost = false;
            fontBIGOnLost = false;
            fontSmallOnLost = false;
            fontHugeOnLost = false;
        }
        else {
            // If reset failed, we try to reload
            if (sceneEnd_reloadFont(sceneEnd_fontSize)) {
                fontOnLost = false;
                fontBIGOnLost = false;
                fontSmallOnLost = false;
                fontHugeOnLost = false;
            }
            else {
                MessageBoxW(NULL, utf8_to_utf16(u8"Failed to reset nor reload font during D3D device reset.").data(), utf8_to_utf16(u8"UnitXP Service Pack 3").data(), MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
                sceneEnd_isEnabled = false;
            }
        }
    }
    return ret;
}

typedef ULONG(__stdcall* D3DDEVICERELEASE)(LPDIRECT3DDEVICE9 pSelf);
static D3DDEVICERELEASE p_D3D_deviceRelease = NULL;
static D3DDEVICERELEASE p_original_D3D_deviceRelease = NULL;
ULONG __stdcall detoured_D3D_deviceRelease(LPDIRECT3DDEVICE9 pSelf) {
    if (p_D3D_deviceReset != NULL) {
        if (MH_DisableHook(p_D3D_deviceReset) != MH_OK) {
            MessageBoxW(NULL, utf8_to_utf16(u8"Failed to disable hook for D3D device Reset function.").data(), utf8_to_utf16(u8"UnitXP Service Pack 3").data(), MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
            sceneEnd_isEnabled = false;
        }
        if (MH_RemoveHook(p_D3D_deviceReset) != MH_OK) {
            MessageBoxW(NULL, utf8_to_utf16(u8"Failed to remove hook for D3D device Reset function.").data(), utf8_to_utf16(u8"UnitXP Service Pack 3").data(), MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
            sceneEnd_isEnabled = false;
        }
        p_D3D_deviceReset = NULL;
        p_original_D3D_deviceReset = NULL;
    }
    if (p_D3D_deviceRelease != NULL) {
        if (MH_DisableHook(p_D3D_deviceRelease) != MH_OK) {
            MessageBoxW(NULL, utf8_to_utf16(u8"Failed to disable hook for D3D device Release function.").data(), utf8_to_utf16(u8"UnitXP Service Pack 3").data(), MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
            sceneEnd_isEnabled = false;
            return p_original_D3D_deviceRelease(pSelf);
        }
        if (MH_RemoveHook(p_D3D_deviceRelease) != MH_OK) {
            MessageBoxW(NULL, utf8_to_utf16(u8"Failed to remove hook for D3D device Release function.").data(), utf8_to_utf16(u8"UnitXP Service Pack 3").data(), MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
            sceneEnd_isEnabled = false;
        }
        p_D3D_deviceRelease = NULL;
        p_original_D3D_deviceRelease = NULL;

    }

    void** vtable = *reinterpret_cast<void***>(pSelf);
    auto p_currentRelease = reinterpret_cast<D3DDEVICERELEASE>(vtable[2]);

    if (p_currentRelease != &detoured_D3D_deviceRelease) {
        return p_currentRelease(pSelf);
    }
    else {
        MessageBoxW(NULL, utf8_to_utf16(u8"Somehow D3D device Release function is still pointing to detoured function.").data(), utf8_to_utf16(u8"UnitXP Service Pack 3").data(), MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);

        // We can't do much as we don't have a working Release()
        sceneEnd_isEnabled = false;

        // We return 1 indicating reference count isn't touching zero so that this object might stand in memory
        return 1;
    }
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
    if (sceneEnd_fontHUGE != NULL) {
        sceneEnd_fontHUGE->Release();
        sceneEnd_fontHUGE = NULL;
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

    // ChatGPT: Microsoft YaHei is an Unicode font and it exists even on English Windows
    const std::string fontName{ u8"Microsoft YaHei" };
    sceneEnd_fontSize = fontSize;

    if (sceneEnd_font == NULL) {
        if (false == SUCCEEDED(p_D3DCreateFontW(lastDXdevice, fontSize, 0, FW_SEMIBOLD, 1, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH | FF_DONTCARE, utf8_to_utf16(fontName).c_str(), &sceneEnd_font))) {
            sceneEnd_font = NULL;
            return false;
        }
        sceneEnd_fontPreload(sceneEnd_font);
        fontOnLost = false;
    }

    if (sceneEnd_fontBIG == NULL) {
        if (false == SUCCEEDED(p_D3DCreateFontW(lastDXdevice, fontSize + 15, 0, FW_BOLD, 1, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH | FF_DONTCARE, utf8_to_utf16(fontName).c_str(), &sceneEnd_fontBIG))) {
            sceneEnd_fontBIG = NULL;
            return false;
        }
        sceneEnd_fontPreload(sceneEnd_fontBIG);
        fontBIGOnLost = false;
    }

    if (sceneEnd_fontSmall == NULL) {
        if (false == SUCCEEDED(p_D3DCreateFontW(lastDXdevice, fontSize - 4, 0, FW_SEMIBOLD, 1, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH | FF_DONTCARE, utf8_to_utf16(fontName).c_str(), &sceneEnd_fontSmall))) {
            sceneEnd_fontSmall = NULL;
            return false;
        }
        sceneEnd_fontPreload(sceneEnd_fontSmall);
        fontSmallOnLost = false;
    }

    if (sceneEnd_fontHUGE == NULL) {
        if (false == SUCCEEDED(p_D3DCreateFontW(lastDXdevice, fontSize + 15, 0, FW_HEAVY, 1, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH | FF_DONTCARE, utf8_to_utf16(fontName).c_str(), &sceneEnd_fontHUGE))) {
            sceneEnd_fontHUGE = NULL;
            return false;
        }
        sceneEnd_fontPreload(sceneEnd_fontHUGE);
        fontHugeOnLost = false;
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

            void** vtable = *reinterpret_cast<void***>(dxDevice);
            p_D3D_deviceReset = reinterpret_cast<D3DDEVICERESET>(vtable[16]);
            if (MH_CreateHook(p_D3D_deviceReset, &detoured_D3D_deviceReset, reinterpret_cast<LPVOID*>(&p_original_D3D_deviceReset)) != MH_OK) {
                MessageBoxW(NULL, utf8_to_utf16(u8"Failed to create hook for D3D device Reset function.").data(), utf8_to_utf16(u8"UnitXP Service Pack 3").data(), MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
                sceneEnd_isEnabled = false;
                lastDXdevice = NULL;
                p_D3D_deviceReset = NULL;
                p_original_D3D_deviceReset = NULL;
                p_original_sceneEnd(CGxDevice);
                return;
            }
            if (MH_EnableHook(p_D3D_deviceReset) != MH_OK) {
                MessageBoxW(NULL, utf8_to_utf16(u8"Failed to enable hook for D3D device Reset function.").data(), utf8_to_utf16(u8"UnitXP Service Pack 3").data(), MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
                MH_RemoveHook(p_D3D_deviceReset);
                sceneEnd_isEnabled = false;
                lastDXdevice = NULL;
                p_D3D_deviceReset = NULL;
                p_original_D3D_deviceReset = NULL;
                p_original_sceneEnd(CGxDevice);
                return;
            }

            p_D3D_deviceRelease = reinterpret_cast<D3DDEVICERELEASE>(vtable[2]);
            if (MH_CreateHook(p_D3D_deviceRelease, &detoured_D3D_deviceRelease, reinterpret_cast<LPVOID*>(&p_original_D3D_deviceRelease)) != MH_OK) {
                MessageBoxW(NULL, utf8_to_utf16(u8"Failed to create hook for D3D device Release function.").data(), utf8_to_utf16(u8"UnitXP Service Pack 3").data(), MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
                sceneEnd_isEnabled = false;
                lastDXdevice = NULL;
                p_D3D_deviceRelease = NULL;
                p_original_D3D_deviceRelease = NULL;
                p_original_sceneEnd(CGxDevice);
                return;
            }
            if (MH_EnableHook(p_D3D_deviceRelease) != MH_OK) {
                MessageBoxW(NULL, utf8_to_utf16(u8"Failed to enable hook for D3D device Release function.").data(), utf8_to_utf16(u8"UnitXP Service Pack 3").data(), MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
                MH_RemoveHook(p_D3D_deviceRelease);
                sceneEnd_isEnabled = false;
                lastDXdevice = NULL;
                p_D3D_deviceRelease = NULL;
                p_original_D3D_deviceRelease = NULL;
                p_original_sceneEnd(CGxDevice);
                return;
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



        // First iteration does not draw but only update RECT for overlapping test
        for (auto it = critTexts.begin(); it != critTexts.end();) {
            int update = it->second.update(sceneEnd_font, sceneEnd_fontBIG, sceneEnd_fontHUGE, lastDXdevice);
            if (update == -1) {
                it = critTexts.erase(it);
                continue;
            }
            it++;
        }

        for (auto it = smallFloatingTexts.begin(); it != smallFloatingTexts.end();) {
            int update = it->update(sceneEnd_fontSmall, lastDXdevice);
            if (update == -1) {
                it = smallFloatingTexts.erase(it);
                continue;
            }


            if (update > 0) {
                for (auto jt = critTexts.begin(); jt != critTexts.end(); jt++) {
                    RECT r = {};
                    if (IntersectRect(&r, &(jt->second.m_rect), &(it->m_rect)) != 0) {
                        update = 0;
                        break;
                    }
                }
            }

            if (update > 0 && fontSmallOnLost == false) {
                it->draw();
            }
            it++;
        }


        for (auto it = floatingTexts.begin(); it != floatingTexts.end();) {
            int update = it->update(sceneEnd_font, lastDXdevice);
            if (update == -1) {
                it = floatingTexts.erase(it);
                continue;
            }

            if (update > 0) {
                for (auto jt = critTexts.begin(); jt != critTexts.end(); jt++) {
                    RECT r = {};
                    if (IntersectRect(&r, &(jt->second.m_rect), &(it->m_rect)) != 0) {
                        update = 0;
                        break;
                    }
                }
            }

            if (update > 0 && fontOnLost == false) {
                it->draw();
            }
            it++;
        }


        for (auto it = critTexts.begin(); it != critTexts.end();) {
            int update = it->second.update(sceneEnd_font, sceneEnd_fontBIG, sceneEnd_fontHUGE, lastDXdevice);
            if (update == -1) {
                it = critTexts.erase(it);
                continue;
            }
            if (update > 0 && fontBIGOnLost == false) {
                it->second.draw();
            }
            it++;
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
    if (sceneEnd_hideEXPtext && type == 4) {
        return;
    }

    if (false == sceneEnd_useXP3combatText) {
        p_original_createWorldText(self, type, text, color, unknown);
        return;
    }

    uint64_t stickToGUID = *reinterpret_cast<uint64_t*>(self + 0x10);
    if (stickToGUID == 0) {
        stickToGUID = UnitGUID("player");
    }
    ID3DXFont* font = sceneEnd_font;
    int r = 255;
    int g = 255;
    int b = 255;
    if (color != 0 && (color & 1) == 0) {
        r = (color >> 8) & 0xff;
        g = (color >> 16) & 0xff;
        b = (color >> 24) & 0xff;
    }

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
    case 4:
    case 5:
    {
        xp3::FloatingUpText newText(text, stickToGUID, r, g, b, 255, font, lastDXdevice);
        sortAddNewFloatingText(newText, floatingTexts);
        return;
    }
    case 2:
    {
        xp3::CritText newCrit(text, stickToGUID, r, g, b, 255, sceneEnd_font, sceneEnd_fontBIG, sceneEnd_fontHUGE, lastDXdevice);

        auto it = critTexts.find(stickToGUID);
        if (it != critTexts.end()) {
            critTexts.erase(it);
        }

        critTexts.insert({ stickToGUID, newCrit });
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

void sceneEnd_addSmallFloatingText(std::string text, int r, int g, int b, int a) {
    xp3::FloatingUpText newText(text, UnitGUID("player"), r, g, b, a, sceneEnd_fontSmall, lastDXdevice);
    sortAddNewFloatingText(newText, smallFloatingTexts);
}

void sceneEnd_addCritText(std::string text, int r, int g, int b, int a) {
    uint64_t player = UnitGUID("player");

    D3DCOLOR color = D3DCOLOR_ARGB(a, r, g, b);
    xp3::CritText newCrit(text, player, r, g, b, a, sceneEnd_font, sceneEnd_fontBIG, sceneEnd_fontHUGE, lastDXdevice);

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
