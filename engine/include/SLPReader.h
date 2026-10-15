#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <memory>

namespace aoe2 {

struct SLPFrame {
    int width{0};
    int height{0};
    int hotspotX{0};
    int hotspotY{0};
    std::vector<uint8_t> pixelData;   // Width * Height 8-bit palette indices (0xFF = transparent)
    std::vector<uint8_t> shadowMask;  // 1 if shadow pixel, 0 otherwise
};

class TShape {
public:
    TShape();
    ~TShape();

    bool loadFromFile(const std::string& filePath);
    bool loadFromMemory(const uint8_t* buffer, size_t size);

    int getFrameCount() const { return static_cast<int>(m_frames.size()); }
    const SLPFrame* getFrame(int index) const;

private:
    bool parseSLP(const uint8_t* buffer, size_t size);

    std::string m_version;
    int m_numFrames{0};
    std::string m_comment;
    std::vector<SLPFrame> m_frames;
};

} // namespace aoe2
