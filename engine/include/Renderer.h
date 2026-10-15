#pragma once

#include <vector>
#include <cstdint>
#include <string>

namespace aoe2 {

struct ColorRGBA {
    uint8_t r{0};
    uint8_t g{0};
    uint8_t b{0};
    uint8_t a{255};
};

struct PlayerPalette {
    ColorRGBA colors[256];
};

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool initialize(int width, int height);
    void resize(int width, int height);
    void beginFrame();
    void endFrame();

    // Palette operations
    void loadPaletteFromBuffer(const uint8_t* palData, size_t size);
    void setPlayerColor(int playerIndex, int baseColorId);

    // Pixel framebuffer operations for WebAssembly direct transfer
    void clear(uint8_t r, uint8_t g, uint8_t b);
    void blitSLPFrame(const uint8_t* pixels, int width, int height, 
                      int screenX, int screenY, int playerIndex = 0, bool isShadow = false);
    void blitSLPFrameScaled(const uint8_t* pixels, int srcW, int srcH, 
                           int dstX, int dstY, int dstW, int dstH, int playerIndex = 0);
    
    void drawRect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    void drawText(const std::string& text, int x, int y, uint32_t colorHex);

    // Framebuffer pointer for Emscripten / WebGL transfer
    const uint32_t* getFramebuffer() const { return m_framebuffer.data(); }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

private:
    int m_width{1280};
    int m_height{720};
    std::vector<uint32_t> m_framebuffer;
    ColorRGBA m_basePalette[256];
    ColorRGBA m_playerPalettes[8][256]; // 8 Player color variations
};

} // namespace aoe2
