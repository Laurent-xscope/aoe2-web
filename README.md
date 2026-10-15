# Age of Empires II - Web Edition

A real-time strategy (RTS) browser recreation inspired by *Age of Empires II*, running purely on static web standards (TypeScript, HTML5 Canvas, Web Audio API) and hosted live on **GitHub Pages**.

## Live Demo
🎮 Play online: **[https://laurent-xscope.github.io/aoe2-web/](https://laurent-xscope.github.io/aoe2-web/)**

---

## Features

* **Isometric 2.5D Engine:** Authentic 2:1 diamond isometric projection (`96x48` tile dimension) with tile elevation, cliff borders, and depth-sorted rendering.
* **Deterministic RTS Core:** 30 Hz fixed-step tick simulation handling unit states, movement, and combat.
* **Pathfinding:** 8-directional $A^*$ pathfinder with obstacle avoidance and corner-cutting prevention.
* **Classic AoE2 Gameplay:**
  * **Economy:** Villagers gather Wood (Trees), Food, Gold, and Stone and carry resources back to Town Centers.
  * **Base Building:** Place Houses, Barracks, Mills, and Farms on the map grid.
  * **Unit Production:** Train Villagers from the Town Center; train Militia from the Barracks.
  * **Combat:** Melee and ranged unit engagements with health bars and attack cooldowns.
  * **Fog of War:** Dynamic vision system with unexplored black fog, explored shrouded terrain, and active line-of-sight.
  * **Minimap Radar:** Interactive diamond minimap with real-time unit and resource markers, plus click-to-navigate camera controls.
* **Authentic Audio:** Web Audio API sound manager loading classic AoE2 sound effects (`.wav`) with procedural fallback synthesizers.

---

## Controls

* **Left Click:** Select unit / building / confirm building placement.
* **Left Click & Drag:** Box selection for multiple units.
* **Right Click:** Move order (ground) / Gather order (trees, gold, stone) / Attack order (enemy).
* **W / A / S / D or Arrow Keys:** Pan camera.
* **Middle Mouse Drag:** Pan camera.
* **Mouse Wheel:** Zoom in / out.
* **Minimap Click / Drag:** Instantly jump camera to target location.

---

## Development

```bash
# Install dependencies
npm install

# Start local dev server (port 3000)
npm run dev

# Build for production
npm run build
```
