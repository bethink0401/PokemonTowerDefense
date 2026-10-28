#include "GameEvent.h"
#include <sstream>

EnemyDeadEvent::EnemyDeadEvent(const std::string& name, int reward, const Position& pos)
    : enemyName(name), reward(reward), position(pos) {}

std::string EnemyDeadEvent::toString() const {
    std::ostringstream oss;
    oss << "EnemyDead(" << enemyName << ", +" << reward << "g, " << position.toString() << ")";
    return oss.str();
}

CoreDamagedEvent::CoreDamagedEvent(int damage, int remainingHp)
    : damage(damage), remainingHp(remainingHp) {}

std::string CoreDamagedEvent::toString() const {
    std::ostringstream oss;
    oss << "CoreDamaged(-" << damage << ", hp=" << remainingHp << ")";
    return oss.str();
}

WaveClearEvent::WaveClearEvent(int wave, int bonus)
    : wave(wave), bonus(bonus) {}

std::string WaveClearEvent::toString() const {
    std::ostringstream oss;
    oss << "WaveClear(wave=" << wave << ", +" << bonus << "g)";
    return oss.str();
}

ReactionTriggeredEvent::ReactionTriggeredEvent(const std::string& name, const Position& pos)
    : reactionName(name), position(pos) {}

std::string ReactionTriggeredEvent::toString() const {
    std::ostringstream oss;
    oss << "Reaction(" << reactionName << " @ " << position.toString() << ")";
    return oss.str();
}
