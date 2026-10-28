#pragma once
#include "Pokemon.h"
#include "Enemy.h"
#include "GameMap.h"
#include <algorithm>
#include <cmath>
namespace UnitInsights {
inline double interval(const Pokemon& p) { return p.getAttackInterval(); }
inline double dps(const Pokemon& p) { return p.getAttackPower()/interval(p); }
inline double remaining(const Pokemon& p) { return p.getAttackCooldown(); }
inline int enemiesInRange(const Pokemon& p,const GameMap& map) {
 int count=0;for(auto* e:map.getEnemies())if(e&&!e->isDead()&&!e->hasReachedCore()&&p.inRange(e->getPosition()))++count;return count;
}
// A compatible teammate must cover at least one common point on a real road.
inline bool sharesRoad(const Pokemon& a,const Pokemon& b,const GameMap& map) {
 const auto pa=a.getPosition(),pb=b.getPosition();
 for(const auto& path:map.getPointPaths())for(const auto& point:path)
  if(std::hypot(pa.getX()-point.x,pa.getY()-point.y)<=a.getRange()&&std::hypot(pb.getX()-point.x,pb.getY()-point.y)<=b.getRange())return true;
 return false;
}
}
