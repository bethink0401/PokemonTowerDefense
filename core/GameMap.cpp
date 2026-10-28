#include "GameMap.h"
#include "Pokemon.h"
#include "Enemy.h"
#include "BattleContext.h"
#include "EventManager.h"
#include <cstdlib>
#include <cstddef>
#include <cmath>

GameMap::GameMap()
    : core(120),
      spawnPoint(0, 6),
      corePosition(19, 6),
      eventManager(nullptr) {}

GameMap::~GameMap() {
    clear();
}

void GameMap::setPaths(const std::vector<std::vector<Position>>& p) {
    paths = p;
    if (!paths.empty() && !paths.front().empty()) {
        spawnPoint = paths.front().front();
        corePosition = paths.front().back();
    }
    rebuildRoadGrid();
}

void GameMap::setTerrain(const char* const* rows) {
    terrain.clear();
    for (int y = 0; y < Position::MAP_HEIGHT && rows[y]; ++y)
        terrain.push_back(std::string(rows[y]));
    rebuildRoadGrid();
}

void GameMap::setPointPaths(const std::vector<std::vector<FWaypoint>>& p) {
    pointPaths = p;
    if (!pointPaths.empty() && !pointPaths.front().empty()) {
        doorPoint = pointPaths.front().back();
        const FWaypoint& f = pointPaths.front().front();
        spawnPoint = Position(static_cast<int>(f.x), static_cast<int>(f.y));
        corePosition = Position(static_cast<int>(doorPoint.x), static_cast<int>(doorPoint.y));
    }
}

bool GameMap::isTallGrass(const Position& pos) const {
    if (!pos.isValid()) return false;
    int x = pos.getX(), y = pos.getY();
    if (y < 0 || y >= static_cast<int>(terrain.size())) return false;
    if (x < 0 || x >= static_cast<int>(terrain[y].size())) return false;
    return terrain[y][x] == 'G';
}

// 点到线段距离
static double roadDistToSeg(double px, double py,
                            double x1, double y1, double x2, double y2) {
    double dx = x2 - x1, dy = y2 - y1;
    double L2 = dx * dx + dy * dy;
    double t = 0.0;
    if (L2 > 1e-9) {
        t = ((px - x1) * dx + (py - y1) * dy) / L2;
        if (t < 0.0) t = 0.0;
        if (t > 1.0) t = 1.0;
    }
    double cx = x1 + t * dx, cy = y1 + t * dy;
    return std::sqrt((px - cx) * (px - cx) + (py - cy) * (py - cy));
}

void GameMap::rebuildRoadGrid() {
    if (!terrain.empty()) {
        roadGrid.assign(Position::MAP_HEIGHT,
                        std::vector<bool>(Position::MAP_WIDTH, false));
        for (int y = 0; y < Position::MAP_HEIGHT && y < static_cast<int>(terrain.size()); ++y)
            for (int x = 0; x < Position::MAP_WIDTH && x < static_cast<int>(terrain[y].size()); ++x)
                roadGrid[y][x] = (terrain[y][x] == '#');
        return;
    }
    roadGrid.assign(Position::MAP_HEIGHT,
                    std::vector<bool>(Position::MAP_WIDTH, false));
    const double ROAD_W = 0.60;   // 与渲染层保持一致
    for (int y = 0; y < Position::MAP_HEIGHT; ++y) {
        for (int x = 0; x < Position::MAP_WIDTH; ++x) {
            double best = 1e9;
            for (const std::vector<Position>& path : paths) {
                for (size_t i = 0; i + 1 < path.size(); ++i) {
                    double d = roadDistToSeg(x, y,
                                             path[i].getX(), path[i].getY(),
                                             path[i + 1].getX(), path[i + 1].getY());
                    if (d < best) best = d;
                }
            }
            if (best < ROAD_W) roadGrid[y][x] = true;
        }
    }
}

bool GameMap::isRoad(const Position& pos) const {
    if (!pos.isValid()) return false;
    int x = pos.getX(), y = pos.getY();
    if (y < 0 || y >= static_cast<int>(roadGrid.size())) return false;
    if (x < 0 || x >= static_cast<int>(roadGrid[y].size())) return false;
    return roadGrid[y][x];
}

void GameMap::addPokemon(Pokemon* pokemon) {
    if (pokemon) defenders.push_back(pokemon);
}

void GameMap::removePokemon(Pokemon* pokemon) {
    for (auto it = defenders.begin(); it != defenders.end(); ++it) {
        if (*it == pokemon) {
            delete *it;                 // GameMap 负责释放
            defenders.erase(it);
            return;
        }
    }
}

void GameMap::addEnemy(Enemy* enemy) {
    if (!enemy) return;
    if (!pointPaths.empty()) {
        // 随机选择一条中心线路线进入（山洞 / 路尽头）
        int idx = std::rand() % static_cast<int>(pointPaths.size());
        enemy->setPath(pointPaths[idx]);
    }
    enemies.push_back(enemy);
}

void GameMap::removeEnemy(Enemy* enemy) {
    for (auto it = enemies.begin(); it != enemies.end(); ++it) {
        if (*it == enemy) {
            delete *it;
            enemies.erase(it);
            return;
        }
    }
}

void GameMap::clear() {
    for (Pokemon* p : defenders) delete p;
    defenders.clear();
    for (Enemy* e : enemies) delete e;
    enemies.clear();
}

void GameMap::update(double dt, BattleContext& ctx) {
    ctx.enemies = &enemies;   // 让普通攻击 / 反应能看到当前战场

    // 1) 防守方：冷却推进 + 选择目标 + 攻击（基类指针调用虚函数）
    for (Pokemon* p : defenders) {
        if (!p || p->isDead()) continue;
        p->clearAttackFlag();
        p->update(dt);

        Enemy* target = p->selectTarget(enemies);
        if (target && p->canAttack() && p->inRange(target->getPosition())) {
            long long before=0,after=0;
            for(auto* e:enemies)if(e)before+=e->getHp();
            p->attack(*target);   // 多态：不同宝可梦不同行为
            for(auto* e:enemies)if(e)after+=e->getHp();
            p->recordDamage(before-after);
        }
    }

    // 2) 敌人：状态结算 + 移动
    for (Enemy* e : enemies) {
        if (e) e->update(dt);
    }

    // 3) 死亡 / 抵达核心结算（通过事件通知其它模块）
    for (size_t i = 0; i < enemies.size();) {
        Enemy* e = enemies[i];
        if (!e) {
            enemies.erase(enemies.begin() + static_cast<long>(i));
            continue;
        }
        if (e->isDead()) {
            if (eventManager) {
                EnemyDeadEvent ev(e->getName(), e->getReward(), e->getPosition());
                eventManager->handle(ev);
            }
            delete e;
            enemies.erase(enemies.begin() + static_cast<long>(i));
        } else if (e->hasReachedCore()) {
            core.takeDamage(e->getDamageToCore());
            if (eventManager) {
                CoreDamagedEvent ev(e->getDamageToCore(), core.getHp());
                eventManager->handle(ev);
            }
            delete e;
            enemies.erase(enemies.begin() + static_cast<long>(i));
        } else {
            ++i;
        }
    }
}
