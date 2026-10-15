#pragma once

#include "Renderer.h"
#include "DRSReader.h"
#include "Simulation.h"

namespace aoe2 {

class PanelSystem {
public:
    PanelSystem();
    ~PanelSystem();

    void onResize(int width, int height);
    void render(Renderer& renderer, DRSReader& drs, const Simulation& sim);

    bool handleMouseDown(int button, int x, int y, Simulation& sim);

private:
    int m_width{1280};
    int m_height{720};
};

} // namespace aoe2
