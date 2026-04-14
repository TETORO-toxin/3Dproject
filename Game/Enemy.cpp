#include "Enemy.h"
#include <cmath>

Enemy::Enemy(const VECTOR& pos)
    : pos_(pos)
{
    // モデルは持たない: 敵は簡易な直方体として描画する
    modelHandle_ = -1;
}

Enemy::~Enemy()
{
    // モデルは所有していないため削除処理は不要
}

void Enemy::Update()
{
    // 簡易的な空中物理（重力・着地判定）
    if (airborne) {
        velY -= 9.8f * (1.0f/60.0f); // 重力
        pos_.y += velY * (1.0f/60.0f);
        if (pos_.y <= 0.0f) {
            pos_.y = 0.0f;
            airborne = false;
            velY = 0.0f;
            // 着地: 状態変化を引き起こす可能性がある
        }
    }
}

void Enemy::Draw()
{
    // World 空間にワイヤーフレームの直方体として描画する。投影の反転アーティファクトを避けるため DrawLine3D を使用。
    const float halfX = 0.5f;
    const float halfZ = 0.5f;
    const float height = 2.0f;

    VECTOR base = pos_; // base center
    VECTOR corners[8];
    // bottom face
    corners[0] = VAdd(base, VGet(-halfX, 0.0f, -halfZ));
    corners[1] = VAdd(base, VGet( halfX, 0.0f, -halfZ));
    corners[2] = VAdd(base, VGet( halfX, 0.0f,  halfZ));
    corners[3] = VAdd(base, VGet(-halfX, 0.0f,  halfZ));
    // top face
    corners[4] = VAdd(base, VGet(-halfX, height, -halfZ));
    corners[5] = VAdd(base, VGet( halfX, height, -halfZ));
    corners[6] = VAdd(base, VGet( halfX, height,  halfZ));
    corners[7] = VAdd(base, VGet(-halfX, height,  halfZ));

    unsigned int colEdge = isWeak ? GetColor(200,200,255) : GetColor(255,100,100);

    auto drawEdge3D = [&](int a, int b){
        DrawLine3D(corners[a], corners[b], colEdge);
    };

    // 底面
    drawEdge3D(0,1); drawEdge3D(1,2); drawEdge3D(2,3); drawEdge3D(3,0);
    // top
    drawEdge3D(4,5); drawEdge3D(5,6); drawEdge3D(6,7); drawEdge3D(7,4);
    // verticals
    drawEdge3D(0,4); drawEdge3D(1,5); drawEdge3D(2,6); drawEdge3D(3,7);
}

VECTOR Enemy::GetPosition() const { return pos_; }

void Enemy::ApplyHitWEAK(float weakGain)
{
    if (!isWeak) {
        // WEAK ゲージを加算
        weakGauge = weakGauge + weakGain;
        if (weakGauge > 100.0f) weakGauge = 100.0f;
        if (weakGauge >= 100.0f) {
            isWeak = true;
            // TODO: エフェクト生成
        }
    }
}

void Enemy::Launch()
{
    if (isWeak && !airborne) {
        airborne = true;
        velY = 12.0f; // initial upward velocity
    }
}

void Enemy::FinishAssault()
{
    if (airborne) {
        // big damage / down
        airborne = false;
        isWeak = false;
        weakGauge = 0.0f;
        pos_.y = 0.0f; // ensure grounded
        velY = 0.0f;
    }
}
