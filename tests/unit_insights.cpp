#include "UnitInsights.h"
#include "Game.h"
#include "SaveManager.h"
#include <cassert>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <iostream>
#include <memory>
int main() {
    Game g;g.getPlayer().addGold(2000);
    Position pos;
    for(int y=0;y<Position::MAP_HEIGHT;++y)for(int x=0;x<Position::MAP_WIDTH;++x)
        if(g.isDeployable(Position(x,y)))pos=Position(x,y);
    auto* p=g.deployPokemon("Charmander",pos);assert(p);
    auto* first=new TankEnemy();first->applyWaveScaling(10);g.getMap().addEnemy(first);
    auto* second=new TankEnemy();second->applyWaveScaling(10);g.getMap().addEnemy(second);
    const std::vector<FWaypoint> lane={{double(pos.getX()+1),double(pos.getY())},{double(pos.getX()+2),double(pos.getY())}};
    first->setPath(lane);second->setPath(lane);
    assert(UnitInsights::enemiesInRange(*p,g.getMap())==2);
    assert(p->selectTarget(g.getMap().getEnemies()));
    BattleContext context;context.enemies=&g.getMap().getEnemies();context.reactionEngine=&g.getReactionEngine();p->setContext(&context);
    const int before=first->getHp()+second->getHp();
    g.getMap().update(.01,context);
    assert(p->getCumulativeDamage()==before-first->getHp()-second->getHp());
    assert(p->getCumulativeDamage()>0&&UnitInsights::remaining(*p)>0);
    const auto damage=p->getCumulativeDamage();
    assert(g.upgradePokemon(p));assert(g.upgradePokemon(p));
    assert(p->getClassName()=="Charmeleon"&&p->getCumulativeDamage()==damage);
    assert(SaveManager::save(g,"build/_insights_save.txt"));
    Game loaded;assert(SaveManager::load(loaded,"build/_insights_save.txt"));
    assert(loaded.getMap().getDefenders().front()->getCumulativeDamage()==damage);
    // V2 still loads: the new cumulative field did not exist in that format.
    std::ifstream in("build/_insights_save.txt");std::string line;std::ostringstream legacy;
    while(std::getline(in,line)) {
        if(line=="POKEMON_TD_SAVE 5")line="POKEMON_TD_SAVE 2";
        if(line.rfind("POKEMON ",0)==0){ line.erase(line.find_last_of(' ')); line+=" 0"; }
        if(line.rfind("EFFECT ",0)==0) {
            std::istringstream row(line);std::string tag,id;double time,magnitude;
            row>>tag>>id>>time>>magnitude;
            std::ostringstream old;old<<std::setprecision(17)<<tag<<' '<<id<<' '<<time<<' '<<magnitude;line=old.str();
        }
        legacy<<line<<'\n';
    }
    in.close();{std::ofstream out("build/_insights_v2.txt");out<<legacy.str();}
    assert(SaveManager::load(loaded,"build/_insights_v2.txt"));
    assert(loaded.getMap().getDefenders().front()->getCumulativeDamage()==0);
    std::unique_ptr<Pokemon> a(createPokemonById("Pichu")),b(createPokemonById("Squirtle"));
    a->setPosition(Position(0,0));b->setPosition(Position(4,0));
    GameMap coverage;coverage.setPointPaths({{{2,0},{3,0}}});
    assert(UnitInsights::sharesRoad(*a,*b,coverage));
    coverage.setPointPaths({{{20,20},{21,20}}});assert(!UnitInsights::sharesRoad(*a,*b,coverage));
    assert(std::abs(UnitInsights::dps(*a)-a->getAttackPower()/UnitInsights::interval(*a))<.00001);
    std::filesystem::remove("build/_insights_save.txt");std::filesystem::remove("build/_insights_v2.txt");
    std::cout<<"PASS: real attack/splash damage attribution, cooldown/target/coverage, evolution and V5 persistence, V2 migration, shared road rather than proximity, DPS formula\n";
}
