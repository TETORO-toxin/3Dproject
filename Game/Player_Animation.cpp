// Player_Animation.cpp
// サマリー:
//  - アニメーション再生/管理に関する実装を分離したファイルです。
//  - アニメーションの再生（レイヤー対応）、イベント登録、アニメ時間進行、ブレンド処理などを担当します。
//  - 各レイヤー（lower/upper/full）ごとにアタッチ、ブレンド、時間更新のロジックが含まれます。

#include "Player.h"
#include "../Sys/DebugPrint.h"
#include <algorithm>

// PlayAnimation now supports an optional layer parameter to target full, lower or upper body animation slots.
void Player::PlayAnimation(const std::string& name, bool loop, AnimLayer layer)
{
    // find anim model
    auto it = animModelHandles_.find(name);
    if (it == animModelHandles_.end()) {
        DebugPrint("PlayAnimation: animation '%s' not found in animModelHandles_.\n", name.c_str());
        return;
    }

    int animModel = it->second;

    // If full-body requested, treat as lower-body play and clear upper layer
    if (layer == AnimLayer::Full) {
        // clear upper layer
        if (upperAttachedAnimAttachIndex_ != -1) {
            MV1DetachAnim(baseModelHandle_, upperAttachedAnimAttachIndex_);
            upperAttachedAnimAttachIndex_ = -1;
            upperAttachedAnimTotalTime_ = 0.0f;
            prevUpperAnim_.clear();
            prevUpperAttachedAnimAttachIndex_ = -1;
            prevUpperAttachedAnimTotalTime_ = 0.0f;
            upperAnim_ = "";
            upperAnimTime_ = 0.0f;
            upperAnimBlendRate_ = 1.0f;
        }
    }

    // Helper lambda to attach to a slot (used for both lower and upper)
    auto attachForLayer = [&](bool isUpper) {
        // Choose state references
        int &attachIndex = isUpper ? upperAttachedAnimAttachIndex_ : attachedAnimAttachIndex_;
        int &prevAttachIndex = isUpper ? prevUpperAttachedAnimAttachIndex_ : prevAttachedAnimAttachIndex_;
        float &attachTotal = isUpper ? upperAttachedAnimTotalTime_ : attachedAnimTotalTime_;
        float &prevAttachTotal = isUpper ? prevUpperAttachedAnimTotalTime_ : prevAttachedAnimTotalTime_;
        std::string &curName = isUpper ? upperAnim_ : currentAnim_;
        std::string &prevName = isUpper ? prevUpperAnim_ : prevAnimName_;
        float &timeSec = isUpper ? upperAnimTime_ : animTime_;
        float &prevTimeSec = isUpper ? prevUpperAnimTimeSeconds_ : prevAnimTimeSeconds_;
        bool &curLoop = isUpper ? upperAnimLoop_ : animLoop_;
        bool &prevLoop = isUpper ? prevUpperAnimLoop_ : prevAnimLoop_;
        float &blendRate = isUpper ? upperAnimBlendRate_ : animBlendRate_;
        float &blendSpeed = animBlendSpeed_; // same speed for both

        // Detach old prev (if any) then move current->prev
        if (prevAttachIndex != -1) {
            MV1DetachAnim(baseModelHandle_, prevAttachIndex);
            prevAttachIndex = -1;
            prevAttachTotal = 0.0f;
            prevName.clear();
            prevTimeSec = 0.0f;
        }

        prevAttachIndex = attachIndex;
        prevAttachTotal = attachTotal;
        prevName = curName;
        prevTimeSec = timeSec;
        prevLoop = curLoop;

        attachIndex = -1;
        attachTotal = 0.0f;

        // determine anim index in anim model (if stored)
        int animIndex = 0;
        auto idxIt = animModelAnimIndex_.find(name);
        if (idxIt != animModelAnimIndex_.end()) {
            animIndex = idxIt->second;
        }
#ifdef MV1GetAnimIndex
        else {
            int idx = MV1GetAnimIndex(animModel, name.c_str());
            if (idx >= 0) animIndex = idx;
            else {
                std::vector<std::string> candidates;
                candidates.push_back(std::string("Armature|") + name);
                candidates.push_back(std::string("Armature.") + name);
                std::string cap = name;
                if (!cap.empty()) cap[0] = static_cast<char>(std::toupper((unsigned char)cap[0]));
                candidates.push_back(cap);
                candidates.push_back(name);

                bool matched = false;
                for (auto &cn : candidates) {
                    int idx2 = MV1GetAnimIndex(animModel, cn.c_str());
                    if (idx2 >= 0) { animIndex = idx2; matched = true; break; }
                }
#ifdef MV1GetAnimNum
#ifdef MV1GetAnimName
                if (!matched) {
                    int ac = MV1GetAnimNum(animModel);
                    std::string want = name; std::transform(want.begin(), want.end(), want.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
                    for (int ai = 0; ai < ac; ++ai) {
                        const char* aname = MV1GetAnimName(animModel, ai);
                        if (!aname) continue;
                        std::string s = aname; std::string low = s; std::transform(low.begin(), low.end(), low.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
                        if (low.find(want) != std::string::npos || (low.size() >= want.size() && low.compare(low.size()-want.size(), want.size(), want) == 0)) { animIndex = ai; matched = true; break; }
                    }
                }
#endif
#endif
                if (!matched) {
                    DebugPrint("PlayAnimation: no matching anim name found for '%s' in anim model %d, defaulting to 0\n", name.c_str(), animModel);
                    animIndex = 0;
                }
            }
        }
#endif

        // Attach to base model
        int attachIdx = MV1AttachAnim(baseModelHandle_, 0, animModel, FALSE);
        DebugPrint("MV1AttachAnim -> attachIndex=%d baseModel=%d animModel=%d animIndex=%d\n", attachIdx, baseModelHandle_, animModel, animIndex);
        if (attachIdx >= 0) {
            attachIndex = attachIdx;
            float total = MV1GetAttachAnimTotalTime(baseModelHandle_, attachIndex);
            attachTotal = total > 0.0f ? total : 0.0f;
        }

        // set metadata
        curName = name;
        curLoop = loop;
        timeSec = 0.0f;
        prevTimeSec = 0.0f;

        // reset events if lower layer
        if (!isUpper) {
            auto evIt = animEvents_.find(curName);
            if (evIt != animEvents_.end()) { for (auto &ev : evIt->second) ev.fired = false; }
        }

        // blending init
        blendRate = (prevAttachIndex == -1) ? 1.0f : 0.0f;
        if (prevAttachIndex != -1) blendRate = 0.0f; // start from 0 when blending

        // decide visible model handle: prefer base when attach successful
        if (attachIndex != -1) {
            if (baseModelHandle_ != -1) modelHandle_ = baseModelHandle_;
        } else {
            modelHandle_ = animModel;
        }
    };

    if (layer == AnimLayer::Upper) {
        attachForLayer(true);
        return;
    }
    else if (layer == AnimLayer::Lower) {
        attachForLayer(false);
        return;
    }
    else { // Full
        attachForLayer(false);
        return;
    }
}

void Player::AddAnimEvent(const std::string& animName, float timeSec, std::function<void()> cb)
{
    AnimEvent ev; ev.time = timeSec; ev.fired = false; ev.cb = cb;
    animEvents_[animName].push_back(ev);
}

void Player::ClearAnimEvents(const std::string& animName)
{
    animEvents_.erase(animName);
}

void Player::UpdateAnimation(float dt)
{
    // LOWER LAYER (original logic, adapted to member names)
    if (!currentAnim_.empty()) {
        // advance blend
        if (animBlendRate_ < 1.0f) {
            animBlendRate_ += animBlendSpeed_;
            if (animBlendRate_ > 1.0f) animBlendRate_ = 1.0f;
        }

        if (attachedAnimAttachIndex_ != -1 && attachedAnimTotalTime_ > 0.0f) {
            prevAnimTime_ = animTime_;
            animTime_ += dt;

            float configLen = 1.0f;
            auto it = animDurations_.find(currentAnim_);
            if (it != animDurations_.end() && it->second > 0.0f) {
                configLen = it->second;
            } else if (attachedAnimAttachIndex_ != -1 && attachedAnimTotalTime_ > 0.0f) {
                configLen = attachedAnimTotalTime_;
            }
            if (configLen <= 0.0f) configLen = 1.0f;

            if (animLoop_) {
                if (animTime_ > configLen) animTime_ = fmodf(animTime_, configLen);
            } else {
                if (animTime_ > configLen) animTime_ = configLen;
            }

            // events
            auto evIt = animEvents_.find(currentAnim_);
            if (evIt != animEvents_.end()) {
                for (auto &ev : evIt->second) {
                    if (ev.fired && animLoop_) {
                        if (animTime_ < prevAnimTime_) ev.fired = false;
                    }
                    if (!ev.fired) {
                        if (prevAnimTime_ <= animTime_) {
                            if (ev.time >= prevAnimTime_ && ev.time <= animTime_) { ev.cb(); ev.fired = true; }
                        } else {
                            if (ev.time >= prevAnimTime_ || ev.time <= animTime_) { ev.cb(); ev.fired = true; }
                        }
                    }
                }
            }

            float ratio = animTime_ / configLen;
            if (ratio < 0.0f) ratio = 0.0f; if (ratio > 1.0f) ratio = 1.0f;
            float dxTime = ratio * attachedAnimTotalTime_;

            static float _debugAnimTickAccum = 0.0f;
            _debugAnimTickAccum += dt;
            if (_debugAnimTickAccum >= 0.5f) {
                DebugPrint("[AnimTick] name=%s loop=%d sec=%.3f/%.3f ratio=%.3f dxTime=%.3f attachTotal=%.3f blend=%.2f\n",
                           currentAnim_.c_str(), animLoop_ ? 1 : 0,
                           animTime_, configLen,
                           ratio,
                           dxTime,
                           attachedAnimTotalTime_, animBlendRate_);
                _debugAnimTickAccum = 0.0f;
            }

            MV1SetAttachAnimTime(baseModelHandle_, attachedAnimAttachIndex_, dxTime);
            MV1SetAttachAnimBlendRate(baseModelHandle_, attachedAnimAttachIndex_, animBlendRate_);

            const float finishedEps = 1e-3f;
            if (!animLoop_ && animTime_ + finishedEps >= configLen) {
                if (onGround_) {
                    if (currentAnim_ == "attack") {
                        PlayAnimation("idle", true);
                        return;
                    }
                }
            }
        }

        // prev lower
        if (prevAttachedAnimAttachIndex_ != -1 && prevAttachedAnimTotalTime_ > 0.0f) {
            prevAnimTimeSeconds_ += dt;

            float prevConfigLen = 1.0f;
            auto pit = animDurations_.find(prevAnimName_);
            if (pit != animDurations_.end() && pit->second > 0.0f) {
                prevConfigLen = pit->second;
            } else if (prevAttachedAnimAttachIndex_ != -1 && prevAttachedAnimTotalTime_ > 0.0f) {
                prevConfigLen = prevAttachedAnimTotalTime_;
            }
            if (prevConfigLen <= 0.0f) prevConfigLen = 1.0f;

            if (prevAnimLoop_) {
                if (prevAnimTimeSeconds_ > prevConfigLen) prevAnimTimeSeconds_ = fmodf(prevAnimTimeSeconds_, prevConfigLen);
            } else {
                if (prevAnimTimeSeconds_ > prevConfigLen) prevAnimTimeSeconds_ = prevConfigLen;
            }

            float pratio = prevAnimTimeSeconds_ / prevConfigLen;
            if (pratio < 0.0f) pratio = 0.0f; if (pratio > 1.0f) pratio = 1.0f;
            float pdxTime = pratio * prevAttachedAnimTotalTime_;
            MV1SetAttachAnimTime(baseModelHandle_, prevAttachedAnimAttachIndex_, pdxTime);

            MV1SetAttachAnimBlendRate(baseModelHandle_, prevAttachedAnimAttachIndex_, 1.0f - animBlendRate_);

            if (animBlendRate_ >= 1.0f) {
                MV1DetachAnim(baseModelHandle_, prevAttachedAnimAttachIndex_);
                prevAttachedAnimAttachIndex_ = -1;
                prevAttachedAnimTotalTime_ = 0.0f;
                prevAnimName_.clear();
                prevAnimTimeSeconds_ = 0.0f;
            }
        }

        // if no attach, advance time and fire events and optionally drive anim model time
        if (attachedAnimAttachIndex_ == -1) {
            prevAnimTime_ = animTime_;
            animTime_ += dt;
            float configLen = 1.0f;
            auto it = animDurations_.find(currentAnim_);
            if (it != animDurations_.end()) configLen = it->second;
            if (configLen <= 0.0f) configLen = 1.0f;
            if (animLoop_) { if (animTime_ > configLen) animTime_ = fmodf(animTime_, configLen); }
            else { if (animTime_ > configLen) animTime_ = configLen; }

            auto evIt = animEvents_.find(currentAnim_);
            if (evIt != animEvents_.end()) {
                for (auto &ev : evIt->second) {
                    if (ev.fired && animLoop_) {
                        if (animTime_ < prevAnimTime_) ev.fired = false;
                    }
                    if (!ev.fired) {
                        if (prevAnimTime_ <= animTime_) {
                            if (ev.time >= prevAnimTime_ && ev.time <= animTime_) { ev.cb(); ev.fired = true; }
                        } else {
                            if (ev.time >= prevAnimTime_ || ev.time <= animTime_) { ev.cb(); ev.fired = true; }
                        }
                    }
                }
            }

#ifdef MV1SetAnimTime
            auto mhIt = animModelHandles_.find(currentAnim_);
            if (mhIt != animModelHandles_.end() && mhIt->second != -1) {
                int amodel = mhIt->second;
                int aindex = 0;
                auto aiIt = animModelAnimIndex_.find(currentAnim_);
                if (aiIt != animModelAnimIndex_.end()) aindex = aiIt->second;
                float atime = animTime_;
                MV1SetAnimTime(amodel, aindex, atime);
            }
#endif

            const float finishedEps2 = 1e-3f;
            if (!animLoop_ && animTime_ + finishedEps2 >= configLen) {
                if (onGround_) {
                    if (currentAnim_ == "attack") {
                        PlayAnimation("idle", true);
                        return;
                    }
                }
            }
        }
    }

    // UPPER LAYER: separate attach slot and blending
    if (!upperAnim_.empty()) {
        if (upperAnimBlendRate_ < 1.0f) {
            upperAnimBlendRate_ += animBlendSpeed_;
            if (upperAnimBlendRate_ > 1.0f) upperAnimBlendRate_ = 1.0f;
        }

        if (upperAttachedAnimAttachIndex_ != -1 && upperAttachedAnimTotalTime_ > 0.0f) {
            prevUpperAnimTimeSeconds_ = upperAnimTime_;
            upperAnimTime_ += dt;

            float configLen = 1.0f;
            auto it = animDurations_.find(upperAnim_);
            if (it != animDurations_.end() && it->second > 0.0f) {
                configLen = it->second;
            } else if (upperAttachedAnimAttachIndex_ != -1 && upperAttachedAnimTotalTime_ > 0.0f) {
                configLen = upperAttachedAnimTotalTime_;
            }
            if (configLen <= 0.0f) configLen = 1.0f;

            if (upperAnimLoop_) {
                if (upperAnimTime_ > configLen) upperAnimTime_ = fmodf(upperAnimTime_, configLen);
            } else {
                if (upperAnimTime_ > configLen) upperAnimTime_ = configLen;
            }

            float ratio = upperAnimTime_ / configLen;
            if (ratio < 0.0f) ratio = 0.0f; if (ratio > 1.0f) ratio = 1.0f;
            float dxTime = ratio * upperAttachedAnimTotalTime_;
            MV1SetAttachAnimTime(baseModelHandle_, upperAttachedAnimAttachIndex_, dxTime);
            MV1SetAttachAnimBlendRate(baseModelHandle_, upperAttachedAnimAttachIndex_, upperAnimBlendRate_);

            // when upper non-loop finishes, simply clear to allow lower to show through
            const float finishEps = 1e-3f;
            if (!upperAnimLoop_ && upperAnimTime_ + finishEps >= configLen) {
                // detach upper and clear
                if (prevUpperAttachedAnimAttachIndex_ != -1) {
                    MV1DetachAnim(baseModelHandle_, prevUpperAttachedAnimAttachIndex_);
                    prevUpperAttachedAnimAttachIndex_ = -1;
                    prevUpperAttachedAnimTotalTime_ = 0.0f;
                    prevUpperAnim_.clear();
                    prevUpperAnimTimeSeconds_ = 0.0f;
                }
                if (upperAttachedAnimAttachIndex_ != -1) {
                    MV1DetachAnim(baseModelHandle_, upperAttachedAnimAttachIndex_);
                    upperAttachedAnimAttachIndex_ = -1;
                    upperAttachedAnimTotalTime_ = 0.0f;
                    upperAnim_.clear();
                    upperAnimTime_ = 0.0f;
                    upperAnimBlendRate_ = 1.0f;
                }
            }
        }

        // prev upper blending
        if (prevUpperAttachedAnimAttachIndex_ != -1 && prevUpperAttachedAnimTotalTime_ > 0.0f) {
            prevUpperAnimTimeSeconds_ += dt;

            float prevConfigLen = 1.0f;
            auto pit = animDurations_.find(prevUpperAnim_);
            if (pit != animDurations_.end() && pit->second > 0.0f) {
                prevConfigLen = pit->second;
            } else if (prevUpperAttachedAnimAttachIndex_ != -1 && prevUpperAttachedAnimTotalTime_ > 0.0f) {
                prevConfigLen = prevUpperAttachedAnimAttachIndex_;
            }
            if (prevConfigLen <= 0.0f) prevConfigLen = 1.0f;

            if (prevUpperAnimLoop_) {
                if (prevUpperAnimTimeSeconds_ > prevConfigLen) prevUpperAnimTimeSeconds_ = fmodf(prevUpperAnimTimeSeconds_, prevConfigLen);
            } else {
                if (prevUpperAnimTimeSeconds_ > prevConfigLen) prevUpperAnimTimeSeconds_ = prevConfigLen;
            }

            float pratio = prevUpperAnimTimeSeconds_ / prevConfigLen;
            if (pratio < 0.0f) pratio = 0.0f; if (pratio > 1.0f) pratio = 1.0f;
            float pdxTime = pratio * prevUpperAttachedAnimTotalTime_;
            MV1SetAttachAnimTime(baseModelHandle_, prevUpperAttachedAnimAttachIndex_, pdxTime);

            MV1SetAttachAnimBlendRate(baseModelHandle_, prevUpperAttachedAnimAttachIndex_, 1.0f - upperAnimBlendRate_);

            if (upperAnimBlendRate_ >= 1.0f) {
                MV1DetachAnim(baseModelHandle_, prevUpperAttachedAnimAttachIndex_);
                prevUpperAttachedAnimAttachIndex_ = -1;
                prevUpperAttachedAnimTotalTime_ = 0.0f;
                prevUpperAnim_.clear();
                prevUpperAnimTimeSeconds_ = 0.0f;
            }
        }
    }
}
