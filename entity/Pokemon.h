#pragma once
// =============================================================
//  Pokemon.h
//  防御方宝可梦基类
//  实验一：类与对象 / 封装 / 构造析构
//  实验二：组合（Position、TargetStrategy）+ 依赖（Enemy）
//  实验三：作为 Pichu / Charmander / Squirtle / Bulbasaur 的基类
//  实验四：virtual attack()/update() 运行时多态
// =============================================================
#include <string>
#include <vector>
#include "Position.h"
#include "TypeSystem.h"

class Enemy;
class TargetStrategy;
struct BattleContext;

class Pokemon {
    friend class SaveManager;
protected:
    std::string name;
    int hp;
    int maxHp;
    int attackPower;
    double range;           // 攻击范围（格）
    double attackInterval;  // 攻击间隔（秒）
    double attackTimer;
    int level;
    int cost;
    int stage;              // 进化阶段：0 初形态 / 1 一阶进化 / 2 二阶进化（最终形态）

    Position position;
    PokemonType type;

    TargetStrategy* strategy;   // 组合：目标选择策略
    BattleContext* context;     // 依赖：战斗上下文（可空）

    // UI 表现用（不影响逻辑）
    double attackAnimTimer;     // 攻击动画计时
    Position lastTargetPos;
    bool didAttackThisFrame;
    long long cumulativeDamage = 0;

    void setStage(int s) { stage = s; }     // 仅供派生类（进化形态）在构造时设定阶段

public:
    Pokemon(const std::string& name, int maxHp, int attackPower, double range,
            double attackInterval, int cost, PokemonType type);
    virtual ~Pokemon();

    // ---- 文档要求的虚函数：运行时多态 ----
    virtual void attack(Enemy& target);
    virtual void update(double dt);

    void takeDamage(int damage);
    void heal(int amount);
    void levelUp();

    // ---- 进化（进化链用多层继承表达：Pichu -> Pikachu -> Raichu） ----
    int getStage() const { return stage; }
    int nextEvolveLevel() const { return stage == 0 ? 3 : 5; }   // 触发进化的等级
    bool canEvolve() const;                 // 是否已到达进化等级
    Pokemon* createEvolved() const;         // 生成进化形态对象（工厂 + 运行时多态）
    std::string evolutionName() const;      // 进化目标的显示名（无则空串）
    TargetStrategy* takeStrategy();         // 转移策略所有权（防止对象替换时双重释放）
    void setHp(int v);      // 存档恢复用
    void setLevel(int v);   // 存档恢复用

    // ---- 组合：策略 ----
    void setStrategy(TargetStrategy* s);
    TargetStrategy* getStrategy() const { return strategy; }
    Enemy* selectTarget(const std::vector<Enemy*>& enemies) const;

    void setContext(BattleContext* ctx) { context = ctx; }
    BattleContext* getContext() const { return context; }

    bool canAttack() const { return attackTimer <= 0.0; }
    bool inRange(const Position& target) const;

    // ---- getters ----
    const std::string& getName() const { return name; }
    int getHp() const { return hp; }
    int getMaxHp() const { return maxHp; }
    int getAttackPower() const { return attackPower; }
    double getRange() const { return range; }
    double getAttackInterval() const { return attackInterval; }
    double getAttackCooldown() const { return attackTimer > 0 ? attackTimer : 0; }
    int getLevel() const { return level; }
    double getArmorPenetration() const { return type==PokemonType::Grass ? .4+.15*stage : 0.0; }
    double getSplashRadius() const { return type==PokemonType::Fire ? 1.4+.2*stage : 0.0; }
    double getSplashRatio() const { return type==PokemonType::Fire ? .35+.05*stage : 0.0; }
    double getSlowRatio() const { return type==PokemonType::Water ? .30+.05*stage+.01*(level-1) : 0.0; }
    double getSlowDuration() const { return 1.6+.15*level+.2*stage; }
    int getCost() const { return cost; }
    int getUpgradeCost() const { return level * 60 + 40; }
    int getSellValue() const { return static_cast<int>(cost * 0.6) + (level - 1) * 30; }
    Position getPosition() const { return position; }
    void setPosition(const Position& p) { position = p; }
    PokemonType getType() const { return type; }
    bool isDead() const { return hp <= 0; }

    double getAttackAnimTimer() const { return attackAnimTimer; }
    Position getLastTargetPos() const { return lastTargetPos; }
    bool didAttack() const { return didAttackThisFrame; }
    void clearAttackFlag() { didAttackThisFrame = false; }
    long long getCumulativeDamage() const { return cumulativeDamage; }
    void recordDamage(long long damage) { if(damage>0)cumulativeDamage+=damage; }

    virtual std::string getClassName() const { return "Pokemon"; }
    // 进化链钩子：返回下一形态的类型 id；最终形态返回 nullptr
    virtual const char* nextStageId() const { return nullptr; }
    virtual std::string describe() const;
};

// 工厂：按类型名创建具体宝可梦（部署 / 存档读取时重建派生对象）
Pokemon* createPokemonById(const std::string& id);
// 可部署的宝可梦类型清单（供 UI 生成按钮）
const std::vector<std::string>& allPokemonIds();
// 宝可梦中文名（日志 / UI 显示用）
std::string pokemonDisplayName(const std::string& id);
