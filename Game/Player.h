#pragma once
#include "DxLib.h"
#include <array>
#include <unordered_map>
#include <string>
#include <functional>
#include <vector>

class AssetsMgr;
class ProjectileManager;
struct AuxUnit;
class CameraRig;

class Player
{
public:
    enum class Mode { Melee, Ranged };

    Player(AssetsMgr* assets = nullptr);
    ~Player();
    void Update();

    // Split update into logic and draw so scene manager can control draw order
    void UpdateLogic();
    void Draw();

    VECTOR GetPosition() const;

    // allow Scene/Camera to inform player about the camera for camera-relative movement/facing
    void SetCamera(CameraRig* cam);

    // Combat API
    void OnIncomingAttack(); // notify an incoming attack for just-dodge timing
    bool IsInvulnerable() const;

    // Aux units
    void UpdateAux(float dt, ProjectileManager& pm);
    void FireAuxLeft(ProjectileManager& pm);
    void FireAuxRight(ProjectileManager& pm);

    // Support/Status
    void SetAwakened(bool v);
    void Heal(float amount);
    float GetHP() const { return hp; }

    // Animation (uses separate MV1 models per animation for compatibility)
    // NOTE: animations are played against the original/base model instead of swapping the active model handle.
    void PlayAnimation(const std::string& name, bool loop = false);
    void UpdateAnimation(float dt);

    // Animation event helpers: call cb when animation time (seconds) is reached
    void AddAnimEvent(const std::string& animName, float timeSec, std::function<void()> cb);
    void ClearAnimEvents(const std::string& animName);

private:
    // original base MV1 model handle (the visible model). Animation models are kept separately and should be applied to this base model.
    int baseModelHandle_ = -1; // original model (do not swap this out)

    // current active MV1 model handle (kept equal to baseModelHandle_ in the new approach)
    int modelHandle_ = -1; // active model handle
    std::unordered_map<std::string,int> animModelHandles_; // map animation name -> MV1 model handle
    // map animation name -> motion index inside the animation MV1 (helps avoid calling MV1GetAnimIndex at runtime)
    std::unordered_map<std::string,int> animModelAnimIndex_;

    float x_ = 0.0f, y_ = 0.0f, z_ = 0.0f;

    // visual scale for the player
    float visualScale_ = 0.04f; // further reduced size

    // HP
    float hp = 100.0f;
    float maxHp = 100.0f;

    // Mode
    Mode mode_ = Mode::Melee;
    unsigned int lastModeSwitchTimeMs_ = 0;

    // Attack/dodge timing (ms)
    unsigned int lastAttackTimeMs_ = 0;
    float baseAttackCooldownMs_ = 300.0f;
    float attackCooldownMs_ = 300.0f;

    unsigned int lastDodgeTimeMs_ = 0;
    unsigned int dodgeCooldownMs_ = 800;
    unsigned int dodgeDurationMs_ = 300; // iframe duration

    // Just dodge detection
    unsigned int lastIncomingTimeMs_ = 0;
    unsigned int baseJustWindowMs_ = 200;
    unsigned int justWindowMs_ = 200;

    // state
    bool justExecuted_ = false;

    // Aux units (left/right)
    AuxUnit* auxLeft = nullptr;
    AuxUnit* auxRight = nullptr;

    // gauge for aux firing / awakening
    float auxGauge = 100.0f; // 0..100, natural regen
    float baseAuxGaugeRegenRate = 5.0f; // per second
    float auxGaugeRegenRate = 5.0f; // current rate
    bool  awakened = false; // awakened = no cost to fire

    // Animation timing and events (per-animation duration is configurable)
    std::unordered_map<std::string, float> animDurations_; // seconds
    std::string currentAnim_;
    float animTime_ = 0.0f; // seconds
    bool animLoop_ = true;

    struct AnimEvent { float time; bool fired; std::function<void()> cb; };
    std::unordered_map<std::string, std::vector<AnimEvent>> animEvents_;
    float prevAnimTime_ = 0.0f;

    // camera reference for camera-relative movement/facing
    CameraRig* camera_ = nullptr;

    // currently attached animation attach index (returned by MV1AttachAnim), or -1 if none
    int attachedAnimAttachIndex_ = -1;
    // previously attached animation attach index (for blending)
    int prevAttachedAnimAttachIndex_ = -1;

    // DXLib attach-animation total time for current and previous attachments
    float attachedAnimTotalTime_ = 0.0f;
    float prevAttachedAnimTotalTime_ = 0.0f;

    // store previous anim name and its seconds-based playhead for event/length lookup
    std::string prevAnimName_;
    float prevAnimTimeSeconds_ = 0.0f;
    bool prevAnimLoop_ = true;

    // blending control
    float animBlendRate_ = 1.0f; // 0..1, goes to 1 when blend completes
    float animBlendSpeed_ = 0.12f; // per-frame increment

    // whether this object owns the base model handle (loaded directly instead of via AssetsMgr)
    bool ownsBaseModel_ = false;

    // --- new: simple vertical physics for jump support ---
    float velY_ = 0.0f;           // vertical velocity (units/sec)
    bool onGround_ = true;        // whether player is on ground
    float gravity_ = -20.0f;      // gravity accel (units/sec^2)
    float jumpVelocity_ = 8.0f;   // initial jump velocity (units/sec)

    // timing helper for jump input (debounce)
    unsigned int lastJumpTimeMs_ = 0;
};
