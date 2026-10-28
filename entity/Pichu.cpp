#include "Pichu.h"
#include "Enemy.h"
#include <cstdlib>

Pichu::Pichu(const std::string& name, int maxHp, int atk, double rng,
             double interval, int cost)
    : Pokemon(name, maxHp, atk, rng, interval, cost, PokemonType::Electric) {
}

void Pichu::attack(Enemy& target) {
    Pokemon::attack(target);
    if (!didAttackThisFrame) return;
    // 皮丘家族特色：20% 概率追加一次电击（不影响普通攻击间隔）
    if (!target.isDead() && (std::rand() % 100) < 20) {
        target.takeDamage(4 + level);
    }
}

void Pichu::update(double dt) {
    Pokemon::update(dt);
}
