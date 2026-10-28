#pragma once
// =============================================================
//  Raichu.h
//  雷丘：电属性，皮丘家族的最终形态（阶段 2）
//  进化链：Pichu -> Pikachu -> Raichu
//  实验三：多层继承 —— 复用 Pikachu 的行为族系，仅增强数值
// =============================================================
#include "Pikachu.h"

class Raichu : public Pikachu {
public:
    Raichu() : Raichu("Raichu", 215, 20, 3.2, 0.68, 100) {
        setStage(2);
    }
    std::string getClassName() const override { return "Raichu"; }
    const char* nextStageId() const override { return nullptr; }   // 最终形态
protected:
    Raichu(const std::string& name, int maxHp, int atk, double rng,
            double interval, int cost)
        : Pikachu(name, maxHp, atk, rng, interval, cost) {}
};
