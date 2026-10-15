#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace aoe2 {

struct Texture {
    int width{0};
    int height{0};
    int channels{0};
    std::vector<uint32_t> pixels; // Format: (A << 24) | (B << 16) | (G << 8) | R

    bool loadFromFile(const std::string& filePath);
    bool loadFromMemory(const uint8_t* memData, size_t size);
    bool isValid() const { return width > 0 && height > 0 && !pixels.empty(); }
};

} // namespace aoe2
