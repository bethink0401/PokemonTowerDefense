#include "Enemy.h"
#include "ElementStatus.h"
#include "StatusEffect.h"
#include <cmath>
#include <cstddef>
#include <sstream>
#include <algorithm>

Enemy::Enemy(const std::string& name, int maxHp, double speed, int reward,
             int damageToCore, double armor, PokemonType type)
    : name(name),
      hp(maxHp > 0 ? maxHp : 1),
      maxHp(maxHp > 0 ? maxHp : 1),
      baseSpeed(speed > 0 ? speed : 0.5),
      reward(reward > 0 ? reward : 0),
      damageToCore(damageToCore > 0 ? damageToCore : 1),
      armor(armor < 0 ? 0 : (armor > 0.8 ? 0.8 : armor)),
      posX(0.0),
      posY(0.0),
      type(type),
      pathIndex(0),
      dead(false),
      reachedCore(false),
      hpFlashTimer(0) {}

Enemy::~Enemy() {
    for (ElementStatus* s : elementStates) delete s;
    elementStates.clear();
    for (StatusEffect* e : statusEffects) delete e;
    statusEffects.clear();
}

void Enemy::setPath(const std::vector<FWaypoint>& p) {
    path = p;
    pathIndex = 0;
    if (!path.empty()) {
        posX = path.front().x;
        posY = path.front().y;
    }
    reachedCore = false;
}

void Enemy::move(double dt) {
    if (dead || reachedCore || isImmobilized() || path.empty()) return;

    advance(dt);
}

void Enemy::advance(double dt) {
    if(dead || reachedCore || path.empty())return;
    double remaining = currentSpeed() * dt;
    while (remaining > 0.0 && pathIndex < static_cast<int>(path.size())) {
        const FWaypoint& wp = path[pathIndex];
        double dx = wp.x - posX;
        double dy = wp.y - posY;
        double dist = std::sqrt(dx * dx + dy * dy);

        if (dist <= remaining || dist < 1e-6) {
            posX = wp.x;
            posY = wp.y;
            remaining -= dist;
            ++pathIndex;
        } else {
            posX += dx / dist * remaining;
            posY += dy / dist * remaining;
            remaining = 0.0;
        }
    }

    if (pathIndex >= static_cast<int>(path.size())) {
        reachedCore = true;
    }
}

void Enemy::update(double dt) {
    if (dead) return;

    // 1) 附着仅更新持续时间，不产生伤害或异常状态
    for (size_t i = 0; i < elementStates.size();) {
        elementStates[i]->update(dt);
        if (elementStates[i]->isExpired()) {
            delete elementStates[i];
            elementStates.erase(elementStates.begin() + static_cast<long>(i));
        } else {
            ++i;
        }
    }

    double blockedTime=0;
    // 2) 异常状态结算
    for (size_t i = 0; i < statusEffects.size();) {
        statusEffects[i]->update(*this, dt);
        blockedTime=std::max(blockedTime,statusEffects[i]->blockedDuration());
        if (statusEffects[i]->isExpired()) {
            delete statusEffects[i];
            statusEffects.erase(statusEffects.begin() + static_cast<long>(i));
        } else {
            ++i;
        }
    }

    // 3) 移动
    advance(std::max(0.0,dt-blockedTime));

    if (hp <= 0) dead = true;
    if (hpFlashTimer > 0) --hpFlashTimer;
}

void Enemy::takeDamage(int damage,double armorPenetration) {
    if (damage <= 0 || dead) return;
    int real = static_cast<int>(damage * (1.0 - armor * (1.0-std::clamp(armorPenetration,0.0,1.0))));
    if (real < 1) real = 1;
    hp -= real;
    if (hp < 0) hp = 0;
    hpFlashTimer = 6;
}

bool Enemy::isDead() const { return dead || hp <= 0; }
bool Enemy::hasReachedCore() const { return reachedCore; }

void Enemy::addElementStatus(ElementStatus* status) {
    // 单一附着槽：同名属性刷新时长，不同属性直接覆盖旧附着
    if (!status) return;
    if (!elementStates.empty()) {
        if (elementStates.front()->getType() == status->getType()) {
            elementStates.front()->setRemainingTime(status->getRemainingTime());
            delete status;
            return;
        }
        clearElementStatuses();
    }
    elementStates.push_back(status);
}

ElementStatus* Enemy::getPrimaryElement() const {
    return elementStates.empty() ? nullptr : elementStates.front();
}

void Enemy::setElementStatus(ElementStatus* status) {
    clearElementStatuses();
    if (status) elementStates.push_back(status);
}

void Enemy::clearElementStatuses() {
    for (ElementStatus* s : elementStates) delete s;
    elementStates.clear();
}

void Enemy::addStatusEffect(StatusEffect* effect) {
    if (!effect) return;
    // Refresh a status of the same class; repeated hits must not accumulate
    // unlimited badges or multiply identical slows on every attack.
    for (StatusEffect*& active : statusEffects) {
        if (active->getClassName() != effect->getClassName()) continue;
        double duration = std::max(active->getRemainingTime(), effect->getRemainingTime());
        // 刷新时保留灼烧累积、麻痹随机状态及恢复窗口，不能反复重置控制。
        active->strengthen(effect->getMagnitude());
        active->setRemainingTime(duration);
        delete effect;
        return;
    }
    statusEffects.push_back(effect);
    effect->apply(*this);
}

void Enemy::removeElementStatus(PokemonType type) {
    for (size_t i = 0; i < elementStates.size();) {
        if (elementStates[i]->getType() == type) {
            delete elementStates[i];
            elementStates.erase(elementStates.begin() + static_cast<long>(i));
        } else {
            ++i;
        }
    }
}

bool Enemy::isImmobilized() const {
    for (StatusEffect* e : statusEffects) {
        if (e->isImmobilize()) return true;
    }
    return false;
}

double Enemy::currentSpeed() const {
    double mult = 1.0;
    for (StatusEffect* e : statusEffects) {
        if (e->isSlow()) {
            mult *= (1.0 - e->getMagnitude());
        }
    }
    if (mult < 0.1) mult = 0.1;
    return baseSpeed * mult;
}

double Enemy::getHpRatio() const {
    if (maxHp <= 0) return 0.0;
    return static_cast<double>(hp) / static_cast<double>(maxHp);
}

void Enemy::applyWaveScaling(int wave) {
    if (wave <= 1) return;
    double factor = 1.0 + 0.18 * (wave - 1);   // 每波 +18% 血量
    int newMax = static_cast<int>(maxHp * factor);
    hp = newMax;
    maxHp = newMax;
    reward += (wave - 1) * 3;
}

Position Enemy::getPosition() const {
    return Position(static_cast<int>(std::lround(posX)),
                    static_cast<int>(std::lround(posY)));
}

std::string Enemy::getStatusLabel() const {
    if (statusEffects.empty() && elementStates.empty()) return "-";
    std::ostringstream oss;
    for (ElementStatus* s : elementStates) oss << s->getClassName() << " ";
    for (StatusEffect* e : statusEffects) oss << e->getClassName() << " ";
    return oss.str();
}

// ==================== 派生敌人 ====================
BasicEnemy::BasicEnemy()
    : Enemy("Basic", 60, 1.5, 12, 8, 0.0, PokemonType::Grass) {}

FastEnemy::FastEnemy()
    : Enemy("Runner", 42, 2.9, 16, 6, 0.0, PokemonType::Electric) {}

void FastEnemy::move(double dt) {
    Enemy::move(dt);   // 复用基类移动逻辑
}

TankEnemy::TankEnemy()
    : Enemy("Tank", 170, 0.9, 26, 18, 0.35, PokemonType::Water) {}

void TankEnemy::update(double dt) {
    Enemy::update(dt);
}

// ==================== 工厂 ====================
Enemy* createEnemyById(const std::string& id) {
    if (id == "FastEnemy") return new FastEnemy();
    if (id == "TankEnemy") return new TankEnemy();
    return new BasicEnemy();
}
