#include "StatusEffect.h"
#include "Enemy.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
StatusEffect::StatusEffect(double time,double value):remainingTime(time),magnitude(value) {}
StatusEffect::~StatusEffect() {}
bool StatusEffect::isExpired() const { return remainingTime<=0; }
BurnEffect::BurnEffect(double time,double dps):StatusEffect(time,dps) {}
void BurnEffect::apply(Enemy&) {}
void BurnEffect::update(Enemy& enemy,double dt) {
    double active=std::min(std::max(0.0,dt),remainingTime);
    while(active>1e-9) {
        const double step=std::min(active,.5-tickAcc);
        active-=step; tickAcc+=step; damageAcc+=magnitude*step;
        if(tickAcc>=.5-1e-9) {
            const int damage=static_cast<int>(std::floor(damageAcc+1e-9));
            enemy.takeDamage(damage); damageAcc=std::max(0.0,damageAcc-damage); tickAcc=0;
        }
    }
    remainingTime=std::max(0.0,remainingTime-dt);
    if(isExpired() && damageAcc>=1-1e-9) {
        enemy.takeDamage(static_cast<int>(std::floor(damageAcc+1e-9))); damageAcc=0;
    }
}
ParalyzeEffect::ParalyzeEffect(double time,double chance)
    :StatusEffect(time,std::clamp(chance,0.0,.5)),randomState(static_cast<std::uint32_t>(std::rand())+1u) {}
void ParalyzeEffect::apply(Enemy&) {}
void ParalyzeEffect::update(Enemy&,double dt) {
    blocked=0;
    double active=std::min(std::max(0.0,dt),remainingTime);
    while(active>1e-9) {
        if(stopRemaining>1e-9) {
            const double step=std::min(active,stopRemaining);
            active-=step;stopRemaining-=step;blocked+=step;
        } else if(checkRemaining>1e-9) {
            const double step=std::min(active,checkRemaining);
            active-=step;checkRemaining-=step;
        } else {
            randomState=randomState*1664525u+1013904223u;
            if(double(randomState)/4294967296.0<magnitude)stopRemaining=.25;
            checkRemaining=.5;
        }
    }
    remainingTime=std::max(0.0,remainingTime-dt);
}
SlowEffect::SlowEffect(double time,double ratio):StatusEffect(time,ratio) {}
void SlowEffect::apply(Enemy&) {}
void SlowEffect::update(Enemy&,double dt) { remainingTime=std::max(0.0,remainingTime-dt); }
