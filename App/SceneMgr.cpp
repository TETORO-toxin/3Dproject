#include "SceneMgr.h"
#include "../Game/Player.h"
#include "../Game/CameraRig.h"
#include "../Sys/Assets.h"
#include "../Game/Enemy.h"
#include "TitleScene.h"
#include "../Sys/EffectManager.h"
#include "../Sys/GlobalEffects.h"
#include "../Sys/DebugPrint.h"

#include "../Sys/Input.h"
#include "../Game/UI3D2D.h"
#include "../Sys/Lighting.h"
#include <cmath>
#include <algorithm>

static void DrawGroundGrid(const VECTOR& planePoint, const VECTOR& planeNormal, float size = 60.0f, float step = 1.0f)
{
    unsigned int col = GetColor(60, 60, 60);
    unsigned int centerCol = GetColor(120, 120, 160);

    // build orthonormal basis (u, v) on the plane
    VECTOR n = planeNormal;
    // normalize n
    float nlen = sqrtf(n.x*n.x + n.y*n.y + n.z*n.z);
    if (nlen < 1e-6f) n = VGet(0.0f, 1.0f, 0.0f);
    else n = VGet(n.x / nlen, n.y / nlen, n.z / nlen);

    // choose arbitrary vector to cross with (not parallel to n)
    VECTOR arbitrary = (fabsf(n.x) < 0.9f) ? VGet(1.0f, 0.0f, 0.0f) : VGet(0.0f, 1.0f, 0.0f);
    // u = normalize(cross(arbitrary, n)) -> tangent direction 1
    VECTOR u = VCross(arbitrary, n);
    float ulen = sqrtf(u.x*u.x + u.y*u.y + u.z*u.z);
    if (ulen < 1e-6f) u = VGet(1.0f, 0.0f, 0.0f);
    else u = VGet(u.x / ulen, u.y / ulen, u.z / ulen);
    // v = cross(n, u)
    VECTOR v = VCross(n, u);
    float vlen = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
    if (vlen < 1e-6f) v = VGet(0.0f, 0.0f, 1.0f);
    else v = VGet(v.x / vlen, v.y / vlen, v.z / vlen);

    // Draw lines parallel to v for different u offsets
    for (float du = -size; du <= size; du += step) {
        VECTOR offset = VAdd(VScale(u, du), planePoint);
        VECTOR a = VAdd(offset, VScale(v, -size));
        VECTOR b = VAdd(offset, VScale(v, size));
        unsigned int c = (fabsf(du) < 1e-6f) ? centerCol : col;
        DrawLine3D(a, b, c);
    }
    // Draw lines parallel to u for different v offsets
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
    // create minimal systems
    assets_ = new AssetsMgr();
    assets_->Init();

    // initialize lighting (ambient/background)
    Lighting::InitDefault();

    player_ = new Player(assets_);
    camera_ = new CameraRig();
    player_->SetCamera(camera_);

    // create global effect manager only if one is not already set.
    // If another owner already exists, do not take ownership.
    EffectManager* existing = GetGlobalEffectManager();
    if (existing == nullptr) {
        effects_ = new EffectManager();
        SetGlobalEffectManager(effects_);
    } else {
        effects_ = nullptr; // do not own
        // leave existing global manager in place
    }

    // create sample enemies
    enemies_.push_back(new Enemy(VGet(6.0f, 0.0f, 10.0f)));
    enemies_.push_back(new Enemy(VGet(-8.0f, 0.0f, 18.0f)));
    enemies_.push_back(new Enemy(VGet(2.0f, 0.0f, 26.0f)));

    prevTimeMs_ = GetNowCount();

    // init title scene
    titleScene_ = new TitleScene();
    titleScene_->Init(player_, camera_, assets_);

    // inform camera about ground plane (default horizontal at y=0)
    groundPoint_ = VGet(0.0f, 0.0f, 0.0f);
    groundNormal_ = VGet(0.0f, 1.0f, 0.0f);
    camera_->SetGroundPlane(groundPoint_, groundNormal_);
}

SceneMgr::~SceneMgr()
{
    // delete title scene
    if (titleScene_) { delete titleScene_; titleScene_ = nullptr; }

    // delete enemies
    for (auto e : enemies_) { delete e; }
    enemies_.clear();

    // delete camera and player
    if (camera_) { delete camera_; camera_ = nullptr; }
    if (player_) { delete player_; player_ = nullptr; }

    // delete assets
    if (assets_) { delete assets_; assets_ = nullptr; }

    // Do not delete or clear the global effect manager here. It will be
    // cleaned up by the application after DxLib shutdown to avoid
    // graphics/device related teardown races.
    effects_ = nullptr;
}

void SceneMgr::Update()
{
    // simple polling
    PadState pad = PollPad();
    bool lockButton = IsButtonDown(pad, PAD_INPUT_4); // example button

    unsigned int now = GetNowCount();
    float dt = (now - prevTimeMs_) / 1000.0f;
    prevTimeMs_ = now;

    if (currentScene_ == Scene::Title) {
        // update and draw title scene
        bool start = titleScene_->Update();
        titleScene_->Draw();
        if (start) {
            ChangeScene(Scene::Base);
        }
        return;
    }

    // Update player logic only (rendering will be done after other draws so player occludes them)
    player_->UpdateLogic();
    VECTOR ppos = player_->GetPosition();

    // Update global effect manager (logic) with player position so playback positions follow player
    EffectManager* gem = GetGlobalEffectManager();
    if (gem) {
        gem->Update(ppos);
    }

    // Debug: press F1 to spawn an effect in front of player to validate effect playback/visibility
    if (CheckHitKey(KEY_INPUT_F1)) {
        if (gem) {
            // spawn relative to camera (B): slightly in front of camera position so it is surely visible
            VECTOR camPos = camera_->GetCameraPosition();
            VECTOR forwardCam = camera_->GetForwardXZ();
            float fl = sqrtf(forwardCam.x*forwardCam.x + forwardCam.y*forwardCam.y + forwardCam.z*forwardCam.z);
            if (fl > 1e-6f) forwardCam = VGet(forwardCam.x/fl, forwardCam.y/fl, forwardCam.z/fl);
            VECTOR posCam = VAdd(camPos, VAdd(VScale(forwardCam, 4.0f), VGet(0.0f, 0.0f, 0.0f)));
            DebugPrint("F1 spawn effect (camera-front) at %.2f,%.2f,%.2f\n", posCam.x, posCam.y, posCam.z);
            gem->PlayEffectAt(posCam, nullptr, 3.0f);

            // also spawn at player-forward for comparison
            VECTOR forward = VGet(0.0f, 0.0f, 1.0f);
            if (camera_) forward = camera_->GetForwardXZ();
            float fl2 = sqrtf(forward.x*forward.x + forward.y*forward.y + forward.z*forward.z);
            if (fl2 > 1e-6f) forward = VGet(forward.x/fl2, forward.y/fl2, forward.z/fl2);
            VECTOR pos = VAdd(ppos, VAdd(VScale(forward, 1.4f), VGet(0.0f, 1.6f, 0.0f)));
            DebugPrint("F1 spawn effect (player-front) at %.2f,%.2f,%.2f\n", pos.x, pos.y, pos.z);
            gem->PlayEffectAt(pos, nullptr, 3.0f);
        }
    }

    // draw ground grid using plane (true 3D grid on plane)
    DrawGroundGrid(groundPoint_, groundNormal_, 60.0f, 1.0f);

    // Lock-on selection: score = angleDiff * weight + distance
    float bestScore = 1e9f;
    Enemy* best = nullptr;
    for (auto e : enemies_) {
        VECTOR epos = e->GetPosition();
        VECTOR toEnemy = VSub(epos, ppos);
        float dist = VSize(toEnemy);
        // forward: assume camera target is set; use target_ member as look direction, if zero fallback
        VECTOR forward = VSub(target_, ppos);
        float flen = VSize(forward);
        if (flen < 0.0001f) forward = VGet(0,0,1);
        else forward = VScale(forward, 1.0f / flen);

        float dot = (forward.x * toEnemy.x + forward.y * toEnemy.y + forward.z * toEnemy.z) / (dist);
        dot = clamp(dot, -1.0f, 1.0f);
        float angle = acosf(dot); // in radians

        // score: angle weight + normalized distance
        float score = angle * 2.0f + dist * 0.1f;
        if (angle < (30.0f * 3.14159f / 180.0f)) { // within 30deg
            if (score < bestScore) { bestScore = score; best = e; }
        }
    }

    bool locked = (best != nullptr) && lockButton;

    // update camera
    if (locked && best) {
        camera_->Update(ppos, best->GetPosition(), true);
    } else {
        camera_->Update(ppos, VGet(0,0,0), false);
    }

    // Build list of enemies with camera-space depth so we can order draws relative to player
    struct EnemyDepth { Enemy* e; float z; bool uiVisible; float screenZ; float screenX; float screenY; };
    std::vector<EnemyDepth> edepths;

    VECTOR camPos = camera_->GetCameraPosition();
    VECTOR fwd = camera_->GetForward();
    float fl = sqrtf(fwd.x*fwd.x + fwd.y*fwd.y + fwd.z*fwd.z);
    if (fl < 1e-6f) fwd = VGet(0.0f, 0.0f, 1.0f);
    else fwd = VGet(fwd.x/fl, fwd.y/fl, fwd.z/fl);

    for (auto e : enemies_) {
        // update enemy logic
        e->Update();

        VECTOR epos = e->GetPosition();
        VECTOR camTo = VSub(epos, camPos);
        float zcam = fwd.x * camTo.x + fwd.y * camTo.y + fwd.z * camTo.z;

        // determine if UI (reticle) should be drawn
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

    // Draw enemy UI overlays (independent of ordering) - keep previous behavior
    Enemy* bestForUI = best;
    for (const auto &ed : edepths) {
        if (!ed.uiVisible) continue;
        DrawCircle((int)ed.screenX, (int)ed.screenY, 18, GetColor(0,255,255), FALSE, 2);
        DrawEnemyUI(*ed.e, ed.e == bestForUI);
    }

    // compute player camera-space depth
    VECTOR camToPlayer = VSub(ppos, camPos);
    float playerZ = fwd.x * camToPlayer.x + fwd.y * camToPlayer.y + fwd.z * camToPlayer.z;

    // sort enemies by depth descending (far -> near)
    std::sort(edepths.begin(), edepths.end(), [](const EnemyDepth&a, const EnemyDepth&b){ return a.z > b.z; });

    // draw enemies farther than player first
    for (const auto &ed : edepths) {
        if (ed.z > playerZ) {
            ed.e->Draw();
        }
    }

    // draw player
    player_->Draw();

    // (effects drawn after all 3D geometry to avoid render state conflicts)

    // draw enemies nearer than or equal to player (so they appear on top)
    for (const auto &ed : edepths) {
        if (ed.z <= playerZ) {
            ed.e->Draw();
        }
    }

    // Draw global effects (3D) so they render in world space with other 3D geometry
    if (gem) {
        gem->Draw();
    }

    // draw HUD / debug text after draws
    DrawFormatString(10, 10, GetColor(255,255,255), "Enemies: %d  Best: %s", (int)enemies_.size(), best ? "Yes" : "No");
}

void SceneMgr::ChangeScene(Scene s)
{
    currentScene_ = s;
}

SceneMgr::Scene SceneMgr::GetCurrentScene() const
{
    return currentScene_;
}
