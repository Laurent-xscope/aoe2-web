#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace aoe2 {

struct ResourceCost {
    int16_t type{-1};  // 0: Food, 1: Wood, 2: Stone, 3: Gold
    int16_t amount{0};
    int16_t flag{0};
};

struct AttackValue {
    uint16_t damageClass{0};
    uint16_t amount{0};
};

struct ArmorValue {
    uint16_t armorClass{0};
    uint16_t amount{0};
};

struct UnitType {
    int id{0};
    std::string name;
    int unitClass{0}; // 0: Archer, 2: Building, 3: Villager, 4: Infantry, 8: Cavalry, 11: Siege
    float hitPoints{0};
    float lineOfSight{0};
    float speed{0};
    int garrisonCapacity{0};
    
    std::vector<ResourceCost> costs;
    std::vector<AttackValue> attacks;
    std::vector<ArmorValue> armors;

    float minRange{0};
    float maxRange{0};
    float reloadTime{0};
    float trainTime{0};

    // Graphics IDs
    int32_t standingGraphic{0};
    int32_t walkingGraphic{0};
    int32_t attackGraphic{0};
    int32_t dyingGraphic{0};
};

struct Civilization {
    int id{0};
    std::string name;
    std::vector<UnitType> units;
};

class DATParser {
public:
    DATParser();
    ~DATParser();

    bool loadDATFile(const std::string& datFilePath);
    const Civilization* getCivilization(int civId) const;
    const UnitType* getUnit(int civId, int unitId) const;

private:
    bool parseUncompressed(const uint8_t* data, size_t size);

    std::string m_version;
    std::vector<Civilization> m_civilizations;
    std::unordered_map<int, Civilization> m_civMap;
};

} // namespace aoe2
