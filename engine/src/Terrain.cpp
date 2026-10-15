#include "Terrain.h"
#include <algorithm>
#include <iostream>

namespace aoe2 {

Terrain::Terrain() {
    createMap(120, 120, TerrainType::Grass);
}

Terrain::~Terrain() = default;

void Terrain::loadTextures() {
    if (m_texturesLoaded) return;

    std::string basePath = "resources/_common/terrain/textures/";
    m_grassTex.loadFromFile(basePath + "g_grs_00_color.png");
    m_desertTex.loadFromFile(basePath + "g_des_00_color.png");
    m_forestTex.loadFromFile(basePath + "g_for_00_color.png");
    m_beachTex.loadFromFile(basePath + "g_bch_00_color.png");

    if (m_grassTex.isValid()) {
        std::cout << "[Terrain] Loaded authentic HD terrain textures ("
                  << m_grassTex.width << "x" << m_grassTex.height << ")" << std::endl;
        m_texturesLoaded = true;
    }
}

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

            // Generate realistic map features
            if (x < 8 || y < 8 || x > m_width - 8 || y > m_height - 8) {
                t.type = TerrainType::Forest;
            } else if ((x - 80) * (x - 80) + (y - 80) * (y - 80) < 144) {
                t.type = TerrainType::Water;
            } else if ((x - 80) * (x - 80) + (y - 80) * (y - 80) < 225) {
                t.type = TerrainType::Beach;
            } else if (x > 32 && x < 48 && y > 32 && y < 48) {
                t.type = TerrainType::Desert;
            } else if ((x > 12 && x < 18 && y > 12 && y < 18) ||
                       (x > 50 && x < 56 && y > 15 && y < 22)) {
                t.type = TerrainType::Forest;
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
    if (!m_texturesLoaded) {
        loadTextures();
    }

    int halfW = static_cast<int>((Isometric::TILE_WIDTH * zoom) / 2.0f);
    int totalH = static_cast<int>(Isometric::TILE_HEIGHT * zoom);
    if (halfW <= 0 || totalH <= 0) return;

    int screenW = renderer.getWidth();
    int screenH = renderer.getHeight();

    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            const Tile& t = m_tiles[y * m_width + x];
            Point2D pt = Isometric::worldToScreen(static_cast<float>(x), static_cast<float>(y), 
                                                  static_cast<float>(t.elevation), camX, camY, zoom);

            // Bounding box cull
            if (pt.x < -halfW * 2 || pt.x > screenW + halfW * 2 ||
                pt.y < -totalH || pt.y > screenH + totalH) {
                continue;
            }

            int px = static_cast<int>(pt.x);
            int py = static_cast<int>(pt.y);

            // Select authentic texture
            const Texture* tex = nullptr;
            if (t.type == TerrainType::Grass && m_grassTex.isValid()) {
                tex = &m_grassTex;
            } else if (t.type == TerrainType::Desert && m_desertTex.isValid()) {
                tex = &m_desertTex;
            } else if (t.type == TerrainType::Forest && m_forestTex.isValid()) {
                tex = &m_forestTex;
            } else if (t.type == TerrainType::Beach && m_beachTex.isValid()) {
                tex = &m_beachTex;
            }

            if (tex && tex->isValid()) {
                int tw = tex->width;
                int th = tex->height;

                for (int dy = 0; dy < totalH; ++dy) {
                    int dstY = py + dy;
                    if (dstY < 0 || dstY >= screenH) continue;

                    int rowWidth = (dy < totalH / 2) ? ((dy + 1) * halfW * 2) / (totalH / 2)
                                                     : ((totalH - dy) * halfW * 2) / (totalH / 2);
                    int startX = px + halfW - (rowWidth / 2);

                    int v = ((y * 24 + dy) % th + th) % th;
                    const uint32_t* texRow = &tex->pixels[v * tw];

                    for (int rx = 0; rx < rowWidth; ++rx) {
                        int dstX = startX + rx;
                        if (dstX < 0 || dstX >= screenW) continue;

                        int u = ((x * 48 + rx) % tw + tw) % tw;
                        uint32_t pixel = texRow[u];

                        // Subtle tile boundary darkening for classic RTS grid clarity
                        if (rx == 0 || rx == rowWidth - 1 || dy == 0 || dy == totalH - 1) {
                            uint8_t r = pixel & 0xFF;
                            uint8_t g = (pixel >> 8) & 0xFF;
                            uint8_t b = (pixel >> 16) & 0xFF;
                            pixel = 0xFF000000 | ((b * 3 / 4) << 16) | ((g * 3 / 4) << 8) | (r * 3 / 4);
                        }

                        renderer.drawRect(dstX, dstY, 1, 1, pixel & 0xFF, (pixel >> 8) & 0xFF, (pixel >> 16) & 0xFF, 255);
                    }
                }
            } else if (t.type == TerrainType::Water) {
                // Realistic animated water tile
                for (int dy = 0; dy < totalH; ++dy) {
                    int dstY = py + dy;
                    if (dstY < 0 || dstY >= screenH) continue;

                    int rowWidth = (dy < totalH / 2) ? ((dy + 1) * halfW * 2) / (totalH / 2)
                                                     : ((totalH - dy) * halfW * 2) / (totalH / 2);
                    int startX = px + halfW - (rowWidth / 2);
                    
                    uint8_t wave = static_cast<uint8_t>(25 + (x * 7 + y * 11 + dy * 3) % 20);
                    renderer.drawRect(startX, dstY, rowWidth, 1, 35, 75 + (wave / 2), 140 + wave, 255);
                }
            } else {
                // Fallback diamond tile
                uint8_t r = 85, g = 135, b = 45;
                if (t.type == TerrainType::Desert) { r = 195; g = 170; b = 105; }
                else if (t.type == TerrainType::Forest) { r = 30; g = 80; b = 30; }

                for (int dy = 0; dy < totalH; ++dy) {
                    int dstY = py + dy;
                    if (dstY < 0 || dstY >= screenH) continue;
                    int rowWidth = (dy < totalH / 2) ? ((dy + 1) * halfW * 2) / (totalH / 2)
                                                     : ((totalH - dy) * halfW * 2) / (totalH / 2);
                    int startX = px + halfW - (rowWidth / 2);
                    renderer.drawRect(startX, dstY, rowWidth, 1, r, g, b, 255);
                }
            }
        }
    }
}

} // namespace aoe2
