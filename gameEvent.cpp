#include "pch.h"

#include "gameEvent.h"
#include "sceneBegin_sceneEnd.h"
#include "Vanilla1121_functions.h"

void onPlayerEnteringWorld() {
    scene_onPlayerEnteringWorld();
}

void onPlayerLeavingWorld() {
    scene_onPlayerLeavingWorld();
}

void onPlayerRegenDisabled() {
    vanilla1121_disableGC(GetContext());
}

void onPlayerRegenEnabled() {
    vanilla1121_enableGC(GetContext());
}
