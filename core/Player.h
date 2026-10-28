#pragma once
// =============================================================
//  Player.h
//  玩家资源：金币与分数
//  Pokemon / Enemy 不直接修改 Player，金币变动统一由 Game 调度
// =============================================================

class Player {
private:
    int gold;
    int score;

public:
    Player();
    Player(int gold, int score);

    int getGold() const { return gold; }
    int getScore() const { return score; }

    void addGold(int amount);
    bool spendGold(int amount);      // 余额不足返回 false
    bool canAfford(int amount) const;

    void addScore(int amount);
    void reset();

    void setGold(int g) { gold = g > 0 ? g : 0; }
    void setScore(int s) { score = s > 0 ? s : 0; }
};
