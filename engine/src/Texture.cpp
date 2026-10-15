#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "Texture.h"
#include <iostream>

namespace aoe2 {

bool Texture::loadFromFile(const std::string& filePath) {
    int w = 0, h = 0, n = 0;
    stbi_set_flip_vertically_on_load(0);
    unsigned char* data = stbi_load(filePath.c_str(), &w, &h, &n, 4);
    if (!data) {
        return false;
    }

    width = w;
    height = h;
    channels = 4;
    pixels.resize(static_cast<size_t>(w) * h);

    for (int i = 0; i < w * h; ++i) {
        uint8_t r = data[i * 4 + 0];
        uint8_t g = data[i * 4 + 1];
        uint8_t b = data[i * 4 + 2];
        uint8_t a = data[i * 4 + 3];
        pixels[i] = (static_cast<uint32_t>(a) << 24) | 
                    (static_cast<uint32_t>(b) << 16) | 
                    (static_cast<uint32_t>(g) << 8)  | 
                    static_cast<uint32_t>(r);
    }

    stbi_image_free(data);
    return true;
}

bool Texture::loadFromMemory(const uint8_t* memData, size_t size) {
    if (!memData || size == 0) return false;
    int w = 0, h = 0, n = 0;
    stbi_set_flip_vertically_on_load(0);
    unsigned char* data = stbi_load_from_memory(memData, static_cast<int>(size), &w, &h, &n, 4);
    if (!data) {
        return false;
    }

    width = w;
    height = h;
    channels = 4;
    pixels.resize(static_cast<size_t>(w) * h);

    for (int i = 0; i < w * h; ++i) {
        uint8_t r = data[i * 4 + 0];
        uint8_t g = data[i * 4 + 1];
        uint8_t b = data[i * 4 + 2];
        uint8_t a = data[i * 4 + 3];
        pixels[i] = (static_cast<uint32_t>(a) << 24) | 
                    (static_cast<uint32_t>(b) << 16) | 
                    (static_cast<uint32_t>(g) << 8)  | 
                    static_cast<uint32_t>(r);
    }

    stbi_image_free(data);
    return true;
}

} // namespace aoe2
