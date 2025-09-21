#pragma once

#include <string>

#include <Windows.h>
#include <d3dx9.h>

#include "performanceProfiling.h"

namespace xp3 {
    class FloatingUpText {
    public:
        FloatingUpText(std::string text, uint64_t stickToGUID, D3DCOLOR color, float timeLength,int startOffset, int floatDistance, ID3DXFont* font, LPDIRECT3DDEVICE9 device);

        // Return 1 = draw; 0 = invisible; -1 = end
        // As device might be lost during animation, we would update it
        int update(ID3DXFont* font, LPDIRECT3DDEVICE9 device);

        void draw();
    private:
        std::string m_text;
        uint64_t m_stickToGUID;
        D3DCOLOR m_color;
        ID3DXFont* m_font;
        LPDIRECT3DDEVICE9 m_device;
        LARGE_INTEGER m_startTime;
        int m_width;
        int m_height;
        RECT m_rect;
        LARGE_INTEGER m_timingPrecision;
        float m_totalTime;
        int m_floatingDistance;
        int m_shadowWeight;
        int m_startOffset;
    };

    class CritText {
    public:
        CritText(std::string text, uint64_t stickToGUID, D3DCOLOR color, float timeLength, int startOffset, ID3DXFont* fontNormal, ID3DXFont* fontBig, LPDIRECT3DDEVICE9 device);

        // Return 1 = draw; 0 = invisible; -1 = end
        // As device might be lost during animation, we would update it
        int update(ID3DXFont* fontNormal, ID3DXFont* fontBig, LPDIRECT3DDEVICE9 device);

        void draw();
    private:
        std::string m_text;
        uint64_t m_stickToGUID;
        D3DCOLOR m_color;
        ID3DXFont* m_fontNormal;
        ID3DXFont* m_fontBig;
        ID3DXFont* m_fontDraw;
        LPDIRECT3DDEVICE9 m_device;
        LARGE_INTEGER m_startTime;
        float m_totalTime;
        int m_shadowWeight;
        int m_width;
        int m_height;
        int m_startOffset;
        RECT m_rect;
    };
}
