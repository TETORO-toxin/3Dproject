#pragma once
#include "DxLib.h"
#include <vector>
class Player;
class CameraRig;
class AssetsMgr;

// =========================
//  TitleScene 実装（1cpp完結）
// =========================
// ※プロジェクト側に TitleScene クラス宣言がある前提です。
//   関数名が違う場合は、あなたのシーン規約に合わせて置換してください。
class TitleScene
{
public:
    void Init(Player* player, CameraRig* camera, AssetsMgr* assets);
    bool Update();
    void Draw();
    void Finalize();

    // このフラグをシーンマネージャが見て遷移する想定（任意）
    bool IsStartRequested() const { return mStartRequested; }

private:
    // ---- リソース/状態（本当は.hに置くが、要望によりcpp内）----
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
