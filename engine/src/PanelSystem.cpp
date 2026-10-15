#include "PanelSystem.h"
#include "Isometric.h"
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

void PanelSystem::onResize(int width, int height) {
    m_width = width;
    m_height = height;

    int hudY = m_height - 156;

    // Minimap bounds on left
    m_miniX = 14;
    m_miniY = hudY + 10;
    m_miniW = 220;
    m_miniH = 136;

    // Center selection details box
    m_selectX = m_miniX + m_miniW + 16;
    m_selectY = hudY + 10;
    m_selectW = std::clamp(m_width - (m_miniW + 16) - (5 * (m_btnSize + m_btnSpacing) + 32), 280, 420);
    m_selectH = 136;

    // Right command grid (3 rows x 5 cols)
    m_cmdX = m_selectX + m_selectW + 16;
    m_cmdY = hudY + 10;

    // Invalidate cached console slices on resize
    m_bottomConsoleSlice.clear();
    m_consoleSliceW = 0;
    m_consoleSliceH = 0;
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
    // 1. Bottom HUD Console Background (authentic carved stone graphics)
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
    const int barH = 32;

    // Authentic dark stone/wood bar background
    renderer.drawRect(0, 0, m_width, barH, 26, 18, 12, 245);
    renderer.drawRect(0, barH - 2, m_width, 2, 192, 150, 48, 255); // Beveled gold rim
    renderer.drawRect(0, barH, m_width, 1, 60, 42, 22, 255);     // Dark shadow seam

    // Load authentic DRS resource icons: 50760.slp (resico.shp)
    // Frame 0: Wood, Frame 1: Food, Frame 2: Gold, Frame 3: Stone, Frame 4: Population
    auto resShape = drs.getShape(50760);
    if (!resShape) {
        resShape = drs.getShape(50732); // Fallback to tradicon.shp
    }

    const auto& econ = sim.getPlayerEconomy(0);

    struct ResourceItem {
        int iconFrame;
        int value;
        int maxVal;
        bool isPop;
    };

    ResourceItem items[5] = {
        { 0, econ.wood, 0, false },
        { 1, econ.food, 0, false },
        { 2, econ.gold, 0, false },
        { 3, econ.stone, 0, false },
        { 4, econ.population, econ.maxPopulation, true }
    };

    int startX = 16;
    for (int i = 0; i < 5; ++i) {
        int iconX = startX + i * 105;
        int iconY = 8;

        // Draw authentic icon
        bool drewIcon = false;
        if (resShape && items[i].iconFrame < resShape->getFrameCount()) {
            const auto* frame = resShape->getFrame(items[i].iconFrame);
            if (frame && !frame->pixelData.empty()) {
                renderer.blitSLPFrame(frame->pixelData.data(), frame->width, frame->height, iconX, iconY);
                drewIcon = true;
            }
        }

        if (!drewIcon) {
            // Elegant colored fallback icon
            uint8_t r = 139, g = 90, b = 43;
            if (i == 1) { r = 192; g = 57; b = 43; }
            else if (i == 2) { r = 212; g = 172; b = 13; }
            else if (i == 3) { r = 127; g = 140; b = 141; }
            else if (i == 4) { r = 41; g = 128; b = 185; }
            renderer.drawRect(iconX, iconY, 20, 16, r, g, b, 255);
            renderer.drawRect(iconX, iconY, 20, 1, 220, 180, 80, 255);
        }

        // Draw resource amount text
        std::string valStr = items[i].isPop ? 
            (std::to_string(items[i].value) + "/" + std::to_string(items[i].maxVal)) :
            std::to_string(items[i].value);

        renderer.drawText(valStr, iconX + 28, 12, 0xFFFFFFFF);
    }

    // Display current age indicator centered in authentic gold
    static const char* ageNames[4] = { "Dark Age", "Feudal Age", "Castle Age", "Imperial Age" };
    int ageIdx = std::clamp(econ.currentAge, 0, 3);
    std::string ageStr = ageNames[ageIdx];
    int ageX = (m_width / 2) - 45;
    renderer.drawText(ageStr, ageX, 12, 0xFFE6C875);

    // Top-right Menu button
    int menuBtnX = m_width - 84;
    renderer.drawRect(menuBtnX, 5, 70, 22, 54, 40, 26, 255);
    renderer.drawRect(menuBtnX, 5, 70, 1, 192, 150, 48, 255);
    renderer.drawRect(menuBtnX, 26, 70, 1, 30, 20, 12, 255);
    renderer.drawText("Menu", menuBtnX + 18, 11, 0xFFFFFFFF);
}

void PanelSystem::renderBottomConsole(Renderer& renderer, DRSReader& drs) {
    const int hudH = 156;
    int hudY = m_height - hudH;

    // Cache genuine carved stone HUD console slice from DRS 50231.slp or 50232.slp or 50221.slp
    if (m_bottomConsoleSlice.empty()) {
        auto consoleShape = drs.getShape(50231);
        if (!consoleShape) consoleShape = drs.getShape(50232);
        if (!consoleShape) consoleShape = drs.getShape(50221);

        if (consoleShape && consoleShape->getFrameCount() > 0) {
            const auto* frame = consoleShape->getFrame(0);
            if (frame && frame->width > 0 && frame->height >= 500) {
                int startY = (frame->height >= 600) ? 444 : std::max(0, frame->height - hudH);
                int sliceH = std::min(hudH, frame->height - startY);
                m_consoleSliceW = frame->width;
                m_consoleSliceH = sliceH;
                m_bottomConsoleSlice.resize(static_cast<size_t>(m_consoleSliceW) * m_consoleSliceH);

                for (int r = 0; r < sliceH; ++r) {
                    std::memcpy(&m_bottomConsoleSlice[r * m_consoleSliceW],
                                &frame->pixelData[(startY + r) * frame->width],
                                m_consoleSliceW);
                }
            }
        }
    }

    // Tile or blit genuine stone console graphics across bottom
    if (!m_bottomConsoleSlice.empty()) {
        for (int x = 0; x < m_width; x += m_consoleSliceW) {
            renderer.blitSLPFrame(m_bottomConsoleSlice.data(), m_consoleSliceW, m_consoleSliceH, x, hudY);
        }
    } else {
        // Fallback authentic stone texture fill
        renderer.drawRect(0, hudY, m_width, hudH, 24, 18, 12, 255);
    }

    // Authentic carved gold rim running along the top of bottom HUD
    renderer.drawRect(0, hudY, m_width, 2, 212, 175, 55, 255);
    renderer.drawRect(0, hudY + 2, m_width, 1, 93, 70, 36, 255);
    renderer.drawRect(0, hudY + 3, m_width, 1, 15, 10, 6, 255);
}

void PanelSystem::renderMinimap(Renderer& renderer, DRSReader& drs, const Simulation& sim,
                               float camX, float camY, float zoom) {
    // 1. Authentic Minimap Stone Background (50763.slp or container)
    auto bezel = drs.getShape(50763);
    if (bezel && bezel->getFrameCount() > 0) {
        const auto* frame = bezel->getFrame(0);
        if (frame && !frame->pixelData.empty()) {
            renderer.blitSLPFrame(frame->pixelData.data(), frame->width, frame->height, m_miniX - 24, m_miniY - 24);
        }
    } else {
        renderer.drawRect(m_miniX, m_miniY, m_miniW, m_miniH, 8, 10, 10, 255);
    }

    // 2. Render Terrain onto Minimap
    auto terrain = sim.getTerrain();
    int mapW = terrain ? terrain->getWidth() : 120;
    int mapH = terrain ? terrain->getHeight() : 120;

    int cx = m_miniX + m_miniW / 2;
    int cy = m_miniY + m_miniH / 2;
    float stepX = (m_miniW * 0.44f) / mapW;
    float stepY = (m_miniH * 0.44f) / mapH;

    if (terrain) {
        for (int ty = 0; ty < mapH; ++ty) {
            for (int tx = 0; tx < mapW; ++tx) {
                const auto* tile = terrain->getTile(tx, ty);
                if (!tile) continue;

                // Rotate 45 degrees into isometric diamond
                int px = static_cast<int>(cx + (tx - ty) * stepX);
                int py = static_cast<int>(cy + (tx + ty) * stepY * 0.5f);

                if (px < m_miniX || px >= m_miniX + m_miniW ||
                    py < m_miniY || py >= m_miniY + m_miniH) {
                    continue;
                }

                // Authentic Genie Engine minimap terrain colors
                uint8_t r = 34, g = 110, b = 30; // Grass
                switch (tile->type) {
                    case TerrainType::Forest: r = 14;  g = 58;  b = 14;  break;
                    case TerrainType::Water:  r = 26;  g = 72;  b = 145; break;
                    case TerrainType::Desert: r = 182; g = 156; b = 92;  break;
                    case TerrainType::Farm:   r = 146; g = 122; b = 42;  break;
                    default: break;
                }
                renderer.drawRect(px, py, 2, 1, r, g, b, 255);
            }
        }
    }

    // 3. Render Unit dots on Minimap
    for (const auto& u : sim.getAllUnits()) {
        if (!u || u->isDead()) continue;

        int upx = static_cast<int>(cx + (u->getX() - u->getY()) * stepX);
        int upy = static_cast<int>(cy + (u->getX() + u->getY()) * stepY * 0.5f);

        if (upx >= m_miniX && upx < m_miniX + m_miniW - 1 &&
            upy >= m_miniY && upy < m_miniY + m_miniH - 1) {
            
            // Player colors: Player 1 = Blue, Player 2 = Red, Gaia = White
            uint8_t ur = 80, ug = 150, ub = 255;
            if (u->getPlayerId() == 2) {
                ur = 235; ug = 40; ub = 40;
            } else if (u->getPlayerId() == 0) {
                ur = 240; ug = 240; ub = 240;
            }

            renderer.drawRect(upx, upy, 2, 2, ur, ug, ub, 255);
        }
    }

    // 4. Render Camera Viewport Rectangle on Minimap
    Point2D c0 = Isometric::screenToWorld(0.0f, 34.0f, camX, camY, zoom);
    Point2D c1 = Isometric::screenToWorld(static_cast<float>(m_width), 34.0f, camX, camY, zoom);
    Point2D c2 = Isometric::screenToWorld(static_cast<float>(m_width), static_cast<float>(m_height - 156), camX, camY, zoom);
    Point2D c3 = Isometric::screenToWorld(0.0f, static_cast<float>(m_height - 156), camX, camY, zoom);

    auto toMini = [&](const Point2D& pt) -> std::pair<int, int> {
        int x = static_cast<int>(cx + (pt.x - pt.y) * stepX);
        int y = static_cast<int>(cy + (pt.x + pt.y) * stepY * 0.5f);
        x = std::clamp(x, m_miniX, m_miniX + m_miniW - 1);
        y = std::clamp(y, m_miniY, m_miniY + m_miniH - 1);
        return { x, y };
    };

    auto m0 = toMini(c0);
    auto m1 = toMini(c1);
    auto m2 = toMini(c2);
    auto m3 = toMini(c3);

    drawLine(renderer, m0.first, m0.second, m1.first, m1.second, 255, 255, 255);
    drawLine(renderer, m1.first, m1.second, m2.first, m2.second, 255, 255, 255);
    drawLine(renderer, m2.first, m2.second, m3.first, m3.second, 255, 255, 255);
    drawLine(renderer, m3.first, m3.second, m0.first, m0.second, 255, 255, 255);

    // 5. Beveled stone frame border
    renderer.drawRect(m_miniX - 2, m_miniY - 2, m_miniW + 4, 2, 142, 104, 45, 255);
    renderer.drawRect(m_miniX - 2, m_miniY + m_miniH, m_miniW + 4, 2, 142, 104, 45, 255);
    renderer.drawRect(m_miniX - 2, m_miniY - 2, 2, m_miniH + 4, 142, 104, 45, 255);
    renderer.drawRect(m_miniX + m_miniW, m_miniY - 2, 2, m_miniH + 4, 142, 104, 45, 255);
}

void PanelSystem::renderSelectionBox(Renderer& renderer, DRSReader& drs, const Simulation& sim) {
    // Carved stone selection details background
    renderer.drawRect(m_selectX, m_selectY, m_selectW, m_selectH, 32, 24, 16, 235);
    renderer.drawRect(m_selectX, m_selectY, m_selectW, 2, 142, 104, 45, 255);
    renderer.drawRect(m_selectX, m_selectY, 2, m_selectH, 142, 104, 45, 255);
    renderer.drawRect(m_selectX + m_selectW - 2, m_selectY, 2, m_selectH, 30, 20, 10, 255);
    renderer.drawRect(m_selectX, m_selectY + m_selectH - 2, m_selectW, 2, 30, 20, 10, 255);

    const auto& selected = sim.getSelectedUnits();
    if (selected.empty()) {
        renderer.drawText("No selection", m_selectX + 20, m_selectY + 60, 0xFFA09080);
        return;
    }

    const auto& u = selected[0];
    const auto& ut = u->getType();

    // 1. Selection Portrait Box with authentic Halo (53003.slp)
    int iconX = m_selectX + 16;
    int iconY = m_selectY + 16;

    auto haloShape = drs.getShape(53003);
    if (haloShape && haloShape->getFrameCount() > 0) {
        const auto* frame = haloShape->getFrame(0);
        if (frame && !frame->pixelData.empty()) {
            renderer.blitSLPFrame(frame->pixelData.data(), frame->width, frame->height, iconX - 2, iconY - 2);
        }
    } else {
        renderer.drawRect(iconX - 2, iconY - 2, 44, 44, 18, 14, 10, 255);
        renderer.drawRect(iconX - 2, iconY - 2, 44, 2, 212, 175, 55, 255);
        renderer.drawRect(iconX - 2, iconY + 40, 44, 2, 142, 104, 45, 255);
    }

    // 2. Authentic Unit Icon from 50730.slp / 50729.slp / 50705.slp
    auto iconShape = drs.getShape(50730);
    if (!iconShape) iconShape = drs.getShape(50729);
    if (!iconShape) iconShape = drs.getShape(50705);

    int iconFrameIdx = 0;
    if (ut.unitClass == 3) iconFrameIdx = 10;      // Villager
    else if (ut.unitClass == 4) iconFrameIdx = 0;  // Infantry / Militia
    else if (ut.unitClass == 0) iconFrameIdx = 4;  // Archer
    else if (ut.unitClass == 8) iconFrameIdx = 8;  // Cavalry
    else if (ut.unitClass == 2) iconFrameIdx = 0;  // Town Center / Building

    bool drewIcon = false;
    if (iconShape && iconFrameIdx < iconShape->getFrameCount()) {
        const auto* frame = iconShape->getFrame(iconFrameIdx);
        if (frame && !frame->pixelData.empty()) {
            renderer.blitSLPFrame(frame->pixelData.data(), frame->width, frame->height, iconX + 2, iconY + 2);
            drewIcon = true;
        }
    }

    if (!drewIcon) {
        // Fallback unit graphic representation
        renderer.drawRect(iconX + 2, iconY + 2, 36, 36, 60, 45, 30, 255);
    }

    // 3. Unit Name and Owner
    std::string displayName = ut.name.empty() ? "Unit" : ut.name;
    renderer.drawText(displayName, m_selectX + 68, m_selectY + 14, 0xFFFFFFFF);

    std::string ownerStr = (u->getPlayerId() == 1) ? "Player 1 (You)" : "Enemy Unit";
    uint32_t ownerColor = (u->getPlayerId() == 1) ? 0xFF8CB4FF : 0xFFFF6060;
    renderer.drawText(ownerStr, m_selectX + 68, m_selectY + 28, ownerColor);

    // 4. Authentic Health Bar (50764.slp - sundial gauge with 100 frames)
    float hpRatio = std::clamp(u->getHP() / std::max(1.0f, u->getMaxHP()), 0.0f, 1.0f);
    int hpBarX = m_selectX + 68;
    int hpBarY = m_selectY + 44;

    auto hpShape = drs.getShape(50764);
    int hpFrameIdx = std::clamp(static_cast<int>(hpRatio * 99.0f), 0, 99);
    bool drewHpBar = false;

    if (hpShape && hpFrameIdx < hpShape->getFrameCount()) {
        const auto* frame = hpShape->getFrame(hpFrameIdx);
        if (frame && !frame->pixelData.empty()) {
            renderer.blitSLPFrame(frame->pixelData.data(), frame->width, frame->height, hpBarX, hpBarY);
            drewHpBar = true;
        }
    }

    if (!drewHpBar) {
        renderer.drawRect(hpBarX, hpBarY, 120, 10, 40, 40, 40, 255);
        renderer.drawRect(hpBarX, hpBarY, static_cast<int>(120.0f * hpRatio), 10, 39, 174, 96, 255);
        renderer.drawRect(hpBarX, hpBarY, 120, 1, 142, 104, 45, 255);
    }

    std::string hpStr = std::to_string(static_cast<int>(u->getHP())) + "/" + std::to_string(static_cast<int>(u->getMaxHP()));
    renderer.drawText(hpStr, hpBarX + 110, hpBarY + 1, 0xFFFFFFFF);

    // 5. Authentic Stats: Attack, Armor, Range (50731.slp - itemicon.shp)
    auto statIcons = drs.getShape(50731);

    // Attack
    int statY = m_selectY + 74;
    if (statIcons && statIcons->getFrame(0)) {
        const auto* frame = statIcons->getFrame(0);
        renderer.blitSLPFrame(frame->pixelData.data(), frame->width, frame->height, m_selectX + 16, statY);
    }
    int atkVal = (!ut.attacks.empty()) ? ut.attacks[0].amount : 4;
    renderer.drawText(std::to_string(atkVal), m_selectX + 54, statY + 8, 0xFFFFFFFF);

    // Armor
    if (statIcons && statIcons->getFrame(2)) {
        const auto* frame = statIcons->getFrame(2);
        renderer.blitSLPFrame(frame->pixelData.data(), frame->width, frame->height, m_selectX + 90, statY);
    }
    int armVal = (!ut.armors.empty()) ? ut.armors[0].amount : 0;
    renderer.drawText(std::to_string(armVal) + "/0", m_selectX + 128, statY + 8, 0xFFFFFFFF);

    // Range
    if (statIcons && statIcons->getFrame(3)) {
        const auto* frame = statIcons->getFrame(3);
        renderer.blitSLPFrame(frame->pixelData.data(), frame->width, frame->height, m_selectX + 175, statY);
    }
    int rngVal = static_cast<int>(ut.maxRange);
    renderer.drawText(rngVal > 1 ? std::to_string(rngVal) : "0", m_selectX + 215, statY + 8, 0xFFFFFFFF);

    // If multiple units are selected, show selection count
    if (selected.size() > 1) {
        std::string countStr = "+" + std::to_string(selected.size() - 1) + " units";
        renderer.drawText(countStr, m_selectX + 68, statY + 28, 0xFFE2B755);
    }
}

void PanelSystem::renderCommandMatrix(Renderer& renderer, DRSReader& drs, const Simulation& sim) {
    auto cmdShape = drs.getShape(50721); // btncmd.shp (36x36 action icons)
    const auto& selected = sim.getSelectedUnits();
    bool hasSelection = !selected.empty();

    for (int row = 0; row < m_cmdRows; ++row) {
        for (int col = 0; col < m_cmdCols; ++col) {
            int idx = row * m_cmdCols + col;
            if (idx >= static_cast<int>(m_commandGrid.size())) continue;

            const auto& btn = m_commandGrid[idx];
            int bx = m_cmdX + col * (m_btnSize + m_btnSpacing);
            int by = m_cmdY + row * (m_btnSize + m_btnSpacing);

            // Carved stone button frame (brightened if units are actively selected)
            uint8_t fillR = hasSelection ? 48 : 34;
            uint8_t fillG = hasSelection ? 36 : 26;
            uint8_t fillB = hasSelection ? 24 : 18;
            renderer.drawRect(bx, by, m_btnSize, m_btnSize, fillR, fillG, fillB, 255);
            renderer.drawRect(bx, by, m_btnSize, 1, 142, 104, 45, 255);     // Top highlight
            renderer.drawRect(bx, by, 1, m_btnSize, 142, 104, 45, 255);     // Left highlight
            renderer.drawRect(bx + m_btnSize - 1, by, 1, m_btnSize, 24, 16, 10, 255); // Right shadow
            renderer.drawRect(bx, by + m_btnSize - 1, m_btnSize, 1, 24, 16, 10, 255); // Bottom shadow

            // Blit authentic command button icon
            bool drewCmd = false;
            if (cmdShape && btn.iconFrame < cmdShape->getFrameCount()) {
                const auto* frame = cmdShape->getFrame(btn.iconFrame);
                if (frame && !frame->pixelData.empty()) {
                    renderer.blitSLPFrame(frame->pixelData.data(), frame->width, frame->height, bx + 2, by + 2);
                    drewCmd = true;
                }
            }

            if (!drewCmd) {
                // Indicator dot for fallback
                renderer.drawRect(bx + 18, by + 18, 4, 4, 192, 150, 48, 255);
            }
        }
    }
}

bool PanelSystem::handleMouseDown(int button, int x, int y, Simulation& sim) {
    // 1. Clicked on top resource bar
    if (y < 34) {
        return true;
    }

    // 2. Clicked on bottom HUD console
    int hudY = m_height - 156;
    if (y >= hudY) {
        // Minimap click navigation & commands
        if (x >= m_miniX && x <= m_miniX + m_miniW && y >= m_miniY && y <= m_miniY + m_miniH) {
            auto terrain = sim.getTerrain();
            int mapW = terrain ? terrain->getWidth() : 120;
            int mapH = terrain ? terrain->getHeight() : 120;

            int cx = m_miniX + m_miniW / 2;
            int cy = m_miniY + m_miniH / 2;
            float stepX = (m_miniW * 0.44f) / mapW;
            float stepY = (m_miniH * 0.44f) / mapH;

            float relX = (x - cx) / stepX;
            float relY = (y - cy) / (stepY * 0.5f);

            float worldX = (relY + relX) * 0.5f;
            float worldY = (relY - relX) * 0.5f;

            if (button == 0) {
                sim.selectAt(worldX, worldY);
            } else if (button == 2) {
                sim.issueMoveOrder(worldX, worldY);
            }
            return true;
        }

        // Command button click dispatch
        if (x >= m_cmdX && x < m_cmdX + m_cmdCols * (m_btnSize + m_btnSpacing) &&
            y >= m_cmdY && y < m_cmdY + m_cmdRows * (m_btnSize + m_btnSpacing)) {
            
            int col = (x - m_cmdX) / (m_btnSize + m_btnSpacing);
            int row = (y - m_cmdY) / (m_btnSize + m_btnSpacing);

            if (col >= 0 && col < m_cmdCols && row >= 0 && row < m_cmdRows) {
                int idx = row * m_cmdCols + col;
                if (idx < static_cast<int>(m_commandGrid.size())) {
                    const auto& cmd = m_commandGrid[idx];
                    const auto& selected = sim.getSelectedUnits();
                    if (!selected.empty()) {
                        if (cmd.name == "Stop") {
                            // Stop order
                            for (auto& u : selected) {
                                u->commandMove(u->getX(), u->getY());
                            }
                        }
                    }
                }
            }
            return true;
        }

        // Selection details box click
        if (x >= m_selectX && x <= m_selectX + m_selectW && y >= m_selectY && y <= m_selectY + m_selectH) {
            return true;
        }

        return true; // Click consumed by bottom HUD
    }

    return false;
}

} // namespace aoe2
