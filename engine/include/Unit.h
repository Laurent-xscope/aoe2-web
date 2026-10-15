#pragma once

#include <cstdint>
#include <string>
#include "DATParser.h"
#include "Renderer.h"
#include "DRSReader.h"

namespace aoe2 {

class Simulation;

enum class ActionState : uint8_t {
    Idle = 0,
    Moving = 1,
    Attacking = 2,
    Gathering = 3,
    Building = 4,
    Dying = 5,
    Dead = 6
};

class Unit {
public:
    Unit(uint32_t id, const UnitType& type, int playerId, float x, float y);
    ~Unit();

    void update(float dt, Simulation& sim);
    void update(float dt);
    void render(Renderer& renderer, DRSReader& drs, float camX, float camY, float zoom);

    // Commands
    void commandMove(float destX, float destY);
    void commandAttack(uint32_t targetId);
    void commandBuild(uint32_t foundationId);

    // State getters
    uint32_t getId() const { return m_id; }
    int getPlayerId() const { return m_playerId; }
    float getX() const { return m_worldX; }
    float getY() const { return m_worldY; }
    float getHP() const { return m_hp; }
    float getMaxHP() const { return m_maxHp; }
    float getHp() const { return m_hp; }
    float getMaxHp() const { return m_maxHp; }
    float getLineOfSight() const { return m_type.lineOfSight; }
    ActionState getState() const { return m_state; }
    const UnitType& getType() const { return m_type; }
    int getTypeId() const { return m_type.id; }
    uint32_t getTargetEntityId() const { return m_targetEntityId; }

    bool isBuilding() const { return m_type.unitClass == 2; }
    bool isFoundation() const { return m_isFoundation; }
    void setFoundation(bool f) { m_isFoundation = f; }
    float getBuildProgress() const { return m_buildProgress; }
    void setBuildProgress(float p) { m_buildProgress = p; }
    void addBuildProgress(float progress);

    int getWidth() const { return m_type.width > 0 ? m_type.width : 1; }
    int getHeight() const { return m_type.height > 0 ? m_type.height : 1; }
    float getAttackRange() const { return m_type.maxRange > 0.0f ? m_type.maxRange : 1.2f; }
    float getAttackDamage() const { return !m_type.attacks.empty() ? m_type.attacks[0].amount : 4.0f; }

    bool isDead() const { return m_state == ActionState::Dead; }
    void applyDamage(float damage);

private:
    uint32_t m_id{0};
    UnitType m_type;
    int m_playerId{1};

    float m_worldX{0.0f};
    float m_worldY{0.0f};
    float m_targetX{0.0f};
    float m_targetY{0.0f};

    float m_hp{100.0f};
    float m_maxHp{100.0f};

    bool m_isFoundation{false};
    float m_buildProgress{1.0f};
    float m_attackCooldown{0.0f};

    ActionState m_state{ActionState::Idle};
    int m_direction{0}; // 0 - 15 (16 directional angles)
    int m_animFrame{0};
    float m_animTimer{0.0f};
    uint32_t m_targetEntityId{0};
};

} // namespace aoe2
