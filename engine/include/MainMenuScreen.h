#pragma once

#include <vector>
#include <string>
#include <functional>
#include <cstdint>
#include <memory>

namespace aoe2 {

class Renderer;
class DRSReader;
class ScreenStack;

enum class MenuButtonId {
    About = 0,
    SinglePlayer = 1,
    MultiPlayer = 2,
    MapEditor = 3,
    History = 4,
    Tutorial = 5,
    Exit = 6,
    Options = 7,
    ModManager = 8
};

struct MenuButton {
    MenuButtonId id{MenuButtonId::SinglePlayer};
    std::string name;
    std::string label;
    std::string rolloverText;
    
    // Normalized 1366x768 reference coordinates (from authentic main-menu.json)
    int refX{0};
    int refY{0};
    int refW{0};
    int refH{0};
    int refTextX{0};
    int refTextY{0};
    int refTextW{0};
    int refTextH{0};
    
    uint32_t textKey{0};
    uint32_t rolloverKey{0};
    std::string upImage;
    std::string downImage;
    std::string hoverImage;

    // Computed screen coordinates
    int x{0};
    int y{0};
    int w{0};
    int h{0};
    int textX{0};
    int textY{0};

    bool isHovered{false};
    bool isPressed{false};
    std::function<void()> onClick;
};

struct DlcBanner {
    int refX{0};
    int refY{0};
    int x{0};
    int y{0};
    std::string name;
    std::string title;
};

class MainMenuScreen {
public:
    explicit MainMenuScreen(ScreenStack* owner = nullptr);
    ~MainMenuScreen();

    void initialize(int screenWidth, int screenHeight);
    void resize(int screenWidth, int screenHeight);
    void tick(double deltaTime);
    void render(Renderer& renderer, DRSReader* drs, int screenWidth, int screenHeight);

    bool onMouseDown(int button, int x, int y);
    bool onMouseUp(int button, int x, int y);
    bool onMouseMove(int x, int y);
    bool onKeyDown(int key);

    const std::string& getActiveRolloverText() const { return m_activeRolloverText; }
    void setOwner(ScreenStack* owner) { m_owner = owner; }

private:
    void setupButtons();
    void updateButtonLayout(int width, int height);
    void renderBackground(Renderer& renderer, DRSReader* drs, int width, int height);
    void renderHeraldryBanner(Renderer& renderer, int width);
    void renderButton(Renderer& renderer, DRSReader* drs, const MenuButton& btn);
    void renderDlcBanners(Renderer& renderer, DRSReader* drs);
    void renderRolloverPanel(Renderer& renderer, int width, int height);

    ScreenStack* m_owner{nullptr};
    std::vector<MenuButton> m_buttons;
    std::vector<DlcBanner> m_dlcBanners;
    std::string m_activeRolloverText;
    float m_animTime{0.0f};
    int m_screenWidth{1280};
    int m_screenHeight{720};
    bool m_classicDrsMode{false};
};

} // namespace aoe2
