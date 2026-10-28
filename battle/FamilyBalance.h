#pragma once
#include "TypeSystem.h"
struct FamilyGrowth { int attack, hp; double range, intervalFactor; };
inline FamilyGrowth familyGrowth(PokemonType type) {
    switch(type) {
    case PokemonType::Electric: return {2,12,.08,.94}; // 高频追击
    case PokemonType::Fire: return {5,20,.08,.985};    // 群伤
    case PokemonType::Water: return {3,30,.15,.98};    // 稳定控制
    case PokemonType::Grass: return {6,22,.22,.99};    // 远射程破甲
    }
    return {2,20,.1,.98};
}
