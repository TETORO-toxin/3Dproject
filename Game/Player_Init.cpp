// Player_Init.cpp
// �T�}���[:
//  - �v���C���[�̐���/�j���Ɋւ���������܂Ƃ߂��t�@�C���ł��B
//  - ���f���ƃA�j���[�V�������f���̓ǂݍ��݁A�A�j���[�V�������^�f�[�^�̏������A
//    �f�o�b�O�����̊��ϐ��`�F�b�N�ȂǁA���\�[�X�������Ɋւ�鏈����S���܂��B
//  - �f�X�g���N�^�ł͊e��A�^�b�`�����ƃ��\�[�X������s���܂��B

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

// �R���X�g���N�^
//  - �A�Z�b�g�}�l�[�W�����^�����Ă���΂�����g���ăx�[�X���f�����擾����B
//  - ��`�ς݂̃A�j���[�V����MV1�t�@�C����񋓂��ēǂݍ��݁A���[�V�����C���f�b�N�X��Đ����Ԃ𐄒肵�Ċi�[����B
//  - �t���[��/�{�[�����̍��ق�f�f���邽�߂̃��M���O���s���iAPI�����݂���ꍇ�j�B
//  - ���ϐ�`DEBUG_FORCE_DRAW_ANIM`���ݒ肳��Ă���ꍇ�́A�x�[�X���f���̑���ɃA�j�����f���𒼐ڕ`�悷��I�v�V������L���ɂ���B
//  - �Ō�ɕ⏕���j�b�g�𐶐�����B
Player::Player(AssetsMgr* assets)
{
    // �A�Z�b�g�}�l�[�W���Q�Ƃ�ێ�
    assets_ = assets;
    DebugPrint("Player ctor: entered\n");

    // �E��t���[���T����⃊�X�g�̓w�b�_�̃e�[�u���𗘗p
    // �x�[�X���f���i�A�C�h���j�����[�h
    if (assets) {
        baseModelHandle_ = assets->LoadModel("assets/models/mirai2.mv1");
        ownsBaseModel_ = false;
        modelHandle_ = baseModelHandle_;
    } else {
        baseModelHandle_ = MV1LoadModel("assets/models/mirai2.mv1");
        ownsBaseModel_ = true;
        modelHandle_ = baseModelHandle_;
    }
    DebugPrint("Player ctor: baseModelHandle=%d ownsBaseModel=%d modelHandle=%d\n", baseModelHandle_, (int)ownsBaseModel_, modelHandle_);

// (Conditional frame-enumeration logging lives later where APIs are present.)

    // �x�[�X���f���̃��[�h����ɉE��t���[����T�����ăL���b�V������
    // MV1SearchFrame ���Ȃ����ł��t���[���� API ������Ζ��O���ƍ����ĉ�������Bs
    rightHandFrameIndex_ = -1;
    rightHandFrameName_.clear();

    if (baseModelHandle_ != -1) {
        // Emit always-on diagnostics to help troubleshoot missing logs
        DebugPrint("Player ctor entered, baseModelHandle=%d\n", baseModelHandle_);
#if defined(MV1SearchFrame)
        DebugPrint("MV1SearchFrame available: yes\n");
#else
        DebugPrint("MV1SearchFrame available: no\n");
#endif
#if defined(MV1GetFrameNum)
        DebugPrint("MV1GetFrameNum available: yes\n");
#else
        DebugPrint("MV1GetFrameNum available: no\n");
#endif
#if defined(MV1GetFrameName)
        DebugPrint("MV1GetFrameName available: yes\n");
#else
        DebugPrint("MV1GetFrameName available: no\n");
#endif

        DebugPrint("Predefined right-hand frame candidates:\n");
        for (const char* const* p = kRightHandFrameCandidates; *p != nullptr; ++p) {
            DebugPrint("  %s\n", *p);
        }
        // diagnostic helpers: track which search method succeeded
        bool foundByPredef = false;
        bool foundByKeyword = false;
        // Gather frame names for keyword-based matching
        int fc = MV1GetFrameNum(baseModelHandle_);
        std::vector<std::pair<int,std::string>> enumeratedFrames;
        for (int i = 0; i < fc; ++i) {
            const char* fname = MV1GetFrameName(baseModelHandle_, i);
            if (fname) {
                enumeratedFrames.emplace_back(i, std::string(fname));
            }
        }

        // Filter names containing keywords (case-insensitive)
        auto containsKeyword = [](const std::string &s)->bool{
            std::string low = s;
            std::transform(low.begin(), low.end(), low.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
            return low.find("hand") != std::string::npos || low.find("right") != std::string::npos || low.find("weapon") != std::string::npos || low.find("attach") != std::string::npos;
        };

        std::vector<std::pair<int,std::string>> keywordCandidates;
        for (auto &fr : enumeratedFrames) {
            if (containsKeyword(fr.second)) {
                keywordCandidates.push_back(fr);
            }
        }
        // Print a few extracted keyword candidates for debugging
        if (!keywordCandidates.empty()) {
            DebugPrint("Player: keywordCandidates count=%d (showing up to 5)\n", (int)keywordCandidates.size());
            int showN = keywordCandidates.size() < 5 ? static_cast<int>(keywordCandidates.size()) : 5;
            for (int i = 0; i < showN; ++i) {
                DebugPrint("  [%d] index=%d name='%s'\n", i, keywordCandidates[i].first, keywordCandidates[i].second.c_str());
            }
            // Also print the full keyword candidate list so the auto-extraction can be inspected
            for (size_t ki = 0; ki < keywordCandidates.size(); ++ki) {
                DebugPrint("KeywordCandidate[%d]: %s\n", (int)ki, keywordCandidates[ki].second.c_str());
            }
        }

        // Optional: dump all frame names when DEBUG_DUMP_FRAMES env var is set
#if defined(_MSC_VER)
        char* dbgVal = nullptr; size_t dbgLen = 0;
        if (_dupenv_s(&dbgVal, &dbgLen, "DEBUG_DUMP_FRAMES") == 0 && dbgVal != nullptr) {
            if (dbgVal[0] != '\0') {
                DebugPrint("Player: dumping all frame names (total=%d)\n", fc);
                for (auto &fr : enumeratedFrames) DebugPrint("  frame[%d] '%s'\n", fr.first, fr.second.c_str());
            }
            free(dbgVal);
        }
#else
        const char* dbgVal = std::getenv("DEBUG_DUMP_FRAMES");
        if (dbgVal && dbgVal[0] != '\0') {
            DebugPrint("Player: dumping all frame names (total=%d)\n", fc);
            for (auto &fr : enumeratedFrames) DebugPrint("  frame[%d] '%s'\n", fr.first, fr.second.c_str());
        }
#endif

        // First try the predefined candidate list via MV1SearchFrame
        for (const char* const* p = kRightHandFrameCandidates; *p != nullptr; ++p) {
            DebugPrint("Try frame candidate: %s\n", *p);
            int fi = MV1SearchFrame(baseModelHandle_, *p);
            DebugPrint("  MV1SearchFrame returned %d for '%s'\n", fi, *p);
            if (fi >= 0) {
                rightHandFrameIndex_ = fi;
                rightHandFrameName_ = *p;
                foundByPredef = true;
                break;
            }
        }

        // If not found, try the keyword-matched names
        if (rightHandFrameIndex_ == -1) {
            for (auto &fr : keywordCandidates) {
                DebugPrint("Try keyword candidate: %s (index=%d)\n", fr.second.c_str(), fr.first);
                int fi = MV1SearchFrame(baseModelHandle_, fr.second.c_str());
                DebugPrint("  MV1SearchFrame returned %d for keyword '%s'\n", fi, fr.second.c_str());
                if (fi >= 0) {
                    rightHandFrameIndex_ = fi;
                    rightHandFrameName_ = fr.second;
                    foundByKeyword = true;
                    break;
                }
            }
        }

        // If still not found, try matching enumerated names against predefined list
        if (rightHandFrameIndex_ == -1) {
            for (auto &fr : enumeratedFrames) {
                for (const char* const* p = kRightHandFrameCandidates; *p != nullptr; ++p) {
                    DebugPrint("Try frame candidate: %s against enumerated frame '%s' (idx=%d)\n", *p, fr.second.c_str(), fr.first);
                    if (fr.second == *p) {
                        DebugPrint("  Match found: '%s' == '%s' -> index=%d\n", fr.second.c_str(), *p, fr.first);
                        rightHandFrameIndex_ = fr.first;
                        rightHandFrameName_ = fr.second;
                        foundByPredef = true;
                        break;
                    }
                }
                if (rightHandFrameIndex_ != -1) break;
            }
        }

        // If still not found, pick the first keyword candidate as a best-effort
        if (rightHandFrameIndex_ == -1 && !keywordCandidates.empty()) {
            rightHandFrameIndex_ = keywordCandidates[0].first;
            rightHandFrameName_ = keywordCandidates[0].second;
            foundByKeyword = true;
        }

        if (rightHandFrameIndex_ == -1) {
            DebugPrint("No right-hand frame found\n");
            // Print prioritized frames that match keywords first
            DebugPrint("Prioritized frame names containing Right/Hand/Arm/Weapon/Attach:\n");
            int printed = 0;
            for (auto &fr : enumeratedFrames) {
                std::string low = fr.second;
                std::transform(low.begin(), low.end(), low.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
                if (low.find("hand") != std::string::npos || low.find("right") != std::string::npos || low.find("weapon") != std::string::npos || low.find("attach") != std::string::npos || low.find("arm") != std::string::npos) {
                    DebugPrint("  frame[%d] '%s'\n", fr.first, fr.second.c_str());
                    ++printed;
                }
            }
            if (printed == 0) DebugPrint("  (no prioritized matches)\n");

            // Also dump full frame list to help manual inspection
            DebugPrint("Full frame list (total=%d):\n", (int)enumeratedFrames.size());
            for (auto &fr : enumeratedFrames) DebugPrint("  frame[%d] '%s'\n", fr.first, fr.second.c_str());
        } else {
            DebugPrint("Picked right-hand frame: %s index=%d\n", rightHandFrameName_.c_str(), rightHandFrameIndex_);
            const char* method = "(unknown)";
            if (foundByPredef) method = "predefined-candidate";
            else if (foundByKeyword) method = "keyword-candidate";
            DebugPrint("Player: right-hand frame cached: name='%s' index=%d method=%s\n", rightHandFrameName_.c_str(), rightHandFrameIndex_, method);
        }
    }

    // �������̃A�j���[�V������o�^�i����: mirai_anim_<name>.mv1�j
    std::vector<std::pair<std::string, std::string>> animFiles = {
        {"idle", "assets/models/mirai_Idle.mv1"},
        {"idle_weapon", "assets/models/mirai_Idle.mv1"},
        {"dodge", "assets/models/mirai_dodge.mv1"},
        {"move", "assets/models/mirai_move2.mv1"},
        {"jump", "assets/models/mirai_jump.mv1"},
        {"attack", "assets/models/mirai_attack.mv1"},
        {"attack_weapon", "assets/models/mirai_attack.mv1"},
        {"aim", "assets/models/mirai_Aim.mv1"}
    };
    for (auto &p : animFiles) {
        int h = MV1LoadModel(p.second.c_str());
        if (h != -1) {
            // ���p�\�Ȃ�A�ǂݍ���MV1�����ۂɃA�j���[�V�������[�V�������܂ނ��m�F����B
            int animCount = -1;
#ifdef MV1GetAnimNum
            animCount = MV1GetAnimNum(h);
#endif
            if (animCount == 0) {
                // ����MV1�Ƀ��[�V�������Ȃ� ? �X�L�b�v���ă��f�����������
                DebugPrint("Warning: animation model '%s' contains 0 motions, skipping.\n", p.second.c_str());
                MV1DeleteModel(h);
                continue;
            }

            // API�����p�\�Ȃ�A�܂܂�郂�[�V��������񋓂��Ė��O�̕s��v��f�f����
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

            // �ǂݍ��񂾃A�j�����f�����̃��[�V�����C���f�b�N�X������BAPI���Ȃ������O��������Ȃ���΃f�t�H���g0�B
            int motionIndex = 0;
#ifdef MV1GetAnimIndex
            {
                int idx = MV1GetAnimIndex(h, p.first.c_str());
                if (idx >= 0) {
                    motionIndex = idx;
                } else {
                    // ���O�̕s��v���y�����邽�߂̈�ʓI�ȃt�H�[���o�b�N�����s�i��: 'Armature|Idle'�A�擪�啶���Ȃǁj
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

                    // ����ł���v���Ȃ��ꍇ�A�iAPI������΁j�܂܂�郂�[�V��������啶�������������̕���������`�F�b�N�ő���
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

            // �\�Ȃ���ۂ̃A�j���[�V��������ǂݎ��B�f�t�H���g��1.0�b
            float dur = 1.0f;
#ifdef MV1GetAnimTotalTime
            {
                float t = MV1GetAnimTotalTime(h, motionIndex);
                if (t > 0.0f) {
                    // �ꕔ�̃r���h�ł͕b�ł͂Ȃ��t���[��/�e�B�b�N��Ԃ����Ƃ�����B
                    // �l���e�B�b�N�炵���ꍇ�i�傫���j�ɂ�60�Ŋ����ĕb�ɕϊ�����q���[���X�e�B�b�N���g���B
                    if (t > 10.0f) {
                        DebugPrint("MV1GetAnimTotalTime for model '%s' motion %d returned %.3f - treating as ticks and converting to seconds by /60\n", p.second.c_str(), motionIndex, t);
                        t = t / 60.0f;
                    }
                    dur = t;
                }
            }
#endif
            animDurations_.emplace(p.first, dur);

            // move�A�j���̃��[�v�����Z�����Ă�藎�������������ɂ���
            if (p.first == "move") {
                // move���[�v�̊Ԋu��Z�����ă��[�v�Ԋu��Z������
                float newDur = dur * 2.5f;
                animDurations_["move"] = newDur;
                DebugPrint("Adjusted move anim duration from %.3f -> %.3f\n", dur, newDur);
            }

            // --- ������ idle �̍Đ����x��x�����邽�߂̒�����ǉ� ---
            if (p.first == "idle" || p.first == "idle_weapon") {
                // idle ��x������i�f�t�H���g��2�{�̒����ɂ��čĐ����x�𔼕��ɂ���j
                float newDur = dur * 2.0f;
                animDurations_[p.first] = newDur;
                DebugPrint("Adjusted %s anim duration from %.3f -> %.3f\n", p.first.c_str(), dur, newDur);
            }

            // �f�o�b�O: ���[�h�����A�j�����f���ƃ��^�f�[�^�����O�o��
            DebugPrint("Loaded anim '%s' handle=%d motionIndex=%d dur=%.3f\n", p.first.c_str(), h, motionIndex, dur);

            // �����Ńx�[�X���f���������ւ��Ȃ�; �A�j���̓x�[�X���f���ɃA�^�b�`�����
        }
    }

    // �\�Ȃ�t���[��/�{�[�������������ăA�^�b�`�s��v��f�f����B
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

            // �P���ȋ��ʗv�f�����v�Z�i�啶�������������j
            auto toLower = [](const std::string &s){ std::string r = s; std::transform(r.begin(), r.end(), r.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); }); return r; };
            std::vector<std::string> baseLow, animLow;
            for (auto &s : baseNames) baseLow.push_back(toLower(s));
            for (auto &s : animNames) animLow.push_back(toLower(s));

            int common = 0;
            for (auto &an : animLow) {
                if (std::find(baseLow.begin(), baseLow.end(), an) != baseLow.end()) ++common;
            }
            DebugPrint("Anim model '%s' common frame names with base: %d / %d\n", kv.first.c_str(), common, (int)animNames.size());

            // �����p�ɍő�10���̌�������\��
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

    // ���ϐ��ŃA�j�����f���̋����`����s�����o�I�Ɋm�F����B
    // MSVC�ł͔񐄏��x��C4996������邽�߈��S��getenv�o���A���g���g�p
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

    // �J�n���ɃA�C�h���A�j�����Đ������悤�ɂ���
    PlayAnimation("idle", true);

    auxLeft = new AuxUnit(0, 0.2f, 3.0f, 60.0f); // �@�֏e
    auxRight = new AuxUnit(1, 1.0f, 12.0f, 40.0f); // �~�T�C��
}

// �f�X�g���N�^
//  - MV1�̃A�^�b�`���������A�ǂݍ��񂾃A�j�����f�����������B
//  - �x�[�X���f�������g�ŏ��L���Ă���ꍇ�͂�����폜����B
//  - �⏕���j�b�g��j������B
Player::~Player()
{
    // ���݂ƑO��̃A�^�b�`�A�j�����f�^�b�`
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

    // �A�j�����f�����폜
    for (auto &kv : animModelHandles_) {
        if (kv.second != -1) MV1DeleteModel(kv.second);
    }

    // �����ŏ��L���Ă���x�[�X���f�����폜
    if (ownsBaseModel_ && baseModelHandle_ != -1) {
        MV1DeleteModel(baseModelHandle_);
    }

    // Delete equipped weapon instance if we own it
    if (equippedWeaponModelOwned_ && equippedWeaponModelHandle_ != -1) {
        MV1DeleteModel(equippedWeaponModelHandle_);
        equippedWeaponModelHandle_ = -1;
        equippedWeaponModelOwned_ = false;
    }

    delete auxLeft; delete auxRight;
}
