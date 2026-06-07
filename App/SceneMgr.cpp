#include "SceneMgr.h"
#include "../Game/Player.h"
#include "../Game/CameraRig.h"
#include "../Sys/Assets.h"
#include "../Game/Enemy.h"
#include "TitleScene.h"
#include "../Sys/EffectManager.h"
#include "../Sys/GlobalEffects.h"
#include "../Sys/DebugPrint.h"
#include "../Game/WeaponPickup.h"
#include "../Game/WeaponTypes.h"

#include "../Sys/Input.h"
#include "../Game/UI3D2D.h"
#include "../Sys/Lighting.h"
#include <cmath>
#include <algorithm>

static void DrawGroundGrid(const VECTOR& planePoint, const VECTOR& planeNormal, float size = 60.0f, float step = 1.0f)
{
    unsigned int col = GetColor(60, 60, 60);
    unsigned int centerCol = GetColor(120, 120, 160);

    // 平面上の直交単位基底 (u, v) を構築
    VECTOR n = planeNormal;
    // n を正規化
    float nlen = sqrtf(n.x*n.x + n.y*n.y + n.z*n.z);
    if (nlen < 1e-6f) n = VGet(0.0f, 1.0f, 0.0f);
    else n = VGet(n.x / nlen, n.y / nlen, n.z / nlen);

    // 交差させる任意のベクトルを選択（n と平行でないもの）
    VECTOR arbitrary = (fabsf(n.x) < 0.9f) ? VGet(1.0f, 0.0f, 0.0f) : VGet(0.0f, 1.0f, 0.0f);
    // u = normalize(cross(arbitrary, n)) -> 接線方向1
    VECTOR u = VCross(arbitrary, n);
    float ulen = sqrtf(u.x*u.x + u.y*u.y + u.z*u.z);
    if (ulen < 1e-6f) u = VGet(1.0f, 0.0f, 0.0f);
    else u = VGet(u.x / ulen, u.y / ulen, u.z / ulen);
    // v = cross(n, u) （接線方向2）
    VECTOR v = VCross(n, u);
    float vlen = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
    if (vlen < 1e-6f) v = VGet(0.0f, 0.0f, 1.0f);
    else v = VGet(v.x / vlen, v.y / vlen, v.z / vlen);

    // u のオフセットごとに v に平行な線を描画
    for (float du = -size; du <= size; du += step) {
        VECTOR offset = VAdd(VScale(u, du), planePoint);
        VECTOR a = VAdd(offset, VScale(v, -size));
        VECTOR b = VAdd(offset, VScale(v, size));
        unsigned int c = (fabsf(du) < 1e-6f) ? centerCol : col;
        DrawLine3D(a, b, c);
    }
    // v のオフセットごとに u に平行な線を描画
    for (float dv = -size; dv <= size; dv += step) {
        VECTOR offset = VAdd(VScale(v, dv), planePoint);
        VECTOR a = VAdd(offset, VScale(u, -size));
        VECTOR b = VAdd(offset, VScale(u, size));
        unsigned int c = (fabsf(dv) < 1e-6f) ? centerCol : col;
        DrawLine3D(a, b, c);
    }
}

void SceneMgr::Init()
{
    // 最小限のシステムを作成
    assets_ = new AssetsMgr();
    assets_->Init();

    // ライティング（環境光/背景）を初期化
    Lighting::InitDefault();

    player_ = new Player(assets_);
    camera_ = new CameraRig();
    player_->SetCamera(camera_);

    // initialize a simple NavMesh covering reasonable play area
    // cellSize = 1.0, origin at (-50,0,-50) covering 100x100
    navMesh_.Initialize(100, 100, 1.0f, VGet(-50.0f, 0.0f, -50.0f));
    // mark some rectangular obstacles (example)
    for (int z = 20; z < 30; ++z) {
        for (int x = 40; x < 50; ++x) {
            navMesh_.SetBlocked(x, z, true);
        }
    }

    // グローバルな EffectManager が未設定の場合のみ生成する。
    // 既に別の所有者がいる場合は所有権を取らない。
    EffectManager* existing = GetGlobalEffectManager();
    if (existing == nullptr) {
        effects_ = new EffectManager();
        SetGlobalEffectManager(effects_);
    } else {
        effects_ = nullptr; // 所有しない
        // 既存のグローバルマネージャーをそのまま維持する
    }

    // サンプルの敵を生成
    enemies_.push_back(new Enemy(VGet(6.0f, 0.0f, 10.0f)));
    enemies_.push_back(new Enemy(VGet(-8.0f, 0.0f, 18.0f)));
    enemies_.push_back(new Enemy(VGet(2.0f, 0.0f, 26.0f)));

    prevTimeMs_ = GetNowCount();

    // タイトルシーンを初期化
    titleScene_ = new TitleScene();
    titleScene_->Init(player_, camera_, assets_);

    // カメラに地面平面を通知（デフォルトは y=0 の水平面）
    groundPoint_ = VGet(0.0f, 0.0f, 0.0f);
    groundNormal_ = VGet(0.0f, 1.0f, 0.0f);
    camera_->SetGroundPlane(groundPoint_, groundNormal_);

    // フィールドに鉄パイプを配置する (プレイヤーの前方にいくつか)
    VECTOR ppos = player_->GetPosition();
    // 1つ目: 少し前 (AssetsMgr を渡してモデル共有)
    weaponPickups_.push_back(Game::WeaponPickup(Game::WeaponType::IronPipe, VAdd(ppos, VGet(0.0f, 0.0f, 3.0f)), assets_));
    // 2つ目: 右前
    weaponPickups_.push_back(Game::WeaponPickup(Game::WeaponType::IronPipe, VAdd(ppos, VGet(2.0f, 0.0f, 4.0f)), assets_));
}

SceneMgr::~SceneMgr()
{
    // タイトルシーンを削除
    if (titleScene_) { delete titleScene_; titleScene_ = nullptr; }

    // 敵を削除
    for (auto e : enemies_) { delete e; }
    enemies_.clear();

    // カメラとプレイヤーを削除
    if (camera_) { delete camera_; camera_ = nullptr; }
    if (player_) { delete player_; player_ = nullptr; }

    // アセットを削除
    if (assets_) { delete assets_; assets_ = nullptr; }

    // グローバルのエフェクトマネージャーをここで削除・クリアしてはいけない。
    // グラフィックス/デバイスの終了処理競合を避けるため、DxLib 終了後にアプリケーション側でクリーンアップされる。
    effects_ = nullptr;
}

void SceneMgr::Update()
{
    // フレーム開始時に一度だけ入力を取得し各システムに配布する。
    // これにより入力処理を集中化し、Player / UI / Pickup 等での競合を制御できる。
    InputState in = PollInput();
    // 必要に応じてデバッグやロックオンの例のためにパッドの生情報取得も許可する
    PadState pad = PollPad();
    bool lockButton = IsButtonDown(pad, PAD_INPUT_4); // 例示用ボタン

    unsigned int now = GetNowCount();
    float dt = (now - prevTimeMs_) / 1000.0f;
    prevTimeMs_ = now;

    // 一時停止やウィンドウ移動時などの大きなジャンプを避けるため dt をクランプ
    if (dt < 0.0f) dt = 0.0f;
    if (dt > 0.05f) dt = 0.05f; // max 50ms

    if (currentScene_ == Scene::Title) {
        // タイトルシーンを更新・描画
        bool start = titleScene_->Update();
        titleScene_->Draw();
        if (start) {
            ChangeScene(Scene::Base);
        }
        return;
    }

    // プレイヤーのロジックのみ更新（描画は他の描画後に行い、プレイヤーで覆い隠せるようにする）
    player_->UpdateLogic(dt, in);
    VECTOR ppos = player_->GetPosition();

    // グローバルエフェクトマネージャーのロジックをプレイヤー位置で更新し、再生位置がプレイヤーに追従するようにする
    EffectManager* gem = GetGlobalEffectManager();
    if (gem) {
        // プレイヤーエフェクト用の前方方向を計算: カメラの XZ 前方を優先、無ければ +Z を使用
        VECTOR forward = VGet(0.0f, 0.0f, 1.0f);
        if (camera_) forward = camera_->GetForwardXZ();
        float fl = sqrtf(forward.x*forward.x + forward.y*forward.y + forward.z*forward.z);
        if (fl > 1e-6f) forward = VGet(forward.x/fl, forward.y/fl, forward.z/fl);
        else forward = VGet(0.0f, 0.0f, 1.0f);
        gem->Update(ppos, forward);
    }

    // デバッグ: F1 を押すとプレイヤー前方にエフェクトを生成して再生/可視性を確認
    if (CheckHitKey(KEY_INPUT_F1)) {
        if (gem) {
            // カメラ基準で生成（B）: カメラ位置の少し前に出して確実に見えるようにする
            VECTOR camPos = camera_->GetCameraPosition();
            VECTOR forwardCam = camera_->GetForwardXZ();
            float fl = sqrtf(forwardCam.x*forwardCam.x + forwardCam.y*forwardCam.y + forwardCam.z*forwardCam.z);
            if (fl > 1e-6f) forwardCam = VGet(forwardCam.x/fl, forwardCam.y/fl, forwardCam.z/fl);
            VECTOR posCam = VAdd(camPos, VAdd(VScale(forwardCam, 4.0f), VGet(0.0f, 0.0f, 0.0f)));
            DebugPrint("F1 spawn effect (camera-front) at %.2f,%.2f,%.2f\n", posCam.x, posCam.y, posCam.z);
            gem->PlayEffectAt(posCam, nullptr, 3.0f);

            // 比較のためプレイヤー前方にも生成
            VECTOR forward = VGet(0.0f, 0.0f, 1.0f);
            if (camera_) forward = camera_->GetForwardXZ();
            float fl2 = sqrtf(forward.x*forward.x + forward.y*forward.y + forward.z*forward.z);
            if (fl2 > 1e-6f) forward = VGet(forward.x/fl2, forward.y/fl2, forward.z/fl2);
            VECTOR pos = VAdd(ppos, VAdd(VScale(forward, 1.4f), VGet(0.0f, 1.6f, 0.0f)));
            DebugPrint("F1 spawn effect (player-front) at %.2f,%.2f,%.2f\n", pos.x, pos.y, pos.z);
            gem->PlayEffectAt(pos, nullptr, 3.0f);
        }
    }

    // 地面平面に沿ったグリッドを描画（平面上の真の 3D グリッド）
    DrawGroundGrid(groundPoint_, groundNormal_, 60.0f, 1.0f);

    // ロックオン選択: スコア = 角度差 * 重み + 距離
    float bestScore = 1e9f;
    Enemy* best = nullptr;
    for (auto e : enemies_) {
        VECTOR epos = e->GetPosition();
        VECTOR toEnemy = VSub(epos, ppos);
        float dist = VSize(toEnemy);
        // forward: カメラのターゲットが設定されている想定。`target_` を視線方向として使い、ゼロならフォールバック
        VECTOR forward = VSub(target_, ppos);
        float flen = VSize(forward);
        if (flen < 0.0001f) forward = VGet(0,0,1);
        else forward = VScale(forward, 1.0f / flen);

        float dot = (forward.x * toEnemy.x + forward.y * toEnemy.y + forward.z * toEnemy.z) / (dist);
        dot = clamp(dot, -1.0f, 1.0f);
        float angle = acosf(dot); // ラジアン単位

        // スコア: 角度重み + 正規化距離
        float score = angle * 2.0f + dist * 0.1f;
        if (angle < (30.0f * 3.14159f / 180.0f)) { // 30度以内
            if (score < bestScore) { bestScore = score; best = e; }
        }
    }

    bool locked = (best != nullptr) && lockButton;

    // カメラを更新
    if (locked && best) {
        camera_->Update(ppos, best->GetPosition(), true, dt);
    } else {
        camera_->Update(ppos, VGet(0,0,0), false, dt);
    }

    // 敵をカメラ空間の深度付きでリスト化し、プレイヤーとの相対描画順を決める
    struct EnemyDepth { Enemy* e; float z; bool uiVisible; float screenZ; float screenX; float screenY; };
    std::vector<EnemyDepth> edepths;

    VECTOR camPos = camera_->GetCameraPosition();
    VECTOR fwd = camera_->GetForward();
    float fl = sqrtf(fwd.x*fwd.x + fwd.y*fwd.y + fwd.z*fwd.z);
    if (fl < 1e-6f) fwd = VGet(0.0f, 0.0f, 1.0f);
    else fwd = VGet(fwd.x/fl, fwd.y/fl, fwd.z/fl);

    for (auto e : enemies_) {
        // 敵のロジックを更新
        e->Update(dt);

        VECTOR epos = e->GetPosition();
        VECTOR camTo = VSub(epos, camPos);
        float zcam = fwd.x * camTo.x + fwd.y * camTo.y + fwd.z * camTo.z;

        // UI（照準）を描画すべきか判定
        VECTOR head = VAdd(epos, VGet(0.0f, 2.0f, 0.0f));
        VECTOR camToHead = VSub(head, camPos);
        float zcamHead = fwd.x * camToHead.x + fwd.y * camToHead.y + fwd.z * camToHead.z;
        bool uiVis = false;
        float sX=0, sY=0, sZ=0;
        if (zcamHead > 0.001f) {
            VECTOR scr = ConvWorldPosToScreenPos(head);
            sX = scr.x; sY = scr.y; sZ = scr.z;
            const float minFrontZ = 0.01f;
            const float screenPosLimit = 10000.0f;
            if (scr.z > minFrontZ && std::fabs(scr.x) < screenPosLimit && std::fabs(scr.y) < screenPosLimit) {
                uiVis = true;
            }
        }

        edepths.push_back({e, zcam, uiVis, sZ, sX, sY});
    }

    // 敵の UI オーバーレイを描画（描画順に依存しない） - 既存挙動を維持
    Enemy* bestForUI = best;
    for (const auto &ed : edepths) {
        if (!ed.uiVisible) continue;
        DrawCircle((int)ed.screenX, (int)ed.screenY, 18, GetColor(0,255,255), FALSE, 2);
        DrawEnemyUI(*ed.e, ed.e == bestForUI);
    }

    // プレイヤーのカメラ空間での深度を計算
    VECTOR camToPlayer = VSub(ppos, camPos);
    float playerZ = fwd.x * camToPlayer.x + fwd.y * camToPlayer.y + fwd.z * camToPlayer.z;

    // 深度でソート（遠い順 -> 近い順）
    std::sort(edepths.begin(), edepths.end(), [](const EnemyDepth&a, const EnemyDepth&b){ return a.z > b.z; });

    // プレイヤーより遠い敵を先に描画
    for (const auto &ed : edepths) {
        if (ed.z > playerZ) {
            ed.e->Draw();
        }
    }

    // プレイヤーを描画
    player_->Draw();

    // （レンダーステートの競合を避けるため、エフェクトは全ての 3D 描画の後に描画）

    // プレイヤーより近い、または同じ深度の敵を描画（プレイヤーの上に表示されるように）
    for (const auto &ed : edepths) {
        if (ed.z <= playerZ) {
            ed.e->Draw();
        }
    }

    // グローバルエフェクト（3D）を描画し、他の 3D ジオメトリと同じワールド空間で表示する
    if (gem) {
        gem->Draw();
    }

    // 描画後に HUD / デバッグテキストを描画
    DrawFormatString(10, 10, GetColor(255,255,255), "Enemies: %d  Best: %s", (int)enemies_.size(), best ? "Yes" : "No");

    // --- ピックアップ更新 / 描画 ---
    // Pickup 関連の処理は責務を分割してヘルパ関数へ移譲します。
    // - UpdateWeaponPickups: 入力とプレイヤー位置を受け取り取得判定/Equip を行う
    // - DrawWeaponPickups: ピックアップの UI 表示と実際の描画・削除を行う
    UpdateWeaponPickups(in, ppos);
    DrawWeaponPickups();

    // HUD: 現在装備
    const char* eq = Game::GetWeaponName(player_->GetEquippedWeapon());
    DrawFormatString(10, 50, GetColor(200,200,255), "装備: %s", eq);
}

void SceneMgr::ChangeScene(Scene s)
{
    currentScene_ = s;
}

SceneMgr::Scene SceneMgr::GetCurrentScene() const
{
    return currentScene_;
}

// ------------------------------------------------------------------
// Pickup helpers
// ------------------------------------------------------------------
// FindNearestPickupIndex:
// 指定したワールド位置(ppos) から XZ 平面での最短距離にある未取得の
// WeaponPickup のインデックスを返します。未取得エントリが無ければ -1.
// この関数は探索だけに責務を限定しているため、将来的に複数候補の収集
// (CollectNearbyPickups) や優先順位付けロジックへ差し替えや拡張が容易です。
int SceneMgr::FindNearestPickupIndex(const VECTOR& ppos) const
{
    int nearestIdx = -1;
    float nearestDist2 = 1e9f;
    for (size_t i = 0; i < weaponPickups_.size(); ++i) {
        const auto &wp = weaponPickups_[i];
        if (wp.IsPicked()) continue;
        float dx = wp.GetPosition().x - ppos.x;
        float dz = wp.GetPosition().z - ppos.z;
        float d2 = dx*dx + dz*dz;
        if (d2 < nearestDist2) { nearestDist2 = d2; nearestIdx = (int)i; }
    }
    return nearestIdx;
}

// UpdateWeaponPickups:
// - 入力(in) とプレイヤー位置(ppos) を受け取り、最寄りピックアップの範囲判定
//   と実際の Equip / ドロップ処理を行います。
// - 実際の描画は DrawWeaponPickups に任せるため、ここでは UI 表示は行いません。
// - 成功/失敗にかかわらず、最後に見つかった最寄りインデックスと距離を
//   lastNearestPickupIdx_/lastNearestPickupDist2_ に保存しておきます。
void SceneMgr::UpdateWeaponPickups(const InputState& in, const VECTOR& ppos)
{
    lastNearestPickupIdx_ = -1;
    lastNearestPickupDist2_ = 1e9f;
    for (size_t i = 0; i < weaponPickups_.size(); ++i) {
        const auto &wp = weaponPickups_[i];
        if (wp.IsPicked()) continue;
        float dx = wp.GetPosition().x - ppos.x;
        float dz = wp.GetPosition().z - ppos.z;
        float d2 = dx*dx + dz*dz;
        if (d2 < lastNearestPickupDist2_) { lastNearestPickupDist2_ = d2; lastNearestPickupIdx_ = (int)i; }
    }

    if (lastNearestPickupIdx_ != -1) {
        const auto &wp = weaponPickups_[lastNearestPickupIdx_];
        float range2 = pickupRange_ * pickupRange_;
        if (lastNearestPickupDist2_ <= range2) {
            // プレイヤーがインタラクト入力を行った場合のみ Equip を行う。
            if (in.interactPressed) {
                Game::WeaponType newW = wp.GetType();
                Game::WeaponType old = player_->EquipWeapon(newW);
                // 値オブジェクトの配列を直接操作して Picked フラグを立てる
                weaponPickups_[lastNearestPickupIdx_].MarkPicked();
                // 以前の武器があればその位置にドロップする
                if (old != Game::WeaponType::None) {
                    VECTOR dropPos = wp.GetPosition();
                    weaponPickups_.push_back(Game::WeaponPickup(old, dropPos));
                }
            }
        }
    }
}

// DrawWeaponPickups:
// - Pickup の HUD 表示（"E で拾う"）や実際の 3D 描画を行います。
// - また取得済み( IsPicked() ) のエントリを配列から削除して、未使用エントリが
//   溜まるのを防ぎます。
// - UI 表示は将来的にデバイス中立化（"E / △ で拾う" や汎用 Interact 表記へ）
//   へ差し替えやすいように分離しています。
void SceneMgr::DrawWeaponPickups()
{
    // 最寄りピックアップが範囲内なら案内 UI を描画する
    if (lastNearestPickupIdx_ != -1) {
        const auto &wp = weaponPickups_[lastNearestPickupIdx_];
        float range2 = pickupRange_ * pickupRange_;
        if (lastNearestPickupDist2_ <= range2) {
            // TODO: 将来的に表示は InputActions 依存の文字列を引けるようにする
            // 現状はキーボード向け "E で拾う" を表示するが、デバイス中立化の
            // ために "E / △ で拾う" へ変更しておく。
            DrawFormatString(10, 30, GetColor(255,255,0), "E / △ で拾う: %s", Game::GetWeaponName(wp.GetType()));
        }
    }

    // 取得済みエントリを削除して未使用エントリの蓄積を防ぐ
    weaponPickups_.erase(std::remove_if(weaponPickups_.begin(), weaponPickups_.end(), [](const Game::WeaponPickup &w){ return w.IsPicked(); }), weaponPickups_.end());

    // すべてのピックアップを描画
    for (auto &wp : weaponPickups_) {
        wp.Draw();
    }
}
