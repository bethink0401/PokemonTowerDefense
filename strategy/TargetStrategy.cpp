#include "TargetStrategy.h"
#include "Enemy.h"
#include "Pokemon.h"

// ---------------- NearestStrategy ----------------
Enemy* NearestStrategy::selectTarget(const std::vector<Enemy*>& enemies, const Pokemon& pokemon) {
    Enemy* best = nullptr;
    double bestDist = 1e18;
    for (Enemy* e : enemies) {
        if (!e || e->isDead() || !pokemon.inRange(e->getPosition())) continue;
        double d = pokemon.getPosition().distanceTo(e->getPosition());
        if (d < bestDist) { bestDist = d; best = e; }
    }
    return best;
}

// ---------------- LowestHPStrategy ----------------
Enemy* LowestHPStrategy::selectTarget(const std::vector<Enemy*>& enemies, const Pokemon& pokemon) {
    Enemy* best = nullptr;
    int bestHp = 0x7fffffff;
    for (Enemy* e : enemies) {
        if (!e || e->isDead() || !pokemon.inRange(e->getPosition())) continue;
        if (e->getHp() < bestHp) { bestHp = e->getHp(); best = e; }
    }
    return best;
}

// ---------------- ClosestToCoreStrategy ----------------
ClosestToCoreStrategy::ClosestToCoreStrategy(const Position& corePos)
    : corePosition(corePos) {}

Enemy* ClosestToCoreStrategy::selectTarget(const std::vector<Enemy*>& enemies, const Pokemon& pokemon) {
    Enemy* best = nullptr;
    double bestDist = 1e18;
    for (Enemy* e : enemies) {
        if (!e || e->isDead() || !pokemon.inRange(e->getPosition())) continue;
        double d = corePosition.distanceTo(e->getPosition());
        if (d < bestDist) { bestDist = d; best = e; }
    }
    return best;
}

// ---------------- FastestEnemyStrategy ----------------
Enemy* FastestEnemyStrategy::selectTarget(const std::vector<Enemy*>& enemies, const Pokemon& pokemon) {
    Enemy* best = nullptr;
    double bestSpeed = -1.0;
    for (Enemy* e : enemies) {
        if (!e || e->isDead() || !pokemon.inRange(e->getPosition())) continue;
        if (e->getBaseSpeed() > bestSpeed) { bestSpeed = e->getBaseSpeed(); best = e; }
    }
    return best;
}

// ---------------- 工厂 ----------------
TargetStrategy* createStrategyByName(const std::string& name, const Position& corePos) {
    if (name == "LowestHP")      return new LowestHPStrategy();
    if (name == "ClosestToCore") return new ClosestToCoreStrategy(corePos);
    if (name == "Fastest")       return new FastestEnemyStrategy();
    return new NearestStrategy();
}
