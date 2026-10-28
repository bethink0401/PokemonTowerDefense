#include "ForestMapData.h"
#include "Position.h"
#include <cstdio>
int main() {
    std::printf("MAP %dx%d\n", Position::MAP_WIDTH, Position::MAP_HEIGHT);
    const char* const* rows = ForestMap::terrainRows();
    for (int y = 0; y < 5; ++y) std::printf("row%d: %s\n", y, rows[y]);
    std::printf("row2 col7 = '%c' (expect G)\n", rows[2][7]);
    std::printf("pathCave size=%zu front=(%d,%d) back=(%d,%d)\n",
                ForestMap::pathCave().size(),
                ForestMap::pathCave().front().getX(), ForestMap::pathCave().front().getY(),
                ForestMap::pathCave().back().getX(),  ForestMap::pathCave().back().getY());
    return 0;
}
