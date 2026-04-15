#include "TitleScene.h"
#include "../Game/Player.h"
#include "../Game/CameraRig.h"
#include "../Sys/Input.h"
#include "DxLib.h"
#include <Windows.h>
#include <vector>
#include <cmath>
#include <cstdlib>

// =========================
//  2D/3D 演出用ユーティリティ
// =========================
static int irand(int a, int b) { return a + (rand() % (b - a + 1)); }
static float frand() { return (float)rand() / (float)RAND_MAX; }

static void AddTri(std::vector<VERTEX3D>& v,
    const VECTOR& p0, const VECTOR& p1, const VECTOR& p2,
    const VECTOR& n, const COLOR_U8& dif)
{
    VERTEX3D a{};
    a.pos = p0;
    a.norm = n;
    a.dif = dif;
    a.spc = GetColorU8(0, 0, 0, 0);
    a.u = a.v = a.su = a.sv = 0.0f;

    VERTEX3D b = a; b.pos = p1;
    VERTEX3D c = a; c.pos = p2;

    v.push_back(a); v.push_back(b); v.push_back(c);
}

// 内装向け：面が内側に向く箱（廊下内部が見える）
static void AddBoxInside(std::vector<VERTEX3D>& v, VECTOR mn, VECTOR mx, COLOR_U8 dif)
{
    VECTOR p000 = VGet(mn.x, mn.y, mn.z);
    VECTOR p001 = VGet(mn.x, mn.y, mx.z);
    VECTOR p010 = VGet(mn.x, mx.y, mn.z);
    VECTOR p011 = VGet(mn.x, mx.y, mx.z);
    VECTOR p100 = VGet(mx.x, mn.y, mn.z);
    VECTOR p101 = VGet(mx.x, mn.y, mx.z);
    VECTOR p110 = VGet(mx.x, mx.y, mn.z);
    VECTOR p111 = VGet(mx.x, mx.y, mx.z);

    // inward faces
    AddTri(v, p100, p111, p110, VGet(-1, 0, 0), dif);
    AddTri(v, p100, p101, p111, VGet(-1, 0, 0), dif);

    AddTri(v, p000, p010, p011, VGet(1, 0, 0), dif);
    AddTri(v, p000, p011, p001, VGet(1, 0, 0), dif);

    AddTri(v, p010, p110, p111, VGet(0, -1, 0), dif);
    AddTri(v, p010, p111, p011, VGet(0, -1, 0), dif);

    AddTri(v, p000, p001, p101, VGet(0, 1, 0), dif);
    AddTri(v, p000, p101, p100, VGet(0, 1, 0), dif);

    AddTri(v, p001, p011, p111, VGet(0, 0, -1), dif);
    AddTri(v, p001, p111, p101, VGet(0, 0, -1), dif);

    AddTri(v, p000, p100, p110, VGet(0, 0, 1), dif);
    AddTri(v, p000, p110, p010, VGet(0, 0, 1), dif);
}


void TitleScene::Init(Player* player, CameraRig* camera, AssetsMgr* assets)
{
    (void)player; (void)camera; (void)assets; // 使わないので警告抑制

    // Use current monitor resolution so title render target matches actual screen
    int W = GetSystemMetrics(SM_CXSCREEN);
    int H = GetSystemMetrics(SM_CYSCREEN);
    // store for use in Draw (scanline/noise)
    mWidth = W;
    mHeight = H;

    mTime = 0.0f;
    mStartRequested = false;

    srand(12345);

    mSceneGraph = MakeScreen(W, H, TRUE);

    mFontTitle = CreateFontToHandle("Meiryo UI", 56, 6, DX_FONTTYPE_ANTIALIASING_8X8);
    mFontSub = CreateFontToHandle("Meiryo UI", 24, 4, DX_FONTTYPE_ANTIALIASING_8X8);

    SetUseZBuffer3D(TRUE);
    SetWriteZBuffer3D(TRUE);
    SetUseBackCulling(TRUE);
    SetUseLighting(FALSE);

    SetFogEnable(TRUE);
    SetFogColor(8, 12, 14);
    SetFogStartEnd(500.0f, 2200.0f);
}
void TitleScene::Finalize()
{
    if (mSceneGraph != -1) { DeleteGraph(mSceneGraph); mSceneGraph = -1; }
    if (mFontTitle != -1) { DeleteFontToHandle(mFontTitle); mFontTitle = -1; }
    if (mFontSub != -1) { DeleteFontToHandle(mFontSub); mFontSub = -1; }
}

bool TitleScene::Update()
{
    // deltaを取れるなら置換。固定でOK
    mTime += 1.0f / 60.0f;

    // Enterで開始（シーン遷移は外側で）
    if (CheckHitKey(KEY_INPUT_RETURN))
    {
        mStartRequested = true;
    }
    // return whether start was requested so the scene manager can transition
    return mStartRequested;
}

void TitleScene::Draw()
{
    const int W = mWidth, H = mHeight;

    // ---- カメラ：自動前進 + 微揺れ ----
    float speed = 70.0f;
    float z = -180.0f + mTime * speed;

    float sway = sinf(mTime * 1.7f) * 6.0f;
    float bob = sinf(mTime * 2.1f) * 3.0f;
    float yaw = sinf(mTime * 0.7f) * 0.10f;
    float pitch = sinf(mTime * 0.9f) * 0.05f;

    VECTOR camPos = VGet(sway, 120.0f + bob, z);
    VECTOR forward = VGet(sinf(yaw) * cosf(pitch), -sinf(pitch), cosf(yaw) * cosf(pitch));
    VECTOR camTarget = VAdd(camPos, forward);

    // ---- 電源フリッカ（瞬断）----
    float flick = 1.0f;
    if (fmod(mTime, 9.0f) > 8.70f) flick = 0.25f;
    if (fmod(mTime, 13.0f) > 12.88f) flick = 0.10f;

    auto scaleCol = [&](COLOR_U8 c)->COLOR_U8 {
        int r = (int)(c.r * flick);
        int g = (int)(c.g * flick);
        int b = (int)(c.b * flick);
        if (r < 0) r = 0; if (g < 0) g = 0; if (b < 0) b = 0;
        return GetColorU8(r, g, b, c.a);
        };

    COLOR_U8 metal = scaleCol(GetColorU8(120, 130, 140, 255));
    COLOR_U8 dark = scaleCol(GetColorU8(65, 72, 80, 255));
    COLOR_U8 panel = scaleCol(GetColorU8(92, 102, 112, 255));
    COLOR_U8 led = scaleCol(GetColorU8(40, 220, 255, 255));
    COLOR_U8 warnOn = scaleCol(GetColorU8(255, 60, 40, 255));
    COLOR_U8 warnOf = scaleCol(GetColorU8(70, 15, 12, 255));

    // =====================
    //  まず3D+UIを mSceneGraph に描く
    // =====================
    SetDrawScreen(mSceneGraph);
    ClearDrawScreen();
    DrawBox(0, 0, W, H, GetColor(0, 0, 0), TRUE);

    SetCameraPositionAndTarget_UpVecY(camPos, camTarget);

    // ---- 廊下メッシュ生成（カメラ周りだけ）----
    mGeo.clear();

    float halfW = 220.0f;
    float floorY = 0.0f;
    float ceilY = 260.0f;

    float zStart = z - 200.0f;
    float zEnd = z + 3200.0f;
    int i0 = (int)floor(zStart / 240.0f);
    int i1 = (int)ceil(zEnd / 240.0f);

    for (int i = i0; i <= i1; i++)
    {
        float z0 = (float)i * 240.0f;
        float z1 = z0 + 240.0f;

        // 床/天井
        AddBoxInside(mGeo, VGet(-halfW, floorY - 22, z0), VGet(halfW, floorY, z1), dark);
        AddBoxInside(mGeo, VGet(-halfW, ceilY, z0), VGet(halfW, ceilY + 22, z1), dark);

        // 左右壁
        AddBoxInside(mGeo, VGet(-halfW - 22, floorY, z0), VGet(-halfW, ceilY, z1), metal);
        AddBoxInside(mGeo, VGet(halfW, floorY, z0), VGet(halfW + 22, ceilY, z1), metal);

        // パネル
        AddBoxInside(mGeo, VGet(-halfW - 18, 40, z0 + 30), VGet(-halfW - 2, 140, z0 + 150), panel);
        AddBoxInside(mGeo, VGet(halfW + 2, 60, z0 + 60), VGet(halfW + 18, 160, z0 + 190), panel);

        // 梁（2タイルに1回）
        if ((i & 1) == 0)
        {
            AddBoxInside(mGeo, VGet(-halfW, ceilY - 16, z0), VGet(halfW, ceilY + 6, z0 + 20), metal);
            AddBoxInside(mGeo, VGet(-halfW - 6, floorY, z0), VGet(-halfW + 14, ceilY, z0 + 20), metal);
            AddBoxInside(mGeo, VGet(halfW - 14, floorY, z0), VGet(halfW + 6, ceilY, z0 + 20), metal);
        }

        // LEDライン（天井左右）
        AddBoxInside(mGeo, VGet(-halfW + 30, ceilY - 9, z0), VGet(-halfW + 40, ceilY - 7, z1), led);
        AddBoxInside(mGeo, VGet(halfW - 40, ceilY - 9, z0), VGet(halfW - 30, ceilY - 7, z1), led);

        // ケーブルラック（配管風）
        AddBoxInside(mGeo, VGet(-halfW + 10, 200, z0), VGet(-halfW + 22, 210, z1), metal);
        AddBoxInside(mGeo, VGet(-halfW + 24, 200, z0), VGet(-halfW + 36, 210, z1), metal);

        // 警告灯（点滅：セグメントごとに位相ズレ）
        float phase = fmod(mTime + i * 0.17f, 1.4f);
        bool blink = phase < 0.14f;
        COLOR_U8 wcol = blink ? warnOn : warnOf;
        AddBoxInside(mGeo, VGet(-halfW + 55, 230, z0 + 10), VGet(-halfW + 75, 250, z0 + 30), wcol);
    }

    if (!mGeo.empty())
    {
        DrawPolygon3D(mGeo.data(), (int)mGeo.size() / 3, DX_NONE_GRAPH, FALSE);
    }

    // UI（mSceneGraph上に描く）
    DrawStringToHandle(60, 70, "ORBITAL STATION : ENGINE SECTOR", GetColor(200, 240, 255), mFontTitle);

    int blinkA = (int)(120 + 120 * (0.5f + 0.5f * sinf(mTime * 3.0f)));
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, blinkA);
    DrawStringToHandle(60, 140, "PRESS ENTER", GetColor(220, 230, 240), mFontSub);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // =====================
    //  back に貼って Scanline/Noise を重ねる
    // =====================
    SetDrawScreen(DX_SCREEN_BACK);
    DrawGraph(0, 0, mSceneGraph, FALSE);

    // Scanline
    for (int y2 = 0; y2 < H; y2 += 3)
    {
        int a2 = (y2 % 6 == 0) ? 22 : 10;
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, a2);
        DrawLine(0, y2, W, y2, GetColor(0, 0, 0));
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // Noise
    int dots = 900;
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 28);
    for (int i = 0; i < dots; i++)
    {
        int x = irand(0, W - 1);
        int y2 = irand(0, H - 1);
        int c = 140 + (int)(frand() * 115);
        DrawPixel(x, y2, GetColor(c, c, c));
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // ※ScreenFlip() は「メインループ側で1回」呼ぶ想定
}