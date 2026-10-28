#pragma once
// =============================================================
//  Position.h
//  坐标类 —— 实验一：类与对象 / 封装 / 构造 / 合法性验证
//  实验二：作为 Pokemon、Enemy 的组合成员（has-a）
//  实验四：operator== 运算符重载
// =============================================================
#include <string>

class Position {
private:
    int x;
    int y;

public:
    static const int MAP_WIDTH  = 46;   // 地图逻辑宽（格）——森林路线图
    static const int MAP_HEIGHT = 23;   // 地图逻辑高（格）

    // 保留输入坐标，不自动截断到地图边缘；调用 isValid() 判断是否为地图内格子。
    // 这样越界输入不会被悄悄转换成可部署的位置，也允许表示地图外的位置。
    Position();
    Position(int x, int y);
    Position(const Position& other);

    int getX() const;
    int getY() const;
    void setX(int x);
    void setY(int y);

    // 合法地图格：0 <= x < MAP_WIDTH，0 <= y < MAP_HEIGHT。
    bool isValid() const;

    double distanceTo(const Position& other) const;

    bool operator==(const Position& other) const;
    bool operator!=(const Position& other) const;

    std::string toString() const;
};

// 浮点路点：网格单位坐标（格 (c,r) 的中心 = (c+0.5, r+0.5)），
// 由地图图片的道路中心线提取，怪物沿它行走才能贴路中心
struct FWaypoint {
    double x, y;
};
