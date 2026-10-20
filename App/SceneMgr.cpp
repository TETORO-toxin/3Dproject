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
#include "../Game/TimescaleManager.h"

#include "../Sys/Input.h"
#include "../Game/UI3D2D.h"
#include "../Sys/Lighting.h"
#include <cmath>
#include <algorithm>

static void DrawGroundGrid(const VECTOR& planePoint, const VECTOR& planeNormal, float size = 60.0f, float step = 1.0f)
{
    unsigned int col = GetColor(60, 60, 60);
    unsigned int centerCol = GetColor(120, 120, 160);

    VECTOR n = planeNormal;
    float nlen = sqrtf(n.x*n.x + n.y*n.y + n.z*n.z);
    if (nlen < 1e-6f) n = VGet(0.0f, 1.0f, 0.0f);
    else n = VGet(n.x / nlen, n.y / nlen, n.z / nlen);

    VECTOR arbitrary = (fabsf(n.x) < 0.9f) ? VGet(1.0f, 0.0f, 0.0f) : VGet(0.0f, 1.0f, 0.0f);
    VECTOR u = VCross(arbitrary, n);
    float ulen = sqrtf(u.x*u.x + u.y*u.y + u.z*u.z);
    if (ulen < 1e-6f) u = VGet(1.0f, 0.0f, 0.0f);
    else u = VGet(u.x / ulen, u.y / ulen, u.z / ulen);
    VECTOR v = VCross(n, u);
    float vlen = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
    if (vlen < 1e-6f) v = VGet(0.0f, 0.0f, 1.0f);
    else v = VGet(v.x / vlen, v.y / vlen, v.z / vlen);

    for (float du = -size; du <= size; du += step) {
        VECTOR offset = VAdd(VScale(u, du), planePoint);
        VECTOR a = VAdd(offset, VScale(v, -size));
        VECTOR b = VAdd(offset, VScale(v, size));
        unsigned int c = (fabsf(du) < 1e-6f) ? centerCol : col;
        DrawLine3D(a, b, c);
    }
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
    assets_ = new AssetsMgr();
    assets_->Init();

    Lighting::InitDefault();

    player_ = new Player(assets_);
    camera_ = new CameraRig();
    player_->SetCamera(camera_);

    navMesh_.Initialize(100, 100, 1.0f, VGet(-50.0f, 0.0f, -50.0f));
    for (int z = 20; z < 30; ++z) {
        for (int x = 40; x < 50; ++x) {
            navMesh_.SetBlocked(x, z, true);
        }
    }

    EffectManager* existing = GetGlobalEffectManager();
    if (existing == nullptr) {
        effects_ = new EffectManager();
        SetGlobalEffectManager(effects_);
    } else {
        effects_ = nullptr;
    }

    enemies_.push_back(new Enemy(VGet(6.0f, 0.0f, 10.0f)));
    enemies_.push_back(new Enemy(VGet(-8.0f, 0.0f, 18.0f)));
    enemies_.push_back(new Enemy(VGet(2.0f, 0.0f, 26.0f)));
    for (auto e : enemies_) { e->SetNavMesh(&navMesh_); }

    prevTimeMs_ = GetNowCount();

    titleScene_ = new TitleScene();
    titleScene_->Init(player_, camera_, assets_);

    groundPoint_ = VGet(0.0f, 0.0f, 0.0f);
    groundNormal_ = VGet(0.0f, 1.0f, 0.0f);
    camera_->SetGroundPlane(groundPoint_, groundNormal_);

    VECTOR ppos = player_->GetPosition();
    weaponPickups_.push_back(Game::WeaponPickup(Game::WeaponType::IronPipe, VAdd(ppos, VGet(0.0f, 0.0f, 3.0f)), assets_));
    weaponPickups_.push_back(Game::WeaponPickup(Game::WeaponType::IronPipe, VAdd(ppos, VGet(2.0f, 0.0f, 4.0f)), assets_));
}

SceneMgr::~SceneMgr()
{
    if (titleScene_) { delete titleScene_; titleScene_ = nullptr; }
    for (auto e : enemies_) { delete e; }
    enemies_.clear();
    if (camera_) { delete camera_; camera_ = nullptr; }
    if (player_) { delete player_; player_ = nullptr; }
    if (assets_) { delete assets_; assets_ = nullptr; }
    effects_ = nullptr;
}

void SceneMgr::Update()
{
    InputState in = PollInput();
    PadState pad = PollPad();
    bool lockButton = IsButtonDown(pad, PAD_INPUT_4);

    unsigned int now = GetNowCount();
    float dt = (now - prevTimeMs_) / 1000.0f;
    prevTimeMs_ = now;

    if (dt < 0.0f) dt = 0.0f;
    if (dt > 0.05f) dt = 0.05f;

    if (player_ && player_->IsThrowCharging()) {
        timescaleManager_.SetTargetTimescale(0.2f);
    } else {
        timescaleManager_.SetTargetTimescale(1.0f);
    }
    timescaleManager_.Update(dt);
    float effectiveDt = dt * timescaleManager_.GetCurrentTimescale();

    if (currentScene_ == Scene::Title) {
        bool start = titleScene_->Update();
        titleScene_->Draw();
        if (start) ChangeScene(Scene::Base);
        return;
    }

    player_->UpdateLogic(effectiveDt, in);
    VECTOR ppos = player_->GetPosition();

    player_->UpdateCoinThrow(effectiveDt, in, enemies_);

    EffectManager* gem = GetGlobalEffectManager();
    if (gem) {
        VECTOR forward = VGet(0.0f, 0.0f, 1.0f);
        if (camera_) forward = camera_->GetForwardXZ();
        float fl = sqrtf(forward.x*forward.x + forward.y*forward.y + forward.z*forward.z);
        if (fl > 1e-6f) forward = VGet(forward.x/fl, forward.y/fl, forward.z/fl);
        else forward = VGet(0.0f, 0.0f, 1.0f);
        gem->Update(ppos, forward);
    }

    DrawGroundGrid(groundPoint_, groundNormal_, 60.0f, 1.0f);
    navMesh_.DebugDraw();

    float bestScore = 1e9f;
    Enemy* best = nullptr;
    for (auto e : enemies_) {
        VECTOR epos = e->GetPosition();
        VECTOR toEnemy = VSub(epos, ppos);
        float dist = VSize(toEnemy);
        VECTOR forward = VSub(target_, ppos);
        float flen = VSize(forward);
        if (flen < 0.0001f) forward = VGet(0,0,1);
        else forward = VScale(forward, 1.0f / flen);

        float dot = (forward.x * toEnemy.x + forward.y * toEnemy.y + forward.z * toEnemy.z) / (dist);
        dot = clamp(dot, -1.0f, 1.0f);
        float angle = acosf(dot);

        float score = angle * 2.0f + dist * 0.1f;
        if (angle < (30.0f * 3.14159f / 180.0f)) {
            if (score < bestScore) { bestScore = score; best = e; }
        }
    }

    bool locked = (best != nullptr) && lockButton;

    if (locked && best) {
        camera_->Update(ppos, best->GetPosition(), true, effectiveDt);
    } else {
        camera_->Update(ppos, VGet(0,0,0), false, effectiveDt);
    }

    struct EnemyDepth { Enemy* e; float z; bool uiVisible; float screenZ; float screenX; float screenY; };
    std::vector<EnemyDepth> edepths;

    VECTOR camPos = camera_->GetCameraPosition();
    VECTOR fwd = camera_->GetForward();
    float fl = sqrtf(fwd.x*fwd.x + fwd.y*fwd.y + fwd.z*fwd.z);
    if (fl < 1e-6f) fwd = VGet(0.0f, 0.0f, 1.0f);
    else fwd = VGet(fwd.x/fl, fwd.y/fl, fwd.z/fl);

    for (auto e : enemies_) {
        e->SetTarget(ppos);
        e->Update(effectiveDt);

        VECTOR epos = e->GetPosition();
        VECTOR camTo = VSub(epos, camPos);
        float zcam = fwd.x * camTo.x + fwd.y * camTo.y + fwd.z * camTo.z;

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

    const float minSeparation = 1.0f;
    const float minSepSq = minSeparation * minSeparation;
    for (size_t i = 0; i < edepths.size(); ++i) {
        for (size_t j = i + 1; j < edepths.size(); ++j) {
            Enemy* a = edepths[i].e;
            Enemy* b = edepths[j].e;
            if (a->IsAirborne() || b->IsAirborne()) continue;
            VECTOR pa = a->GetPosition();
            VECTOR pb = b->GetPosition();
            float dx = pb.x - pa.x;
            float dz = pb.z - pa.z;
            float d2 = dx*dx + dz*dz;
            if (d2 < minSepSq) {
                float d = sqrtf(d2);
                if (d < 1e-5f) {
                    float push = minSeparation * 0.5f;
                    a->NudgePosition(VGet(-push, 0.0f, 0.0f));
                    b->NudgePosition(VGet(push, 0.0f, 0.0f));
                } else {
                    float overlap = minSeparation - d;
                    float half = overlap * 0.5f;
                    float nx = dx / d;
                    float nz = dz / d;
                    a->NudgePosition(VGet(-nx * half, 0.0f, -nz * half));
                    b->NudgePosition(VGet(nx * half, 0.0f, nz * half));
                }
            }
        }
    }

    for (auto &ed : edepths) {
        VECTOR epos = ed.e->GetPosition();
        VECTOR camTo = VSub(epos, camPos);
        ed.z = fwd.x * camTo.x + fwd.y * camTo.y + fwd.z * camTo.z;

        VECTOR head = VAdd(epos, VGet(0.0f, 2.0f, 0.0f));
        VECTOR camToHead = VSub(head, camPos);
        float zcamHead = fwd.x * camToHead.x + fwd.y * camToHead.y + fwd.z * camToHead.z;
        ed.uiVisible = false;
        ed.screenX = 0; ed.screenY = 0; ed.screenZ = 0;
        if (zcamHead > 0.001f) {
            VECTOR scr = ConvWorldPosToScreenPos(head);
            ed.screenX = scr.x; ed.screenY = scr.y; ed.screenZ = scr.z;
            const float minFrontZ = 0.01f;
            const float screenPosLimit = 10000.0f;
            if (scr.z > minFrontZ && std::fabs(scr.x) < screenPosLimit && std::fabs(scr.y) < screenPosLimit) {
                ed.uiVisible = true;
            }
        }
    }

    Enemy* bestForUI = best;
    for (const auto &ed : edepths) {
        if (!ed.uiVisible) continue;
        DrawCircle((int)ed.screenX, (int)ed.screenY, 18, GetColor(0,255,255), FALSE, 2);
        DrawEnemyUI(*ed.e, ed.e == bestForUI);
    }

    VECTOR camToPlayer = VSub(ppos, camPos);
    float playerZ = fwd.x * camToPlayer.x + fwd.y * camToPlayer.y + fwd.z * camToPlayer.z;

    std::sort(edepths.begin(), edepths.end(), [](const EnemyDepth&a, const EnemyDepth&b){ return a.z > b.z; });

    for (const auto &ed : edepths) {
        if (ed.z > playerZ) {
            ed.e->Draw();
        }
    }

    player_->Draw();
    player_->DrawCoinThrowEffect();
    player_->DrawReflectTrajectory();

    for (const auto &ed : edepths) {
        if (ed.z <= playerZ) {
            ed.e->Draw();
        }
    }

    if (gem) gem->Draw();

    DrawFormatString(10, 10, GetColor(255,255,255), "Enemies: %d  Best: %s", (int)enemies_.size(), best ? "Yes" : "No");

    UpdateWeaponPickups(in, ppos);
    DrawWeaponPickups();

    const char* eq = Game::GetWeaponName(player_->GetEquippedWeapon());
    DrawFormatString(10, 50, GetColor(200,200,255), "Weapon: %s", eq);
}

void SceneMgr::ChangeScene(Scene s)
{
    currentScene_ = s;
}

SceneMgr::Scene SceneMgr::GetCurrentScene() const
{
    return currentScene_;
}

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
            if (in.interactPressed) {
                Game::WeaponType newW = wp.GetType();
                Game::WeaponType old = player_->EquipWeapon(newW);
                weaponPickups_[lastNearestPickupIdx_].MarkPicked();
                if (old != Game::WeaponType::None) {
                    VECTOR dropPos = wp.GetPosition();
                    weaponPickups_.push_back(Game::WeaponPickup(old, dropPos));
                }
            }
        }
    }
}

void SceneMgr::DrawWeaponPickups()
{
    if (lastNearestPickupIdx_ != -1) {
        const auto &wp = weaponPickups_[lastNearestPickupIdx_];
        float range2 = pickupRange_ * pickupRange_;
        if (lastNearestPickupDist2_ <= range2) {
            DrawFormatString(10, 30, GetColor(255,255,0), "E / 取得: %s", Game::GetWeaponName(wp.GetType()));
        }
    }

    weaponPickups_.erase(std::remove_if(weaponPickups_.begin(), weaponPickups_.end(), [](const Game::WeaponPickup &w){ return w.IsPicked(); }), weaponPickups_.end());

    for (auto &wp : weaponPickups_) {
        wp.Draw();
    }
}
