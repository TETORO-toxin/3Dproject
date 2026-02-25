#include "TitleScene.h"
#include "../Game/Player.h"
#include "../Game/CameraRig.h"
#include "../Sys/Input.h"

TitleScene::TitleScene()
{
}

TitleScene::~TitleScene()
{
}

void TitleScene::Init(Player* player, CameraRig* camera, AssetsMgr* assets)
{
    player_ = player;
    camera_ = camera;
    assets_ = assets;
    prevTimeMs_ = GetNowCount();
}

bool TitleScene::Update()
{
    unsigned int now = GetNowCount();
    float dt = (now - prevTimeMs_) / 1000.0f;
    prevTimeMs_ = now;

    // Rotate camera slowly around player
    angle_ += dt * (3.14159f / 16.0f); // slow
    float radius = 18.0f;
    VECTOR eye = VGet(sinf(angle_) * radius, 6.0f, cosf(angle_) * radius);
    VECTOR tgt = player_->GetPosition();
    camera_->SetTarget(tgt.x, tgt.y + 2.0f, tgt.z);
    SetCameraNearFar(0.1f, 1200.0f);
    SetCameraPositionAndTarget_UpVecY(eye, tgt);

    // dark background
    DrawBox(0, 0, 800, 600, GetColor(10,10,20), TRUE);

    // draw player model
    player_->Update();

    // Title text centered
    const char* title = "MY GAME TITLE";
    int w = GetDrawStringWidth(title, strlen(title));
    DrawString((800 - w) / 2, 220, title, GetColor(200,200,255));

    // PRESS ENTER blinking
    if ((now / 500) % 2 == 0) {
        const char* prompt = "PRESS ENTER";
        int pw = GetDrawStringWidth(prompt, strlen(prompt));
        DrawString((800 - pw) / 2, 300, prompt, GetColor(200,200,200));
    }

    if (CheckHitKey(KEY_INPUT_RETURN)) {
        return true;
    }
    return false;
}
