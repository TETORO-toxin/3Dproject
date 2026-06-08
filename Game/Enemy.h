#pragma once
#include "DxLib.h"
#include <unordered_map>
#include <string>
#include <vector>

class NavMesh;

class Enemy
{
public:
    Enemy(const VECTOR& pos);
    ~Enemy();

    // ゲームロジック（物理/状態）だけを更新する。描画は Draw() に分離し、呼び出し側で描画順を制御できるようにする。
    void Update(float dt);
    void Draw() const;

    VECTOR GetPosition() const;

    // Nudge position by delta (used by scene manager separation pass)
    void NudgePosition(const VECTOR& delta);

    // Combat-related
    float GetWeakGauge() const { return weakGauge; }
    bool  IsWeak() const { return isWeak; }
    bool  IsAirborne() const { return airborne; }

    void ApplyHitWEAK(float weakGain);
    void Launch();
    void FinishAssault();
    // Animation
    void LoadAnimations();
    void PlayAnimation(const std::string& name, bool loop = false);
    void UpdateAnimation(float dt);
    void SetAnimationSpeed(const std::string& name, float speed) { animPlaybackSpeed_[name] = speed; }

    // Simple AI / movement target
    void SetTarget(const VECTOR& t) { targetPos_ = t; }
    void SetNavMesh(const NavMesh* nav) { navMesh_ = nav; }
    void RequestPathTo(const VECTOR& goal);

    // Line-of-sight check using NavMesh occupancy (samples along segment)
    bool HasLineOfSight(const VECTOR& a, const VECTOR& b) const;

private:
    int modelHandle_ = -1;
    VECTOR pos_;

    // Animation / model attachments
    std::unordered_map<std::string,int> animModelHandles_;
    // optional: map animation name -> animation index inside anim model
    std::unordered_map<std::string,int> animModelAnimIndex_;
    std::unordered_map<std::string,bool> attachAttempted_;
    // per-animation playback speed multiplier (1.0 = normal)
    std::unordered_map<std::string,float> animPlaybackSpeed_;
    int attachedAnimAttachIndex_ = -1;
    // When attachment is not possible, draw the anim model directly as a fallback.
    // Stores the anim model handle to draw instead of relying on attach API.
    int animDirectModelHandle_ = -1;
    float attachedAnimTotalTime_ = 0.0f;
    std::string currentAnim_;
    float animTime_ = 0.0f;
    bool animLoop_ = true;

    // orientation and movement
    float yaw_ = 0.0f;
    float moveSpeed_ = 3.5f;
    float turnSpeed_ = 8.0f;
    VECTOR targetPos_;
    // NavMesh path following
    const NavMesh* navMesh_ = nullptr;
    std::vector<VECTOR> path_;
    int currentPathIndex_ = 0;
    float pathRecalcTimer_ = 0.0f;

    // WEAK / airborne
    float weakGauge = 0.0f; // 0..100
    bool  isWeak = false;
    bool  airborne = false;

    // simple vertical velocity for lift during launch
    float velY = 0.0f;
};
