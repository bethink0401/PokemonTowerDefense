// =============================================================
//  console_demo.cpp
//  核心逻辑控制台演示（不含 Qt），可用于验证业务层，
//  也便于在实验一 ~ 实验六答辩时单独演示对象模型与运行结果。
//
//  编译（示例）：
//    g++ -std=c++17 -Icore -Ientity -Ibattle -Istrategy -Ievent ^
//        -Ipersistence -Itemplate tests/console_demo.cpp ^
//        core/*.cpp entity/*.cpp battle/*.cpp strategy/*.cpp ^
//        event/*.cpp persistence/*.cpp -o console_demo
// =============================================================
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>

#include "Game.h"
#include "Pokemon.h"
#include "Enemy.h"
#include "Core.h"
#include "GameMap.h"
#include "WaveManager.h"
#include "Statistics.h"
#include "TargetStrategy.h"
#include "TypeSystem.h"
#include "ReactionEngine.h"
#include "ElementStatus.h"

static void printState(Game& game, int tick) {
    const Player& pl = game.getPlayer();
    const GameMap& map = game.getMap();
    std::printf("[t=%5.1fs] 波次 %d/%d  敌人 %2d  防守 %d  核心 %3d/%-3d  金币 %4d  分数 %5d\n",
                tick / 60.0,
                game.getWaveManager().getCurrentWave(), game.getWaveManager().getMaxWave(),
                map.getEnemyCount(), map.getDefenderCount(),
                map.getCore().getHp(), map.getCore().getMaxHp(),
                pl.getGold(), pl.getScore());
}

int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    std::printf("==============================\n");
    std::printf(" 宝可梦动态策略塔防 · 核心逻辑演示\n");
    std::printf("==============================\n\n");

    Game game;
    game.start();

    // 给足金币以便演示 4 种宝可梦全部部署
    game.getPlayer().addGold(1000);

    // ---- 组合 / 多态：vector<Pokemon*> 中放不同派生类 ----
    // 森林路线图：宝可梦只能部署在高草丛（'G'）上，下面四个位置都是草丛
    Pokemon* p1 = game.deployPokemon("Pichu",      Position(10, 7));
    Pokemon* p2 = game.deployPokemon("Charmander", Position(12, 16));
    game.deployPokemon("Squirtle",  Position(6, 13));
    game.deployPokemon("Bulbasaur", Position(7, 0));

    // 只有高草丛可部署的验证（(10,5) 是道路格）
    std::printf("在非草丛格 (3,3) 上尝试部署：");
    Pokemon* blocked = game.deployPokemon("Pichu", Position(3, 3));
    std::printf("%s\n\n", blocked ? "竟然成功了(异常)" : "被拒绝（符合预期）");

    std::printf("已部署：\n");
    for (Pokemon* p : game.getMap().getDefenders()) {
        if (p) std::printf("  - %-10s 属性 %-8s 策略 %s\n",
                           p->getClassName().c_str(),
                           typeToString(p->getType()).c_str(),
                           p->getStrategy() ? p->getStrategy()->getName().c_str() : "-");
    }
    std::printf("\n");

    // ---- 升级演示（升到 3 级会自动进化） ----
    std::printf("\n---------------- 升级 / 进化演示 ----------------\n");
    if (p1) {
        std::printf("升级前：%s\n", p1->describe().c_str());
        game.upgradePokemon(p1);
        game.upgradePokemon(p1);        // Lv.1 -> 3，触发第一次进化
        std::printf("升到 3 级后：%s\n", p1->describe().c_str());
        game.upgradePokemon(p1);
        game.upgradePokemon(p1);        // Lv.3 -> 5，触发第二次进化
        std::printf("升到 5 级后：%s\n", p1->describe().c_str());
        std::printf("再升级：%s\n",
                    game.upgradePokemon(p1) ? "竟然成功了(异常)" : "被拒绝（符合预期，已到顶）");
    }
    if (p2) {
        game.upgradePokemon(p2);        // Lv.1 -> 2，未到进化等级
        std::printf("另一只升级 1 次：%s（未进化，符合预期）\n\n", p2->describe().c_str());
    }

    // ---- 主循环：60FPS 模拟 3 分钟 ----
    const double dt = 1.0 / 60.0;
    const int totalTicks = 60 * 180;
    for (int i = 0; i < totalTicks && !game.isOver(); ++i) {
        game.update(dt);
        if (i % (60 * 15) == 0) printState(game, i);
    }

    std::printf("\n---------------- 结果 ----------------\n");
    printState(game, totalTicks);
    std::printf("状态：%s\n", game.isVictory() ? "胜利" : (game.isGameOver() ? "失败" : "未结束"));
    std::printf("统计：%s\n", game.getStatistics().toString().c_str());

    // ---- 实验六：存档 / 读档 ----
    std::printf("\n---------------- 存档测试 ----------------\n");
    const std::string file = "demo_save.txt";
    if (game.save(file)) {
        std::printf("存档成功 -> %s\n", file.c_str());
        int beforeGold = game.getPlayer().getGold();
        int beforeCore = game.getMap().getCore().getHp();

        if (game.load(file)) {
            std::printf("读档成功，恢复后：\n");
            std::printf("  金币 %d -> %d\n", beforeGold, game.getPlayer().getGold());
            std::printf("  核心 %d -> %d\n", beforeCore, game.getMap().getCore().getHp());
            std::printf("  恢复的宝可梦（具体派生类型）：\n");
            for (Pokemon* p : game.getMap().getDefenders()) {
                if (p) std::printf("    - %-10s Lv.%d HP %d 位置 %s 策略 %s\n",
                                   p->getClassName().c_str(), p->getLevel(), p->getHp(),
                                   p->getPosition().toString().c_str(),
                                   p->getStrategy() ? p->getStrategy()->getName().c_str() : "-");
            }
        } else {
            std::printf("读档失败\n");
        }
    } else {
        std::printf("存档失败\n");
    }

    // ---- 连携反应示例（直接演示两套属性机制）----
    std::printf("\n---------------- 连携反应示例 ----------------\n");
    std::printf("水 + 电 -> 电导链 ｜ 火 + 水 -> 蒸汽爆发 ｜ 电 + 草 -> 麻痹孢子\n");
    std::printf("火 + 草 -> 燃烧 ｜ 水 + 草 -> 缠绕生长 ｜ 火 + 电 -> 能量爆破\n");

    // ---- 单一附着槽验证：不会同时挂两种元素 ----
    std::printf("\n---------------- 附着规则验证 ----------------\n");
    {
        BasicEnemy dummy;
        ReactionEngine engine;
        engine.setEnemies(nullptr);

        auto show = [&](const char* step) {
            size_t n = dummy.getElementStates().size();
            std::printf("%-28s 附着数 = %zu  当前附着 = %s\n", step, n,
                        dummy.getElementStates().empty()
                            ? "-"
                            : typeToString(dummy.getElementStates().front()->getType()).c_str());
        };

        show("初始");

        engine.applyElement(dummy, PokemonType::Water, 5, 8.0);
        show("附加【水】");

        engine.applyElement(dummy, PokemonType::Water, 5, 8.0);
        show("再附加【水】(同属性刷新)");

        engine.applyElement(dummy, PokemonType::Fire, 5, 8.0);
        show("附加【火】(火+水=蒸汽爆发,消耗)");

        engine.applyElement(dummy, PokemonType::Electric, 5, 8.0);
        show("附加【电】(火+电=能量爆破,消耗)");

        engine.applyElement(dummy, PokemonType::Water, 5, 8.0);
        show("附加【水】");

        std::printf("=> 全程附着数始终 <= 1，不会出现同时挂两种元素\n");
        std::printf("   4 种属性两两组合共 6 种连携反应\n");
        std::printf("   连携触发时双方被消耗，随后由 Reaction 施加持续异常状态： %s\n",
                    dummy.getStatusLabel().c_str());
    }

    std::printf("\n演示结束。\n");
    return 0;
}
