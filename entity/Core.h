#pragma once
// =============================================================
//  Core.h
//  能量结晶（防守目标）
//  实验二：作为 GameMap 的组合成员（GameMap has-a Core）
// =============================================================

class Core {
private:
    int hp;
    int maxHp;
    int hitFlashTimer;   // 受击闪烁计时（用于 UI 表现）

public:
    Core();
    explicit Core(int hp);

    void takeDamage(int damage);
    void heal(int amount);
    bool isDestroyed() const;

    int getHp() const;
    int getMaxHp() const;
    double getHpRatio() const;
    int getHitFlashTimer() const;
    void setHitFlashTimer(int t);
    void setHp(int v);

    void reset();
};
