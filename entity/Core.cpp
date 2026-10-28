#include "Core.h"

Core::Core() : hp(100), maxHp(100), hitFlashTimer(0) {}

Core::Core(int hp)
    : hp(hp > 0 ? hp : 1),
      maxHp(hp > 0 ? hp : 1),
      hitFlashTimer(0) {}

void Core::takeDamage(int damage) {
    if (damage <= 0) return;
    hp -= damage;
    if (hp < 0) hp = 0;
    hitFlashTimer = 12;
}

void Core::heal(int amount) {
    if (amount <= 0) return;
    hp += amount;
    if (hp > maxHp) hp = maxHp;
}

bool Core::isDestroyed() const { return hp <= 0; }

int Core::getHp() const { return hp; }
int Core::getMaxHp() const { return maxHp; }

double Core::getHpRatio() const {
    if (maxHp <= 0) return 0.0;
    return static_cast<double>(hp) / static_cast<double>(maxHp);
}

int Core::getHitFlashTimer() const { return hitFlashTimer; }
void Core::setHitFlashTimer(int t) { hitFlashTimer = t; }

void Core::setHp(int v) {
    hp = v;
    if (hp < 0) hp = 0;
    if (hp > maxHp) hp = maxHp;
}

void Core::reset() {
    hp = maxHp;
    hitFlashTimer = 0;
}
