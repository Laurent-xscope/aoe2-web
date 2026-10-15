#include "PanelSystem.h"

namespace aoe2 {

PanelSystem::PanelSystem() = default;
PanelSystem::~PanelSystem() = default;

void PanelSystem::onResize(int width, int height) {
    m_width = width;
    m_height = height;
}

void PanelSystem::render(Renderer& renderer, DRSReader& drs, const Simulation& sim) {
    // 1. Top Resource Bar (height 36px)
    renderer.drawRect(0, 0, m_width, 36, 25, 18, 12, 240);
    renderer.drawRect(0, 34, m_width, 2, 142, 104, 45, 255); // Gold border

    // Draw authentic DRS resource icons (Wood: 50700, Food: 50701, Gold: 50702, Stone: 50703)
    [[maybe_unused]] const auto& econ = sim.getPlayerEconomy(0);
    // Draw indicators
    renderer.drawRect(20, 8, 20, 20, 139, 90, 43, 255);   // Wood icon
    renderer.drawRect(120, 8, 20, 20, 192, 57, 43, 255);  // Food icon
    renderer.drawRect(220, 8, 20, 20, 212, 172, 13, 255); // Gold icon
    renderer.drawRect(320, 8, 20, 20, 127, 140, 141, 255);// Stone icon

    // 2. Bottom HUD Console (height 150px)
    int hudY = m_height - 150;
    renderer.drawRect(0, hudY, m_width, 150, 20, 14, 9, 250);
    renderer.drawRect(0, hudY, m_width, 3, 142, 104, 45, 255); // Top rim

    // Left: Minimap Frame (220px width)
    renderer.drawRect(10, hudY + 10, 200, 130, 5, 8, 5, 255);
    renderer.drawRect(10, hudY + 10, 200, 1, 93, 70, 36, 255);

    // Middle: Selection Details Frame
    int middleX = 230;
    int middleW = 400;
    renderer.drawRect(middleX, hudY + 10, middleW, 130, 35, 25, 16, 200);

    const auto& selected = sim.getSelectedUnits();
    if (!selected.empty()) {
        const auto& u = selected[0];
        // Selection icon box
        renderer.drawRect(middleX + 15, hudY + 25, 74, 74, 50, 36, 22, 255);
        renderer.drawRect(middleX + 15, hudY + 25, 74, 2, 212, 175, 55, 255);

        // HP Bar
        float hpRatio = u->getHP() / u->getMaxHP();
        renderer.drawRect(middleX + 105, hudY + 55, 160, 10, 40, 40, 40, 255);
        renderer.drawRect(middleX + 105, hudY + 55, static_cast<int>(160.0f * hpRatio), 10, 39, 174, 96, 255);
    }

    // Right: Action Command Grid (5x3 matrix)
    int cmdX = middleX + middleW + 20;
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 5; ++col) {
            int bx = cmdX + col * 46;
            int by = hudY + 10 + row * 44;
            renderer.drawRect(bx, by, 42, 40, 56, 41, 26, 255);
            renderer.drawRect(bx, by, 42, 1, 142, 104, 45, 255);
        }
    }
}

bool PanelSystem::handleMouseDown(int button, int x, int y, Simulation& sim) {
    if (y < 36) {
        return true; // Clicked on top resource bar
    }
    if (y >= m_height - 150) {
        return true; // Clicked on bottom HUD panel
    }
    return false;
}

} // namespace aoe2
