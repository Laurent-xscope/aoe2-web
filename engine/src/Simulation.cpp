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
    spawnUnit(1, 74, 2, 73.0f, 71.0f);  // AI Militia
    spawnUnit(1, 74, 2, 28.0f, 25.0f);  // Enemy scout invading base perimeter

    // Update initial Line of Sight around player units
    updateVisibility();
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

void Simulation::updateVisibility() {
    if (!m_terrain) return;
    int mapW = m_terrain->getWidth();
    int mapH = m_terrain->getHeight();

    // Reset visibility flag for all tiles (preserving explored state)
    for (int y = 0; y < mapH; ++y) {
        for (int x = 0; x < mapW; ++x) {
            Tile* t = m_terrain->getTile(x, y);
            if (t) t->visible = 0;
        }
    }

    // Reveal tiles within Line of Sight of Player 1 units and buildings
    for (const auto& u : m_units) {
        if (!u || u->isDead() || u->getPlayerId() != 1) continue;

        int los = static_cast<int>(u->getLineOfSight());
        if (los <= 0) los = (u->getTypeId() == 109) ? 10 : 5;

        int ux = static_cast<int>(u->getX());
        int uy = static_cast<int>(u->getY());

        for (int dy = -los; dy <= los; ++dy) {
            for (int dx = -los; dx <= los; ++dx) {
                if (dx * dx + dy * dy <= los * los) {
                    Tile* t = m_terrain->getTile(ux + dx, uy + dy);
                    if (t) {
                        t->explored = 1;
                        t->visible = 1;
                    }
                }
            }
        }
    }
}

#include "Isometric.h"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

void Simulation::tick(double deltaTime) {
    float dt = static_cast<float>(deltaTime);

    // Update all live units
    for (auto& unit : m_units) {
        bool wasFoundation = unit->isFoundation();
        unit->update(dt, *this);
        if (wasFoundation && !unit->isFoundation()) {
            // Foundation complete! House adds +5 max population
            if (unit->getTypeId() == 70) {
                int pIdx = std::clamp(unit->getPlayerId() - 1, 0, 7);
                m_players[pIdx].maxPopulation += 5;
            }
        }
    }

    // Update active projectiles (Arrows / Missiles)
    for (auto& m : m_missiles) {
        if (m.dead) continue;
        m.distTraveled += m.speed * dt;
        float t = m.distTraveled / m.totalDist;
        if (t >= 1.0f) {
            m.dead = true;
            m.currentX = m.targetX;
            m.currentY = m.targetY;
            m.currentZ = m.targetZ;
            auto target = getUnitById(m.targetEntityId);
            if (target && !target->isDead()) {
                target->applyDamage(m.damage);
                triggerSound("5045.wav"); // Arrow impact
            }
        } else {
            m.currentX = m.startX + (m.targetX - m.startX) * t;
            m.currentY = m.startY + (m.targetY - m.startY) * t;
            float peak = std::min(2.5f, m.totalDist * 0.35f);
            m.currentZ = m.startZ + (m.targetZ - m.startZ) * t + 4.0f * peak * t * (1.0f - t);
        }
    }

    m_missiles.erase(std::remove_if(m_missiles.begin(), m_missiles.end(), [](const Missile& m) {
        return m.dead;
    }), m_missiles.end());

    // Refresh dynamic line of sight as units move
    updateVisibility();

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
        if (!unit || unit->isDead()) continue;

        // Friendly units always render; enemy units only render if standing on visible tile
        if (unit->getPlayerId() != 1 && m_terrain) {
            const auto* tile = m_terrain->getTile(static_cast<int>(unit->getX()), static_cast<int>(unit->getY()));
            if (!tile || tile->visible == 0) continue;
        }

        unit->render(renderer, drs, camX, camY, zoom);
    }

    // Render active projectiles (Missile SLP 1034)
    for (const auto& m : m_missiles) {
        if (m.dead) continue;
        Point2D pt = Isometric::worldToScreen(m.currentX, m.currentY, m.currentZ, camX, camY, zoom);
        auto shape = drs.getShape(static_cast<uint32_t>(m.graphicId));
        if (shape && shape->getFrameCount() > 0) {
            float dx = m.targetX - m.startX;
            float dy = m.targetY - m.startY;
            float angle = std::atan2(dy, dx);
            if (angle < 0) angle += 2.0f * 3.14159265f;
            int frameIdx = static_cast<int>((angle / (2.0f * 3.14159265f)) * shape->getFrameCount()) % shape->getFrameCount();
            const auto* frame = shape->getFrame(frameIdx);
            if (frame) {
                int drawX = static_cast<int>(pt.x - frame->hotspotX * zoom);
                int drawY = static_cast<int>(pt.y - frame->hotspotY * zoom);
                renderer.blitSLPFrame(frame->pixelData.data(), frame->width, frame->height, drawX, drawY, m.ownerPlayerId - 1);
            }
        } else {
            renderer.drawRect(static_cast<int>(pt.x - 2), static_cast<int>(pt.y - 2), 4, 4, 255, 230, 150, 255);
        }
    }
}

std::shared_ptr<Unit> Simulation::getUnitById(uint32_t id) const {
    for (const auto& u : m_units) {
        if (u && u->getId() == id) return u;
    }
    return nullptr;
}

std::shared_ptr<Unit> Simulation::spawnFoundation(int civId, int unitTypeId, int playerId, float x, float y) {
    auto bldg = spawnUnit(civId, unitTypeId, playerId, x, y);
    if (bldg) {
        bldg->setFoundation(true);
        bldg->setBuildProgress(0.01f);
    }
    return bldg;
}

void Simulation::spawnMissile(uint32_t sourceId, int playerId, float sx, float sy, float sz,
                              uint32_t targetId, float tx, float ty, float tz, float damage) {
    Missile m;
    m.id = m_nextMissileId++;
    m.ownerPlayerId = playerId;
    m.targetEntityId = targetId;
    m.startX = sx; m.startY = sy; m.startZ = sz;
    m.targetX = tx; m.targetY = ty; m.targetZ = tz;
    m.currentX = sx; m.currentY = sy; m.currentZ = sz;
    m.speed = 8.5f;
    float dx = tx - sx;
    float dy = ty - sy;
    m.totalDist = std::max(0.1f, std::sqrt(dx * dx + dy * dy));
    m.distTraveled = 0.0f;
    m.damage = damage;
    m.graphicId = 1034;
    m.dead = false;
    m_missiles.push_back(m);

    triggerSound("5043.wav"); // Arrow bow shoot
}

void Simulation::selectAt(float worldX, float worldY, bool multiSelect) {
    if (!multiSelect) {
        m_selectedUnits.clear();
    }

    // 1. Check mobile units first (smaller collision radius)
    for (auto& unit : m_units) {
        if (!unit || unit->isDead()) continue;
        if (unit->isBuilding()) continue;

        float dx = unit->getX() - worldX;
        float dy = unit->getY() - worldY;
        if (dx * dx + dy * dy < 2.5f) {
            m_selectedUnits.push_back(unit);
            return;
        }
    }

    // 2. Check buildings (footprint-based collision)
    for (auto& unit : m_units) {
        if (!unit || unit->isDead()) continue;
        if (unit->isBuilding()) {
            float minX = unit->getX();
            float minY = unit->getY();
            float maxX = minX + unit->getWidth();
            float maxY = minY + unit->getHeight();
            if (worldX >= minX - 0.5f && worldX <= maxX + 0.5f &&
                worldY >= minY - 0.5f && worldY <= maxY + 0.5f) {
                m_selectedUnits.push_back(unit);
                return;
            }
        }
    }
}

void Simulation::selectBox(float minWorldX, float minWorldY, float maxWorldX, float maxWorldY, bool multiSelect) {
    if (!multiSelect) {
        m_selectedUnits.clear();
    }
    for (auto& unit : m_units) {
        if (!unit || unit->isDead() || unit->getPlayerId() != 1) continue;
        float ux = unit->getX();
        float uy = unit->getY();
        if (ux >= minWorldX && ux <= maxWorldX && uy >= minWorldY && uy <= maxWorldY) {
            if (std::find(m_selectedUnits.begin(), m_selectedUnits.end(), unit) == m_selectedUnits.end()) {
                m_selectedUnits.push_back(unit);
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
                    vill->commandMove(spawnX + 1.5f, spawnY + 1.5f);
                    triggerSound("5164.wav"); // Villager created chime
                }
                break;
            }
        }
    }
}

void Simulation::trainMilitia(int playerId) {
    int idx = std::clamp(playerId - 1, 0, 7);
    if (m_players[idx].food >= 60 && m_players[idx].gold >= 20 && m_players[idx].population < m_players[idx].maxPopulation) {
        m_players[idx].food -= 60;
        m_players[idx].gold -= 20;
        m_players[idx].population++;

        // Find player's completed Barracks
        for (auto& u : m_units) {
            if (u && !u->isDead() && u->getPlayerId() == playerId && u->getTypeId() == 12 && !u->isFoundation()) {
                float spawnX = u->getX() + 1.5f;
                float spawnY = u->getY() + 1.5f;
                auto militia = spawnUnit(1, 74, playerId, spawnX, spawnY);
                if (militia) {
                    militia->commandMove(spawnX + 1.5f, spawnY + 1.5f);
                    triggerSound("5061.wav"); // Sword chime
                }
                break;
            }
        }
    }
}

void Simulation::issueMoveOrder(float worldX, float worldY) {
    for (size_t i = 0; i < m_selectedUnits.size(); ++i) {
        float ox = (static_cast<float>(i % 3) - 1.0f) * 0.8f;
        float oy = (static_cast<float>(i / 3) - 1.0f) * 0.8f;
        m_selectedUnits[i]->commandMove(worldX + ox, worldY + oy);
    }
    triggerSound("5032.wav"); // Move order affirmative
}

void Simulation::issueAttackOrder(uint32_t targetId) {
    for (auto& unit : m_selectedUnits) {
        unit->commandAttack(targetId);
    }
    triggerSound("5032.wav");
}

void Simulation::issueBuildOrder(uint32_t foundationId) {
    for (auto& unit : m_selectedUnits) {
        if (unit && unit->getTypeId() == 83) { // Villagers can build
            unit->commandBuild(foundationId);
        }
    }
    triggerSound("5003.wav"); // "Builder" affirmative
}

void Simulation::triggerSound(const char* soundFile) {
#ifdef __EMSCRIPTEN__
    EM_ASM({
        if (window.playGameSound) {
            window.playGameSound(UTF8ToString($0));
        }
    }, soundFile);
#endif
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
