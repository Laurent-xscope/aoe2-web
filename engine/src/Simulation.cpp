#include "Simulation.h"
#include <cmath>
#include <algorithm>

namespace aoe2 {

Simulation::Simulation() = default;
Simulation::~Simulation() = default;

void Simulation::initialize(std::shared_ptr<DATParser> rules, std::shared_ptr<Terrain> terrain) {
    m_rules = rules;
    m_terrain = terrain;

    // Reset players economy
    for (int i = 0; i < 8; ++i) {
        m_players[i] = PlayerEconomy();
    }

    // Spawn Player 1 base setup (Town Center, 3 Villagers, 1 Swordsman, 1 Archer)
    spawnUnit(1, 109, 1, 20.0f, 20.0f); // Town Center
    spawnUnit(1, 83, 1, 23.0f, 21.0f);  // Villager 1
    spawnUnit(1, 83, 1, 24.0f, 20.0f);  // Villager 2
    spawnUnit(1, 83, 1, 22.0f, 22.0f);  // Villager 3
    spawnUnit(1, 74, 1, 25.0f, 23.0f);  // Swordsman
    spawnUnit(1, 4, 1, 23.0f, 24.0f);   // Archer

    // Spawn AI Player 2 setup
    spawnUnit(1, 109, 2, 70.0f, 70.0f); // AI Town Center
    spawnUnit(1, 83, 2, 72.0f, 72.0f);
    spawnUnit(1, 74, 2, 73.0f, 71.0f);  // Militia
}

std::shared_ptr<Unit> Simulation::spawnUnit(int civId, int unitTypeId, int playerId, float x, float y) {
    const UnitType* type = m_rules ? m_rules->getUnit(civId, unitTypeId) : nullptr;
    if (!type) {
        UnitType defaultType;
        defaultType.id = unitTypeId;
        defaultType.hitPoints = 100.0f;
        defaultType.speed = 1.0f;
        defaultType.standingGraphic = unitTypeId;
        auto unit = std::make_shared<Unit>(m_nextEntityId++, defaultType, playerId, x, y);
        m_units.push_back(unit);
        return unit;
    }

    auto unit = std::make_shared<Unit>(m_nextEntityId++, *type, playerId, x, y);
    m_units.push_back(unit);
    return unit;
}

void Simulation::tick(double deltaTime) {
    float dt = static_cast<float>(deltaTime);

    // Update all live units
    for (auto& unit : m_units) {
        unit->update(dt);
    }

    // Erase dead units
    m_units.erase(std::remove_if(m_units.begin(), m_units.end(), [](const std::shared_ptr<Unit>& u) {
        return u->isDead();
    }), m_units.end());
}

void Simulation::render(Renderer& renderer, DRSReader& drs, float camX, float camY, float zoom) {
    // Sort units by isometric depth (worldX + worldY) for proper painter's algorithm
    std::vector<std::shared_ptr<Unit>> sortedUnits = m_units;
    std::sort(sortedUnits.begin(), sortedUnits.end(), [](const std::shared_ptr<Unit>& a, const std::shared_ptr<Unit>& b) {
        return (a->getX() + a->getY()) < (b->getX() + b->getY());
    });

    for (auto& unit : sortedUnits) {
        unit->render(renderer, drs, camX, camY, zoom);
    }
}

void Simulation::selectAt(float worldX, float worldY, bool multiSelect) {
    if (!multiSelect) {
        m_selectedUnits.clear();
    }

    // 1. Check mobile units first (smaller collision radius)
    for (auto& unit : m_units) {
        if (!unit || unit->isDead()) continue;
        if (unit->getTypeId() == 109) continue; // Check Town Center after units

        float dx = unit->getX() - worldX;
        float dy = unit->getY() - worldY;
        if (dx * dx + dy * dy < 2.5f) {
            m_selectedUnits.push_back(unit);
            return;
        }
    }

    // 2. Check buildings (Town Center has 4x4 tile footprint)
    for (auto& unit : m_units) {
        if (!unit || unit->isDead()) continue;
        if (unit->getTypeId() == 109) {
            float dx = unit->getX() - worldX;
            float dy = unit->getY() - worldY;
            if (dx * dx + dy * dy < 8.0f) {
                m_selectedUnits.push_back(unit);
                return;
            }
        }
    }
}

void Simulation::trainVillager(int playerId) {
    int idx = std::clamp(playerId - 1, 0, 7);
    if (m_players[idx].food >= 50 && m_players[idx].population < m_players[idx].maxPopulation) {
        m_players[idx].food -= 50;
        m_players[idx].population++;

        // Find player's Town Center
        for (auto& u : m_units) {
            if (u && !u->isDead() && u->getPlayerId() == playerId && u->getTypeId() == 109) {
                float spawnX = u->getX() + 2.0f;
                float spawnY = u->getY() + 2.5f;
                auto vill = spawnUnit(1, 83, playerId, spawnX, spawnY);
                if (vill) {
                    // Send villager slightly outward
                    vill->commandMove(spawnX + 1.5f, spawnY + 1.5f);
                }
                break;
            }
        }
    }
}

void Simulation::issueMoveOrder(float worldX, float worldY) {
    for (size_t i = 0; i < m_selectedUnits.size(); ++i) {
        // Offset formation slightly for multiple units
        float ox = (static_cast<float>(i % 3) - 1.0f) * 0.8f;
        float oy = (static_cast<float>(i / 3) - 1.0f) * 0.8f;
        m_selectedUnits[i]->commandMove(worldX + ox, worldY + oy);
    }
}

void Simulation::issueAttackOrder(uint32_t targetId) {
    for (auto& unit : m_selectedUnits) {
        unit->commandAttack(targetId);
    }
}

const PlayerEconomy& Simulation::getPlayerEconomy(int playerIndex) const {
    if (playerIndex >= 0 && playerIndex < 8) {
        return m_players[playerIndex];
    }
    return m_players[0];
}

PlayerEconomy& Simulation::getPlayerEconomy(int playerIndex) {
    if (playerIndex >= 0 && playerIndex < 8) {
        return m_players[playerIndex];
    }
    return m_players[0];
}

} // namespace aoe2
