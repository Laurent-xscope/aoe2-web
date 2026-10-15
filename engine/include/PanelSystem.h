#pragma once

#include "Renderer.h"
#include "DRSReader.h"
#include "Simulation.h"
#include "Texture.h"
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
    void loadTextures();
    void renderResourceBar(Renderer& renderer, DRSReader& drs, const Simulation& sim);
    void renderBottomConsole(Renderer& renderer, DRSReader& drs);
    void renderMinimap(Renderer& renderer, DRSReader& drs, const Simulation& sim, 
                       float camX, float camY, float zoom);
    void renderSelectionBox(Renderer& renderer, DRSReader& drs, const Simulation& sim);
    void renderCommandMatrix(Renderer& renderer, DRSReader& drs, const Simulation& sim);

    void drawLine(Renderer& renderer, int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b);

    int m_width{1280};
    int m_height{720};

    // Authentic HD HUD textures decoded from game_b1.slp
    Texture m_texCmdFrame;       // Frame 1: 325x175 (Left command matrix frame)
    Texture m_texCenterSlice;    // Frame 2: 34x175 (Repeatable center parchment slice)
    Texture m_texMinimapFrame;   // Frame 4: 391x175 (Right minimap frame)
    Texture m_texStatusPlaque;   // Frame 5: 239x113 (Center status plaque)
    Texture m_texTopResource;    // Frame 6: 392x25 (Top resource bar)
    Texture m_texMenuPlaque;     // Frame 7: 165x21 (Top-right menu plaque)
    bool m_hudTexturesLoaded{false};

    // Minimap layout bounds
    int m_miniX{0};
    int m_miniY{0};
    int m_miniW{391};
    int m_miniH{175};

    // Selection box layout bounds
    int m_selectX{335};
    int m_selectY{0};
    int m_selectW{390};
    int m_selectH{175};

    // Command matrix layout bounds
    int m_cmdX{30};
    int m_cmdY{0};
    int m_cmdRows{3};
    int m_cmdCols{5};
    int m_btnSize{40};
    int m_btnSpacing{6};

    // Preconfigured 3x5 standard Genie command actions
    std::vector<CommandButton> m_commandGrid;
};

} // namespace aoe2
