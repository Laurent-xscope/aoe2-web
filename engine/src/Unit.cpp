#include "Unit.h"
#include "Isometric.h"
#include "Simulation.h"
#include <cmath>
#include <algorithm>

namespace aoe2 {

Unit::Unit(uint32_t id, const UnitType& type, int playerId, float x, float y)
    : m_id(id), m_type(type), m_playerId(playerId), m_worldX(x), m_worldY(y),
      m_targetX(x), m_targetY(y), m_hp(type.hitPoints), m_maxHp(type.hitPoints) {
    if (type.unitClass == 2) { // Building
        m_buildProgress = 1.0f;
    }
}

Unit::~Unit() = default;

void Unit::commandMove(float destX, float destY) {
    m_targetX = destX;
    m_targetY = destY;
    m_targetEntityId = 0;
    m_state = ActionState::Moving;

    // Calculate 16-directional heading angle
    float dx = m_targetX - m_worldX;
    float dy = m_targetY - m_worldY;
    float angle = std::atan2(dy, dx); // [-PI, PI]
    if (angle < 0) angle += 2.0f * 3.14159265f;
    m_direction = static_cast<int>(std::round((angle / (2.0f * 3.14159265f)) * 16.0f)) % 16;
}

void Unit::commandAttack(uint32_t targetId) {
    m_targetEntityId = targetId;
    m_state = ActionState::Attacking;
    m_attackCooldown = 0.2f; // Quick first strike
}

void Unit::commandBuild(uint32_t foundationId) {
    m_targetEntityId = foundationId;
    m_state = ActionState::Building;
}

void Unit::addBuildProgress(float progress) {
    m_buildProgress += progress;
    if (m_buildProgress >= 1.0f) {
        m_buildProgress = 1.0f;
        m_isFoundation = false;
        m_hp = m_maxHp;
    } else {
        m_hp = std::max(1.0f, m_maxHp * m_buildProgress);
    }
}

void Unit::applyDamage(float damage) {
    m_hp -= damage;
    if (m_hp <= 0.0f) {
        m_hp = 0.0f;
        m_state = ActionState::Dying;
        m_animFrame = 0;
    }
}

void Unit::update(float dt) {
    // Default fallback update without simulation pointer
    if (m_state == ActionState::Moving) {
        float dx = m_targetX - m_worldX;
        float dy = m_targetY - m_worldY;
        float dist = std::sqrt(dx * dx + dy * dy);
        float speed = m_type.speed > 0.0f ? m_type.speed * 2.5f : 2.0f;
        float step = speed * dt;

        if (dist <= step) {
            m_worldX = m_targetX;
            m_worldY = m_targetY;
            m_state = ActionState::Idle;
        } else {
            m_worldX += (dx / dist) * step;
            m_worldY += (dy / dist) * step;

            m_animTimer += dt;
            if (m_animTimer >= 0.08f) {
                m_animTimer = 0.0f;
                m_animFrame = (m_animFrame + 1) % 10;
            }
        }
    } else if (m_state == ActionState::Dying) {
        m_animTimer += dt;
        if (m_animTimer >= 0.1f) {
            m_animTimer = 0.0f;
            m_animFrame++;
            if (m_animFrame >= 8) {
                m_state = ActionState::Dead;
            }
        }
    }
}

void Unit::update(float dt, Simulation& sim) {
    if (m_state == ActionState::Moving) {
        float dx = m_targetX - m_worldX;
        float dy = m_targetY - m_worldY;
        float dist = std::sqrt(dx * dx + dy * dy);
        float speed = m_type.speed > 0.0f ? m_type.speed * 2.5f : 2.0f;
        float step = speed * dt;

        if (dist <= step) {
            m_worldX = m_targetX;
            m_worldY = m_targetY;
            m_state = ActionState::Idle;
        } else {
            m_worldX += (dx / dist) * step;
            m_worldY += (dy / dist) * step;

            m_animTimer += dt;
            if (m_animTimer >= 0.08f) {
                m_animTimer = 0.0f;
                m_animFrame = (m_animFrame + 1) % 10;
            }
        }
    } else if (m_state == ActionState::Building) {
        auto foundation = sim.getUnitById(m_targetEntityId);
        if (foundation && foundation->isFoundation()) {
            float fx = foundation->getX() + foundation->getWidth() * 0.5f;
            float fy = foundation->getY() + foundation->getHeight() * 0.5f;
            float dx = fx - m_worldX;
            float dy = fy - m_worldY;
            float dist = std::sqrt(dx * dx + dy * dy);
            float reach = std::max(1.8f, (foundation->getWidth() + foundation->getHeight()) * 0.5f + 0.6f);

            if (dist > reach) {
                // Move towards foundation perimeter
                m_targetX = fx;
                m_targetY = fy;
                float speed = m_type.speed > 0.0f ? m_type.speed * 2.5f : 2.0f;
                float step = speed * dt;
                m_worldX += (dx / dist) * step;
                m_worldY += (dy / dist) * step;

                float angle = std::atan2(dy, dx);
                if (angle < 0) angle += 2.0f * 3.14159265f;
                m_direction = static_cast<int>(std::round((angle / (2.0f * 3.14159265f)) * 16.0f)) % 16;

                m_animTimer += dt;
                if (m_animTimer >= 0.08f) {
                    m_animTimer = 0.0f;
                    m_animFrame = (m_animFrame + 1) % 10;
                }
            } else {
                // In contact range: Hammer foundation
                foundation->addBuildProgress(dt * 0.09f); // ~11 seconds construction
                m_animTimer += dt;
                if (m_animTimer >= 0.1f) {
                    m_animTimer = 0.0f;
                    m_animFrame = (m_animFrame + 1) % 15;
                }

                if (!foundation->isFoundation()) {
                    m_state = ActionState::Idle;
                    sim.triggerSound("5100.wav"); // Building complete trumpet
                }
            }
        } else {
            m_state = ActionState::Idle;
        }
    } else if (m_state == ActionState::Attacking) {
        auto target = sim.getUnitById(m_targetEntityId);
        if (target && !target->isDead()) {
            float tx = target->getX();
            float ty = target->getY();
            float dx = tx - m_worldX;
            float dy = ty - m_worldY;
            float dist = std::sqrt(dx * dx + dy * dy);
            float range = getAttackRange();

            float angle = std::atan2(dy, dx);
            if (angle < 0) angle += 2.0f * 3.14159265f;
            m_direction = static_cast<int>(std::round((angle / (2.0f * 3.14159265f)) * 16.0f)) % 16;

            if (dist > range) {
                // Move towards target
                m_targetX = tx;
                m_targetY = ty;
                float speed = m_type.speed > 0.0f ? m_type.speed * 2.5f : 2.0f;
                float step = speed * dt;
                m_worldX += (dx / dist) * step;
                m_worldY += (dy / dist) * step;

                m_animTimer += dt;
                if (m_animTimer >= 0.08f) {
                    m_animTimer = 0.0f;
                    m_animFrame = (m_animFrame + 1) % 10;
                }
            } else {
                // In range: shoot or strike
                m_attackCooldown -= dt;
                m_animTimer += dt;
                if (m_animTimer >= 0.12f) {
                    m_animTimer = 0.0f;
                    m_animFrame = (m_animFrame + 1) % 8;
                }

                if (m_attackCooldown <= 0.0f) {
                    if (m_type.maxRange > 1.5f) {
                        // Ranged missile (Arrow ID 1034)
                        sim.spawnMissile(m_id, m_playerId, m_worldX, m_worldY, 0.4f,
                                         target->getId(), target->getX(), target->getY(), 0.0f,
                                         getAttackDamage());
                    } else {
                        // Melee attack
                        target->applyDamage(getAttackDamage());
                        sim.triggerSound("5061.wav"); // Sword clank / swing
                    }
                    m_attackCooldown = (m_type.reloadTime > 0.0f ? m_type.reloadTime : 1.5f);
                }
            }
        } else {
            m_state = ActionState::Idle;
        }
    } else if (m_state == ActionState::Dying) {
        m_animTimer += dt;
        if (m_animTimer >= 0.1f) {
            m_animTimer = 0.0f;
            m_animFrame++;
            if (m_animFrame >= 8) {
                m_state = ActionState::Dead;
            }
        }
    }
}

void Unit::render(Renderer& renderer, DRSReader& drs, float camX, float camY, float zoom) {
    if (m_state == ActionState::Dead) return;

    Point2D pt = Isometric::worldToScreen(m_worldX, m_worldY, 0.0f, camX, camY, zoom);

    int graphicId = m_type.standingGraphic;
    if (m_isFoundation && m_type.walkingGraphic != 0) {
        graphicId = m_type.walkingGraphic; // Use 1007/1008 foundation SLP
    } else if (m_state == ActionState::Moving && m_type.walkingGraphic != 0) {
        graphicId = m_type.walkingGraphic;
    } else if (m_state == ActionState::Dying && m_type.dyingGraphic != 0) {
        graphicId = m_type.dyingGraphic;
    }

    auto shape = drs.getShape(static_cast<uint32_t>(graphicId));
    if (shape && shape->getFrameCount() > 0) {
        int frameIdx = 0;
        if (!isBuilding()) {
            frameIdx = (m_direction * 10 + m_animFrame) % shape->getFrameCount();
        }

        const SLPFrame* frame = shape->getFrame(frameIdx);
        if (frame) {
            int drawX, drawY;
            if (frame->hotspotX > 0 && frame->hotspotX <= frame->width && 
                frame->hotspotY > 0 && frame->hotspotY <= frame->height) {
                drawX = static_cast<int>(pt.x - frame->hotspotX * zoom);
                drawY = static_cast<int>(pt.y - frame->hotspotY * zoom);
            } else {
                drawX = static_cast<int>(pt.x - (frame->width / 2.0f) * zoom);
                drawY = static_cast<int>(pt.y - (frame->height * 0.85f) * zoom);
            }
            renderer.blitSLPFrame(frame->pixelData.data(), frame->width, frame->height, 
                                  drawX, drawY, m_playerId - 1);

            // If building foundation, draw construction progress bar above foundation
            if (m_isFoundation) {
                int barW = 48;
                int barH = 5;
                int barX = drawX + (frame->width - barW) / 2;
                int barY = drawY + 10;
                renderer.drawRect(barX - 1, barY - 1, barW + 2, barH + 2, 20, 20, 20, 240);
                int fillW = static_cast<int>(barW * std::clamp(m_buildProgress, 0.0f, 1.0f));
                renderer.drawRect(barX, barY, fillW, barH, 60, 220, 60, 255);
            }
        }
    } else {
        // Fallback marker if shape is not loaded
        renderer.drawRect(static_cast<int>(pt.x - 8), static_cast<int>(pt.y - 16), 16, 24, 220, 50, 50, 255);
    }
}

} // namespace aoe2
