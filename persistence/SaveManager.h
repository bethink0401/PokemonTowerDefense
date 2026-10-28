#pragma once
// =============================================================
//  SaveManager.h
//  实验六：文件流保存 / 恢复对象状态
//  必须真正保存具体派生类型，并在读取时重新构造派生对象
// =============================================================
#include <string>

class Game;

class SaveManager {
public:
    // V5：单位累计伤害、完整战场、波次出怪、攻击计时、异常内部计时与随机状态、附着、统计；临时文件写完后原子替换。
    static bool save(Game& game, const std::string& filename);

    // 校验并重建全部对象后提交；兼容 V1–V4（V1 本波重新出怪并暂停）。
    static bool load(Game& game, const std::string& filename);
};
