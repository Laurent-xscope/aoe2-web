#include "Unit.h"
#include "Isometric.h"
#include <cmath>
#include <algorithm>

namespace aoe2 {

Unit::Unit(uint32_t id, const UnitType& type, int playerId, float x, float y)
    : m_id(id), m_type(type), m_playerId(playerId), m_worldX(x), m_worldY(y),
      m_targetX(x), m_targetY(y), m_hp(type.hitPoints), m_maxHp(type.hitPoints) {
}

Unit::~Unit() = default;

void Unit::commandMove(float destX, float destY) {
    m_targetX = destX;
    m_targetY = destY;
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

            // Cycle walking animation frames
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

void Unit::render(Renderer& renderer, DRSReader& drs, float camX, float camY, float zoom) {
    if (m_state == ActionState::Dead) return;

    Point2D pt = Isometric::worldToScreen(m_worldX, m_worldY, 0.0f, camX, camY, zoom);

    int graphicId = m_type.standingGraphic;
    if (m_state == ActionState::Moving && m_type.walkingGraphic != 0) {
        graphicId = m_type.walkingGraphic;
    } else if (m_state == ActionState::Dying && m_type.dyingGraphic != 0) {
        graphicId = m_type.dyingGraphic;
    }

    auto shape = drs.getShape(static_cast<uint32_t>(graphicId));
    if (shape && shape->getFrameCount() > 0) {
        // Calculate frame index from direction and animFrame
        int frameIdx = (m_direction * 10 + m_animFrame) % shape->getFrameCount();
        const SLPFrame* frame = shape->getFrame(frameIdx);
        if (frame) {
            int drawX = static_cast<int>(pt.x - frame->hotspotX * zoom);
            int drawY = static_cast<int>(pt.y - frame->hotspotY * zoom);
            renderer.blitSLPFrame(frame->pixelData.data(), frame->width, frame->height, 
                                  drawX, drawY, m_playerId - 1);
        }
    } else {
        // Fallback marker if shape is not loaded
        renderer.drawRect(static_cast<int>(pt.x - 8), static_cast<int>(pt.y - 16), 16, 24, 220, 50, 50, 255);
    }
}

} // namespace aoe2
