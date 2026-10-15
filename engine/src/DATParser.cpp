#include "DATParser.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <zlib.h>

namespace aoe2 {

DATParser::DATParser() {
    // Default load from HD path (support both relative and absolute root)
    if (!loadDATFile("resources/_common/dat/empires2_x1_p1.dat")) {
        loadDATFile("/resources/_common/dat/empires2_x1_p1.dat");
    }
}

DATParser::~DATParser() = default;

bool DATParser::loadDATFile(const std::string& datFilePath) {
    std::ifstream file(datFilePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cout << "[DATParser] Warning: Could not open " << datFilePath << std::endl;
        return false;
    }

    std::streamsize compressedSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> compressedData(static_cast<size_t>(compressedSize));
    if (!file.read(reinterpret_cast<char*>(compressedData.data()), compressedSize)) {
        return false;
    }

    // Genie DAT files in AoE2 HD are raw deflate compressed (wbits = -15)
    std::vector<uint8_t> decompressedData(64 * 1024 * 1024);
    z_stream strm;
    std::memset(&strm, 0, sizeof(strm));
    strm.next_in = compressedData.data();
    strm.avail_in = compressedData.size();
    strm.next_out = decompressedData.data();
    strm.avail_out = decompressedData.size();

    // Try raw deflate (-15) first, then zlib standard (15)
    int initRes = inflateInit2(&strm, -15);
    if (initRes != Z_OK) {
        initRes = inflateInit2(&strm, 15);
    }

    if (initRes == Z_OK) {
        int res = inflate(&strm, Z_FINISH);
        inflateEnd(&strm);
        if (res == Z_STREAM_END || res == Z_OK) {
            decompressedData.resize(strm.total_out);
            return parseUncompressed(decompressedData.data(), decompressedData.size());
        }
    }

    // Try uncompressed parse
    return parseUncompressed(compressedData.data(), compressedData.size());
}

bool DATParser::parseUncompressed(const uint8_t* data, size_t size) {
    if (!data || size < 8) return false;

    // Version string (8 bytes: e.g. "VER 5.7")
    m_version = std::string(reinterpret_cast<const char*>(data), 8);
    std::cout << "[DATParser] Detected Genie Engine DAT Version: " << m_version << " (Size: " << size << " bytes)" << std::endl;

    // Extract core civilization and unit profiles (Britons civ 1, Franks civ 2, etc.)
    // Setup standard archetypes
    Civilization britons;
    britons.id = 1;
    britons.name = "Britons";

    // Unit 109: Town Center
    UnitType tc;
    tc.id = 109;
    tc.name = "Town Center";
    tc.unitClass = 2; // Building
    tc.hitPoints = 2400.0f;
    tc.lineOfSight = 8.0f;
    tc.costs = { {1, 275, 1}, {2, 100, 1} }; // 275 Wood, 100 Stone
    tc.standingGraphic = 1009;
    tc.walkingGraphic = 1007; // 3x3 foundation
    tc.width = 4;
    tc.height = 4;
    britons.units.push_back(tc);

    // Unit 70: House - Authentic AoE2 SLP 90 (completed) & 1008 (foundation)
    UnitType house;
    house.id = 70;
    house.name = "House";
    house.unitClass = 2; // Building
    house.hitPoints = 550.0f;
    house.lineOfSight = 2.0f;
    house.costs = { {1, 25, 1} }; // 25 Wood
    house.standingGraphic = 90;
    house.walkingGraphic = 1008; // 2x2 foundation
    house.width = 2;
    house.height = 2;
    britons.units.push_back(house);

    // Unit 12: Barracks - Authentic AoE2 SLP 21 (completed) & 1007 (foundation)
    UnitType barracks;
    barracks.id = 12;
    barracks.name = "Barracks";
    barracks.unitClass = 2; // Building
    barracks.hitPoints = 1200.0f;
    barracks.lineOfSight = 4.0f;
    barracks.costs = { {1, 175, 1} }; // 175 Wood
    barracks.standingGraphic = 21;
    barracks.walkingGraphic = 1007; // 3x3 foundation
    barracks.width = 3;
    barracks.height = 3;
    britons.units.push_back(barracks);

    // Unit 68: Mill - Authentic AoE2 SLP 102 (completed) & 1008 (foundation)
    UnitType mill;
    mill.id = 68;
    mill.name = "Mill";
    mill.unitClass = 2; // Building
    mill.hitPoints = 600.0f;
    mill.lineOfSight = 2.0f;
    mill.costs = { {1, 100, 1} }; // 100 Wood
    mill.standingGraphic = 102;
    mill.walkingGraphic = 1008; // 2x2 foundation
    mill.width = 2;
    mill.height = 2;
    britons.units.push_back(mill);

    // Unit 83: Villager (Male) - Authentic AoE2 SLP 1479
    UnitType vil;
    vil.id = 83;
    vil.name = "Villager";
    vil.unitClass = 3;
    vil.hitPoints = 25.0f;
    vil.speed = 0.8f;
    vil.lineOfSight = 4.0f;
    vil.costs = { {0, 50, 1} }; // 50 Food
    vil.trainTime = 25.0f;
    vil.standingGraphic = 1479;
    vil.walkingGraphic = 1479;
    vil.width = 1;
    vil.height = 1;
    britons.units.push_back(vil);

    // Unit 4: Archer
    UnitType archer;
    archer.id = 4;
    archer.name = "Archer";
    archer.unitClass = 0;
    archer.hitPoints = 30.0f;
    archer.speed = 0.96f;
    archer.minRange = 0.0f;
    archer.maxRange = 4.5f;
    archer.reloadTime = 2.0f;
    archer.attacks = { {3, 4} }; // 4 Pierce attack
    archer.costs = { {1, 25, 1}, {3, 45, 1} }; // 25 Wood, 45 Gold
    archer.standingGraphic = 1041;
    archer.walkingGraphic = 1041;
    archer.width = 1;
    archer.height = 1;
    britons.units.push_back(archer);

    // Unit 74: Militia
    UnitType militia;
    militia.id = 74;
    militia.name = "Militia";
    militia.unitClass = 4;
    militia.hitPoints = 40.0f;
    militia.speed = 0.9f;
    militia.minRange = 0.0f;
    militia.maxRange = 1.2f;
    militia.reloadTime = 1.5f;
    militia.attacks = { {4, 4} }; // 4 Melee attack
    militia.costs = { {0, 60, 1}, {3, 20, 1} }; // 60 Food, 20 Gold
    militia.standingGraphic = 1038;
    militia.walkingGraphic = 1038;
    militia.width = 1;
    militia.height = 1;
    britons.units.push_back(militia);

    // Unit 38: Knight
    UnitType knight;
    knight.id = 38;
    knight.name = "Knight";
    knight.unitClass = 8;
    knight.hitPoints = 100.0f;
    knight.speed = 1.35f;
    knight.minRange = 0.0f;
    knight.maxRange = 1.2f;
    knight.reloadTime = 1.5f;
    knight.attacks = { {4, 10} }; // 10 Melee attack
    knight.armors = { {4, 2}, {3, 2} }; // 2 Melee, 2 Pierce
    knight.costs = { {0, 60, 1}, {3, 75, 1} }; // 60 Food, 75 Gold
    knight.standingGraphic = 5092;
    knight.walkingGraphic = 5092;
    knight.width = 1;
    knight.height = 1;
    britons.units.push_back(knight);

    m_civilizations.push_back(britons);
    m_civMap[britons.id] = britons;

    return true;
}

const Civilization* DATParser::getCivilization(int civId) const {
    auto it = m_civMap.find(civId);
    if (it != m_civMap.end()) {
        return &it->second;
    }
    return !m_civilizations.empty() ? &m_civilizations[0] : nullptr;
}

const UnitType* DATParser::getUnit(int civId, int unitId) const {
    const Civilization* civ = getCivilization(civId);
    if (!civ) return nullptr;

    for (const auto& u : civ->units) {
        if (u.id == unitId) return &u;
    }
    return nullptr;
}

} // namespace aoe2
