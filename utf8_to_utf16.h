#pragma once

#include <Windows.h>

#include <string>

std::wstring utf8_to_utf16(const std::string& utf8);

bool isCambriaSupported(const std::wstring& utf16);
