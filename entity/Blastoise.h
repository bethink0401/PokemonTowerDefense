#pragma once
// =============================================================
//  Blastoise.h
//  水箭龟：水属性，杰尼龟家族的最终形态（阶段 2）
//  进化链：Squirtle -> Wartortle -> Blastoise
//  实验三：多层继承 —— 复用 Wartortle 的行为族系，仅增强数值
// =============================================================
#include "Wartortle.h"

class Blastoise : public Wartortle {
public:
    Blastoise() : Blastoise("Blastoise", 255, 22, 3.2, 1.25, 120) {
        setStage(2);
    }
    std::string getClassName() const override { return "Blastoise"; }
    const char* nextStageId() const override { return nullptr; }   // 最终形态
protected:
    Blastoise(const std::string& name, int maxHp, int atk, double rng,
            double interval, int cost)
        : Wartortle(name, maxHp, atk, rng, interval, cost) {}
};
