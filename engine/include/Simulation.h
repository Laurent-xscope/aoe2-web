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

class Simulation {
public:
    Simulation();
    ~Simulation();

    void initialize(std::shared_ptr<DATParser> rules, std::shared_ptr<Terrain> terrain);
    void tick(double deltaTime);
    void render(Renderer& renderer, DRSReader& drs, float camX, float camY, float zoom);

    // Entity creation
    std::shared_ptr<Unit> spawnUnit(int civId, int unitTypeId, int playerId, float x, float y);

    // Selection & commands
    void selectAt(float worldX, float worldY, bool multiSelect = false);
    void issueMoveOrder(float worldX, float worldY);
    void issueAttackOrder(uint32_t targetId);
    void trainVillager(int playerId = 1);

    const std::vector<std::shared_ptr<Unit>>& getSelectedUnits() const { return m_selectedUnits; }
    const std::vector<std::shared_ptr<Unit>>& getAllUnits() const { return m_units; }
    std::shared_ptr<Terrain> getTerrain() const { return m_terrain; }
    const PlayerEconomy& getPlayerEconomy(int playerIndex) const;
    PlayerEconomy& getPlayerEconomy(int playerIndex);

private:
    std::shared_ptr<DATParser> m_rules;
    std::shared_ptr<Terrain> m_terrain;

    uint32_t m_nextEntityId{1};
    std::vector<std::shared_ptr<Unit>> m_units;
    std::vector<std::shared_ptr<Unit>> m_selectedUnits;

    PlayerEconomy m_players[8];
    void updateVisibility();
};

} // namespace aoe2
