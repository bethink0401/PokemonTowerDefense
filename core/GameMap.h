#pragma once
// =============================================================
//  GameMap.h
//  战场：组合 Core（has-a）+ vector<Pokemon*> + vector<Enemy*>
//  实验二：组合与依赖   实验五：vector 管理多态对象
//  支持多条预定义路径（不做 A*）
// =============================================================
#include <string>
#include <vector>
#include "Core.h"
#include "Position.h"

class Pokemon;
class Enemy;
class EventManager;
struct BattleContext;

class GameMap {
private:
    Core core;
    std::vector<Pokemon*> defenders;   // 基类指针容器：多态统一管理
    std::vector<Enemy*> enemies;

    std::vector<std::vector<Position>> paths;   // 旧接口（整数格，森林图不再使用）
    std::vector<std::vector<FWaypoint>> pointPaths; // 道路中心线路径（浮点）
    FWaypoint doorPoint{0, 0};                  // 终点：宝可梦中心正门
    std::vector<std::vector<bool>> roadGrid;    // 道路格子标记
    std::vector<std::string> terrain;           // 地形：'#'路 'G'草坪 '.'阻挡 '~'水
    Position spawnPoint;
    Position corePosition;

    EventManager* eventManager;
    void rebuildRoadGrid();

public:
    GameMap();
    ~GameMap();

    void setPaths(const std::vector<std::vector<Position>>& p);
    const std::vector<std::vector<Position>>& getPaths() const { return paths; }
    std::vector<Position> getMainPath() const {
        return paths.empty() ? std::vector<Position>() : paths.front();
    }
    Position getSpawnPoint() const { return spawnPoint; }
    Position getCorePosition() const { return corePosition; }

    // 路径格子（敌人行走的道路，不允许部署）
    bool isRoad(const Position& pos) const;
    // 森林路线图：只有深色草甸可以部署宝可梦
    void setTerrain(const char* const* rows);
    bool isTallGrass(const Position& pos) const;
    // 森林路线图：浮点中心线路径 + 门坐标
    void setPointPaths(const std::vector<std::vector<FWaypoint>>& p);
    const std::vector<std::vector<FWaypoint>>& getPointPaths() const { return pointPaths; }
    const FWaypoint& getDoorPoint() const { return doorPoint; }

    void addPokemon(Pokemon* pokemon);
    void removePokemon(Pokemon* pokemon);
    void addEnemy(Enemy* enemy);
    void removeEnemy(Enemy* enemy);
    void clear();

    // 每帧更新：防守方攻击 + 敌方移动 + 死亡结算
    void update(double dt, BattleContext& ctx);

    Core& getCore() { return core; }
    const Core& getCore() const { return core; }

    std::vector<Pokemon*>& getDefenders() { return defenders; }
    const std::vector<Pokemon*>& getDefenders() const { return defenders; }

    std::vector<Enemy*>& getEnemies() { return enemies; }
    const std::vector<Enemy*>& getEnemies() const { return enemies; }

    void setEventManager(EventManager* em) { eventManager = em; }

    int getEnemyCount() const { return static_cast<int>(enemies.size()); }
    int getDefenderCount() const { return static_cast<int>(defenders.size()); }
};
