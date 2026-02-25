#pragma once
#include "DxLib.h"

namespace Lighting
{
    // Initialize default scene lighting / ambient settings.
    // Currently adjusts background/ambient tone; extend later for more lights.
    void InitDefault();

    // Simple helper to set ambient/background color (0..255)
    inline void SetAmbientColor(int r, int g, int b) { SetBackgroundColor(r, g, b); }
}
