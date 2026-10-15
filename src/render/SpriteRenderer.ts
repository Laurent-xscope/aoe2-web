import {
  UnitType,
  UnitState,
  ResourceType,
  BUILDING_CONFIGS
} from '../core/Constants';
import { GameState, Unit, Building, Projectile } from '../core/GameState';
import { Isometric, Point } from '../core/Isometric';

interface RenderItem {
  sortKey: number;
  render: (ctx: CanvasRenderingContext2D) => void;
}

export class SpriteRenderer {
  private customSprites: Map<string, HTMLImageElement> = new Map();

  constructor() {
    this.loadExtractedSprites();
  }

  private loadExtractedSprites() {
    const list = [100, 101, 102, 103, 104, 105, 1006, 1007, 1008, 1009, 1018, 1019, 1020, 1021, 1119, 1122, 1124];
    for (const id of list) {
      const img = new Image();
      img.src = `./assets/sprites/slp_${id}_f0.png`;
      img.onload = () => {
        this.customSprites.set(`slp_${id}`, img);
      };
    }
  }

  public render(
    ctx: CanvasRenderingContext2D,
    state: GameState,
    cameraX: number,
    cameraY: number,
    zoom: number
  ) {
    const items: RenderItem[] = [];

    // 1. Resources (Trees, Gold, Stone)
    for (const res of state.resources.values()) {
      const tileX = res.x;
      const tileY = res.y;
      if (!state.explored[tileY][tileX]) continue;

      const elev = state.elevations[tileY][tileX];
      const screenPos = Isometric.worldToScreen(tileX + 0.5, tileY + 0.5, elev, cameraX, cameraY, zoom);

      items.push({
        sortKey: tileX + tileY + 0.5,
        render: (c) => this.renderResourceNode(c, res.type, screenPos, zoom, state.visible[tileY][tileX]),
      });
    }

    // 2. Buildings
    for (const bldg of state.buildings.values()) {
      const isVisible = this.isAreaVisible(state, bldg.x, bldg.y, bldg.sizeX, bldg.sizeY);
      const isExplored = this.isAreaExplored(state, bldg.x, bldg.y, bldg.sizeX, bldg.sizeY);
      if (!isExplored) continue;

      const centerElev = state.elevations[bldg.y][bldg.x];
      const screenPos = Isometric.worldToScreen(
        bldg.x + bldg.sizeX / 2,
        bldg.y + bldg.sizeY / 2,
        centerElev,
        cameraX,
        cameraY,
        zoom
      );

      const isSelected = state.selectedBuildingId === bldg.id;

      items.push({
        sortKey: bldg.x + bldg.sizeX + bldg.y + bldg.sizeY,
        render: (c) => this.renderBuilding(c, bldg, screenPos, zoom, isVisible, isSelected),
      });
    }

    // 3. Units
    for (const unit of state.units.values()) {
      if (unit.state === UnitState.DEAD || unit.state === UnitState.GARRISONED) continue;

      const uTileX = Math.floor(unit.x);
      const uTileY = Math.floor(unit.y);

      if (unit.player !== 1 && (!state.isValidTile(uTileX, uTileY) || !state.visible[uTileY][uTileX])) {
        continue;
      }

      const elev = state.isValidTile(uTileX, uTileY) ? state.elevations[uTileY][uTileX] : 0;
      const screenPos = Isometric.worldToScreen(unit.x, unit.y, elev, cameraX, cameraY, zoom);
      const isSelected = state.selectedUnitIds.has(unit.id);

      items.push({
        sortKey: unit.x + unit.y,
        render: (c) => this.renderUnit(c, unit, screenPos, zoom, isSelected),
      });
    }

    // Sort back-to-front
    items.sort((a, b) => a.sortKey - b.sortKey);

    for (const item of items) {
      item.render(ctx);
    }

    // 4. Render Projectiles (Arrows flying above ground)
    for (const p of state.projectiles) {
      this.renderProjectile(ctx, p, cameraX, cameraY, zoom);
    }
  }

  private isAreaVisible(state: GameState, x: number, y: number, w: number, h: number): boolean {
    for (let dy = 0; dy < h; dy++) {
      for (let dx = 0; dx < w; dx++) {
        if (state.isValidTile(x + dx, y + dy) && state.visible[y + dy][x + dx]) {
          return true;
        }
      }
    }
    return false;
  }

  private isAreaExplored(state: GameState, x: number, y: number, w: number, h: number): boolean {
    for (let dy = 0; dy < h; dy++) {
      for (let dx = 0; dx < w; dx++) {
        if (state.isValidTile(x + dx, y + dy) && state.explored[y + dy][x + dx]) {
          return true;
        }
      }
    }
    return false;
  }

  private renderResourceNode(
    ctx: CanvasRenderingContext2D,
    type: ResourceType,
    pos: Point,
    zoom: number,
    isVisible: boolean
  ) {
    ctx.save();
    if (!isVisible) ctx.globalAlpha = 0.55;

    if (type === ResourceType.WOOD) {
      ctx.beginPath();
      ctx.ellipse(pos.x + 8 * zoom, pos.y + 4 * zoom, 16 * zoom, 8 * zoom, 0, 0, Math.PI * 2);
      ctx.fillStyle = 'rgba(0,0,0,0.3)';
      ctx.fill();

      ctx.fillStyle = '#5c3a21';
      ctx.fillRect(pos.x - 3 * zoom, pos.y - 20 * zoom, 6 * zoom, 22 * zoom);

      ctx.beginPath();
      ctx.arc(pos.x, pos.y - 32 * zoom, 18 * zoom, 0, Math.PI * 2);
      ctx.fillStyle = '#2d6a2e';
      ctx.fill();

      ctx.beginPath();
      ctx.arc(pos.x - 5 * zoom, pos.y - 38 * zoom, 14 * zoom, 0, Math.PI * 2);
      ctx.fillStyle = '#3a833b';
      ctx.fill();
    } else if (type === ResourceType.GOLD) {
      ctx.beginPath();
      ctx.ellipse(pos.x, pos.y, 14 * zoom, 8 * zoom, 0, 0, Math.PI * 2);
      ctx.fillStyle = '#8a6218';
      ctx.fill();

      ctx.fillStyle = '#ffd700';
      ctx.beginPath();
      ctx.arc(pos.x - 4 * zoom, pos.y - 3 * zoom, 5 * zoom, 0, Math.PI * 2);
      ctx.arc(pos.x + 3 * zoom, pos.y - 5 * zoom, 4 * zoom, 0, Math.PI * 2);
      ctx.arc(pos.x, pos.y + 2 * zoom, 4 * zoom, 0, Math.PI * 2);
      ctx.fill();
    } else if (type === ResourceType.STONE) {
      ctx.beginPath();
      ctx.ellipse(pos.x, pos.y, 14 * zoom, 8 * zoom, 0, 0, Math.PI * 2);
      ctx.fillStyle = '#555';
      ctx.fill();

      ctx.fillStyle = '#999';
      ctx.beginPath();
      ctx.arc(pos.x - 3 * zoom, pos.y - 4 * zoom, 6 * zoom, 0, Math.PI * 2);
      ctx.arc(pos.x + 4 * zoom, pos.y - 2 * zoom, 5 * zoom, 0, Math.PI * 2);
      ctx.fill();
    }
    ctx.restore();
  }

  private renderBuilding(
    ctx: CanvasRenderingContext2D,
    bldg: Building,
    pos: Point,
    zoom: number,
    isVisible: boolean,
    isSelected: boolean
  ) {
    ctx.save();
    if (!isVisible) ctx.globalAlpha = 0.55;

    const playerColor = bldg.player === 1 ? '#1e60d5' : '#d63031';
    const cfg = BUILDING_CONFIGS[bldg.type];

    // Selection ring
    if (isSelected) {
      ctx.strokeStyle = '#2ecc71';
      ctx.lineWidth = 2;
      ctx.beginPath();
      const hw = (bldg.sizeX * 48) * zoom;
      const hh = (bldg.sizeY * 24) * zoom;
      ctx.ellipse(pos.x, pos.y, hw, hh, 0, 0, Math.PI * 2);
      ctx.stroke();
    }

    // Check if custom extracted sprite exists
    const spriteImg = this.customSprites.get(cfg.slpKey);

    if (bldg.isConstructed && spriteImg && spriteImg.complete && spriteImg.naturalWidth > 0) {
      const renderW = spriteImg.width * 0.42 * zoom;
      const renderH = spriteImg.height * 0.42 * zoom;
      ctx.drawImage(spriteImg, pos.x - renderW / 2, pos.y - renderH + 20 * zoom, renderW, renderH);

      // Player banner
      ctx.fillStyle = playerColor;
      ctx.fillRect(pos.x - 2 * zoom, pos.y - renderH + 10 * zoom, 16 * zoom, 8 * zoom);
    } else {
      // Scaffolding or procedural architecture
      const bW = bldg.sizeX * 36 * zoom;
      const bH = bldg.sizeY * 30 * zoom;

      if (!bldg.isConstructed) {
        // Scaffolding timbers
        ctx.fillStyle = '#8e7052';
        ctx.fillRect(pos.x - bW / 2, pos.y - 12 * zoom, bW, 14 * zoom);
        ctx.strokeStyle = '#5c4530';
        ctx.lineWidth = 2 * zoom;
        ctx.beginPath();
        ctx.moveTo(pos.x - bW / 2, pos.y - 12 * zoom);
        ctx.lineTo(pos.x + bW / 2, pos.y - 12 * zoom - bH * 0.5);
        ctx.moveTo(pos.x + bW / 2, pos.y - 12 * zoom);
        ctx.lineTo(pos.x - bW / 2, pos.y - 12 * zoom - bH * 0.5);
        ctx.stroke();
      } else {
        ctx.fillStyle = '#d5c4a1';
        ctx.beginPath();
        ctx.roundRect(pos.x - bW / 2, pos.y - bH, bW, bH, 4 * zoom);
        ctx.fill();
        ctx.strokeStyle = '#5a4a32';
        ctx.lineWidth = 2 * zoom;
        ctx.stroke();

        ctx.fillStyle = '#a04000';
        ctx.beginPath();
        ctx.moveTo(pos.x - bW / 2 - 4 * zoom, pos.y - bH);
        ctx.lineTo(pos.x, pos.y - bH - 24 * zoom);
        ctx.lineTo(pos.x + bW / 2 + 4 * zoom, pos.y - bH);
        ctx.closePath();
        ctx.fill();
        ctx.stroke();

        ctx.fillStyle = playerColor;
        ctx.fillRect(pos.x - 2 * zoom, pos.y - bH - 36 * zoom, 18 * zoom, 10 * zoom);
      }
    }

    // Construction Progress Bar
    if (!bldg.isConstructed) {
      const barW = 60 * zoom;
      const barH = 6 * zoom;
      ctx.fillStyle = '#000';
      ctx.fillRect(pos.x - barW / 2, pos.y - 10 * zoom, barW, barH);
      ctx.fillStyle = '#f39c12';
      ctx.fillRect(pos.x - barW / 2, pos.y - 10 * zoom, (barW * bldg.buildProgress) / 100, barH);
    }

    // Health Bar
    if (isSelected || bldg.hp < bldg.maxHp) {
      const barW = 70 * zoom;
      const barH = 6 * zoom;
      const hpPct = bldg.hp / bldg.maxHp;

      ctx.fillStyle = 'rgba(0,0,0,0.8)';
      ctx.fillRect(pos.x - barW / 2 - 1, pos.y - 65 * zoom - 1, barW + 2, barH + 2);

      ctx.fillStyle = hpPct > 0.5 ? '#2ecc71' : hpPct > 0.2 ? '#f1c40f' : '#e74c3c';
      ctx.fillRect(pos.x - barW / 2, pos.y - 65 * zoom, barW * hpPct, barH);
    }

    ctx.restore();
  }

  private renderUnit(
    ctx: CanvasRenderingContext2D,
    unit: Unit,
    pos: Point,
    zoom: number,
    isSelected: boolean
  ) {
    ctx.save();
    const playerHex = unit.player === 1 ? '#1e60d5' : '#d63031';

    // Selection Circle
    if (isSelected) {
      ctx.beginPath();
      ctx.ellipse(pos.x, pos.y + 2 * zoom, 12 * zoom, 6 * zoom, 0, 0, Math.PI * 2);
      ctx.strokeStyle = '#00ff00';
      ctx.lineWidth = 1.5;
      ctx.stroke();
    }

    // Unit Ground Shadow
    ctx.beginPath();
    ctx.ellipse(pos.x + 2 * zoom, pos.y + 2 * zoom, 9 * zoom, 4.5 * zoom, 0, 0, Math.PI * 2);
    ctx.fillStyle = 'rgba(0,0,0,0.35)';
    ctx.fill();

    // Check custom extracted sprite
    const customImg = this.customSprites.get(unit.type === UnitType.VILLAGER ? 'slp_101' : 'slp_100');
    if (customImg && customImg.complete && customImg.naturalWidth > 0 && (unit.type === UnitType.MILITIA || unit.type === UnitType.MAN_AT_ARMS)) {
      const renderW = customImg.width * 0.4 * zoom;
      const renderH = customImg.height * 0.4 * zoom;
      const flip = unit.direction === 1 || unit.direction === 2 || unit.direction === 3;
      ctx.save();
      ctx.translate(pos.x, pos.y);
      if (flip) ctx.scale(-1, 1);
      ctx.drawImage(customImg, -renderW / 2, -renderH + 4 * zoom, renderW, renderH);
      ctx.restore();
    } else {
      const bob = Math.sin(unit.animFrame * 0.6) * (unit.state === UnitState.MOVING ? 2 * zoom : 0.5 * zoom);

      // Body (Tunic / Armor in Player Color)
      ctx.fillStyle = playerHex;
      ctx.fillRect(pos.x - 4 * zoom, pos.y - 18 * zoom + bob, 8 * zoom, 12 * zoom);

      // Head
      ctx.fillStyle = '#f5cba7';
      ctx.beginPath();
      ctx.arc(pos.x, pos.y - 22 * zoom + bob, 4 * zoom, 0, Math.PI * 2);
      ctx.fill();

      // Helmet / Headwear
      ctx.fillStyle = unit.type === UnitType.MILITIA ? '#7f8c8d' : unit.type === UnitType.KNIGHT ? '#bdc3c7' : '#8e44ad';
      ctx.beginPath();
      ctx.arc(pos.x, pos.y - 24 * zoom + bob, 4.2 * zoom, Math.PI, 0);
      ctx.fill();

      // Weapon / Tool
      if (unit.type === UnitType.MILITIA || unit.type === UnitType.MAN_AT_ARMS) {
        ctx.strokeStyle = '#ecf0f1';
        ctx.lineWidth = 2 * zoom;
        ctx.beginPath();
        ctx.moveTo(pos.x + 4 * zoom, pos.y - 12 * zoom + bob);
        ctx.lineTo(pos.x + 10 * zoom, pos.y - 22 * zoom + bob);
        ctx.stroke();
      } else if (unit.type === UnitType.ARCHER || unit.type === UnitType.CROSSBOWMAN) {
        ctx.strokeStyle = '#8b5a2b';
        ctx.lineWidth = 1.5 * zoom;
        ctx.beginPath();
        ctx.arc(pos.x + 6 * zoom, pos.y - 16 * zoom + bob, 8 * zoom, -Math.PI / 2, Math.PI / 2);
        ctx.stroke();
      } else if (unit.type === UnitType.KNIGHT) {
        // Lance & Shield
        ctx.strokeStyle = '#e67e22';
        ctx.lineWidth = 2.5 * zoom;
        ctx.beginPath();
        ctx.moveTo(pos.x + 4 * zoom, pos.y - 8 * zoom + bob);
        ctx.lineTo(pos.x + 16 * zoom, pos.y - 26 * zoom + bob);
        ctx.stroke();
      } else if (unit.type === UnitType.VILLAGER) {
        ctx.strokeStyle = '#5a381e';
        ctx.lineWidth = 1.5 * zoom;
        ctx.beginPath();
        ctx.moveTo(pos.x + 3 * zoom, pos.y - 10 * zoom + bob);
        ctx.lineTo(pos.x + 8 * zoom, pos.y - 18 * zoom + bob);
        ctx.stroke();

        if (unit.carrying) {
          ctx.fillStyle = unit.carrying.type === ResourceType.GOLD ? '#ffd700' : '#8B5A2B';
          ctx.fillRect(pos.x - 7 * zoom, pos.y - 17 * zoom + bob, 4 * zoom, 6 * zoom);
        }
      }
    }

    // Health Bar
    if (isSelected || unit.hp < unit.maxHp) {
      const barW = 24 * zoom;
      const barH = 3.5 * zoom;
      const hpPct = unit.hp / unit.maxHp;

      ctx.fillStyle = 'rgba(0,0,0,0.8)';
      ctx.fillRect(pos.x - barW / 2 - 1, pos.y - 32 * zoom - 1, barW + 2, barH + 2);

      ctx.fillStyle = hpPct > 0.5 ? '#2ecc71' : hpPct > 0.2 ? '#f1c40f' : '#e74c3c';
      ctx.fillRect(pos.x - barW / 2, pos.y - 32 * zoom, barW * hpPct, barH);
    }

    ctx.restore();
  }

  private renderProjectile(
    ctx: CanvasRenderingContext2D,
    p: Projectile,
    cameraX: number,
    cameraY: number,
    zoom: number
  ) {
    const curElev = p.startElev + (p.targetElev - p.startElev) * p.progress;
    const baseScreen = Isometric.worldToScreen(p.currentX, p.currentY, curElev, cameraX, cameraY, zoom);

    // Apply 3D parabolic height offset
    const screenX = baseScreen.x;
    const screenY = baseScreen.y - p.currentHeight * zoom;

    // Shadow on the ground directly below arrow
    ctx.beginPath();
    ctx.ellipse(baseScreen.x, baseScreen.y, 4 * zoom, 2 * zoom, 0, 0, Math.PI * 2);
    ctx.fillStyle = 'rgba(0,0,0,0.25)';
    ctx.fill();

    // Arrow body
    const dx = p.targetX - p.startX;
    const dy = p.targetY - p.startY;
    const angle = Math.atan2(dy, dx);

    ctx.save();
    ctx.translate(screenX, screenY);
    ctx.rotate(angle);

    ctx.strokeStyle = '#d7ccc8';
    ctx.lineWidth = 1.5 * zoom;
    ctx.beginPath();
    ctx.moveTo(-6 * zoom, 0);
    ctx.lineTo(6 * zoom, 0);
    ctx.stroke();

    // Arrowhead
    ctx.fillStyle = '#616161';
    ctx.beginPath();
    ctx.moveTo(6 * zoom, 0);
    ctx.lineTo(3 * zoom, -2 * zoom);
    ctx.lineTo(3 * zoom, 2 * zoom);
    ctx.closePath();
    ctx.fill();

    ctx.restore();
  }
}
