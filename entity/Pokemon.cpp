#include "Pokemon.h"
#include "Enemy.h"
#include "ElementStatus.h"
#include "ReactionEngine.h"
#include "FamilyBalance.h"
#include <cmath>
#include "TargetStrategy.h"
#include "BattleContext.h"
#include "Pichu.h"
#include "Charmander.h"
#include "Squirtle.h"
#include "Bulbasaur.h"
#include "Pikachu.h"
#include "Raichu.h"
#include "Charmeleon.h"
#include "Charizard.h"
#include "Wartortle.h"
#include "Blastoise.h"
#include "Ivysaur.h"
#include "Venusaur.h"
#include <sstream>
#include <algorithm>
#include <map>

Pokemon::Pokemon(const std::string& name, int maxHp, int attackPower, double range,
                 double attackInterval, int cost, PokemonType type)
    : name(name),
      hp(maxHp > 0 ? maxHp : 1),
      maxHp(maxHp > 0 ? maxHp : 1),
      attackPower(attackPower > 0 ? attackPower : 1),
      range(range > 0 ? range * 1.25 : 1.25),
      attackInterval(attackInterval > 0 ? attackInterval : 1.0),
      attackTimer(0.0),
      level(1),
      cost(cost > 0 ? cost : 0),
      stage(0),
      position(),
      type(type),
      strategy(nullptr),
      context(nullptr),
      attackAnimTimer(0.0),
      lastTargetPos(),
      didAttackThisFrame(false) {}

Pokemon::~Pokemon() {
    // 组合关系：由 Pokemon 负责释放自己持有的策略
    delete strategy;
    strategy = nullptr;
}

// 普通攻击只有一套计时，升级与进化的攻速成长直接生效。
void Pokemon::attack(Enemy& target) {
    didAttackThisFrame = false;
    if (isDead() || target.isDead() || !canAttack()) return;

    const int damage = std::max(1, static_cast<int>(std::round(
        attackPower * TypeSystem::getMultiplier(type, target.getType()))));
    const double duration = type == PokemonType::Grass ? 4.0 : 8.0;
    // 先处理附着与反应，使致命普攻也能正常触发连携。
    if (context && context->reactionEngine) {
        context->reactionEngine->applyElement(target, type, 0, duration);
    } else {
        target.setElementStatus(createElementStatus(type, duration, 0));
    }
    target.takeDamage(damage, getArmorPenetration());
    attackTimer = attackInterval;
    attackAnimTimer = 0.25;
    lastTargetPos = target.getPosition();
    didAttackThisFrame = true;
}

void Pokemon::update(double dt) {
    attackTimer = std::max(0.0, attackTimer - dt);
    attackAnimTimer = std::max(0.0, attackAnimTimer - dt);
}

void Pokemon::takeDamage(int damage) {
    if (damage <= 0) return;
    hp -= damage;
    if (hp < 0) hp = 0;
}

void Pokemon::heal(int amount) {
    if (amount <= 0) return;
    hp += amount;
    if (hp > maxHp) hp = maxHp;
}

void Pokemon::levelUp() {
    if (level >= 5) return;   // 最高 5 级
    ++level;
    const auto growth=familyGrowth(type);
    maxHp += growth.hp; hp += growth.hp;
    attackPower += growth.attack; range += growth.range;
    attackInterval *= growth.intervalFactor;
}

void Pokemon::setHp(int v) {
    hp = v;
    if (hp < 1) hp = 1;
    if (hp > maxHp) hp = maxHp;
}

void Pokemon::setLevel(int v) {
    while (level < v && level < 5) levelUp();
}

void Pokemon::setStrategy(TargetStrategy* s) {
    if (strategy == s) return;
    delete strategy;          // 组合：策略的生命周期由 Pokemon 管理
    strategy = s;
}

Enemy* Pokemon::selectTarget(const std::vector<Enemy*>& enemies) const {
    if (!strategy) return nullptr;
    return strategy->selectTarget(enemies, *this);
}

bool Pokemon::inRange(const Position& target) const {
    return position.distanceTo(target) <= range;
}

std::string Pokemon::describe() const {
    std::ostringstream oss;
    oss << name << " Lv." << level
        << "  HP " << hp << "/" << maxHp
        << "  攻击 " << attackPower
        << "  范围 " << range
        << "  属性 " << typeToString(type);
    return oss.str();
}

// ==================== 进化 ====================
std::string Pokemon::evolutionName() const {
    const char* id = nextStageId();
    return id ? pokemonDisplayName(id) : std::string();
}

bool Pokemon::canEvolve() const {
    return nextStageId() != nullptr && level >= nextEvolveLevel();
}

Pokemon* Pokemon::createEvolved() const {
    const char* id = nextStageId();
    if (!id) return nullptr;
    Pokemon* evo = createPokemonById(id);   // 多态工厂：按 id 重建具体派生对象
    if (!evo) return nullptr;
    evo->setLevel(level);                   // 等级迁移（同时继承等级成长）
    evo->cumulativeDamage=cumulativeDamage;
    evo->setHp(evo->getMaxHp());            // 进化后回满血
    return evo;
}

TargetStrategy* Pokemon::takeStrategy() {
    TargetStrategy* s = strategy;
    strategy = nullptr;                     // 所有权移交调用方，避免对象替换时双重释放
    return s;
}

std::string pokemonDisplayName(const std::string& id) {
    static const std::map<std::string, std::string> kNames = {
        {"Pichu", "皮丘"},       {"Pikachu", "皮卡丘"},   {"Raichu", "雷丘"},
        {"Charmander", "小火龙"}, {"Charmeleon", "火恐龙"}, {"Charizard", "喷火龙"},
        {"Squirtle", "杰尼龟"},   {"Wartortle", "卡咪龟"},  {"Blastoise", "水箭龟"},
        {"Bulbasaur", "妙蛙种子"}, {"Ivysaur", "妙蛙草"},    {"Venusaur", "妙蛙花"},
    };
    auto it = kNames.find(id);
    return it != kNames.end() ? it->second : id;
}

// ==================== 工厂 ====================
Pokemon* createPokemonById(const std::string& id) {
    // 部署用（最初形态）
    if (id == "Pichu")      return new Pichu();
    if (id == "Charmander") return new Charmander();
    if (id == "Squirtle")   return new Squirtle();
    if (id == "Bulbasaur")  return new Bulbasaur();
    // 进化形态（升级触发进化 / 存档读取时按 id 重建）
    if (id == "Pikachu")    return new Pikachu();
    if (id == "Raichu")     return new Raichu();
    if (id == "Charmeleon") return new Charmeleon();
    if (id == "Charizard")  return new Charizard();
    if (id == "Wartortle")  return new Wartortle();
    if (id == "Blastoise")  return new Blastoise();
    if (id == "Ivysaur")    return new Ivysaur();
    if (id == "Venusaur")   return new Venusaur();
    return nullptr;
}

const std::vector<std::string>& allPokemonIds() {
    static const std::vector<std::string> ids = {
        "Pichu", "Charmander", "Squirtle", "Bulbasaur"
    };
    return ids;
}
