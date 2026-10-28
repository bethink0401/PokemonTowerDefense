#include "TypeSystem.h"

std::string typeToString(PokemonType t) {
    switch (t) {
    case PokemonType::Fire:     return "Fire";
    case PokemonType::Water:    return "Water";
    case PokemonType::Grass:    return "Grass";
    case PokemonType::Electric: return "Electric";
    }
    return "Unknown";
}

std::string typeDisplayName(PokemonType t) {
    switch (t) {
    case PokemonType::Fire: return "火";
    case PokemonType::Water: return "水";
    case PokemonType::Grass: return "草";
    case PokemonType::Electric: return "电";
    }
    return "未知";
}

bool stringToType(const std::string& s, PokemonType& out) {
    if (s == "Fire")          { out = PokemonType::Fire;     return true; }
    if (s == "Water")         { out = PokemonType::Water;    return true; }
    if (s == "Grass")         { out = PokemonType::Grass;    return true; }
    if (s == "Electric")      { out = PokemonType::Electric; return true; }
    return false;
}

// 只做固有属性的克制关系（数据表，非行为分派，用条件判断是合理的）
static bool isStrongAgainst(PokemonType a, PokemonType d) {
    if (a == PokemonType::Fire     && d == PokemonType::Grass)    return true;
    if (a == PokemonType::Water    && d == PokemonType::Fire)     return true;
    if (a == PokemonType::Grass    && d == PokemonType::Water)    return true;
    if (a == PokemonType::Electric && d == PokemonType::Water)    return true;
    return false;
}

double TypeSystem::getMultiplier(PokemonType attacker, PokemonType defender) {
    if (attacker == defender) return 1.0;
    if (isStrongAgainst(attacker, defender)) return 1.5;
    if (isStrongAgainst(defender, attacker)) return 0.75;
    return 1.0;
}

std::string TypeSystem::describeMultiplier(double m) {
    if (m > 1.0) return "效果拔群";
    if (m < 1.0) return "效果不佳";
    return "效果一般";
}
