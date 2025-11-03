#include "pch.h"

#include <string>

#include <Windows.h>

#include "utf8_to_utf16.h"

// By ChatGPT
std::wstring utf8_to_utf16(const std::string& utf8)
{
    if (utf8.empty())
        return std::wstring();

    // Calculate the size of the destination buffer
    int size_needed = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
        utf8.data(), static_cast<int>(utf8.size()),
        nullptr, 0);
    if (size_needed == 0)
    {
        MessageBoxW(NULL, L"MultiByteToWideChar failed: invalid UTF-8 or system error.", L"UnitXP Service Pack 3", MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
        return std::wstring();
    }

    std::wstring utf16(size_needed, 0);

    // Perform the actual conversion
    int result = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
        utf8.data(), static_cast<int>(utf8.size()),
        &utf16[0], size_needed);
    if (result == 0)
    {
        MessageBoxW(NULL, L"MultiByteToWideChar failed during conversion.", L"UnitXP Service Pack 3", MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
        return std::wstring();
    }

    return utf16;
}

// By ChatGPT
bool isCambriaSupported(const std::wstring& utf16) {
    for (wchar_t ch : utf16) {
        // Latin
        if ((ch >= 0x0020 && ch <= 0x007E) ||           // Basic Latin
            (ch >= 0x00A0 && ch <= 0x00FF) ||           // Latin-1 Supplement
            (ch >= 0x0100 && ch <= 0x017F) ||           // Latin Extended-A
            (ch >= 0x0180 && ch <= 0x024F)) {           // Latin Extended-B
            continue;
        }

        // Cyrillic
        if ((ch >= 0x0400 && ch <= 0x04FF) ||           // Cyrillic
            (ch >= 0x0500 && ch <= 0x052F)) {           // Cyrillic Supplement
            continue;
        }

        // Greek
        if ((ch >= 0x0370 && ch <= 0x03FF)) {           // Greek & Coptic
            continue;
        }

        // Numbers & Punctuation
        if ((ch >= 0x2000 && ch <= 0x206F) ||           // General punctuation
            (ch >= 0x20A0 && ch <= 0x20CF) ||           // Currency symbols
            (ch >= 0x2150 && ch <= 0x218F)) {           // Number fractions
            continue;
        }

        // If character didn't match any supported block, cannot render
        return false;
    }

    return true;
}
