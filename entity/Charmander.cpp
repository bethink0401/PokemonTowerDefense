#include "Charmander.h"
#include "Enemy.h"
#include "StatusEffect.h"
#include "BattleContext.h"
#include <algorithm>
#include <cmath>

Charmander::Charmander(const std::string& name, int maxHp, int atk, double rng,
                       double interval, int cost)
    : Pokemon(name, maxHp, atk, rng, interval, cost, PokemonType::Fire) {
}

void Charmander::attack(Enemy& target) {
    Pokemon::attack(target);
    if (!didAttackThisFrame) return;
    if (context && context->enemies) {
        const int damage = std::max(1, static_cast<int>(std::round(
            attackPower * TypeSystem::getMultiplier(type, target.getType()))));
        for (Enemy* other : *context->enemies) {
            if (other && other != &target && !other->isDead() &&
                other->getPosition().distanceTo(target.getPosition()) <= getSplashRadius()) {
                other->takeDamage(std::max(1, static_cast<int>(damage * getSplashRatio())));
            }
        }
    }
    // 小火龙家族特色：命中后附加灼烧
    if (!target.isDead()) {
        target.addStatusEffect(new BurnEffect(2.0, 2.0+stage));
    }
}

void Charmander::update(double dt) {
    Pokemon::update(dt);
}
