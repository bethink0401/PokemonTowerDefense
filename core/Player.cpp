#include "Player.h"

Player::Player() : gold(400), score(0) {}

Player::Player(int gold, int score)
    : gold(gold > 0 ? gold : 0), score(score > 0 ? score : 0) {}

void Player::addGold(int amount) {
    if (amount <= 0) return;
    gold += amount;
}

bool Player::spendGold(int amount) {
    if (amount <= 0) return true;
    if (gold < amount) return false;
    gold -= amount;
    return true;
}

bool Player::canAfford(int amount) const {
    return gold >= amount;
}

void Player::addScore(int amount) {
    if (amount <= 0) return;
    score += amount;
}

void Player::reset() {
    gold = 400;
    score = 0;
}
