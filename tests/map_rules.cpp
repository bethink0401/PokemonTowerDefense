#include "Game.h"
#include "Enemy.h"
#include "Pokemon.h"
#include "StatusEffect.h"
#include "ElementStatus.h"
#include "ReactionEngine.h"
#include "TargetStrategy.h"
#include "BattleContext.h"
#include "ForestMapData.h"
#include "GameWidget.h"
#include <QApplication>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <cstdio>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    // Repeated water-gun slows refresh one status, never stack 100 icons/slows.
    TankEnemy waterEnemy;
    for(int i=0;i<100;++i) waterEnemy.addStatusEffect(new SlowEffect(2.5,.45));
    assert(waterEnemy.getStatusEffects().size()==1);
    assert(std::abs(waterEnemy.currentSpeed()-waterEnemy.getBaseSpeed()*.55)<1e-9);
    waterEnemy.addStatusEffect(new SlowEffect(5,.6));
    assert(waterEnemy.getStatusEffects().size()==1);
    assert(std::abs(waterEnemy.currentSpeed()-waterEnemy.getBaseSpeed()*.4)<1e-9);
    assert(waterEnemy.getStatusEffects()[0]->getRemainingTime()==5);
    waterEnemy.update(6);
    assert(waterEnemy.getStatusEffects().empty());
    ReactionEngine reactions;
    for(int i=0;i<100;++i) reactions.applyElement(waterEnemy,PokemonType::Fire,1,8);
    assert(waterEnemy.getType()==PokemonType::Water);
    assert(waterEnemy.getElementStates().size()==1);
    assert(waterEnemy.getPrimaryElement()->getType()==PokemonType::Fire);
    reactions.applyElement(waterEnemy,PokemonType::Water,1,8);
    assert(waterEnemy.getElementStates().empty());
    Pokemon* ranged=createPokemonById("Pichu");
    assert(std::abs(ranged->getRange()-3.5)<1e-9);
    ranged->setLevel(3);
    Pokemon* evolved=ranged->createEvolved();
    assert(evolved && evolved->getClassName()=="Pikachu" && evolved->getRange()>ranged->getRange());
    delete evolved; delete ranged;
    // Every form attacks on its own interval, with no second cooldown.
    for(const char* id : {"Pichu","Pikachu","Raichu","Charmander","Charmeleon","Charizard","Squirtle","Wartortle","Blastoise","Bulbasaur","Ivysaur","Venusaur"}) {
        Pokemon* unit=createPokemonById(id);
        Enemy target("BasicAttackAudit",10000,1,0,1,0,PokemonType::Water);
        ReactionEngine engine;int count=0;
        engine.setReactionCallback([&](const std::string&,const Position&){++count;});
        BattleContext context{};context.reactionEngine=&engine;unit->setContext(&context);
        unit->attack(target);assert(unit->didAttack());
        for(int i=0;i<8;++i) {
            const double interval=unit->getAttackInterval();
            unit->update(interval-.001);
            int hp=target.getHp();unit->attack(target);
            assert(!unit->didAttack() && target.getHp()==hp);
            unit->update(.002);unit->attack(target);assert(unit->didAttack());
            assert(!target.isImmobilized());
            if(unit->getType()==PokemonType::Grass) assert(target.getStatusEffects().empty());
        }
        assert(count==0);
        unit->setLevel(3);unit->update(10);
        Pokemon* evolution=unit->createEvolved();
        if(evolution) {
            assert(evolution->getAttackInterval()<unit->getAttackInterval());
            evolution->attack(target);assert(evolution->didAttack());
            evolution->update(evolution->getAttackInterval()+.001);
            evolution->attack(target);assert(evolution->didAttack());
            delete evolution;
        }
        delete unit;
    }
    // Ordinary attack damage has no hidden move power.
    Pokemon* grass=createPokemonById("Bulbasaur");
    Enemy neutral("DamageAudit",1000,1,0,1,0,PokemonType::Fire);
    grass->attack(neutral);
    assert(1000-neutral.getHp()==std::max(1,static_cast<int>(std::round(grass->getAttackPower()*TypeSystem::getMultiplier(grass->getType(),neutral.getType())))));
    delete grass;
    TankEnemy growthTarget;ReactionEngine growthEngine;
    growthEngine.applyElement(growthTarget,PokemonType::Water,1,8);
    growthEngine.applyElement(growthTarget,PokemonType::Grass,1,4);
    assert(growthTarget.getStatusEffects().size()==1);
    assert(growthTarget.getStatusEffects()[0]->getClassName()=="SlowEffect");

    // A lethal second hit must still emit the element reaction.
    TankEnemy lethalTarget;
    lethalTarget.takeDamage(260);
    lethalTarget.setElementStatus(new WaterStatus(8,1));
    ReactionEngine lethalEngine;
    int lethalReactions=0;
    lethalEngine.setReactionCallback([&](const std::string& name,const Position&){assert(name=="蒸汽爆发");++lethalReactions;});
    BattleContext lethalContext{};lethalContext.reactionEngine=&lethalEngine;
    Pokemon* attacker=createPokemonById("Charmander");
    attacker->setContext(&lethalContext);
    attacker->attack(lethalTarget);
    assert(lethalTarget.isDead() && lethalReactions==1);
    delete attacker;

    // All four target choices select within range.
    Pokemon* chooser=createPokemonById("Pichu");chooser->setPosition(Position(5,5));
    BasicEnemy near,low,far;FastEnemy fast;
    near.setPath({{6,5}});low.setPath({{5,7}});fast.setPath({{8,5}});far.setPath({{30,20}});
    low.takeDamage(50);far.takeDamage(59);
    std::vector<Enemy*> choices{&near,&low,&fast,&far};
    for(const auto& pair : std::vector<std::pair<std::string,Enemy*>>{{"Nearest",&near},{"LowestHP",&low},{"ClosestToCore",&fast},{"Fastest",&fast}}) {
        chooser->setStrategy(createStrategyByName(pair.first,Position(9,5)));
        assert(chooser->selectTarget(choices)==pair.second);
    }
    delete chooser;

    // Reactions stay local, remain briefly visible, then expire without a top banner.
    const std::vector<std::pair<PokemonType,PokemonType>> pairs={
        {PokemonType::Water,PokemonType::Electric},{PokemonType::Fire,PokemonType::Grass},
        {PokemonType::Water,PokemonType::Grass},{PokemonType::Fire,PokemonType::Electric},
        {PokemonType::Fire,PokemonType::Water},
        {PokemonType::Electric,PokemonType::Grass}};
    for(const auto& pair : pairs) {
        Game reactionGame;GameWidget reactionView;reactionView.setGame(&reactionGame);reactionView.resize(900,450);
        auto* target=new TankEnemy();reactionGame.getMap().addEnemy(target);target->setPath({{20,11}});
        QImage topBefore=reactionView.grab().toImage().copy(330,5,240,50);
        reactionGame.getReactionEngine().applyElement(*target,pair.first,1,8);
        reactionGame.getReactionEngine().applyElement(*target,pair.second,1,8);
        assert(reactionGame.getStatistics().getReactionsTriggered()==1);
        assert(reactionView.grab().toImage().copy(330,5,240,50)==topBefore);
        reactionView.tick(.25);
        QImage active=reactionView.grab().toImage().copy(320,140,160,150);
        reactionView.tick(.65);
        QImage settled=reactionView.grab().toImage().copy(320,140,160,150);
        assert(active!=settled);
        reactionView.tick(.2);
        assert(reactionView.grab().toImage().copy(320,140,160,150)==settled);
        assert(reactionView.grab().toImage().copy(330,5,240,50)==topBefore);
    }

    // Actual attack events must produce visible muzzle/flight/impact pixels.
    for(const auto* id : {"Pichu","Charmander","Squirtle","Bulbasaur"}) {
        Game fxGame;GameWidget fxView;fxView.setGame(&fxGame);fxView.resize(900,450);
        Pokemon* unit=createPokemonById(id);unit->setPosition(Position(18,10));fxGame.getMap().addPokemon(unit);
        auto* target=new TankEnemy();fxGame.getMap().addEnemy(target);target->setPath({{21,10}});
        QImage before=fxView.grab().toImage().copy(378,180,25,22);
        unit->attack(*target);fxView.tick(0);unit->clearAttackFlag();fxView.tick(.06);
        assert(fxView.grab().toImage().copy(378,180,25,22)!=before);
    }

    Game game;
    int meadowCount = 0;
    for (int y = 0; y < Position::MAP_HEIGHT; ++y) {
        assert(std::strlen(ForestMap::terrainRows()[y]) == Position::MAP_WIDTH);
        for (int x = 0; x < Position::MAP_WIDTH; ++x) {
            Position pos(x,y);
            bool meadow = ForestMap::terrainRows()[y][x] == 'G';
            assert(game.isDeployable(pos) == meadow);
            if (meadow) ++meadowCount;
            else assert(game.deployPokemon("Pichu", pos) == nullptr);
        }
    }
    assert(meadowCount >= 10);
    // Walk every segment with actual enemy movement, including rounded bends.
    for (const auto& path : {ForestMap::pathCave(), ForestMap::pathDen()}) {
        BasicEnemy e;
        e.setPath(path);
        for (size_t i = 1; i < path.size(); ++i) {
            double length = std::hypot(path[i].x-path[i-1].x, path[i].y-path[i-1].y);
            e.move(length / e.currentSpeed());
            assert(std::hypot(e.getX()-path[i].x,e.getY()-path[i].y) < 1e-5);
        }
        e.move(0.001);
        assert(e.hasReachedCore());
    }
    for (int y=0; y<Position::MAP_HEIGHT; ++y)
        for (int x=0; x<Position::MAP_WIDTH; ++x)
            if (game.isDeployable(Position(x,y))) {
                Pokemon* unit=game.deployPokemon("Pichu",Position(x,y));
                assert(unit);
                assert(game.setPokemonStrategy(unit,"Fastest"));
                assert(!game.setPokemonStrategy(unit,"Invalid"));
                game.getPlayer().addGold(1000);
                assert(game.upgradePokemon(unit));assert(game.upgradePokemon(unit));
                assert(unit->getClassName()=="Pikachu" && unit->getStrategy()->getName()=="Fastest");
                assert(!game.isDeployable(Position(x,y)));
                goto deployed;
            }
deployed:
    GameWidget view;
    view.setGame(&game);
    view.resize(1220,650);
    assert(game.save("build/basic_attack_audit.txt"));
    Game restored;assert(restored.load("build/basic_attack_audit.txt"));
    assert(restored.getMap().getDefenders().size()==1);
    Pokemon* loaded=restored.getMap().getDefenders()[0];
    assert(loaded->getClassName()=="Pikachu");
    assert(loaded->getStrategy()->getName()=="Fastest");
    std::remove("build/basic_attack_audit.txt");
    const QImage hiddenMarkers = view.grab().toImage();
    view.setDeployType("Squirtle");
    assert(view.grab().toImage() != hiddenMarkers);
    assert(view.grab().save("assets/maps/_game_preview.png"));
    view.clearDeployType();
    assert(view.grab().toImage() == hiddenMarkers);
    std::cout << "PASS: 12 ordinary-attack forms, attack/upgrade/evolution cadence, panel damage, no poison, save/load and cooldown safety, 4 attack renderers, lethal reactions, 6 short local reactions, no top banner, 4 strategies and evolution persistence, deployment marker visibility, status deduplication, innate/attached elements, range/evolution, terrain, rejected deployment, occupancy, both enemy routes, Qt render\n";
}

