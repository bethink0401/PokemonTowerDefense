#include "Position.h"
#include "Game.h"
#include <cassert>
#include <cmath>
#include <limits>
#include <iostream>

int main() {
    Position origin;
    assert(origin.isValid() && origin.getX() == 0 && origin.getY() == 0);
    for (int y = 0; y < Position::MAP_HEIGHT; ++y)
        for (int x = 0; x < Position::MAP_WIDTH; ++x)
            assert(Position(x, y).isValid());

    const int low = std::numeric_limits<int>::min();
    const int high = std::numeric_limits<int>::max();
    const Position invalid[] = {
        Position(-1, 0), Position(0, -1),
        Position(Position::MAP_WIDTH, 0), Position(0, Position::MAP_HEIGHT),
        Position(Position::MAP_WIDTH, Position::MAP_HEIGHT),
        Position(low, low), Position(high, high)
    };
    Game game;
    const int gold = game.getPlayer().getGold();
    const int count = game.getMap().getDefenderCount();
    for (const auto& p : invalid) {
        assert(!p.isValid());
        assert(!game.getMap().isTallGrass(p));
        assert(!game.getMap().isRoad(p));
        assert(!game.isDeployable(p));
        assert(!game.deployPokemon("Pichu", p));
        assert(game.getPlayer().getGold() == gold);
        assert(game.getMap().getDefenderCount() == count);
    }
    // Every off-map edge is rejected, even beside a deployable border tile.
    for (int x = 0; x < Position::MAP_WIDTH; ++x) {
        assert(!game.deployPokemon("Pichu", Position(x, -1)));
        assert(!game.deployPokemon("Pichu", Position(x, Position::MAP_HEIGHT)));
    }
    for (int y = 0; y < Position::MAP_HEIGHT; ++y) {
        assert(!game.deployPokemon("Pichu", Position(-1, y)));
        assert(!game.deployPokemon("Pichu", Position(Position::MAP_WIDTH, y)));
    }
    assert(game.getPlayer().getGold() == gold);
    assert(game.getMap().getDefenderCount() == count);

    Position changed(1, 1);
    changed.setX(-1);
    assert(changed.getX() == -1 && !changed.isValid());
    changed.setX(Position::MAP_WIDTH - 1);
    changed.setY(Position::MAP_HEIGHT);
    assert(!changed.isValid());
    changed.setY(Position::MAP_HEIGHT - 1);
    assert(changed.isValid());
    Position copied(changed);
    assert(copied == changed && copied.isValid());
    assert(std::abs(Position(0, 0).distanceTo(Position(3, 4)) - 5.0) < 1e-9);
    assert(std::isfinite(Position(low, low).distanceTo(Position(high, high))));

    bool deployed = false;
    for (int y = 0; y < Position::MAP_HEIGHT && !deployed; ++y)
        for (int x = 0; x < Position::MAP_WIDTH && !deployed; ++x)
            if (game.isDeployable(Position(x, y)))
                deployed = game.deployPokemon("Pichu", Position(x, y)) != nullptr;
    assert(deployed && game.getMap().getDefenderCount() == count + 1);
    std::cout << "PASS: map bounds, preserved invalid inputs, setters/copy, extreme distances, rejected edge deployment and valid deployment\n";
}
