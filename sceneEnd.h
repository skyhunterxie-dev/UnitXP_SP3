#pragma once

#include <unordered_map>
#include <list>

#include <Windows.h>
#include <d3dx9.h>

#include "worldText.h"

typedef HRESULT (WINAPI* LPD3DXCREATEFFONTW)(
    LPDIRECT3DDEVICE9       pDevice,
    INT                     Height,
    UINT                    Width,
    UINT                    Weight,
    UINT                    MipLevels,
    BOOL                    Italic,
    DWORD                   CharSet,
    DWORD                   OutputPrecision,
    DWORD                   Quality,
    DWORD                   PitchAndFamily,
    LPCWSTR                 pFaceName,
    LPD3DXFONT* ppFont);
extern LPD3DXCREATEFFONTW p_D3DCreateFontW;
extern bool sceneEnd_isEnabled;
extern bool sceneEnd_useXP3combatText;
void sceneEnd_init();
void sceneEnd_end();
bool sceneEnd_reloadFont(int fontSize);


typedef void(__fastcall* ISCENEEND)(uint32_t);
extern ISCENEEND p_sceneEnd;
extern ISCENEEND p_original_sceneEnd;
void __fastcall detoured_sceneEnd(uint32_t CGxDevice);


// The technique of hooking __thiscall function is from: https://tresp4sser.wordpress.com/2012/10/06/how-to-hook-thiscall-functions/
// -- Pointer is __thiscall with 1st param being THIS
// -- The detoured function is __fastcall with 1st param being THIS, and 2nd param being IGNORED
typedef void(__thiscall* CREATEWORLDTEXT)(uint32_t, int, char const*, uint32_t, uint32_t);
extern CREATEWORLDTEXT p_createWorldText;
extern CREATEWORLDTEXT p_original_createWorldText;
void __fastcall detoured_createWorldText(uint32_t self, void* ignored, int type, char const* text, uint32_t color, uint32_t unknown);

void sceneEnd_addSmallFloatingText(std::string text, D3DCOLOR color = D3DCOLOR_XRGB(255, 255, 255));
void sceneEnd_addCritText(std::string text, D3DCOLOR color = D3DCOLOR_XRGB(255, 255, 255));
std::string sceneEnd_debugText();
