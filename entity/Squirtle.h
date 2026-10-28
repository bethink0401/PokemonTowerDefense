#pragma once
// =============================================================
//  Squirtle.h
//  杰尼龟：水属性、控制（减速）
//  进化链：Squirtle -> Wartortle -> Blastoise
// =============================================================
#include "Pokemon.h"

class Squirtle : public Pokemon {
public:
    Squirtle() : Squirtle("Squirtle", 150, 12, 2.8, 1.35, 120) {}
    void attack(Enemy& target) override;
    void update(double dt) override;
    std::string getClassName() const override { return "Squirtle"; }
    const char* nextStageId() const override { return "Wartortle"; }

protected:
    Squirtle(const std::string& name, int maxHp, int atk, double rng,
             double interval, int cost);
};
