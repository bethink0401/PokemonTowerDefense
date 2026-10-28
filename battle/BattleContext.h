#pragma once
// =============================================================
//  BattleContext.h
//  战斗上下文：把"当前战场"以依赖方式临时传给普通攻击 / 反应
//  避免把全局状态写死进对象内部（实验二 依赖关系 的体现）
// =============================================================
#include <vector>

class Enemy;
class ReactionEngine;

struct BattleContext {
    std::vector<Enemy*>* enemies = nullptr;        // 当前战场上的敌人（用于 AOE）
    ReactionEngine* reactionEngine = nullptr;      // 属性附着后触发元素反应

    bool valid() const { return enemies != nullptr; }
};
