// Player_Animation.cpp
// �T�v:
//  - �A�j���[�V�����Đ��ƊǗ��̏�����؂�o�����t�@�C���ł��B
//  - ���C���[�Ή��i�S�g/�����g/�㔼�g�j�̍Đ��A�C�x���g�o�^�A���Ԑi�s�A�u�����h������S�����܂��B
//  - �e���C���[���ƂɃA�^�b�`�A�u�����h�A���ԍX�V���W�b�N���܂݂܂��B

#include "Player.h"
#include "CameraRig.h"
#include "../Sys/DebugPrint.h"
#include "../Sys/GlobalEffects.h"
#include <algorithm>

// PlayAnimation �́A�S�g�E�����g�E�㔼�g�̃A�j���[�V�����X���b�g���w�肷��I�v�V�����̃��C���[�p�����[�^�ɑΉ����܂����B
void Player::PlayAnimation(const std::string& name, bool loop, AnimLayer layer)
{
    // �A�j���[�V�������f��������
    auto it = animModelHandles_.find(name);
    if (it == animModelHandles_.end()) {
        DebugPrint("PlayAnimation: animation '%s' not found in animModelHandles_.\n", name.c_str());
        return;
    }

    int animModel = it->second;

    // �S�g�w��̏ꍇ�͉����g�Ƃ��Ĉ����A�㔼�g���C���[���N���A����
    if (layer == AnimLayer::Full) {
        // �㔼�g���C���[���N���A
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

    // �X���b�g�ɃA�^�b�`���邽�߂̃w���p�[�����_�i�����g�E�㔼�g�̗����Ŏg�p�j
    auto attachForLayer = [&](bool isUpper) {
        // ��ԎQ�Ƃ�I��
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
        float &blendSpeed = animBlendSpeed_; // �����œ������x

        // �Â� prev ���f�^�b�`�i���݂���ꍇ�j���Acurrent �� prev �Ɉړ�
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

        // �A�j�����f�����̃A�j���C���f�b�N�X������i�ۑ�����Ă���΁j
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

        // �x�[�X���f���ɃA�^�b�`
        int attachIdx = MV1AttachAnim(baseModelHandle_, 0, animModel, FALSE);
        DebugPrint("MV1AttachAnim -> attachIndex=%d baseModel=%d animModel=%d animIndex=%d\n", attachIdx, baseModelHandle_, animModel, animIndex);
        if (attachIdx >= 0) {
            attachIndex = attachIdx;
            float total = MV1GetAttachAnimTotalTime(baseModelHandle_, attachIndex);
            attachTotal = total > 0.0f ? total : 0.0f;
        }

        // ���^�f�[�^��ݒ�
        curName = name;
        curLoop = loop;
        timeSec = 0.0f;
        prevTimeSec = 0.0f;

        // �����g���C���[�̏ꍇ�̓C�x���g�����Z�b�g
        if (!isUpper) {
            auto evIt = animEvents_.find(curName);
            if (evIt != animEvents_.end()) { for (auto &ev : evIt->second) ev.fired = false; }
        }

        // �u�����h������
        blendRate = (prevAttachIndex == -1) ? 1.0f : 0.0f;
        if (prevAttachIndex != -1) blendRate = 0.0f; // �u�����h���� 0 ����J�n

        // �\�����郂�f���n���h��������F�A�^�b�`�������̓x�[�X��D��
        if (attachIndex != -1) {
            if (baseModelHandle_ != -1) modelHandle_ = baseModelHandle_;
        } else {
            modelHandle_ = animModel;
        }
    };

    if (layer == AnimLayer::Upper) {
        attachForLayer(true);
        // �㔼�g���C���[�ōU���p�̃G�t�F�N�g�C�x���g��o�^
        if (name == "attack") {
            ClearAnimEvents("attack");
            AddAnimEvent("attack", 0.05f, [this]() {
                DebugPrint("Player attack event fired\n");
                EffectManager* em = GetGlobalEffectManager();
                if (em) {
                    VECTOR p = GetPosition();
                    // �J������XZ���ʂ̑O������D�悵�A������� +Z �𗘗p
                    VECTOR forward = VGet(0.0f, 0.0f, 1.0f);
                    if (camera_) forward = camera_->GetForwardXZ();
                    float fl = sqrtf(forward.x*forward.x + forward.y*forward.y + forward.z*forward.z);
                    if (fl > 1e-6f) forward = VGet(forward.x/fl, forward.y/fl, forward.z/fl);
                    // �v���C���[�̑O��������ɏ��������ăG�t�F�N�g��z�u
                    VECTOR pos = VAdd(p, VAdd(VScale(forward, 1.4f), VGet(0.0f, 1.6f, 0.0f)));
                    DebugPrint("Player attack effect pos=%.2f,%.2f,%.2f\n", pos.x, pos.y, pos.z);
                    // Use the global default effect resource (configured by SceneMgr)
                    // to avoid relying on asset path differences. Pass nullptr to use
                    // the preloaded effect and an explicit scale.
                    em->PlayEffectAt(pos, nullptr, 1.0f);
                }
            });
        }
        return;
    }
    else if (layer == AnimLayer::Lower) {
        attachForLayer(false);
        return;
    }
    else { // �S�g�w��
        attachForLayer(false);
        if (name == "attack") {
            ClearAnimEvents("attack");
            AddAnimEvent("attack", 0.05f, [this]() {
                DebugPrint("Player attack event fired\n");
                EffectManager* em = GetGlobalEffectManager();
                if (em) {
                    VECTOR p = GetPosition();
                    VECTOR forward = VGet(0.0f, 0.0f, 1.0f);
                    if (camera_) forward = camera_->GetForwardXZ();
                    float fl = sqrtf(forward.x*forward.x + forward.y*forward.y + forward.z*forward.z);
                    if (fl > 1e-6f) forward = VGet(forward.x/fl, forward.y/fl, forward.z/fl);
                    VECTOR pos = VAdd(p, VAdd(VScale(forward, 1.4f), VGet(0.0f, 1.6f, 0.0f)));
                    DebugPrint("Player attack effect pos=%.2f,%.2f,%.2f\n", pos.x, pos.y, pos.z);
                    // Use the application's default effect resource rather than
                    // a hard-coded asset path which may not match the working dir.
                    em->PlayEffectAt(pos, nullptr, 1.0f);
                }
            });
        }
        return;
    }
}

// �U�����ɃG�t�F�N�g�C�x���g��o�^
// �����̌�ɒu�����ƂőO���錾�̖��������
#include "../Sys/GlobalEffects.h"

// PlayAnimation �ɏ����ȃw���p�[��t���Fattack ���J�n���ꂽ�ꍇ�A0.05 �b�̃C�x���g��ǉ�
// ������ AddAnimEvent ���Ăяo���ăt�b�N����iPlayAnimation ���ƂɃ����V���b�g�j�B
// ����: �{�v���W�F�N�g�ɓK�����ȈՓI�Ȏ����ł��B


// attack �A�j�����Đ����ꂽ�ꍇ�A0.05 �b�̃^�C�~���O�ŃG�t�F�N�g�𔭐������鏬���ȃC�x���g��o�^���܂��B
// ���̃V�X�e����傫���ς����ɃA�j���ƃG�t�F�N�g���߂Â��邽�߂̏����ł��B
// �A�v�����ŃO���[�o���G�t�F�N�g�}�l�[�W�����ݒ肳��Ă��邱�Ƃ��K�v�ł��B
// ����: �����A�j�����ɑ΂��ăC�x���g���d�����Ē~�ς���邽�߁A�����ł� name=="attack" �����C���[���㔼�g/�S�g�̂Ƃ��̂ݒǉ����܂��B

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
    // �����g���C���[�i���̃��W�b�N�A�����o���ɍ��킹�Ē����j
    if (!currentAnim_.empty()) {
        // �u�����h��i�s
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

            // �C�x���g����
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
                    if (currentAnim_ == "attack" || currentAnim_ == "attack_weapon") {
                        const bool hasWeapon = (equippedWeapon_ != Game::WeaponType::None);
                        PlayAnimation(hasWeapon ? "idle_weapon" : "idle", true);
                        return;
                    }
                }
            }
        }

        // �O�̉����g�A�j���[�V����
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

        // �A�^�b�`���Ȃ��ꍇ�͎��Ԃ�i�߁A�C�x���g�𔭉΂��A�K�v�Ȃ�A�j�����f�����Ԃ��X�V
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
                    if (currentAnim_ == "attack" || currentAnim_ == "attack_weapon") {
                        const bool hasWeapon = (equippedWeapon_ != Game::WeaponType::None);
                        PlayAnimation(hasWeapon ? "idle_weapon" : "idle", true);
                        return;
                    }
                }
            }
        }
    }

    // �㔼�g���C���[�F�A�^�b�`�X���b�g�ƃu�����h�𕪗�
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

            // �㔼�g���񃋁[�v�ŏI��������P���ɃN���A���ĉ��̃A�j����������悤�ɂ���
            const float finishEps = 1e-3f;
            if (!upperAnimLoop_ && upperAnimTime_ + finishEps >= configLen) {   
                // �㔼�g���f�^�b�`���ăN���A
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

        // �O�̏㔼�g�u�����h
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
