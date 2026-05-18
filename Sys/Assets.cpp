#include "Assets.h"
#include "../Game/WeaponTypes.h"
#include <cstdio>

AssetsMgr::AssetsMgr()
{
}

AssetsMgr::~AssetsMgr()
{
    // Release all cached models
    for (auto& kv : modelCache_) {
        if (kv.second != -1) MV1DeleteModel(kv.second);
    }
    // weaponModelHandles_ entries point into modelCache_ values, so no separate deletion
}


void AssetsMgr::Init()
{
    // Preload commonly used weapon models here. This avoids per-frame MV1LoadModel calls.
    // For now we attempt to preload the IronPipe models if paths are provided in WeaponSpec.
    using namespace Game;
    const WeaponSpec& w = GetWeaponSpec(WeaponType::IronPipe);
    if (w.pickupModelPath) {
        int h = LoadModel(w.pickupModelPath);
        if (h != -1) {
            int key = (static_cast<int>(WeaponType::IronPipe) << 1) | 0;
            weaponModelHandles_[key] = h;
        }
    }
    if (w.equipModelPath) {
        int h = LoadModel(w.equipModelPath);
        if (h != -1) {
            int key = (static_cast<int>(WeaponType::IronPipe) << 1) | 1;
            weaponModelHandles_[key] = h;
        }
    }
}

int AssetsMgr::LoadModel(const std::string& path)
{
    auto it = modelCache_.find(path);
    if (it != modelCache_.end()) return it->second;

    int handle = MV1LoadModel(path.c_str());
    if (handle == -1) {
        char buf[512];
        sprintf_s(buf, "Failed to load model: %s", path.c_str());
        DrawString(10, 60, buf, GetColor(255, 0, 0));
    }
    modelCache_.emplace(path, handle);
    return handle;
}

int AssetsMgr::GetWeaponModelHandle(Game::WeaponType type, bool equip)
{
    int key = (static_cast<int>(type) << 1) | (equip ? 1 : 0);
    auto it = weaponModelHandles_.find(key);
    if (it != weaponModelHandles_.end()) return it->second;

    // Not preloaded: attempt to load based on WeaponSpec
    using namespace Game;
    const WeaponSpec& w = GetWeaponSpec(type);
    const char* path = equip ? w.equipModelPath : w.pickupModelPath;
    if (!path) return -1;
    int h = LoadModel(path);
    if (h != -1) {
        weaponModelHandles_[key] = h;
    }
    return h;
}
