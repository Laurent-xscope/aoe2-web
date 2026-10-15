#pragma once

#include <vector>
#include <memory>
#include <cstdint>

namespace aoe2 {

class Renderer;
class MainMenuScreen;

enum class ScreenType {
    MainMenu = 0,
    SinglePlayer = 1,
    Campaign = 2,
    InGame = 3
};

class ScreenStack {
public:
    ScreenStack();
    ~ScreenStack();

    void tick(double deltaTime);
    void render(Renderer& renderer);

    void onMouseDown(int button, int x, int y);
    void onMouseUp(int button, int x, int y);
    void onMouseMove(int x, int y);
    void onKeyDown(int key);
    void onKeyUp(int key);

    void pushScreen(ScreenType type);
    void popScreen();

    void centerCameraOn(float worldX, float worldY);
    void setCamera(float cx, float cy) { m_camX = cx; m_camY = cy; }
    float getCameraX() const { return m_camX; }
    float getCameraY() const { return m_camY; }
    float getZoom() const { return m_zoom; }

    bool isInGame() const { return m_currentScreen == ScreenType::InGame; }
    ScreenType getCurrentScreen() const { return m_currentScreen; }
    MainMenuScreen* getMainMenu() const { return m_mainMenuScreen.get(); }

private:
    ScreenType m_currentScreen{ScreenType::MainMenu};
    std::vector<ScreenType> m_history;
    std::unique_ptr<MainMenuScreen> m_mainMenuScreen;

    float m_camX{640.0f};
    float m_camY{360.0f};
    float m_zoom{1.0f};

    // Camera pan state
    bool m_isPanning{false};
    int m_lastMouseX{0};
    int m_lastMouseY{0};

    // Drag selection box state
    bool m_isBoxSelecting{false};
    int m_boxStartX{0};
    int m_boxStartY{0};
    int m_boxCurX{0};
    int m_boxCurY{0};

    // Placement preview tile
    float m_previewTileX{0.0f};
    float m_previewTileY{0.0f};
};

} // namespace aoe2
