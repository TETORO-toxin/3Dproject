#include <Windows.h>
#include "main.h"
#include "App/SceneMgr.h"

// Helper: confine cursor to the current active window's client area
static bool ConfineCursorToActiveWindow()
{
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) return false;
    RECT rcClient;
    if (!GetClientRect(hwnd, &rcClient)) return false;
    POINT tl = { rcClient.left, rcClient.top };
    POINT br = { rcClient.right, rcClient.bottom };
    // convert client coords to screen coords
    MapWindowPoints(hwnd, NULL, &tl, 1);
    MapWindowPoints(hwnd, NULL, &br, 1);
    RECT r = { tl.x, tl.y, br.x, br.y };
    return (ClipCursor(&r) == TRUE);
}

// Helper: release cursor clipping
static void ReleaseCursorClip()
{
    ClipCursor(NULL);
}

//-------------------------------------------------------//
//　　　　　　　　【なつやすみかだい】　　　　　　　　   //
// 自分の好きなゲームを作ってみよう！ 　　　　　　　　   //
//                                                       //
// Init(),Update()に処理を実装しよう！　　　　　　　　   //
// 必要な変数,関数はmain.hに追加してみよう！　　　　     //
// 使いたい処理があれば下記リンクから探して使ってみよう！//
// https://dxlib.xsrv.jp/dxfunc.html                     //
// 　　　　　　　　　　　　　　　　　　　　　　　　　　　//
//-------------------------------------------------------//


/// <summary>
/// アプリケーションを起動する補助関数（WinMainはDXLibが提供するため重複定義を避ける）
/// ビルド／実行はDxLibが提供するエントリポイントから行われます。
/// </summary>
int RunApp()
{
    // Choose monitor to fullscreen: use the monitor containing the mouse cursor.
    POINT pt;
    if (GetCursorPos(&pt)) {
        HMONITOR hMon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = {};
        mi.cbSize = sizeof(mi);
        if (GetMonitorInfo(hMon, &mi)) {
            int monitorW = mi.rcMonitor.right - mi.rcMonitor.left;
            int monitorH = mi.rcMonitor.bottom - mi.rcMonitor.top;
            // Set DxLib graph mode to the monitor resolution and request fullscreen exclusive
            SetGraphMode(monitorW, monitorH, 32);
            ChangeWindowMode(FALSE); // fullscreen
        } else {
            // fallback to primary display size if monitor info failed
            int pw = GetSystemMetrics(SM_CXSCREEN);
            int ph = GetSystemMetrics(SM_CYSCREEN);
            SetGraphMode(pw, ph, 32);
            ChangeWindowMode(FALSE);
        }
    } else {
        // fallback to primary display
        int pw = GetSystemMetrics(SM_CXSCREEN);
        int ph = GetSystemMetrics(SM_CYSCREEN);
        SetGraphMode(pw, ph, 32);
        ChangeWindowMode(FALSE);
    }

    SetMainWindowText("GameProgramming - DXLib");

    if (DxLib_Init() == -1) return -1;
    SetDrawScreen(DX_SCREEN_BACK);

    SceneMgr* scene = new SceneMgr();
    scene->Init();

    // track scene changes
    auto prevScene = scene->GetCurrentScene();

    // If starting in gameplay scene, confine cursor immediately
    if (prevScene == SceneMgr::Scene::Base) {
        ConfineCursorToActiveWindow();
    }

    // Main loop
    while (ProcessMessage() == 0 && ClearDrawScreen() == 0)
    {
        scene->Update();

        // detect scene transition
        auto currScene = scene->GetCurrentScene();
        if (currScene != prevScene) {
            // simple on-change handling (could play SFX, reset timers, etc.)
            const char* names[] = { "Title", "Base", "Mission", "Result" };
            const char* from = names[(int)prevScene];
            const char* to = names[(int)currScene];
            DrawFormatString(10, 560, GetColor(255,255,0), "Scene changed: %s -> %s", from, to);

            // When entering gameplay (Base) confine cursor to game window; otherwise release it
            if (currScene == SceneMgr::Scene::Base) {
                ConfineCursorToActiveWindow();
            } else {
                ReleaseCursorClip();
            }

            prevScene = currScene;
        }

        // Example high-level handling: if reached Result, wait for Enter to return to Title
        if (currScene == SceneMgr::Scene::Result) {
            DrawFormatString(300, 560, GetColor(255,180,100), "Press Enter to return to Title");
            if (CheckHitKey(KEY_INPUT_RETURN)) {
                scene->ChangeScene(SceneMgr::Scene::Title);
            }
        }

        ScreenFlip();
        // simple frame limiter (~60fps)
        WaitTimer(16);
    }

    // ensure cursor clipping is released on exit
    ReleaseCursorClip();

    delete scene;
    DxLib_End();
    return 0;
}

/// <summary>
/// デストラクタでシーンを解放
/// </summary>
Main::~Main()
{
    if (scene_) {
        delete scene_;
        scene_ = nullptr;
    }
}

/// <summary>
/// ゲームを起動して最初の１回のみ実行される処理
/// </summary>
void Main::Init()
{
    // initialize SceneMgr used by the DxLibMain loop
    scene_ = new SceneMgr();
    scene_->Init();

    // optional small hint
    DrawString(0, 0, "Initialized main scene", GetColor(255, 255, 255));
}

/// <summary>
/// 繰り返し呼び出される処理　(１秒間に30～120回)
/// </summary>
void Main::Update()
{
    if (scene_) {
        scene_->Update();
    } else {
        DrawString(0, 0, "Scene not initialized", GetColor(255, 0, 0));
    }
}