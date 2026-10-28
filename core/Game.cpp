#include "Game.h"
#include "ForestMapData.h"
#include "Pokemon.h"
#include "Enemy.h"
#include "TargetStrategy.h"
#include "SaveManager.h"
#include <sstream>
#include <algorithm>

Game::Game()
    : running(false), paused(false), gameOver(false), victory(false),
      evolutionCount(0) {
    buildMap();
    map.setEventManager(&eventManager);
    context.reactionEngine = &reactionEngine;
    context.enemies = &map.getEnemies();

    subscribeEvents();

    // 元素反应触发后，通过事件通知统计 / UI
    reactionEngine.setReactionCallback([this](const std::string& name, const Position& pos) {
        ReactionTriggeredEvent ev(name, pos);
        eventManager.handle(ev);
    });
    reactionEngine.setEnemies(&map.getEnemies());
}

void Game::buildMap() {
    // 森林路线图：地形由 tools/analyze_forest_map.py 从地图图片分析生成
    // 刷怪点：左上山洞、左下路尽头；终点：右下宝可梦中心门
    map.setTerrain(ForestMap::terrainRows());
    std::vector<std::vector<FWaypoint>> lanes;
    lanes.push_back(ForestMap::pathCave());
    lanes.push_back(ForestMap::pathDen());
    map.setPointPaths(lanes);
}

void Game::subscribeEvents() {
    eventManager.subscribe([this](GameEvent& e) {
        switch (e.getType()) {
        case EventType::EnemyDead: {
            auto& ev = static_cast<EnemyDeadEvent&>(e);
            player.addGold(ev.getReward());
            player.addScore(ev.getReward());
            statistics.addKill();
            statistics.addGold(ev.getReward());
            addLog("击破 " + ev.getEnemyName() + "  +" + std::to_string(ev.getReward()) + " 金币");
            break;
        }
        case EventType::CoreDamaged: {
            auto& ev = static_cast<CoreDamagedEvent&>(e);
            statistics.addLeak();
            addLog("核心受损 -" + std::to_string(ev.getDamage()));
            break;
        }
        case EventType::WaveClear: {
            auto& ev = static_cast<WaveClearEvent&>(e);
            player.addGold(ev.getBonus());
            player.addScore(ev.getBonus());
            statistics.addWaveCleared();
            addLog("第 " + std::to_string(ev.getWave()) + " 波清空  +" + std::to_string(ev.getBonus()) + " 金币");
            break;
        }
        case EventType::ReactionTriggered: {
            auto& ev = static_cast<ReactionTriggeredEvent&>(e);
            statistics.addReaction();
            addLog("元素反应： " + ev.getReactionName());
            break;
        }
        }
    });
}

void Game::addLog(const std::string& msg) {
    log.push_back(msg);
    if (log.size() > 6) log.erase(log.begin());
}

void Game::applyContextToDefenders() {
    for (Pokemon* p : map.getDefenders()) {
        if (p) p->setContext(&context);
    }
}

void Game::reset() {
    map.clear();
    map.getCore().reset();
    player.reset();
    waveManager.reset();
    statistics.reset();
    log.clear();
    evolutionCount = 0;
    lastEvolvePos = Position();
    running = false;
    paused = false;
    gameOver = false;
    victory = false;
}

void Game::start() {
    if(running && !isOver()) return;
    if(isOver()) reset();
    running=true;paused=false;gameOver=false;victory=false;
    if(waveManager.getCurrentWave()==0) waveManager.startWave();
    addLog("战斗开始！第 "+std::to_string(waveManager.getCurrentWave())+" 波来袭");
}

void Game::restart() {
    reset();
    start();
}

void Game::pause()  { paused = true; }
void Game::resume() { paused = false; }

void Game::update(double dt) {
    if (!running || gameOver || victory) return;
    if (paused) return;

    // 核心受击闪烁计时衰减
    int flash = map.getCore().getHitFlashTimer();
    if (flash > 0) map.getCore().setHitFlashTimer(flash - 1);

    applyContextToDefenders();

    // 1) 波次生成
    waveManager.update(dt, [this](Enemy* e) {
        map.addEnemy(e);
    });

    // 2) 战场推进（攻击 / 移动 / 死亡结算）
    map.update(dt, context);

    // 3) 波次结束判定
    if (waveManager.getCurrentWave() > 0
        && !waveManager.isWaveActive()
        && waveManager.isSpawningDone()
        && map.getEnemyCount() == 0) {
        int wave = waveManager.getCurrentWave();
        int bonus = 25 + wave * 8;
        WaveClearEvent ev(wave, bonus);
        eventManager.handle(ev);

        if (waveManager.isFinalWave()) {
            victory = true;
            running = false;
        } else {
            waveManager.startWave();
        }
    }

    // 4) 失败判定
    if (map.getCore().isDestroyed()) {
        gameOver = true;
        running = false;
    }
}

Pokemon* Game::deployPokemon(const std::string& typeName, const Position& pos) {
    if (isOver()) return nullptr;
    if (!pos.isValid()) { addLog("坐标超出地图范围，无法部署"); return nullptr; }
    if (!map.isTallGrass(pos)) { addLog("宝可梦只能部署在标记点位上"); return nullptr; }
    if (isPositionOccupied(pos)) { addLog("该位置已被占用"); return nullptr; }

    int cost = deployCost(typeName);
    if (!player.spendGold(cost)) {
        addLog("金币不足，无法部署 " + typeName);
        return nullptr;
    }

    Pokemon* p = createPokemonById(typeName);
    if (!p) {
        player.addGold(cost);
        return nullptr;
    }

    p->setPosition(pos);
    p->setStrategy(createStrategyByName(defaultStrategyFor(typeName), map.getCorePosition()));
    p->setContext(&context);
    map.addPokemon(p);
    addLog("部署 " + typeName + "  -" + std::to_string(cost) + " 金币");
    return p;
}

bool Game::upgradePokemon(Pokemon*& p) {
    if (!p) return false;
    if (p->getLevel() >= 5) { addLog("已达最高等级"); return false; }
    int cost = p->getUpgradeCost();
    if (!player.spendGold(cost)) { addLog("金币不足，无法升级"); return false; }
    p->levelUp();
    addLog(pokemonDisplayName(p->getClassName()) + " 升级至 Lv." + std::to_string(p->getLevel()));
    tryEvolve(p);                       // 升级到 3 级 / 5 级自动进化
    return true;
}

// 进化：旧形态对象被新形态对象**整体替换**（运行时多态的最直观体现）。
// 注意：替换后原指针失效，因此本函数通过引用同步调用方持有的指针，
//       并且策略对象的所有权要先转移出去，避免双重释放。
void Game::tryEvolve(Pokemon*& p) {
    if (!p || !p->canEvolve()) return;

    const std::string oldName = pokemonDisplayName(p->getClassName());
    const std::string newName = p->evolutionName();
    const Position pos = p->getPosition();
    TargetStrategy* strat = p->takeStrategy();

    Pokemon* evo = p->createEvolved();
    if (!evo) {
        p->setStrategy(strat);
        return;
    }
    evo->setStrategy(strat);            // 保留玩家选择的索敌策略
    evo->setContext(&context);
    evo->setPosition(pos);

    map.removePokemon(p);               // GameMap 负责释放旧对象
    map.addPokemon(evo);
    p = evo;

    ++evolutionCount;
    lastEvolvePos = pos;
    addLog("★ 进化！" + oldName + " → " + newName);
}

bool Game::sellPokemon(Pokemon* p) {
    if (!p) return false;
    int refund = p->getSellValue();
    player.addGold(refund);
    addLog("出售 " + p->getName() + "  +" + std::to_string(refund) + " 金币");
    map.removePokemon(p);
    return true;
}

bool Game::isPositionOccupied(const Position& pos) const {
    for (Pokemon* p : map.getDefenders()) {
        if (p && p->getPosition() == pos) return true;
    }
    return false;
}

bool Game::setPokemonStrategy(Pokemon* p, const std::string& name) {
    const auto& units=map.getDefenders();
    if(!p || std::find(units.begin(),units.end(),p)==units.end()) return false;
    if(name!="Nearest" && name!="LowestHP" && name!="ClosestToCore" && name!="Fastest") return false;
    if(p->getStrategy() && p->getStrategy()->getName()==name) return true;
    p->setStrategy(createStrategyByName(name,map.getCorePosition()));
    const std::string label=name=="LowestHP" ? "最低生命" : name=="ClosestToCore" ? "最接近核心" : name=="Fastest" ? "最快敌人" : "最近目标";
    addLog(pokemonDisplayName(p->getClassName())+"：策略改为"+label);
    return true;
}

bool Game::isDeployable(const Position& pos) const {
    return map.isTallGrass(pos) && !isPositionOccupied(pos);
}

bool Game::save(const std::string& filename) {
    bool ok = SaveManager::save(*this, filename);
    addLog(ok ? "进度已保存" : "保存失败，请检查存档目录是否可写");
    return ok;
}

bool Game::load(const std::string& filename) {
    bool ok = SaveManager::load(*this, filename);
    addLog(ok ? "进度已恢复" : "读取失败：存档不存在或已损坏，当前进度已保留");
    return ok;
}

int Game::deployCost(const std::string& typeName) {
    if (typeName == "Pichu")   return 100;
    if (typeName == "Charmander") return 150;
    if (typeName == "Squirtle")  return 120;
    if (typeName == "Bulbasaur") return 130;
    return 100;
}

std::string Game::defaultStrategyFor(const std::string& typeName) {
    if (typeName == "Squirtle")  return "ClosestToCore";
    if (typeName == "Bulbasaur") return "LowestHP";
    return "Nearest";
}
