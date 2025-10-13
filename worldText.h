#pragma once

#include <string>

#include <Windows.h>
#include <d3dx9.h>

#include "performanceProfiling.h"

namespace xp3 {
    extern const int animationFPS;

    class FloatingUpText {
    public:
        FloatingUpText(std::string text, uint64_t stickToGUID, int r, int g, int b, int a, ID3DXFont* font, LPDIRECT3DDEVICE9 device);

        // Return 1 = draw; 0 = invisible; -1 = end
        // As device might be lost during animation, we would update it
        int update(ID3DXFont* font, LPDIRECT3DDEVICE9 device);
        void draw();
        void fastForward(int ffDistance);

        RECT m_rect;
    private:
        std::string m_text;
        uint64_t m_stickToGUID;
        int m_r;
        int m_g;
        int m_b;
        int m_a;
        ID3DXFont* m_font;
        LPDIRECT3DDEVICE9 m_device;
        LARGE_INTEGER m_startTime;
        int m_width;
        int m_height;
        LARGE_INTEGER m_timingPrecision;
        double m_totalTime;
        double m_fadeOutTime;
        int m_floatingDistance;
        int m_shadowWeight;
        int m_offsetY;
        int m_offsetX;

        // Additional alpha blending
        double m_alpha;
    };

    class CritText {
    public:
        CritText(std::string text, uint64_t stickToGUID, int r, int g, int b, int a, ID3DXFont* fontNormal, ID3DXFont* fontBig, ID3DXFont* fontHuge, LPDIRECT3DDEVICE9 device);

        // Return 1 = draw; 0 = invisible; -1 = end
        // As device might be lost during animation, we would update it
        int update(ID3DXFont* fontNormal, ID3DXFont* fontBig, ID3DXFont* fontHuge, LPDIRECT3DDEVICE9 device);

        void draw();

        RECT m_rect;
    private:
        std::string m_text;
        uint64_t m_stickToGUID;
        int m_r;
        int m_g;
        int m_b;
        int m_a;
        
        // Additional alpha blending
        double m_alpha;
        double m_alpha_forHugeFont;

        ID3DXFont* m_fontNormal;
        ID3DXFont* m_fontBig;
        ID3DXFont* m_fontHuge;
        ID3DXFont* m_fontDraw;
        LPDIRECT3DDEVICE9 m_device;
        LARGE_INTEGER m_startTime;
        LARGE_INTEGER m_timingPrecision;
        double m_totalTime;
        double m_fadeOutTime;
        int m_shadowWeight;
        int m_floatingDistance;
        int m_width;
        int m_height;
    };
}
