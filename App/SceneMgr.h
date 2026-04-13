#pragma once
#include "DxLib.h"
#include <vector>

class Player;
class CameraRig;
class AssetsMgr;
class Enemy;
class TitleScene;
class EffectManager;

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
};
