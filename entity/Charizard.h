#pragma once
// =============================================================
//  Charizard.h
//  喷火龙：火属性，小火龙家族的最终形态（阶段 2）
//  进化链：Charmander -> Charmeleon -> Charizard
//  实验三：多层继承 —— 复用 Charmeleon 的行为族系，仅增强数值
// =============================================================
#include "Charmeleon.h"

class Charizard : public Charmeleon {
public:
    Charizard() : Charizard("Charizard", 205, 38, 2.7, 1.7, 150) {
        setStage(2);
    }
    std::string getClassName() const override { return "Charizard"; }
    const char* nextStageId() const override { return nullptr; }   // 最终形态
protected:
    Charizard(const std::string& name, int maxHp, int atk, double rng,
            double interval, int cost)
        : Charmeleon(name, maxHp, atk, rng, interval, cost) {}
};
