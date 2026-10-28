#pragma once
// =============================================================
//  Pikachu.h
//  皮卡丘：电属性，皮丘的进化形态（阶段 1）
//  进化链：Pichu -> Pikachu -> Raichu
//  实验三：多层继承 —— 复用 Pichu 的行为族系，仅增强数值
// =============================================================
#include "Pichu.h"

class Pikachu : public Pichu {
public:
    Pikachu() : Pikachu("Pikachu", 165, 16, 3.0, 0.76, 100) {
        setStage(1);
    }
    std::string getClassName() const override { return "Pikachu"; }
    const char* nextStageId() const override { return "Raichu"; }
protected:
    Pikachu(const std::string& name, int maxHp, int atk, double rng,
            double interval, int cost)
        : Pichu(name, maxHp, atk, rng, interval, cost) {}
};
