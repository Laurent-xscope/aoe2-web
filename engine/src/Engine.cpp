#include "Engine.h"
#include "Renderer.h"
#include <iostream>

namespace aoe2 {

Engine* g_engine = nullptr;

Engine::Engine() {
    g_engine = this;
}

Engine::~Engine() {
    if (g_engine == this) {
        g_engine = nullptr;
    }
}

bool Engine::initialize(int screenWidth, int screenHeight) {
    m_width = screenWidth;
    m_height = screenHeight;

    std::cout << "[AoE2 Engine] Initializing Genie Engine Core (" << m_width << "x" << m_height << ")..." << std::endl;

    // Instantiate core singletons
    m_fileSystem = std::make_shared<DRSReader>();
    m_rules = std::make_shared<DATParser>();
    m_terrain = std::make_shared<Terrain>();
    m_simulation = std::make_shared<Simulation>();
    m_screenStack = std::make_shared<ScreenStack>();
    m_panelSystem = std::make_shared<PanelSystem>();

    // Load authentic 50500 primary palette
    if (m_renderer && m_fileSystem) {
        auto palData = m_fileSystem->getResource(50500, "bina");
        if (!palData.empty()) {
            std::cout << "[AoE2 Engine] Loaded authentic 50500 palette (" << palData.size() << " bytes)." << std::endl;
            m_renderer->loadPaletteFromBuffer(palData.data(), palData.size());
        }
    }

    m_initialized = true;
    std::cout << "[AoE2 Engine] Genie Engine Core initialized successfully." << std::endl;
    return true;
}

void Engine::tick(double deltaTime) {
    if (!m_initialized) return;

    if (m_screenStack) {
        m_screenStack->tick(deltaTime);
    }

    if (m_simulation && m_screenStack && m_screenStack->isInGame()) {
        m_simulation->tick(deltaTime);
    }
}

void Engine::render() {
    if (!m_initialized) return;

    if (m_renderer) {
        render(*m_renderer);
    }
}

void Engine::render(Renderer& renderer) {
    if (!m_initialized) return;

    if (m_screenStack) {
        m_screenStack->render(renderer);
    }
}

void Engine::onMouseDown(int button, int x, int y) {
    if (m_screenStack) {
        m_screenStack->onMouseDown(button, x, y);
    }
}

void Engine::onMouseUp(int button, int x, int y) {
    if (m_screenStack) {
        m_screenStack->onMouseUp(button, x, y);
    }
}

void Engine::onMouseMove(int x, int y) {
    if (m_screenStack) {
        m_screenStack->onMouseMove(x, y);
    }
}

void Engine::onKeyDown(int key) {
    if (m_screenStack) {
        m_screenStack->onKeyDown(key);
    }
}

void Engine::onKeyUp(int key) {
    if (m_screenStack) {
        m_screenStack->onKeyUp(key);
    }
}

void Engine::onResize(int width, int height) {
    m_width = width;
    m_height = height;
    if (m_panelSystem) {
        m_panelSystem->onResize(width, height);
    }
}

} // namespace aoe2
