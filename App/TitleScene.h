#pragma once
#include "DxLib.h"
#include <vector>
class Player;
class CameraRig;
class AssetsMgr;

// =========================
//  TitleScene é¿ëï
// =========================

class TitleScene
{
public:
    void Init(Player* player, CameraRig* camera, AssetsMgr* assets);
    bool Update();
    void Draw();
    void Finalize();

    bool IsStartRequested() const { return mStartRequested; }

private:
    // ---- ÉäÉ\Å[ÉX/èÛë‘----
    int mSceneGraph = -1;
    int mFontTitle = -1;
    int mFontSub = -1;

    float mTime = 0.0f;
    bool  mStartRequested = false;

    std::vector<VERTEX3D> mGeo;
    Player* player_ = nullptr;
    CameraRig* camera_ = nullptr;
    AssetsMgr* assets_ = nullptr;

    float angle_ = 0.0f;
    unsigned int prevTimeMs_ = 0;
    int mWidth = 1280;
    int mHeight = 720;
};
