#pragma once

#include <cstdint>
#include <string>

typedef void(__fastcall* GXSCENEPRESENT_0x58a960)(uint32_t);
extern GXSCENEPRESENT_0x58a960 p_GxScenePresent_0x58a960;
extern GXSCENEPRESENT_0x58a960 p_original_GxScenePresent_0x58a960;
void __fastcall detoured_GxScenePresent_0x58a960(uint32_t unknown);

void initFPScap();
void endFPScap();
std::string debugFPScap();

extern LARGE_INTEGER targetFrameInterval;
extern LARGE_INTEGER backgroundFrameInterval;
