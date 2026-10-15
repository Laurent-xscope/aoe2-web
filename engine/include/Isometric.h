#pragma once

namespace aoe2 {

struct Point2D {
    float x{0.0f};
    float y{0.0f};
};

class Isometric {
public:
    static constexpr float TILE_WIDTH = 96.0f;
    static constexpr float TILE_HEIGHT = 48.0f;
    static constexpr float ELEVATION_STEP = 24.0f;

    static Point2D worldToScreen(float worldX, float worldY, float elevation, float camX, float camY, float zoom = 1.0f) {
        float sx = (worldX - worldY) * (TILE_WIDTH / 2.0f);
        float sy = (worldX + worldY) * (TILE_HEIGHT / 2.0f) - (elevation * ELEVATION_STEP);
        return { (sx * zoom) + camX, (sy * zoom) + camY };
    }

    static Point2D screenToWorld(float screenX, float screenY, float camX, float camY, float zoom = 1.0f) {
        float sx = (screenX - camX) / zoom;
        float sy = (screenY - camY) / zoom;

        float worldX = (sx / (TILE_WIDTH / 2.0f) + sy / (TILE_HEIGHT / 2.0f)) / 2.0f;
        float worldY = (sy / (TILE_HEIGHT / 2.0f) - sx / (TILE_WIDTH / 2.0f)) / 2.0f;

        return { worldX, worldY };
    }
};

} // namespace aoe2
