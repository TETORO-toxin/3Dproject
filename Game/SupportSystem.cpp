#include "SupportSystem.h"
#include "Player.h"
#include "Enemy.h"

SupportSystem::SupportSystem()
{
}

void SupportSystem::Tick(float dt, Player* player)
{
    TickSupport(support_, dt);
    if (healCooldownRemaining_ > 0.0f) healCooldownRemaining_ -= dt;
    if (remoteCooldownRemaining_ > 0.0f) remoteCooldownRemaining_ -= dt;

    // apply awakening effects to player
    if (support_.awakening && player) {
        player->SetAwakened(true);
    } else if (player) {
        player->SetAwakened(false);
    }
}

bool SupportSystem::ActivateAwakening(Player* player)
{
    if (support_.awakening) return false;
    ::ActivateAwakening(support_, 8.0f);
    return true;
}

bool SupportSystem::EmergencyHeal(Player* player, float amount)
{
    if (healCooldownRemaining_ > 0.0f || !player) return false;
    player->Heal(amount);
    healCooldownRemaining_ = healCooldownMax_;
    return true;
}

bool SupportSystem::RemoteSupportAttack(Enemy* target, float weakGain)
{
    if (remoteCooldownRemaining_ > 0.0f || !target) return false;
    target->ApplyHitWEAK(weakGain);
    remoteCooldownRemaining_ = remoteCooldownMax_;
    return true;
}
