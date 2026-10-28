#pragma once
// =============================================================
//  Charmeleon.h
//  火恐龙：火属性，小火龙的进化形态（阶段 1）
//  进化链：Charmander -> Charmeleon -> Charizard
//  实验三：多层继承 —— 复用 Charmander 的行为族系，仅增强数值
// =============================================================
#include "Charmander.h"

class Charmeleon : public Charmander {
public:
    Charmeleon() : Charmeleon("Charmeleon", 150, 30, 2.55, 1.75, 150) {
        setStage(1);
    }
    std::string getClassName() const override { return "Charmeleon"; }
    const char* nextStageId() const override { return "Charizard"; }
protected:
    Charmeleon(const std::string& name, int maxHp, int atk, double rng,
            double interval, int cost)
        : Charmander(name, maxHp, atk, rng, interval, cost) {}
};
