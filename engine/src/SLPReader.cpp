#include "SLPReader.h"
#include <fstream>
#include <iostream>
#include <cstring>

namespace aoe2 {

TShape::TShape() = default;
TShape::~TShape() = default;

bool TShape::loadFromFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        return false;
    }

    return loadFromMemory(buffer.data(), buffer.size());
}

bool TShape::loadFromMemory(const uint8_t* buffer, size_t size) {
    return parseSLP(buffer, size);
}

const SLPFrame* TShape::getFrame(int index) const {
    if (index >= 0 && index < static_cast<int>(m_frames.size())) {
        return &m_frames[index];
    }
    return nullptr;
}

bool TShape::parseSLP(const uint8_t* buffer, size_t size) {
    if (!buffer || size < 32) return false;

    // 1. Header (32 bytes)
    m_version = std::string(reinterpret_cast<const char*>(buffer), 4);
    if (m_version != "2.0N" && m_version != "2.0 ") {
        // Fallback or accept 2.0
    }

    m_numFrames = *reinterpret_cast<const int32_t*>(buffer + 4);
    m_comment = std::string(reinterpret_cast<const char*>(buffer + 8), 24);

    if (m_numFrames <= 0 || m_numFrames > 10000) return false;

    // 2. Frame Header array (32 bytes per frame)
    size_t headerOffset = 32;
    if (headerOffset + m_numFrames * 32 > size) return false;

    struct RawFrameHeader {
        uint32_t cmdTableOffset;
        uint32_t outlineTableOffset;
        uint32_t paletteOffset;
        uint32_t properties;
        int32_t width;
        int32_t height;
        int32_t hotspotX;
        int32_t hotspotY;
    };

    std::vector<RawFrameHeader> rawHeaders(m_numFrames);
    for (int i = 0; i < m_numFrames; ++i) {
        std::memcpy(&rawHeaders[i], buffer + headerOffset + i * 32, sizeof(RawFrameHeader));
    }

    m_frames.resize(m_numFrames);

    // 3. Decode each frame
    for (int f = 0; f < m_numFrames; ++f) {
        const auto& rh = rawHeaders[f];
        SLPFrame& frame = m_frames[f];
        frame.width = rh.width;
        frame.height = rh.height;
        frame.hotspotX = rh.hotspotX;
        frame.hotspotY = rh.hotspotY;

        if (frame.width <= 0 || frame.height <= 0 || frame.width > 4096 || frame.height > 4096) {
            continue;
        }

        frame.pixelData.assign(static_cast<size_t>(frame.width) * frame.height, 0xFF);
        frame.shadowMask.assign(static_cast<size_t>(frame.width) * frame.height, 0);

        if (rh.cmdTableOffset >= size) continue;

        // Command row offsets table: uint32 per row
        const uint32_t* rowOffsets = reinterpret_cast<const uint32_t*>(buffer + rh.cmdTableOffset);

        // Outline table: uint16 left, uint16 right per row
        struct RawOutlineRow {
            uint16_t left;
            uint16_t right;
        };
        const RawOutlineRow* outlines = nullptr;
        if (rh.outlineTableOffset > 0 && rh.outlineTableOffset + static_cast<size_t>(frame.height) * sizeof(RawOutlineRow) <= size) {
            outlines = reinterpret_cast<const RawOutlineRow*>(buffer + rh.outlineTableOffset);
        }

        for (int y = 0; y < frame.height; ++y) {
            uint32_t rowCmdOffset = rowOffsets[y];
            if (rowCmdOffset >= size) continue;

            int x = 0;
            if (outlines) {
                if (outlines[y].left == 0x8000) {
                    continue; // Transparent empty row
                }
                x = outlines[y].left;
            }

            const uint8_t* p = buffer + rowCmdOffset;

            while (x < frame.width && p < buffer + size) {
                uint8_t cmd = *p++;
                uint8_t opcode = cmd & 0x0F;

                // Genie Engine SLP command byte interpreter
                switch (opcode) {
                    case 0x00: // Block color copy: (cmd >> 2) pixels
                    case 0x04:
                    case 0x08:
                    case 0x0C: {
                        int count = cmd >> 2;
                        for (int i = 0; i < count && x < frame.width; ++i) {
                            frame.pixelData[y * frame.width + x++] = *p++;
                        }
                        break;
                    }

                    case 0x01: // Skip pixels: (cmd >> 2)
                    case 0x05:
                    case 0x09:
                    case 0x0D: {
                        int count = cmd >> 2;
                        x += count;
                        break;
                    }

                    case 0x02: { // Extended color copy
                        int count = (cmd >> 4) << 8 | (*p++);
                        for (int i = 0; i < count && x < frame.width; ++i) {
                            frame.pixelData[y * frame.width + x++] = *p++;
                        }
                        break;
                    }

                    case 0x03: { // Extended skip pixels
                        int count = (cmd >> 4) << 8 | (*p++);
                        x += count;
                        break;
                    }

                    case 0x06: { // Player color copy (mapped ramp)
                        int count = cmd >> 4;
                        if (count == 0) count = *p++;
                        for (int i = 0; i < count && x < frame.width; ++i) {
                            // In AoE2, player color base ramp is mapped from indices 16-23
                            frame.pixelData[y * frame.width + x++] = 16 + (*p++ & 0x07);
                        }
                        break;
                    }

                    case 0x07: { // Fill with color
                        int count = cmd >> 4;
                        if (count == 0) count = *p++;
                        uint8_t color = *p++;
                        for (int i = 0; i < count && x < frame.width; ++i) {
                            frame.pixelData[y * frame.width + x++] = color;
                        }
                        break;
                    }

                    case 0x0A: { // Player color fill
                        int count = cmd >> 4;
                        if (count == 0) count = *p++;
                        uint8_t color = 16 + (*p++ & 0x07);
                        for (int i = 0; i < count && x < frame.width; ++i) {
                            frame.pixelData[y * frame.width + x++] = color;
                        }
                        break;
                    }

                    case 0x0B: { // Shadow pixel run
                        int count = cmd >> 2;
                        if (count == 0) count = *p++;
                        for (int i = 0; i < count && x < frame.width; ++i) {
                            frame.shadowMask[y * frame.width + x] = 1;
                            frame.pixelData[y * frame.width + x] = 0xFE; // Shadow sentinel
                            x++;
                        }
                        break;
                    }

                    case 0x0F: // End of line
                        x = frame.width;
                        break;

                    default:
                        // Unrecognized opcode, advance
                        break;
                }
            }
        }
    }

    return true;
}

} // namespace aoe2
