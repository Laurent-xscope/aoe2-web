#include "Renderer.h"
#include "Texture.h"
#include <cstring>
#include <algorithm>
#include <sstream>
#include <iostream>

namespace aoe2 {

Renderer::Renderer() {
    // Initialize default standard Genie palette grayscale fallback
    for (int i = 0; i < 256; ++i) {
        m_basePalette[i] = { static_cast<uint8_t>(i), static_cast<uint8_t>(i), static_cast<uint8_t>(i), 255 };
        m_menuPalette[i] = m_basePalette[i];
    }
    setPlayerColor(0, 0);
}

Renderer::~Renderer() = default;

bool Renderer::initialize(int width, int height) {
    resize(width, height);
    return true;
}

void Renderer::resize(int width, int height) {
    m_width = width;
    m_height = height;
    m_framebuffer.resize(static_cast<size_t>(m_width) * m_height, 0xFF000000);
}

void Renderer::beginFrame() {
    std::fill(m_framebuffer.begin(), m_framebuffer.end(), 0xFF000000);
}

void Renderer::endFrame() {
    // Framebuffer ready for WebGL texture upload or 2D canvas putImageData
}

void Renderer::loadPaletteFromBuffer(const uint8_t* palData, size_t size) {
    if (!palData || size == 0) return;

    if (size >= 8 && std::memcmp(palData, "JASC-PAL", 8) == 0) {
        std::string text(reinterpret_cast<const char*>(palData), size);
        std::istringstream iss(text);
        std::string line;
        int headerCount = 0;
        int colorCount = 0;

        while (std::getline(iss, line) && colorCount < 256) {
            while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) {
                line.pop_back();
            }
            if (line.empty()) continue;

            if (headerCount < 3) {
                headerCount++;
                continue;
            }

            std::istringstream liness(line);
            int r = 0, g = 0, b = 0;
            if (liness >> r >> g >> b) {
                m_basePalette[colorCount].r = static_cast<uint8_t>(std::clamp(r, 0, 255));
                m_basePalette[colorCount].g = static_cast<uint8_t>(std::clamp(g, 0, 255));
                m_basePalette[colorCount].b = static_cast<uint8_t>(std::clamp(b, 0, 255));
                m_basePalette[colorCount].a = 255;
                colorCount++;
            }
        }
    } else if (size >= 256 * 3) {
        for (int i = 0; i < 256 && (i * 3 + 2 < static_cast<int>(size)); ++i) {
            m_basePalette[i].r = palData[i * 3 + 0];
            m_basePalette[i].g = palData[i * 3 + 1];
            m_basePalette[i].b = palData[i * 3 + 2];
            m_basePalette[i].a = 255;
        }
    }

    setPlayerColor(0, 0);
}

void Renderer::loadMenuPaletteFromBuffer(const uint8_t* palData, size_t size) {
    if (!palData || size == 0) return;

    if (size >= 8 && std::memcmp(palData, "JASC-PAL", 8) == 0) {
        std::string text(reinterpret_cast<const char*>(palData), size);
        std::istringstream iss(text);
        std::string line;
        int headerCount = 0;
        int colorCount = 0;

        while (std::getline(iss, line) && colorCount < 256) {
            while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) {
                line.pop_back();
            }
            if (line.empty()) continue;

            if (headerCount < 3) {
                headerCount++;
                continue;
            }

            std::istringstream liness(line);
            int r = 0, g = 0, b = 0;
            if (liness >> r >> g >> b) {
                m_menuPalette[colorCount].r = static_cast<uint8_t>(std::clamp(r, 0, 255));
                m_menuPalette[colorCount].g = static_cast<uint8_t>(std::clamp(g, 0, 255));
                m_menuPalette[colorCount].b = static_cast<uint8_t>(std::clamp(b, 0, 255));
                m_menuPalette[colorCount].a = 255;
                colorCount++;
            }
        }
    } else if (size >= 256 * 3) {
        for (int i = 0; i < 256 && (i * 3 + 2 < static_cast<int>(size)); ++i) {
            m_menuPalette[i].r = palData[i * 3 + 0];
            m_menuPalette[i].g = palData[i * 3 + 1];
            m_menuPalette[i].b = palData[i * 3 + 2];
            m_menuPalette[i].a = 255;
        }
    }
}

void Renderer::setPlayerColor(int /*playerIndex*/, int /*baseColorId*/) {
    // Generate player palette variations
    // In AoE2, color indices 16-23 are the player color ramp
    // 0: Blue, 1: Red, 2: Green, 3: Yellow, 4: Cyan, 5: Purple, 6: Grey, 7: Orange
    const uint8_t playerRamps[8][8][3] = {
        // Blue
        { {15,35,115}, {25,55,145}, {35,75,175}, {45,95,205}, {65,115,225}, {95,145,240}, {130,175,250}, {170,205,255} },
        // Red
        { {120,20,20}, {150,25,25}, {180,35,35}, {210,45,45}, {230,65,65}, {245,95,95}, {255,130,130}, {255,170,170} },
        // Green
        { {20,90,20}, {25,120,25}, {35,150,35}, {45,180,45}, {65,210,65}, {95,230,95}, {130,245,130}, {170,255,170} },
        // Yellow
        { {120,110,15}, {150,140,20}, {180,170,25}, {210,200,35}, {230,220,55}, {245,235,85}, {255,245,120}, {255,255,160} },
        // Cyan
        { {15,100,110}, {20,130,140}, {25,160,170}, {35,190,200}, {55,215,225}, {85,235,240}, {120,245,250}, {160,255,255} },
        // Purple
        { {80,15,100}, {110,20,130}, {140,25,160}, {170,35,190}, {195,55,215}, {215,85,235}, {235,120,245}, {250,160,255} },
        // Grey
        { {60,60,60}, {85,85,85}, {110,110,110}, {135,135,135}, {160,160,160}, {185,185,185}, {210,210,210}, {235,235,235} },
        // Orange
        { {140,65,15}, {170,85,20}, {200,105,25}, {225,125,35}, {240,150,55}, {250,175,85}, {255,200,120}, {255,225,160} }
    };

    for (int p = 0; p < 8; ++p) {
        std::memcpy(m_playerPalettes[p], m_basePalette, sizeof(m_basePalette));
        for (int r = 0; r < 8; ++r) {
            m_playerPalettes[p][16 + r].r = playerRamps[p][r][0];
            m_playerPalettes[p][16 + r].g = playerRamps[p][r][1];
            m_playerPalettes[p][16 + r].b = playerRamps[p][r][2];
            m_playerPalettes[p][16 + r].a = 255;
        }
    }
}

void Renderer::clear(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t color = 0xFF000000 | (static_cast<uint32_t>(b) << 16) | 
                     (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(r);
    std::fill(m_framebuffer.begin(), m_framebuffer.end(), color);
}

void Renderer::blitSLPFrame(const uint8_t* pixels, int width, int height, 
                           int screenX, int screenY, int playerIndex, bool isShadow) {
    if (!pixels || width <= 0 || height <= 0) return;

    playerIndex = std::clamp(playerIndex, 0, 7);
    const ColorRGBA* pal = m_useMenuPalette ? m_menuPalette : m_playerPalettes[playerIndex];

    for (int y = 0; y < height; ++y) {
        int dstY = screenY + y;
        if (dstY < 0 || dstY >= m_height) continue;

        for (int x = 0; x < width; ++x) {
            int dstX = screenX + x;
            if (dstX < 0 || dstX >= m_width) continue;

            uint8_t colorIdx = pixels[y * width + x];
            if (colorIdx == 0xFF) continue; // Transparent

            size_t dstIdx = static_cast<size_t>(dstY) * m_width + dstX;

            if (isShadow) {
                // 50% Shadow darken blend
                uint32_t orig = m_framebuffer[dstIdx];
                uint8_t b = (orig >> 16) & 0xFF;
                uint8_t g = (orig >> 8) & 0xFF;
                uint8_t r = orig & 0xFF;
                m_framebuffer[dstIdx] = 0xFF000000 | ((b >> 1) << 16) | ((g >> 1) << 8) | (r >> 1);
            } else {
                const ColorRGBA& c = pal[colorIdx];
                m_framebuffer[dstIdx] = 0xFF000000 | (static_cast<uint32_t>(c.b) << 16) | 
                                        (static_cast<uint32_t>(c.g) << 8) | static_cast<uint32_t>(c.r);
            }
        }
    }
}

void Renderer::blitSLPFrameScaled(const uint8_t* pixels, int srcW, int srcH, 
                                  int dstX, int dstY, int dstW, int dstH, int playerIndex) {
    if (!pixels || srcW <= 0 || srcH <= 0 || dstW <= 0 || dstH <= 0) return;

    playerIndex = std::clamp(playerIndex, 0, 7);
    const ColorRGBA* pal = m_useMenuPalette ? m_menuPalette : m_playerPalettes[playerIndex];

    for (int y = 0; y < dstH; ++y) {
        int screenY = dstY + y;
        if (screenY < 0 || screenY >= m_height) continue;

        int srcY = (y * srcH) / dstH;
        if (srcY >= srcH) srcY = srcH - 1;

        for (int x = 0; x < dstW; ++x) {
            int screenX = dstX + x;
            if (screenX < 0 || screenX >= m_width) continue;

            int srcX = (x * srcW) / dstW;
            if (srcX >= srcW) srcX = srcW - 1;

            uint8_t colorIdx = pixels[srcY * srcW + srcX];
            if (colorIdx == 0xFF) continue; // Transparent

            size_t dstIdx = static_cast<size_t>(screenY) * m_width + screenX;
            const ColorRGBA& c = pal[colorIdx];
            m_framebuffer[dstIdx] = 0xFF000000 | (static_cast<uint32_t>(c.b) << 16) | 
                                    (static_cast<uint32_t>(c.g) << 8) | static_cast<uint32_t>(c.r);
        }
    }
}

void Renderer::blitTexture(const Texture& tex, int dstX, int dstY, float alpha) {
    if (!tex.isValid()) return;
    int tw = tex.width;
    int th = tex.height;

    uint32_t alphaFactor = static_cast<uint32_t>(std::clamp(alpha, 0.0f, 1.0f) * 255.0f);

    for (int y = 0; y < th; ++y) {
        int screenY = dstY + y;
        if (screenY < 0 || screenY >= m_height) continue;

        for (int x = 0; x < tw; ++x) {
            int screenX = dstX + x;
            if (screenX < 0 || screenX >= m_width) continue;

            uint32_t srcPixel = tex.pixels[y * tw + x];
            uint8_t srcA = (srcPixel >> 24) & 0xFF;
            if (srcA == 0) continue; // Completely transparent

            if (alphaFactor < 255) {
                srcA = static_cast<uint8_t>((srcA * alphaFactor) / 255);
            }

            size_t dstIdx = static_cast<size_t>(screenY) * m_width + screenX;

            if (srcA >= 250) {
                m_framebuffer[dstIdx] = srcPixel;
            } else {
                uint32_t dstPixel = m_framebuffer[dstIdx];
                uint8_t invA = 255 - srcA;

                uint8_t sr = srcPixel & 0xFF;
                uint8_t sg = (srcPixel >> 8) & 0xFF;
                uint8_t sb = (srcPixel >> 16) & 0xFF;

                uint8_t dr = dstPixel & 0xFF;
                uint8_t dg = (dstPixel >> 8) & 0xFF;
                uint8_t db = (dstPixel >> 16) & 0xFF;

                uint8_t outR = static_cast<uint8_t>((sr * srcA + dr * invA) / 255);
                uint8_t outG = static_cast<uint8_t>((sg * srcA + dg * invA) / 255);
                uint8_t outB = static_cast<uint8_t>((sb * srcA + db * invA) / 255);

                m_framebuffer[dstIdx] = 0xFF000000 | (static_cast<uint32_t>(outB) << 16) | 
                                        (static_cast<uint32_t>(outG) << 8) | outR;
            }
        }
    }
}

void Renderer::blitTextureScaled(const Texture& tex, int dstX, int dstY, int dstW, int dstH, float alpha) {
    if (!tex.isValid() || dstW <= 0 || dstH <= 0) return;
    int srcW = tex.width;
    int srcH = tex.height;

    uint32_t alphaFactor = static_cast<uint32_t>(std::clamp(alpha, 0.0f, 1.0f) * 255.0f);

    for (int y = 0; y < dstH; ++y) {
        int screenY = dstY + y;
        if (screenY < 0 || screenY >= m_height) continue;

        int srcY = (y * srcH) / dstH;
        if (srcY >= srcH) srcY = srcH - 1;

        for (int x = 0; x < dstW; ++x) {
            int screenX = dstX + x;
            if (screenX < 0 || screenX >= m_width) continue;

            int srcX = (x * srcW) / dstW;
            if (srcX >= srcW) srcX = srcW - 1;

            uint32_t srcPixel = tex.pixels[srcY * srcW + srcX];
            uint8_t srcA = (srcPixel >> 24) & 0xFF;
            if (srcA == 0) continue;

            if (alphaFactor < 255) {
                srcA = static_cast<uint8_t>((srcA * alphaFactor) / 255);
            }

            size_t dstIdx = static_cast<size_t>(screenY) * m_width + screenX;

            if (srcA >= 250) {
                m_framebuffer[dstIdx] = srcPixel;
            } else {
                uint32_t dstPixel = m_framebuffer[dstIdx];
                uint8_t invA = 255 - srcA;

                uint8_t sr = srcPixel & 0xFF;
                uint8_t sg = (srcPixel >> 8) & 0xFF;
                uint8_t sb = (srcPixel >> 16) & 0xFF;

                uint8_t dr = dstPixel & 0xFF;
                uint8_t dg = (dstPixel >> 8) & 0xFF;
                uint8_t db = (dstPixel >> 16) & 0xFF;

                uint8_t outR = static_cast<uint8_t>((sr * srcA + dr * invA) / 255);
                uint8_t outG = static_cast<uint8_t>((sg * srcA + dg * invA) / 255);
                uint8_t outB = static_cast<uint8_t>((sb * srcA + db * invA) / 255);

                m_framebuffer[dstIdx] = 0xFF000000 | (static_cast<uint32_t>(outB) << 16) | 
                                        (static_cast<uint32_t>(outG) << 8) | outR;
            }
        }
    }
}

void Renderer::drawRect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    uint32_t color = (static_cast<uint32_t>(a) << 24) | (static_cast<uint32_t>(b) << 16) | 
                     (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(r);

    for (int cy = std::max(0, y); cy < std::min(m_height, y + h); ++cy) {
        for (int cx = std::max(0, x); cx < std::min(m_width, x + w); ++cx) {
            m_framebuffer[static_cast<size_t>(cy) * m_width + cx] = color;
        }
    }
}

// Standard 8x8 font glyphs lookup for in-engine HUD text rendering
static const uint8_t* getGlyph8x8(char c) {
    static const uint8_t s_empty[8] = {0,0,0,0,0,0,0,0};
    switch (c) {
        case '0': { static const uint8_t g[8] = {0x3C, 0x66, 0x6E, 0x76, 0x66, 0x66, 0x3C, 0x00}; return g; }
        case '1': { static const uint8_t g[8] = {0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x00}; return g; }
        case '2': { static const uint8_t g[8] = {0x3C, 0x66, 0x06, 0x0C, 0x18, 0x30, 0x7E, 0x00}; return g; }
        case '3': { static const uint8_t g[8] = {0x7E, 0x0C, 0x18, 0x0C, 0x06, 0x66, 0x3C, 0x00}; return g; }
        case '4': { static const uint8_t g[8] = {0x0C, 0x1C, 0x3C, 0x6C, 0x7E, 0x0C, 0x0C, 0x00}; return g; }
        case '5': { static const uint8_t g[8] = {0x7E, 0x60, 0x7C, 0x06, 0x06, 0x66, 0x3C, 0x00}; return g; }
        case '6': { static const uint8_t g[8] = {0x1C, 0x30, 0x60, 0x7C, 0x66, 0x66, 0x3C, 0x00}; return g; }
        case '7': { static const uint8_t g[8] = {0x7E, 0x06, 0x0C, 0x18, 0x30, 0x30, 0x30, 0x00}; return g; }
        case '8': { static const uint8_t g[8] = {0x3C, 0x66, 0x66, 0x3C, 0x66, 0x66, 0x3C, 0x00}; return g; }
        case '9': { static const uint8_t g[8] = {0x3C, 0x66, 0x66, 0x3E, 0x06, 0x0C, 0x38, 0x00}; return g; }
        case 'A': { static const uint8_t g[8] = {0x18, 0x3C, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00}; return g; }
        case 'B': { static const uint8_t g[8] = {0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x7C, 0x00}; return g; }
        case 'C': { static const uint8_t g[8] = {0x3C, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3C, 0x00}; return g; }
        case 'D': { static const uint8_t g[8] = {0x78, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0x78, 0x00}; return g; }
        case 'E': { static const uint8_t g[8] = {0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x7E, 0x00}; return g; }
        case 'F': { static const uint8_t g[8] = {0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x60, 0x00}; return g; }
        case 'G': { static const uint8_t g[8] = {0x3C, 0x66, 0x60, 0x6E, 0x66, 0x66, 0x3A, 0x00}; return g; }
        case 'H': { static const uint8_t g[8] = {0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00}; return g; }
        case 'I': { static const uint8_t g[8] = {0x3C, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00}; return g; }
        case 'J': { static const uint8_t g[8] = {0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x6C, 0x38, 0x00}; return g; }
        case 'K': { static const uint8_t g[8] = {0x66, 0x6C, 0x78, 0x70, 0x78, 0x6C, 0x66, 0x00}; return g; }
        case 'L': { static const uint8_t g[8] = {0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7E, 0x00}; return g; }
        case 'M': { static const uint8_t g[8] = {0x63, 0x77, 0x7F, 0x6B, 0x63, 0x63, 0x63, 0x00}; return g; }
        case 'N': { static const uint8_t g[8] = {0x66, 0x76, 0x7E, 0x7E, 0x6E, 0x66, 0x66, 0x00}; return g; }
        case 'O': { static const uint8_t g[8] = {0x3C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00}; return g; }
        case 'P': { static const uint8_t g[8] = {0x7C, 0x66, 0x66, 0x7C, 0x60, 0x60, 0x60, 0x00}; return g; }
        case 'Q': { static const uint8_t g[8] = {0x3C, 0x66, 0x66, 0x66, 0x6A, 0x3C, 0x0E, 0x00}; return g; }
        case 'R': { static const uint8_t g[8] = {0x7C, 0x66, 0x66, 0x7C, 0x78, 0x6C, 0x66, 0x00}; return g; }
        case 'S': { static const uint8_t g[8] = {0x3C, 0x66, 0x60, 0x3C, 0x06, 0x66, 0x3C, 0x00}; return g; }
        case 'T': { static const uint8_t g[8] = {0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00}; return g; }
        case 'U': { static const uint8_t g[8] = {0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00}; return g; }
        case 'V': { static const uint8_t g[8] = {0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x00}; return g; }
        case 'W': { static const uint8_t g[8] = {0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00}; return g; }
        case 'X': { static const uint8_t g[8] = {0x66, 0x66, 0x3C, 0x18, 0x3C, 0x66, 0x66, 0x00}; return g; }
        case 'Y': { static const uint8_t g[8] = {0x66, 0x66, 0x66, 0x3C, 0x18, 0x18, 0x18, 0x00}; return g; }
        case 'Z': { static const uint8_t g[8] = {0x7E, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x7E, 0x00}; return g; }
        case 'a': { static const uint8_t g[8] = {0x00, 0x00, 0x3C, 0x06, 0x3E, 0x66, 0x3B, 0x00}; return g; }
        case 'b': { static const uint8_t g[8] = {0x60, 0x60, 0x7C, 0x66, 0x66, 0x66, 0x7C, 0x00}; return g; }
        case 'c': { static const uint8_t g[8] = {0x00, 0x00, 0x3C, 0x66, 0x60, 0x66, 0x3C, 0x00}; return g; }
        case 'd': { static const uint8_t g[8] = {0x06, 0x06, 0x3E, 0x66, 0x66, 0x66, 0x3E, 0x00}; return g; }
        case 'e': { static const uint8_t g[8] = {0x00, 0x00, 0x3C, 0x66, 0x7E, 0x60, 0x3C, 0x00}; return g; }
        case 'f': { static const uint8_t g[8] = {0x0E, 0x18, 0x7E, 0x18, 0x18, 0x18, 0x18, 0x00}; return g; }
        case 'g': { static const uint8_t g[8] = {0x00, 0x00, 0x3B, 0x66, 0x66, 0x3E, 0x06, 0x3C}; return g; }
        case 'h': { static const uint8_t g[8] = {0x60, 0x60, 0x7C, 0x66, 0x66, 0x66, 0x66, 0x00}; return g; }
        case 'i': { static const uint8_t g[8] = {0x18, 0x00, 0x38, 0x18, 0x18, 0x18, 0x3C, 0x00}; return g; }
        case 'j': { static const uint8_t g[8] = {0x06, 0x00, 0x0E, 0x06, 0x06, 0x66, 0x3C, 0x00}; return g; }
        case 'k': { static const uint8_t g[8] = {0x60, 0x60, 0x66, 0x6C, 0x78, 0x6C, 0x66, 0x00}; return g; }
        case 'l': { static const uint8_t g[8] = {0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00}; return g; }
        case 'm': { static const uint8_t g[8] = {0x00, 0x00, 0x66, 0x7F, 0x7F, 0x6B, 0x63, 0x00}; return g; }
        case 'n': { static const uint8_t g[8] = {0x00, 0x00, 0x7C, 0x66, 0x66, 0x66, 0x66, 0x00}; return g; }
        case 'o': { static const uint8_t g[8] = {0x00, 0x00, 0x3C, 0x66, 0x66, 0x66, 0x3C, 0x00}; return g; }
        case 'p': { static const uint8_t g[8] = {0x00, 0x00, 0x7C, 0x66, 0x66, 0x7C, 0x60, 0x60}; return g; }
        case 'q': { static const uint8_t g[8] = {0x00, 0x00, 0x3E, 0x66, 0x66, 0x3E, 0x06, 0x06}; return g; }
        case 'r': { static const uint8_t g[8] = {0x00, 0x00, 0x7C, 0x66, 0x60, 0x60, 0x60, 0x00}; return g; }
        case 's': { static const uint8_t g[8] = {0x00, 0x00, 0x3E, 0x60, 0x3C, 0x06, 0x7C, 0x00}; return g; }
        case 't': { static const uint8_t g[8] = {0x18, 0x18, 0x7E, 0x18, 0x18, 0x18, 0x0E, 0x00}; return g; }
        case 'u': { static const uint8_t g[8] = {0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x3E, 0x00}; return g; }
        case 'v': { static const uint8_t g[8] = {0x00, 0x00, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x00}; return g; }
        case 'w': { static const uint8_t g[8] = {0x00, 0x00, 0x63, 0x6B, 0x7F, 0x7F, 0x36, 0x00}; return g; }
        case 'x': { static const uint8_t g[8] = {0x00, 0x00, 0x66, 0x3C, 0x18, 0x3C, 0x66, 0x00}; return g; }
        case 'y': { static const uint8_t g[8] = {0x00, 0x00, 0x66, 0x66, 0x66, 0x3E, 0x06, 0x3C}; return g; }
        case 'z': { static const uint8_t g[8] = {0x00, 0x00, 0x7E, 0x0C, 0x18, 0x30, 0x7E, 0x00}; return g; }
        case ':': { static const uint8_t g[8] = {0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00, 0x00}; return g; }
        case '/': { static const uint8_t g[8] = {0x06, 0x0C, 0x18, 0x30, 0x60, 0xC0, 0x80, 0x00}; return g; }
        case ' ': { static const uint8_t g[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; return g; }
        case '-': { static const uint8_t g[8] = {0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00}; return g; }
        case '+': { static const uint8_t g[8] = {0x00, 0x18, 0x18, 0x7E, 0x18, 0x18, 0x00, 0x00}; return g; }
        case '.': { static const uint8_t g[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00}; return g; }
        case '(': { static const uint8_t g[8] = {0x0C, 0x18, 0x30, 0x30, 0x30, 0x18, 0x0C, 0x00}; return g; }
        case ')': { static const uint8_t g[8] = {0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x18, 0x30, 0x00}; return g; }
        default: return s_empty;
    }
}

void Renderer::drawText(const std::string& text, int x, int y, uint32_t colorHex) {
    int curX = x;
    for (char c : text) {
        const uint8_t* glyph = getGlyph8x8(c);
        for (int r = 0; r < 8; ++r) {
            int py = y + r;
            if (py < 0 || py >= m_height) continue;
            uint8_t row = glyph[r];
            for (int col = 0; col < 8; ++col) {
                if (row & (0x80 >> col)) {
                    int px = curX + col;
                    if (px >= 0 && px < m_width) {
                        m_framebuffer[static_cast<size_t>(py) * m_width + px] = colorHex;
                    }
                }
            }
        }
        curX += 8;
    }
}

} // namespace aoe2
