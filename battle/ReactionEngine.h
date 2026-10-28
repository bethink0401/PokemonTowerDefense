#pragma once
// =============================================================
//  ReactionEngine.h
//  职责：读取已有附着 -> 判断新属性 -> 找到匹配 Reaction -> 调用 execute()
//  反应"如何选"由本引擎负责，"如何执行"由 Reaction 子类负责（职责分离）
// =============================================================
#include <vector>
#include <functional>
#include <string>
#include "Reaction.h"
#include "TypeSystem.h"
#include "Position.h"

class Enemy;

class ReactionEngine {
private:
    std::vector<Reaction*> reactions;
    std::vector<Enemy*>* enemies;                                   // 用于范围反应
    std::function<void(const std::string&, const Position&)> onReaction;  // 通知 UI / 统计

public:
    ReactionEngine();
    ~ReactionEngine();

    void setEnemies(std::vector<Enemy*>* e) { enemies = e; }
    void setReactionCallback(std::function<void(const std::string&, const Position&)> cb) {
        onReaction = cb;
    }

    // 核心接口：给敌人附加新属性，并按"单一附着槽"规则处理连携
    //   已有附着 == 新属性  -> 刷新时长
    //   已有附着 + 新属性 构成连携 -> 触发反应并消耗双方（清空附着）
    //   无匹配连携        -> 新元素覆盖旧附着（任何时刻只有一个附着）
    void applyElement(Enemy& enemy, PokemonType newElement, int strength, double duration);

    // 文档要求接口（保留）：内部转调 applyElement
    void checkReaction(Enemy& enemy, PokemonType newElement);

    const std::vector<Reaction*>& getReactions() const { return reactions; }
};
