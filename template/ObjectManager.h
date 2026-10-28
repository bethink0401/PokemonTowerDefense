#pragma once
// =============================================================
//  ObjectManager.h
//  模板类：统一管理某一类基类指针的容器（实验五 模板加强项）
//  解决"不同派生类对象共用同一套增删查"的重复问题
// =============================================================
#include <vector>
#include <functional>
#include <cstddef>

template <typename T>
class ObjectManager {
private:
    std::vector<T*> objects;

public:
    ObjectManager() {}
    ~ObjectManager() { clear(); }

    void add(T* obj) {
        if (obj) objects.push_back(obj);
    }

    bool remove(T* obj) {
        for (auto it = objects.begin(); it != objects.end(); ++it) {
            if (*it == obj) {
                objects.erase(it);
                return true;
            }
        }
        return false;
    }

    std::size_t size() const { return objects.size(); }
    bool empty() const { return objects.empty(); }

    std::vector<T*>& getAll() { return objects; }
    const std::vector<T*>& getAll() const { return objects; }

    T* at(std::size_t index) const {
        return index < objects.size() ? objects[index] : nullptr;
    }

    void forEach(std::function<void(T*)> fn) {
        for (T* o : objects) if (o) fn(o);
    }

    void clear() { objects.clear(); }   // 不负责 delete（所有权在外部）
};
