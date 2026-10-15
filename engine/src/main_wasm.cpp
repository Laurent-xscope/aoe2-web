#include "Engine.h"
#include "Renderer.h"
#include <cstdint>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define WASM_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define WASM_EXPORT
#endif

extern "C" {

static aoe2::Engine* s_engineInstance = nullptr;
static aoe2::Renderer s_rendererInstance;

WASM_EXPORT int aoe2_init(int width, int height) {
    if (!s_engineInstance) {
        s_engineInstance = new aoe2::Engine();
    }
    s_rendererInstance.initialize(width, height);
    return s_engineInstance->initialize(width, height) ? 1 : 0;
}

WASM_EXPORT void aoe2_tick(double dt) {
    if (s_engineInstance) {
        s_engineInstance->tick(dt);
    }
}

WASM_EXPORT void aoe2_render() {
    if (s_engineInstance) {
        s_engineInstance->render();
    }
}

WASM_EXPORT const uint32_t* aoe2_get_framebuffer() {
    return s_rendererInstance.getFramebuffer();
}

WASM_EXPORT void aoe2_mouse_down(int button, int x, int y) {
    if (s_engineInstance) {
        s_engineInstance->onMouseDown(button, x, y);
    }
}

WASM_EXPORT void aoe2_mouse_up(int button, int x, int y) {
    if (s_engineInstance) {
        s_engineInstance->onMouseUp(button, x, y);
    }
}

WASM_EXPORT void aoe2_mouse_move(int x, int y) {
    if (s_engineInstance) {
        s_engineInstance->onMouseMove(x, y);
    }
}

WASM_EXPORT void aoe2_key_down(int key) {
    if (s_engineInstance) {
        s_engineInstance->onKeyDown(key);
    }
}

WASM_EXPORT void aoe2_key_up(int key) {
    if (s_engineInstance) {
        s_engineInstance->onKeyUp(key);
    }
}

WASM_EXPORT void aoe2_resize(int width, int height) {
    if (s_engineInstance) {
        s_engineInstance->onResize(width, height);
    }
    s_rendererInstance.resize(width, height);
}

} // extern "C"
