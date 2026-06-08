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

    // Create a duplicated model instance for a weapon. Caller is responsible for
    // deleting the returned model handle with MV1DeleteModel when done.
    // Returns -1 on failure.
    int CreateWeaponModelInstance(Game::WeaponType type, bool equip = false);

    // Enemy base/animation helpers
    // Ensure the enemy base model is loaded and return the cached handle (shared).
    // Returns -1 on failure.
    int GetEnemyBaseModelHandle();

    // Create a duplicated model instance for an enemy. Returns duplicated handle
    // (caller owns and must delete with MV1DeleteModel) or -1 on failure.
    int CreateEnemyModelInstance();

    // Obtain a cached enemy animation model handle for a named animation (e.g. "idle").
    // The implementation will try common filename variants. Returns -1 on failure.
    int GetEnemyAnimModelHandle(const std::string& animName);

private:
    // cache path -> MV1 handle
    std::unordered_map<std::string, int> modelCache_;

    // map (weaponType<<1 | equipFlag) -> handle
    std::unordered_map<int, int> weaponModelHandles_;
    // Optionally cache a resolved enemy base path -> handle under modelCache_.
};

// Global accessor for the singleton AssetsMgr instance
AssetsMgr& GetAssetsMgr();
