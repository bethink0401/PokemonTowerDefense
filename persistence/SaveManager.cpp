#include "SaveManager.h"
#include "Game.h"
#include "Pokemon.h"
#include "Enemy.h"
#include "ElementStatus.h"
#include "StatusEffect.h"
#include "TargetStrategy.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <locale>
#include <memory>
#include <set>
#include <sstream>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {
struct UnitRecord {
    std::string id,strategy;
    int x=0,y=0,level=1,hp=1;
    double attack=0,animation=0,move=0;
    long long damage=0;
};
struct ElementRecord {std::string type;double time=0,tick=0;int strength=0;};
struct EffectRecord {std::string id;double time=0,magnitude=0,tick=0,damage=0,check=0,stop=0;std::uint64_t rng=0;};
struct EnemyRecord {
    std::string id;int hp=0,maxHp=0,reward=0,index=0;
    double x=0,y=0;
    std::vector<FWaypoint> path;
    std::vector<ElementRecord> elements;
    std::vector<EffectRecord> effects;
};
struct Snapshot {
    int running=0,paused=0,over=0,victory=0,evolutions=0,evoX=0,evoY=0;
    int gold=400,score=0,core=120,wave=0,maxWave=8,active=0,total=0,spawned=0;
    double timer=0,interval=.9;
    std::array<int,6> stats{};
    std::vector<UnitRecord> units;
    std::vector<EnemyRecord> enemies;
    std::vector<std::string> logs;
    bool legacy=false;
    int version=1;
};
bool number(double x,double lo,double hi){return std::isfinite(x)&&x>=lo&&x<=hi;}
bool bit(int x){return x==0||x==1;}
bool strategy(const std::string& id){return id=="Nearest"||id=="LowestHP"||id=="ClosestToCore"||id=="Fastest";}
bool unitId(const std::string& id) {
    static const std::set<std::string> ids={"Pichu","Pikachu","Raichu","Charmander","Charmeleon","Charizard","Squirtle","Wartortle","Blastoise","Bulbasaur","Ivysaur","Venusaur"};
    return ids.count(id)!=0;
}
bool enemyId(const std::string& id){return id=="BasicEnemy"||id=="FastEnemy"||id=="TankEnemy";}
bool tag(std::istream& in,const char* expected){std::string value;return bool(in>>value)&&value==expected;}
template<class... Args> bool record(std::istream& in,const char* expected,Args&... args){return tag(in,expected)&&bool((in>>...>>args));}
bool readV2(std::istream& in,Snapshot& s,bool hasDamage,bool hasMove=true) {
    if(!record(in,"GAME",s.running,s.paused,s.over,s.victory,s.evolutions,s.evoX,s.evoY) ||
       !record(in,"PLAYER",s.gold,s.score) || !record(in,"CORE",s.core) ||
       !record(in,"WAVE",s.wave,s.maxWave,s.active,s.timer,s.interval,s.total,s.spawned) || !tag(in,"STATS"))return false;
    for(int& value:s.stats)if(!(in>>value))return false;
    int count=0;
    if(!record(in,"DEFENDERS",count)||count<0||count>128)return false;
    for(int i=0;i<count;++i) {
        UnitRecord u;
        if(!tag(in,"POKEMON") || !(in>>std::quoted(u.id)>>u.x>>u.y>>u.level>>u.hp>>u.attack>>u.animation>>std::quoted(u.strategy)))return false;
        if(hasMove && !(in>>u.move))return false;
        if(hasDamage&&!(in>>u.damage))return false;
        s.units.push_back(u);
    }
    if(!record(in,"ENEMIES",count)||count<0||count>512)return false;
    for(int i=0;i<count;++i) {
        EnemyRecord e;
        if(!tag(in,"ENEMY")||!(in>>std::quoted(e.id)>>e.hp>>e.maxHp>>e.reward>>e.x>>e.y>>e.index))return false;
        int points=0;
        if(!record(in,"PATH",points)||points<1||points>4096)return false;
        for(int j=0;j<points;++j){FWaypoint p;if(!(in>>p.x>>p.y))return false;e.path.push_back(p);}
        int elements=0;
        if(!record(in,"ELEMENTS",elements)||elements<0||elements>1)return false;
        for(int j=0;j<elements;++j){ElementRecord el;if(!record(in,"ELEMENT",el.type,el.time,el.strength,el.tick))return false;e.elements.push_back(el);}
        int effects=0;
        if(!record(in,"EFFECTS",effects)||effects<0||effects>4)return false;
        for(int j=0;j<effects;++j){
            EffectRecord ef;if(!record(in,"EFFECT",ef.id,ef.time,ef.magnitude))return false;
            if(s.version>=5 && !(in>>ef.tick>>ef.damage>>ef.check>>ef.stop>>ef.rng))return false;
            e.effects.push_back(ef);
        }
        s.enemies.push_back(std::move(e));
    }
    if(!record(in,"LOGS",count)||count<0||count>64)return false;
    for(int i=0;i<count;++i){std::string line;if(!tag(in,"LOG")||!(in>>std::quoted(line))||line.size()>4096)return false;s.logs.push_back(line);}
    if(!tag(in,"END"))return false;
    in>>std::ws;
    return in.eof();
}
bool readLegacy(std::istream& in,Snapshot& s) {
    bool player=false,core=false,wave=false,stats=false;s.legacy=true;
    std::string line;
    while(std::getline(in,line)) {
        if(line.empty()||line[0]=='#')continue;
        std::istringstream row(line);row.imbue(std::locale::classic());std::string id;if(!(row>>id))continue;
        if(id=="PLAYER"&&!player){if(!(row>>s.gold>>s.score))return false;player=true;}
        else if(id=="CORE"&&!core){if(!(row>>s.core))return false;core=true;}
        else if(id=="WAVE"&&!wave){if(!(row>>s.wave))return false;wave=true;}
        else if(id=="STATS"&&!stats){if(!(row>>s.stats[0]>>s.stats[2]>>s.stats[3]))return false;stats=true;}
        else if(id=="POKEMON") {
            UnitRecord u;if(!(row>>u.id>>u.x>>u.y>>u.level>>u.hp>>u.strategy)||s.units.size()>=128)return false;
            s.units.push_back(u);
        } else return false;
        row>>std::ws;if(!row.eof())return false;
    }
    if(!player||!core||!wave||!stats||s.wave<0||s.wave>s.maxWave)return false;
    s.running=s.wave>0&&s.core>0;s.paused=s.running;s.over=s.core==0;
    s.active=s.running;s.total=s.wave>0?5+2*s.wave:0;s.spawned=0;s.timer=s.wave>0?.2:0;
    s.stats[4]=std::max(0,s.wave-1);
    return true;
}
bool valid(const Snapshot& s,int coreMax) {
    if(!bit(s.running)||!bit(s.paused)||!bit(s.over)||!bit(s.victory)||!bit(s.active)||
       ((s.over||s.victory)&&s.running)||s.evolutions<0||s.evolutions>1000000 ||
       s.evoX<0||s.evoX>=Position::MAP_WIDTH||s.evoY<0||s.evoY>=Position::MAP_HEIGHT ||
       s.gold<0||s.gold>1000000000||s.score<0||s.score>1000000000||s.core<0||s.core>coreMax ||
       s.maxWave<1||s.maxWave>1000||s.wave<0||s.wave>s.maxWave||s.total<0||s.total>100000||
       s.spawned<0||s.spawned>s.total||!number(s.timer,-1,3600)||!number(s.interval,.01,3600))return false;
    if(s.wave==0&&(s.total!=0||s.spawned!=0||s.active))return false;
    for(int value:s.stats)if(value<0||value>1000000000)return false;
    std::set<std::pair<int,int>> occupied;
    for(const auto& u:s.units) {
        if(!unitId(u.id)||!strategy(u.strategy)||u.x<0||u.x>=Position::MAP_WIDTH||u.y<0||u.y>=Position::MAP_HEIGHT||
           u.level<1||u.level>5||u.hp<1||!number(u.attack,0,3600)||!number(u.animation,0,1)||!number(u.move,0,3600)||
           u.damage<0||u.damage>9000000000000000LL||!occupied.emplace(u.x,u.y).second)return false;
    }
    for(const auto& e:s.enemies) {
        if(!enemyId(e.id)||e.hp<1||e.hp>e.maxHp||e.maxHp>10000000||e.reward<0||e.reward>1000000||
           !number(e.x,-1,Position::MAP_WIDTH)||!number(e.y,-1,Position::MAP_HEIGHT)||e.index<0||e.index>=int(e.path.size()))return false;
        for(const auto& point:e.path)if(!number(point.x,-1,Position::MAP_WIDTH)||!number(point.y,-1,Position::MAP_HEIGHT))return false;
        for(const auto& el:e.elements){PokemonType type;if((!stringToType(el.type,type) && !(s.version<5 && el.type=="Ice"))||!number(el.time,.000001,3600)||el.strength<0||el.strength>100000||!number(el.tick,0,60))return false;}
        std::set<std::string> effects;
        for(const auto& ef:e.effects) {
            if((ef.id!="BurnEffect"&&!(s.version<5 && ef.id=="FreezeEffect")&&ef.id!="ParalyzeEffect"&&ef.id!="SlowEffect")||
               !effects.insert(ef.id).second||!number(ef.time,.000001,3600)||!number(ef.magnitude,0,ef.id=="BurnEffect"?10000:(s.version>=5 && ef.id=="ParalyzeEffect"?.5:1)) ||
               !number(ef.tick,0,.5)||!number(ef.damage,0,5000)||!number(ef.check,0,.5)||!number(ef.stop,0,.25)||ef.rng>4294967295ULL)return false;
        }
    }
    return true;
}
bool atomicReplace(const std::filesystem::path& temporary,const std::filesystem::path& destination) {
#ifdef _WIN32
    return MoveFileExW(temporary.c_str(),destination.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
    std::error_code error;std::filesystem::rename(temporary,destination,error);return !error;
#endif
}
}

bool SaveManager::save(Game& game,const std::string& filename) {
    try {
        const auto destination=std::filesystem::u8path(filename),temporary=std::filesystem::u8path(filename+".tmp");
        std::ofstream out(temporary,std::ios::binary|std::ios::trunc);
        if(!out)return false;
        out.imbue(std::locale::classic());out<<std::setprecision(17);
        out<<"POKEMON_TD_SAVE 5\nGAME "<<game.running<<' '<<game.paused<<' '<<game.gameOver<<' '<<game.victory<<' '<<game.evolutionCount<<' '<<game.lastEvolvePos.getX()<<' '<<game.lastEvolvePos.getY()<<'\n';
        out<<"PLAYER "<<game.player.getGold()<<' '<<game.player.getScore()<<"\nCORE "<<game.map.getCore().getHp()<<'\n';
        const auto& wave=game.waveManager;
        out<<"WAVE "<<wave.currentWave<<' '<<wave.maxWave<<' '<<wave.waveActive<<' '<<wave.spawnTimer<<' '<<wave.spawnInterval<<' '<<wave.toSpawn<<' '<<wave.spawned<<'\n';
        const auto& stats=game.statistics;
        out<<"STATS "<<stats.kills<<' '<<stats.totalDamage<<' '<<stats.reactionsTriggered<<' '<<stats.goldEarned<<' '<<stats.wavesCleared<<' '<<stats.enemiesLeaked<<'\n';
        out<<"DEFENDERS "<<game.map.getDefenders().size()<<'\n';
        for(const auto* p:game.map.getDefenders()) {
            if(!p)return false;
            out<<"POKEMON "<<std::quoted(p->getClassName())<<' '<<p->position.getX()<<' '<<p->position.getY()<<' '<<p->level<<' '<<p->hp<<' '<<std::max(0.0,p->attackTimer)<<' '<<std::max(0.0,p->attackAnimTimer)<<' '<<std::quoted(p->strategy?p->strategy->getName():"Nearest")<<' '<<p->cumulativeDamage<<'\n';
        }
        std::vector<const Enemy*> live;
        for(const auto* e:game.map.getEnemies())if(e&&!e->isDead()&&!e->hasReachedCore())live.push_back(e);
        out<<"ENEMIES "<<live.size()<<'\n';
        for(const auto* e:live) {
            out<<"ENEMY "<<std::quoted(e->getClassName())<<' '<<e->hp<<' '<<e->maxHp<<' '<<e->reward<<' '<<e->posX<<' '<<e->posY<<' '<<e->pathIndex<<'\n';
            out<<"PATH "<<e->path.size();for(const auto& point:e->path)out<<' '<<point.x<<' '<<point.y;out<<'\n';
            out<<"ELEMENTS "<<e->elementStates.size()<<'\n';
            for(const auto* el:e->elementStates)out<<"ELEMENT "<<typeToString(el->type)<<' '<<el->remainingTime<<' '<<el->strength<<' '<<el->tickAcc<<'\n';
            out<<"EFFECTS "<<e->statusEffects.size()<<'\n';
            for(const auto* effect:e->statusEffects) {
                double tick=0,damage=0,check=0,stop=0;std::uint32_t rng=0;
                if(auto* burn=dynamic_cast<const BurnEffect*>(effect)){tick=burn->tickAcc;damage=burn->damageAcc;}
                if(auto* paralysis=dynamic_cast<const ParalyzeEffect*>(effect)){check=paralysis->checkRemaining;stop=paralysis->stopRemaining;rng=paralysis->randomState;}
                out<<"EFFECT "<<effect->getClassName()<<' '<<effect->getRemainingTime()<<' '<<effect->getMagnitude()<<' '<<tick<<' '<<damage<<' '<<check<<' '<<stop<<' '<<rng<<'\n';
            }
        }
        out<<"LOGS "<<game.log.size()<<'\n';for(const auto& line:game.log)out<<"LOG "<<std::quoted(line)<<'\n';
        out<<"END\n";out.flush();bool good=bool(out);out.close();
        if(!good||out.fail())return false;
        if(atomicReplace(temporary,destination))return true;
        std::error_code ignored;std::filesystem::remove(temporary,ignored);return false;
    } catch(const std::exception&) {return false;}
}

bool SaveManager::load(Game& game,const std::string& filename) {
    try {
        const auto source=std::filesystem::u8path(filename);
        std::error_code error;const auto size=std::filesystem::file_size(source,error);
        if(error||size==0||size>10*1024*1024)return false;
        std::ifstream in(source,std::ios::binary);in.imbue(std::locale::classic());if(!in)return false;
        std::string header;if(!std::getline(in,header))return false;
        if(!header.empty()&&header.back()=='\r')header.pop_back();
        if(header.compare(0,3,"\xef\xbb\xbf")==0)header.erase(0,3);
        Snapshot s;
        if(header=="POKEMON_TD_SAVE 2"||header=="POKEMON_TD_SAVE 3"||header=="POKEMON_TD_SAVE 4"||header=="POKEMON_TD_SAVE 5") {
            s.version=header.back()-'0';
            if(!readV2(in,s,s.version>=3,s.version<4))return false;
        }
        else if(header=="# PokemonTowerDefense Save File v1") {if(!readLegacy(in,s))return false;}
        else return false;
        if(!valid(s,game.map.getCore().getMaxHp()))return false;
        // Build and validate everything before replacing the running game.
        std::vector<std::unique_ptr<Pokemon>> units;
        std::vector<std::unique_ptr<Enemy>> enemies;
        for(const auto& u:s.units) {
            if(!game.map.isTallGrass(Position(u.x,u.y)))return false;
            std::unique_ptr<Pokemon> p(createPokemonById(u.id));p->setLevel(u.level);
            // Upgrading shortens the interval while the previous attack timer can
            // still be counting down at its old value.
            if(s.version>=5 && u.hp>p->getMaxHp())return false;
            p->setPosition(Position(u.x,u.y));p->setHp(u.hp);p->setStrategy(createStrategyByName(u.strategy,game.map.getCorePosition()));
            p->cumulativeDamage=u.damage;p->attackTimer=u.attack;p->attackAnimTimer=u.animation;
            units.push_back(std::move(p));
        }
        for(const auto& saved:s.enemies) {
            std::unique_ptr<Enemy> e(createEnemyById(saved.id));
            e->hp=saved.hp;e->maxHp=saved.maxHp;e->reward=saved.reward;e->posX=saved.x;e->posY=saved.y;e->path=saved.path;e->pathIndex=saved.index;
            for(const auto& element:saved.elements) {
                if(s.version<5 && element.type=="Ice")continue;
                PokemonType type;stringToType(element.type,type);
                auto* state=createElementStatus(type,element.time,element.strength);state->tickAcc=element.tick;e->setElementStatus(state);
            }
            for(const auto& effect:saved.effects) {
                if(s.version<5 && effect.id=="FreezeEffect")continue;
                StatusEffect* state=nullptr;
                if(effect.id=="BurnEffect") {
                    auto* burn=new BurnEffect(effect.time,effect.magnitude);
                    burn->tickAcc=effect.tick;burn->damageAcc=effect.damage;state=burn;
                } else if(effect.id=="ParalyzeEffect") {
                    auto* paralysis=new ParalyzeEffect(effect.time,std::min(.5,effect.magnitude));
                    if(s.version>=5){paralysis->checkRemaining=effect.check;paralysis->stopRemaining=effect.stop;paralysis->randomState=static_cast<std::uint32_t>(effect.rng);}
                    state=paralysis;
                } else state=new SlowEffect(effect.time,s.version<5?std::min(.5,effect.magnitude):effect.magnitude);
                e->addStatusEffect(state);
            }
            enemies.push_back(std::move(e));
        }
        if(s.legacy&&s.wave>0)s.logs.push_back("旧版存档缺少敌人记录，本波已重置；点击继续开战");
        if(s.logs.size()>6)s.logs.erase(s.logs.begin(),s.logs.end()-6);
        game.map.getDefenders().reserve(units.size());game.map.getEnemies().reserve(enemies.size());
        game.reset();
        game.player.setGold(s.gold);game.player.setScore(s.score);game.map.getCore().setHp(s.core);
        auto& wave=game.waveManager;wave.currentWave=s.wave;wave.maxWave=s.maxWave;wave.waveActive=s.active;
        wave.spawnTimer=s.timer;wave.spawnInterval=s.interval;wave.toSpawn=s.total;wave.spawned=s.spawned;
        auto& stats=game.statistics;stats.kills=s.stats[0];stats.totalDamage=s.stats[1];stats.reactionsTriggered=s.stats[2];stats.goldEarned=s.stats[3];stats.wavesCleared=s.stats[4];stats.enemiesLeaked=s.stats[5];
        for(auto& p:units)game.map.addPokemon(p.release());
        for(auto& e:enemies)game.map.getEnemies().push_back(e.release());
        game.applyContextToDefenders();game.running=s.running;game.paused=s.paused;game.gameOver=s.over;game.victory=s.victory;
        game.evolutionCount=s.evolutions;game.lastEvolvePos=Position(s.evoX,s.evoY);game.log=std::move(s.logs);
        return true;
    } catch(const std::exception&) {return false;}
}
