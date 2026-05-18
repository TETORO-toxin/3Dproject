#pragma once
#include "DxLib.h"
#include <string>
#include <unordered_map>

namespace Game { enum class WeaponType; struct WeaponSpec; }

class AssetsMgr
{
public:
    AssetsMgr();
    ~AssetsMgr();

    void Init();

    // Load a model by path. Models are cached and a previously loaded handle will be returned
    // for subsequent calls with the same path. Returns -1 on failure.
    int LoadModel(const std::string& path);

    // Weapon model helpers
    // - If the requested model is not yet loaded, this will attempt to load it based on
    //   the WeaponSpec paths (pickup/equip) and return the handle.
    // - If no model is specified for the weapon or the load failed, returns -1.
    int GetWeaponModelHandle(Game::WeaponType type, bool equip = false);

private:
    // cache path -> MV1 handle
    std::unordered_map<std::string, int> modelCache_;

    // map (weaponType<<1 | equipFlag) -> handle
    std::unordered_map<int, int> weaponModelHandles_;
};
