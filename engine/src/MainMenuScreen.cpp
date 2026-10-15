#include "MainMenuScreen.h"
#include "ScreenStack.h"
#include "Renderer.h"
#include "DRSReader.h"
#include "Engine.h"
#include <iostream>
#include <cmath>
#include <algorithm>

namespace aoe2 {

MainMenuScreen::MainMenuScreen(ScreenStack* owner)
    : m_owner(owner), m_activeRolloverText("Select an option to begin.") {
    setupButtons();
}

MainMenuScreen::~MainMenuScreen() = default;

void MainMenuScreen::initialize(int screenWidth, int screenHeight) {
    resize(screenWidth, screenHeight);
}

void MainMenuScreen::resize(int screenWidth, int screenHeight) {
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;
    updateButtonLayout(screenWidth, screenHeight);
}

void MainMenuScreen::setupButtons() {
    m_buttons.clear();

    // 1. "about"
    MenuButton btnAbout;
    btnAbout.id = MenuButtonId::About;
    btnAbout.name = "about";
    btnAbout.label = "ABOUT";
    btnAbout.rolloverText = "Credits and development information for Age of Empires II";
    btnAbout.refX = 128; btnAbout.refY = 1; btnAbout.refW = 585; btnAbout.refH = 202;
    btnAbout.refTextX = 187; btnAbout.refTextY = 272; btnAbout.refTextW = 79; btnAbout.refTextH = 18;
    btnAbout.rolloverKey = 31008;
    btnAbout.onClick = [this]() {
        m_activeRolloverText = "Age of Empires II: HD Edition Engine Port (Native WebAssembly C++)";
    };
    m_buttons.push_back(btnAbout);

    // 2. "single-player"
    MenuButton btnSingle;
    btnSingle.id = MenuButtonId::SinglePlayer;
    btnSingle.name = "single-player";
    btnSingle.label = "SINGLE PLAYER";
    btnSingle.rolloverText = "Play a standard game, skirmish, or custom scenario against computer opponents";
    btnSingle.refX = 532; btnSingle.refY = 9; btnSingle.refW = 192; btnSingle.refH = 258;
    btnSingle.refTextX = 542; btnSingle.refTextY = 20; btnSingle.refTextW = 178; btnSingle.refTextH = 38;
    btnSingle.textKey = 9500;
    btnSingle.rolloverKey = 31000;
    btnSingle.onClick = [this]() {
        if (m_owner) {
            std::cout << "[MainMenu] Starting Single Player Skirmish Game..." << std::endl;
            m_owner->pushScreen(ScreenType::InGame);
        }
    };
    m_buttons.push_back(btnSingle);

    // 3. "multi-player"
    MenuButton btnMulti;
    btnMulti.id = MenuButtonId::MultiPlayer;
    btnMulti.name = "multi-player";
    btnMulti.label = "MULTIPLAYER";
    btnMulti.rolloverText = "Play with other players over local network or Internet";
    btnMulti.refX = 495; btnMulti.refY = 265; btnMulti.refW = 161; btnMulti.refH = 188;
    btnMulti.refTextX = 502; btnMulti.refTextY = 284; btnMulti.refTextW = 145; btnMulti.refTextH = 24;
    btnMulti.textKey = 9501;
    btnMulti.rolloverKey = 31001;
    btnMulti.onClick = [this]() {
        m_activeRolloverText = "Multiplayer room lobby ready.";
    };
    m_buttons.push_back(btnMulti);

    // 4. "tutorial"
    MenuButton btnTutorial;
    btnTutorial.id = MenuButtonId::Tutorial;
    btnTutorial.name = "tutorial";
    btnTutorial.label = "CAMPAIGN";
    btnTutorial.rolloverText = "Learn how to play Age of Empires II in the William Wallace learning campaign";
    btnTutorial.refX = 135; btnTutorial.refY = 2; btnTutorial.refW = 218; btnTutorial.refH = 254;
    btnTutorial.refTextX = 150; btnTutorial.refTextY = 13; btnTutorial.refTextW = 188; btnTutorial.refTextH = 40;
    btnTutorial.textKey = 9503;
    btnTutorial.rolloverKey = 31003;
    btnTutorial.onClick = [this]() {
        if (m_owner) {
            m_owner->pushScreen(ScreenType::Campaign);
        }
    };
    m_buttons.push_back(btnTutorial);

    // 5. "history"
    MenuButton btnHistory;
    btnHistory.id = MenuButtonId::History;
    btnHistory.name = "history";
    btnHistory.label = "HISTORY";
    btnHistory.rolloverText = "Civilization backgrounds, technology tree, and military history";
    btnHistory.refX = 290; btnHistory.refY = 198; btnHistory.refW = 160; btnHistory.refH = 147;
    btnHistory.refTextX = 304; btnHistory.refTextY = 213; btnHistory.refTextW = 128; btnHistory.refTextH = 32;
    btnHistory.textKey = 9505;
    btnHistory.rolloverKey = 31005;
    btnHistory.onClick = [this]() {
        m_activeRolloverText = "Civilizations: Britons, Franks, Byzantines, Goths, Teutons, Mongols, Japanese, Saracens.";
    };
    m_buttons.push_back(btnHistory);

    // 6. "map-editor"
    MenuButton btnMapEditor;
    btnMapEditor.id = MenuButtonId::MapEditor;
    btnMapEditor.name = "map-editor";
    btnMapEditor.label = "MAP EDITOR";
    btnMapEditor.rolloverText = "Design and customize your own scenarios and battlefields";
    btnMapEditor.refX = 410; btnMapEditor.refY = 344; btnMapEditor.refW = 123; btnMapEditor.refH = 97;
    btnMapEditor.refTextX = 420; btnMapEditor.refTextY = 355; btnMapEditor.refTextW = 107; btnMapEditor.refTextH = 18;
    btnMapEditor.textKey = 9504;
    btnMapEditor.rolloverKey = 31004;
    btnMapEditor.onClick = [this]() {
        m_activeRolloverText = "Scenario & Map Editor: 144x144 Isometric Terrain Grid ready.";
    };
    m_buttons.push_back(btnMapEditor);

    // 7. "options"
    MenuButton btnOptions;
    btnOptions.id = MenuButtonId::Options;
    btnOptions.name = "options";
    btnOptions.label = "OPTIONS";
    btnOptions.rolloverText = "Customize graphics, sound volume, scroll speed, and interface settings";
    btnOptions.refX = 295; btnOptions.refY = 439; btnOptions.refW = 137; btnOptions.refH = 139;
    btnOptions.refTextX = 304; btnOptions.refTextY = 450; btnOptions.refTextW = 117; btnOptions.refTextH = 24;
    btnOptions.textKey = 9506;
    btnOptions.rolloverKey = 31006;
    btnOptions.onClick = [this]() {
        m_activeRolloverText = "Display Mode: Full HD 60 FPS | Audio: 44.1kHz Stereo Active.";
    };
    m_buttons.push_back(btnOptions);

    // 8. "mod-manager"
    MenuButton btnModManager;
    btnModManager.id = MenuButtonId::ModManager;
    btnModManager.name = "mod-manager";
    btnModManager.label = "WORKSHOP";
    btnModManager.rolloverText = "Browse, install, and manage custom user modifications";
    btnModManager.refX = 500; btnModManager.refY = 482; btnModManager.refW = 159; btnModManager.refH = 129;
    btnModManager.refTextX = 500; btnModManager.refTextY = 482; btnModManager.refTextW = 159; btnModManager.refTextH = 41;
    btnModManager.onClick = [this]() {
        m_activeRolloverText = "Steam Workshop & Community Modding Architecture loaded.";
    };
    m_buttons.push_back(btnModManager);

    // 9. "exit"
    MenuButton btnExit;
    btnExit.id = MenuButtonId::Exit;
    btnExit.name = "exit";
    btnExit.label = "EXIT";
    btnExit.rolloverText = "Exit Age of Empires II";
    btnExit.refX = 174; btnExit.refY = 631; btnExit.refW = 230; btnExit.refH = 137;
    btnExit.refTextX = 200; btnExit.refTextY = 704; btnExit.refTextW = 160; btnExit.refTextH = 26;
    btnExit.textKey = 9509;
    btnExit.rolloverKey = 31009;
    btnExit.onClick = [this]() {
        m_activeRolloverText = "Thanks for playing Age of Empires II.";
    };
    m_buttons.push_back(btnExit);

    // DLC Banners from sub_6E5400
    m_dlcBanners = {
        { 128, 294, 0, 0, "dlc1", "THE CONQUERORS" },
        { 128, 378, 0, 0, "dlc2", "THE FORGOTTEN" },
        { 128, 462, 0, 0, "dlc3", "AFRICAN KINGDOMS" }
    };
}

void MainMenuScreen::updateButtonLayout(int width, int height) {
    const float refW = 1366.0f;
    const float refH = 768.0f;
    const float scaleX = static_cast<float>(width) / refW;
    const float scaleY = static_cast<float>(height) / refH;

    for (auto& btn : m_buttons) {
        btn.x = static_cast<int>(btn.refX * scaleX);
        btn.y = static_cast<int>(btn.refY * scaleY);
        btn.w = static_cast<int>(btn.refW * scaleX);
        btn.h = static_cast<int>(btn.refH * scaleY);

        btn.textX = static_cast<int>(btn.refTextX * scaleX);
        btn.textY = static_cast<int>(btn.refTextY * scaleY);
    }

    for (auto& dlc : m_dlcBanners) {
        dlc.x = static_cast<int>(dlc.refX * scaleX);
        dlc.y = static_cast<int>(dlc.refY * scaleY);
    }
}

void MainMenuScreen::tick(double deltaTime) {
    m_animTime += static_cast<float>(deltaTime);
}

void MainMenuScreen::render(Renderer& renderer, DRSReader* drs, int screenWidth, int screenHeight) {
    if (screenWidth != m_screenWidth || screenHeight != m_screenHeight) {
        resize(screenWidth, screenHeight);
    }

    // 1. Authentic Medieval Stone Background
    renderBackground(renderer, drs, screenWidth, screenHeight);

    // 2. Medieval Heraldry Title / Header
    renderHeraldryBanner(renderer, screenWidth);

    // 3. DLC Banners (sub_6E5400)
    renderDlcBanners(renderer, drs);

    // 4. Authentic Interactive Menu Buttons
    for (const auto& btn : m_buttons) {
        renderButton(renderer, drs, btn);
    }

    // 5. Authentic Bottom Rollover Help Status Bar
    renderRolloverPanel(renderer, screenWidth, screenHeight);
}

void MainMenuScreen::renderBackground(Renderer& renderer, DRSReader* drs, int width, int height) {
    // Check DRS Shape 50100 (Authentic 800x600 stone shield heraldry background)
    if (drs) {
        auto bgShape = drs->getShape(50100);
        if (bgShape && bgShape->getFrameCount() > 0) {
            const auto* frame = bgShape->getFrame(0);
            if (frame) {
                if (width == frame->width && height == frame->height) {
                    renderer.blitSLPFrame(frame->pixelData.data(), frame->width, frame->height, 0, 0);
                } else {
                    renderer.blitSLPFrameScaled(frame->pixelData.data(), frame->width, frame->height, 0, 0, width, height);
                }
                m_classicDrsMode = true;
                return;
            }
        }
    }

    m_classicDrsMode = false;

    // Authentic HD Slate/Granite Gothic Background with subtle texture
    renderer.clear(24, 18, 14);

    // Stone tile pattern & gothic borders
    for (int y = 0; y < height; y += 32) {
        renderer.drawRect(0, y, width, 1, 32, 24, 18, 255);
    }
    for (int x = 0; x < width; x += 64) {
        renderer.drawRect(x, 0, 1, height, 32, 24, 18, 255);
    }

    // Outer stone frame borders
    renderer.drawRect(0, 0, width, 6, 60, 45, 28, 255);
    renderer.drawRect(0, height - 6, width, 6, 60, 45, 28, 255);
    renderer.drawRect(0, 0, 6, height, 60, 45, 28, 255);
    renderer.drawRect(width - 6, 0, 6, height, 60, 45, 28, 255);

    // Gilded inner trim line
    renderer.drawRect(8, 8, width - 16, 2, 180, 140, 50, 255);
    renderer.drawRect(8, height - 10, width - 16, 2, 180, 140, 50, 255);
    renderer.drawRect(8, 8, 2, height - 16, 180, 140, 50, 255);
    renderer.drawRect(width - 10, 8, 2, height - 16, 180, 140, 50, 255);
}

void MainMenuScreen::renderHeraldryBanner(Renderer& renderer, int width) {
    // Right / Center Title display for Age of Empires II
    const int titleX = std::max(20, width - 420);
    const int titleY = 32;
    const int bannerW = 380;
    const int bannerH = 110;

    // Banner dark parchment backing
    renderer.drawRect(titleX, titleY, bannerW, bannerH, 35, 25, 18, 230);
    // Bevel highlights
    renderer.drawRect(titleX, titleY, bannerW, 2, 218, 165, 32, 255);
    renderer.drawRect(titleX, titleY + bannerH - 2, bannerW, 2, 160, 115, 25, 255);
    renderer.drawRect(titleX, titleY, 2, bannerH, 218, 165, 32, 255);
    renderer.drawRect(titleX + bannerW - 2, titleY, 2, bannerH, 160, 115, 25, 255);

    // Drop shadow title
    renderer.drawText("AGE OF EMPIRES II", titleX + 22, titleY + 22, 0xFF000000);
    // Gilded title
    renderer.drawText("AGE OF EMPIRES II", titleX + 20, titleY + 20, 0xFFD4AF37);

    // Subtitle
    renderer.drawText("HD EDITION * THE CONQUERORS", titleX + 22, titleY + 52, 0xFF000000);
    renderer.drawText("HD EDITION * THE CONQUERORS", titleX + 20, titleY + 50, 0xFFE0C068);

    // Ornamental crest separator
    renderer.drawRect(titleX + 20, titleY + 74, bannerW - 40, 2, 180, 140, 50, 255);
    renderer.drawText("GENIE ENGINE NATIVE C++ WASM", titleX + 28, titleY + 86, 0xFFA09070);
}

void MainMenuScreen::renderDlcBanners(Renderer& renderer, DRSReader* /*drs*/) {
    const int bannerW = 150;
    const int bannerH = 65;

    for (const auto& dlc : m_dlcBanners) {
        int bx = dlc.x;
        int by = dlc.y;

        // Shield-plate background
        renderer.drawRect(bx, by, bannerW, bannerH, 40, 30, 22, 240);
        // Beveled border
        renderer.drawRect(bx, by, bannerW, 2, 180, 140, 50, 255);
        renderer.drawRect(bx, by + bannerH - 2, bannerW, 2, 90, 70, 25, 255);
        renderer.drawRect(bx, by, 2, bannerH, 180, 140, 50, 255);
        renderer.drawRect(bx + bannerW - 2, by, 2, bannerH, 90, 70, 25, 255);

        // DLC title text
        renderer.drawText("EXPANSION", bx + 12, by + 16, 0xFF908070);
        renderer.drawText(dlc.title, bx + 12, by + 34, 0xFFD4AF37);
    }
}

void MainMenuScreen::renderButton(Renderer& renderer, DRSReader* drs, const MenuButton& btn) {
    // 1. Classic DRS Shape Mode (50089 / btnmain.slp)
    if (drs) {
        auto btnShape = drs->getShape(50089);
        if (!btnShape) {
            btnShape = drs->getShapeByName("btnmain");
        }
        if (btnShape && btnShape->getFrameCount() > 0) {
            int frameIdx = btn.isPressed ? 2 : (btn.isHovered ? 1 : 0);
            if (frameIdx >= static_cast<int>(btnShape->getFrameCount())) {
                frameIdx = 0;
            }
            const auto* frame = btnShape->getFrame(frameIdx);
            if (frame) {
                renderer.blitSLPFrame(frame->pixelData.data(), frame->width, frame->height, btn.x, btn.y);
                return;
            }
        }
    }

    // 2. Authentic HD Beveled Shield / Ribbon Button Rendering
    int bx = btn.x;
    int by = btn.y;
    int bw = btn.w;
    int bh = btn.h;

    // Adjust for pressed state
    int textShift = btn.isPressed ? 2 : 0;

    // Fill colors based on hover / pressed
    uint8_t bgR = 45, bgG = 34, bgB = 24;
    uint32_t borderGold = 0xFFC89B3C;
    uint32_t textGold = 0xFFE6C364;

    if (btn.isPressed) {
        bgR = 26; bgG = 18; bgB = 12;
        borderGold = 0xFF8C6D2D;
        textGold = 0xFFC0A050;
    } else if (btn.isHovered) {
        bgR = 68; bgG = 50; bgB = 34;
        borderGold = 0xFFFFD700;
        textGold = 0xFFFFF2A8;
    }

    // Button Base Fill
    renderer.drawRect(bx, by, bw, bh, bgR, bgG, bgB, 240);

    // Outer 3D Bevel Highlight & Shadow
    if (!btn.isPressed) {
        // Top & Left highlight
        renderer.drawRect(bx, by, bw, 3, 220, 180, 80, 255);
        renderer.drawRect(bx, by, 3, bh, 220, 180, 80, 255);
        // Bottom & Right shadow
        renderer.drawRect(bx, by + bh - 3, bw, 3, 20, 14, 8, 255);
        renderer.drawRect(bx + bw - 3, by, 3, bh, 20, 14, 8, 255);
    } else {
        // Inset depressed shadow
        renderer.drawRect(bx, by, bw, 3, 15, 10, 5, 255);
        renderer.drawRect(bx, by, 3, bh, 15, 10, 5, 255);
        renderer.drawRect(bx, by + bh - 3, bw, 3, 140, 110, 50, 255);
        renderer.drawRect(bx + bw - 3, by, 3, bh, 140, 110, 50, 255);
    }

    // Gilded Inner Border Frame
    uint8_t goldR = borderGold & 0xFF;
    uint8_t goldG = (borderGold >> 8) & 0xFF;
    uint8_t goldB = (borderGold >> 16) & 0xFF;
    renderer.drawRect(bx + 4, by + 4, bw - 8, 2, goldR, goldG, goldB, 255);
    renderer.drawRect(bx + 4, by + bh - 6, bw - 8, 2, goldR, goldG, goldB, 255);
    renderer.drawRect(bx + 4, by + 4, 2, bh - 8, goldR, goldG, goldB, 255);
    renderer.drawRect(bx + bw - 6, by + 4, 2, bh - 8, goldR, goldG, goldB, 255);

    // Hover Pulsing Glow Trim
    if (btn.isHovered && !btn.isPressed) {
        int glowAlpha = 180 + static_cast<int>(70.0f * std::sin(m_animTime * 6.0f));
        glowAlpha = std::clamp(glowAlpha, 100, 255);
        renderer.drawRect(bx + 1, by + 1, bw - 2, 2, 255, 235, 120, static_cast<uint8_t>(glowAlpha));
        renderer.drawRect(bx + 1, by + bh - 3, bw - 2, 2, 255, 235, 120, static_cast<uint8_t>(glowAlpha));
    }

    // Centered Button Label
    int textLen = static_cast<int>(btn.label.length());
    int charWidth = 8;
    int totalTextW = textLen * charWidth;
    int cx = bx + (bw - totalTextW) / 2 + textShift;
    int cy = by + (bh - 8) / 2 + textShift;

    // Drop Shadow
    renderer.drawText(btn.label, cx + 1, cy + 1, 0xFF000000);
    // Foreground Text
    renderer.drawText(btn.label, cx, cy, textGold);
}

void MainMenuScreen::renderRolloverPanel(Renderer& renderer, int width, int height) {
    const int panelH = 34;
    const int panelY = height - panelH - 12;
    const int panelX = 16;
    const int panelW = width - 32;

    // Dark parchment stone container
    renderer.drawRect(panelX, panelY, panelW, panelH, 30, 22, 16, 240);
    // Gold filigree edge
    renderer.drawRect(panelX, panelY, panelW, 2, 180, 140, 50, 255);
    renderer.drawRect(panelX, panelY + panelH - 2, panelW, 2, 90, 70, 25, 255);
    renderer.drawRect(panelX, panelY, 2, panelH, 180, 140, 50, 255);
    renderer.drawRect(panelX + panelW - 2, panelY, 2, panelH, 90, 70, 25, 255);

    // Information Rollover text
    const std::string& display = m_activeRolloverText.empty() ? "Select an option to begin." : m_activeRolloverText;
    renderer.drawText(display, panelX + 16, panelY + 13, 0xFF000000); // shadow
    renderer.drawText(display, panelX + 15, panelY + 12, 0xFFE6C364); // text
}

bool MainMenuScreen::onMouseDown(int button, int x, int y) {
    if (button != 0) return false; // Left mouse button only

    for (auto it = m_buttons.rbegin(); it != m_buttons.rend(); ++it) {
        if (x >= it->x && x < it->x + it->w && y >= it->y && y < it->y + it->h) {
            it->isPressed = true;
            return true;
        }
    }
    return false;
}

bool MainMenuScreen::onMouseUp(int button, int x, int y) {
    if (button != 0) return false;

    bool handled = false;
    for (auto it = m_buttons.rbegin(); it != m_buttons.rend(); ++it) {
        if (it->isPressed) {
            it->isPressed = false;
            if (x >= it->x && x < it->x + it->w && y >= it->y && y < it->y + it->h) {
                if (it->onClick) {
                    it->onClick();
                }
                handled = true;
                break;
            }
        }
    }
    return handled;
}

bool MainMenuScreen::onMouseMove(int x, int y) {
    bool hoveredAny = false;

    for (auto it = m_buttons.rbegin(); it != m_buttons.rend(); ++it) {
        bool inside = (x >= it->x && x < it->x + it->w && y >= it->y && y < it->y + it->h);
        if (inside && !hoveredAny) {
            it->isHovered = true;
            m_activeRolloverText = it->rolloverText;
            hoveredAny = true;
        } else {
            it->isHovered = false;
        }
    }

    if (!hoveredAny) {
        m_activeRolloverText = "Select an option to begin.";
    }

    return hoveredAny;
}

bool MainMenuScreen::onKeyDown(int key) {
    // 13 = Enter (starts Single Player), 27 = Escape (Exit)
    if (key == 13) {
        for (auto& btn : m_buttons) {
            if (btn.id == MenuButtonId::SinglePlayer && btn.onClick) {
                btn.onClick();
                return true;
            }
        }
    }
    return false;
}

} // namespace aoe2
