#pragma once
// =============================================================
//  Pichu.h
//  皮丘：电属性、高频单体输出（皮丘家族的最初形态）
//  实验三：继承与派生（is-a Pokemon）
//  实验四：重写 attack()/update() 体现多态
//  进化链：Pichu -> Pikachu -> Raichu
// =============================================================
#include "Pokemon.h"

class Pichu : public Pokemon {
public:
    Pichu() : Pichu("Pichu", 120, 12, 2.8, 0.85, 100) {}
    void attack(Enemy& target) override;
    void update(double dt) override;
    std::string getClassName() const override { return "Pichu"; }
    const char* nextStageId() const override { return "Pikachu"; }

protected:
    // 供进化形态复用：同一家族共享行为族系，只替换数值
    Pichu(const std::string& name, int maxHp, int atk, double rng,
          double interval, int cost);
};
