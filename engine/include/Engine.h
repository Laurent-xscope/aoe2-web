#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

#include "DRSReader.h"
#include "SLPReader.h"
#include "DATParser.h"
#include "Terrain.h"
#include "Unit.h"
#include "Simulation.h"
#include "ScreenStack.h"
#include "PanelSystem.h"

namespace aoe2 {

class Engine {
public:
    Engine();
    ~Engine();

    bool initialize(int screenWidth, int screenHeight);
    void tick(double deltaTime);
    void render();

    // Input Events dispatched from WebAssembly JS runtime
    void onMouseDown(int button, int x, int y);
    void onMouseUp(int button, int x, int y);
    void onMouseMove(int x, int y);
    void onKeyDown(int key);
    void onKeyUp(int key);
    void onResize(int width, int height);

    // Subsystem accessors
    std::shared_ptr<DRSReader> getFileSystem() const { return m_fileSystem; }
    std::shared_ptr<DATParser> getRules() const { return m_rules; }
    std::shared_ptr<Terrain> getTerrain() const { return m_terrain; }
    std::shared_ptr<Simulation> getSimulation() const { return m_simulation; }
    std::shared_ptr<ScreenStack> getScreens() const { return m_screenStack; }
    std::shared_ptr<PanelSystem> getPanels() const { return m_panelSystem; }

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

private:
    int m_width{1280};
    int m_height{720};
    bool m_initialized{false};

    std::shared_ptr<DRSReader> m_fileSystem;
    std::shared_ptr<DATParser> m_rules;
    std::shared_ptr<Terrain> m_terrain;
    std::shared_ptr<Simulation> m_simulation;
    std::shared_ptr<ScreenStack> m_screenStack;
    std::shared_ptr<PanelSystem> m_panelSystem;
};

// Global singleton instance for WebAssembly C ABI entrypoints
extern Engine* g_engine;

} // namespace aoe2
