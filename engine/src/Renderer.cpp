#include "Renderer.h"
#include <cstring>
#include <algorithm>

namespace aoe2 {

Renderer::Renderer() {
    // Initialize default standard Genie palette grayscale fallback
    for (int i = 0; i < 256; ++i) {
        m_basePalette[i] = { static_cast<uint8_t>(i), static_cast<uint8_t>(i), static_cast<uint8_t>(i), 255 };
    }
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
    if (!palData || size < 256 * 3) return;
    
    // Check if JASC-PAL or raw RGB
    size_t offset = 0;
    if (size >= 24 && std::memcmp(palData, "JASC-PAL", 8) == 0) {
        // Skip header lines to reach color definitions
        // JASC-PAL standard parser
        offset = 24; 
    }

    for (int i = 0; i < 256 && (offset + 3 <= size); ++i) {
        m_basePalette[i].r = palData[offset++];
        m_basePalette[i].g = palData[offset++];
        m_basePalette[i].b = palData[offset++];
        m_basePalette[i].a = 255;
    }

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
    const ColorRGBA* pal = m_playerPalettes[playerIndex];

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

void Renderer::drawRect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    uint32_t color = (static_cast<uint32_t>(a) << 24) | (static_cast<uint32_t>(b) << 16) | 
                     (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(r);

    for (int cy = std::max(0, y); cy < std::min(m_height, y + h); ++cy) {
        for (int cx = std::max(0, x); cx < std::min(m_width, x + w); ++cx) {
            m_framebuffer[static_cast<size_t>(cy) * m_width + cx] = color;
        }
    }
}

} // namespace aoe2
