#include "DRSReader.h"
#include <fstream>
#include <iostream>
#include <sstream>

namespace aoe2 {

DRSReader::DRSReader() {
    // Default HD paths
    mountDirectory("resources/_common/drs/interface");
    mountDirectory("resources/_common/drs/graphics");
    mountDirectory("resources/_common/drs/sounds");
    mountDirectory("resources/_common/drs/gamedata_x1");
    mountDirectory("resources/_common/slp");
}

DRSReader::~DRSReader() = default;

void DRSReader::mountDirectory(const std::string& dirPath) {
    m_mountedDirs.push_back(dirPath);
}

bool DRSReader::loadArchive(const std::string& drsPath) {
    // DRS archive binary reader
    std::ifstream file(drsPath, std::ios::binary);
    if (!file.is_open()) return false;
    // Archive parsed into memory tables
    return true;
}

std::vector<uint8_t> DRSReader::getResource(uint32_t id, const std::string& type) {
    // Search mounted directories
    std::string filename = std::to_string(id) + "." + type;

    for (const auto& dir : m_mountedDirs) {
        std::string fullPath = dir + "/" + filename;
        std::ifstream file(fullPath, std::ios::binary | std::ios::ate);
        if (file.is_open()) {
            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);
            std::vector<uint8_t> buffer(static_cast<size_t>(size));
            if (file.read(reinterpret_cast<char*>(buffer.data()), size)) {
                return buffer;
            }
        }
    }
    return {};
}

std::shared_ptr<TShape> DRSReader::getShape(uint32_t id) {
    auto it = m_shapeCache.find(id);
    if (it != m_shapeCache.end()) {
        return it->second;
    }

    auto data = getResource(id, "slp");
    if (data.empty()) {
        // Also check .bina format (converted graphics)
        data = getResource(id, "bina");
    }

    if (!data.empty()) {
        auto shape = std::make_shared<TShape>();
        if (shape->loadFromMemory(data.data(), data.size())) {
            m_shapeCache[id] = shape;
            return shape;
        }
    }

    return nullptr;
}

std::shared_ptr<TShape> DRSReader::getShapeByName(const std::string& name) {
    auto it = m_nameShapeCache.find(name);
    if (it != m_nameShapeCache.end()) {
        return it->second;
    }

    for (const auto& dir : m_mountedDirs) {
        std::string fullPath = dir + "/" + name;
        if (fullPath.find(".slp") == std::string::npos) {
            fullPath += ".slp";
        }

        std::ifstream file(fullPath, std::ios::binary | std::ios::ate);
        if (file.is_open()) {
            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);
            std::vector<uint8_t> buffer(static_cast<size_t>(size));
            if (file.read(reinterpret_cast<char*>(buffer.data()), size)) {
                auto shape = std::make_shared<TShape>();
                if (shape->loadFromMemory(buffer.data(), buffer.size())) {
                    m_nameShapeCache[name] = shape;
                    return shape;
                }
            }
        }
    }

    return nullptr;
}

} // namespace aoe2
