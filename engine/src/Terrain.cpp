#include "Terrain.h"
#include <algorithm>

namespace aoe2 {

Terrain::Terrain() {
    createMap(120, 120, TerrainType::Grass);
}

Terrain::~Terrain() = default;

void Terrain::createMap(int width, int height, TerrainType baseType) {
    m_width = width;
    m_height = height;
    m_tiles.resize(static_cast<size_t>(m_width) * m_height);

    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            Tile& t = m_tiles[y * m_width + x];
            t.type = baseType;
            t.elevation = 0;
            t.explored = 1;
            t.visible = 1;

            // Generate map features (forest clumps, water body)
            if (x < 10 || y < 10 || x > m_width - 10 || y > m_height - 10) {
                t.type = TerrainType::Forest;
            } else if ((x - 80) * (x - 80) + (y - 80) * (y - 80) < 144) {
                t.type = TerrainType::Water;
            } else if (x > 30 && x < 45 && y > 30 && y < 45) {
                t.type = TerrainType::Desert;
            }
        }
    }
}

Tile* Terrain::getTile(int x, int y) {
    if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
        return &m_tiles[y * m_width + x];
    }
    return nullptr;
}

const Tile* Terrain::getTile(int x, int y) const {
    if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
        return &m_tiles[y * m_width + x];
    }
    return nullptr;
}

void Terrain::render(Renderer& renderer, float camX, float camY, float zoom) {
    // Determine visible bounding tile box from screen viewport
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            const Tile& t = m_tiles[y * m_width + x];
            Point2D pt = Isometric::worldToScreen(static_cast<float>(x), static_cast<float>(y), 
                                                  static_cast<float>(t.elevation), camX, camY, zoom);

            // Bounding box cull
            if (pt.x < -Isometric::TILE_WIDTH || pt.x > renderer.getWidth() + Isometric::TILE_WIDTH ||
                pt.y < -Isometric::TILE_HEIGHT || pt.y > renderer.getHeight() + Isometric::TILE_HEIGHT) {
                continue;
            }

            // Draw diamond isometric tile
            uint8_t r = 85, g = 135, b = 45; // Grass
            if (t.type == TerrainType::Desert) {
                r = 195; g = 170; b = 105;
            } else if (t.type == TerrainType::Water) {
                r = 40; g = 90; b = 160;
            } else if (t.type == TerrainType::Forest) {
                r = 30; g = 80; b = 30;
            }

            int halfW = static_cast<int>((Isometric::TILE_WIDTH * zoom) / 2.0f);
            int totalH = static_cast<int>(Isometric::TILE_HEIGHT * zoom);
            int px = static_cast<int>(pt.x);
            int py = static_cast<int>(pt.y);

            // True diamond isometric rasterization
            for (int dy = 0; dy < totalH; ++dy) {
                int rowWidth = (dy < totalH / 2) ? ((dy + 1) * halfW * 2) / (totalH / 2)
                                                 : ((totalH - dy) * halfW * 2) / (totalH / 2);
                int startX = px + halfW - (rowWidth / 2);
                renderer.drawRect(startX, py + dy, rowWidth, 1, r, g, b, 255);
            }

            // Subtle diamond tile border
            uint8_t br = static_cast<uint8_t>(std::max(0, r - 16));
            uint8_t bg = static_cast<uint8_t>(std::max(0, g - 22));
            uint8_t bb = static_cast<uint8_t>(std::max(0, b - 12));
            renderer.drawRect(px + halfW, py, 1, 1, br, bg, bb, 255);
            renderer.drawRect(px + halfW, py + totalH - 1, 1, 1, br, bg, bb, 255);
            renderer.drawRect(px, py + totalH / 2, 1, 1, br, bg, bb, 255);
            renderer.drawRect(px + halfW * 2 - 1, py + totalH / 2, 1, 1, br, bg, bb, 255);
        }
    }
}

} // namespace aoe2
