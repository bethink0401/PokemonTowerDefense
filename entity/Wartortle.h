#pragma once
// =============================================================
//  Wartortle.h
//  卡咪龟：水属性，杰尼龟的进化形态（阶段 1）
//  进化链：Squirtle -> Wartortle -> Blastoise
//  实验三：多层继承 —— 复用 Squirtle 的行为族系，仅增强数值
// =============================================================
#include "Squirtle.h"

class Wartortle : public Squirtle {
public:
    Wartortle() : Wartortle("Wartortle", 195, 17, 3.0, 1.3, 120) {
        setStage(1);
    }
    std::string getClassName() const override { return "Wartortle"; }
    const char* nextStageId() const override { return "Blastoise"; }
protected:
    Wartortle(const std::string& name, int maxHp, int atk, double rng,
            double interval, int cost)
        : Squirtle(name, maxHp, atk, rng, interval, cost) {}
};
