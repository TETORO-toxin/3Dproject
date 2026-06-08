#include "Enemy.h"
#include "Log.h"
#include <string>
#include <iostream>
#include <map>
#include <algorithm>
#include <cctype>
#include <vector>

void Enemy::LoadAnimations()
{
    // load animation models for named animations
    // Try NX_* naming first, then fall back to mirai naming if load fails.
    std::map<std::string, std::pair<std::string, std::string>> candidates;
    // pair: first = NX style, second = mirai style (exact case)
    candidates["idle"]  = { "assets/models/NX_idle.mv1",  "assets/models/mirai_Idle.mv1" };
    candidates["move"]  = { "assets/models/NX_move2.mv1",  "assets/models/mirai_move.mv1" };
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
        // Determine animation index inside the anim model so we can attach the correct anim
        if (h >= 0) {
            int animIndex = 0;
#ifdef MV1GetAnimIndex
            int idx = MV1GetAnimIndex(h, name.c_str());
            if (idx >= 0) {
                animIndex = idx;
            } else {
                // try some common candidate names
                std::vector<std::string> candidates;
                candidates.push_back(std::string("Armature|") + name);
                candidates.push_back(std::string("Armature.") + name);
                std::string cap = name; if (!cap.empty()) cap[0] = static_cast<char>(std::toupper((unsigned char)cap[0]));
                candidates.push_back(cap);
                candidates.push_back(name);

                bool matched = false;
                for (auto &cn : candidates) {
                    int idx2 = MV1GetAnimIndex(h, cn.c_str());
                    if (idx2 >= 0) { animIndex = idx2; matched = true; break; }
                }
#ifdef MV1GetAnimNum
#ifdef MV1GetAnimName
                if (!matched) {
                    int ac = MV1GetAnimNum(h);
                    std::string want = name; std::transform(want.begin(), want.end(), want.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
                    for (int ai = 0; ai < ac; ++ai) {
                        const char* aname = MV1GetAnimName(h, ai);
                        if (!aname) continue;
                        std::string s = aname; std::string low = s; std::transform(low.begin(), low.end(), low.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
                        if (low.find(want) != std::string::npos || (low.size() >= want.size() && low.compare(low.size()-want.size(), want.size(), want) == 0)) { animIndex = ai; matched = true; break; }
                    }
                }
#endif
#endif
                if (!matched) {
                    LogMsg(std::string("Enemy: could not find matching anim index for '") + name + "' in anim model " + std::to_string(h) + ", defaulting to 0");
                    animIndex = 0;
                }
            }
#else
            // MV1GetAnimIndex unavailable; default to 0
            animIndex = 0;
#endif
            animModelAnimIndex_[name] = animIndex;
        }
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
                // choose animIndex if provided, otherwise default to 0
                int animIndex = 0;
                auto aiRe = animModelAnimIndex_.find(name);
                if (aiRe != animModelAnimIndex_.end()) animIndex = aiRe->second;
                // try attaching with explicit anim index and disable auto-copy (match player implementation)
                attachedAnimAttachIndex_ = MV1AttachAnim(modelHandle_, animIndex, itRe->second, FALSE);
                if (attachedAnimAttachIndex_ >= 0) {
                    attachedAnimTotalTime_ = MV1GetAttachAnimTotalTime(modelHandle_, attachedAnimAttachIndex_);
                    if (attachedAnimTotalTime_ <= 0.0f) {
                        LogMsg(std::string("Enemy: attach succeeded but total time is <= 0 for anim '") + name + "' (attachIndex=" + std::to_string(attachedAnimAttachIndex_) + ", totalTime=" + std::to_string(attachedAnimTotalTime_) + ")");
                    }
                    MV1SetAttachAnimTime(modelHandle_, attachedAnimAttachIndex_, 0.0f);
                    MV1SetAttachAnimBlendRate(modelHandle_, attachedAnimAttachIndex_, 1.0f);
                    LogMsg(std::string("Enemy: attached anim '") + name + "' (animIndex=" + std::to_string(animIndex) + ", attachIndex=" + std::to_string(attachedAnimAttachIndex_) + ", totalTime=" + std::to_string(attachedAnimTotalTime_) + ")");
                } else {
                    LogMsg(std::string("Enemy: MV1AttachAnim failed for anim '") + name + "' (modelHandle=" + std::to_string(modelHandle_) + ", animModel=" + std::to_string(itRe->second) + ", animIndex=" + std::to_string(animIndex) + ")");
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
            // attempt attach (do not permanently mark as attempted on failure so we can retry)
            if (!attachAttempted_[name]) {
                int animIndex = 0;
                auto aiIt = animModelAnimIndex_.find(name);
                if (aiIt != animModelAnimIndex_.end()) animIndex = aiIt->second;
                attachedAnimAttachIndex_ = MV1AttachAnim(modelHandle_, animIndex, animModel, FALSE);
                if (attachedAnimAttachIndex_ >= 0) {
                    attachedAnimTotalTime_ = MV1GetAttachAnimTotalTime(modelHandle_, attachedAnimAttachIndex_);
                    // start at 0
                    MV1SetAttachAnimTime(modelHandle_, attachedAnimAttachIndex_, 0.0f);
                    // set blend rate to immediate for simplicity
                    MV1SetAttachAnimBlendRate(modelHandle_, attachedAnimAttachIndex_, 1.0f);
                    // clear any direct-draw fallback since attach succeeded
                    animDirectModelHandle_ = -1;
                    if (attachedAnimTotalTime_ <= 0.0f) {
                        LogMsg(std::string("Enemy: attached anim '") + name + "' but total time <= 0 (animIndex=" + std::to_string(animIndex) + ", attachIndex=" + std::to_string(attachedAnimAttachIndex_) + ")");
                    } else {
                        LogMsg(std::string("Enemy: attached anim '") + name + "' (animIndex=" + std::to_string(animIndex) + ", attachIndex=" + std::to_string(attachedAnimAttachIndex_) + ", totalTime=" + std::to_string(attachedAnimTotalTime_) + ")");
                    }
                    // mark that attach succeeded so we don't retry unnecessarily
                    attachAttempted_[name] = true;
                }
                else {
                    LogMsg(std::string("Enemy: MV1AttachAnim failed for anim '") + name + "' (modelHandle=" + std::to_string(modelHandle_) + ", animModel=" + std::to_string(animModel) + ", animIndex=" + std::to_string(animIndex) + ")");
                    // fallback: use anim model directly for drawing if attach fails
                    animDirectModelHandle_ = animModel;
                    // do NOT mark attachAttempted_ true so we will retry on next opportunity
                }
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
        float speed = 1.0f;
        auto spIt = animPlaybackSpeed_.find(currentAnim_);
        if (spIt != animPlaybackSpeed_.end() && spIt->second > 0.0f) speed = spIt->second;
        animTime_ += dt * speed;
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
        float speed = 1.0f;
        auto spIt = animPlaybackSpeed_.find(currentAnim_);
        if (spIt != animPlaybackSpeed_.end() && spIt->second > 0.0f) speed = spIt->second;
        animTime_ += dt * speed;
        float total = MV1GetAnimTotalTime(animDirectModelHandle_, 0);
        if (total > 0.0f) {
            if (animLoop_) {
                while (animTime_ > total) animTime_ -= total;
            } else {
                if (animTime_ > total) animTime_ = total;
            }
        }
        
    }
}
