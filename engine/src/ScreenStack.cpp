#include "ScreenStack.h"
#include "MainMenuScreen.h"
#include "Engine.h"
#include "Isometric.h"
#include "Renderer.h"
#include <iostream>

namespace aoe2 {

ScreenStack::ScreenStack() {
    m_camX = 640.0f;
    m_camY = 360.0f;
    m_mainMenuScreen = std::make_unique<MainMenuScreen>(this);
    if (g_engine) {
        m_mainMenuScreen->initialize(g_engine->getWidth(), g_engine->getHeight());
    }
}

ScreenStack::~ScreenStack() = default;

void ScreenStack::pushScreen(ScreenType type) {
    m_history.push_back(m_currentScreen);
    m_currentScreen = type;

    if (type == ScreenType::InGame && g_engine) {
        std::cout << "[ScreenStack] Transitioning to InGame Simulation Screen..." << std::endl;
        auto sim = g_engine->getSimulation();
        auto rules = g_engine->getRules();
        auto terrain = g_engine->getTerrain();
        if (sim && rules && terrain) {
            sim->initialize(rules, terrain);
        }

        // Center camera on Player 1 starting Town Center & Villagers (world 22, 22)
        m_camX = g_engine->getWidth() / 2.0f;
        m_camY = (g_engine->getHeight() / 2.0f - 80.0f) - ((22.0f + 22.0f) * (Isometric::TILE_HEIGHT / 2.0f));
    }
}

void ScreenStack::popScreen() {
    if (!m_history.empty()) {
        m_currentScreen = m_history.back();
        m_history.pop_back();
    }
}

void ScreenStack::tick(double deltaTime) {
    if (m_currentScreen == ScreenType::MainMenu && m_mainMenuScreen) {
        m_mainMenuScreen->tick(deltaTime);
    }
}

void ScreenStack::render(Renderer& renderer) {
    if (!g_engine) return;

    auto fs = g_engine->getFileSystem();
    auto terrain = g_engine->getTerrain();
    auto sim = g_engine->getSimulation();
    auto panels = g_engine->getPanels();

    if (m_currentScreen == ScreenType::MainMenu) {
        if (m_mainMenuScreen) {
            m_mainMenuScreen->render(renderer, fs.get(), g_engine->getWidth(), g_engine->getHeight());
        }
    } else if (m_currentScreen == ScreenType::InGame) {
        // 1. Render Terrain Tiles
        if (terrain) {
            terrain->render(renderer, m_camX, m_camY, m_zoom);
        }

        // 2. Render Units & Buildings
        if (sim && fs) {
            sim->render(renderer, *fs, m_camX, m_camY, m_zoom);
        }

        // 3. Render Placement Preview if in building placement mode
        if (panels && panels->getPlacementBuildingTypeId() > 0) {
            Point2D pt = Isometric::worldToScreen(m_previewTileX, m_previewTileY, 0.0f, m_camX, m_camY, m_zoom);
            renderer.drawRect(static_cast<int>(pt.x - 32), static_cast<int>(pt.y - 16), 64, 32, 60, 240, 60, 160);
            renderer.drawRect(static_cast<int>(pt.x - 33), static_cast<int>(pt.y - 17), 66, 34, 180, 255, 180, 200);
        }

        // 4. Render Drag Selection Box
        if (m_isBoxSelecting) {
            int minX = std::min(m_boxStartX, m_boxCurX);
            int maxX = std::max(m_boxStartX, m_boxCurX);
            int minY = std::min(m_boxStartY, m_boxCurY);
            int maxY = std::max(m_boxStartY, m_boxCurY);
            int w = maxX - minX;
            int h = maxY - minY;
            if (w > 2 && h > 2) {
                renderer.drawRect(minX, minY, w, 1, 80, 255, 80, 255);
                renderer.drawRect(minX, maxY, w, 1, 80, 255, 80, 255);
                renderer.drawRect(minX, minY, 1, h, 80, 255, 80, 255);
                renderer.drawRect(maxX, minY, 1, h, 80, 255, 80, 255);
            }
        }

        // 5. Render authentic HUD panels
        if (panels && fs && sim) {
            panels->render(renderer, *fs, *sim, m_camX, m_camY, m_zoom);
        }
    }
}

void ScreenStack::centerCameraOn(float worldX, float worldY) {
    if (!g_engine) return;
    int w = g_engine->getWidth();
    int h = g_engine->getHeight();
    m_camX = (w / 2.0f) - ((worldX - worldY) * (Isometric::TILE_WIDTH / 2.0f) * m_zoom);
    m_camY = ((h - 175.0f) / 2.0f) - ((worldX + worldY) * (Isometric::TILE_HEIGHT / 2.0f) * m_zoom);
}

void ScreenStack::onMouseDown(int button, int x, int y) {
    if (m_currentScreen == ScreenType::MainMenu) {
        if (m_mainMenuScreen && m_mainMenuScreen->onMouseDown(button, x, y)) {
            return;
        }
    } else if (m_currentScreen == ScreenType::InGame && g_engine) {
        auto sim = g_engine->getSimulation();
        auto panels = g_engine->getPanels();
        if (panels && sim && panels->handleMouseDown(button, x, y, *sim)) {
            return; // Click consumed by HUD panel
        }

        if (button == 0) { // Left click
            if (panels && panels->getPlacementBuildingTypeId() > 0 && sim) {
                // Place building foundation
                int woodCost = panels->getPlacementWoodCost();
                auto& econ = sim->getPlayerEconomy(0);
                if (econ.wood >= woodCost) {
                    econ.wood -= woodCost;
                    int bldgType = panels->getPlacementBuildingTypeId();
                    auto foundation = sim->spawnFoundation(1, bldgType, 1, m_previewTileX, m_previewTileY);
                    if (foundation) {
                        sim->issueBuildOrder(foundation->getId());
                    }
                }
                panels->cancelPlacement();
            } else {
                // Start drag box selection
                m_isBoxSelecting = true;
                m_boxStartX = x;
                m_boxStartY = y;
                m_boxCurX = x;
                m_boxCurY = y;
            }
        } else if (button == 2) { // Right click
            if (panels && panels->getPlacementBuildingTypeId() > 0) {
                panels->cancelPlacement();
            } else if (sim) {
                Point2D world = Isometric::screenToWorld(static_cast<float>(x), static_cast<float>(y), m_camX, m_camY, m_zoom);
                
                // Contextual command dispatch: check if clicking enemy unit or foundation
                bool handled = false;
                for (const auto& u : sim->getAllUnits()) {
                    if (!u || u->isDead()) continue;
                    float dx = u->getX() - world.x;
                    float dy = u->getY() - world.y;
                    if (dx * dx + dy * dy < 3.0f) {
                        if (u->getPlayerId() != 1) { // Enemy unit: attack order
                            sim->issueAttackOrder(u->getId());
                            handled = true;
                            break;
                        } else if (u->isFoundation()) { // Friendly foundation: build order
                            sim->issueBuildOrder(u->getId());
                            handled = true;
                            break;
                        }
                    }
                }

                if (!handled) {
                    sim->issueMoveOrder(world.x, world.y);
                }
            }
        } else if (button == 1) { // Middle click: Start camera pan
            m_isPanning = true;
            m_lastMouseX = x;
            m_lastMouseY = y;
        }
    }
}

void ScreenStack::onMouseUp(int button, int x, int y) {
    if (m_currentScreen == ScreenType::MainMenu) {
        if (m_mainMenuScreen && m_mainMenuScreen->onMouseUp(button, x, y)) {
            return;
        }
    } else if (m_currentScreen == ScreenType::InGame && g_engine) {
        auto sim = g_engine->getSimulation();
        if (button == 0 && m_isBoxSelecting) {
            m_isBoxSelecting = false;
            m_boxCurX = x;
            m_boxCurY = y;

            int dx = std::abs(m_boxCurX - m_boxStartX);
            int dy = std::abs(m_boxCurY - m_boxStartY);

            if (dx > 6 || dy > 6) {
                // Drag selection box completed: select all units within bounds
                int minX = std::min(m_boxStartX, m_boxCurX);
                int maxX = std::max(m_boxStartX, m_boxCurX);
                int minY = std::min(m_boxStartY, m_boxCurY);
                int maxY = std::max(m_boxStartY, m_boxCurY);

                Point2D w1 = Isometric::screenToWorld(static_cast<float>(minX), static_cast<float>(minY), m_camX, m_camY, m_zoom);
                Point2D w2 = Isometric::screenToWorld(static_cast<float>(maxX), static_cast<float>(minY), m_camX, m_camY, m_zoom);
                Point2D w3 = Isometric::screenToWorld(static_cast<float>(minX), static_cast<float>(maxY), m_camX, m_camY, m_zoom);
                Point2D w4 = Isometric::screenToWorld(static_cast<float>(maxX), static_cast<float>(maxY), m_camX, m_camY, m_zoom);

                float minWX = std::min({w1.x, w2.x, w3.x, w4.x});
                float maxWX = std::max({w1.x, w2.x, w3.x, w4.x});
                float minWY = std::min({w1.y, w2.y, w3.y, w4.y});
                float maxWY = std::max({w1.y, w2.y, w3.y, w4.y});

                if (sim) sim->selectBox(minWX, minWY, maxWX, maxWY);
            } else {
                // Simple click selection
                Point2D world = Isometric::screenToWorld(static_cast<float>(m_boxStartX), static_cast<float>(m_boxStartY), m_camX, m_camY, m_zoom);
                if (sim) sim->selectAt(world.x, world.y);
            }
        } else if (button == 1) {
            m_isPanning = false;
        }
    }
}

void ScreenStack::onMouseMove(int x, int y) {
    if (m_currentScreen == ScreenType::MainMenu) {
        if (m_mainMenuScreen) {
            m_mainMenuScreen->onMouseMove(x, y);
        }
    } else if (m_currentScreen == ScreenType::InGame) {
        if (m_isPanning) {
            m_camX += (x - m_lastMouseX);
            m_camY += (y - m_lastMouseY);
            m_lastMouseX = x;
            m_lastMouseY = y;
        }

        if (m_isBoxSelecting) {
            m_boxCurX = x;
            m_boxCurY = y;
        }

        Point2D world = Isometric::screenToWorld(static_cast<float>(x), static_cast<float>(y), m_camX, m_camY, m_zoom);
        m_previewTileX = std::round(world.x);
        m_previewTileY = std::round(world.y);
    }
}

void ScreenStack::onKeyDown(int key) {
    if (m_currentScreen == ScreenType::MainMenu) {
        if (m_mainMenuScreen && m_mainMenuScreen->onKeyDown(key)) {
            return;
        }
    } else if (m_currentScreen == ScreenType::InGame) {
        const float panSpeed = 24.0f;
        if (key == 37 || key == 65) m_camX += panSpeed; // Left (A or Arrow)
        if (key == 39 || key == 68) m_camX -= panSpeed; // Right (D or Arrow)
        if (key == 38 || key == 87) m_camY += panSpeed; // Up (W or Arrow)
        if (key == 40 || key == 83) m_camY -= panSpeed; // Down (S or Arrow)
    }
}

void ScreenStack::onKeyUp(int /*key*/) {
}

} // namespace aoe2
