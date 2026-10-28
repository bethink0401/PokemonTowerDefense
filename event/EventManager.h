#pragma once
// =============================================================
//  EventManager.h
//  简化事件分发：订阅者注册回调，事件发生时统一分发
//  Enemy 只负责自身死亡状态，金币 / 统计 / UI 各管各的职责
// =============================================================
#include <functional>
#include <vector>
#include <cstddef>
#include "GameEvent.h"

class EventManager {
private:
    std::vector<std::function<void(GameEvent&)>> listeners;

public:
    void subscribe(std::function<void(GameEvent&)> fn);
    void handle(GameEvent& event);   // 文档要求接口
    void clear();

    size_t listenerCount() const { return listeners.size(); }
};
