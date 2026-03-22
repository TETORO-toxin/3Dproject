// Player_AuxAndCombat.cpp
// サマリー:
//  - 補助ユニット（Aux）の更新と発射処理、被弾/無敵判定、回復/覚醒状態のハンドリングを含みます。
//  - ゲームプレイに関わるステータス変化や、外部からの攻撃通知を受けるAPIを提供します。

#include "Player.h"
#include "AuxUnit.h"
#include "Enemy.h"
#include "Projectile.h"

VECTOR Player::GetPosition() const
{
    return VGet(x_, y_, z_);
}

void Player::SetCamera(CameraRig* cam)
{
    camera_ = cam;
}

void Player::OnIncomingAttack()
{
    lastIncomingTimeMs_ = GetNowCount();
}

bool Player::IsInvulnerable() const
{
    unsigned int now = GetNowCount();
    return (now - lastDodgeTimeMs_) < dodgeDurationMs_;
}

void Player::SetAwakened(bool v)
{
    awakened = v;
    if (v) {
        // バフパラメータ
        attackCooldownMs_ = baseAttackCooldownMs_ * 0.6f;
        auxGaugeRegenRate = baseAuxGaugeRegenRate * 3.0f;
        justWindowMs_ = baseJustWindowMs_ + 100;
    } else {
        attackCooldownMs_ = baseAttackCooldownMs_;
        auxGaugeRegenRate = baseAuxGaugeRegenRate;
        justWindowMs_ = baseJustWindowMs_;
    }
}

void Player::Heal(float amount)
{
    hp += amount;
    if (hp > maxHp) hp = maxHp;
}

void Player::UpdateAux(float dt, ProjectileManager& pm)
{
    if (auxLeft) auxLeft->Update(dt);
    if (auxRight) auxRight->Update(dt);

    // パッシブ回復
    if (!awakened) {
        auxGauge += auxGaugeRegenRate * dt;
        if (auxGauge > 100.0f) auxGauge = 100.0f;
    } else {
        // awakened状態: 発射コストなしだがゲージは上限で制限
        auxGauge += auxGaugeRegenRate * dt;
        if (auxGauge > 100.0f) auxGauge = 100.0f;
    }
}

void Player::FireAuxLeft(ProjectileManager& pm)
{
    if (!auxLeft) return;
    VECTOR origin = VAdd(GetPosition(), VGet(-1.2f, 1.6f, 0.0f));
    Enemy* target = nullptr;
    float cost = 10.0f;
    auxLeft->Fire(origin, target, pm, !awakened, auxGauge, cost);
}

void Player::FireAuxRight(ProjectileManager& pm)
{
    if (!auxRight) return;
    VECTOR origin = VAdd(GetPosition(), VGet(1.2f, 1.6f, 0.0f));
    Enemy* target = nullptr;
    float cost = 25.0f;
    auxRight->Fire(origin, target, pm, !awakened, auxGauge, cost);
}
