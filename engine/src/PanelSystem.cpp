#include "PanelSystem.h"
#include "Isometric.h"
#include "Engine.h"
#include "ScreenStack.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>

namespace aoe2 {

PanelSystem::PanelSystem() {
    // Pre-populate standard 3x5 Genie Engine command matrix actions
    // Icons correspond to authentic frames in 50721.slp (btncmd.shp)
    m_commandGrid = {
        // Row 0: Military / Movement Commands
        { 0, 0,  "Attack",       "Command selected units to attack", true, true },
        { 1, 1,  "Move",         "Order units to move or set stance", true, true },
        { 2, 2,  "Stop",         "Halt current unit action", true, true },
        { 3, 3,  "Patrol",       "Patrol between waypoints", true, true },
        { 4, 4,  "Guard",        "Guard target entity or unit", true, true },

        // Row 1: Tactical & Formation Actions
        { 5, 5,  "Garrison",     "Garrison inside building or transport", true, true },
        { 6, 6,  "Delete",       "Disband or destroy entity", true, true },
        { 7, 7,  "Repair",       "Repair target building or siege unit", true, true },
        { 8, 8,  "Line",         "Formation: Line layout", true, true },
        { 9, 9,  "Staggered",    "Formation: Staggered spread layout", true, true },

        // Row 2: Construction / Stance Actions
        { 10, 12, "Economic",    "Build economic buildings", true, true },
        { 11, 13, "Military",    "Build military buildings", true, true },
        { 12, 14, "Aggressive",  "Stance: Attack on sight", true, true },
        { 13, 15, "Defensive",   "Stance: Defend nearby perimeter", true, true },
        { 14, 16, "No Attack",   "Stance: Do not retaliate", true, true }
    };

    onResize(m_width, m_height);
}

PanelSystem::~PanelSystem() = default;

void PanelSystem::loadTextures() {
    if (m_hudTexturesLoaded) return;

    std::string basePath = "resources/_common/textures/ui/hud/";
    m_texCmdFrame.loadFromFile(basePath + "hud_cmd_frame.png");
    m_texCenterSlice.loadFromFile(basePath + "hud_center_slice.png");
    m_texMinimapFrame.loadFromFile(basePath + "hud_minimap_frame.png");
    m_texStatusPlaque.loadFromFile(basePath + "hud_status_plaque.png");
    m_texTopResource.loadFromFile(basePath + "hud_top_resource.png");
    m_texMenuPlaque.loadFromFile(basePath + "hud_menu_plaque.png");

    if (m_texCmdFrame.isValid()) {
        std::cout << "[PanelSystem] Loaded authentic HD HUD textures" << std::endl;
        m_hudTexturesLoaded = true;
    }
}

void PanelSystem::onResize(int width, int height) {
    m_width = width;
    m_height = height;

    const int hudH = 175;
    int hudY = m_height - hudH;

    // Minimap frame on bottom right
    m_miniW = 391;
    m_miniH = 175;
    m_miniX = m_width - m_miniW;
    m_miniY = hudY;

    // Command matrix inside left frame
    m_cmdX = 35;
    m_cmdY = hudY + 22;

    // Center selection details box
    m_selectX = 335;
    m_selectY = hudY + 12;
    m_selectW = std::max(200, m_miniX - m_selectX - 10);
    m_selectH = 150;
}

void PanelSystem::drawLine(Renderer& renderer, int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b) {
    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        if (x0 >= 0 && x0 < m_width && y0 >= 0 && y0 < m_height) {
            renderer.drawRect(x0, y0, 1, 1, r, g, b, 255);
        }
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void PanelSystem::render(Renderer& renderer, DRSReader& drs, const Simulation& sim,
                         float camX, float camY, float zoom) {
    if (!m_hudTexturesLoaded) {
        loadTextures();
    }

    // 1. Bottom HUD Console Background (authentic HD stone & parchment graphics)
    renderBottomConsole(renderer, drs);

    // 2. Authentic Minimap
    renderMinimap(renderer, drs, sim, camX, camY, zoom);

    // 3. Selected Unit / Building Details
    renderSelectionBox(renderer, drs, sim);

    // 4. Command Matrix (3x5 button grid)
    renderCommandMatrix(renderer, drs, sim);

    // 5. Top Resource Bar & DRS Resource Icons
    renderResourceBar(renderer, drs, sim);
}

void PanelSystem::renderResourceBar(Renderer& renderer, DRSReader& drs, const Simulation& sim) {
    // 1. Blit authentic carved top resource bar
    if (m_texTopResource.isValid()) {
        renderer.blitTexture(m_texTopResource, 0, 0);
    } else {
        renderer.drawRect(0, 0, m_width, 25, 26, 18, 12, 245);
    }

    // 2. Top-right Menu plaque
    if (m_texMenuPlaque.isValid()) {
        int menuX = m_width - m_texMenuPlaque.width;
        renderer.blitTexture(m_texMenuPlaque, menuX, 2);
        renderer.drawText("Menu", menuX + 60, 9, 0xFFFFFFFF);
    } else {
        int menuBtnX = m_width - 84;
        renderer.drawRect(menuBtnX, 4, 70, 20, 54, 40, 26, 255);
        renderer.drawText("Menu", menuBtnX + 18, 9, 0xFFFFFFFF);
    }

    // 3. Resource values
    const auto& econ = sim.getPlayerEconomy(0);

    struct ResourceItem {
        int value;
        int maxVal;
        bool isPop;
    };

    ResourceItem items[5] = {
        { econ.wood, 0, false },
        { econ.food, 0, false },
        { econ.gold, 0, false },
        { econ.stone, 0, false },
        { econ.population, econ.maxPopulation, true }
    };

    int startX = 28;
    for (int i = 0; i < 5; ++i) {
        int textX = startX + i * 65;
        std::string valStr = items[i].isPop ? 
            (std::to_string(items[i].value) + "/" + std::to_string(items[i].maxVal)) :
            std::to_string(items[i].value);

        renderer.drawText(valStr, textX, 8, 0xFFFFFFFF);
    }

    // Current Age Indicator in authentic gold Trajan
    static const char* ageNames[4] = { "Dark Age", "Feudal Age", "Castle Age", "Imperial Age" };
    int ageIdx = std::clamp(econ.currentAge, 0, 3);
    std::string ageStr = ageNames[ageIdx];
    int ageX = (m_width / 2) - 35;
    renderer.drawText(ageStr, ageX, 8, 0xFFE6C875);
}

void PanelSystem::renderBottomConsole(Renderer& renderer, DRSReader& drs) {
    const int hudH = 175;
    int hudY = m_height - hudH;

    // 1. Center parchment slice tiled horizontally from x=325 to rightEdge
    int rightEdge = m_width - 391;
    if (m_texCenterSlice.isValid()) {
        int x = 325;
        while (x < rightEdge) {
            renderer.blitTexture(m_texCenterSlice, x, hudY);
            x += m_texCenterSlice.width;
        }
    } else {
        renderer.drawRect(325, hudY, rightEdge - 325, hudH, 204, 182, 142, 255);
    }

    // 2. Left Command Matrix stone frame
    if (m_texCmdFrame.isValid()) {
        renderer.blitTexture(m_texCmdFrame, 0, hudY);
    }

    // 3. Right Minimap frame
    if (m_texMinimapFrame.isValid()) {
        renderer.blitTexture(m_texMinimapFrame, m_width - 391, hudY);
    }
}

void PanelSystem::renderMinimap(Renderer& renderer, DRSReader& drs, const Simulation& sim,
                               float camX, float camY, float zoom) {
    auto terrain = sim.getTerrain();
    int mapW = terrain ? terrain->getWidth() : 120;
    int mapH = terrain ? terrain->getHeight() : 120;

    // Center of minimap diamond cutout inside hud_minimap_frame.png
    int cx = m_miniX + 248;
    int cy = m_miniY + 86;

    float stepX = (210.0f * 0.5f) / mapW;
    float stepY = (105.0f * 0.5f) / mapH;

    // 1. Render Explored Terrain tiles onto Minimap
    if (terrain) {
        for (int ty = 0; ty < mapH; ++ty) {
            for (int tx = 0; tx < mapW; ++tx) {
                const auto* tile = terrain->getTile(tx, ty);
                if (!tile || tile->explored == 0) continue;

                int px = static_cast<int>(cx + (tx - ty) * stepX);
                int py = static_cast<int>(cy + (tx + ty) * stepY * 0.5f);

                if (px < m_miniX + 130 || px >= m_miniX + 370 ||
                    py < m_miniY + 25 || py >= m_miniY + 155) {
                    continue;
                }

                uint8_t r = 34, g = 110, b = 30; // Grass
                switch (tile->type) {
                    case TerrainType::Forest: r = 14;  g = 58;  b = 14;  break;
                    case TerrainType::Water:  r = 26;  g = 72;  b = 145; break;
                    case TerrainType::Desert: r = 182; g = 156; b = 92;  break;
                    case TerrainType::Beach:  r = 196; g = 180; b = 110; break;
                    case TerrainType::Farm:   r = 146; g = 122; b = 42;  break;
                    default: break;
                }

                // Darken tiles in Fog of War (explored but not currently visible)
                if (tile->visible == 0) {
                    r /= 2; g /= 2; b /= 2;
                }

                renderer.drawRect(px, py, 2, 1, r, g, b, 255);
            }
        }
    }

    // 2. Render Unit dots on Minimap
    for (const auto& u : sim.getAllUnits()) {
        if (!u || u->isDead()) continue;

        // Enemy units only visible if tile is currently visible
        if (u->getPlayerId() != 1 && terrain) {
            const auto* tile = terrain->getTile(static_cast<int>(u->getX()), static_cast<int>(u->getY()));
            if (!tile || tile->visible == 0) continue;
        }

        int upx = static_cast<int>(cx + (u->getX() - u->getY()) * stepX);
        int upy = static_cast<int>(cy + (u->getX() + u->getY()) * stepY * 0.5f);

        if (upx >= m_miniX + 130 && upx < m_miniX + 370 &&
            upy >= m_miniY + 25 && upy < m_miniY + 155) {
            
            uint8_t ur = 80, ug = 150, ub = 255; // Player 1 Blue
            if (u->getPlayerId() == 2) {
                ur = 230; ug = 40; ub = 40;     // Player 2 Red
            }

            int dotSize = (u->getTypeId() == 109) ? 4 : 2;
            renderer.drawRect(upx - dotSize / 2, upy - dotSize / 2, dotSize, dotSize, ur, ug, ub, 255);
        }
    }

    // 3. Camera Viewport Wireframe
    float camTileX = (camY / (Isometric::TILE_HEIGHT * zoom)) + (camX / (Isometric::TILE_WIDTH * zoom));
    float camTileY = (camY / (Isometric::TILE_HEIGHT * zoom)) - (camX / (Isometric::TILE_WIDTH * zoom));

    int vpx = static_cast<int>(cx + (camTileX - camTileY) * stepX);
    int vpy = static_cast<int>(cy + (camTileX + camTileY) * stepY * 0.5f);

    int boxW = static_cast<int>((m_width / (Isometric::TILE_WIDTH * zoom)) * stepX * 1.4f);
    int boxH = static_cast<int>((m_height / (Isometric::TILE_HEIGHT * zoom)) * stepY * 0.7f);

    drawLine(renderer, vpx - boxW, vpy, vpx, vpy - boxH, 255, 255, 255);
    drawLine(renderer, vpx, vpy - boxH, vpx + boxW, vpy, 255, 255, 255);
    drawLine(renderer, vpx + boxW, vpy, vpx, vpy + boxH, 255, 255, 255);
    drawLine(renderer, vpx, vpy + boxH, vpx - boxW, vpy, 255, 255, 255);
}

void PanelSystem::renderSelectionBox(Renderer& renderer, DRSReader& drs, const Simulation& sim) {
    const auto& selected = sim.getSelectedUnits();
    if (selected.empty()) {
        renderer.drawText("No selection", m_selectX + 16, m_selectY + 40, 0xFF4A3520);
        return;
    }

    auto unit = selected[0];
    if (!unit || unit->isDead()) return;

    // 1. Draw Authentic Unit Portrait Icon from DRS 50729.slp / 50730.slp
    auto iconShape = drs.getShape(50729);
    if (!iconShape) iconShape = drs.getShape(50730);

    int portraitX = m_selectX + 16;
    int portraitY = m_selectY + 16;
    int iconW = 60;
    int iconH = 60;

    int frameIdx = 0;
    if (unit->getTypeId() == 109) frameIdx = 109 % 40;
    else if (unit->getTypeId() == 83) frameIdx = 15;
    else if (unit->getTypeId() == 74) frameIdx = 0;
    else if (unit->getTypeId() == 4) frameIdx = 4;

    bool drewIcon = false;
    if (iconShape && frameIdx < iconShape->getFrameCount()) {
        const auto* frame = iconShape->getFrame(frameIdx);
        if (frame && !frame->pixelData.empty()) {
            renderer.blitSLPFrame(frame->pixelData.data(), frame->width, frame->height, portraitX, portraitY);
            drewIcon = true;
        }
    }

    if (!drewIcon) {
        renderer.drawRect(portraitX, portraitY, iconW, iconH, 30, 45, 60, 255);
    }

    // Gold icon border
    renderer.drawRect(portraitX - 1, portraitY - 1, iconW + 2, 1, 160, 130, 70, 255);
    renderer.drawRect(portraitX - 1, portraitY + iconH, iconW + 2, 1, 160, 130, 70, 255);
    renderer.drawRect(portraitX - 1, portraitY, 1, iconH, 160, 130, 70, 255);
    renderer.drawRect(portraitX + iconW, portraitY, 1, iconH, 160, 130, 70, 255);

    // 2. Unit Name & Ownership Text
    int textX = portraitX + iconW + 18;
    std::string unitName = (unit->getTypeId() == 109) ? "Town Center" :
                           (unit->getTypeId() == 83)  ? "Villager" :
                           (unit->getTypeId() == 74)  ? "Militia" :
                           (unit->getTypeId() == 4)   ? "Archer" : "Unit";

    renderer.drawText(unitName, textX, portraitY + 2, 0xFF2A1C10);

    std::string playerStr = (unit->getPlayerId() == 1) ? "Player 1 (You)" : "Player 2 (Enemy)";
    uint32_t playerColor = (unit->getPlayerId() == 1) ? 0xFF1E5BB5 : 0xFFB51E1E;
    renderer.drawText(playerStr, textX, portraitY + 20, playerColor);

    // 3. Health Bar Gauge
    int barX = textX;
    int barY = portraitY + 40;
    int barW = 140;
    int barH = 12;

    float hpRatio = std::clamp(unit->getHp() / unit->getMaxHp(), 0.0f, 1.0f);
    int fillW = static_cast<int>(barW * hpRatio);

    renderer.drawRect(barX, barY, barW, barH, 20, 15, 10, 255);
    renderer.drawRect(barX, barY, barW, 1, 80, 60, 30, 255);

    uint8_t hr = (hpRatio > 0.5f) ? static_cast<uint8_t>((1.0f - hpRatio) * 2.0f * 200) : 220;
    uint8_t hg = (hpRatio > 0.5f) ? 200 : static_cast<uint8_t>(hpRatio * 2.0f * 200);
    renderer.drawRect(barX + 1, barY + 1, fillW - 2, barH - 2, hr, hg, 30, 255);

    std::string hpStr = std::to_string(static_cast<int>(unit->getHp())) + "/" +
                        std::to_string(static_cast<int>(unit->getMaxHp()));
    renderer.drawText(hpStr, barX + 35, barY + 1, 0xFFFFFFFF);

    // 4. Combat Stats (Attack, Armor, Range)
    int statY = barY + 20;
    if (unit->getTypeId() == 109) {
        renderer.drawText("Garrison: 0/15", barX, statY, 0xFF4A3520);
        renderer.drawText("Attack: 5   Range: 6", barX, statY + 16, 0xFF4A3520);
    } else {
        std::string statStr = "Attack: " + std::to_string(static_cast<int>(unit->getType().attacks.empty() ? 4 : unit->getType().attacks[0].amount)) +
                              "  Armor: 0/0  Range: " + std::to_string(static_cast<int>(unit->getType().maxRange));
        renderer.drawText(statStr, barX, statY, 0xFF4A3520);
    }
}

void PanelSystem::renderCommandMatrix(Renderer& renderer, DRSReader& drs, const Simulation& sim) {
    auto cmdShape = drs.getShape(50721); // btncmd.shp

    const auto& selected = sim.getSelectedUnits();
    bool isTownCenter = (!selected.empty() && selected[0]->getTypeId() == 109);
    bool isVillager = (!selected.empty() && selected[0]->getTypeId() == 83);
    bool isBarracks = (!selected.empty() && selected[0]->getTypeId() == 12 && !selected[0]->isFoundation());

    for (int r = 0; r < m_cmdRows; ++r) {
        for (int c = 0; c < m_cmdCols; ++c) {
            int idx = r * m_cmdCols + c;
            int bx = m_cmdX + c * (m_btnSize + m_btnSpacing);
            int by = m_cmdY + r * (m_btnSize + m_btnSpacing);

            if (isTownCenter) {
                // Town Center Command Matrix: Button 0 = Train Villager
                if (idx == 0) {
                    renderer.drawRect(bx, by, m_btnSize, m_btnSize, 35, 45, 55, 255);
                    renderer.drawRect(bx, by, m_btnSize, 1, 192, 150, 48, 255);
                    renderer.drawRect(bx, by + m_btnSize - 1, m_btnSize, 1, 80, 60, 20, 255);
                    renderer.drawRect(bx, by, 1, m_btnSize, 192, 150, 48, 255);
                    renderer.drawRect(bx + m_btnSize - 1, by, 1, m_btnSize, 80, 60, 20, 255);

                    // Train Villager icon (Portrait frame 15)
                    auto iconShape = drs.getShape(50729);
                    if (iconShape && iconShape->getFrameCount() > 15) {
                        const auto* f = iconShape->getFrame(15);
                        if (f && !f->pixelData.empty()) {
                            renderer.blitSLPFrame(f->pixelData.data(), f->width, f->height, bx + 2, by + 2);
                        }
                    } else {
                        renderer.drawText("VIL", bx + 10, by + 14, 0xFFFFFFFF);
                    }
                }
                continue;
            }

            if (isBarracks) {
                // Barracks Command Matrix: Button 0 = Train Militia
                if (idx == 0) {
                    renderer.drawRect(bx, by, m_btnSize, m_btnSize, 45, 35, 30, 255);
                    renderer.drawRect(bx, by, m_btnSize, 1, 192, 150, 48, 255);
                    renderer.drawRect(bx, by + m_btnSize - 1, m_btnSize, 1, 80, 60, 20, 255);
                    renderer.drawRect(bx, by, 1, m_btnSize, 192, 150, 48, 255);
                    renderer.drawRect(bx + m_btnSize - 1, by, 1, m_btnSize, 80, 60, 20, 255);

                    // Authentic Militia icon (btncmd frame 17)
                    if (cmdShape && cmdShape->getFrameCount() > 17) {
                        const auto* f = cmdShape->getFrame(17);
                        if (f && !f->pixelData.empty()) {
                            renderer.blitSLPFrame(f->pixelData.data(), f->width, f->height, bx + 2, by + 2);
                        }
                    } else {
                        renderer.drawText("MIL", bx + 10, by + 14, 0xFFFFFFFF);
                    }
                }
                continue;
            }

            if (isVillager) {
                // Villager Construction Matrix:
                // Slot 0: House (Wood 25), icon 13
                // Slot 1: Barracks (Wood 175), icon 8
                // Slot 2: Mill (Wood 100), icon 14
                if (idx < 3) {
                    renderer.drawRect(bx, by, m_btnSize, m_btnSize, 30, 38, 25, 255);
                    renderer.drawRect(bx, by, m_btnSize, 1, 170, 140, 50, 255);
                    renderer.drawRect(bx, by + m_btnSize - 1, m_btnSize, 1, 60, 50, 20, 255);
                    renderer.drawRect(bx, by, 1, m_btnSize, 170, 140, 50, 255);
                    renderer.drawRect(bx + m_btnSize - 1, by, 1, m_btnSize, 60, 50, 20, 255);

                    int frameNo = (idx == 0) ? 13 : ((idx == 1) ? 8 : 14);
                    if (cmdShape && frameNo < cmdShape->getFrameCount()) {
                        const auto* f = cmdShape->getFrame(frameNo);
                        if (f && !f->pixelData.empty()) {
                            renderer.blitSLPFrame(f->pixelData.data(), f->width, f->height, bx + 2, by + 2);
                        }
                    } else {
                        const char* labels[] = {"HOU", "BAR", "MIL"};
                        renderer.drawText(labels[idx], bx + 8, by + 14, 0xFFFFFFFF);
                    }
                    continue;
                }
            }

            if (idx >= static_cast<int>(m_commandGrid.size())) continue;
            const auto& btn = m_commandGrid[idx];
            if (!btn.isVisible) continue;

            // Draw button bevel frame
            renderer.drawRect(bx, by, m_btnSize, m_btnSize, 30, 24, 18, 255);
            renderer.drawRect(bx, by, m_btnSize, 1, 140, 110, 40, 255);
            renderer.drawRect(bx, by + m_btnSize - 1, m_btnSize, 1, 40, 30, 15, 255);
            renderer.drawRect(bx, by, 1, m_btnSize, 140, 110, 40, 255);
            renderer.drawRect(bx + m_btnSize - 1, by, 1, m_btnSize, 40, 30, 15, 255);

            // Draw authentic icon frame from 50721.slp
            bool drewIcon = false;
            if (cmdShape && btn.iconFrame < cmdShape->getFrameCount()) {
                const auto* frame = cmdShape->getFrame(btn.iconFrame);
                if (frame && !frame->pixelData.empty()) {
                    renderer.blitSLPFrame(frame->pixelData.data(), frame->width, frame->height, bx + 2, by + 2);
                    drewIcon = true;
                }
            }

            if (!drewIcon) {
                renderer.drawRect(bx + 4, by + 4, m_btnSize - 8, m_btnSize - 8, 55, 45, 35, 255);
            }
        }
    }
}

bool PanelSystem::handleMouseDown(int button, int x, int y, Simulation& sim) {
    // 1. Clicked on top resource bar
    if (y < 26) {
        return true;
    }

    // 2. Clicked on bottom HUD console
    const int hudH = 175;
    int hudY = m_height - hudH;
    if (y >= hudY) {
        // Minimap click navigation & orders (Right Frame)
        if (x >= m_miniX + 130 && x <= m_miniX + 370 && y >= m_miniY + 25 && y <= m_miniY + 155) {
            auto terrain = sim.getTerrain();
            int mapW = terrain ? terrain->getWidth() : 120;
            int mapH = terrain ? terrain->getHeight() : 120;

            int cx = m_miniX + 248;
            int cy = m_miniY + 86;
            float stepX = (210.0f * 0.5f) / mapW;
            float stepY = (105.0f * 0.5f) / mapH;

            float relX = (x - cx) / stepX;
            float relY = (y - cy) / (stepY * 0.5f);

            float worldX = (relY + relX) * 0.5f;
            float worldY = (relY - relX) * 0.5f;

            if (button == 0) {
                // Center camera smoothly on clicked minimap coordinate
                if (g_engine && g_engine->getScreens()) {
                    g_engine->getScreens()->centerCameraOn(worldX, worldY);
                }
            } else if (button == 2) {
                sim.issueMoveOrder(worldX, worldY);
            }
            return true;
        }

        // Command button click dispatch (Left Frame)
        if (x >= m_cmdX && x < m_cmdX + m_cmdCols * (m_btnSize + m_btnSpacing) &&
            y >= m_cmdY && y < m_cmdY + m_cmdRows * (m_btnSize + m_btnSpacing)) {
            
            int col = (x - m_cmdX) / (m_btnSize + m_btnSpacing);
            int row = (y - m_cmdY) / (m_btnSize + m_btnSpacing);

            if (col >= 0 && col < m_cmdCols && row >= 0 && row < m_cmdRows) {
                int idx = row * m_cmdCols + col;
                const auto& selected = sim.getSelectedUnits();
                if (!selected.empty()) {
                    auto u = selected[0];
                    if (u->getTypeId() == 109) { // Town Center
                        if (idx == 0) {
                            sim.trainVillager(u->getPlayerId());
                        }
                    } else if (u->getTypeId() == 12 && !u->isFoundation()) { // Barracks
                        if (idx == 0) {
                            sim.trainMilitia(u->getPlayerId());
                        }
                    } else if (u->getTypeId() == 83) { // Villager: Build Menu
                        if (idx == 0) {
                            setPlacementBuilding(70, 25); // House (25 Wood)
                        } else if (idx == 1) {
                            setPlacementBuilding(12, 175); // Barracks (175 Wood)
                        } else if (idx == 2) {
                            setPlacementBuilding(68, 100); // Mill (100 Wood)
                        }
                    } else if (idx < static_cast<int>(m_commandGrid.size())) {
                        const auto& cmd = m_commandGrid[idx];
                        if (cmd.name == "Stop") {
                            for (auto& sel : selected) {
                                sel->commandMove(sel->getX(), sel->getY());
                            }
                        }
                    }
                }
            }
            return true;
        }

        // Center selection details box
        if (x >= m_selectX && x <= m_selectX + m_selectW && y >= m_selectY && y <= m_selectY + m_selectH) {
            return true;
        }

        return true; // Click consumed by bottom HUD
    }

    return false;
}

} // namespace aoe2
