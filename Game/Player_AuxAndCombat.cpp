// Player_AuxAndCombat.cpp
// 概要:
//  - 補助ユニット（Aux）の管理と戦闘関連処理を扱うファイル。
//  - Aux の発射、ゲージ回復、覚醒状態の管理などの API を提供します。

#include "Player.h"
#include "../Sys/Assets.h"
#include "AuxUnit.h"
#include "Enemy.h"
#include "Projectile.h"
#include "WeaponTypes.h"

VECTOR Player::GetPosition() const
{
    return VGet(x_, y_, z_);
}

Game::WeaponType Player::GetEquippedWeapon() const
{
    return equippedWeapon_;
}

// `newWeapon` を装備しようとする。交換が発生した場合は true を返し、
// 以前の装備を `oldOut` に書き込む（nullptr でなければ）。
// `newWeapon` が現在の装備と同じで変更がなかった場合は false を返す。
bool Player::TryEquipWeapon(Game::WeaponType newWeapon, Game::WeaponType* oldOut)
{
    // No-op when equipping the same weapon
    if (newWeapon == equippedWeapon_) return false;

    // Return previous weapon to caller if requested
    if (oldOut) *oldOut = equippedWeapon_;

    // If switching to None, invalidate model handle and update state
    if (newWeapon == Game::WeaponType::None) {
        equippedWeapon_ = Game::WeaponType::None;
        equippedWeaponModelHandle_ = -1; // explicitly invalidate
        return true;
    }

    // Update equipped weapon state and attempt to load an "equip" variant of the model.
    // If loading the equip-specific model fails, try a non-equip fallback. If that also
    // fails leave the handle as -1; drawing code should handle -1 safely.
    equippedWeapon_ = newWeapon;
    equippedWeaponModelHandle_ = -1;
    if (assets_) {
        equippedWeaponModelHandle_ = assets_->GetWeaponModelHandle(newWeapon, /*equip=*/true);
        if (equippedWeaponModelHandle_ == -1) {
            // Fallback: try non-equip model if equip-specific not available
            equippedWeaponModelHandle_ = assets_->GetWeaponModelHandle(newWeapon, /*equip=*/false);
        }
    }

    // If assets_ is null or both loads failed, equippedWeaponModelHandle_ stays -1 which
    // indicates to the renderer that no model is available for this equipped weapon.
    return true;
}

// 後方互換用のヘルパー: 交換が行われた場合は以前の武器を返し、
// そうでない場合は `Game::WeaponType::None` を返す。
Game::WeaponType Player::EquipWeapon(Game::WeaponType newWeapon)
{
    Game::WeaponType old = Game::WeaponType::None;
    if (TryEquipWeapon(newWeapon, &old)) return old;
    return Game::WeaponType::None;
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
