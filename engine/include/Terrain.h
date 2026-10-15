#pragma once

#include <vector>
#include <cstdint>
#include "Isometric.h"
#include "Renderer.h"

namespace aoe2 {

enum class TerrainType : uint8_t {
    Grass = 0,
    Forest = 1,
    Desert = 2,
    Water = 3,
    Farm = 4
};

struct Tile {
    TerrainType type{TerrainType::Grass};
    uint8_t elevation{0};
    uint8_t explored{0}; // Fog of war
    uint8_t visible{0};
};

class Terrain {
public:
    Terrain();
    ~Terrain();

    void createMap(int width, int height, TerrainType baseType = TerrainType::Grass);
    Tile* getTile(int x, int y);
    const Tile* getTile(int x, int y) const;

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

    void render(Renderer& renderer, float camX, float camY, float zoom = 1.0f);

private:
    int m_width{120};
    int m_height{120};
    std::vector<Tile> m_tiles;
};

} // namespace aoe2
