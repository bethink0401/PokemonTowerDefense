#pragma once
// =============================================================
//  ElementStatus.h
//  属性附着 —— "目标身上当前附着着什么属性"
//  与 PokemonType（固有属性）严格区分
//  实验三/四：继承体系 + 虚函数
// =============================================================
#include <string>
#include "TypeSystem.h"


class ElementStatus {
    friend class SaveManager;
protected:
    PokemonType type;
    double remainingTime;   // 剩余存在时间
    int strength;           // 旧存档兼容字段，附着本身不产生效果
    double tickAcc;         // 旧存档兼容字段，不再推进或结算

public:
    ElementStatus(PokemonType t, double time, int strength);
    virtual ~ElementStatus();

    virtual void update(double dt);

    bool isExpired() const;

    PokemonType getType() const { return type; }
    double getRemainingTime() const { return remainingTime; }
    void setRemainingTime(double t) { remainingTime = t; }
    int getStrength() const { return strength; }

    virtual std::string getClassName() const { return "ElementStatus"; }
    virtual std::string toSaveString() const;
};

class FireStatus : public ElementStatus {
public:
    FireStatus(double time, int strength);
    std::string getClassName() const override { return "FireStatus"; }
};

class WaterStatus : public ElementStatus {
public:
    WaterStatus(double time, int strength);
    std::string getClassName() const override { return "WaterStatus"; }
};

class GrassStatus : public ElementStatus {
public:
    GrassStatus(double time, int strength);
    std::string getClassName() const override { return "GrassStatus"; }
};

class ElectricStatus : public ElementStatus {
public:
    ElectricStatus(double time, int strength);
    std::string getClassName() const override { return "ElectricStatus"; }
};


// 工厂：按属性创建对应的附着（多态创建）
ElementStatus* createElementStatus(PokemonType type, double time, int strength);
