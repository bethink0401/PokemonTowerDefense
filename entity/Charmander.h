#pragma once
// =============================================================
//  Charmander.h
//  小火龙：火属性、范围伤害（小火龙家族的最初形态）
//  进化链：Charmander -> Charmeleon -> Charizard
// =============================================================
#include "Pokemon.h"

class Charmander : public Pokemon {
public:
    Charmander() : Charmander("Charmander", 110, 23, 2.4, 1.8, 150) {}
    void attack(Enemy& target) override;
    void update(double dt) override;
    std::string getClassName() const override { return "Charmander"; }
    const char* nextStageId() const override { return "Charmeleon"; }

protected:
    Charmander(const std::string& name, int maxHp, int atk, double rng,
               double interval, int cost);
};
