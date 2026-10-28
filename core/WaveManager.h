#pragma once
// =============================================================
//  WaveManager.h
//  波次：生成敌人 / 判断波次结束 / 推进下一波（不负责战斗细节）
// =============================================================
#include <functional>
#include <string>
#include <vector>
#include "Position.h"

class Enemy;

class WaveManager {
    friend class SaveManager;
private:
    int currentWave;
    int maxWave;
    bool waveActive;      // 是否正在生成敌人
    double spawnTimer;
    double spawnInterval;
    int toSpawn;          // 本波需要生成的总数
    int spawned;          // 本波已生成数

public:
    WaveManager();

    void reset();
    void startWave();
    void nextWave();

    // 每帧推进：按间隔通过回调生成敌人
    void update(double dt, std::function<void(Enemy*)> spawnCallback);

    bool isWaveActive() const { return waveActive; }
    bool isSpawningDone() const { return spawned >= toSpawn; }
    bool isFinalWave() const { return currentWave >= maxWave; }

    int getCurrentWave() const { return currentWave; }
    int getMaxWave() const { return maxWave; }
    int getSpawned() const { return spawned; }
    int getToSpawn() const { return toSpawn; }

    void setCurrentWave(int w) { currentWave = w; }   // 存档恢复用

    std::string currentWaveComposition() const;

private:
    Enemy* makeEnemyForWave() const;
};
