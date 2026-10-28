#include "Squirtle.h"
#include "Enemy.h"
#include "StatusEffect.h"

Squirtle::Squirtle(const std::string& name, int maxHp, int atk, double rng,
                   double interval, int cost)
    : Pokemon(name, maxHp, atk, rng, interval, cost, PokemonType::Water) {
}

void Squirtle::attack(Enemy& target) {
    Pokemon::attack(target);
    if (!didAttackThisFrame) return;
    // 杰尼龟家族特色：命中后附加减速，为连携反应创造附着条件
    if (!target.isDead()) {
        target.addStatusEffect(new SlowEffect(getSlowDuration(), getSlowRatio()));
    }
}

void Squirtle::update(double dt) {
    Pokemon::update(dt);
}
