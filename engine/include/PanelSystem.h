#pragma once

#include "Renderer.h"
#include "DRSReader.h"
#include "Simulation.h"
#include <vector>
#include <memory>
#include <string>

namespace aoe2 {

struct CommandButton {
    int id{0};
    int iconFrame{0};
    std::string name;
    std::string tooltip;
    bool enabled{true};
    bool isVisible{true};
};

class PanelSystem {
public:
    PanelSystem();
    ~PanelSystem();

    void onResize(int width, int height);
    void render(Renderer& renderer, DRSReader& drs, const Simulation& sim,
                float camX = 640.0f, float camY = 360.0f, float zoom = 1.0f);

    bool handleMouseDown(int button, int x, int y, Simulation& sim);

private:
    void renderResourceBar(Renderer& renderer, DRSReader& drs, const Simulation& sim);
    void renderBottomConsole(Renderer& renderer, DRSReader& drs);
    void renderMinimap(Renderer& renderer, DRSReader& drs, const Simulation& sim, 
                       float camX, float camY, float zoom);
    void renderSelectionBox(Renderer& renderer, DRSReader& drs, const Simulation& sim);
    void renderCommandMatrix(Renderer& renderer, DRSReader& drs, const Simulation& sim);

    void drawLine(Renderer& renderer, int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b);

    int m_width{1280};
    int m_height{720};

    // Cached console slices from DRS interface SLPs (50231/50232/50221)
    std::vector<uint8_t> m_bottomConsoleSlice;
    int m_consoleSliceW{0};
    int m_consoleSliceH{0};

    // Minimap layout bounds
    int m_miniX{14};
    int m_miniY{0};
    int m_miniW{220};
    int m_miniH{136};

    // Selection box layout bounds
    int m_selectX{250};
    int m_selectY{0};
    int m_selectW{390};
    int m_selectH{136};

    // Command matrix layout bounds
    int m_cmdX{660};
    int m_cmdY{0};
    int m_cmdRows{3};
    int m_cmdCols{5};
    int m_btnSize{40};
    int m_btnSpacing{4};

    // Preconfigured 3x5 standard Genie command actions
    std::vector<CommandButton> m_commandGrid;
};

} // namespace aoe2
