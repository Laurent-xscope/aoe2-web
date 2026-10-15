#include "ScreenStack.h"
#include "Engine.h"
#include "Isometric.h"
#include <iostream>

namespace aoe2 {

ScreenStack::ScreenStack() {
    m_camX = 640.0f;
    m_camY = 360.0f;

    // Center 3 buttons on Main Menu: "Single Player", "Campaign", "Options"
    m_mainMenuButtons = {
        { 480, 280, 320, 50, ScreenType::InGame, false },
        { 480, 350, 320, 50, ScreenType::Campaign, false },
        { 480, 420, 320, 50, ScreenType::MainMenu, false }
    };
}

ScreenStack::~ScreenStack() = default;

void ScreenStack::pushScreen(ScreenType type) {
    m_history.push_back(m_currentScreen);
    m_currentScreen = type;

    if (type == ScreenType::InGame && g_engine) {
        auto sim = g_engine->getSimulation();
        auto rules = g_engine->getRules();
        auto terrain = g_engine->getTerrain();
        if (sim && rules && terrain) {
            sim->initialize(rules, terrain);
        }
    }
}

void ScreenStack::popScreen() {
    if (!m_history.empty()) {
        m_currentScreen = m_history.back();
        m_history.pop_back();
    }
}

void ScreenStack::tick(double deltaTime) {
    // Screen transitions or animations
}

void ScreenStack::render() {
    if (!g_engine) return;

    Renderer renderer;
    renderer.initialize(g_engine->getWidth(), g_engine->getHeight());
    renderer.beginFrame();

    auto fs = g_engine->getFileSystem();
    auto terrain = g_engine->getTerrain();
    auto sim = g_engine->getSimulation();
    auto panels = g_engine->getPanels();

    if (m_currentScreen == ScreenType::MainMenu) {
        // Render authentic Main Menu background
        // Try DRS 50100 or background shape
        if (fs) {
            auto bgShape = fs->getShape(50100);
            if (bgShape && bgShape->getFrameCount() > 0) {
                const auto* frame = bgShape->getFrame(0);
                if (frame) {
                    renderer.blitSLPFrame(frame->pixelData.data(), frame->width, frame->height, 0, 0);
                }
            } else {
                // Dark medieval background
                renderer.clear(25, 18, 12);
            }

            // Draw button shapes (btnmain.slp)
            auto btnShape = fs->getShapeByName("btnmain");
            for (const auto& btn : m_mainMenuButtons) {
                if (btnShape && btnShape->getFrameCount() > 0) {
                    const auto* frame = btnShape->getFrame(btn.isHovered ? 1 : 0);
                    if (frame) {
                        renderer.blitSLPFrame(frame->pixelData.data(), frame->width, frame->height, btn.x, btn.y);
                    }
                } else {
                    // Golden border button
                    renderer.drawRect(btn.x, btn.y, btn.w, btn.h, 60, 42, 25, 255);
                    renderer.drawRect(btn.x, btn.y, btn.w, 2, 212, 175, 55, 255);
                    renderer.drawRect(btn.x, btn.y + btn.h - 2, btn.w, 2, 212, 175, 55, 255);
                }
            }
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
            panels->render(renderer, *fs, *sim);
        }
    }

    renderer.endFrame();
}

void ScreenStack::onMouseDown(int button, int x, int y) {
    if (m_currentScreen == ScreenType::MainMenu) {
        for (const auto& btn : m_mainMenuButtons) {
            if (x >= btn.x && x <= btn.x + btn.w && y >= btn.y && y <= btn.y + btn.h) {
                pushScreen(btn.targetScreen);
                return;
            }
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
    if (button == 1) {
        m_isPanning = false;
    }
}

void ScreenStack::onMouseMove(int x, int y) {
    if (m_currentScreen == ScreenType::MainMenu) {
        for (auto& btn : m_mainMenuButtons) {
            btn.isHovered = (x >= btn.x && x <= btn.x + btn.w && y >= btn.y && y <= btn.y + btn.h);
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
    if (m_currentScreen == ScreenType::InGame) {
        const float panSpeed = 24.0f;
        if (key == 37 || key == 65) m_camX += panSpeed; // Left (A or Arrow)
        if (key == 39 || key == 68) m_camX -= panSpeed; // Right (D or Arrow)
        if (key == 38 || key == 87) m_camY += panSpeed; // Up (W or Arrow)
        if (key == 40 || key == 83) m_camY -= panSpeed; // Down (S or Arrow)
    }
}

void ScreenStack::onKeyUp(int key) {
}

} // namespace aoe2
