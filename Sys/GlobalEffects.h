#pragma once
#include "EffectManager.h"

// Simple global accessor for the EffectManager owned by the application.
void SetGlobalEffectManager(EffectManager* mgr);
// Returns pointer to global EffectManager or nullptr if not set.
EffectManager* GetGlobalEffectManager();
