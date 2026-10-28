#include "WaveManager.h"
#include "Enemy.h"
#include <cstdlib>
#include <sstream>

WaveManager::WaveManager() {
    reset();
}

void WaveManager::reset() {
    currentWave = 0;
    maxWave = 8;
    waveActive = false;
    spawnTimer = 0.0;
    spawnInterval = 0.9;
    toSpawn = 0;
    spawned = 0;
}

void WaveManager::startWave() {
    if (waveActive) return;
    ++currentWave;
    waveActive = true;
    toSpawn = 5 + currentWave * 2;
    spawned = 0;
    spawnTimer = 0.2;
}

void WaveManager::nextWave() {
    waveActive = false;
}

Enemy* WaveManager::makeEnemyForWave() const {
    int w = currentWave;
    int roll = std::rand() % 100;

    Enemy* e = nullptr;
    if (w <= 1) {
        e = createEnemyById("BasicEnemy");
    } else if (w <= 3) {
        e = (roll < 65) ? createEnemyById("BasicEnemy")
                        : createEnemyById("FastEnemy");
    } else {
        if (roll < 45)      e = createEnemyById("BasicEnemy");
        else if (roll < 75) e = createEnemyById("FastEnemy");
        else                e = createEnemyById("TankEnemy");
    }
    if (e) e->applyWaveScaling(w);
    return e;
}

void WaveManager::update(double dt, std::function<void(Enemy*)> spawnCallback) {
    if (!waveActive) return;
    if (spawned >= toSpawn) {
        waveActive = false;
        return;
    }

    spawnTimer -= dt;
    if (spawnTimer <= 0.0) {
        Enemy* e = makeEnemyForWave();
        if (e && spawnCallback) spawnCallback(e);
        ++spawned;
        spawnTimer = spawnInterval;
    }
}

std::string WaveManager::currentWaveComposition() const {
    std::ostringstream oss;
    oss << "第 " << currentWave << " / " << maxWave << " 波  ("
        << spawned << "/" << toSpawn << ")";
    return oss.str();
}
