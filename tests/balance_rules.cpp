#include "Game.h"
#include "Pokemon.h"
#include "Enemy.h"
#include "StatusEffect.h"
#include "ElementStatus.h"
#include "TargetStrategy.h"
#include "UnitInsights.h"
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <filesystem>
#include <array>

static std::unique_ptr<Pokemon> grown(const char* id,int level) {
    std::unique_ptr<Pokemon> p(createPokemonById(id));p->setLevel(level);
    while(p->canEvolve())p.reset(p->createEvolved());
    return p;
}
static double measure(const char* id,int level,int count,bool armored) {
    double sum=0;
    for(int seed=1;seed<=16;++seed) {
        std::srand(seed);GameMap map;ReactionEngine engine;BattleContext ctx;
        auto unit=grown(id,level);unit->setPosition(Position(20,10));
        unit->setStrategy(createStrategyByName("Nearest",Position(40,10)));
        auto* p=unit.release();map.addPokemon(p);p->setContext(&ctx);
        ctx.enemies=&map.getEnemies();ctx.reactionEngine=&engine;engine.setEnemies(ctx.enemies);
        for(int i=0;i<count;++i) {
            // Targets stay within range; group matches ordinary grass enemies.
            auto* e=new Enemy("Benchmark",100000,.000001,1,1,armored?.35:0,
                armored?PokemonType::Water:count>1?PokemonType::Grass:p->getType());
            map.addEnemy(e);e->setPath({{21,10},{39,10}});
        }
        for(int i=0;i<3000;++i)map.update(.01,ctx);
        int damage=count*100000;for(auto* e:map.getEnemies())damage-=e->getHp();
        sum+=damage/30.0;
    }
    return sum/16;
}
static Position spot(Game& g) {
    for(int y=0;y<Position::MAP_HEIGHT;++y)for(int x=0;x<Position::MAP_WIDTH;++x)
        if(g.isDeployable(Position(x,y)))return Position(x,y);
    assert(false);return Position();
}
static std::string read(const char* path) {std::ifstream in(path);return {std::istreambuf_iterator<char>(in),{}};}
int main() {
    PokemonType unused;assert(!stringToType("Ice",unused));
    const std::array<PokemonType,4> types={PokemonType::Fire,PokemonType::Water,PokemonType::Grass,PokemonType::Electric};
    // All four attachments are inert, even for legacy high-strength save values.
    for(auto type:types)for(int strength:{0,1,999}) {
        Enemy target("AttachmentMarker",10000,1,1,1,0,PokemonType::Water);
        target.setPath({{0,0},{100,0}});ReactionEngine engine;
        engine.applyElement(target,type,strength,8);
        target.update(1);
        assert(target.getHp()==10000 && target.getStatusEffects().empty());
        assert(!target.isImmobilized() && std::abs(target.currentSpeed()-1)<1e-9);
        assert(std::abs(target.getX()-1)<1e-9 && target.getPrimaryElement());
        engine.applyElement(target,type,strength,8); // Same-element refresh has no side effects.
        target.update(7.9);assert(target.getPrimaryElement());
        target.update(.2);
        assert(!target.getPrimaryElement() && target.getHp()==10000);
        assert(target.getStatusEffects().empty() && std::abs(target.getX()-9.1)<1e-9);
    }
    for(auto a:types)for(auto b:types) {
        Enemy target("ReactionAudit",10000,1,1,1,0,a);ReactionEngine engine;int count=0;
        engine.setReactionCallback([&](const std::string&,const Position&){++count;});
        engine.applyElement(target,a,1,8);engine.applyElement(target,b,1,8);
        assert(count==(a==b?0:1));
    }
    // Burn is frame-rate independent and refreshes do not reset its fractional tick.
    for(double dt:{.01,.05,.2,2.5}) {
        Enemy target("BurnAudit",1000,1,1,1,0,PokemonType::Water);
        target.addStatusEffect(new BurnEffect(2.5,2.4));
        for(double time=0;time<2.5-1e-8;time+=dt)target.update(std::min(dt,2.5-time));
        assert(target.getHp()==994);
    }
    Enemy refreshed("BurnRefresh",1000,1,1,1,0,PokemonType::Water);
    for(int i=0;i<200;++i){refreshed.addStatusEffect(new BurnEffect(2,2));refreshed.update(.01);}
    assert(refreshed.getHp()==996 && refreshed.getStatusEffects().size()==1);
    // The same RNG sequence and stop durations survive different update step sizes.
    std::srand(42);Enemy coarse("Coarse",1000,1,1,1,0,PokemonType::Water);
    coarse.setPath({{0,0},{100,0}});coarse.addStatusEffect(new ParalyzeEffect(20,.5));coarse.update(10);
    std::srand(42);Enemy fine("Fine",1000,1,1,1,0,PokemonType::Water);
    fine.setPath({{0,0},{100,0}});fine.addStatusEffect(new ParalyzeEffect(20,.5));
    for(int i=0;i<1000;++i)fine.update(.01);
    assert(std::abs(coarse.getX()-fine.getX())<1e-6);
    double averageBlocked=0;
    for(int seed=1;seed<=128;++seed) {
        std::srand(seed);Enemy target("Stutter",1000,1,1,1,0,PokemonType::Water);
        target.setPath({{0,0},{1000,0}});double longest=0,run=0;
        for(int i=0;i<6000;++i) {
            if(i%10==0)target.addStatusEffect(new ParalyzeEffect(3,.5));
            assert(std::abs(target.currentSpeed()-1)<1e-9); // paralysis is not a slow
            double before=target.getX();target.update(.01);
            double blocked=.01-(target.getX()-before);
            if(blocked>.009)run+=.01;else run=0;
            longest=std::max(longest,run);
        }
        assert(longest<=.26+1e-6 && target.getStatusEffects().size()==1);
        averageBlocked+=(60-target.getX())/60;
    }
    averageBlocked/=128;assert(averageBlocked>.1 && averageBlocked<.3);
    // Save in the middle of a burn tick and paralysis cycle; future movement must match.
    Game original,loaded;assert(original.deployPokemon("Pichu",spot(original)));
    auto* target=new TankEnemy();target->applyWaveScaling(50);original.getMap().addEnemy(target);
    target->setPath({{10,10},{38,10}});target->addStatusEffect(new BurnEffect(10,2.4));
    target->addStatusEffect(new ParalyzeEffect(20,.5));target->update(.63);
    assert(original.save("build/_balance_save.txt"));assert(loaded.load("build/_balance_save.txt"));
    auto* restored=loaded.getMap().getEnemies().front();
    for(int i=0;i<500;++i) {
        target->update(.01);restored->update(.01);
        assert(target->getHp()==restored->getHp());
        assert(std::abs(target->getX()-restored->getX())<1e-9);
        assert(target->isImmobilized()==restored->isImmobilized());
    }
    // A genuine old ice/freeze snapshot migrates with those obsolete states removed.
    Game old;assert(old.deployPokemon("Pichu",spot(old)));auto* legacy=new TankEnemy();old.getMap().addEnemy(legacy);
    legacy->setElementStatus(new WaterStatus(4,1));legacy->addStatusEffect(new SlowEffect(2,.4));
    assert(old.save("build/_balance_save.txt"));
    std::istringstream rows(read("build/_balance_save.txt"));std::ostringstream converted;std::string line;
    while(std::getline(rows,line)) {
        if(line=="POKEMON_TD_SAVE 5")line="POKEMON_TD_SAVE 4";
        if(line.rfind("ELEMENT Water",0)==0)line="ELEMENT Ice 4 1 0";
        if(line.rfind("EFFECT ",0)==0)line="EFFECT FreezeEffect 2 0";
        converted<<line<<'\n';
    }
    {std::ofstream out("build/_balance_legacy.txt");out<<converted.str();}
    assert(loaded.load("build/_balance_legacy.txt"));
    assert(loaded.getMap().getEnemies().front()->getElementStates().empty());
    assert(loaded.getMap().getEnemies().front()->getStatusEffects().empty());
    std::filesystem::remove("build/_balance_save.txt");std::filesystem::remove("build/_balance_legacy.txt");
    std::ofstream report("build/balance_report.csv");
    report<<"family,level,investment,attack,interval,range,neutral_dps,tank_dps,group5_dps\n"<<std::fixed<<std::setprecision(3);
    const char* families[]={"Pichu","Charmander","Squirtle","Bulbasaur"};
    double neutral[3][4]{},tank[3][4]{},group[3][4]{},cost[3][4]{};int k=0;
    for(int level:{1,3,5}) {
        for(int f=0;f<4;++f) {
            auto p=grown(families[f],level);int investment=p->getCost();
            for(int l=1;l<level;++l)investment+=l*60+40;
            neutral[k][f]=measure(families[f],level,1,false);
            tank[k][f]=measure(families[f],level,1,true);
            group[k][f]=measure(families[f],level,5,false);cost[k][f]=investment;
            report<<families[f]<<','<<level<<','<<investment<<','<<p->getAttackPower()<<','<<p->getAttackInterval()<<','<<p->getRange()<<','<<neutral[k][f]<<','<<tank[k][f]<<','<<group[k][f]<<'\n';
            if(level==5) {
                assert(p->getAttackInterval()<grown(families[f],3)->getAttackInterval());
                if(f==0)assert(p->getAttackInterval()<.55);
                if(f==1)assert(p->getSplashRadius()>1.7);
                if(f==2)assert(p->getSlowRatio()>.38 && p->getSlowRatio()<.45);
                if(f==3)assert(p->getRange()>5.5 && p->getArmorPenetration()>.69);
            }
        }
        // Roles win different encounters after accounting for full investment.
        assert(neutral[k][0]/cost[k][0]>neutral[k][3]/cost[k][3]);
        for(int f:{0,2,3})assert(group[k][1]/cost[k][1]>group[k][f]/cost[k][f]);
        assert(neutral[k][2]<neutral[k][0]);
        ++k;
    }
    report.close();
    assert(tank[2][3]/cost[2][3]>tank[2][0]/cost[2][0]);
    // Full eight-wave runs with fixed opening budgets, shared deployment/upgrade policy.
    const std::vector<std::vector<const char*>> openings={
        {"Pichu","Pichu","Pichu","Pichu"},{"Charmander","Charmander"},
        {"Squirtle","Squirtle","Squirtle"},{"Bulbasaur","Bulbasaur","Bulbasaur"},
        {"Pichu","Charmander","Squirtle"},{"Pichu","Squirtle","Bulbasaur"},
        {"Charmander","Squirtle","Bulbasaur"},{"Pichu","Charmander","Bulbasaur"}};
    std::ofstream waves("build/wave_balance_report.csv");waves<<"opening,wins,seeds,average_core,average_wave\n";
    int mixedWins=0;
    for(size_t opening=0;opening<openings.size();++opening) {
        int wins=0;double health=0,progress=0;
        for(int seed=1;seed<=16;++seed) {
            std::srand(seed);Game game;
            std::vector<FWaypoint> samples;
            for(const auto& lane:game.getMap().getPointPaths())for(size_t i=1;i<lane.size();++i) {
                double length=std::hypot(lane[i].x-lane[i-1].x,lane[i].y-lane[i-1].y);
                int steps=std::max(1,int(std::ceil(length/.5)));
                for(int j=0;j<steps;++j){double t=double(j)/steps;samples.push_back({lane[i-1].x+(lane[i].x-lane[i-1].x)*t,lane[i-1].y+(lane[i].y-lane[i-1].y)*t});}
            }
            std::vector<int> coverage(samples.size());
            for(const char* id:openings[opening]) {
                auto prototype=grown(id,1);Position best;double bestScore=-1;
                for(int y=0;y<Position::MAP_HEIGHT;++y)for(int x=0;x<Position::MAP_WIDTH;++x)if(game.isDeployable(Position(x,y))) {
                    double score=0;
                    for(size_t i=0;i<samples.size();++i)if(std::hypot(x-samples[i].x,y-samples[i].y)<=prototype->getRange())score+=1.0/(1+.4*coverage[i]);
                    if(score>bestScore){bestScore=score;best=Position(x,y);}
                }
                auto* unit=game.deployPokemon(id,best);assert(unit);
                for(size_t i=0;i<samples.size();++i)if(std::hypot(best.getX()-samples[i].x,best.getY()-samples[i].y)<=unit->getRange())++coverage[i];
            }
            game.start();
            for(int tick=0;tick<20000 && !game.isOver();++tick) {
                game.update(.05);
                // Upgrade the lowest-level affordable defender, preserving a common policy.
                Pokemon* choice=nullptr;
                for(auto* unit:game.getMap().getDefenders())if(unit->getLevel()<5 && game.getPlayer().canAfford(unit->getUpgradeCost()) && (!choice||unit->getLevel()<choice->getLevel()))choice=unit;
                if(choice)assert(game.upgradePokemon(choice));
            }
            assert(game.isOver());wins+=game.isVictory();health+=game.getMap().getCore().getHp();progress+=game.getWaveManager().getCurrentWave();
        }
        if(opening>=4)mixedWins+=wins;
        waves<<opening<<','<<wins<<",16,"<<health/16<<','<<progress/16<<'\n';
    }
    waves.close();assert(mixedWins>0);
    std::cout<<"PASS: inert attachments/refresh/expiry for all 4 elements, 6 reactions, no ice, frame-independent burns/stutters, refresh lockout protection, exact status/RNG save continuation, legacy ice migration, 16-seed 30s role benchmarks at levels 1/3/5, 128 full eight-wave runs; paralysis blocked fraction="<<averageBlocked<<"\n";
}
