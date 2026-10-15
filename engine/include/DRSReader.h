#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>

#include "SLPReader.h"

namespace aoe2 {

struct DRSFileEntry {
    uint32_t id{0};
    uint32_t offset{0};
    uint32_t length{0};
};

struct DRSTable {
    char type[4]; // 'slp ', 'wav ', 'bina'
    uint32_t offset{0};
    uint32_t numFiles{0};
    std::vector<DRSFileEntry> entries;
};

class DRSReader {
public:
    DRSReader();
    ~DRSReader();

    // Loads archive file (.drs)
    bool loadArchive(const std::string& drsPath);

    // Mounts an uncompressed directory (HD Edition resources folder)
    void mountDirectory(const std::string& dirPath);

    // Retrieves raw resource bytes by ID and type ('slp ', 'bina', 'wav ')
    std::vector<uint8_t> getResource(uint32_t id, const std::string& type = "slp");

    // Retrieves or caches a decoded TShape SLP graphic
    std::shared_ptr<TShape> getShape(uint32_t id);
    std::shared_ptr<TShape> getShapeByName(const std::string& name);

private:
    std::vector<std::string> m_mountedDirs;
    std::unordered_map<uint32_t, std::shared_ptr<TShape>> m_shapeCache;
    std::unordered_map<std::string, std::shared_ptr<TShape>> m_nameShapeCache;
};

} // namespace aoe2
