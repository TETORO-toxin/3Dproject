#include "Enemy.h"
#include <cmath>
#include <string>
#include <iostream>

// DXLib functions used: MV1LoadModel, MV1SetPosition, MV1SetRotationXYZ, MV1DrawModel,
// MV1DeleteModel, MV1AttachAnim, MV1GetAttachAnimTotalTime, MV1SetAttachAnimTime,
// MV1DetachAnim

Enemy::Enemy(const VECTOR& pos)
    : pos_(pos)
{
    // try to load base model: prefer NX, fall back to mirai
    modelHandle_ = MV1LoadModel("assets/models/NX.mv1");
    if (modelHandle_ < 0) {
        // try mirai fallback
        modelHandle_ = MV1LoadModel("assets/models/mirai.mv1");
        if (modelHandle_ >= 0) {
            std::cerr << "Enemy: loaded base model from 'assets/models/mirai.mv1' (handle=" << modelHandle_ << ")\n";
        } else {
            std::cerr << "Enemy: failed to load base model from NX and mirai\n";
        }
    } else {
        std::cerr << "Enemy: loaded base model from 'assets/models/NX.mv1' (handle=" << modelHandle_ << ")\n";
    }
    // scale down base model for visual size
    if (modelHandle_ >= 0) {
        MV1SetScale(modelHandle_, VGet(0.04f, 0.04f, 0.04f));
    }
    // load animations
    LoadAnimations();
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

Enemy::~Enemy()
{
    // delete loaded models if owned
    if (modelHandle_ >= 0) MV1DeleteModel(modelHandle_);
    for (auto &p : animModelHandles_) {
        if (p.second >= 0) MV1DeleteModel(p.second);
    }
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
    // check if current animation needs to be reattached
    if (currentAnim_ == "move" && attachedAnimAttachIndex_ == -1) {
        PlayAnimation("move", true);
    }

    // simple AI: move towards targetPos_ on XZ plane
    VECTOR diff = VSub(targetPos_, pos_);
    float dx = diff.x;
    float dz = diff.z;
    float distXZ = sqrtf(dx*dx + dz*dz);
    // snap to target when very close to avoid tiny residual motion preventing idle
    if (distXZ <= 0.05f) {
        pos_.x = targetPos_.x;
        pos_.z = targetPos_.z;
        if (currentAnim_ != "idle") PlayAnimation("idle", true);
    } else if (distXZ > 0.01f) {
        float nx = dx / distXZ;
        float nz = dz / distXZ;
        float move = moveSpeed_ * dt;
        if (move > distXZ) move = distXZ;
        pos_.x += nx * move;
        pos_.z += nz * move;
        // face movement direction (yaw around Y)
        yaw_ = atan2f(nx, nz);
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
