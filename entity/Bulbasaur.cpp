#include "Bulbasaur.h"
#include "Enemy.h"
#include "StatusEffect.h"

Bulbasaur::Bulbasaur(const std::string& name, int maxHp, int atk, double rng,
                     double interval, int cost)
    : Pokemon(name, maxHp, atk, rng, interval, cost, PokemonType::Grass) {
}

void Bulbasaur::attack(Enemy& target) {
    Pokemon::attack(target);

}

void Bulbasaur::update(double dt) {
    Pokemon::update(dt);
}
