#include "Position.h"
#include <cmath>
#include <sstream>

Position::Position() : x(0), y(0) {}

Position::Position(int x, int y) : x(x), y(y) {}

Position::Position(const Position& other) : x(other.x), y(other.y) {}

int Position::getX() const { return x; }
int Position::getY() const { return y; }

void Position::setX(int x) { this->x = x; }
void Position::setY(int y) { this->y = y; }

bool Position::isValid() const {
    return x >= 0 && y >= 0 && x < MAP_WIDTH && y < MAP_HEIGHT;
}

double Position::distanceTo(const Position& other) const {
    double dx = static_cast<double>(x) - static_cast<double>(other.x);
    double dy = static_cast<double>(y) - static_cast<double>(other.y);
    return std::sqrt(dx * dx + dy * dy);
}

bool Position::operator==(const Position& other) const {
    return x == other.x && y == other.y;
}

bool Position::operator!=(const Position& other) const {
    return !(*this == other);
}

std::string Position::toString() const {
    std::ostringstream oss;
    oss << "(" << x << "," << y << ")";
    return oss.str();
}
