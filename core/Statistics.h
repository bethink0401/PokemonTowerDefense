#pragma once
// =============================================================
//  Statistics.h
//  战斗统计：由 EventManager 分发的事件驱动更新
// =============================================================
#include <string>

class Statistics {
    friend class SaveManager;
private:
    int kills;
    int totalDamage;
    int reactionsTriggered;
    int goldEarned;
    int wavesCleared;
    int enemiesLeaked;

public:
    Statistics();

    void addKill();
    void addDamage(int d);
    void addReaction();
    void addGold(int g);
    void addWaveCleared();
    void addLeak();

    int getKills() const { return kills; }
    int getTotalDamage() const { return totalDamage; }
    int getReactionsTriggered() const { return reactionsTriggered; }
    int getGoldEarned() const { return goldEarned; }
    int getWavesCleared() const { return wavesCleared; }
    int getEnemiesLeaked() const { return enemiesLeaked; }

    void reset();
    std::string toString() const;
};
