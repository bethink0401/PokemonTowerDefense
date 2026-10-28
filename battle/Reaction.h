#pragma once
// =============================================================
//  Reaction.h
//  属性元素反应 —— 项目最有辨识度的玩法机制
//  实验四：纯虚函数 canTrigger()/execute() 运行时多态
// =============================================================
#include <string>
#include "TypeSystem.h"

class Enemy;

class Reaction {
public:
    virtual ~Reaction() {}

    // 判断"已有附着 oldType" 遇到 "新属性 newType" 是否触发
    virtual bool canTrigger(PokemonType oldType, PokemonType newType) const = 0;

    // 触发效果
    virtual void execute(Enemy& enemy) = 0;

    virtual std::string getName() const = 0;
    virtual std::string getDescription() const = 0;

    // 是否为范围效果（由 ReactionEngine 负责溅射）
    virtual bool isArea() const { return false; }
    virtual int getAreaDamage() const { return 0; }
    virtual double getAreaRadius() const { return 2.0; }
};



// 水 + 电 -> 电导链（单体伤害与随机停顿）
class ConductReaction : public Reaction {
public:
    bool canTrigger(PokemonType o, PokemonType n) const override;
    void execute(Enemy& enemy) override;
    std::string getName() const override { return "电导链"; }
    std::string getDescription() const override { return "水+电：单体伤害并随机停顿"; }
};

// 火 + 草 -> 燃烧（持续伤害）
class BurnReaction : public Reaction {
public:
    bool canTrigger(PokemonType o, PokemonType n) const override;
    void execute(Enemy& enemy) override;
    std::string getName() const override { return "燃烧"; }
    std::string getDescription() const override { return "火+草：持续灼烧"; }
};

// 水 + 草 -> 缠绕生长（减速 / 束缚）
class GrowthReaction : public Reaction {
public:
    bool canTrigger(PokemonType o, PokemonType n) const override;
    void execute(Enemy& enemy) override;
    std::string getName() const override { return "缠绕生长"; }
    std::string getDescription() const override { return "水+草：强力减速"; }
};

// 火 + 水 -> 蒸汽爆发（范围伤害 + 遮蔽减速）
class SteamReaction : public Reaction {
public:
    bool canTrigger(PokemonType o, PokemonType n) const override;
    void execute(Enemy& enemy) override;
    std::string getName() const override { return "蒸汽爆发"; }
    std::string getDescription() const override { return "火+水：蒸汽范围伤害并减速"; }
    bool isArea() const override { return true; }
    int getAreaDamage() const override { return 6; }
    double getAreaRadius() const override { return 1.5; }
};


// 电 + 草 -> 麻痹孢子（随机停顿与范围伤害）
class SporeReaction : public Reaction {
public:
    bool canTrigger(PokemonType o, PokemonType n) const override;
    void execute(Enemy& enemy) override;
    std::string getName() const override { return "麻痹孢子"; }
    std::string getDescription() const override { return "电+草：目标随机停顿，周围小额伤害"; }
    bool isArea() const override { return true; }
    int getAreaDamage() const override { return 4; }
    double getAreaRadius() const override { return 1.6; }
};

// 火 + 电 -> 能量爆破（小范围 AOE）
class ExplosionReaction : public Reaction {
public:
    bool canTrigger(PokemonType o, PokemonType n) const override;
    void execute(Enemy& enemy) override;
    std::string getName() const override { return "能量爆破"; }
    std::string getDescription() const override { return "火+电：小范围 AOE 伤害"; }
    bool isArea() const override { return true; }
    int getAreaDamage() const override { return 12; }
    double getAreaRadius() const override { return 1.6; }
};
