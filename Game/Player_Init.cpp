// Player_Init.cpp
// サマリー:
//  - プレイヤーの生成/破棄に関する実装をまとめたファイルです。
//  - モデルとアニメーションモデルの読み込み、アニメーションメタデータの初期化、
//    デバッグ向けの環境変数チェックなど、リソース初期化に関わる処理を担います。
//  - デストラクタでは各種アタッチ解除とリソース解放を行います。

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

// コンストラクタ
//  - アセットマネージャが与えられていればそれを使ってベースモデルを取得する。
//  - 定義済みのアニメーションMV1ファイルを列挙して読み込み、モーションインデックスや再生時間を推定して格納する。
//  - フレーム/ボーン名の差異を診断するためのロギングを行う（APIが存在する場合）。
//  - 環境変数`DEBUG_FORCE_DRAW_ANIM`が設定されている場合は、ベースモデルの代わりにアニメモデルを直接描画するオプションを有効にする。
//  - 最後に補助ユニットを生成する。
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
                // このMV1にモーションがない ? スキップしてモデルを解放する
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

    // 可能ならフレーム/ボーン名を検査してアタッチ不一致を診断する。
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

// デストラクタ
//  - MV1のアタッチを解除し、読み込んだアニメモデルを解放する。
//  - ベースモデルを自身で所有している場合はそれを削除する。
//  - 補助ユニットを破棄する。
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
