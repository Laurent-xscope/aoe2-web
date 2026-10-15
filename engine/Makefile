CXX ?= clang++
EMCXX ?= em++
CXXFLAGS = -std=c++17 -Wall -Wextra -Iinclude
EMFLAGS = -std=c++17 -O2 -Iinclude -s WASM=1 -s USE_ZLIB=1 \
          -s EXPORTED_FUNCTIONS="['_aoe2_init','_aoe2_tick','_aoe2_render','_aoe2_get_framebuffer','_aoe2_mouse_down','_aoe2_mouse_up','_aoe2_mouse_move','_aoe2_key_down','_aoe2_key_up','_aoe2_resize','_malloc','_free']" \
          -s EXPORTED_RUNTIME_METHODS="['ccall','cwrap','FS','HEAPU8','HEAPU32']" \
          -s FORCE_FILESYSTEM=1 \
          -s ALLOW_MEMORY_GROWTH=1

SRCS = src/DATParser.cpp \
       src/DRSReader.cpp \
       src/Engine.cpp \
       src/MainMenuScreen.cpp \
       src/PanelSystem.cpp \
       src/Renderer.cpp \
       src/ScreenStack.cpp \
       src/Simulation.cpp \
       src/SLPReader.cpp \
       src/Terrain.cpp \
       src/Texture.cpp \
       src/Unit.cpp \
       src/main_wasm.cpp

all: wasm

wasm: $(SRCS)
	$(EMCXX) $(EMFLAGS) $(SRCS) -o ../public/aoe2_engine.js

native: $(SRCS)
	$(CXX) $(CXXFLAGS) -c $(SRCS)
	rm -f *.o

clean:
	rm -f ../public/aoe2_engine.js ../public/aoe2_engine.wasm *.o

.PHONY: all wasm native clean
