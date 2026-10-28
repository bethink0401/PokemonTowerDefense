#pragma once
// =============================================================
//  Game.h
//  顶层调度：组合 GameMap / Player / WaveManager / ReactionEngine /
//            EventManager / Statistics（实验二 组合 + 依赖）
//  Game 负责调度，不承担任何具体业务细节（不是万能类）
// =============================================================
#include <string>
#include <vector>
#include "GameMap.h"
#include "Player.h"
#include "WaveManager.h"
#include "Statistics.h"
#include "ReactionEngine.h"
#include "EventManager.h"
#include "BattleContext.h"

class Pokemon;
class Enemy;
class SaveManager;

class Game {
    friend class SaveManager;   // 存档需要读写对象内部状态
private:
    GameMap map;
    Player player;
    WaveManager waveManager;
    ReactionEngine reactionEngine;
    EventManager eventManager;
    Statistics statistics;
    BattleContext context;

    bool running;
    bool paused;
    bool gameOver;
    bool victory;

    int evolutionCount;                 // 已发生的进化次数（供 UI 播放特效）
    Position lastEvolvePos;             // 最近一次进化的位置

    std::vector<std::string> log;       // 最近事件日志（供 UI 显示）

public:
    Game();

    // 生命周期
    void start();
    void restart();
    void pause();
    void resume();
    void reset();
    void update(double dt);

    // 玩家操作
    Pokemon* deployPokemon(const std::string& typeName, const Position& pos);
    bool upgradePokemon(Pokemon*& p);   // 传入指针引用：进化会替换对象，调用方需同步
    bool sellPokemon(Pokemon* p);
    bool setPokemonStrategy(Pokemon* p, const std::string& strategyName);
    bool isPositionOccupied(const Position& pos) const;
    bool isDeployable(const Position& pos) const;   // 非道路且未被占用

    // 存档
    bool save(const std::string& filename);
    bool load(const std::string& filename);

    // 状态查询
    bool isRunning() const { return running; }
    bool isPaused() const { return paused; }
    bool isGameOver() const { return gameOver; }
    bool isVictory() const { return victory; }
    bool isOver() const { return gameOver || victory; }

    GameMap& getMap() { return map; }
    const GameMap& getMap() const { return map; }
    Player& getPlayer() { return player; }
    const Player& getPlayer() const { return player; }
    WaveManager& getWaveManager() { return waveManager; }
    const WaveManager& getWaveManager() const { return waveManager; }
    ReactionEngine& getReactionEngine() { return reactionEngine; }
    EventManager& getEventManager() { return eventManager; }
    Statistics& getStatistics() { return statistics; }
    const Statistics& getStatistics() const { return statistics; }
    const std::vector<std::string>& getLog() const { return log; }

    // 进化可视化（UI 轮询这两个值即可播放特效）
    int getEvolutionCount() const { return evolutionCount; }
    const Position& getLastEvolvePosition() const { return lastEvolvePos; }

    static int deployCost(const std::string& typeName);
    static std::string defaultStrategyFor(const std::string& typeName);

private:
    void buildMap();
    void tryEvolve(Pokemon*& p);
    void subscribeEvents();
    void addLog(const std::string& msg);
    void applyContextToDefenders();
};
