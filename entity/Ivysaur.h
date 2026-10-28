#pragma once
// =============================================================
//  Ivysaur.h
//  妙蛙草：草属性，妙蛙种子的进化形态（阶段 1）
//  进化链：Bulbasaur -> Ivysaur -> Venusaur
//  实验三：多层继承 —— 复用 Bulbasaur 的行为族系，仅增强数值
// =============================================================
#include "Bulbasaur.h"

class Ivysaur : public Bulbasaur {
public:
    Ivysaur() : Ivysaur("Ivysaur", 185, 35, 3.5, 1.6, 130) {
        setStage(1);
    }
    std::string getClassName() const override { return "Ivysaur"; }
    const char* nextStageId() const override { return "Venusaur"; }
protected:
    Ivysaur(const std::string& name, int maxHp, int atk, double rng,
            double interval, int cost)
        : Bulbasaur(name, maxHp, atk, rng, interval, cost) {}
};
