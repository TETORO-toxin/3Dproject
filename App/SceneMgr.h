#pragma once
#include "DxLib.h"
#include <vector>
#include "../Sys/Input.h"

class Player;
class CameraRig;
class AssetsMgr;
class Enemy;
class TitleScene;
class EffectManager;

#include "../Game/WeaponPickup.h"
#include "../Game/NavMesh.h"

class SceneMgr
{
public:
    ~SceneMgr();
    enum class Scene
    {
        Title,
        Base,
        Mission,
        Result
    };

    void Init();
    void Update();
    void ChangeScene(Scene s);
    Scene GetCurrentScene() const; // getter for main to observe transitions

private:
    Scene currentScene_ = Scene::Title;
    Player* player_ = nullptr;
    CameraRig* camera_ = nullptr;
    // navigation mesh for AI pathfinding
    NavMesh navMesh_;
    AssetsMgr* assets_ = nullptr;
    std::vector<Enemy*> enemies_;
    VECTOR target_ = VGet(0,0,1);

    // ground represented as a plane (point + normal)
    VECTOR groundPoint_ = VGet(0.0f, 0.0f, 0.0f);
    VECTOR groundNormal_ = VGet(0.0f, 1.0f, 0.0f);

    // Title screen camera rotation
    float titleAngle_ = 0.0f; // radians
    unsigned int prevTimeMs_ = 0;

    // Title scene
    TitleScene* titleScene_ = nullptr;
    // owned effect manager (registered as global)
    EffectManager* effects_ = nullptr;
    // --- 武器ピックアップ管理 ---
    std::vector<Game::WeaponPickup> weaponPickups_;
    float pickupRange_ = 2.0f; // プレイヤーからの取得可能距離 (XZ平面)
    
    // ------------------------------------------------------------------
    // Pickup 関連の補助状態 / ヘルパ関数
    // ------------------------------------------------------------------
    // Update() 内で毎フレーム最寄りピックアップ探索や取得処理を行うための
    // ヘルパを分離しています。これにより SceneMgr::Update() 本体が肥大化
    // するのを防ぎ、探索条件や取得 UI を将来的に簡単に差し替え可能に
    // します。
    // - UpdateWeaponPickups: 入力とプレイヤー位置に基づいてピックアップの
    //   取得判定・実際の Equip 処理を行う（ロジックのみ）。
    // - DrawWeaponPickups: ピックアップに関する HUD 表示と実際のピックアップ
    //   モデルの描画・未取得エントリの削除を行う（描画側の責務）。
    // - FindNearestPickupIndex: 指定位置から最も近い未取得ピックアップの
    //   インデックスを返す。将来の "CollectNearbyPickups" 等の拡張に接続しやすい。
    
    // 最後に探索された最寄りピックアップのインデックス/距離は Draw 側で
    // 参照するために保持します。存在しない場合は -1。
    int lastNearestPickupIdx_ = -1;
    float lastNearestPickupDist2_ = 1e9f;

    // 宣言: 定義は SceneMgr.cpp にあります
    void UpdateWeaponPickups(const InputState& in, const VECTOR& ppos);
    void DrawWeaponPickups();
    int FindNearestPickupIndex(const VECTOR& ppos) const;
};
