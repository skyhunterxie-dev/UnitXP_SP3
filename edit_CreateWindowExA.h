#pragma once

#include <Windows.h>

// Enable Minimize button even the game works in maximized window mode

typedef HWND(WINAPI* CREATEWINDOWEXA)(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
extern CREATEWINDOWEXA p_createWindowExA;
extern CREATEWINDOWEXA p_original_createWindowExA;
HWND WINAPI detoured_createWindowExA(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
