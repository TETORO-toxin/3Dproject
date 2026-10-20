#pragma once
#include "DxLib.h"
#include <array>
#include <unordered_map>
#include <string>
#include <functional>
#include <vector>
#include "../Sys/Input.h"
#include "WeaponTypes.h"

class Enemy;

enum class PlayerState
{
    STATE_NORMAL,
    STATE_THROW_CHARGING
};

static const char* const kRightHandFrameCandidates[] = {
    "mixamorig:LeftHand",
    "LeftHand",
    "Hand_L",
    "L_Hand",
    "l_hand",
    "hand_l",
    "hand.L",
    "WeaponAttach_L",
    "Left",
    nullptr
};

static const char* const kUpperBodyFrameCandidates[] = {
    "mixamorig:Spine",
    "Spine",
    "Chest",
    "Torso",
    "UpperBody",
    "Spine1",
    "Spine2",
    nullptr
};

class AssetsMgr;
class ProjectileManager;
struct AuxUnit;
class CameraRig;

class Player
{
public:
    enum class Mode { Melee, Ranged };
    enum class AnimLayer { Full = 0, Lower = 1, Upper = 2 };

    Player(AssetsMgr* assets = nullptr);
    ~Player();
    void Update();
    void UpdateLogic(float dt, const InputState& in);
    void Draw();

    VECTOR GetPosition() const;
    void SetCamera(CameraRig* cam);

    PlayerState GetState() const { return state_; }
    bool IsThrowCharging() const { return state_ == PlayerState::STATE_THROW_CHARGING; }
    bool HasCoinInAir() const { return coinAlive_; }
    VECTOR GetCoinPosition() const { return coinPos_; }
    void SetCoinFocusTarget(const VECTOR& coinPos);
    void UpdateCoinThrow(float dt, const InputState& in, const std::vector<Enemy*>& enemies);
    void DrawCoinThrowEffect() const;
    void DrawReflectTrajectory() const;

    void OnIncomingAttack();
    bool IsInvulnerable() const;

    void UpdateAux(float dt, ProjectileManager& pm);
    void FireAuxLeft(ProjectileManager& pm);
    void FireAuxRight(ProjectileManager& pm);

    void SetAwakened(bool v);
    void Heal(float amount);
    float GetHP() const { return hp; }

    void PlayAnimation(const std::string& name, bool loop = false, AnimLayer layer = AnimLayer::Full);
    void UpdateAnimation(float dt);

    void AddAnimEvent(const std::string& animName, float timeSec, std::function<void()> cb);
    void ClearAnimEvents(const std::string& animName);

    Game::WeaponType GetEquippedWeapon() const;
    bool TryEquipWeapon(Game::WeaponType newWeapon, Game::WeaponType* oldOut);
    Game::WeaponType EquipWeapon(Game::WeaponType newWeapon);

private:
    int baseModelHandle_ = -1;
    int modelHandle_ = -1;
    std::unordered_map<std::string,int> animModelHandles_;
    std::unordered_map<std::string,int> animModelAnimIndex_;

    float x_ = 0.0f, y_ = 0.0f, z_ = 0.0f;
    float visualScale_ = 0.04f;

    float hp = 100.0f;
    float maxHp = 100.0f;

    Mode mode_ = Mode::Melee;
    unsigned int lastModeSwitchTimeMs_ = 0;

    unsigned int lastAttackTimeMs_ = 0;
    float baseAttackCooldownMs_ = 300.0f;
    float attackCooldownMs_ = 300.0f;

    unsigned int lastDodgeTimeMs_ = 0;
    unsigned int dodgeCooldownMs_ = 800;
    unsigned int dodgeDurationMs_ = 300;

    unsigned int lastIncomingTimeMs_ = 0;
    unsigned int baseJustWindowMs_ = 200;
    unsigned int justWindowMs_ = 200;

    bool justExecuted_ = false;

    AuxUnit* auxLeft = nullptr;
    AuxUnit* auxRight = nullptr;

    float auxGauge = 100.0f;
    float baseAuxGaugeRegenRate = 5.0f;
    float auxGaugeRegenRate = 5.0f;
    bool awakened = false;

    std::unordered_map<std::string, float> animDurations_;
    std::string currentAnim_;
    float animTime_ = 0.0f;
    bool animLoop_ = true;

    struct AnimEvent { float time = 0.0f; bool fired = false; std::function<void()> cb; };
    std::unordered_map<std::string, std::vector<AnimEvent>> animEvents_;
    float prevAnimTime_ = 0.0f;

    CameraRig* camera_ = nullptr;

    float currentYaw_ = 0.0f;
    float targetYaw_ = 0.0f;
    float yawTurnSpeed_ = 8.0f;

    int attachedAnimAttachIndex_ = -1;
    int prevAttachedAnimAttachIndex_ = -1;
    float attachedAnimTotalTime_ = 0.0f;
    float prevAttachedAnimTotalTime_ = 0.0f;

    std::string prevAnimName_;
    float prevAnimTimeSeconds_ = 0.0f;
    bool prevAnimLoop_ = true;

    float animBlendRate_ = 1.0f;
    float animBlendSpeed_ = 0.12f;

    std::string upperAnim_;
    float upperAnimTime_ = 0.0f;
    bool upperAnimLoop_ = true;

    int upperAttachedAnimAttachIndex_ = -1;
    int prevUpperAttachedAnimAttachIndex_ = -1;
    float upperAttachedAnimTotalTime_ = 0.0f;
    float prevUpperAttachedAnimTotalTime_ = 0.0f;

    std::string prevUpperAnim_;
    float prevUpperAnimTimeSeconds_ = 0.0f;
    bool prevUpperAnimLoop_ = true;

    float upperAnimBlendRate_ = 1.0f;
    bool ownsBaseModel_ = false;

    float velY_ = 0.0f;
    bool onGround_ = true;
    float gravity_ = -20.0f;
    float jumpVelocity_ = 8.0f;
    unsigned int lastJumpTimeMs_ = 0;

    bool prevAttackBtnDown_ = false;

    Game::WeaponType equippedWeapon_ = Game::WeaponType::None;
    AssetsMgr* assets_ = nullptr;

    int rightHandFrameIndex_ = -1;
    std::string rightHandFrameName_;

    int upperBodyFrameIndex_ = -1;
    std::string upperBodyFrameName_;

    int equippedWeaponModelHandle_ = -1;
    bool equippedWeaponModelOwned_ = false;

    PlayerState state_ = PlayerState::STATE_NORMAL;
    bool coinAlive_ = false;
    VECTOR coinPos_ = VGet(0.0f, 0.0f, 0.0f);
    VECTOR coinVel_ = VGet(0.0f, 0.0f, 0.0f);
    float chargeTime_ = 0.0f;
    float slowMotionScale_ = 0.2f;
    float currentTimeScale_ = 1.0f;
    bool attackKeyHeld_ = false;
    bool reflectShotDrawn_ = false;
    bool reflectHit_ = false;
    VECTOR reflectTarget_ = VGet(0.0f, 0.0f, 0.0f);
    float reflectDrawTimer_ = 0.0f;
    VECTOR coinFocusTarget_ = VGet(0.0f, 0.0f, 0.0f);
    Enemy* reflectHitEnemy_ = nullptr;
};
