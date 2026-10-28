#pragma once
// =============================================================
//  Enemy.h
//  进攻方敌人基类
//  实验二：Enemy 组合 ElementStatus / StatusEffect
//  实验三：作为 BasicEnemy / FastEnemy / TankEnemy 的基类
//  实验四：virtual move()/update() 多态
// =============================================================
#include <string>
#include <vector>
#include "Position.h"
#include "TypeSystem.h"

class ElementStatus;
class StatusEffect;

class Enemy {
    friend class SaveManager;
protected:
    std::string name;
    int hp;
    int maxHp;
    double baseSpeed;
    int reward;
    int damageToCore;
    double armor;            // 护甲减伤比例（0~0.8），坦克型敌人使用

    void advance(double dt);

    double posX;             // 精确坐标（用于平滑移动）
    double posY;
    PokemonType type;

    std::vector<FWaypoint> path;  // 道路中心线路径（浮点，贴路中心行走）
    int pathIndex;

    std::vector<ElementStatus*> elementStates;
    std::vector<StatusEffect*> statusEffects;

    bool dead;
    bool reachedCore;

    int hpFlashTimer;        // UI 受击闪烁

public:
    Enemy(const std::string& name, int maxHp, double speed, int reward,
          int damageToCore, double armor, PokemonType type);
    virtual ~Enemy();

    virtual void move(double dt);
    virtual void update(double dt);

    void takeDamage(int damage, double armorPenetration=0.0);
    bool isDead() const;
    bool hasReachedCore() const;

    void addElementStatus(ElementStatus* status);
    void addStatusEffect(StatusEffect* effect);
    void removeElementStatus(PokemonType type);   // 连携反应消耗掉旧附着

    // ---- 单一附着槽：同一敌人任何时刻最多只保留一种属性附着 ----
    ElementStatus* getPrimaryElement() const;              // 当前附着（无则 nullptr）
    void setElementStatus(ElementStatus* status);          // 覆盖式设置（先清空再挂）
    void clearElementStatuses();                           // 连携反应后清空

    double currentSpeed() const;
    bool isImmobilized() const;

    void setPath(const std::vector<FWaypoint>& p);

    // 波次增强：血量与奖励随波次提升
    void applyWaveScaling(int wave);

    // ---- getters ----
    const std::string& getName() const { return name; }
    int getHp() const { return hp; }
    int getMaxHp() const { return maxHp; }
    double getHpRatio() const;
    int getReward() const { return reward; }
    int getDamageToCore() const { return damageToCore; }
    double getBaseSpeed() const { return baseSpeed; }
    Position getPosition() const;
    double getX() const { return posX; }
    double getY() const { return posY; }
    PokemonType getType() const { return type; }
    int getPathIndex() const { return pathIndex; }
    int getHitFlashTimer() const { return hpFlashTimer; }
    void setHitFlashTimer(int t) { hpFlashTimer = t; }

    const std::vector<ElementStatus*>& getElementStates() const { return elementStates; }
    const std::vector<StatusEffect*>& getStatusEffects() const { return statusEffects; }

    std::string getStatusLabel() const;

    virtual std::string getClassName() const { return "Enemy"; }
};

// ---------- 派生敌人 ----------
class BasicEnemy : public Enemy {
public:
    BasicEnemy();
    std::string getClassName() const override { return "BasicEnemy"; }
};

class FastEnemy : public Enemy {
public:
    FastEnemy();
    void move(double dt) override;      // 高速单位：移动逻辑可特化
    std::string getClassName() const override { return "FastEnemy"; }
};

class TankEnemy : public Enemy {
public:
    TankEnemy();
    void update(double dt) override;
    std::string getClassName() const override { return "TankEnemy"; }
};

// 工厂：按类型名创建敌人（波次生成 / 存档读取）
Enemy* createEnemyById(const std::string& id);
