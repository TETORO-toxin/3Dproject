#include "Lighting.h"

namespace Lighting
{
    void InitDefault()
    {
        // Increase ambient/background brightness so scene is not too dark.
        // These values are tuned for the current assets; adjust as needed.
        SetBackgroundColor(30, 30, 40);

        // Disable engine's built-in lighting to use custom lighting instead.
        // SetSetUseLighting to FALSE as requested.
        SetUseLighting(FALSE);

        // Enable basic 3D lighting if available on engine; DxLib provides functions like SetUseLighting
        // but to stay compatible across versions we only set material ambient-like parameters where possible.
#ifdef __cplusplus
        // Attempt to set ambient-ish material via SetMaterial or similar if available.
#if defined(SETMATERIAL) || defined(SetMaterial)
        // no-op: placeholder in case macro is present
#endif
#endif
    }
}
