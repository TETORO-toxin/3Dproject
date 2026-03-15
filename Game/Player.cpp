#include "Player.h"
#include "../Sys/Assets.h"
#include "../Sys/Input.h"
#include "../Sys/DebugPrint.h"
#include "AuxUnit.h"
#include "Projectile.h"
#include "Support.h"
#include "CameraRig.h"
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <cctype>
#include <cstdlib>

Player::Player(AssetsMgr* assets)
{
    // ベースモデル（アイドル）をロード
    if (assets) {
        baseModelHandle_ = assets->LoadModel("assets/models/mirai2.mv1");
        ownsBaseModel_ = false;
        modelHandle_ = baseModelHandle_;
    } else {
        baseModelHandle_ = MV1LoadModel("assets/models/mirai2.mv1");
        ownsBaseModel_ = true;
        modelHandle_ = baseModelHandle_;
    }

    // いくつかのアニメーションを登録（慣例: mirai_anim_<name>.mv1）
    std::vector<std::pair<std::string, std::string>> animFiles = {
        {"idle", "assets/models/mirai_Idle.mv1"},
        {"dodge", "assets/models/mirai_dodge.mv1"},
        {"move", "assets/models/mirai_move2.mv1"},
        {"jump", "assets/models/mirai_jump.mv1"},
        {"attack", "assets/models/mirai_attack.mv1"},
        {"aim", "assets/models/mirai_Aim.mv1"}
    };
    for (auto &p : animFiles) {
        int h = MV1LoadModel(p.second.c_str());
        if (h != -1) {
            // 利用可能なら、読み込んだMV1が実際にアニメーションモーションを含むか確認する。
            int animCount = -1;
#ifdef MV1GetAnimNum
            animCount = MV1GetAnimNum(h);
#endif
            if (animCount == 0) {
                // このMV1にモーションがない — スキップしてモデルを解放する
                DebugPrint("Warning: animation model '%s' contains 0 motions, skipping.\n", p.second.c_str());
                MV1DeleteModel(h);
                continue;
            }

            // APIが利用可能なら、含まれるモーション名を列挙して名前の不一致を診断する
#ifdef MV1GetAnimNum
#ifdef MV1GetAnimName
            if (animCount > 0) {
                DebugPrint("Anim model '%s' contains %d motions:\n", p.second.c_str(), animCount);
                for (int ai = 0; ai < animCount; ++ai) {
                    const char* aname = MV1GetAnimName(h, ai);
                    DebugPrint("  [%d] '%s'\n", ai, aname ? aname : "<null>");
                }
            }
#endif
#endif

            // 読み込んだアニメモデル内のモーションインデックスを決定。APIがないか名前が見つからなければデフォルト0。
            int motionIndex = 0;
#ifdef MV1GetAnimIndex
            {
                int idx = MV1GetAnimIndex(h, p.first.c_str());
                if (idx >= 0) {
                    motionIndex = idx;
                } else {
                    // 名前の不一致を軽減するための一般的なフォールバックを試行（例: 'Armature|Idle'、先頭大文字など）
                    std::vector<std::string> candidates;
                    candidates.push_back(std::string("Armature|") + p.first);
                    candidates.push_back(std::string("Armature.") + p.first);
                    std::string cap = p.first;
                    if (!cap.empty()) cap[0] = static_cast<char>(std::toupper((unsigned char)cap[0]));
                    candidates.push_back(cap);
                    candidates.push_back(p.first);

                    bool matched = false;
                    for (auto &cn : candidates) {
                        int idx2 = MV1GetAnimIndex(h, cn.c_str());
                        if (idx2 >= 0) {
                            motionIndex = idx2;
                            DebugPrint("Anim load: fallback matched '%s' -> '%s' index=%d\n", p.first.c_str(), cn.c_str(), idx2);
                            matched = true;
                            break;
                        }
                    }

                    // それでも一致しない場合、（APIがあれば）含まれるモーション名を大文字小文字無視の部分文字列チェックで走査
#ifdef MV1GetAnimNum
#ifdef MV1GetAnimName
                    if (!matched) {
                        int ac = MV1GetAnimNum(h);
                        std::string want = p.first;
                        std::transform(want.begin(), want.end(), want.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
                        for (int ai = 0; ai < ac; ++ai) {
                            const char* aname = MV1GetAnimName(h, ai);
                            if (!aname) continue;
                            std::string s = aname;
                            std::string low = s;
                            std::transform(low.begin(), low.end(), low.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
                            if (low.find(want) != std::string::npos || (low.size() >= want.size() && low.compare(low.size()-want.size(), want.size(), want) == 0)) {
                                motionIndex = ai;
                                DebugPrint("Anim load: scanned-match '%s' -> '%s' index=%d\n", p.first.c_str(), aname, ai);
                                matched = true;
                                break;
                            }
                        }
                    }
#endif
#endif

                    if (!matched) {
                        DebugPrint("Anim load: no matching anim name found for '%s' in '%s', defaulting to 0\n", p.first.c_str(), p.second.c_str());
                        motionIndex = 0;
                    }
                }
            }
#endif

            animModelHandles_.emplace(p.first, h);
            animModelAnimIndex_.emplace(p.first, motionIndex);

            // 可能なら実際のアニメーション長を読み取る。デフォルトは1.0秒
            float dur = 1.0f;
#ifdef MV1GetAnimTotalTime
            {
                float t = MV1GetAnimTotalTime(h, motionIndex);
                if (t > 0.0f) {
                    // 一部のビルドでは秒ではなくフレーム/ティックを返すことがある。
                    // 値がティックらしい場合（大きい）には60で割って秒に変換するヒューリスティックを使う。
                    if (t > 10.0f) {
                        DebugPrint("MV1GetAnimTotalTime for model '%s' motion %d returned %.3f - treating as ticks and converting to seconds by /60\n", p.second.c_str(), motionIndex, t);
                        t = t / 60.0f;
                    }
                    dur = t;
                }
            }
#endif
            animDurations_.emplace(p.first, dur);

            // moveアニメのループをやや短くしてより落ち着いた感じにする
            if (p.first == "move") {
                // moveループの間隔を短くしてループ間隔を短くする
                float newDur = dur * 2.5f;
                animDurations_["move"] = newDur;
                DebugPrint("Adjusted move anim duration from %.3f -> %.3f\n", dur, newDur);
            }

            // --- ここに idle の再生速度を遅くするための調整を追加 ---
            if (p.first == "idle") {
                // idle を遅くする（デフォルトの2倍の長さにして再生速度を半分にする）
                float newDur = dur * 2.0f;
                animDurations_["idle"] = newDur;
                DebugPrint("Adjusted idle anim duration from %.3f -> %.3f\n", dur, newDur);
            }

            // デバッグ: ロードしたアニメモデルとメタデータをログ出力
            DebugPrint("Loaded anim '%s' handle=%d motionIndex=%d dur=%.3f\n", p.first.c_str(), h, motionIndex, dur);

            // ここでベースモデルを差し替えない; アニメはベースモデルにアタッチされる
        }
    }

    // 可能ならフレーム/ボーン名を検査してアタッチ不一致を診断する
#ifdef MV1GetFrameNum
#ifdef MV1GetFrameName
    if (baseModelHandle_ != -1) {
        int baseFrames = MV1GetFrameNum(baseModelHandle_);
        DebugPrint("Base model frame count: %d\n", baseFrames);
        std::vector<std::string> baseNames;
        for (int i = 0; i < baseFrames; ++i) {
            const char* fn = MV1GetFrameName(baseModelHandle_, i);
            if (fn) baseNames.emplace_back(fn);
        }

        for (auto &kv : animModelHandles_) {
            int h = kv.second;
            if (h == -1) continue;
            int af = MV1GetFrameNum(h);
            DebugPrint("Anim model '%s' frame count: %d\n", kv.first.c_str(), af);
            std::vector<std::string> animNames;
            for (int i = 0; i < af; ++i) {
                const char* fn = MV1GetFrameName(h, i);
                if (fn) animNames.emplace_back(fn);
            }

            // 単純な共通要素数を計算（大文字小文字無視）
            auto toLower = [](const std::string &s){ std::string r = s; std::transform(r.begin(), r.end(), r.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); }); return r; };
            std::vector<std::string> baseLow, animLow;
            for (auto &s : baseNames) baseLow.push_back(toLower(s));
            for (auto &s : animNames) animLow.push_back(toLower(s));

            int common = 0;
            for (auto &an : animLow) {
                if (std::find(baseLow.begin(), baseLow.end(), an) != baseLow.end()) ++common;
            }
            DebugPrint("Anim model '%s' common frame names with base: %d / %d\n", kv.first.c_str(), common, (int)animNames.size());

            // 検査用に最大10件の欠落名を表示
            int printed = 0;
            for (size_t i = 0; i < animNames.size() && printed < 10; ++i) {
                std::string low = animLow[i];
                if (std::find(baseLow.begin(), baseLow.end(), low) == baseLow.end()) {
                    DebugPrint("  Missing in base: '%s'\n", animNames[i].c_str());
                    ++printed;
                }
            }
        }
    }
#endif
#endif

    // 環境変数でアニメモデルの強制描画を行い視覚的に確認する。
    // MSVCでは非推奨警告C4996を避けるため安全なgetenvバリアントを使用
#if defined(_MSC_VER)
    char* envVal = nullptr;
    size_t envLen = 0;
    if (_dupenv_s(&envVal, &envLen, "DEBUG_FORCE_DRAW_ANIM") == 0 && envVal != nullptr) {
        if (envVal[0] != '\0') {
            auto itf = animModelHandles_.find("idle");
            if (itf != animModelHandles_.end()) {
                DebugPrint("DEBUG_FORCE_DRAW_ANIM set - drawing anim model for 'idle' directly to confirm motion.\n");
                modelHandle_ = itf->second;
            }
        }
        free(envVal);
    }
#else
    const char* forceDraw = std::getenv("DEBUG_FORCE_DRAW_ANIM");
    if (forceDraw && forceDraw[0] != '\0') {
        auto itf = animModelHandles_.find("idle");
        if (itf != animModelHandles_.end()) {
            DebugPrint("DEBUG_FORCE_DRAW_ANIM set - drawing anim model for 'idle' directly to confirm motion.\n");
            modelHandle_ = itf->second;
        }
    }
#endif

    // 開始時にアイドルアニメが再生されるようにする
    PlayAnimation("idle", true);

    auxLeft = new AuxUnit(0, 0.2f, 3.0f, 60.0f); // 機関銃
    auxRight = new AuxUnit(1, 1.0f, 12.0f, 40.0f); // ミサイル
}

Player::~Player()
{
    // 現在と前回のアタッチアニメをデタッチ
    if (attachedAnimAttachIndex_ != -1) {
        MV1DetachAnim(baseModelHandle_, attachedAnimAttachIndex_);
        attachedAnimAttachIndex_ = -1;
        attachedAnimTotalTime_ = 0.0f;
    }
    if (prevAttachedAnimAttachIndex_ != -1) {
        MV1DetachAnim(baseModelHandle_, prevAttachedAnimAttachIndex_);
        prevAttachedAnimAttachIndex_ = -1;
        prevAttachedAnimTotalTime_ = 0.0f;
    }

    // upper layer detach
    if (upperAttachedAnimAttachIndex_ != -1) {
        MV1DetachAnim(baseModelHandle_, upperAttachedAnimAttachIndex_);
        upperAttachedAnimAttachIndex_ = -1;
        upperAttachedAnimTotalTime_ = 0.0f;
    }
    if (prevUpperAttachedAnimAttachIndex_ != -1) {
        MV1DetachAnim(baseModelHandle_, prevUpperAttachedAnimAttachIndex_);
        prevUpperAttachedAnimAttachIndex_ = -1;
        prevUpperAttachedAnimTotalTime_ = 0.0f;
    }

    // アニメモデルを削除
    for (auto &kv : animModelHandles_) {
        if (kv.second != -1) MV1DeleteModel(kv.second);
    }

    // 自分で所有しているベースモデルを削除
    if (ownsBaseModel_ && baseModelHandle_ != -1) {
        MV1DeleteModel(baseModelHandle_);
    }

    delete auxLeft; delete auxRight;
}

VECTOR Player::GetPosition() const
{
    return VGet(x_, y_, z_);
}

void Player::SetCamera(CameraRig* cam)
{
    camera_ = cam;
}

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
                prevConfigLen = prevUpperAttachedAnimTotalTime_;
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

void Player::OnIncomingAttack()
{
    lastIncomingTimeMs_ = GetNowCount();
}

bool Player::IsInvulnerable() const
{
    unsigned int now = GetNowCount();
    return (now - lastDodgeTimeMs_) < dodgeDurationMs_;
}

void Player::SetAwakened(bool v)
{
    awakened = v;
    if (v) {
        // バフパラメータ
        attackCooldownMs_ = baseAttackCooldownMs_ * 0.6f;
        auxGaugeRegenRate = baseAuxGaugeRegenRate * 3.0f;
        justWindowMs_ = baseJustWindowMs_ + 100;
    } else {
        attackCooldownMs_ = baseAttackCooldownMs_;
        auxGaugeRegenRate = baseAuxGaugeRegenRate;
        justWindowMs_ = baseJustWindowMs_;
    }
}

void Player::Heal(float amount)
{
    hp += amount;
    if (hp > maxHp) hp = maxHp;
}

void Player::UpdateAux(float dt, ProjectileManager& pm)
{
    if (auxLeft) auxLeft->Update(dt);
    if (auxRight) auxRight->Update(dt);

    // パッシブ回復
    if (!awakened) {
        auxGauge += auxGaugeRegenRate * dt;
        if (auxGauge > 100.0f) auxGauge = 100.0f;
    } else {
        // awakened状態: 発射コストなしだがゲージは上限で制限
        auxGauge += auxGaugeRegenRate * dt;
        if (auxGauge > 100.0f) auxGauge = 100.0f;
    }
}

void Player::FireAuxLeft(ProjectileManager& pm)
{
    if (!auxLeft) return;
    VECTOR origin = VAdd(GetPosition(), VGet(-1.2f, 1.6f, 0.0f));
    Enemy* target = nullptr;
    float cost = 10.0f;
    auxLeft->Fire(origin, target, pm, !awakened, auxGauge, cost);
}

void Player::FireAuxRight(ProjectileManager& pm)
{
    if (!auxRight) return;
    VECTOR origin = VAdd(GetPosition(), VGet(1.2f, 1.6f, 0.0f));
    Enemy* target = nullptr;
    float cost = 25.0f;
    auxRight->Fire(origin, target, pm, !awakened, auxGauge, cost);
}

// 新: ゲームロジックのみ更新（描画無し）
void Player::UpdateLogic()
{
    // 固定タイムステップ
    float dt = 1.0f / 60.0f;

    // コントローラまたはキーボード+マウスの統一入力
    InputState in = PollInput();

    // モード切替（例: A / space/Z）
    if (in.btnA) {
        unsigned int now = GetNowCount();
        if (now - lastModeSwitchTimeMs_ > 200) {
            lastModeSwitchTimeMs_ = now;
            mode_ = (mode_ == Mode::Melee) ? Mode::Ranged : Mode::Melee;
        }
    }

    // 回避（例: B / X）
    if (in.btnB) {
        unsigned int now = GetNowCount();
        if (now - lastDodgeTimeMs_ > dodgeCooldownMs_) {
            // ジャスト回避か判定
            if (now - lastIncomingTimeMs_ <= justWindowMs_) {
                justExecuted_ = true;
                // ジャスト回避ボーナス（短い前方テレポート）を適用
                x_ += 2.0f;
            }
            lastDodgeTimeMs_ = now;
        }
    }

    // 補助攻撃（左/右トリガーやマウスボタン）
    if (in.leftTrigger > 0.5f) {
        // 左補助発射 - 外部呼び出しを期待
    }
    if (in.rightTrigger > 0.5f) {
        // 右補助発射 - 外部呼び出しを期待
    }

    // 攻撃とジャンプ入力
    bool attackInput = false;
    bool jumpInput = false;
    // コントローラ割当
    if (in.btnX) attackInput = true;
    if (in.btnY) jumpInput = true;
    // マウス/キーボードの代替割当
    if (in.mouseLeft) attackInput = true;
    // キーボードジャンプ: モード切替と競合しないようCキーに割当
    if (CheckHitKey(KEY_INPUT_C)) jumpInput = true;

    // 左スティック/キーボードによる単純移動
    // 移動はカメラ基準: 前方入力でプレイヤーがカメラから遠ざかる（プレイヤーの背がカメラ向き）ように移動
    float moveX = in.moveX;
    float moveY = in.moveY;

    if (camera_ != nullptr) {
          VECTOR camF = camera_->GetForwardXZ(); // カメラからターゲット（プレイヤー）への正規化された前方ベクトル
          // プレイヤーの前方はカメラの前方とは逆向き（XZ平面）。前へ入力(moveY>0)でプレイヤーはカメラから離れる
          // XZ上の右ベクトルを計算
          VECTOR right = VGet(camF.z, 0.0f, -camF.x); // 90度回転
      
          // カメラ基準軸を使ってワールドXZの移動を合成。moveX=右, moveY=前
          float worldDX = right.x * moveX + (-camF.x) * moveY;
          float worldDZ = right.z * moveX + (-camF.z) * moveY;

          x_ += worldDX * 0.2f;
          z_ += worldDZ * 0.2f;

          // プレイヤーの向きを移動方向に合わせる
          {
              float faceDX = worldDX;
              float faceDZ = worldDZ;
              if (fabsf(faceDX) > 0.0001f || fabsf(faceDZ) > 0.0001f) {
                  float yaw = atan2f(faceDX, faceDZ);
                  // モデルハンドルに回転を適用
                  if (modelHandle_ != -1) MV1SetRotationXYZ(modelHandle_, VGet(0.0f, yaw + DX_PI_F, 0.0f));
                  if (baseModelHandle_ != -1 && baseModelHandle_ != modelHandle_) MV1SetRotationXYZ(baseModelHandle_, VGet(0.0f, yaw + DX_PI_F, 0.0f));
              }
          }
      } else {
          x_ += moveX * 0.2f;
          z_ += moveY * 0.2f;

          // プレイヤーの向きを移動方向に合わせる
          {
              float faceDX = moveX;
              float faceDZ = moveY;
              if (fabsf(faceDX) > 0.0001f || fabsf(faceDZ) > 0.0001f) {
                  float yaw = atan2f(faceDX, faceDZ);
                  if (modelHandle_ != -1) MV1SetRotationXYZ(modelHandle_, VGet(0.0f, yaw + DX_PI_F, 0.0f));
                  if (baseModelHandle_ != -1 && baseModelHandle_ != modelHandle_) MV1SetRotationXYZ(baseModelHandle_, VGet(0.0f, yaw + DX_PI_F, 0.0f));
              }
          }
      }
    // 地上にいる時、WASD/キーボード移動（またはコントローラ左スティック）でmoveアニメを再生
    bool moving = (fabsf(moveX) > 0.0001f || fabsf(moveY) > 0.0001f);
    if (onGround_) {
        // 攻撃やジャンプアニメを上書きしない
        if (currentAnim_ != "attack" && currentAnim_ != "jump") {
            if (moving) {
                if (currentAnim_ != "move") PlayAnimation("move", true, AnimLayer::Lower);
            } else {
                if (currentAnim_ != "idle") PlayAnimation("idle", true, AnimLayer::Lower);
            }
        }
    }
     
     // 攻撃入力の処理（移動より優先、ジャンプよりは劣後）
    unsigned int now = GetNowCount();
    if (attackInput) {
        if (now - lastAttackTimeMs_ > (unsigned int)attackCooldownMs_) {
            lastAttackTimeMs_ = now;
            // Play upper-body attack while keeping lower-body state (move/idle)
            PlayAnimation("attack", false, AnimLayer::Upper);
        }
    }

    // ジャンプ入力の処理
    if (jumpInput && onGround_) {
        unsigned int nowJ = GetNowCount();
        if (nowJ - lastJumpTimeMs_ > 200) {
            lastJumpTimeMs_ = nowJ;
            // ジャンプ開始
            velY_ = jumpVelocity_;
            onGround_ = false;
            // Jump is full-body, so play full animation (clears upper)
            PlayAnimation("jump", false, AnimLayer::Full);
        }
    }

    // 単純な垂直方向物理を統合
    if (!onGround_) {
        velY_ += gravity_ * dt;
        y_ += velY_ * dt;
        if (y_ <= 0.0f) {
            // 着地
            y_ = 0.0f;
            velY_ = 0.0f;
            onGround_ = true;
            // 着地時に移動またはアイドルアニメに復帰
            if (currentAnim_ != "attack") {
                // プレイヤーが移動中ならmove、そうでなければidle
                bool movingNow = (fabsf(in.moveX) > 0.0001f || fabsf(in.moveY) > 0.0001f);
                if (movingNow) PlayAnimation("move", true, AnimLayer::Lower); else PlayAnimation("idle", true, AnimLayer::Lower);
            }
        }
    }

    // アニメ更新（フレーム時間で進める）
    UpdateAnimation(dt);
}

// 新: 描画のみ（ゲームロジック無し）
void Player::Draw()
{
    // カメラ距離に基づいてフェードアルファを計算し、カメラがプレイヤー内部に入り込むのを回避
    int drawAlpha = 255;
    if (camera_ != nullptr) {
        VECTOR camPos = camera_->GetCameraPosition();

        // 簡易貫通判定: カメラがプレイヤーのローカルバウンディングボックス内なら完全に隠す
        float halfX = 0.6f * visualScale_; // 半幅
        float halfZ = 0.6f * visualScale_; // 半奥行
        float height = 1.6f * visualScale_; // 高さ

        float dx = camPos.x - x_;
        float dy = camPos.y - y_;
        float dz = camPos.z - z_;

        bool insideBox = (fabsf(dx) <= halfX * 0.9f) && (fabsf(dz) <= halfZ * 0.9f) && (dy >= 0.0f) && (dy <= height * 1.05f);
        if (insideBox) {
            drawAlpha = 0;
        } else {
            VECTOR toCam = VSub(camPos, GetPosition());
            float dist = VSize(toCam);
            const float fadeStart = 2.0f; // フェード開始距離
            const float fadeEnd = 0.6f;   // 完全透明になる距離
            if (dist < fadeStart) {
                float t = (dist - fadeEnd) / (fadeStart - fadeEnd);
                if (t < 0.0f) t = 0.0f;
                if (t > 1.0f) t = 1.0f;
                drawAlpha = (int)(255.0f * t + 0.5f);
            }
        }
    }

    // モデル/プリミティブ描画にアルファブレンドを適用
    DxLib::SetDrawBlendMode(DX_BLENDMODE_ALPHA, drawAlpha);

    // モデルを描画
    // アクティブなモデルハンドルを優先して描画する。MV1AttachModelが利用できない場合のフォールバックでは
    // 以前は`modelHandle_`をアニメモデルに差し替えていたため、最初に`baseModelHandle_`を描画するとフォールバックが見えなくなっていた。
    // コンストラクタで`modelHandle_`はベースに初期化されているため、これを主要な描画ハンドルとして使う。
    if (modelHandle_ != -1)
    {
        MV1SetPosition(modelHandle_, VGet(x_, y_, z_));
        // 均一スケールを適用
        MV1SetScale(modelHandle_, VGet(visualScale_, visualScale_, visualScale_));
        MV1DrawModel(modelHandle_);
    }
    else if (baseModelHandle_ != -1)
    {
        MV1SetPosition(baseModelHandle_, VGet(x_, y_, z_));
        // 均一スケールを適用
        MV1SetScale(baseModelHandle_, VGet(visualScale_, visualScale_, visualScale_));
        MV1DrawModel(baseModelHandle_);
    }
    else
    {
        // プレースホルダ: ワールド上の角を投影してプレイヤーの周りに3Dキューブを描く
        float halfX = 0.6f * visualScale_; // 半幅
        float halfZ = 0.6f * visualScale_; // 半奥行
        float height = 1.6f * visualScale_; // 高さ
        VECTOR base = VGet(x_, y_, z_);
        VECTOR corners[8];
        // 下面
        corners[0] = VAdd(base, VGet(-halfX, 0.0f, -halfZ));
        corners[1] = VAdd(base, VGet( halfX, 0.0f, -halfZ));
        corners[2] = VAdd(base, VGet( halfX, 0.0f,  halfZ));
        corners[3] = VAdd(base, VGet(-halfX, 0.0f,  halfZ));
        // 上面
        corners[4] = VAdd(base, VGet(-halfX, height, -halfZ));
        corners[5] = VAdd(base, VGet( halfX, height, -halfZ));
        corners[6] = VAdd(base, VGet( halfX, height,  halfZ));
        corners[7] = VAdd(base, VGet(-halfX, height,  halfZ));

        VECTOR scr[8];
        for (int i = 0; i < 8; ++i) scr[i] = ConvWorldPosToScreenPos(corners[i]);

        auto drawEdge = [&](int a, int b, unsigned int color){
            if (scr[a].z > 0.0f && scr[b].z > 0.0f) {
                DrawLine((int)scr[a].x, (int)scr[a].y, (int)scr[b].x, (int)scr[b].y, color);
            }
        };

        unsigned int colFill = GetColor(100,100,255);
        unsigned int colEdge = GetColor(255,255,255);
        // エッジ: 下面
        drawEdge(0,1,colEdge); drawEdge(1,2,colEdge); drawEdge(2,3,colEdge); drawEdge(3,0,colEdge);
        // エッジ: 上面
        drawEdge(4,5,colEdge); drawEdge(5,6,colEdge); drawEdge(6,7,colEdge); drawEdge(7,4,colEdge);
        // 縦線
        drawEdge(0,4,colEdge); drawEdge(1,5,colEdge); drawEdge(2,6,colEdge); drawEdge(3,7,colEdge);
    }

    // ブレンドを解除してその後のUI/テキスト描画に影響しないようにする
    DxLib::SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    DrawFormatString(10, 30, GetColor(255, 255, 255), "Mode: %s  Just:%s  AuxGauge: %.1f  HP: %.0f", mode_==Mode::Melee?"Melee":"Ranged", justExecuted_?"Yes":"No", auxGauge, hp);
}

// 下位互換: Updateはロジック->描画を呼ぶ
void Player::Update()
{
    UpdateLogic();
    Draw();
}
