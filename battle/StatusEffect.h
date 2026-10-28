#pragma once
#include <string>
#include <cstdint>
class Enemy;
class StatusEffect {
protected:
    double remainingTime, magnitude;
public:
    StatusEffect(double time,double magnitude);
    virtual ~StatusEffect();
    virtual void apply(Enemy& enemy)=0;
    virtual void update(Enemy& enemy,double dt)=0;
    bool isExpired() const;
    double getRemainingTime() const { return remainingTime; }
    void setRemainingTime(double t) { remainingTime=t; }
    double getMagnitude() const { return magnitude; }
    void strengthen(double m) { if(m>magnitude)magnitude=m; }
    virtual bool isSlow() const { return false; }
    virtual bool isImmobilize() const { return false; }
    virtual double blockedDuration() const { return 0; }
    virtual std::string getClassName() const=0;
};
class BurnEffect : public StatusEffect {
    friend class SaveManager;
    double tickAcc=0, damageAcc=0;
public:
    BurnEffect(double time,double dps);
    void apply(Enemy&) override;
    void update(Enemy&,double dt) override;
    std::string getClassName() const override { return "BurnEffect"; }
};
class ParalyzeEffect : public StatusEffect {
    friend class SaveManager;
    double checkRemaining=.5, stopRemaining=0, blocked=0;
    std::uint32_t randomState;
public:
    // 每次恢复移动至少 .5 秒，之后按概率停步 .25 秒。
    ParalyzeEffect(double time,double chance);
    void apply(Enemy&) override;
    void update(Enemy&,double dt) override;
    bool isImmobilize() const override { return stopRemaining>0; }
    double blockedDuration() const override { return blocked; }
    std::string getClassName() const override { return "ParalyzeEffect"; }
};
class SlowEffect : public StatusEffect {
public:
    SlowEffect(double time,double slowRatio);
    void apply(Enemy&) override;
    void update(Enemy&,double dt) override;
    bool isSlow() const override { return true; }
    std::string getClassName() const override { return "SlowEffect"; }
};
