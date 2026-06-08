#include "Enemy.h"
#include "Log.h"
#include "../Sys/Assets.h"
#include <cmath>
#include <algorithm>
#include <string>
#include <iostream>
#include <cstdio>
#ifdef _WIN32
#include <windows.h>
#endif
#include "NavMesh.h"
#include <vector>


static float NormalizeAngle(float a)
{
    while (a > 3.14159265f) a -= 6.2831853f;
    while (a < -3.14159265f) a += 6.2831853f;
    return a;
}

static float ApproachAngle(float current, float target, float maxDelta)
{
    float diff = NormalizeAngle(target - current);
    if (diff > maxDelta) diff = maxDelta;
    if (diff < -maxDelta) diff = -maxDelta;
    return NormalizeAngle(current + diff);
}

// DXLib functions used: MV1LoadModel, MV1SetPosition, MV1SetRotationXYZ, MV1DrawModel,
// MV1DeleteModel, MV1AttachAnim, MV1GetAttachAnimTotalTime, MV1SetAttachAnimTime,
// MV1DetachAnim

Enemy::Enemy(const VECTOR& pos)
    : pos_(pos)
{
    // try to load base model: prefer NX, fall back to mirai
    LogMsg("Enemy: constructor start");
    // Use AssetsMgr to obtain a duplicated instance of the shared base model.
    AssetsMgr& am = GetAssetsMgr();
    modelHandle_ = am.CreateEnemyModelInstance();
    if (modelHandle_ >= 0) {
        LogMsg(std::string("Enemy: created duplicated base model (handle=") + std::to_string(modelHandle_) + ")");
    } else {
        LogMsg("Enemy: failed to create duplicated base model from shared assets");
    }
    // scale down base model for visual size
    if (modelHandle_ >= 0) {
        MV1SetScale(modelHandle_, VGet(0.04f, 0.04f, 0.04f));
    }
    // load animations
    LoadAnimations();
    // default playback speed: make move animation slightly faster
    SetAnimationSpeed("move", 30.0f);
    attachedAnimAttachIndex_ = -1;
    currentAnim_ = "idle";
    animTime_ = 0.0f;
    animLoop_ = true;
    yaw_ = 0.0f;
    moveSpeed_ = 3.5f;
    targetPos_ = pos_;
    // start idle if model loaded
    if (modelHandle_ >= 0) PlayAnimation("idle", true);
}

void Enemy::RequestPathTo(const VECTOR& goal)
{
    if (!navMesh_) return;
    // If the goal cell is not walkable, find the nearest walkable cell and request
    // a path to that cell instead. This prevents freezing pursuit when the target
    // (player) stands on an impassable node ? enemy will approach as close as
    // possible rather than stop chasing entirely.
    int gx, gz;
    VECTOR adjustedGoal = goal;
    if (navMesh_->WorldToCell(goal, gx, gz)) {
        if (!navMesh_->IsWalkable(gx, gz)) {
            const int maxRadius = 10; // search up to this many cells away
            bool found = false;
            for (int r = 1; r <= maxRadius && !found; ++r) {
                for (int dx = -r; dx <= r && !found; ++dx) {
                    for (int dz = -r; dz <= r && !found; ++dz) {
                        // only consider cells on current ring (chebyshev distance)
                        if ((std::max)(std::abs(dx), std::abs(dz)) != r) continue;
                        int cx = gx + dx;
                        int cz = gz + dz;
                        if (navMesh_->IsWalkable(cx, cz)) {
                            adjustedGoal = navMesh_->CellToWorld(cx, cz);
                            found = true;
                        }
                    }
                }
            }
            // if none found within radius, adjustedGoal remains original and FindPath
            // will likely return empty; that's acceptable fallback.
        }
    }

    path_ = navMesh_->FindPath(pos_, adjustedGoal);
    currentPathIndex_ = 0;
}

bool Enemy::HasLineOfSight(const VECTOR& a, const VECTOR& b) const
{
    // If no NavMesh available, assume visible
    if (!navMesh_) return true;

    // sample along segment a->b and ensure sampled cells are walkable
    const int samples = 8;
    for (int i = 1; i < samples; ++i) {
        float t = (float)i / (float)samples;
        VECTOR p = VGet(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t);
        int cx, cz;
        if (!navMesh_->WorldToCell(p, cx, cz)) return false;
        if (!navMesh_->IsWalkable(cx, cz)) return false;
    }
    return true;
}

Enemy::~Enemy()
{
    // delete loaded models if owned
    // modelHandle_ is a duplicated instance created via AssetsMgr::CreateEnemyModelInstance
    if (modelHandle_ >= 0) MV1DeleteModel(modelHandle_);
    // animModelHandles_ store shared cached handles from AssetsMgr; do NOT delete them here.
}

void Enemy::Update(float dt)
{
    // airborne vertical physics
    if (airborne) {
        velY -= 9.8f * dt; // 重力
        pos_.y += velY * dt;
        if (pos_.y <= 0.0f) {
            pos_.y = 0.0f;
            airborne = false;
            velY = 0.0f;
        }
    }
    // ensure current animation is attached; retry attach if missing
    if (attachedAnimAttachIndex_ == -1 && modelHandle_ >= 0 && !currentAnim_.empty()) {
        PlayAnimation(currentAnim_, animLoop_);

        // limited debug logging when attach still missing
        static std::string lastMissingAnim;
        if (attachedAnimAttachIndex_ == -1 && lastMissingAnim != currentAnim_) {
            std::cerr << "Enemy: attach missing for anim '" << currentAnim_ << "' (modelHandle=" << modelHandle_ << ")\n";
            for (auto &p : animModelHandles_) {
                std::cerr << "  anim candidate '" << p.first << "' -> handle=" << p.second << "\n";
            }
            lastMissingAnim = currentAnim_;
        }
    }

    // path-following: if navMesh_ is set, periodically request path to targetPos_
    pathRecalcTimer_ -= dt;
    if (navMesh_ && pathRecalcTimer_ <= 0.0f) {
        pathRecalcTimer_ = 0.2f; // more frequent path updates for smoother pursuit
        RequestPathTo(targetPos_);
    }

    // choose a move target from the path, preferring the farthest visible waypoint
    // By default we won't blindly move straight to the world-space target when a NavMesh
    // is present but no path could be found; that caused enemies to walk through
    // non-walkable nodes. If navMesh_ exists and path_ is empty, freeze movement.
    VECTOR moveTarget = targetPos_;
    int bestIndex = currentPathIndex_;
    if (!path_.empty() && currentPathIndex_ < (int)path_.size()) {
        const int maxAdvance = 4; // try up to 4 waypoints ahead
        int lastIndex = (int)path_.size() - 1;
        for (int i = currentPathIndex_; i <= lastIndex && i <= currentPathIndex_ + maxAdvance; ++i) {
            // if this waypoint is visible from our position, prefer it
            if (HasLineOfSight(pos_, path_[i])) {
                bestIndex = i;
            }
        }
        moveTarget = path_[bestIndex];
    }

    // If NavMesh is available but no path was found, do not attempt to move directly
    // toward the world-space target (which may lie behind blocked cells). This prevents
    // enemies from walking through non-walkable nodes.
    if (navMesh_ && path_.empty()) {
        moveTarget = pos_;
    }

    // simple AI: move towards moveTarget on XZ plane
    VECTOR diff = VSub(moveTarget, pos_);
    float dx = diff.x;
    float dz = diff.z;
    float distXZ = sqrtf(dx*dx + dz*dz);
    // snap to target when very close to avoid tiny residual motion preventing idle
    const float arrivalThreshold = 0.6f; // slightly more forgiving than before
    if (distXZ <= arrivalThreshold) {
        if (!path_.empty() && currentPathIndex_ < (int)path_.size() - 1) {
            // if we could see further ahead, jump the path index forward
            // bestIndex was computed above; advance to it if appropriate
            if (bestIndex > currentPathIndex_) currentPathIndex_ = bestIndex;
            else currentPathIndex_++;
        } else {
            if (currentAnim_ != "idle") PlayAnimation("idle", true);
        }
    } else if (distXZ > 0.01f) {
        float nx = dx / distXZ;
        float nz = dz / distXZ;
        float move = moveSpeed_ * dt;
        if (move > distXZ) move = distXZ;
        pos_.x += nx * move;
        pos_.z += nz * move;
        // face target direction (keep looking toward overall target rather than instantaneous move)
        VECTOR lookDiff = VSub(targetPos_, pos_);
        float ldx = lookDiff.x;
        float ldz = lookDiff.z;
        float lookDist = sqrtf(ldx*ldx + ldz*ldz);
        if (lookDist > 0.001f) {
            float targetYaw = atan2f(ldx, ldz) + 3.14159265f; // flip so model faces forward
            yaw_ = ApproachAngle(yaw_, targetYaw, turnSpeed_ * dt);
        }
        // play move anim
        if (currentAnim_ != "move") PlayAnimation("move", true);
    } else {
        if (currentAnim_ != "idle") PlayAnimation("idle", true);
    }

    // update animation playhead
    UpdateAnimation(dt);
}

void Enemy::Draw() const
{
    // If attach failed for current animation, we may draw the anim model directly as a fallback.
    if (animDirectModelHandle_ >= 0) {
        MV1SetPosition(animDirectModelHandle_, pos_);
        VECTOR rot = VGet(0.0f, yaw_, 0.0f);
        MV1SetRotationXYZ(animDirectModelHandle_, rot);
        MV1DrawModel(animDirectModelHandle_);
        return;
    }

    if (modelHandle_ >= 0) {
        // set position and rotation then draw
        MV1SetPosition(modelHandle_, pos_);
        VECTOR rot = VGet(0.0f, yaw_, 0.0f);
        MV1SetRotationXYZ(modelHandle_, rot);
        MV1DrawModel(modelHandle_);
    } else {
        // fallback: draw box
        const float halfX = 0.5f;
        const float halfZ = 0.5f;
        const float height = 2.0f;

        VECTOR base = pos_; // base center
        VECTOR corners[8]{};
        // bottom face
        corners[0] = VAdd(base, VGet(-halfX, 0.0f, -halfZ));
        corners[1] = VAdd(base, VGet( halfX, 0.0f, -halfZ));
        corners[2] = VAdd(base, VGet( halfX, 0.0f,  halfZ));
        corners[3] = VAdd(base, VGet(-halfX, 0.0f,  halfZ));
        // top face
        corners[4] = VAdd(base, VGet(-halfX, height, -halfZ));
        corners[5] = VAdd(base, VGet( halfX, height, -halfZ));
        corners[6] = VAdd(base, VGet( halfX, height,  halfZ));
        corners[7] = VAdd(base, VGet(-halfX, height,  halfZ));

        unsigned int colEdge = isWeak ? GetColor(200,200,255) : GetColor(255,100,100);

        auto drawEdge3D = [&](int a, int b){
            DrawLine3D(corners[a], corners[b], colEdge);
        };

        // bottom
        drawEdge3D(0,1); drawEdge3D(1,2); drawEdge3D(2,3); drawEdge3D(3,0);
        // top
        drawEdge3D(4,5); drawEdge3D(5,6); drawEdge3D(6,7); drawEdge3D(7,4);
        // verticals
        drawEdge3D(0,4); drawEdge3D(1,5); drawEdge3D(2,6); drawEdge3D(3,7);
    }
}

VECTOR Enemy::GetPosition() const { return pos_; }

void Enemy::NudgePosition(const VECTOR& delta)
{
    pos_.x += delta.x;
    pos_.y += delta.y;
    pos_.z += delta.z;
}

void Enemy::ApplyHitWEAK(float weakGain)
{
    if (!isWeak) {
        // WEAK ゲージを加算
        weakGauge = weakGauge + weakGain;
        if (weakGauge > 100.0f) weakGauge = 100.0f;
        if (weakGauge >= 100.0f) {
            isWeak = true;
            // TODO: エフェクト生成
        }
    }
}

void Enemy::Launch()
{
    if (isWeak && !airborne) {
        airborne = true;
        velY = 12.0f; // initial upward velocity
    }
}

void Enemy::FinishAssault()
{
    if (airborne) {
        // big damage / down
        airborne = false;
        isWeak = false;
        weakGauge = 0.0f;
        pos_.y = 0.0f; // ensure grounded
        velY = 0.0f;
    }
}
