import { MAP_SIZE, BUILDING_CONFIGS, UnitState, UnitType, BuildingType } from '../core/Constants';
import { GameState, Building } from '../core/GameState';
import { Isometric, Point } from '../core/Isometric';
import { AudioManager } from '../audio/AudioManager';
import { UIManager } from '../ui/UIManager';
import { Pathfinding } from '../core/Pathfinding';

export class InputHandler {
  public cameraX = 400;
  public cameraY = 200;
  public zoom = 1.0;

  private isDraggingSelection = false;
  private selectionStart: Point = { x: 0, y: 0 };
  private selectionCurrent: Point = { x: 0, y: 0 };

  private isPanning = false;
  private panStart: Point = { x: 0, y: 0 };

  private keysDown: Set<string> = new Set();
  public mousePos: Point = { x: 0, y: 0 };
  public hoveredTile: Point | null = null;

  // Waypoint animation ping
  public pingEffect: { x: number; y: number; color: string; timer: number } | null = null;

  constructor(
    private canvas: HTMLCanvasElement,
    private state: GameState,
    private audio: AudioManager,
    private ui: UIManager
  ) {
    this.setupListeners();
    // Center camera on Player 1 base
    const startIso = Isometric.worldToScreen(12, 12, 0, 0, 0, 1.0);
    this.cameraX = canvas.width / 2 - startIso.x;
    this.cameraY = canvas.height / 2 - startIso.y;
  }

  private setupListeners() {
    window.addEventListener('keydown', (e) => this.keysDown.add(e.key.toLowerCase()));
    window.addEventListener('keyup', (e) => this.keysDown.delete(e.key.toLowerCase()));

    this.canvas.addEventListener('contextmenu', (e) => e.preventDefault());

    this.canvas.addEventListener('mousedown', (e) => this.onMouseDown(e));
    window.addEventListener('mousemove', (e) => this.onMouseMove(e));
    window.addEventListener('mouseup', (e) => this.onMouseUp(e));
    this.canvas.addEventListener('wheel', (e) => this.onWheel(e), { passive: false });
  }

  private onMouseDown(e: MouseEvent) {
    const rect = this.canvas.getBoundingClientRect();
    const sx = e.clientX - rect.left;
    const sy = e.clientY - rect.top;

    if (e.button === 1 || (e.button === 0 && e.altKey)) {
      // Middle click or Alt+LeftClick -> Pan
      this.isPanning = true;
      this.panStart = { x: sx - this.cameraX, y: sy - this.cameraY };
      return;
    }

    if (e.button === 0) {
      // Left Click
      if (this.ui.activeBuildingPlacement) {
        // Place building
        this.placeActiveBuilding();
        return;
      }

      this.isDraggingSelection = true;
      this.selectionStart = { x: sx, y: sy };
      this.selectionCurrent = { x: sx, y: sy };
    } else if (e.button === 2) {
      // Right Click
      if (this.ui.activeBuildingPlacement) {
        // Cancel building placement
        this.ui.activeBuildingPlacement = null;
        this.ui.showBanner('Building placement canceled.');
        return;
      }

      this.handleRightClick(sx, sy);
    }
  }

  private onMouseMove(e: MouseEvent) {
    const rect = this.canvas.getBoundingClientRect();
    const sx = e.clientX - rect.left;
    const sy = e.clientY - rect.top;
    this.mousePos = { x: sx, y: sy };

    if (this.isPanning) {
      this.cameraX = sx - this.panStart.x;
      this.cameraY = sy - this.panStart.y;
      return;
    }

    if (this.isDraggingSelection) {
      this.selectionCurrent = { x: sx, y: sy };
    }

    // Update hovered tile
    const worldPt = Isometric.screenToWorld(sx, sy, this.cameraX, this.cameraY, this.zoom);
    const tx = Math.floor(worldPt.x);
    const ty = Math.floor(worldPt.y);
    if (this.state.isValidTile(tx, ty)) {
      this.hoveredTile = { x: tx, y: ty };
    } else {
      this.hoveredTile = null;
    }
  }

  private onMouseUp(e: MouseEvent) {
    if (e.button === 1) {
      this.isPanning = false;
      return;
    }

    if (e.button === 0 && this.isDraggingSelection) {
      this.isDraggingSelection = false;
      const dist = Math.hypot(
        this.selectionCurrent.x - this.selectionStart.x,
        this.selectionCurrent.y - this.selectionStart.y
      );

      if (dist < 6) {
        // Single Click Selection
        this.handleSingleClick(this.selectionStart.x, this.selectionStart.y);
      } else {
        // Drag Box Selection
        this.handleBoxSelection();
      }
    }
  }

  private onWheel(e: WheelEvent) {
    e.preventDefault();
    const zoomFactor = e.deltaY < 0 ? 1.1 : 0.9;
    const newZoom = Math.max(0.5, Math.min(2.0, this.zoom * zoomFactor));

    // Zoom centered on cursor
    const rect = this.canvas.getBoundingClientRect();
    const mx = e.clientX - rect.left;
    const my = e.clientY - rect.top;

    this.cameraX = mx - (mx - this.cameraX) * (newZoom / this.zoom);
    this.cameraY = my - (my - this.cameraY) * (newZoom / this.zoom);
    this.zoom = newZoom;
  }

  private handleSingleClick(sx: number, sy: number) {
    // 1. Check clicking units first
    let clickedUnitId: number | null = null;
    let minUnitDist = Infinity;

    for (const u of this.state.units.values()) {
      if (u.state === UnitState.DEAD) continue;
      const uTileX = Math.floor(u.x);
      const uTileY = Math.floor(u.y);
      if (u.player !== 1 && (!this.state.isValidTile(uTileX, uTileY) || !this.state.visible[uTileY][uTileX])) continue;

      const elev = this.state.isValidTile(uTileX, uTileY) ? this.state.elevations[uTileY][uTileX] : 0;
      const screenPos = Isometric.worldToScreen(u.x, u.y, elev, this.cameraX, this.cameraY, this.zoom);
      const d = Math.hypot(screenPos.x - sx, screenPos.y - 12 * this.zoom - sy);
      if (d < 24 * this.zoom && d < minUnitDist) {
        minUnitDist = d;
        clickedUnitId = u.id;
      }
    }

    if (clickedUnitId !== null) {
      this.state.selectedUnitIds.clear();
      this.state.selectedUnitIds.add(clickedUnitId);
      this.state.selectedBuildingId = null;
      this.audio.playSelect();
      return;
    }

    // 2. Check clicking buildings
    const worldPt = Isometric.screenToWorld(sx, sy, this.cameraX, this.cameraY, this.zoom);
    for (const b of this.state.buildings.values()) {
      if (
        worldPt.x >= b.x &&
        worldPt.x < b.x + b.sizeX &&
        worldPt.y >= b.y &&
        worldPt.y < b.y + b.sizeY
      ) {
        if (b.player === 1 || this.state.explored[b.y][b.x]) {
          this.state.selectedUnitIds.clear();
          this.state.selectedBuildingId = b.id;
          this.audio.playSelect();
          return;
        }
      }
    }

    // Clear selection
    this.state.selectedUnitIds.clear();
    this.state.selectedBuildingId = null;
  }

  private handleBoxSelection() {
    const minX = Math.min(this.selectionStart.x, this.selectionCurrent.x);
    const maxX = Math.max(this.selectionStart.x, this.selectionCurrent.x);
    const minY = Math.min(this.selectionStart.y, this.selectionCurrent.y);
    const maxY = Math.max(this.selectionStart.y, this.selectionCurrent.y);

    this.state.selectedUnitIds.clear();
    this.state.selectedBuildingId = null;

    for (const u of this.state.units.values()) {
      if (u.player !== 1 || u.state === UnitState.DEAD) continue;
      const uTileX = Math.floor(u.x);
      const uTileY = Math.floor(u.y);
      const elev = this.state.isValidTile(uTileX, uTileY) ? this.state.elevations[uTileY][uTileX] : 0;
      const screenPos = Isometric.worldToScreen(u.x, u.y, elev, this.cameraX, this.cameraY, this.zoom);

      if (
        screenPos.x >= minX &&
        screenPos.x <= maxX &&
        screenPos.y >= minY &&
        screenPos.y <= maxY
      ) {
        this.state.selectedUnitIds.add(u.id);
      }
    }

    if (this.state.selectedUnitIds.size > 0) {
      this.audio.playSelect();
    }
  }

  private handleRightClick(sx: number, sy: number) {
    if (this.state.selectedUnitIds.size === 0) return;

    // 1. Check if clicking on an enemy unit
    let targetEnemy: number | null = null;
    for (const u of this.state.units.values()) {
      if (u.player === 2 && u.state !== UnitState.DEAD) {
        const uTileX = Math.floor(u.x);
        const uTileY = Math.floor(u.y);
        if (!this.state.visible[uTileY][uTileX]) continue;

        const elev = this.state.elevations[uTileY][uTileX];
        const screenPos = Isometric.worldToScreen(u.x, u.y, elev, this.cameraX, this.cameraY, this.zoom);
        if (Math.hypot(screenPos.x - sx, screenPos.y - 12 * this.zoom - sy) < 24 * this.zoom) {
          targetEnemy = u.id;
          break;
        }
      }
    }

    if (targetEnemy !== null) {
      // Order units to attack enemy
      for (const id of this.state.selectedUnitIds) {
        const unit = this.state.units.get(id);
        if (unit && unit.player === 1) {
          unit.targetUnitId = targetEnemy;
          unit.targetBuildingId = null;
          unit.targetResource = null;
        }
      }
      this.triggerPing(sx, sy, '#e74c3c');
      this.audio.playAttack();
      return;
    }

    // 2. Check if clicking on a building
    const worldPt = Isometric.screenToWorld(sx, sy, this.cameraX, this.cameraY, this.zoom);
    let clickedBldg: Building | null = null;
    for (const b of this.state.buildings.values()) {
      if (
        worldPt.x >= b.x &&
        worldPt.x < b.x + b.sizeX &&
        worldPt.y >= b.y &&
        worldPt.y < b.y + b.sizeY
      ) {
        clickedBldg = b;
        break;
      }
    }

    if (clickedBldg) {
      if (clickedBldg.player !== 1) {
        // Enemy building -> Order attack!
        for (const id of this.state.selectedUnitIds) {
          const unit = this.state.units.get(id);
          if (unit && unit.player === 1) {
            unit.targetBuildingId = clickedBldg.id;
            unit.targetUnitId = null;
            unit.targetResource = null;
            unit.targetConstructBuildingId = null;
          }
        }
        this.triggerPing(sx, sy, '#e74c3c');
        this.audio.playAttack();
        return;
      } else {
        // Friendly building
        if (!clickedBldg.isConstructed || clickedBldg.hp < clickedBldg.maxHp) {
          // Construct / Repair
          for (const id of this.state.selectedUnitIds) {
            const unit = this.state.units.get(id);
            if (unit && unit.player === 1 && unit.type === UnitType.VILLAGER) {
              unit.targetConstructBuildingId = clickedBldg.id;
              unit.targetBuildingId = null;
              unit.targetUnitId = null;
              unit.targetResource = null;
              unit.path = Pathfinding.findPath(
                unit.x,
                unit.y,
                clickedBldg.x + clickedBldg.sizeX / 2,
                clickedBldg.y + clickedBldg.sizeY / 2,
                (x, y) => this.state.isTileWalkable(x, y)
              );
            }
          }
          this.triggerPing(sx, sy, '#3498db');
          this.audio.playHammer();
          return;
        } else if (
          clickedBldg.type === BuildingType.TOWN_CENTER ||
          clickedBldg.type === BuildingType.WATCH_TOWER
        ) {
          // Garrison into TC or Watch Tower
          for (const id of this.state.selectedUnitIds) {
            const unit = this.state.units.get(id);
            if (unit && unit.player === 1) {
              unit.targetBuildingId = null;
              unit.targetUnitId = null;
              unit.targetResource = null;
              unit.targetConstructBuildingId = null;
              unit.path = Pathfinding.findPath(
                unit.x,
                unit.y,
                clickedBldg.x + clickedBldg.sizeX / 2,
                clickedBldg.y + clickedBldg.sizeY / 2,
                (x, y) => this.state.isTileWalkable(x, y)
              );
              const targetBldg = clickedBldg;
              const pollGarrison = setInterval(() => {
                if (!unit || unit.state === UnitState.DEAD) {
                  clearInterval(pollGarrison);
                  return;
                }
                if (Math.hypot(targetBldg.x + targetBldg.sizeX / 2 - unit.x, targetBldg.y + targetBldg.sizeY / 2 - unit.y) < targetBldg.sizeX / 2 + 1.2) {
                  this.state.garrisonUnit(unit.id, targetBldg.id);
                  clearInterval(pollGarrison);
                }
              }, 250);
            }
          }
          this.triggerPing(sx, sy, '#f1c40f');
          this.audio.playHorn();
          return;
        }
      }
    }

    const tx = Math.floor(worldPt.x);
    const ty = Math.floor(worldPt.y);

    if (!this.state.isValidTile(tx, ty)) return;

    // Check Resource Node (Tree, Gold, Stone)
    const resNode = this.state.resources.get(ty * MAP_SIZE + tx);
    if (resNode && resNode.amount > 0) {
      for (const id of this.state.selectedUnitIds) {
        const unit = this.state.units.get(id);
        if (unit && unit.player === 1 && unit.type === UnitType.VILLAGER) {
          unit.targetResource = { x: tx, y: ty, type: resNode.type };
          unit.targetUnitId = null;
          unit.targetBuildingId = null;
          unit.targetConstructBuildingId = null;
          unit.path = Pathfinding.findPath(unit.x, unit.y, tx, ty, (x, y) => this.state.isTileWalkable(x, y));
        }
      }
      this.triggerPing(sx, sy, '#f39c12');
      this.audio.playGather();
      return;
    }

    // Move Order to ground
    let idx = 0;
    const selectedList = Array.from(this.state.selectedUnitIds);
    for (const id of selectedList) {
      const unit = this.state.units.get(id);
      if (unit && unit.player === 1) {
        unit.targetUnitId = null;
        unit.targetBuildingId = null;
        unit.targetResource = null;
        unit.targetConstructBuildingId = null;

        // Spread squad slightly so they don't stack on exact same pixel
        const offsetX = ((idx % 3) - 1) * 0.6;
        const offsetY = (Math.floor(idx / 3) - 1) * 0.6;
        idx++;

        const destX = Math.max(0, Math.min(MAP_SIZE - 1, worldPt.x + offsetX));
        const destY = Math.max(0, Math.min(MAP_SIZE - 1, worldPt.y + offsetY));

        unit.path = Pathfinding.findPath(unit.x, unit.y, destX, destY, (x, y) => this.state.isTileWalkable(x, y));
      }
    }

    this.triggerPing(sx, sy, '#2ecc71');
    this.audio.playCommand();
  }

  private placeActiveBuilding() {
    if (!this.ui.activeBuildingPlacement || !this.hoveredTile) return;

    const bType = this.ui.activeBuildingPlacement;
    const cfg = BUILDING_CONFIGS[bType];
    const bx = this.hoveredTile.x;
    const by = this.hoveredTile.y;

    // Check placement validity
    let canPlace = true;
    for (let dy = 0; dy < cfg.sizeY; dy++) {
      for (let dx = 0; dx < cfg.sizeX; dx++) {
        const tx = bx + dx;
        const ty = by + dy;
        if (!this.state.isTileWalkable(tx, ty)) {
          canPlace = false;
          break;
        }
      }
    }

    if (!canPlace) {
      this.ui.showBanner('Cannot place building here (obstructed)!');
      this.audio.playCommand();
      return;
    }

    // Deduct resources
    const p1 = this.state.players.get(1)!;
    p1.resources.wood -= cfg.cost.wood;
    p1.resources.stone -= cfg.cost.stone;

    // Spawn under-construction building
    const newBldg = this.state.spawnBuilding(bType, 1, bx, by, false);

    // Order selected villagers to build it!
    for (const uid of this.state.selectedUnitIds) {
      const u = this.state.units.get(uid);
      if (u && u.type === UnitType.VILLAGER) {
        u.targetConstructBuildingId = newBldg.id;
        u.targetResource = null;
        u.targetUnitId = null;
        u.targetBuildingId = null;
        u.path = Pathfinding.findPath(
          u.x,
          u.y,
          bx + cfg.sizeX / 2,
          by + cfg.sizeY / 2,
          (x, y) => this.state.isTileWalkable(x, y)
        );
      }
    }

    this.ui.activeBuildingPlacement = null;
    this.ui.showBanner(`${cfg.name} placed!`);
    this.audio.playHorn();
  }

  private triggerPing(x: number, y: number, color: string) {
    this.pingEffect = { x, y, color, timer: 15 };
  }

  public update() {
    // Keyboard Pan
    const panSpeed = 12 / this.zoom;
    if (this.keysDown.has('w') || this.keysDown.has('arrowup')) this.cameraY += panSpeed;
    if (this.keysDown.has('s') || this.keysDown.has('arrowdown')) this.cameraY -= panSpeed;
    if (this.keysDown.has('a') || this.keysDown.has('arrowleft')) this.cameraX += panSpeed;
    if (this.keysDown.has('d') || this.keysDown.has('arrowright')) this.cameraX -= panSpeed;

    // Decrement ping effect
    if (this.pingEffect) {
      this.pingEffect.timer--;
      if (this.pingEffect.timer <= 0) this.pingEffect = null;
    }
  }

  public renderOverlay(ctx: CanvasRenderingContext2D) {
    // 1. Draw Drag Selection Box
    if (this.isDraggingSelection) {
      const x = Math.min(this.selectionStart.x, this.selectionCurrent.x);
      const y = Math.min(this.selectionStart.y, this.selectionCurrent.y);
      const w = Math.abs(this.selectionCurrent.x - this.selectionStart.x);
      const h = Math.abs(this.selectionCurrent.y - this.selectionStart.y);

      ctx.strokeStyle = '#00ff00';
      ctx.lineWidth = 1;
      ctx.strokeRect(x, y, w, h);
      ctx.fillStyle = 'rgba(0, 255, 0, 0.12)';
      ctx.fillRect(x, y, w, h);
    }

    // 2. Draw Building Placement Ghost
    if (this.ui.activeBuildingPlacement && this.hoveredTile) {
      const cfg = BUILDING_CONFIGS[this.ui.activeBuildingPlacement];
      const bx = this.hoveredTile.x;
      const by = this.hoveredTile.y;

      // Draw footprint diamonds
      for (let dy = 0; dy < cfg.sizeY; dy++) {
        for (let dx = 0; dx < cfg.sizeX; dx++) {
          const tx = bx + dx;
          const ty = by + dy;
          const walkable = this.state.isTileWalkable(tx, ty);

          const diamond = Isometric.getTileDiamond(tx, ty, 0, this.cameraX, this.cameraY, this.zoom);
          ctx.beginPath();
          ctx.moveTo(diamond[0].x, diamond[0].y);
          ctx.lineTo(diamond[1].x, diamond[1].y);
          ctx.lineTo(diamond[2].x, diamond[2].y);
          ctx.lineTo(diamond[3].x, diamond[3].y);
          ctx.closePath();
          ctx.fillStyle = walkable ? 'rgba(46, 204, 113, 0.4)' : 'rgba(231, 76, 60, 0.4)';
          ctx.fill();
          ctx.strokeStyle = walkable ? '#2ecc71' : '#e74c3c';
          ctx.stroke();
        }
      }
    }

    // 3. Draw Right-Click Waypoint Ping
    if (this.pingEffect) {
      const p = this.pingEffect;
      const radius = (16 - p.timer) * 1.5;
      ctx.beginPath();
      ctx.ellipse(p.x, p.y, radius * 2, radius, 0, 0, Math.PI * 2);
      ctx.strokeStyle = p.color;
      ctx.lineWidth = 2;
      ctx.stroke();
    }
  }
}
