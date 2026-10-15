#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include "Unit.h"
#include "Terrain.h"
#include "DATParser.h"
#include "DRSReader.h"
#include "Renderer.h"

namespace aoe2 {

struct PlayerEconomy {
    int wood{200};
    int food{200};
    int gold{100};
    int stone{200};
    int population{4};
    int maxPopulation{10};
    int currentAge{0}; // 0: Dark, 1: Feudal, 2: Castle, 3: Imperial
};

struct Missile {
    uint32_t id{0};
    int ownerPlayerId{1};
    uint32_t targetEntityId{0};
    float startX{0.0f};
    float startY{0.0f};
    float startZ{0.0f};
    float targetX{0.0f};
    float targetY{0.0f};
    float targetZ{0.0f};
    float currentX{0.0f};
    float currentY{0.0f};
    float currentZ{0.0f};
    float speed{8.0f};     // tiles per second
    float totalDist{1.0f};
    float distTraveled{0.0f};
    float damage{4.0f};
    int graphicId{1034};
    bool dead{false};
};

class Simulation {
public:
    Simulation();
    ~Simulation();

    void initialize(std::shared_ptr<DATParser> rules, std::shared_ptr<Terrain> terrain);
    void tick(double deltaTime);
    void render(Renderer& renderer, DRSReader& drs, float camX, float camY, float zoom);

    // Entity creation
    std::shared_ptr<Unit> spawnUnit(int civId, int unitTypeId, int playerId, float x, float y);
    std::shared_ptr<Unit> spawnFoundation(int civId, int unitTypeId, int playerId, float x, float y);
    void spawnMissile(uint32_t sourceId, int playerId, float sx, float sy, float sz,
                      uint32_t targetId, float tx, float ty, float tz, float damage);

    // Selection & commands
    void selectAt(float worldX, float worldY, bool multiSelect = false);
    void selectBox(float minWorldX, float minWorldY, float maxWorldX, float maxWorldY, bool multiSelect = false);
    void issueMoveOrder(float worldX, float worldY);
    void issueAttackOrder(uint32_t targetId);
    void issueBuildOrder(uint32_t foundationId);
    void trainVillager(int playerId = 1);
    void trainMilitia(int playerId = 1);

    std::shared_ptr<Unit> getUnitById(uint32_t id) const;
    const std::vector<std::shared_ptr<Unit>>& getSelectedUnits() const { return m_selectedUnits; }
    const std::vector<std::shared_ptr<Unit>>& getAllUnits() const { return m_units; }
    const std::vector<Missile>& getMissiles() const { return m_missiles; }
    std::shared_ptr<Terrain> getTerrain() const { return m_terrain; }
    const PlayerEconomy& getPlayerEconomy(int playerIndex) const;
    PlayerEconomy& getPlayerEconomy(int playerIndex);

    void triggerSound(const char* soundFile);

private:
    std::shared_ptr<DATParser> m_rules;
    std::shared_ptr<Terrain> m_terrain;

    uint32_t m_nextEntityId{1};
    uint32_t m_nextMissileId{1};
    std::vector<std::shared_ptr<Unit>> m_units;
    std::vector<std::shared_ptr<Unit>> m_selectedUnits;
    std::vector<Missile> m_missiles;

    PlayerEconomy m_players[8];
    void updateVisibility();
};

} // namespace aoe2
