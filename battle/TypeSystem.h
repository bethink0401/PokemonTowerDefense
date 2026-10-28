#pragma once
// =============================================================
//  TypeSystem.h
//  固有属性枚举 + 属性克制计算
//  OOP 知识点：枚举、static 成员函数、数据与行为分离
//  注意：PokemonType 是"固有属性"，与战斗中的临时附着 ElementStatus 不同
// =============================================================
#include <string>

enum class PokemonType {
    Fire,
    Water,
    Grass,
    Electric
};

std::string typeToString(PokemonType t);
std::string typeDisplayName(PokemonType t);
bool stringToType(const std::string& s, PokemonType& out);

class TypeSystem {
public:
    // 克制 1.5 / 普通 1.0 / 被克制 0.75
    static double getMultiplier(PokemonType attacker, PokemonType defender);
    static std::string describeMultiplier(double m);
};
