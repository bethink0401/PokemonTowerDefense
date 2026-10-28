#include "Game.h"
#include "Pokemon.h"
#include "Enemy.h"
#include "ElementStatus.h"
#include "StatusEffect.h"
#include "SaveManager.h"
#include <cassert>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <iostream>

static std::string read(const std::string& path) {
    std::ifstream in(std::filesystem::u8path(path),std::ios::binary);
    return {std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>()};
}
static void write(const std::string& path,const std::string& value) {
    std::ofstream out(std::filesystem::u8path(path),std::ios::binary);out<<value;
}
static Position spot(Game& g) {
    for(int y=0;y<Position::MAP_HEIGHT;++y)for(int x=0;x<Position::MAP_WIDTH;++x)
        if(g.isDeployable(Position(x,y)))return Position(x,y);
    assert(false);return Position();
}
static void roundtrip(Game& source,Game& target) {
    assert(SaveManager::save(source,"build/_save_a.txt"));
    assert(SaveManager::load(target,"build/_save_a.txt"));
    assert(SaveManager::save(target,"build/_save_b.txt"));
    assert(read("build/_save_a.txt")==read("build/_save_b.txt"));
}
int main() {
    Game live,restored;
    live.getPlayer().addGold(5000);
    auto* unit=live.deployPokemon("Pichu",spot(live));assert(unit);
    assert(live.upgradePokemon(unit));assert(live.upgradePokemon(unit));
    assert(live.setPokemonStrategy(unit,"Fastest"));
    const int before=live.getPlayer().getGold();
    live.start();assert(live.getMap().getDefenderCount()==1);
    assert(live.getPlayer().getGold()==before);
    for(int i=0;i<35;++i)live.update(.05);
    assert(live.getMap().getEnemyCount()>0);
    auto* enemy=live.getMap().getEnemies().back();
    auto* tank=new TankEnemy();tank->applyWaveScaling(4);live.getMap().addEnemy(tank);
    unit->attack(*tank);assert(unit->getAttackCooldown()>0);
    enemy->setElementStatus(new FireStatus(4.2,8));
    enemy->addStatusEffect(new SlowEffect(2.7,.45));enemy->update(.17);
    live.getMap().getCore().takeDamage(7);
    live.getStatistics().addDamage(123);live.getStatistics().addLeak();
    live.pause();roundtrip(live,restored);
    assert(restored.isRunning()&&restored.isPaused());
    assert(restored.getEvolutionCount()==1);
    assert(live.upgradePokemon(unit)); // Keep the previous, longer attack timer.
    assert(unit->getAttackCooldown()>unit->getAttackInterval());roundtrip(live,restored);
    const int enemyCount=restored.getMap().getEnemyCount();
    restored.update(.05);assert(restored.getMap().getEnemyCount()==enemyCount);
    restored.resume();restored.update(.05);
    assert(restored.getWaveManager().getCurrentWave()==1);
    // Save after timers have expired; negative internal countdowns are normalized.
    unit->update(2);roundtrip(live,restored);
    const auto good=read("build/_save_a.txt");
    // V3 skill cooldown is accepted for migration but cannot delay normal attacks.
    std::istringstream current(good);std::ostringstream v3;std::string line;
    while(std::getline(current,line)) {
        if(line=="POKEMON_TD_SAVE 5")line="POKEMON_TD_SAVE 3";
        if(line.rfind("POKEMON ",0)==0)line.insert(line.find_last_of(' ')," 0.8");
        if(line.rfind("EFFECT ",0)==0) {
            std::istringstream row(line);std::string tag,id;double time,magnitude;
            row>>tag>>id>>time>>magnitude;
            std::ostringstream old;old<<std::setprecision(17)<<tag<<' '<<id<<' '<<time<<' '<<magnitude;line=old.str();
        }
        v3<<line<<'\n';
    }
    write("build/_v3_save.txt",v3.str());assert(SaveManager::load(restored,"build/_v3_save.txt"));
    assert(restored.getMap().getDefenders().front()->getAttackCooldown()==unit->getAttackCooldown());
    auto reject=[&](std::string bad){
        write("build/_bad_save.txt",bad);
        auto* original=restored.getMap().getDefenders().front();
        assert(SaveManager::save(restored,"build/_save_b.txt"));const auto state=read("build/_save_b.txt");
        assert(!SaveManager::load(restored,"build/_bad_save.txt"));
        assert(restored.getMap().getDefenders().front()==original);
        assert(SaveManager::save(restored,"build/_save_b.txt"));assert(read("build/_save_b.txt")==state);
    };
    reject(good.substr(0,good.size()/2));reject("garbage");
    auto bad=good;auto at=bad.find("\"Pikachu\"");assert(at!=std::string::npos);bad.replace(at,9,"\"Missing\"");reject(bad);
    bad=good;at=bad.find("PLAYER ");bad.insert(at+7,"-");reject(bad);
    // Atomic replacement and failure preserve the existing destination.
    assert(SaveManager::save(live,"build/_save_a.txt"));assert(read("build/_save_a.txt")==good);
    std::filesystem::create_directory("build/_blocked_save");
    assert(!SaveManager::save(live,"build/_blocked_save"));assert(std::filesystem::is_directory("build/_blocked_save"));
    const auto pos=unit->getPosition();std::ostringstream old;
    old<<"# PokemonTowerDefense Save File v1\nPLAYER 900 12\nCORE 100\nWAVE 3\nSTATS 2 1 30\nPOKEMON Pikachu "<<pos.getX()<<' '<<pos.getY()<<" 3 100 Fastest\n";
    write("build/_legacy_save.txt",old.str());assert(SaveManager::load(restored,"build/_legacy_save.txt"));
    assert(restored.isPaused()&&restored.getWaveManager().getCurrentWave()==3);
    restored.resume();restored.update(.21);assert(restored.getMap().getEnemyCount()==1);
    assert(restored.getWaveManager().getCurrentWave()==3);
    Game idle;assert(idle.deployPokemon("Squirtle",spot(idle)));roundtrip(idle,restored);
    restored.start();assert(restored.getMap().getDefenderCount()==1);
    restored.restart();assert(restored.getMap().getDefenderCount()==0&&restored.getPlayer().getGold()==400);
    restored.getMap().getCore().takeDamage(10000);restored.update(.01);assert(restored.isGameOver());
    roundtrip(restored,idle);assert(idle.isGameOver()&&!idle.isRunning());
    // Reach the actual victory transition, without relying on private fields.
    Game winner;winner.start();
    for(int i=0;i<200&&!winner.isOver();++i) {
        winner.update(1);
        for(auto* e:winner.getMap().getEnemies())e->takeDamage(1000000);
    }
    assert(winner.isVictory());roundtrip(winner,idle);assert(idle.isVictory());
    for(const char* id:{"Pichu","Pikachu","Raichu","Charmander","Charmeleon","Charizard","Squirtle","Wartortle","Blastoise","Bulbasaur","Ivysaur","Venusaur"}) {
        Game family;auto* p=createPokemonById(id);p->setLevel(5);p->setPosition(spot(family));family.getMap().addPokemon(p);
        roundtrip(family,idle);assert(idle.getMap().getDefenders().front()->getClassName()==id);
    }
    // Exercise Unicode Windows paths used by a Chinese-language installation.
    assert(SaveManager::save(live,u8"build/_测试存档.txt"));assert(SaveManager::load(idle,u8"build/_测试存档.txt"));
    for(auto path:{"build/_save_a.txt","build/_save_b.txt","build/_bad_save.txt","build/_legacy_save.txt","build/_v3_save.txt",u8"build/_测试存档.txt"})std::filesystem::remove(std::filesystem::u8path(path));
    std::filesystem::remove("build/_blocked_save");
    std::cout<<"PASS: live/paused/idle/game-over snapshots, cooldowns/effects/path/wave/statistics exact roundtrip, legacy migration, atomic overwrite, corrupt-file isolation, Unicode paths, start/restart\n";
}
