#include "EventManager.h"

void EventManager::subscribe(std::function<void(GameEvent&)> fn) {
    if (fn) listeners.push_back(fn);
}

void EventManager::handle(GameEvent& event) {
    for (auto& fn : listeners) {
        if (fn) fn(event);
    }
}

void EventManager::clear() {
    listeners.clear();
}
