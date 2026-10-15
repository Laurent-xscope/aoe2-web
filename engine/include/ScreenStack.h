#pragma once

#include <vector>
#include <memory>
#include <cstdint>

namespace aoe2 {

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
    void render();

    void onMouseDown(int button, int x, int y);
    void onMouseUp(int button, int x, int y);
    void onMouseMove(int x, int y);
    void onKeyDown(int key);
    void onKeyUp(int key);

    void pushScreen(ScreenType type);
    void popScreen();

    bool isInGame() const { return m_currentScreen == ScreenType::InGame; }

private:
    ScreenType m_currentScreen{ScreenType::MainMenu};
    std::vector<ScreenType> m_history;

    // Main menu buttons hit testing
    struct MenuButton {
        int x{0};
        int y{0};
        int w{0};
        int h{0};
        ScreenType targetScreen;
        bool isHovered{false};
    };

    std::vector<MenuButton> m_mainMenuButtons;
    float m_camX{0.0f};
    float m_camY{0.0f};
    float m_zoom{1.0f};

    // Camera pan state
    bool m_isPanning{false};
    int m_lastMouseX{0};
    int m_lastMouseY{0};
};

} // namespace aoe2
