#include "Enemy.h"
#include "Log.h"
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
            LogMsg(std::string("Enemy: failed to load anim '") + name + "' from '" + fn1 + "'");
            // try fallback mirai (with provided variant)
            h = MV1LoadModel(fn2.c_str());
            if (h < 0) {
                // try additional common variants (lowercase/Capitalized)
                std::string alt1 = std::string("assets/models/NX_") + name + ".mv1";
                h = MV1LoadModel(alt1.c_str());
                if (h < 0) {
                    std::string cap = name;
                    if (!cap.empty()) cap[0] = (char)toupper((unsigned char)cap[0]);
                    std::string alt2 = std::string("assets/models/NX_") + cap + ".mv1";
                    h = MV1LoadModel(alt2.c_str());
                    if (h >= 0) {
                        LogMsg(std::string("Enemy: loaded anim '") + name + "' from variant '" + alt2 + "' (handle=" + std::to_string(h) + ")");
                    }
                } else {
                    LogMsg(std::string("Enemy: loaded anim '") + name + "' from variant '" + alt1 + "' (handle=" + std::to_string(h) + ")");
                }
                if (h < 0) {
                    LogMsg(std::string("Enemy: failed to load anim '") + name + "' from fallback '" + fn2 + "'");
                }
            } else {
                LogMsg(std::string("Enemy: loaded anim '") + name + "' from fallback '" + fn2 + "' (handle=" + std::to_string(h) + ")");
            }
        } else {
            LogMsg(std::string("Enemy: loaded anim '") + name + "' from '" + fn1 + "' (handle=" + std::to_string(h) + ")");
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
                        LogMsg(std::string("Enemy: attach succeeded but total time is <= 0 for anim '") + name + "' (attachIndex=" + std::to_string(attachedAnimAttachIndex_) + ", totalTime=" + std::to_string(attachedAnimTotalTime_) + ")");
                    }
                    MV1SetAttachAnimTime(modelHandle_, attachedAnimAttachIndex_, 0.0f);
                    MV1SetAttachAnimBlendRate(modelHandle_, attachedAnimAttachIndex_, 1.0f);
                    LogMsg(std::string("Enemy: attached anim '") + name + "' (attachIndex=" + std::to_string(attachedAnimAttachIndex_) + ", totalTime=" + std::to_string(attachedAnimTotalTime_) + ")");
                } else {
                    LogMsg(std::string("Enemy: MV1AttachAnim failed for anim '") + name + "' (modelHandle=" + std::to_string(modelHandle_) + ", animModel=" + std::to_string(itRe->second) + ")");
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
        // attach animation (only try once per anim to avoid flooding logs)
        if (!attachAttempted_[name]) {
            attachedAnimAttachIndex_ = MV1AttachAnim(modelHandle_, animModel);
            if (attachedAnimAttachIndex_ >= 0) {
                attachedAnimTotalTime_ = MV1GetAttachAnimTotalTime(modelHandle_, attachedAnimAttachIndex_);
                // start at 0
                MV1SetAttachAnimTime(modelHandle_, attachedAnimAttachIndex_, 0.0f);
                // set blend rate to immediate for simplicity
                MV1SetAttachAnimBlendRate(modelHandle_, attachedAnimAttachIndex_, 1.0f);
                // clear any direct-draw fallback since attach succeeded
                animDirectModelHandle_ = -1;
                if (attachedAnimTotalTime_ <= 0.0f) {
                    LogMsg(std::string("Enemy: attached anim '") + name + "' but total time <= 0 (attachIndex=" + std::to_string(attachedAnimAttachIndex_) + ")");
                } else {
                    LogMsg(std::string("Enemy: attached anim '") + name + "' (attachIndex=" + std::to_string(attachedAnimAttachIndex_) + ", totalTime=" + std::to_string(attachedAnimTotalTime_) + ")");
                }
            }
            else {
                LogMsg(std::string("Enemy: MV1AttachAnim failed for anim '") + name + "' (modelHandle=" + std::to_string(modelHandle_) + ", animModel=" + std::to_string(animModel) + ")");
                // fallback: use anim model directly for drawing if attach fails
                animDirectModelHandle_ = animModel;
            }
            attachAttempted_[name] = true;
        }
    }

    currentAnim_ = name;
    animTime_ = 0.0f;
    animLoop_ = loop;
}

void Enemy::UpdateAnimation(float dt)
{
    // If we have an attached animation, update its playhead.
    if (attachedAnimAttachIndex_ >= 0 && modelHandle_ >= 0) {
        animTime_ += dt;
        if (attachedAnimTotalTime_ > 0.0f) {
            if (animLoop_) {
                while (animTime_ > attachedAnimTotalTime_) animTime_ -= attachedAnimTotalTime_;
            } else {
                if (animTime_ > attachedAnimTotalTime_) animTime_ = attachedAnimTotalTime_;
            }
        }
        // Even if attachedAnimTotalTime_ <= 0, try to set time anyway.
        MV1SetAttachAnimTime(modelHandle_, attachedAnimAttachIndex_, animTime_);
        return;
    }

    // Fallback: if attach failed and we have an anim model, advance its internal animation
    // and draw it directly during Draw(). We still keep animTime_ and looping semantics.
    if (animDirectModelHandle_ >= 0) {
        animTime_ += dt;
        float total = MV1GetAnimTotalTime(animDirectModelHandle_, 0);
        if (total > 0.0f) {
            if (animLoop_) {
                while (animTime_ > total) animTime_ -= total;
            } else {
                if (animTime_ > total) animTime_ = total;
            }
        }
        // Note: DxLib in this build may not expose a function to set anim time
        // for standalone anim models. We still advance `animTime_` for bookkeeping
        // and draw the anim model directly in Draw(). If needed, add time control
        // when a suitable API is available.
    }
}
