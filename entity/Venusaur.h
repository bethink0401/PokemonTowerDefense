#pragma once
// =============================================================
//  Venusaur.h
//  妙蛙花：草属性，妙蛙种子家族的最终形态（阶段 2）
//  进化链：Bulbasaur -> Ivysaur -> Venusaur
//  实验三：多层继承 —— 复用 Ivysaur 的行为族系，仅增强数值
// =============================================================
#include "Ivysaur.h"

class Venusaur : public Ivysaur {
public:
    Venusaur() : Venusaur("Venusaur", 245, 45, 3.9, 1.55, 130) {
        setStage(2);
    }
    std::string getClassName() const override { return "Venusaur"; }
    const char* nextStageId() const override { return nullptr; }   // 最终形态
protected:
    Venusaur(const std::string& name, int maxHp, int atk, double rng,
            double interval, int cost)
        : Ivysaur(name, maxHp, atk, rng, interval, cost) {}
};
