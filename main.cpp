#include <Windows.h>
#include "main.h"
#include "App/SceneMgr.h"
#include "Sys/GlobalEffects.h"
// フレームレート固定用ユーティリティ
#include "Sys/FrameLimiter.h"

// ヘルパー: フォアグラウンドウィンドウのクライアント領域にカーソルを制限する
static bool ConfineCursorToActiveWindow()
{
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) return false;
    RECT rcClient;
    if (!GetClientRect(hwnd, &rcClient)) return false;
    POINT tl = { rcClient.left, rcClient.top };
    POINT br = { rcClient.right, rcClient.bottom };
    // クライアント座標をスクリーン座標に変換
    MapWindowPoints(hwnd, NULL, &tl, 1);
    MapWindowPoints(hwnd, NULL, &br, 1);
    RECT r = { tl.x, tl.y, br.x, br.y };
    return (ClipCursor(&r) == TRUE);
}

// ヘルパー: カーソルのクリッピングを解除する
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
    // フルスクリーンにするモニターの選択: マウスカーソルがあるモニターを使用
    POINT pt;
    if (GetCursorPos(&pt)) {
        HMONITOR hMon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = {};
        mi.cbSize = sizeof(mi);
        if (GetMonitorInfo(hMon, &mi)) {
            int monitorW = mi.rcMonitor.right - mi.rcMonitor.left;
            int monitorH = mi.rcMonitor.bottom - mi.rcMonitor.top;
            // DxLibのグラフモードをモニター解像度に設定し、フルスクリーンを要求する
            SetGraphMode(monitorW, monitorH, 32);
            ChangeWindowMode(FALSE); // フルスクリーン
        } else {
            // モニター情報が取得できなかった場合はプライマリ表示をフォールバック
            int pw = GetSystemMetrics(SM_CXSCREEN);
            int ph = GetSystemMetrics(SM_CYSCREEN);
            SetGraphMode(pw, ph, 32);
            ChangeWindowMode(FALSE);
        }
    } else {
        // プライマリ表示をフォールバック
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

    // フレームレート固定: ここで FrameLimiter を作成します。
    // どのスクリーン（解像度/モニタ）でも同じ FPS になるよう高精度タイマーで制御します。
    FrameLimiter limiter(60); // 60 FPS 固定

    // シーンの変更を追跡
    auto prevScene = scene->GetCurrentScene();

    // ゲームプレイシーンで開始する場合は即座にカーソルを制限
    if (prevScene == SceneMgr::Scene::Base) {
        ConfineCursorToActiveWindow();
    }

    // メインループ
    while (ProcessMessage() == 0 && ClearDrawScreen() == 0)
    {
        // フレーム先頭のタイムスタンプを取得
        limiter.FrameStart();

        // 更新処理
        scene->Update();

        // シーン遷移を検出
        auto currScene = scene->GetCurrentScene();
        if (currScene != prevScene) {
            // シンプルな遷移時処理（SFX再生やタイマーリセットなど）
            const char* names[] = { "Title", "Base", "Mission", "Result" };
            const char* from = names[(int)prevScene];
            const char* to = names[(int)currScene];
            DrawFormatString(10, 560, GetColor(255,255,0), "Scene changed: %s -> %s", from, to);

            // ゲームプレイ（Base）に入るときはカーソルをゲームウィンドウに制限し、それ以外は解除
            if (currScene == SceneMgr::Scene::Base) {
                ConfineCursorToActiveWindow();
            } else {
                ReleaseCursorClip();
            }

            prevScene = currScene;
        }

        // 高レベルな例: Resultに達したらEnterでTitleに戻る
        if (currScene == SceneMgr::Scene::Result) {
            DrawFormatString(300, 560, GetColor(255,180,100), "Press Enter to return to Title");
            if (CheckHitKey(KEY_INPUT_RETURN)) {
                scene->ChangeScene(SceneMgr::Scene::Title);
            }
        }

        ScreenFlip();

        // フレーム末尾で次フレーム開始まで待機します
        limiter.FrameEndWait();
    }

    // 終了時にカーソルのクリッピングを必ず解除
    ReleaseCursorClip();

    // 重要: 描画が停止した後、まずシーンを解放する（MV1DeleteModel 等がここで走っても DxLib はまだ有効）
    if (scene) {
        delete scene;
        scene = nullptr;
    }

    // 次にグローバルな EffectManager を削除する（DeleteEffekseerEffect 等がここで走っても DxLib はまだ有効）
    {
        EffectManager* gm = GetGlobalEffectManager();
        if (gm) {
            SetGlobalEffectManager(nullptr);
            delete gm;
        }
    }

    // 最後に DxLib の終了処理を行う
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