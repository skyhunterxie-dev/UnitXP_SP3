#include "pch.h"

#include <string>

#include "edit_CreateWindowExA.h"

CREATEWINDOWEXA p_createWindowExA = NULL;
CREATEWINDOWEXA p_original_createWindowExA = NULL;

HWND WINAPI detoured_createWindowExA(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) {
    std::string className{ lpClassName };
    if (className == "GxWindowClassD3d" || className == "GxWindowClassOpenGl") {
        if ((dwStyle & WS_MINIMIZEBOX) == 0) {
            dwStyle |= WS_MINIMIZEBOX;
        }
    }

    return p_original_createWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
}
