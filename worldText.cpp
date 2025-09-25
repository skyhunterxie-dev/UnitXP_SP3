#include "pch.h"

#define _USE_MATH_DEFINES
#include <cmath>

#include "worldText.h"
#include "Vanilla1121_functions.h"
#include "utf8_to_utf16.h"
#include "inSight.h"

const int xp3::animationFPS = 180;

xp3::FloatingUpText::FloatingUpText(std::string text, uint64_t stickToGUID, D3DCOLOR color, float timeLength, int startOffsetX, int startOffsetY, int floatingDistance, ID3DXFont* font, LPDIRECT3DDEVICE9 device) {
    m_text = text;
    m_stickToGUID = stickToGUID;
    m_color = color;
    
    QueryPerformanceCounter(&m_startTime);

    SetRect(&m_rect, 0, 0, 1, 1);
    m_height = font->DrawTextW(NULL, utf8_to_utf16(text).c_str(), -1, &m_rect, DT_LEFT | DT_CALCRECT, color);
    m_width = m_rect.right;

    m_timingPrecision.QuadPart = getPerformanceCounterFrequency().QuadPart / animationFPS;
    m_totalTime = timeLength;
    m_floatingDistance = floatingDistance;
    m_shadowWeight = 2;
    m_startOffsetY = startOffsetY;
    m_startOffsetX = startOffsetX;

    update(font, device);
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
    if (vanilla1121_objectType(stickTo) != OBJECT_TYPE_Player && vanilla1121_objectType(stickTo) != OBJECT_TYPE_Unit) {
        return 0;
    }

    C3Vector p = vanilla1121_worldToScreen(vanilla1121_unitPosition(stickTo));
    if (p.x < 0.0f || p.y < 0.0f) {
        return 0;
    }

    double step = static_cast<double>((now.QuadPart - m_startTime.QuadPart) / m_timingPrecision.QuadPart) / static_cast<double>((m_totalTime * getPerformanceCounterFrequency().QuadPart) / m_timingPrecision.QuadPart);
    SetRect(&m_rect, static_cast<int>(p.x) - m_width / 2 + m_startOffsetX, static_cast<int>(p.y) - m_height - m_startOffsetY - static_cast<int>(step * m_floatingDistance), static_cast<int>(p.x) + m_width / 2 + m_startOffsetX, static_cast<int>(p.y) - m_startOffsetY - static_cast<int>(step * m_floatingDistance));

    // We test inSight so late that even the unit is out of sight, its m_rect get updated
    if (0 >= camera_inSight(reinterpret_cast<void*>(stickTo))) {
        return 0;
    }

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

void xp3::FloatingUpText::fastForward(int ffDistance) {
    m_startTime.QuadPart -= static_cast<LONGLONG>((static_cast<double>(ffDistance) / static_cast<double>(m_floatingDistance)) * m_totalTime * animationFPS) * m_timingPrecision.QuadPart;
}

xp3::CritText::CritText(std::string text, uint64_t stickToGUID, D3DCOLOR color, float timeLength, int startOffsetY, ID3DXFont* fontNormal, ID3DXFont* fontBig, LPDIRECT3DDEVICE9 device) {
    m_text = text;
    m_stickToGUID = stickToGUID;
    m_color = color;
    m_totalTime = timeLength;

    QueryPerformanceCounter(&m_startTime);

    SetRect(&m_rect, 0, 0, 1, 1);
    m_height = fontBig->DrawTextW(NULL, utf8_to_utf16(text).c_str(), -1, &m_rect, DT_LEFT | DT_CALCRECT, color);
    m_width = m_rect.right;

    m_totalTime = timeLength;
    m_shadowWeight = 2;
    m_fontDraw = m_fontNormal;
    m_startOffsetY = startOffsetY;

    update(fontNormal, fontBig, device);
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
    if (vanilla1121_objectType(stickTo) != OBJECT_TYPE_Player && vanilla1121_objectType(stickTo) != OBJECT_TYPE_Unit) {
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

    SetRect(&m_rect, static_cast<int>(p.x) - m_width / 2 - m_width / 3, static_cast<int>(p.y) - m_height - m_startOffsetY, static_cast<int>(p.x) + m_width / 2 - m_width / 3, static_cast<int>(p.y) - m_startOffsetY);

    // We test inSight so late that even the unit is out of sight, its m_rect get updated
    if (0 >= camera_inSight(reinterpret_cast<void*>(stickTo))) {
        return 0;
    }

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
