#include "GlobalEffects.h"

static EffectManager* g_mgr = nullptr;

void SetGlobalEffectManager(EffectManager* mgr)
{
    g_mgr = mgr;
}

EffectManager* GetGlobalEffectManager()
{
    return g_mgr;
}
