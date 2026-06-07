#include "Enemy.h"
#include <string>
#include <iostream>
#include <map>

void Enemy::LoadAnimations()
{
    // load animation models for named animations
    // Try NX_* naming first, then fall back to mirai naming if load fails.
    std::map<std::string, std::pair<std::string, std::string>> candidates;
    // pair: first = NX style, second = mirai style (exact case)
    candidates["idle"]  = { "assets/models/NX_idle.mv1",  "assets/models/mirai_Idle.mv1" };
    candidates["move"]  = { "assets/models/NX_move.mv1",  "assets/models/mirai_move.mv1" };
    candidates["attack"] = { "assets/models/NX_attack.mv1", "assets/models/mirai_attack.mv1" };

    for (auto &kv : candidates) {
        const std::string &name = kv.first;
        const std::string &fn1 = kv.second.first;
        const std::string &fn2 = kv.second.second;
        int h = MV1LoadModel(fn1.c_str());
        if (h < 0) {
            std::cerr << "Enemy: failed to load anim '" << name << "' from '" << fn1 << "'\n";
            // try fallback mirai
            h = MV1LoadModel(fn2.c_str());
            if (h < 0) {
                std::cerr << "Enemy: failed to load anim '" << name << "' from fallback '" << fn2 << "'\n";
            } else {
                std::cerr << "Enemy: loaded anim '" << name << "' from fallback '" << fn2 << "' (handle=" << h << ")\n";
            }
        } else {
            std::cerr << "Enemy: loaded anim '" << name << "' from '" << fn1 << "' (handle=" << h << ")\n";
        }
        animModelHandles_[name] = h;
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
                    if (attachedAnimTotalTime_ <= 0.0f) {
                        std::cerr << "Enemy: attach succeeded but total time is <= 0 for anim '" << name << "' (attachIndex=" << attachedAnimAttachIndex_ << ", totalTime=" << attachedAnimTotalTime_ << ")\n";
                    }
                    MV1SetAttachAnimTime(modelHandle_, attachedAnimAttachIndex_, 0.0f);
                    MV1SetAttachAnimBlendRate(modelHandle_, attachedAnimAttachIndex_, 1.0f);
                    std::cerr << "Enemy: attached anim '" << name << "' (attachIndex=" << attachedAnimAttachIndex_ << ", totalTime=" << attachedAnimTotalTime_ << ")\n";
                } else {
                    std::cerr << "Enemy: MV1AttachAnim failed for anim '" << name << "' (modelHandle=" << modelHandle_ << ", animModel=" << itRe->second << ")\n";
                }
            } else {
                std::cerr << "Enemy: no anim model available to attach for '" << name << "' (currentAnim_='" << currentAnim_ << "')\n";
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
        std::cerr << "Enemy: requested anim '" << name << "' not loaded (handle=" << (it==animModelHandles_.end() ? -1 : it->second) << ")\n";
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
            if (attachedAnimTotalTime_ <= 0.0f) {
                std::cerr << "Enemy: attached anim '" << name << "' but total time <= 0 (attachIndex=" << attachedAnimAttachIndex_ << ")\n";
            } else {
                std::cerr << "Enemy: attached anim '" << name << "' (attachIndex=" << attachedAnimAttachIndex_ << ", totalTime=" << attachedAnimTotalTime_ << ")\n";
            }
        }
        else {
            std::cerr << "Enemy: MV1AttachAnim failed for anim '" << name << "' (modelHandle=" << modelHandle_ << ", animModel=" << animModel << ")\n";
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
