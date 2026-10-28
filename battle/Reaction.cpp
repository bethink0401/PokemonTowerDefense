#include "Reaction.h"
#include "Enemy.h"
#include "StatusEffect.h"

// 辅助：判断无序属性对是否匹配
static bool pairMatch(PokemonType o, PokemonType n, PokemonType a, PokemonType b) {
    return (o == a && n == b) || (o == b && n == a);
}



// ---------------- 电导链：水 + 电 ----------------
bool ConductReaction::canTrigger(PokemonType o, PokemonType n) const {
    return pairMatch(o, n, PokemonType::Water, PokemonType::Electric);
}
void ConductReaction::execute(Enemy& enemy) {
    enemy.takeDamage(10);
    enemy.addStatusEffect(new ParalyzeEffect(3.0, 0.35));
}

// ---------------- 燃烧：火 + 草 ----------------
bool BurnReaction::canTrigger(PokemonType o, PokemonType n) const {
    return pairMatch(o, n, PokemonType::Fire, PokemonType::Grass);
}
void BurnReaction::execute(Enemy& enemy) {
    enemy.addStatusEffect(new BurnEffect(3.0, 4.0));
}

// ---------------- 缠绕生长：水 + 草 ----------------
bool GrowthReaction::canTrigger(PokemonType o, PokemonType n) const {
    return pairMatch(o, n, PokemonType::Water, PokemonType::Grass);
}
void GrowthReaction::execute(Enemy& enemy) {
    enemy.addStatusEffect(new SlowEffect(2.5, 0.4));
}

// ---------------- 蒸汽爆发：火 + 水 ----------------
bool SteamReaction::canTrigger(PokemonType o, PokemonType n) const {
    return pairMatch(o, n, PokemonType::Fire, PokemonType::Water);
}
void SteamReaction::execute(Enemy& enemy) {
    enemy.takeDamage(8);
    enemy.addStatusEffect(new SlowEffect(1.5, 0.2));   // 高温蒸汽遮蔽视野，减速
    // 范围伤害由 ReactionEngine 依据 isArea()/getAreaDamage() 处理
}


// ---------------- 麻痹孢子：电 + 草 ----------------
bool SporeReaction::canTrigger(PokemonType o, PokemonType n) const {
    return pairMatch(o, n, PokemonType::Electric, PokemonType::Grass);
}
void SporeReaction::execute(Enemy& enemy) {
    enemy.addStatusEffect(new ParalyzeEffect(2.5, 0.5)); // 目标随机停顿
    // 范围伤害由 ReactionEngine 处理
}

// ---------------- 能量爆破：火 + 电 ----------------
bool ExplosionReaction::canTrigger(PokemonType o, PokemonType n) const {
    return pairMatch(o, n, PokemonType::Fire, PokemonType::Electric);
}
void ExplosionReaction::execute(Enemy& enemy) {
    enemy.takeDamage(12);
    // 范围溅射由 ReactionEngine 依据 isArea()/getAreaDamage() 处理
}
