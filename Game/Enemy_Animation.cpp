#include "Enemy.h"
#include <string>

void Enemy::LoadAnimations()
{
    // load simple animation models for named animations
    // filenames follow convention: assets/models/mirai_<name>.mv1
    // e.g. idle, move, attack
    const char* names[] = { "idle", "move", "attack", nullptr };
    for (const char** p = names; *p; ++p) {
        std::string fn = std::string("assets/models/NX_") + *p + ".mv1";
        int h = MV1LoadModel(fn.c_str());
        animModelHandles_[*p] = h;
        if (h >= 0) {
            MV1SetScale(h, VGet(0.04f, 0.04f, 0.04f));
        }
    }
}

void Enemy::PlayAnimation(const std::string& name, bool loop)
{
    // If already the same animation name, ensure an attachment exists — reattach if missing.
    if (currentAnim_ == name) {
        if (attachedAnimAttachIndex_ < 0) {
            auto itRe = animModelHandles_.find(name);
            if (itRe != animModelHandles_.end() && itRe->second >= 0 && modelHandle_ >= 0) {
                attachedAnimAttachIndex_ = MV1AttachAnim(modelHandle_, itRe->second);
                if (attachedAnimAttachIndex_ >= 0) {
                    attachedAnimTotalTime_ = MV1GetAttachAnimTotalTime(modelHandle_, attachedAnimAttachIndex_);
                    MV1SetAttachAnimTime(modelHandle_, attachedAnimAttachIndex_, 0.0f);
                    MV1SetAttachAnimBlendRate(modelHandle_, attachedAnimAttachIndex_, 1.0f);
                }
            }
        }
        animLoop_ = loop;
        return;
    }
    // detach previous attach
    if (attachedAnimAttachIndex_ >= 0 && modelHandle_ >= 0) {
        MV1DetachAnim(modelHandle_, attachedAnimAttachIndex_);
        attachedAnimAttachIndex_ = -1;
    }

    auto it = animModelHandles_.find(name);
    if (it == animModelHandles_.end() || it->second < 0) {
        // missing anim model -> just set name and return
        currentAnim_ = name;
        animTime_ = 0.0f;
        animLoop_ = loop;
        attachedAnimTotalTime_ = 0.0f;
        return;
    }

    int animModel = it->second;
    if (modelHandle_ >= 0) {
        // attach animation
        attachedAnimAttachIndex_ = MV1AttachAnim(modelHandle_, animModel);
        if (attachedAnimAttachIndex_ >= 0) {
            attachedAnimTotalTime_ = MV1GetAttachAnimTotalTime(modelHandle_, attachedAnimAttachIndex_);
            // start at 0
            MV1SetAttachAnimTime(modelHandle_, attachedAnimAttachIndex_, 0.0f);
            // set blend rate to immediate for simplicity
            MV1SetAttachAnimBlendRate(modelHandle_, attachedAnimAttachIndex_, 1.0f);
        }
    }

    currentAnim_ = name;
    animTime_ = 0.0f;
    animLoop_ = loop;
}

void Enemy::UpdateAnimation(float dt)
{
    if (attachedAnimAttachIndex_ < 0 || modelHandle_ < 0) return;
    if (attachedAnimTotalTime_ <= 0.0f) return;

    animTime_ += dt;
    if (animLoop_) {
        while (animTime_ > attachedAnimTotalTime_) animTime_ -= attachedAnimTotalTime_;
    } else {
        if (animTime_ > attachedAnimTotalTime_) animTime_ = attachedAnimTotalTime_;
    }
    MV1SetAttachAnimTime(modelHandle_, attachedAnimAttachIndex_, animTime_);
}
