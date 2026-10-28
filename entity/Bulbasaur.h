#pragma once
// =============================================================
//  Bulbasaur.h
//  妙蛙种子：草属性、普通攻击与草附着
//  进化链：Bulbasaur -> Ivysaur -> Venusaur
// =============================================================
#include "Pokemon.h"

class Bulbasaur : public Pokemon {
public:
    Bulbasaur() : Bulbasaur("Bulbasaur", 140, 26, 3.1, 1.65, 130) {}
    void attack(Enemy& target) override;
    void update(double dt) override;
    std::string getClassName() const override { return "Bulbasaur"; }
    const char* nextStageId() const override { return "Ivysaur"; }

protected:
    Bulbasaur(const std::string& name, int maxHp, int atk, double rng,
              double interval, int cost);
};
