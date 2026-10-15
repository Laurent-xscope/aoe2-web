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

        // 3. Render authentic HUD panels
        if (panels && fs && sim) {
            panels->render(renderer, *fs, *sim, m_camX, m_camY, m_zoom);
        }
    }
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

        if (button == 0) { // Left click: Select
            Point2D world = Isometric::screenToWorld(static_cast<float>(x), static_cast<float>(y), m_camX, m_camY, m_zoom);
            if (sim) sim->selectAt(world.x, world.y);
        } else if (button == 2) { // Right click: Command Move / Attack
            Point2D world = Isometric::screenToWorld(static_cast<float>(x), static_cast<float>(y), m_camX, m_camY, m_zoom);
            if (sim) sim->issueMoveOrder(world.x, world.y);
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
    } else if (m_currentScreen == ScreenType::InGame) {
        if (button == 1) {
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
