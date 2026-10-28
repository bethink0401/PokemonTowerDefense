#pragma once
// =============================================================
//  TargetStrategy.h
//  目标选择策略 —— 与 Pokemon 分离的独立变化维度（组合 + 抽象 + 多态）
//  实验四：基类指针 strategy->selectTarget(...) 运行时多态
// =============================================================
#include <string>
#include <vector>
#include "Position.h"

class Enemy;
class Pokemon;

class TargetStrategy {
public:
    virtual ~TargetStrategy() {}

    virtual Enemy* selectTarget(const std::vector<Enemy*>& enemies,
                                const Pokemon& pokemon) = 0;

    virtual std::string getName() const = 0;
};

// 最近目标
class NearestStrategy : public TargetStrategy {
public:
    Enemy* selectTarget(const std::vector<Enemy*>& enemies, const Pokemon& pokemon) override;
    std::string getName() const override { return "Nearest"; }
};

// 血量最低目标
class LowestHPStrategy : public TargetStrategy {
public:
    Enemy* selectTarget(const std::vector<Enemy*>& enemies, const Pokemon& pokemon) override;
    std::string getName() const override { return "LowestHP"; }
};

// 距离核心最近（最危险）的目标
class ClosestToCoreStrategy : public TargetStrategy {
private:
    Position corePosition;
public:
    explicit ClosestToCoreStrategy(const Position& corePos = Position());
    void setCorePosition(const Position& p) { corePosition = p; }
    Enemy* selectTarget(const std::vector<Enemy*>& enemies, const Pokemon& pokemon) override;
    std::string getName() const override { return "ClosestToCore"; }
};

// 速度最快的目标
class FastestEnemyStrategy : public TargetStrategy {
public:
    Enemy* selectTarget(const std::vector<Enemy*>& enemies, const Pokemon& pokemon) override;
    std::string getName() const override { return "Fastest"; }
};

// 工厂：按名称创建策略（存档读取时重建）
TargetStrategy* createStrategyByName(const std::string& name, const Position& corePos = Position());
