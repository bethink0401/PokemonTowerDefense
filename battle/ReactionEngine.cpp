#include "ReactionEngine.h"
#include "Enemy.h"
#include "ElementStatus.h"
#include <cmath>

ReactionEngine::ReactionEngine() : enemies(nullptr) {
    // 四种元素两两组合，共六种反应。
    reactions.push_back(new ConductReaction());
    reactions.push_back(new BurnReaction());
    reactions.push_back(new GrowthReaction());
    reactions.push_back(new SteamReaction());
    reactions.push_back(new SporeReaction());
    reactions.push_back(new ExplosionReaction());
}

ReactionEngine::~ReactionEngine() {
    for (Reaction* r : reactions) delete r;
    reactions.clear();
}

void ReactionEngine::applyElement(Enemy& enemy, PokemonType newElement,
                                  int strength, double duration) {
    if (enemy.isDead()) return;

    ElementStatus* existing = enemy.getPrimaryElement();

    // 1) 没有任何附着：直接挂上
    if (!existing) {
        enemy.setElementStatus(createElementStatus(newElement, duration, strength));
        return;
    }

    // 2) 同属性：刷新持续时间
    if (existing->getType() == newElement) {
        existing->setRemainingTime(duration);
        return;
    }

    // 3) 与已有附着构成元素反应：触发反应，并消耗双方（不残留第二个元素）
    for (Reaction* r : reactions) {
        if (r->canTrigger(existing->getType(), newElement)) {
            Position pos = enemy.getPosition();
            r->execute(enemy);

            if (r->isArea() && enemies) {
                for (Enemy* other : *enemies) {
                    if (!other || other == &enemy || other->isDead()) continue;
                    if (other->getPosition().distanceTo(pos) <= r->getAreaRadius()) {
                        other->takeDamage(r->getAreaDamage());
                    }
                }
            }

            enemy.clearElementStatuses();   // 反应后不再保留任何附着
            if (onReaction) onReaction(r->getName(), pos);
            return;
        }
    }

    // 4) 无匹配元素反应：新元素覆盖旧附着（始终只有一个附着）
    enemy.setElementStatus(createElementStatus(newElement, duration, strength));
}

void ReactionEngine::checkReaction(Enemy& enemy, PokemonType newElement) {
    applyElement(enemy, newElement, 1, 8.0);
}
