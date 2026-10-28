#pragma once
// =============================================================
//  GameEvent.h
//  简化版事件对象：避免 Enemy::die() 里直接调用 player / statistics / ui
//  实验三/四：继承体系 + 多态 getType()
// =============================================================
#include <string>
#include "Position.h"

enum class EventType {
    EnemyDead,
    CoreDamaged,
    WaveClear,
    ReactionTriggered
};

class GameEvent {
public:
    virtual ~GameEvent() {}
    virtual EventType getType() const = 0;
    virtual std::string toString() const = 0;
};

// 敌人死亡
class EnemyDeadEvent : public GameEvent {
private:
    std::string enemyName;
    int reward;
    Position position;
public:
    EnemyDeadEvent(const std::string& name, int reward, const Position& pos);
    EventType getType() const override { return EventType::EnemyDead; }
    std::string toString() const override;

    const std::string& getEnemyName() const { return enemyName; }
    int getReward() const { return reward; }
    Position getPosition() const { return position; }
};

// 核心受损
class CoreDamagedEvent : public GameEvent {
private:
    int damage;
    int remainingHp;
public:
    CoreDamagedEvent(int damage, int remainingHp);
    EventType getType() const override { return EventType::CoreDamaged; }
    std::string toString() const override;

    int getDamage() const { return damage; }
    int getRemainingHp() const { return remainingHp; }
};

// 波次清空
class WaveClearEvent : public GameEvent {
private:
    int wave;
    int bonus;
public:
    WaveClearEvent(int wave, int bonus);
    EventType getType() const override { return EventType::WaveClear; }
    std::string toString() const override;

    int getWave() const { return wave; }
    int getBonus() const { return bonus; }
};

// 连携反应触发
class ReactionTriggeredEvent : public GameEvent {
private:
    std::string reactionName;
    Position position;
public:
    ReactionTriggeredEvent(const std::string& name, const Position& pos);
    EventType getType() const override { return EventType::ReactionTriggered; }
    std::string toString() const override;

    const std::string& getReactionName() const { return reactionName; }
    Position getPosition() const { return position; }
};
