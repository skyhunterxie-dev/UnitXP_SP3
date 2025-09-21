#include "pch.h"

#define _USE_MATH_DEFINES
#include <cmath>

#include "worldText.h"
#include "Vanilla1121_functions.h"
#include "utf8_to_utf16.h"

xp3::FloatingUpText::FloatingUpText(std::string text, uint64_t stickToGUID, D3DCOLOR color, float timeLength, int startOffset, int floatingDistance, ID3DXFont* font, LPDIRECT3DDEVICE9 device) {
    m_text = text;
    m_stickToGUID = stickToGUID;
    m_color = color;
    m_font = font;
    m_device = device;
    
    QueryPerformanceCounter(&m_startTime);

    SetRect(&m_rect, 0, 0, 1, 1);
    m_height = font->DrawTextW(NULL, utf8_to_utf16(text).c_str(), -1, &m_rect, DT_LEFT | DT_CALCRECT, color);
    m_width = m_rect.right;

    m_timingPrecision.QuadPart = getPerformanceCounterFrequency().QuadPart / 500;
    m_totalTime = timeLength;
    m_floatingDistance = floatingDistance;
    m_shadowWeight = 3;
    m_startOffset = startOffset;
}

int xp3::FloatingUpText::update(ID3DXFont* font, LPDIRECT3DDEVICE9 device) {
    m_font = font;
    m_device = device;

    LARGE_INTEGER now = {};
    QueryPerformanceCounter(&now);

    if (now.QuadPart - m_startTime.QuadPart > m_totalTime * getPerformanceCounterFrequency().QuadPart) {
        return -1;
    }

    uint32_t stickTo = vanilla1121_getVisiableObject(m_stickToGUID);
    if (stickTo == 0) {
        return 0;
    }

    C3Vector p = vanilla1121_worldToScreen(vanilla1121_unitPosition(stickTo));
    if (p.x < 0.0f || p.y < 0.0f) {
        return 0;
    }

    double step = static_cast<double>((now.QuadPart - m_startTime.QuadPart) / m_timingPrecision.QuadPart) / static_cast<double>((m_totalTime * getPerformanceCounterFrequency().QuadPart) / m_timingPrecision.QuadPart);
    step = 1.0 - std::cos(step * M_PI_2);

    SetRect(&m_rect, static_cast<int>(p.x) - m_width / 2, static_cast<int>(p.y) - m_height - m_startOffset - static_cast<int>(step * m_floatingDistance), static_cast<int>(p.x) + m_width / 2, static_cast<int>(p.y) - m_startOffset - static_cast<int>(step * m_floatingDistance));
    return 1;
}

void xp3::FloatingUpText::draw() {
    RECT shadowRect = {};
    CopyRect(&shadowRect, &m_rect);
    shadowRect.left += m_shadowWeight;
    shadowRect.right += m_shadowWeight;
    shadowRect.top += m_shadowWeight;
    shadowRect.bottom += m_shadowWeight;

    m_font->DrawTextW(NULL, utf8_to_utf16(m_text).c_str(), -1, &shadowRect, DT_LEFT, D3DCOLOR_XRGB(0, 0, 0));
    m_font->DrawTextW(NULL, utf8_to_utf16(m_text).c_str(), -1, &m_rect, DT_LEFT, m_color);
}

xp3::CritText::CritText(std::string text, uint64_t stickToGUID, D3DCOLOR color, float timeLength, int startOffset, ID3DXFont* fontNormal, ID3DXFont* fontBig, LPDIRECT3DDEVICE9 device) {
    m_text = text;
    m_stickToGUID = stickToGUID;
    m_color = color;
    m_totalTime = timeLength;
    m_fontBig = fontBig;
    m_fontNormal = fontNormal;
    m_device = device;

    QueryPerformanceCounter(&m_startTime);

    SetRect(&m_rect, 0, 0, 1, 1);
    m_height = fontBig->DrawTextW(NULL, utf8_to_utf16(text).c_str(), -1, &m_rect, DT_LEFT | DT_CALCRECT, color);
    m_width = m_rect.right;

    m_totalTime = timeLength;
    m_shadowWeight = 3;
    m_fontDraw = m_fontNormal;
    m_startOffset = startOffset;
}

int xp3::CritText::update(ID3DXFont* fontNormal, ID3DXFont* fontBig, LPDIRECT3DDEVICE9 device) {
    m_fontNormal = fontNormal;
    m_fontBig = fontBig;
    m_device = device;

    LARGE_INTEGER now = {};
    QueryPerformanceCounter(&now);

    if (now.QuadPart - m_startTime.QuadPart > m_totalTime * getPerformanceCounterFrequency().QuadPart) {
        return -1;
    }

    uint32_t stickTo = vanilla1121_getVisiableObject(m_stickToGUID);
    if (stickTo == 0) {
        return 0;
    }

    C3Vector p = vanilla1121_worldToScreen(vanilla1121_unitPosition(stickTo));
    if (p.x < 0.0f || p.y < 0.0f) {
        return 0;
    }

    if (now.QuadPart - m_startTime.QuadPart < 0.2f * getPerformanceCounterFrequency().QuadPart) {
        m_fontDraw = m_fontNormal;
    }
    else {
        m_fontDraw = m_fontBig;
    }

    SetRect(&m_rect, static_cast<int>(p.x) - m_width / 2 - m_width / 3, static_cast<int>(p.y) - m_height - m_startOffset, static_cast<int>(p.x) + m_width / 2 - m_width / 3, static_cast<int>(p.y) - m_startOffset);
    return 1;
}

void xp3::CritText::draw() {
    RECT shadowRect = {};
    CopyRect(&shadowRect, &m_rect);
    shadowRect.left += m_shadowWeight;
    shadowRect.right += m_shadowWeight;
    shadowRect.top += m_shadowWeight;
    shadowRect.bottom += m_shadowWeight;

    m_fontDraw->DrawTextW(NULL, utf8_to_utf16(m_text).c_str(), -1, &shadowRect, DT_LEFT, D3DCOLOR_XRGB(0, 0, 0));
    m_fontDraw->DrawTextW(NULL, utf8_to_utf16(m_text).c_str(), -1, &m_rect, DT_LEFT, m_color);
}
