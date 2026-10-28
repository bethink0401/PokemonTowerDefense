#include "ElementStatus.h"

ElementStatus::ElementStatus(PokemonType t, double time, int strength)
    : type(t), remainingTime(time), strength(strength), tickAcc(0.0) {}

ElementStatus::~ElementStatus() {}

void ElementStatus::update(double dt) { remainingTime -= dt; }

bool ElementStatus::isExpired() const { return remainingTime <= 0.0; }

std::string ElementStatus::toSaveString() const {
    return typeToString(type);
}

// ---------- FireStatus：火元素标记 ----------
FireStatus::FireStatus(double time, int strength)
    : ElementStatus(PokemonType::Fire, time, strength) {}

// ---------- WaterStatus：水属性附着 ----------
WaterStatus::WaterStatus(double time, int strength)
    : ElementStatus(PokemonType::Water, time, strength) {}

// ---------- GrassStatus：草属性附着 ----------
GrassStatus::GrassStatus(double time, int strength)
    : ElementStatus(PokemonType::Grass, time, strength) {}

// ---------- ElectricStatus：电元素标记 ----------
ElectricStatus::ElectricStatus(double time, int strength)
    : ElementStatus(PokemonType::Electric, time, strength) {}

ElementStatus* createElementStatus(PokemonType type, double time, int strength) {
    switch (type) {
    case PokemonType::Fire:     return new FireStatus(time, strength);
    case PokemonType::Water:    return new WaterStatus(time, strength);
    case PokemonType::Grass:    return new GrassStatus(time, strength);
    case PokemonType::Electric: return new ElectricStatus(time, strength);
    }
    return new WaterStatus(time, strength);
}
