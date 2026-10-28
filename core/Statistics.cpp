#include "Statistics.h"
#include <sstream>

Statistics::Statistics() { reset(); }

void Statistics::addKill() { ++kills; }
void Statistics::addDamage(int d) { if (d > 0) totalDamage += d; }
void Statistics::addReaction() { ++reactionsTriggered; }
void Statistics::addGold(int g) { if (g > 0) goldEarned += g; }
void Statistics::addWaveCleared() { ++wavesCleared; }
void Statistics::addLeak() { ++enemiesLeaked; }

void Statistics::reset() {
    kills = 0;
    totalDamage = 0;
    reactionsTriggered = 0;
    goldEarned = 0;
    wavesCleared = 0;
    enemiesLeaked = 0;
}

std::string Statistics::toString() const {
    std::ostringstream oss;
    oss << "击杀 " << kills
        << " | 元素反应 " << reactionsTriggered
        << " | 漏怪 " << enemiesLeaked
        << " | 累计金币 " << goldEarned;
    return oss.str();
}
