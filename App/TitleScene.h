#pragma once
#include "DxLib.h"

class Player;
class CameraRig;
class AssetsMgr;

class TitleScene
{
public:
    TitleScene();
    ~TitleScene();

    void Init(Player* player, CameraRig* camera, AssetsMgr* assets);
    // Update returns true when the player requested to start the game (press Enter)
    bool Update();

private:
    Player* player_ = nullptr;
    CameraRig* camera_ = nullptr;
    AssetsMgr* assets_ = nullptr;

    float angle_ = 0.0f;
    unsigned int prevTimeMs_ = 0;
};
